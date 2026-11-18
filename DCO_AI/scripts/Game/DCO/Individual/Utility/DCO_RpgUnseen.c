class DCO_RpgUnseen
{
	static int Rockets(IEntity me)
	{
		BaseWeaponManagerComponent wm = BaseWeaponManagerComponent.Cast(me.FindComponent(BaseWeaponManagerComponent));
		if (!wm)
			return 0;
		InventoryStorageManagerComponent inv = InventoryStorageManagerComponent.Cast(me.FindComponent(InventoryStorageManagerComponent));
		int n;
		array<IEntity> weapons = {};
		wm.GetWeaponsList(weapons);
		foreach (IEntity w : weapons)
		{
			BaseWeaponComponent wc = BaseWeaponComponent.Cast(w.FindComponent(BaseWeaponComponent));
			if (!wc || wc.GetWeaponType() != EWeaponType.WT_ROCKETLAUNCHER)
				continue;
			BaseMagazineComponent mag = wc.GetCurrentMagazine();
			if (mag)
				n += mag.GetAmmoCount();
			if (inv)
				n += inv.GetMagazineCountByWeapon(wc);
		}
		return n;
	}

	static int NearTargets(SCR_AIUtilityComponent u, vector pos, float radius)
	{
		AIAgent agent = u.GetAIAgent();
		if (!agent)
			return 0;
		SCR_AIGroup grp = SCR_AIGroup.Cast(agent.GetParentGroup());
		PerceptionManager pm = GetGame().GetPerceptionManager();
		if (!grp || !pm)
			return 0;
		SCR_AIGroupUtilityComponent gu = grp.GetGroupUtilityComponent();
		if (!gu || !gu.m_Perception)
			return 0;
		float pmNow = pm.GetTime();
		int n;
		foreach (SCR_AITargetInfo t : gu.m_Perception.m_aTargets)
		{
			if (t && pmNow - t.m_fTimestamp <= 20.0 && vector.DistanceXZ(t.m_vWorldPos, pos) <= radius)
				n++;
		}
		return n;
	}

	static float MaxM(SCR_AIUtilityComponent u)
	{
		return DCO_Stress.T(u, "at.max_m", 350.0);
	}

	static bool Fortified(IEntity me, IEntity target)
	{
		if (!me || !target)
			return false;
		vector tp = target.GetOrigin();
		vector dir = me.GetOrigin() - tp;
		dir[1] = 0;
		if (dir.LengthSq() < 1.0)
			return false;
		dir.Normalize();
		IEntity hit;
		float h;
		int prot = DCO_CoverId.Prot(tp, dir, target, hit, h);
		if (prot < 2 || !hit)
			return false;
		bool hard;
		string ent;
		DCO_CoverId.Kind(hit, hard, ent);
		return hard;
	}

	static string Kind(SCR_AIUtilityComponent u, IEntity target, out bool hard)
	{
		hard = false;
		if (!target)
			return "unknown";
		IEntity root = target.GetRootParent();
		if (Vehicle.Cast(root))
		{
			hard = true;
			return "vehicle";
		}
		ChimeraCharacter tc = ChimeraCharacter.Cast(target);
		if (tc && tc.IsInVehicle())
		{
			hard = true;
			return "crew";
		}
		if (DCO_CoverManagerComponent.IsEntityInsideBuilding(target))
		{
			hard = true;
			return "building";
		}
		SCR_AICombatComponent comp = u.m_CombatComponent;
		if (!comp || !u.m_OwnerEntity)
			return "soft";
		float now = GetGame().GetWorld().GetWorldTime();
		if (comp.m_DCOFortTgt != target || now - comp.m_fDCOFortAt_ms > 2000.0)
		{
			comp.m_DCOFortTgt = target;
			comp.m_fDCOFortAt_ms = now;
			comp.m_bDCOFort = Fortified(u.m_OwnerEntity, target);
			if (comp.m_bDCOFort)
				DCO_Perf.Count("spec:at_target_fortified");
		}
		if (comp.m_bDCOFort)
		{
			hard = true;
			return "fortified";
		}
		return "soft";
	}

	protected static bool Gate(SCR_AIUtilityComponent u, IEntity target, vector pos, bool seen, float seenAgo)
	{
		SCR_AICombatComponent comp = u.m_CombatComponent;
		IEntity me = u.m_OwnerEntity;
		if (!comp || !me)
			return false;
		float now = GetGame().GetWorld().GetWorldTime();
		float since = now - comp.m_fDCORpgUnseenAt_ms;
		if (comp.m_fDCORpgUnseenAt_ms >= 0 && since < 15000.0)
			return true;

		string why;
		float d = vector.Distance(me.GetOrigin(), pos);
		bool hard;
		string kind = Kind(u, target, hard);
		int rockets = Rockets(me);
		float minM = 15.0;
		float maxM = 250.0;
		if (seen)
		{
			minM = 10.0;
			maxM = MaxM(u);
		}
		if (d < minM)
			why = "close";
		else if (d > maxM)
			why = "far";
		else if (!seen && comp.m_fDCORpgUnseenAt_ms >= 0 && since < 30000.0)
			why = "cooldown";
		else if (rockets <= 0)
			why = "ammo";
		else if (!hard && rockets <= 1 && DCO_Specialist.ArmorKnown(u))
			why = "ammo_last_soft";
		else if (!hard && NearTargets(u, pos, 10.0) < 2)
			why = "soft";
		string mode = "unseen";
		if (seen)
			mode = "seen";
		if (!why.IsEmpty())
		{
			DCO_Perf.Count("rpg_" + mode + "_block:" + why);
			if (now >= comp.m_fDCOAtGateLogAt_ms && DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO))
			{
				comp.m_fDCOAtGateLogAt_ms = now + 5000.0;
				DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("at_fire_gate unit=%1 tgt=%2 kind=%3 dist=%4 seen_s=%5 why=%6 max_m=%7 rockets=%8", DCO_Log.IdUnit(me), DCO_Log.IdUnit(target), kind, Math.Round(d), seenAgo.ToString(-1, 1), why, Math.Round(maxM), rockets));
			}
			return false;
		}
		comp.m_fDCORpgUnseenAt_ms = now;
		DCO_Perf.Count("rpg_" + mode + "_fire");
		DCO_Perf.Count("spec:at_rocket");
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("spec_at_rocket unit=%1 tgt=%2 dist=%3 hard=%4 rockets=%5 kind=%6 seen=%7", DCO_Log.IdUnit(me), DCO_Log.IdUnit(target), Math.Round(d), DCO_Util.B01(hard), rockets, kind, DCO_Util.B01(seen)));
		return true;
	}

	static bool Allowed(SCR_AIUtilityComponent u, IEntity target, vector pos)
	{
		return Gate(u, target, pos, false, 99.0);
	}

	static bool AllowedSeen(SCR_AIUtilityComponent u, IEntity target, vector pos, float seenAgo)
	{
		return Gate(u, target, pos, seenAgo <= 5.0, seenAgo);
	}
}
