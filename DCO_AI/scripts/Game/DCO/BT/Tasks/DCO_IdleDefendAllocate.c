class SCR_DCOAIAllocateActionsIdleDefendActivity: AITaskScripted
{
	protected SCR_AIGroup m_groupOwner;
	protected SCR_AIGroupUtilityComponent m_Utility;
	protected SCR_MailboxComponent m_Mailbox;

	static override bool VisibleInPalette() {return true;}

	override void OnInit(AIAgent owner)
	{
		m_groupOwner = SCR_AIGroup.Cast(owner);
		if (!m_groupOwner)
		{
			m_groupOwner = SCR_AIGroup.Cast(owner.GetParentGroup());
			if (!m_groupOwner)
			{
				SCR_AgentMustBeAIGroup(this, owner);
				return;
			}
		}
		m_Utility = SCR_AIGroupUtilityComponent.Cast(m_groupOwner.FindComponent(SCR_AIGroupUtilityComponent));
		if (m_Utility)
			m_Mailbox = m_Utility.m_Mailbox;
	}

	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		if (!m_groupOwner || !m_Mailbox || !m_Utility)
			return ENodeResult.SUCCESS;

		if (!DCO_IdleTurretRules.BlockReason(m_groupOwner).IsEmpty())
			return ENodeResult.SUCCESS;

		array<AIAgent> agents = {};
		m_groupOwner.GetAgents(agents);
		AIAgent leader = m_groupOwner.GetLeaderAgent();
		array<AIAgent> freeAgents = {};
		foreach (AIAgent a : agents)
		{
			if (!a || a == leader || SCR_AICompartmentHandling.IsInCompartment(a))
				continue;
			SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
			if (!ca || !DCO_Util.IsAlive(ca.GetControlledEntity()))
				continue;
			string ctrlLim;
			if (DCO_CtrlLock.Ask(a, DCO_ECtrlReq.TURRET_IDLE, ctrlLim) != DCO_ECtrlAns.ALLOW)
				continue;
			freeAgents.Insert(a);
		}

		array<BaseCompartmentSlot> compartments = {};
		m_groupOwner.GetAllocatedCompartments(compartments);
		for (int i = compartments.Count() - 1; i >= 0; i--)
		{
			TurretCompartmentSlot ts = TurretCompartmentSlot.Cast(compartments[i]);
			if (!ts || ts.GetOccupant())
				compartments.Remove(i);
			else if (DCO_Statics.IsPendingSlot(ts))
			{
				DCO_Perf.Count("idle_turret:skip_static_pending");
				compartments.Remove(i);
			}
		}

		array<AISmartActionComponent> smartActions = {};
		m_groupOwner.GetAllocatedSmartActions(smartActions);

		SCR_AIIdleActivity idleAct = SCR_AIIdleActivity.Cast(m_Utility.GetExecutedAction());
		int saCount;

		while (!freeAgents.IsEmpty() && (!compartments.IsEmpty() || !smartActions.IsEmpty()))
		{
			int j = freeAgents.GetRandomIndex();
			AIAgent who = freeAgents[j];
			freeAgents.Remove(j);
			if (!compartments.IsEmpty())
			{
				if (OccupyTurret(who, compartments[0], idleAct))
				{
					compartments.Remove(0);
					continue;
				}
				compartments.Remove(0);
			}
			if (!smartActions.IsEmpty())
			{
				if (OccupySA(who, smartActions[0], idleAct))
					saCount++;
				smartActions.Remove(0);
			}
		}

		if (saCount > 0 && DCO_Log.On(DCO_ELogChannel.SQUAD, DCO_ELogLevel.DEBUG))
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.DEBUG, DCO_Log.IdGroup(m_groupOwner), string.Format("idle_sa grp=%1 n=%2", DCO_Log.IdGroup(m_groupOwner), saCount));
		return ENodeResult.SUCCESS;
	}

	protected bool OccupyTurret(AIAgent who, BaseCompartmentSlot slot, SCR_AIIdleActivity idleAct)
	{
		IEntity vehicle = slot.GetVehicle();
		if (!vehicle)
			return false;
		SCR_AIVehicleUsageComponent vehicleUsageComp = SCR_AIVehicleUsageComponent.FindOnNearestParent(vehicle, vehicle);
		string kind = DCO_IdleTurretRules.Kind(vehicleUsageComp, m_groupOwner);
		if (kind.IsEmpty())
			return false;
		SCR_AIMessage_GetIn getInMessage = SCR_AIMessage_GetIn.Create(vehicle, null, EAICompartmentType.Turret, false, 0, null, idleAct, slot);
		m_Mailbox.RequestBroadcast(getInMessage, who);
		DCO_Perf.Count("idle_alloc:order_turret");
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(m_groupOwner), string.Format("idle_alloc_order grp=%1 unit=%2 kind=turret level=0 related=%3", DCO_Log.IdGroup(m_groupOwner), DCO_Log.IdUnit(who.GetControlledEntity()), idleAct != null));
		if (kind == "static")
			m_Utility.AddUsableVehicle(vehicleUsageComp);
		if (idleAct)
			idleAct.DCO_AddIdleTurret(who);

		IEntity e = who.GetControlledEntity();
		string veh = "-";
		if (vehicle.GetPrefabData())
			veh = FilePath.StripExtension(FilePath.StripPath(vehicle.GetPrefabData().GetPrefabName()));
		float dist = -1;
		if (e)
			dist = vector.DistanceXZ(e.GetOrigin(), vehicle.GetOrigin());
		DCO_Perf.Count("idle_turret:" + kind);
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(m_groupOwner), string.Format("idle_turret grp=%1 who=%2 kind=%3 veh=%4 dist=%5",
			DCO_Log.IdGroup(m_groupOwner), DCO_Log.IdUnit(e), kind, veh, Math.Round(dist)));
		return true;
	}

	protected bool OccupySA(AIAgent who, AISmartActionComponent smartAction, SCR_AIIdleActivity idleAct)
	{
		SCR_AISmartActionComponent smartActionComponent = SCR_AISmartActionComponent.Cast(smartAction);
		if (!smartActionComponent)
			return false;
		smartActionComponent.ReserveAction(who);
		SCR_AIMessage_PerformAction actionMessage = SCR_AIMessage_PerformAction.Create(null, "", smartActionComponent, false, 0, null, idleAct);
		m_Mailbox.RequestBroadcast(actionMessage, who);
		DCO_Perf.Count("idle_alloc:order_sa");
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.DEBUG, DCO_Log.IdGroup(m_groupOwner), string.Format("idle_alloc_order grp=%1 unit=%2 kind=smart_action level=0 related=%3", DCO_Log.IdGroup(m_groupOwner), DCO_Log.IdUnit(who.GetControlledEntity()), idleAct != null));
		return true;
	}

	static override string GetOnHoverDescription()
	{
		return "DCO idle: gives allocated turrets, then smart actions, to group members (leader excluded) while the group is idle and not in contact.";
	}
}

