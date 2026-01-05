modded class SCR_AICombatComponent
{
	
	static const int			 TARGET_ENDANGERED_TIMEOUT_S = 12;				//!< How long after last time target was endangering we stop treating it as such
	static const float			 ENDANGERING_TARGET_SCORE_MULTIPLIER = 1.5;		//!< Multiplier of target score if target is considered endangering
	
	// Score increments for assigned targets and endangering targets
	protected static const float ASSIGNED_TARGETS_SCORE_INCREMENT = 1.0;
	protected static const float ENDANGERING_TARGETS_SCORE_INCREMENT = 20.0;
	
	// AIM IMPROVEMENT
	static const float AIM_IMPROVEMENT_INCREASE = 0.00001;
	static const float AIM_IMPROVEMENT_DECREASE = 0.000001;
	static const float AIM_MORALE = 0.00001;
	static const float AIM_IMPROVEMENT_CONST_DECREASE = AIM_IMPROVEMENT_DECREASE * 12;
	static const float AIM_IMPROVEMENT_CONST_DECREASE_SUPPRESSED_MULTIPLIER = 3;
	
	          static const float TARGET_MAX_LAST_SEEN = 11.0;
	protected static const float TARGET_MIN_INDIRECT_TRACE_FRACTION_MIN = 0.5;	//!< Min value of TraceFraction for indirect attacks. \see BaseTarget.GetTraceFraction.
	protected static const float TARGET_MAX_DISTANCE_VEHICLE = 1700.0;		//!<
	protected static const float TARGET_MAX_DISTANCE_DISARMED = 0.2;		//!< Max distance at which disarmed targets are considered for attack. Now it's 0, so they never shoot disarmed targets
	protected static const float TARGET_MAX_TIME_SINCE_ENDANGERED = 5.0; 	//!< Max time since we were endangered to consider the enemy target endangering
	protected static const float TARGET_SCORE_RETREAT = 75.0;				//!< Threshold for target score for retreating from that target
	static const float TARGET_SCORE_HIGH_PRIORITY_ATTACK = 98.5;			//!< Threshold for high priority attack
		
	protected const float PERCEPTION_FACTOR_SAFE = 1.0;			//!< We are safe and are good at recognising enemies
	protected const float PERCEPTION_FACTOR_VIGILANT = 3;		//!< When vigilant and alert we are very good at recognising enemies
	protected const float PERCEPTION_FACTOR_ALERTED = 3;
	protected const float PERCEPTION_FACTOR_THREATENED = 1.5;	// We are suppressed and are bad at recognizing enemies
	
	float CURRENT_AIM_IMPROVEMENT;
	
	bool ChangeTarget = false;
	
	void ImproveAim()
	{
		CURRENT_AIM_IMPROVEMENT += AIM_IMPROVEMENT_INCREASE;
	}
	
	void DecreaseAim()
	{
		Math.Clamp(CURRENT_AIM_IMPROVEMENT - AIM_IMPROVEMENT_DECREASE, 0, int.MAX);
	}
	
	void ChangeTargetCompensation()
	{
		Math.Clamp(CURRENT_AIM_IMPROVEMENT - AIM_IMPROVEMENT_CONST_DECREASE, 0, int.MAX);
	}
	
	void DangerSuppressedDecreaseAIM(float dec)
	{
		float decrease = dec * AIM_IMPROVEMENT_DECREASE * AIM_IMPROVEMENT_CONST_DECREASE_SUPPRESSED_MULTIPLIER;
		Math.Clamp(CURRENT_AIM_IMPROVEMENT - decrease, 0, int.MAX);
	}
	
	void MoraleDropAIM(float val)
	{
		Math.Clamp(CURRENT_AIM_IMPROVEMENT - (val * AIM_MORALE), 0, int.MAX);
	}
	
	override void Update(float timeSliceMs)
	{
		super.Update(timeSliceMs);
		if (m_SelectedTargetVisible)
		{
			if (ChangeTarget)
			{
				ChangeTargetCompensation();
			}
			else
			{	
				ImproveAim();
			}
		} else if (!m_SelectedTargetVisible)
		{
			DecreaseAim();
		}
	}
	
	float GetCurrentAimImprovement()
	{
		return CURRENT_AIM_IMPROVEMENT;
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
		
		// Resolve if we want to think of throwing grenade
		// Grenade throw is synchronized via group
		array<EWeaponType> weaponBlacklist;
		if (groupInfoComp)
		{
			if (agentThreat > FRAG_GRENADE_MAX_THREAT || !groupInfoComp.IsGrenadeThrowAllowed(myAgent))
				weaponBlacklist = s_aWeaponBlacklistFragGrenades;
		}
		
		bool useCompartmentWeapons = m_AIInfo.HasUnitState(EUnitState.IN_TURRET); // True when we are in a turret
		
		// Which assigned targets array to use?
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
						// We want to throw a grenade
						// Notify group immediately
						groupInfoComp.OnAgentSelectedGrenade(myAgent);
					}
				}
			}
			
			m_SelectedWeaponComp = newWeaponComp;
			m_iSelectedMuzzle = newMuzzleId;
			m_SelectedMagazineComp = newMagazineComp;
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
		
		// Check if we must retreat from some target
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
		
		// Check if compartment has changed
		BaseCompartmentSlot currentCompartment = m_CompartmentAccess.GetCompartment();
		if (currentCompartment != m_WeaponEvaluationCompartment)
		{
			compartmentChanged = true;
			m_WeaponEvaluationCompartment = currentCompartment;
		}
		
		// Reset last velocity if target changed
		if (selectedTargetChanged)
		{
			m_SelectedTargetVisible = false;
			m_SelectedTargetDestinationPos = vector.Zero;
		}
			
		if (newTarget)
		{
			bool visible = IsTargetVisible(newTarget);
			IEntity targetEntity = newTarget.GetTargetEntity();
			ChangeTarget = true;
			if (visible != m_SelectedTargetVisible)
			{
				m_SelectedTargetVisible = visible;
				
				// Save position (destination) of target pos at the time we figured we've lost LOS
				// Note: this solution is dependent on update rate of EvaluateWeaponAndTarget
				if (!visible && targetEntity)
					m_SelectedTargetDestinationPos = targetEntity.GetOrigin();
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
}