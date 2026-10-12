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

	static bool Allowed(SCR_AIUtilityComponent u, IEntity target, vector pos)
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
		if (target)
		{
			IEntity root = target.GetRootParent();
			hard = Vehicle.Cast(root) != null;
			ChimeraCharacter tc = ChimeraCharacter.Cast(target);
			if (!hard && tc)
				hard = tc.IsInVehicle() || SCR_CoverManagerComponent.IsEntityInsideBuilding(target);
		}
		int rockets = Rockets(me);
		if (d < 15.0)
			why = "close";
		else if (d > 250.0)
			why = "far";
		else if (comp.m_fDCORpgUnseenAt_ms >= 0 && since < 30000.0)
			why = "cooldown";
		else if (rockets <= 0)
			why = "ammo";
		else if (!hard && rockets <= 1)
			why = "ammo_last_soft";
		else if (!hard && NearTargets(u, pos, 10.0) < 2)
			why = "soft";
		if (!why.IsEmpty())
		{
			DCO_Perf.Count("rpg_unseen_block:" + why);
			return false;
		}
		comp.m_fDCORpgUnseenAt_ms = now;
		DCO_Perf.Count("rpg_unseen_fire");
		DCO_Perf.Count("spec:at_rocket");
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("spec_at_rocket unit=%1 tgt=%2 dist=%3 hard=%4 rockets=%5", DCO_Log.IdUnit(me), DCO_Log.IdUnit(target), Math.Round(d), DCO_Util.B01(hard), rockets));
		return true;
	}
}
