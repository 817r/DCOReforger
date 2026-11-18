class DCO_SuppressWeapon
{
	protected static ref map<IEntity, float> s_mLogAt;

	protected static bool LogOk(IEntity me, float now)
	{
		if (!s_mLogAt)
			s_mLogAt = new map<IEntity, float>();
		float at;
		if (s_mLogAt.Find(me, at) && now < at)
			return false;
		if (s_mLogAt.Count() > 128)
			s_mLogAt.Clear();
		s_mLogAt.Set(me, now + 5000.0);
		return true;
	}

	static void Adjust(AIAgent owner, SCR_AICombatComponent combat, float dist, inout BaseWeaponComponent weaponComp, inout int muzzleId, inout BaseMagazineComponent magazineComp)
	{
		IEntity me = owner.GetControlledEntity();
		if (!me || !weaponComp)
			return;
		SCR_AIUtilityComponent u = SCR_AIUtilityComponent.Cast(owner.FindComponent(SCR_AIUtilityComponent));
		if (!u)
			return;
		float now = GetGame().GetWorld().GetWorldTime();
		if (u.m_AIInfo && u.m_AIInfo.HasUnitState(EUnitState.IN_VEHICLE))
		{
			VehicleMG(u, me, now, weaponComp, muzzleId, magazineComp);
			return;
		}
		int ugl = DCO_UGLUtility.GetUGLMuzzleIndex(weaponComp);
		if (ugl < 0 || muzzleId != ugl)
			return;
		DCO_GunnerBehavior gb = DCO_GunnerBehavior.Get(u);
		if (gb && gb.DCO_IsLobbing())
		{
			DCO_Perf.Count("supp_weapon:ugl_lob_ok");
			return;
		}
		array<BaseMuzzleComponent> muzzles = {};
		weaponComp.GetMuzzlesList(muzzles);
		for (int i = 0; i < muzzles.Count(); i++)
		{
			if (i == ugl || !muzzles[i])
				continue;
			BaseMagazineComponent mag = muzzles[i].GetMagazine();
			if (!mag || mag.GetAmmoCount() <= 0)
				continue;
			muzzleId = i;
			magazineComp = mag;
			DCO_Perf.Count("supp_weapon:ugl_to_rifle");
			if (LogOk(me, now))
				DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("supp_muzzle_guard unit=%1 from=ugl to=%2 dist=%3 why=no_gl_lob", DCO_Log.IdUnit(me), i, Math.Round(dist)));
			return;
		}
		DCO_Perf.Count("supp_weapon:ugl_kept_no_rifle_mag");
	}

	protected static void VehicleMG(SCR_AIUtilityComponent u, IEntity me, float now, inout BaseWeaponComponent weaponComp, inout int muzzleId, inout BaseMagazineComponent magazineComp)
	{
		if (weaponComp.GetWeaponType() == EWeaponType.WT_MACHINEGUN || DCO_Stress.T(u, "ammo.supp_mode", 1.0) <= 0)
			return;
		CompartmentAccessComponent ca = CompartmentAccessComponent.Cast(me.FindComponent(CompartmentAccessComponent));
		if (!ca || !ca.GetCompartment())
			return;
		BaseCompartmentSlot slot = ca.GetCompartment();
		TurretControllerComponent tc = TurretControllerComponent.Cast(slot.GetController());
		if (!tc)
		{
			TurretCompartmentSlot ts = TurretCompartmentSlot.Cast(slot);
			if (ts)
				tc = ts.GetAttachedTurret();
		}
		if (!tc || !tc.GetWeaponManager())
			return;
		array<IEntity> weapons = {};
		tc.GetWeaponManager().GetWeaponsList(weapons);
		foreach (IEntity w : weapons)
		{
			BaseWeaponComponent wc = BaseWeaponComponent.Cast(w.FindComponent(BaseWeaponComponent));
			if (!wc || wc.GetWeaponType() != EWeaponType.WT_MACHINEGUN)
				continue;
			BaseMagazineComponent mag = wc.GetCurrentMagazine();
			if (!mag || mag.GetAmmoCount() <= 0)
				continue;
			string from = typename.EnumToString(EWeaponType, weaponComp.GetWeaponType());
			weaponComp = wc;
			muzzleId = 0;
			magazineComp = mag;
			DCO_Perf.Count("supp_weapon:veh_mg");
			if (LogOk(me, now))
				DCO_Log.Write(DCO_ELogChannel.VEH, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("supp_veh_mg unit=%1 veh=%2 from=%3 mg_ammo=%4 why=light_suppression", DCO_Log.IdUnit(me), DCO_Log.IdVeh(slot.GetOwner()), from, mag.GetAmmoCount()));
			return;
		}
		DCO_Perf.Count("supp_weapon:veh_no_mg");
	}
}

modded class SCR_AIEvaluateSuppressionWeapon
{
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		if (!m_CombatComponent)
			return ENodeResult.FAIL;

		float distanceToTgt = 100.0;
		GetVariableIn(PORT_DISTANCE, distanceToTgt);

		BaseWeaponComponent weaponComp;
		BaseMagazineComponent magazineComp;
		int muzzleId;

		m_CombatComponent.EvaluateSuppressionWeapon(EAIUnitType.UnitType_Infantry, distanceToTgt, weaponComp, muzzleId, magazineComp);

		if (!weaponComp)
			return ENodeResult.FAIL;

		DCO_SuppressWeapon.Adjust(owner, m_CombatComponent, distanceToTgt, weaponComp, muzzleId, magazineComp);

		SetVariableOut(PORT_WEAPON_COMPONENT, weaponComp);
		SetVariableOut(PORT_MAGAZINE_COMPONENT, magazineComp);
		SetVariableOut(PORT_MUZZLE_ID, muzzleId);

		return ENodeResult.SUCCESS;
	}
}
