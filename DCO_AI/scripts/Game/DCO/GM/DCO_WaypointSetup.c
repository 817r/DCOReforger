modded class SCR_AIWaypoint
{
	protected float m_fDCOSetupUntilS;

	bool DCO_WantsSetupDialog() { return false; }

	string DCO_SetupInfo() { return "type=" + ClassName(); }

	void DCO_HoldForSetup(float seconds)
	{
		m_fDCOSetupUntilS = DCO_Util.NowS() + seconds;
		DCO_Perf.Count("wp:setup_hold");
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", string.Format("wp_setup_hold wp=%1 %2 max_s=%3", DCO_Log.Pos(GetOrigin()), DCO_SetupInfo(), seconds));
	}

	void DCO_ReleaseSetup(string why)
	{
		if (m_fDCOSetupUntilS <= 0)
			return;
		float left = m_fDCOSetupUntilS - DCO_Util.NowS();
		m_fDCOSetupUntilS = 0;
		DCO_Perf.Count("wp:setup_release_" + why);
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", string.Format("wp_setup_release wp=%1 why=%2 left_s=%3 %4", DCO_Log.Pos(GetOrigin()), why, left, DCO_SetupInfo()));
	}

	bool DCO_SetupPending()
	{
		if (m_fDCOSetupUntilS <= 0)
			return false;
		if (DCO_Util.NowS() < m_fDCOSetupUntilS)
			return true;
		DCO_ReleaseSetup("timeout");
		return false;
	}

	static SCR_AIWaypoint DCO_SetupWaypointOf(Managed item)
	{
		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(item);
		if (!editable)
			return null;
		SCR_AIWaypoint wp = SCR_AIWaypoint.Cast(editable.GetOwner());
		if (!wp || !wp.DCO_WantsSetupDialog())
			return null;
		return wp;
	}
}

modded class SCR_PlacingEditorComponent
{
	override void EOnEditorActivate()
	{
		super.EOnEditorActivate();
		GetOnPlaceEntity().Remove(DCO_OnPlacedEntity);
		GetOnPlaceEntity().Insert(DCO_OnPlacedEntity);
	}

	override void EOnEditorDeactivate()
	{
		GetOnPlaceEntity().Remove(DCO_OnPlacedEntity);
		super.EOnEditorDeactivate();
	}

	override protected void OnEntityCreatedServer(array<SCR_EditableEntityComponent> entities)
	{
		super.OnEntityCreatedServer(entities);
		foreach (SCR_EditableEntityComponent e : entities)
		{
			SCR_AIWaypoint wp = SCR_AIWaypoint.DCO_SetupWaypointOf(e);
			if (wp)
				wp.DCO_HoldForSetup(60);
		}
	}

	protected void DCO_OnPlacedEntity(int prefabID, SCR_EditableEntityComponent entity)
	{
		if (!SCR_AIWaypoint.DCO_SetupWaypointOf(entity))
			return;
		GetGame().GetCallqueue().CallLater(DCO_OpenPlacedAttributes, 100, false, entity, 0);
	}

	void DCO_OpenPlacedAttributes(SCR_EditableEntityComponent entity, int attempt)
	{
		SCR_AIWaypoint wp = SCR_AIWaypoint.DCO_SetupWaypointOf(entity);
		if (!wp)
			return;
		SCR_AttributesManagerEditorComponent attributes = SCR_AttributesManagerEditorComponent.Cast(SCR_AttributesManagerEditorComponent.GetInstance(SCR_AttributesManagerEditorComponent));
		if (!attributes)
		{
			DCO_Perf.Count("gm:wp_attr_open_fail");
			DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.WARN, "-", string.Format("gm_wp_attr_open result=no_attribute_manager %1", wp.DCO_SetupInfo()));
			wp.DCO_ReleaseSetup("no_dialog");
			return;
		}

		SCR_StatesEditorComponent states = SCR_StatesEditorComponent.Cast(SCR_StatesEditorComponent.GetInstance(SCR_StatesEditorComponent));
		string busy;
		if (Replication.FindItemId(entity) == -1)
			busy = "no_rpl_id";
		else if (states && states.IsWaiting())
			busy = "editor_waiting";
		else if (states && states.GetState() == EEditorState.PLACING)
			busy = "still_placing";
		else if (GetGame().GetMenuManager().IsAnyDialogOpen())
			busy = "dialog_open";

		if (busy != string.Empty)
		{
			if (attempt < 30)
			{
				GetGame().GetCallqueue().CallLater(DCO_OpenPlacedAttributes, 100, false, entity, attempt + 1);
				return;
			}
			DCO_Perf.Count("gm:wp_attr_open_fail");
			DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.WARN, "-", string.Format("gm_wp_attr_open result=gave_up why=%1 attempts=%2 %3", busy, attempt, wp.DCO_SetupInfo()));
			wp.DCO_ReleaseSetup("no_dialog");
			return;
		}

		attributes.DCO_ExpectPlacedWaypoint(entity, attempt);
		array<Managed> items = {};
		items.Insert(entity);
		if (!attributes.DCO_Open(items))
		{
			attributes.DCO_ExpectPlacedWaypoint(null, attempt);
			DCO_Perf.Count("gm:wp_attr_open_fail");
			DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.WARN, "-", string.Format("gm_wp_attr_open result=dco_open_refused attempt=%1 %2", attempt, wp.DCO_SetupInfo()));
			if (attempt < 30)
				GetGame().GetCallqueue().CallLater(DCO_OpenPlacedAttributes, 200, false, entity, attempt + 2);
			else
				wp.DCO_ReleaseSetup("no_dialog");
			return;
		}
		DCO_Perf.Count("gm:wp_attr_open");
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", string.Format("gm_wp_attr_open result=requested dialog=dco pos=%1 waited_ms=%2 %3", DCO_Log.Pos(wp.GetOrigin()), 100 + attempt * 100, wp.DCO_SetupInfo()));
	}
}

