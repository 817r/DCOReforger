class DCO_Statics
{
	protected static ref map<IEntity, AIAgent> s_mPending;
	protected static ref map<IEntity, float> s_mPendingAt;
	protected static ref map<IEntity, AIAgent> s_mManned;

	protected static void InitMaps()
	{
		if (s_mPending)
			return;
		s_mPending = new map<IEntity, AIAgent>();
		s_mPendingAt = new map<IEntity, float>();
		s_mManned = new map<IEntity, AIAgent>();
	}

	static bool KeepGunner(IEntity ch)
	{
		if (!DCO_CtrlLock.StaticGunner(ch))
			return false;
		string why;
		if (DCO_CtrlLock.GunnerMayLeave(ch, why))
		{
			DCO_Perf.Count("static_leave:" + why);
			return false;
		}
		return true;
	}

	protected static void TrackLeave(IEntity ent, TurretCompartmentSlot slot, SCR_AIGroup grp)
	{
		IEntity occ = slot.GetOccupant();
		if (occ)
		{
			AIControlComponent ctrl = AIControlComponent.Cast(occ.FindComponent(AIControlComponent));
			if (ctrl && ctrl.GetAIAgent())
				s_mManned.Set(ent, ctrl.GetAIAgent());
			return;
		}
		AIAgent was;
		if (!s_mManned.Find(ent, was))
			return;
		s_mManned.Remove(ent);
		if (!was || !DCO_Util.IsAlive(was.GetControlledEntity()))
			return;
		string cause = "-";
		SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(was);
		if (ca && ca.m_UtilityComponent && ca.m_UtilityComponent.GetCurrentAction())
			cause = ca.m_UtilityComponent.GetCurrentAction().ClassName();
		DCO_Perf.Count("static_leave");
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("static_leave grp=%1 unit=%2 reason=left cause=%3", grp.GetName(), DCO_Log.IdUnit(was.GetControlledEntity()), cause));
	}

	static bool IsPendingSlot(BaseCompartmentSlot slot)
	{
		if (!slot)
			return false;
		InitMaps();
		float now = GetGame().GetWorld().GetWorldTime();
		if (slot.GetVehicle() && Pending(slot.GetVehicle(), now))
			return true;
		return slot.GetOwner() && Pending(slot.GetOwner().GetRootParent(), now);
	}

	protected static bool Pending(IEntity ent, float now)
	{
		AIAgent a;
		if (!s_mPending.Find(ent, a))
			return false;
		float at = 0;
		s_mPendingAt.Find(ent, at);
		bool getting = false;
		SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
		if (ca && ca.m_UtilityComponent && SCR_AIGetInVehicle.Cast(ca.m_UtilityComponent.GetCurrentAction()))
			getting = true;
		ChimeraCharacter ch;
		if (a)
			ch = ChimeraCharacter.Cast(a.GetControlledEntity());
		if (ch && DCO_Util.IsAlive(ch) && !ch.IsInVehicle() && (now - at < 30000.0 || getting))
			return true;
		s_mPending.Remove(ent);
		s_mPendingAt.Remove(ent);
		return false;
	}
	static void Find(vector center, float radius, notnull array<IEntity> outEnts)
	{
		ChimeraWorld world = ChimeraWorld.CastFrom(GetGame().GetWorld());
		if (!world)
			return;
		TagSystem tags = TagSystem.Cast(world.FindSystem(TagSystem));
		if (tags)
			tags.GetTagsInRange(outEnts, center, radius, ETagCategory.StaticTurret);
	}

	static int CountManned(vector center, float radius, FactionKey fk)
	{
		array<IEntity> ents = {};
		Find(center, radius, ents);
		int n;
		foreach (IEntity ent : ents)
		{
			SCR_AIVehicleUsageComponent usage = SCR_AIVehicleUsageComponent.Cast(ent.FindComponent(SCR_AIVehicleUsageComponent));
			if (!usage || usage.GetVehicleType() != EAIVehicleType.STATIC_WEAPON || !usage.GetTurretCompartmentSlot())
				continue;
			IEntity occ = usage.GetTurretCompartmentSlot().GetOccupant();
			if (occ && DCO_Util.IsAlive(occ) && DCO_Util.FactionKeyOf(occ, false) == fk)
				n++;
		}
		return n;
	}

	static bool HasFree(vector center, float radius, SCR_AIGroup grp)
	{
		array<IEntity> ents = {};
		Find(center, radius, ents);
		foreach (IEntity ent : ents)
		{
			if (FreeSlot(ent, grp))
				return true;
		}
		return false;
	}

	protected static TurretCompartmentSlot FreeSlot(IEntity ent, SCR_AIGroup grp)
	{
		SCR_AIVehicleUsageComponent usage = SCR_AIVehicleUsageComponent.Cast(ent.FindComponent(SCR_AIVehicleUsageComponent));
		if (!usage || usage.GetVehicleType() != EAIVehicleType.STATIC_WEAPON)
			return null;
		if (usage.m_DCOOwnerGroup && usage.m_DCOOwnerGroup != grp)
			return null;
		TurretCompartmentSlot slot = usage.GetTurretCompartmentSlot();
		if (!slot || slot.GetOccupant() || slot.AttachedOccupant() || !slot.IsCompartmentAccessible() || slot.IsReserved())
			return null;
		return slot;
	}

	static void GroupTick(SCR_AIGroupUtilityComponent gu, SCR_AIGroup grp, DCO_GroupUtilityComponent g)
	{
		if (!g || !grp || !gu.m_Mailbox || !DCO_GlobalAIComponent.BehaviorOnFor(grp, DCO_EBehavior.COMBINED_ARMS) || g.IsPlayerGroup() || g.IsInTransport())
			return;
		DCO_EGroupTask t = g.GetTask();
		if (t != DCO_EGroupTask.DEFEND && t != DCO_EGroupTask.GARRISON && gu.DCO_GetGarrisonMode() < 0)
			return;
		bool contact = g.HasState(DCO_EGroupState.IN_CONTACT) || (gu.m_Perception && !gu.m_Perception.m_aTargetClusters.IsEmpty());
		if (!contact && !g.m_bDCOManStatics)
			return;
		IEntity leader = grp.GetLeaderEntity();
		if (!leader)
			return;

		vector center = leader.GetOrigin();
		float radius = 50.0;
		if (!contact && g.GetGroupObjective())
		{
			center = g.GetGroupObjective().GetOwner().GetOrigin();
			radius = g.GetGroupObjective().GetRadius() + 50.0;
		}
		array<IEntity> ents = {};
		Find(center, radius, ents);
		if (ents.IsEmpty())
			return;
		InitMaps();
		float now = GetGame().GetWorld().GetWorldTime();
		array<AIAgent> agents = {};
		grp.GetAgents(agents);
		foreach (IEntity pe, AIAgent pa : s_mPending)
			agents.RemoveItem(pa);
		int maxOrders = Math.Min(2, agents.Count() / 3);
		int ordered;
		foreach (IEntity ent : ents)
		{
			SCR_AIVehicleUsageComponent tu = SCR_AIVehicleUsageComponent.Cast(ent.FindComponent(SCR_AIVehicleUsageComponent));
			if (tu && tu.GetTurretCompartmentSlot())
				TrackLeave(ent, tu.GetTurretCompartmentSlot(), grp);
			if (ordered >= maxOrders)
				continue;
			TurretCompartmentSlot slot = FreeSlot(ent, grp);
			if (!slot)
				continue;
			if (Pending(ent, now))
			{
				DCO_Perf.Count("static_man:skip_pending");
				continue;
			}
			AIAgent pick;
			float bestD = float.MAX;
			foreach (AIAgent a : agents)
			{
				ChimeraCharacter ch = ChimeraCharacter.Cast(a.GetControlledEntity());
				if (!ch || ch == leader || ch.IsInVehicle() || !DCO_Util.IsAlive(ch))
					continue;
				float d = vector.DistanceSqXZ(ch.GetOrigin(), ent.GetOrigin());
				if (d < bestD)
				{
					bestD = d;
					pick = a;
				}
			}
			if (!pick)
				return;
			SCR_AIVehicleUsageComponent usage = SCR_AIVehicleUsageComponent.Cast(ent.FindComponent(SCR_AIVehicleUsageComponent));
			gu.AddUsableVehicle(usage);
			array<BaseCompartmentSlot> alloc = {};
			grp.GetAllocatedCompartments(alloc);
			if (alloc.Contains(slot))
				DCO_Perf.Count("static_man:already_allocated");
			else
				grp.AllocateCompartment(slot);
			SCR_AIMessage_GetIn msg = SCR_AIMessage_GetIn.Create(ent, null, EAICompartmentType.Turret, false, 50, null, null, slot);
			gu.m_Mailbox.RequestBroadcast(msg, pick);
			s_mPending.Set(ent, pick);
			s_mPendingAt.Set(ent, now);
			agents.RemoveItem(pick);
			ordered++;
			DCO_Perf.Count("static_man");
			string why = "contact";
			if (!contact)
				why = "commander";
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("static_man grp=%1 dist=%2 why=%3 pending=0 unit=%4", grp.GetName(), Math.Round(Math.Sqrt(bestD)), why, DCO_Log.IdUnit(pick.GetControlledEntity())));
		}
	}
}