modded class SCR_AIIdleActivity
{
	protected ref array<AIAgent> m_aDCOIdleTurretAgents;

	void DCO_AddIdleTurret(AIAgent who)
	{
		if (!m_aDCOIdleTurretAgents)
			m_aDCOIdleTurretAgents = {};
		if (!m_aDCOIdleTurretAgents.Contains(who))
			m_aDCOIdleTurretAgents.Insert(who);
	}

	protected void DCO_ReleaseIdleTurrets()
	{
		if (!m_aDCOIdleTurretAgents || m_aDCOIdleTurretAgents.IsEmpty() || !m_Utility)
			return;
		SCR_AIGroup group = SCR_AIGroup.Cast(m_Utility.GetAIAgent());
		if (!group || !group.GetCurrentWaypoint())
			return;
		int n;
		int kept;
		array<AIAgent> keepAgents = {};
		foreach (AIAgent a : m_aDCOIdleTurretAgents)
		{
			if (!a || !a.GetControlledEntity())
				continue;
			IEntity veh = CompartmentAccessComponent.GetVehicleIn(a.GetControlledEntity());
			if (!veh)
				continue;
			if (DCO_Statics.KeepGunner(a.GetControlledEntity()))
			{
				keepAgents.Insert(a);
				kept++;
				DCO_Perf.Count("idle_turret:keep_static_contact");
				continue;
			}
			CompartmentAccessComponent cac = CompartmentAccessComponent.Cast(a.GetControlledEntity().FindComponent(CompartmentAccessComponent));
			if (cac && cac.GetCompartment())
				group.ReleaseCompartment(cac.GetCompartment());
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(group), string.Format("static_leave grp=%1 unit=%2 reason=idle_turret_release", DCO_Log.IdGroup(group), DCO_Log.IdUnit(a.GetControlledEntity())));
			if (m_Utility.m_Mailbox)
				m_Utility.m_Mailbox.RequestBroadcast(SCR_AIMessage_GetOut.Create(veh, new SCR_AIBoardingParameters(), null), a);
			n++;
		}
		m_aDCOIdleTurretAgents.Clear();
		m_aDCOIdleTurretAgents.InsertAll(keepAgents);
		DCO_Perf.Count("idle_turret:release");
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(group), string.Format("idle_turret_release grp=%1 n=%2 kept_contact=%3 reason=waypoint", DCO_Log.IdGroup(group), n, kept));
	}

	override void OnActionDeselected()
	{
		super.OnActionDeselected();
		if (!DCO_Util.Teardown("idle_turret_deselect"))
			DCO_ReleaseIdleTurrets();
	}

	override void OnActionCompleted()
	{
		super.OnActionCompleted();
		if (!DCO_Util.Teardown("idle_turret_completed"))
			DCO_ReleaseIdleTurrets();
	}

	override void OnActionFailed()
	{
		super.OnActionFailed();
		if (!DCO_Util.Teardown("idle_turret_failed"))
			DCO_ReleaseIdleTurrets();
	}
}