modded class SCR_AttributesManagerEditorComponent
{
	protected SCR_EditableEntityComponent m_DCOPlacedWp;
	protected int m_iDCOPlacedAttempt;
	protected ref array<SCR_AIWaypoint> m_aDCOSetupWps = {};

	override void EOnEditorActivate()
	{
		super.EOnEditorActivate();
		GetOnAttributesRequest().Remove(DCO_OnAttrRequest);
		GetOnAttributesRequest().Insert(DCO_OnAttrRequest);
	}

	protected void DCO_OnAttrRequest(array<Managed> items)
	{
		m_aDCOSetupWps.Clear();
		if (!items)
			return;
		foreach (Managed item : items)
		{
			SCR_AIWaypoint wp = SCR_AIWaypoint.DCO_SetupWaypointOf(item);
			if (wp)
				m_aDCOSetupWps.Insert(wp);
		}
	}

	void DCO_ExpectPlacedWaypoint(SCR_EditableEntityComponent entity, int attempt)
	{
		m_DCOPlacedWp = entity;
		m_iDCOPlacedAttempt = attempt;
		GetOnAttributesStart().Remove(DCO_OnPlacedAttrStart);
		GetOnAttributesStart().Insert(DCO_OnPlacedAttrStart);
	}

	protected void DCO_OnPlacedAttrStart(array<SCR_BaseEditorAttribute> attrs)
	{
		GetOnAttributesStart().Remove(DCO_OnPlacedAttrStart);
		SCR_EditableEntityComponent entity = m_DCOPlacedWp;
		m_DCOPlacedWp = null;
		if (!entity)
			return;

		array<Managed> items = {};
		GetEditedItems(items);
		int attrCount;
		if (attrs)
			attrCount = attrs.Count();
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", string.Format("gm_wp_attr_start items=%1 attrs=%2 attempt=%3", items.Count(), attrCount, m_iDCOPlacedAttempt));

		if (attrCount > 0 && items.Find(entity) != -1)
		{
			DCO_Perf.Count("gm:wp_attr_shown");
			return;
		}

		bool retry = m_iDCOPlacedAttempt < 30;
		DCO_Perf.Count("gm:wp_attr_empty");
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.WARN, "-", string.Format("gm_wp_attr_empty items=%1 attrs=%2 retry=%3", items.Count(), attrCount, retry));
		if (retry)
			m_aDCOSetupWps.Clear();
		CancelEditing();
		if (!retry)
			return;
		SCR_PlacingEditorComponent placing = SCR_PlacingEditorComponent.Cast(SCR_PlacingEditorComponent.GetInstance(SCR_PlacingEditorComponent));
		if (placing)
			GetGame().GetCallqueue().CallLater(placing.DCO_OpenPlacedAttributes, 400, false, entity, m_iDCOPlacedAttempt + 5);
	}

	override void ConfirmEditing()
	{
		super.ConfirmEditing();
		DCO_ReleaseSetupWps("confirm");
	}

	override void CancelEditing()
	{
		super.CancelEditing();
		DCO_ReleaseSetupWps("cancel");
	}

	protected void DCO_ReleaseSetupWps(string why)
	{
		foreach (SCR_AIWaypoint wp : m_aDCOSetupWps)
		{
			if (wp)
				wp.DCO_ReleaseSetup(why);
		}
		m_aDCOSetupWps.Clear();
	}
}

class DCO_AIWaitWaypointSetup : AITaskScripted
{
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		AIGroup group = AIGroup.Cast(owner);
		if (!group)
			return ENodeResult.SUCCESS;
		SCR_AIWaypoint wp = SCR_AIWaypoint.Cast(group.GetCurrentWaypoint());
		if (wp && wp.DCO_SetupPending())
		{
			DCO_Perf.Count("wp:move_wait_setup");
			return ENodeResult.RUNNING;
		}
		return ENodeResult.SUCCESS;
	}

	static override bool VisibleInPalette() { return true; }

	static override string GetOnHoverDescription() { return "DCO: hold the group until the GM confirms the waypoint settings."; }
}
