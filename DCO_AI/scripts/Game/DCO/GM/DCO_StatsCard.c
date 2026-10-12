modded class SCR_ChimeraCharacter
{
	[RplProp()]
	protected float m_fDCO_Threat = -1;

	[RplProp()]
	protected int m_iDCO_ThreatState;

	[RplProp()]
	protected float m_fDCO_Morale;

	[RplProp()]
	protected int m_iDCO_MoraleState;

	float DCO_GetThreat()		{ return m_fDCO_Threat; }
	int DCO_GetThreatState()	{ return m_iDCO_ThreatState; }
	float DCO_GetMorale()		{ return m_fDCO_Morale; }
	int DCO_GetMoraleState()	{ return m_iDCO_MoraleState; }

	void DCO_SetStats(float threat, int threatState, float morale, int mState)
	{
		if (Math.AbsFloat(threat - m_fDCO_Threat) < 0.05 && Math.AbsFloat(morale - m_fDCO_Morale) < 0.05 && threatState == m_iDCO_ThreatState && mState == m_iDCO_MoraleState)
			return;
		m_fDCO_Threat = threat;
		m_iDCO_ThreatState = threatState;
		m_fDCO_Morale = morale;
		m_iDCO_MoraleState = mState;
		Replication.BumpMe();
	}
}

modded class SCR_AIGroup
{
	[RplProp()]
	protected float m_fDCO_AvgThreat = -1;

	[RplProp()]
	protected float m_fDCO_AvgMorale;

	[RplProp()]
	protected string m_sDCO_Task;

	[RplProp()]
	protected string m_sDCO_Doing;

	[RplProp()]
	protected vector m_vDCO_Dest;

	string DCO_GetTaskInfo()	{ return m_sDCO_Task; }
	string DCO_GetDoingInfo()	{ return m_sDCO_Doing; }
	vector DCO_GetDest()		{ return m_vDCO_Dest; }

	void DCO_SetTaskInfo(string task, string doing, vector dest)
	{
		if (task == m_sDCO_Task && doing == m_sDCO_Doing && vector.DistanceSqXZ(dest, m_vDCO_Dest) < 4)
			return;
		m_sDCO_Task = task;
		m_sDCO_Doing = doing;
		m_vDCO_Dest = dest;
		Replication.BumpMe();
	}

	float DCO_GetAvgThreat()	{ return m_fDCO_AvgThreat; }
	float DCO_GetAvgMorale()	{ return m_fDCO_AvgMorale; }

	void DCO_SetAvgStats(float threat, float morale)
	{
		if (Math.AbsFloat(threat - m_fDCO_AvgThreat) < 0.05 && Math.AbsFloat(morale - m_fDCO_AvgMorale) < 0.05)
			return;
		m_fDCO_AvgThreat = threat;
		m_fDCO_AvgMorale = morale;
		Replication.BumpMe();
	}

	void DCO_UpdateStats()
	{
		array<AIAgent> agents = {};
		GetAgents(agents);
		float threat;
		float morale;
		int n;
		foreach (AIAgent a : agents)
		{
			SCR_ChimeraCharacter ch = SCR_ChimeraCharacter.Cast(a.GetControlledEntity());
			SCR_AIUtilityComponent util = SCR_AIUtilityComponent.Cast(a.FindComponent(SCR_AIUtilityComponent));
			if (!ch || !util || !util.m_ThreatSystem || !DCO_Util.IsAlive(ch))
				continue;
			float t = util.m_ThreatSystem.GetThreatMeasure();
			float m;
			int ms;
			DCO_AIMoraleSystem mor = util.GetMoraleSystem();
			if (mor)
			{
				m = mor.GetMoraleMeasure();
				ms = mor.GetState();
			}
			ch.DCO_SetStats(t, util.m_ThreatSystem.GetState(), m, ms);
			threat += t;
			morale += m;
			n++;
		}
		if (n == 0)
			return;
		DCO_SetAvgStats(threat / n, morale / n);
	}
}

class DCO_GroupThreatTooltipDetail : DCO_GroupTooltipDetail
{
	void DCO_GroupThreatTooltipDetail()
	{
		m_sDisplayName = "Avg threat / morale";
	}

