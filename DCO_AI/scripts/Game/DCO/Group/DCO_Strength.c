class DCO_StrengthInfo
{
	float m_fStrength;
	bool m_bHasAT;
	bool m_bHasVehicles;
	bool m_bInArmor;
	int m_iCount;

	void Reset()
	{
		m_fStrength = 0;
		m_bHasAT = false;
		m_bHasVehicles = false;
		m_bInArmor = false;
		m_iCount = 0;
	}

	void Add(DCO_StrengthInfo other)
	{
		m_fStrength += other.m_fStrength;
		m_bHasAT = m_bHasAT || other.m_bHasAT;
		m_bHasVehicles = m_bHasVehicles || other.m_bHasVehicles;
		m_bInArmor = m_bInArmor || other.m_bInArmor;
		m_iCount += other.m_iCount;
	}
}

class DCO_Strength
{
	static const float W_MEMBER = 1.0;
	static const float W_MG = 0.5;
	static const float W_AT = 0.5;
	static const float W_AT_VS_VEHICLES = 2.0;
	static const float W_SOFT_ARMED = 2.0;
	static const float W_APC = 5.0;
	static const float W_TANK = 8.0;
	static const float BUILDING_SCALE = 1.5;

	protected static ref array<IEntity> s_aVehicles = {};
	protected static ref array<AIAgent> s_aAgents = {};

	static float VehicleWeight(IEntity vehicle)
	{
		if (!vehicle)
			return 0;

		PerceivableComponent perc = PerceivableComponent.Cast(vehicle.FindComponent(PerceivableComponent));
		if (perc)
		{
			EAIUnitType type = perc.GetUnitType();
			if (type == EAIUnitType.UnitType_VehicleHeavy)
				return W_TANK;
			if (type == EAIUnitType.UnitType_VehicleMedium)
				return W_APC;
		}

		if (HasTurret(vehicle))
			return W_SOFT_ARMED;
		return 0;
	}

	static bool HasTurret(IEntity vehicle)
	{
		SCR_BaseCompartmentManagerComponent mgr = SCR_BaseCompartmentManagerComponent.Cast(vehicle.FindComponent(SCR_BaseCompartmentManagerComponent));
		if (!mgr)
			return false;

		array<BaseCompartmentSlot> slots = {};
		mgr.GetCompartments(slots);
		foreach (BaseCompartmentSlot slot : slots)
		{
			if (TurretCompartmentSlot.Cast(slot))
				return true;
		}
		return false;
	}

	static bool HasWeaponInHands(IEntity ent, EWeaponType type)
	{
		if (!ent)
			return false;

		BaseWeaponManagerComponent wm = BaseWeaponManagerComponent.Cast(ent.FindComponent(BaseWeaponManagerComponent));
		return wm && wm.GetCurrentWeapon() && wm.GetCurrentWeapon().GetWeaponType() == type;
	}

	static void OfGroup(SCR_AIGroup grp, bool enemyHasVehicles, notnull DCO_StrengthInfo outInfo)
	{
		outInfo.Reset();
		if (!grp)
			return;

		s_aAgents.Clear();
		s_aVehicles.Clear();
		grp.GetAgents(s_aAgents);

		float atWeight = W_AT;
		if (enemyHasVehicles)
			atWeight = W_AT_VS_VEHICLES;

		foreach (AIAgent a : s_aAgents)
		{
			SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
			if (!ca)
				continue;

			SCR_AIInfoComponent info = ca.m_InfoComponent;
			if (info && info.HasUnitState(EUnitState.UNCONSCIOUS))
				continue;

			outInfo.m_iCount++;
			outInfo.m_fStrength += W_MEMBER;
			if (info && info.HasRole(EUnitRole.MACHINEGUNNER))
				outInfo.m_fStrength += W_MG;
			if (info && info.HasRole(EUnitRole.AT_SPECIALIST))
			{
				outInfo.m_fStrength += atWeight;
				outInfo.m_bHasAT = true;
			}

			IEntity veh = DCO_VehicleCombat.GetVehicle(a.GetControlledEntity());
			if (veh && !s_aVehicles.Contains(veh))
			{
				s_aVehicles.Insert(veh);
				float w = VehicleWeight(veh);
				outInfo.m_fStrength += w;
				if (w > 0)
					outInfo.m_bHasVehicles = true;
				if (DCO_VehicleCombat.IsArmored(veh))
					outInfo.m_bInArmor = true;
			}
		}

		s_aAgents.Clear();
		s_aVehicles.Clear();
	}

	static void OfCluster(SCR_AIGroupTargetCluster c, notnull DCO_StrengthInfo outInfo)
	{
		outInfo.Reset();
		if (!c)
			return;

		s_aVehicles.Clear();
		foreach (SCR_AITargetInfo t : c.m_aTargets)
		{
			if (!t)
				continue;

			EAITargetInfoCategory cat = t.m_eCategory;
			if (cat != EAITargetInfoCategory.DETECTED && cat != EAITargetInfoCategory.IDENTIFIED && cat != EAITargetInfoCategory.LOST)
				continue;

			outInfo.m_iCount++;
			IEntity ent = t.m_Entity;
			if (ent && Vehicle.Cast(ent))
			{
				AddVehicle(ent, outInfo);
				continue;
			}

			float w = W_MEMBER;
			if (ent)
			{
				if (HasWeaponInHands(ent, EWeaponType.WT_ROCKETLAUNCHER))
				{
					w += W_AT;
					outInfo.m_bHasAT = true;
				}
				else if (HasWeaponInHands(ent, EWeaponType.WT_MACHINEGUN))
				{
					w += W_MG;
				}

				IEntity veh = DCO_VehicleCombat.GetVehicle(ent);
				if (veh)
					AddVehicle(veh, outInfo);
				else if (SCR_CoverManagerComponent.DCO_GetBuildingAt(ent))
					w *= BUILDING_SCALE;
			}
			outInfo.m_fStrength += w;
		}
		s_aVehicles.Clear();
	}

	protected static void AddVehicle(IEntity veh, DCO_StrengthInfo outInfo)
	{
		if (s_aVehicles.Contains(veh))
			return;

		s_aVehicles.Insert(veh);
		float w = VehicleWeight(veh);
		outInfo.m_fStrength += w;
		if (w > 0)
			outInfo.m_bHasVehicles = true;
	}

	static bool IsSuperior(DCO_StrengthInfo mine, DCO_StrengthInfo enemy, float ratio)
	{
		if (enemy.m_iCount <= 0)
			return false;
		if (mine.m_bInArmor && !enemy.m_bHasAT && !enemy.m_bHasVehicles)
			return true;
		return mine.m_fStrength >= ratio * Math.Max(enemy.m_fStrength, 0.5);
	}

	static float Ratio(DCO_StrengthInfo mine, DCO_StrengthInfo enemy)
	{
		if (enemy.m_iCount <= 0)
			return 0;
		if (mine.m_bInArmor && !enemy.m_bHasAT && !enemy.m_bHasVehicles)
			return 99;
		return mine.m_fStrength / Math.Max(enemy.m_fStrength, 0.5);
	}
}
