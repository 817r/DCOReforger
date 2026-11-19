class DCO_VehExit
{
	protected static ref map<IEntity, string> s_mCause;
	protected static map<IEntity, string> GetCause()
	{
		if (!s_mCause)
			s_mCause = new map<IEntity, string>();
		return s_mCause;
	}
	protected static ref map<IEntity, float> s_mCauseAt;
	protected static map<IEntity, float> GetCauseAt()
	{
		if (!s_mCauseAt)
			s_mCauseAt = new map<IEntity, float>();
		return s_mCauseAt;
	}
	protected static ref map<IEntity, float> s_mDenyLogAt;
	protected static map<IEntity, float> GetDenyLogAt()
	{
		if (!s_mDenyLogAt)
			s_mDenyLogAt = new map<IEntity, float>();
		return s_mDenyLogAt;
	}
	protected static ref array<HitZone> s_aZones;

	static SCR_AIGetOutVehicle Tag(SCR_AIGetOutVehicle action, string cause)
	{
		if (action)
			action.m_sDCOCause = cause;
		return action;
	}

	static void Note(IEntity ch, string cause)
	{
		if (!ch)
			return;
		if (GetCause().Count() > 256)
			Prune(GetGame().GetWorld().GetWorldTime());
		GetCause().Set(ch, cause);
		GetCauseAt().Set(ch, GetGame().GetWorld().GetWorldTime());
	}

	static void NoteGroup(AIGroup grp, string cause)
	{
		if (!grp)
			return;
		array<AIAgent> agents = {};
		grp.GetAgents(agents);
		foreach (AIAgent agent : agents)
		{
			if (agent)
				Note(agent.GetControlledEntity(), cause);
		}
	}

	protected static string Fresh(IEntity ch, float window_ms)
	{
		float at;
		if (!GetCauseAt().Find(ch, at) || GetGame().GetWorld().GetWorldTime() - at > window_ms)
			return "";
		return GetCause().Get(ch);
	}

	static string Infer(SCR_AIGetOutVehicle action, bool ordered)
	{
		if (!action.m_sDCOCause.IsEmpty())
			return action.m_sDCOCause;
		if (ordered)
			return "order_wp";
		AIActionBase act = action.GetRelatedGroupActivity();
		if (SCR_AIGetOutActivity.Cast(act))
			return "group_getout";
		if (SCR_AIVehicleCombatActivity.Cast(act))
			return "vehicle_combat";
		if (act)
			return "act_" + act.ClassName();
		float prio = action.GetPriority();
		if (Math.AbsFloat(prio - SCR_AIActionBase.PRIORITY_BEHAVIOR_DISMOUNT_TURRET) < 0.5)
			return "turret_dismount";
		if (Math.AbsFloat(prio - SCR_AIActionBase.PRIORITY_BEHAVIOR_GET_OUT_VEHICLE_HIGH_PRIORITY) < 0.5)
			return "high_untagged";
		if (Math.AbsFloat(prio - SCR_AIActionBase.PRIORITY_BEHAVIOR_GET_OUT_VEHICLE) < 0.5)
			return "msg_dismount";
		return "untagged";
	}

	static bool OnGate(SCR_AIGetOutVehicle action, SCR_AIUtilityComponent u, IEntity veh, bool ordered, bool allowed)
	{
		if (!action || !u || !u.m_OwnerEntity)
			return allowed;
		IEntity inVeh = DCO_VehicleCombat.GetVehicle(u.m_OwnerEntity);
		if (inVeh)
			veh = inVeh;
		if (!Vehicle.Cast(veh))
			return allowed;
		string cause = Infer(action, ordered);
		if (cause == "order_wp")
		{
			string noted = Fresh(u.m_OwnerEntity, 60000.0);
			if (!noted.IsEmpty())
				cause = noted;
		}
		action.m_sDCOCause = cause;
		string verdict = "deny";
		if (allowed && DCO_VehBrain.KeepCrew(u, veh, cause))
		{
			allowed = false;
			verdict = "keep";
		}
		else if (allowed)
			verdict = "allow";
		DCO_Perf.Count("veh_exit:" + cause + ":" + verdict);
		float now = GetGame().GetWorld().GetWorldTime();
		if (allowed)
			Note(u.m_OwnerEntity, cause);
		else
		{
			float next;
			if (GetDenyLogAt().Find(u.m_OwnerEntity, next) && now < next)
				return allowed;
			GetDenyLogAt().Set(u.m_OwnerEntity, now + 10000.0);
		}
		string seat = Seat(u.m_OwnerEntity);
		DCO_VehDbg.NoteExit(veh, string.Format("%1 %2 (%3)", seat, cause, verdict));
		if (!DCO_Log.On(DCO_ELogChannel.VEH, DCO_ELogLevel.INFO))
			return allowed;
		DCO_Log.Write(DCO_ELogChannel.VEH, DCO_ELogLevel.INFO, DCO_Log.IdUnit(u.m_OwnerEntity), string.Format("veh_exit unit=%1 veh=%2 cause=%3 verdict=%4 seat=%5 prio=%6 %7",
			DCO_Log.IdUnit(u.m_OwnerEntity), DCO_Log.IdVeh(veh), cause, verdict, seat, Math.Round(action.GetPriority()), Damage(veh)));
		return allowed;
	}

	static void OnLeft(IEntity ch, IEntity veh)
	{
		if (!ch || !veh || !Replication.IsServer() || !Vehicle.Cast(veh.GetRootParent()))
			return;
		PlayerManager pm = GetGame().GetPlayerManager();
		if (pm && pm.GetPlayerIdFromControlledEntity(ch) > 0)
			return;
		float now = GetGame().GetWorld().GetWorldTime();
		string cause = "untagged";
		float at;
		if (!DCO_Util.IsAlive(ch))
			cause = "dead";
		else if (GetCauseAt().Find(ch, at) && now - at < 30000.0)
			cause = GetCause().Get(ch);
		GetCause().Remove(ch);
		GetCauseAt().Remove(ch);
		GetDenyLogAt().Remove(ch);
		DCO_Perf.Count("veh_left:" + cause);
		IEntity root = veh.GetRootParent();
		DCO_VehDbg.NoteExit(root, "left: " + cause);
		if (!DCO_Log.On(DCO_ELogChannel.VEH, DCO_ELogLevel.INFO))
			return;
		DCO_Log.Write(DCO_ELogChannel.VEH, DCO_ELogLevel.INFO, DCO_Log.IdUnit(ch), string.Format("veh_left unit=%1 veh=%2 cause=%3 %4",
			DCO_Log.IdUnit(ch), DCO_Log.IdVeh(root), cause, Damage(root)));
	}

	static string Seat(IEntity ch)
	{
		ChimeraCharacter cc = ChimeraCharacter.Cast(ch);
		if (!cc || !cc.GetCompartmentAccessComponent() || !cc.GetCompartmentAccessComponent().GetCompartment())
			return "-";
		BaseCompartmentSlot slot = cc.GetCompartmentAccessComponent().GetCompartment();
		if (PilotCompartmentSlot.Cast(slot))
			return "driver";
		if (TurretCompartmentSlot.Cast(slot))
			return "gunner";
		return "cargo";
	}

	static string Damage(IEntity veh)
	{
		if (!veh)
			return "dmg=-";
		SCR_VehicleDamageManagerComponent dmg = SCR_VehicleDamageManagerComponent.Cast(veh.FindComponent(SCR_VehicleDamageManagerComponent));
		if (!dmg)
			return "dmg=-";
		if (!s_aZones)
			s_aZones = {};
		s_aZones.Clear();
		dmg.GetAllHitZonesInHierarchy(s_aZones);
		int wheels;
		int wheelsDead;
		foreach (HitZone hz : s_aZones)
		{
			if (!SCR_WheelHitZone.Cast(hz))
				continue;
			wheels++;
			if (hz.GetDamageState() == EDamageState.DESTROYED)
				wheelsDead++;
		}
		s_aZones.Clear();
		return string.Format("hp=%1 engine=%2 move_dmg=%3 wheels_dead=%4/%5 fire=%6 cls=%7",
			dmg.GetHealthScaled().ToString(1, 2), DCO_Util.B01(dmg.GetEngineFunctional()), dmg.GetMovementDamage().ToString(1, 2), wheelsDead, wheels,
			DCO_Util.B01(dmg.IsOnFire()), DCO_VehicleActive.ClassName(DCO_VehicleActive.VehClass(veh)));
	}

	protected static void Prune(float now)
	{
		array<IEntity> stale = {};
		foreach (IEntity c, float at : GetCauseAt())
		{
			if (!c || now - at > 30000.0)
				stale.Insert(c);
		}
		foreach (IEntity sc : stale)
		{
			GetCause().Remove(sc);
			GetCauseAt().Remove(sc);
			GetDenyLogAt().Remove(sc);
		}
	}
}

modded class SCR_CompartmentAccessComponent
{
	override protected void OnCompartmentLeft(IEntity targetEntity, BaseCompartmentManagerComponent manager, int mgrID, int slotID, bool move)
	{
		super.OnCompartmentLeft(targetEntity, manager, mgrID, slotID, move);
		if (!move)
			DCO_VehExit.OnLeft(GetOwner(), targetEntity);
	}
}
