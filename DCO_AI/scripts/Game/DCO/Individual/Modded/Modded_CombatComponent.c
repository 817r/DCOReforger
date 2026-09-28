modded class SCR_AICombatComponent
{
	static const int			 TARGET_ENDANGERED_TIMEOUT_S = 12;
	static const float			 ENDANGERING_TARGET_SCORE_MULTIPLIER = 1.1;

	static const float AIM_IMPROVEMENT_INCREASE = 0.001;
	static const float AIM_IMPROVEMENT_DECREASE = 0.005;
	static const float AIM_MORALE = 0.00001;
	static const float AIM_IMPROVEMENT_CONST_DECREASE = AIM_IMPROVEMENT_DECREASE;
	static const float AIM_IMPROVEMENT_CONST_DECREASE_SUPPRESSED_MULTIPLIER = 2;
	static const float AIM_IMPROVEMENT_TARGET = 0.3;
	static const float AIM_IMPROVEMENT_MIN = 0.05;

	static const float SCOPE_PERCEPTION_GAIN = 0.15;
	static const float SCOPE_TUNNEL_HALF_ANGLE = 20.0;
	static const float SCOPE_TUNNEL_DELAY_PER_MAG = 1.0;

	          static const float TARGET_MAX_LAST_SEEN = 11.0;
	protected static const float TARGET_MIN_INDIRECT_TRACE_FRACTION_MIN = 0.5;
	protected static const float TARGET_MAX_DISTANCE_VEHICLE = 800.0;
	protected static const float TARGET_MAX_DISTANCE_DISARMED = 0.2;
	protected static const float TARGET_MAX_TIME_SINCE_ENDANGERED = 5.0;
	protected static const float TARGET_SCORE_RETREAT = 75.0;
	static const float TARGET_SCORE_HIGH_PRIORITY_ATTACK = 98.5;

	protected const float PERCEPTION_FACTOR_SAFE = 1.2;
	protected const float PERCEPTION_FACTOR_VIGILANT = 4.2;
	protected const float PERCEPTION_FACTOR_ALERTED = 3.2;
	protected const float PERCEPTION_FACTOR_THREATENED = 1.7;

	float CURRENT_AIM_IMPROVEMENT;

	bool ChangeTarget = false;

	protected IEntity ownerEntity;

	const float m_fStartAccuracy = 1;
	protected float m_fTargetAccuracy;
	float m_fTimeElapsed = 0.0;
	protected float m_fDurationN;

	protected float slicedTime;

	protected DCO_GroupTactics m_eAICurrentTactics;

	protected float m_fDCOEngageInfantry = -1;
	protected float m_fDCOEngageVehicle = -1;
	protected bool m_bDCOCloseCombat;
	protected AIGroup m_DCOEngageGroup;
	protected DCO_GroupConfigComponent m_DCOEngageConfig;

	protected void DCO_ResolveEngagement(out float infantry, out float vehicle)
	{
		infantry = TARGET_MAX_DISTANCE_INFANTRY;
		vehicle = TARGET_MAX_DISTANCE_VEHICLE;

		SCR_ChimeraAIAgent agent = GetAiAgent();
		if (!agent)
			return;

		AIGroup group = agent.GetParentGroup();
		if (group != m_DCOEngageGroup)
		{
			m_DCOEngageGroup = group;
			m_DCOEngageConfig = null;
			if (group)
				m_DCOEngageConfig = DCO_GroupConfigComponent.Cast(group.FindComponent(DCO_GroupConfigComponent));
		}

		if (!m_DCOEngageConfig)
			return;

		infantry = m_DCOEngageConfig.GetEngagementDistanceInfantry();
		vehicle = m_DCOEngageConfig.GetEngagementDistanceVehicle();
	}

	override void SetTargetSelectionProperties(bool closeCombat)
	{
		m_bDCOCloseCombat = closeCombat;
		DCO_ResolveEngagement(m_fDCOEngageInfantry, m_fDCOEngageVehicle);

		if (closeCombat)
		{
			m_WeaponTargetSelector.SetSelectionProperties(TARGET_MAX_LAST_SEEN_DIRECT_ATTACK_CLOSE, TARGET_MAX_LAST_SEEN_INDIRECT_ATTACK_CLOSE, TARGET_MAX_LAST_SEEN_INDIRECT_ATTACK_CLOSE,
				TARGET_MIN_INDIRECT_TRACE_FRACTION_MIN, m_fDCOEngageInfantry, m_fDCOEngageVehicle, TARGET_MAX_TIME_SINCE_ENDANGERED, TARGET_MAX_DISTANCE_DISARMED);
			return;
		}

		m_WeaponTargetSelector.SetSelectionProperties(TARGET_MAX_LAST_SEEN_DIRECT_ATTACK, TARGET_MAX_LAST_SEEN_INDIRECT_ATTACK, TARGET_MAX_LAST_SEEN,
			TARGET_MIN_INDIRECT_TRACE_FRACTION_MIN, m_fDCOEngageInfantry, m_fDCOEngageVehicle, TARGET_MAX_TIME_SINCE_ENDANGERED, TARGET_MAX_DISTANCE_DISARMED);
	}

	void DecreaseAim()
	{
		m_fTimeElapsed -= slicedTime * 1.2;
    	if (m_fTimeElapsed < 0)
			m_fTimeElapsed = 0;
	}

	void ChangeTargetCompensation()
	{
		float penalty = 3.0;
		if (DCO_CQC.IsCQC(m_Utility))
			penalty *= DCO_CQC.TARGET_SWITCH_SCALE;
		m_fTimeElapsed -= slicedTime * penalty;
    	if (m_fTimeElapsed < 0)
			m_fTimeElapsed = 0;
	}

	void DangerSuppressedDecreaseAIM(float dec)
	{
		m_fTimeElapsed -= slicedTime * 1.5 * dec;
    	if (m_fTimeElapsed < 0)
			m_fTimeElapsed = 0;
	}

	void MoraleDropAIM(float val)
	{
		m_fTimeElapsed -= slicedTime * 1.2 * val;
    	if (m_fTimeElapsed < 0)
			m_fTimeElapsed = 0;
	}

	protected float m_fDCOLean;
	protected float m_fDCOLeanNextEval_ms;
	protected float m_fDCOLeanHoldUntil_ms;

	protected void DCO_UpdateLean()
	{
		ChimeraCharacter ch = ChimeraCharacter.Cast(ownerEntity);
		if (!ch || !m_CharacterController)
			return;

		float now = GetGame().GetWorld().GetWorldTime();
		if (now < m_fDCOLeanNextEval_ms)
			return;
		m_fDCOLeanNextEval_ms = now + DCO_CQC.LEAN_EVAL_MS;

		float wanted = 0;
		bool canLean = m_SelectedTarget && m_SelectedTarget.GetTargetEntity() && !ch.IsInVehicle()
			&& m_SelectedTarget.GetTimeSinceSeen() < DCO_CQC.LEAN_TARGET_SEEN_S
			&& m_CharacterController.GetStance() != ECharacterStance.PRONE
			&& (!ch.GetPhysics() || ch.GetPhysics().GetVelocity().Length() < DCO_CQC.LEAN_MAX_SPEED);

		if (canLean)
		{
			vector targetPos = m_SelectedTarget.GetLastSeenPosition() + vector.Up * DCO_CQC.LEAN_AIM_HEIGHT;
			if (vector.Distance(ch.GetOrigin(), targetPos) < DCO_CQC.LEAN_MAX_DIST)
				wanted = DCO_CQC.ResolveLean(ch, m_SelectedTarget.GetTargetEntity(), targetPos, m_fDCOLean);

			if (wanted == 0 && m_fDCOLean != 0 && now < m_fDCOLeanHoldUntil_ms)
				return;
		}

		if (wanted == m_fDCOLean)
		{
			if (wanted != 0)
				m_fDCOLeanHoldUntil_ms = now + DCO_CQC.LEAN_HOLD_MS;
			return;
		}

		m_fDCOLean = wanted;
		m_fDCOLeanHoldUntil_ms = now + DCO_CQC.LEAN_HOLD_MS;
		m_CharacterController.SetWantedLeaning(wanted);
	}

	float DCO_GetLean()
	{
		return m_fDCOLean;
	}

	override void Update(float timeSliceMs)
	{
		super.Update(timeSliceMs);
		slicedTime = timeSliceMs;
		DCO_UpdateLean();
		if (m_SelectedTarget)
		{
			if (ChangeTarget)
				ChangeTargetCompensation();
			else
				UpdateAccuracy(timeSliceMs);
		} else
		{
			DecreaseAim();
		}
	}

	float GetCurrentAimImprovement()
	{
		return CURRENT_AIM_IMPROVEMENT;
	}

	SCR_AIUtilityComponent GetUtilityComponent()
	{
		return m_Utility;
	}

	override void EvaluateWeaponAndTarget(out bool outWeaponEvent, out bool outSelectedTargetChanged,
		out BaseTarget outPrevTarget, out BaseTarget outCurrentTarget,
		out bool outRetreatTargetChanged, out bool outCompartmentChanged)
	{
		float worldTime = GetGame().GetWorld().GetWorldTime();
		if (worldTime < m_fNextWeaponTargetEvaluation_ms)
		{
			outWeaponEvent = false;
			outSelectedTargetChanged = false;
			return;
		}

		m_fNextWeaponTargetEvaluation_ms = worldTime + WEAPON_TARGET_UPDATE_PERIOD_MS;

		float engageInfantry, engageVehicle;
		DCO_ResolveEngagement(engageInfantry, engageVehicle);
		if (engageInfantry != m_fDCOEngageInfantry || engageVehicle != m_fDCOEngageVehicle)
			SetTargetSelectionProperties(m_bDCOCloseCombat);

		SCR_ChimeraAIAgent myAgent = GetAiAgent();
		float agentThreat = m_Utility.m_ThreatSystem.GetThreatMeasure();

		AIGroup myGroup = myAgent.GetParentGroup();
		SCR_AIGroupInfoComponent groupInfoComp;
		if (myGroup)
			groupInfoComp = SCR_AIGroupInfoComponent.Cast(myGroup.FindComponent(SCR_AIGroupInfoComponent));

		BaseTarget newTarget = null;
		bool weaponEvent = false;
		bool selectedTargetChanged = false;
		bool retreatTargetChanged = false;
		bool compartmentChanged = false;

		array<EWeaponType> weaponBlacklist;
		if (groupInfoComp)
		{
			if (agentThreat > FRAG_GRENADE_MAX_THREAT || !groupInfoComp.IsGrenadeThrowAllowed(myAgent))
				weaponBlacklist = s_aWeaponBlacklistFragGrenades;
		}

		if (m_Utility && m_Utility.m_DCOConfig && m_Utility.m_DCOConfig.GetGrenadeUsage() <= 0.0)
			weaponBlacklist = s_aWeaponBlacklistFragGrenades;

		bool useCompartmentWeapons = m_AIInfo.HasUnitState(EUnitState.IN_TURRET);

		array<IEntity> assignedTargets;
		if (m_TargetClusterState && m_TargetClusterState.m_Cluster && m_TargetClusterState.m_Cluster.m_aEntities)
			assignedTargets = m_TargetClusterState.m_Cluster.m_aEntities;
		else
			assignedTargets = m_aAssignedTargets;

		bool selectedWpnTarget = m_WeaponTargetSelector.SelectWeaponAndTarget(assignedTargets,
			ASSIGNED_TARGETS_SCORE_INCREMENT, ENDANGERING_TARGETS_SCORE_INCREMENT,
			useCompartmentWeapons, weaponTypesBlacklist: weaponBlacklist);

		m_eUnitTypesCanAttack = m_WeaponTargetSelector.GetUnitTypesCanAttack();
		if (selectedWpnTarget)
		{
			BaseWeaponComponent newWeaponComp;
			BaseMagazineComponent newMagazineComp;
			int newMuzzleId;

			newTarget = m_WeaponTargetSelector.GetSelectedTarget();
			m_WeaponTargetSelector.GetSelectedWeapon(newWeaponComp, newMuzzleId, newMagazineComp);
			m_WeaponTargetSelector.GetSelectedWeaponProperties(m_fSelectedWeaponMinDist, m_fSelectedWeaponMaxDist, m_bSelectedWeaponDirectDamage);

			weaponEvent = newWeaponComp != m_SelectedWeaponComp ||
							newMuzzleId != m_iSelectedMuzzle ||
							newMagazineComp != m_SelectedMagazineComp;

			bool weaponOrMuzzleChanged = newWeaponComp != m_SelectedWeaponComp ||
									newMuzzleId != m_iSelectedMuzzle;

			if (weaponOrMuzzleChanged)
			{
				ref array<BaseMuzzleComponent> muzzles = {};
				newWeaponComp.GetMuzzlesList(muzzles);
				if (newMuzzleId >= muzzles.Count() || newMuzzleId < 0)
					m_SelectedWeaponResource = m_ConfigComponent.GetTreeNameForWeaponType(newWeaponComp.GetWeaponType(),0);
				else
					m_SelectedWeaponResource = m_ConfigComponent.GetTreeNameForWeaponType(newWeaponComp.GetWeaponType(),muzzles[newMuzzleId].GetMuzzleType());

				if (newWeaponComp)
				{
					EWeaponType weaponType = newWeaponComp.GetWeaponType();
					if (groupInfoComp && weaponType == EWeaponType.WT_FRAGGRENADE)
					{
						groupInfoComp.OnAgentSelectedGrenade(myAgent);
					}
				}

				DecreaseAim();
			}

			m_SelectedWeaponComp = newWeaponComp;
			m_iSelectedMuzzle = newMuzzleId;
			m_SelectedMagazineComp = newMagazineComp;
		}

		if (newTarget)
		{
			BaseTarget atTarget = DCO_GetVehicleATTarget();
			if (atTarget)
				newTarget = atTarget;
		}

		if (DCO_IsTunnelVisionBlocked(newTarget))
		{
			PrintFormat("[DCO_TMP_TUNNEL] %1 keep target, side-recognized %2 s", ownerEntity, newTarget.GetTimeSinceSideRecognized());
			newTarget = m_SelectedTarget;
		}

		BaseTarget prevTarget = m_SelectedTarget;
		if (newTarget != m_SelectedTarget)
		{
			#ifdef AI_DEBUG
			AddDebugMessage(string.Format("Target has changed. New: %1, Previous: %2", newTarget, m_SelectedTarget));
			#endif
			m_SelectedTarget = newTarget;
			selectedTargetChanged = true;
		}

		BaseTarget targetCantAttack;
		float targetCantAttackScore;
		m_WeaponTargetSelector.GetMostRelevantTargetCantAttack(targetCantAttack, targetCantAttackScore);
		if (targetCantAttackScore < TARGET_SCORE_RETREAT)
			targetCantAttack = null;
		if (targetCantAttack != m_SelectedRetreatTarget)
		{
			m_SelectedRetreatTarget = targetCantAttack;
			retreatTargetChanged = true;
		}

		BaseCompartmentSlot currentCompartment = m_CompartmentAccess.GetCompartment();
		if (currentCompartment != m_WeaponEvaluationCompartment)
		{
			compartmentChanged = true;
			m_WeaponEvaluationCompartment = currentCompartment;
		}

		if (selectedTargetChanged)
		{
			m_SelectedTargetVisible = false;
			m_SelectedTargetDestinationPos = vector.Zero;
		}

		if (newTarget)
		{
			bool visible = IsTargetVisible(newTarget);
			IEntity targetEntity = newTarget.GetTargetEntity();
			ChangeTarget = selectedTargetChanged;
			if (visible != m_SelectedTargetVisible)
			{
				m_SelectedTargetVisible = visible;

				if (!visible && targetEntity)
					m_SelectedTargetDestinationPos = targetEntity.GetOrigin();

				DecreaseAim();
			}
		} else
		{
			ChangeTarget = false;
		}

		outWeaponEvent = weaponEvent;
		outSelectedTargetChanged = selectedTargetChanged;
		outRetreatTargetChanged = retreatTargetChanged;
		outCompartmentChanged = compartmentChanged;
		outCurrentTarget = newTarget;
		outPrevTarget = prevTarget;
	}

	override void UpdatePerceptionFactor(PerceptionComponent perceptionComp, SCR_AIThreatSystem threatSystem)
	{
		EAIThreatState threatState = threatSystem.GetState();
		float perceptionFactor;
		switch (threatState)
		{
			case EAIThreatState.SAFE:
				perceptionFactor = PERCEPTION_FACTOR_SAFE; break;
			case EAIThreatState.VIGILANT:
				perceptionFactor = PERCEPTION_FACTOR_VIGILANT; break;
			case EAIThreatState.ALERTED:
				perceptionFactor = PERCEPTION_FACTOR_ALERTED; break;
			case EAIThreatState.THREATENED:
				perceptionFactor = PERCEPTION_FACTOR_THREATENED; break;
		}

		perceptionFactor *= m_fEquipmentPerceptionFactor;
		perceptionFactor *= m_fPerceptionFactor;
		perceptionFactor *= m_Utility.m_DCOConfig.GetPerception();

		float extraMag = DCO_GetScopedADSExtraMag();
		if (extraMag > 0)
			perceptionFactor *= 1 + extraMag * SCOPE_PERCEPTION_GAIN;

		perceptionComp.SetPerceptionFactor(perceptionFactor);
	}

	protected BaseTarget DCO_GetVehicleATTarget()
	{
		if (!m_AIInfo || !m_AIInfo.HasUnitState(EUnitState.IN_TURRET) || !m_Utility || !m_Utility.m_PerceptionComponent)
			return null;

		IEntity shooter;
		vector pos;
		if (!DCO_VehicleCombat.GetATThreat(DCO_VehicleCombat.GetVehicle(ownerEntity), shooter, pos))
			return null;

		BaseTarget target = m_Utility.m_PerceptionComponent.GetTargetPerceptionObject(shooter, ETargetCategory.ENEMY);
		if (!target || target.GetTimeSinceSeen() > DCO_VehicleCombat.AT_SEEN_MAX_S)
			return null;

		return target;
	}

	protected float DCO_GetScopedADSExtraMag()
	{
		if (!m_CharacterController || !m_CharacterController.IsWeaponADS())
			return 0;

		return DCO_AimUtility.GetSightMagnification(this) - 1;
	}

	protected bool DCO_IsTunnelVisionBlocked(BaseTarget candidate)
	{
		if (!candidate || !m_SelectedTarget || candidate == m_SelectedTarget || candidate.IsEndangering())
			return false;

		IEntity current = m_SelectedTarget.GetTargetEntity();
		IEntity other = candidate.GetTargetEntity();
		if (!current || !other || !ownerEntity || !IsTargetVisible(m_SelectedTarget))
			return false;

		float extraMag = DCO_GetScopedADSExtraMag();
		if (extraMag <= 0 || candidate.GetTimeSinceSideRecognized() > extraMag * SCOPE_TUNNEL_DELAY_PER_MAG)
			return false;

		vector me = ownerEntity.GetOrigin();
		vector toCurrent = current.GetOrigin() - me;
		vector toOther = other.GetOrigin() - me;
		toCurrent[1] = 0;
		toOther[1] = 0;
		return vector.Dot(toCurrent.Normalized(), toOther.Normalized()) < Math.Cos(SCOPE_TUNNEL_HALF_ANGLE * Math.DEG2RAD);
	}

	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		CURRENT_AIM_IMPROVEMENT = m_fStartAccuracy;
		ownerEntity = owner;
	}

	protected bool RefreshAccuracyParams()
	{
		if (!m_Utility || !m_Utility.m_DCOConfig)
			return false;

		m_fDurationN = m_Utility.m_DCOConfig.GetAccuracyTime() * 1000;
		float accuracy = m_Utility.m_DCOConfig.GetAccuracy();
		if (m_fDurationN <= 0 || accuracy <= 0)
			return false;

		m_fTargetAccuracy = Math.Clamp(AIM_IMPROVEMENT_TARGET / accuracy, AIM_IMPROVEMENT_MIN, m_fStartAccuracy);
		return true;
	}

	void UpdateAccuracy(float timeSlice)
	{
		if (!ownerEntity || !RefreshAccuracyParams())
			return;

		Physics phys = ownerEntity.GetPhysics();
		float currentSpeed = 0.0;

		if (phys)
		{
			currentSpeed = phys.GetVelocity().Length();
		}

		float speedThreshold = 0.5;
		float aimProgressRate = 1.0;

		if (currentSpeed > speedThreshold)
		{
			aimProgressRate = -1.5 * (currentSpeed / 2.0);
		}

		if (DCO_CQC.IsCQC(m_Utility))
		{
			if (aimProgressRate > 0)
				aimProgressRate *= DCO_CQC.AIM_SPEEDUP;
			else
				aimProgressRate *= DCO_CQC.MOVING_PENALTY_SCALE;
		}

		m_fTimeElapsed += timeSlice * aimProgressRate;
		m_fTimeElapsed = Math.Clamp(m_fTimeElapsed, 0.0, m_fDurationN);

		float rawProgress = m_fTimeElapsed / m_fDurationN;
		float smoothProgress = rawProgress * rawProgress * (3.0 - 2.0 * rawProgress);

		CURRENT_AIM_IMPROVEMENT = Math.Lerp(m_fStartAccuracy, m_fTargetAccuracy, smoothProgress);
	}

	SCR_CharacterControllerComponent GetCharacterController()
	{
		return m_CharacterController;
	}
}