	override bool InitDetail(SCR_EditableEntityComponent entity, Widget widget)
	{
		SCR_EditableGroupComponent editableGroup = SCR_EditableGroupComponent.Cast(entity);
		if (!editableGroup)
			return false;
		m_Group = editableGroup.GetAIGroupComponent();
		m_Text = TextWidget.Cast(widget);
		if (!m_Group || !m_Text)
			return false;
		m_sShown = string.Empty;
		return true;
	}

	override protected string BuildText()
	{
		if (m_Group.DCO_GetAvgThreat() < 0)
			return "-";
		return m_Group.DCO_GetAvgThreat().ToString(1, 2) + " / " + m_Group.DCO_GetAvgMorale().ToString(1, 2);
	}
}

class DCO_GroupTaskTooltipDetail : DCO_GroupTooltipDetail
{
	protected bool m_bDoing;

	void DCO_GroupTaskTooltipDetail(bool doing)
	{
		m_bDoing = doing;
		if (doing)
			m_sDisplayName = "Doing";
		else
			m_sDisplayName = "Task";
	}

	override protected string BuildText()
	{
		string text = m_Group.DCO_GetTaskInfo();
		if (m_bDoing)
			text = m_Group.DCO_GetDoingInfo();
		if (text.IsEmpty())
			return "-";
		return text;
	}
}

[BaseContainerProps(), SCR_BaseContainerCustomTitleEnum(EEditableEntityState, "m_State")]
modded class SCR_BaseEditableEntityFilter
{
	override void OnFrameBase(float timeSlice)
	{
		super.OnFrameBase(timeSlice);
		if (m_State != EEditableEntityState.SELECTED || !m_Entities)
			return;

		BaseWorld w = GetGame().GetWorld();
		foreach (SCR_EditableEntityComponent entity : m_Entities)
		{
			SCR_EditableGroupComponent editableGroup = SCR_EditableGroupComponent.Cast(entity);
			if (!editableGroup || !editableGroup.GetAIGroupComponent())
				continue;

			vector dest = editableGroup.GetAIGroupComponent().DCO_GetDest();
			vector from;
			if (dest == vector.Zero || !entity.GetPos(from))
				continue;

			from[1] = w.GetSurfaceY(from[0], from[2]) + 2;
			dest[1] = w.GetSurfaceY(dest[0], dest[2]) + 2;
			Shape.CreateArrow(from, dest, 8, ARGB(255, 255, 210, 0), ShapeFlags.ONCE | ShapeFlags.NOZBUFFER);
		}
	}
}

class DCO_CharStatsTooltipDetail : SCR_EntityTooltipDetail
{
	protected SCR_ChimeraCharacter m_Char;
	protected TextWidget m_Text;
	protected string m_sShown;
	protected bool m_bMorale;

	void DCO_CharStatsTooltipDetail(bool morale)
	{
		m_Layout = "{93AF6A0EA7571B84}UI/layouts/Editor/Tooltips/TooltipPrefabs/TooltipPrefab_Text.layout";
		m_bShowLabel = true;
		m_bMorale = morale;
		if (morale)
			m_sDisplayName = "Morale";
		else
			m_sDisplayName = "Threat";
	}

	override bool NeedUpdate()
	{
		return true;
	}

	override bool InitDetail(SCR_EditableEntityComponent entity, Widget widget)
	{
		m_Char = SCR_ChimeraCharacter.Cast(entity.GetOwner());
		m_Text = TextWidget.Cast(widget);
		if (!m_Char || !m_Text || m_Char.DCO_GetThreat() < 0)
			return false;
		m_sShown = string.Empty;
		return true;
	}

	override void UpdateDetail(SCR_EditableEntityComponent entity)
	{
		if (!m_Char || !m_Text)
			return;
		string text;
		if (m_bMorale)
			text = m_Char.DCO_GetMorale().ToString(1, 2) + " " + typename.EnumToString(moraleState, m_Char.DCO_GetMoraleState());
		else
			text = m_Char.DCO_GetThreat().ToString(1, 2) + " " + typename.EnumToString(EAIThreatState, m_Char.DCO_GetThreatState());
		if (text == m_sShown)
			return;
		m_sShown = text;
		m_Text.SetText(text);
	}
}
