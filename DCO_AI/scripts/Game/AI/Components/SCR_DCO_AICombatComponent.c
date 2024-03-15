modded enum EAISkill
{
	RECRUIT	= 10,
	TRAINED	= 30,
};

modded class SCR_AICombatComponent : ScriptComponent
{
	protected SCR_AIGroup m_SCR_AIGroup;
	protected IEntity m_ControlledEntity;
	protected SCR_ChimeraAIAgent m_SCR_ChimeraAIAgent;
	
	protected DCO_AIInfoComponent m_DCO_AIInfoComponent;
	
	protected static const float ASSIGNED_TARGETS_SCORE_INCREMENT = 15.0;
	protected static const float ENDANGERING_TARGETS_SCORE_INCREMENT = 30.0;

<<<<<<< HEAD
	protected static const float TARGET_INVESTIGATE_TIME = 1.0;	
	
	protected static const float TARGET_MAX_DISTANCE_DISARMED = 0.5;
	
=======
>>>>>>> Reforger_1.1
	protected static const float TARGET_MAX_DISTANCE_INFANTRY = 500.0;
	protected static const float TARGET_MAX_LAST_SEEN_DIRECT_ATTACK = 0.8;
			  static const float TARGET_MAX_LAST_SEEN_INDIRECT_ATTACK = 3.0;
			  static const float TARGET_MAX_LAST_SEEN_INDIRECT_ATTACK_MG = 8.0;
			  static const float TARGET_MAX_LAST_SEEN = 20.0;
	static const float TARGET_SCORE_HIGH_PRIORITY_ATTACK = 100.0;
	static const float TARGET_MAX_LAST_SEEN_VISIBLE = 0.7;
	protected const float PERCEPTION_FACTOR_SAFE = 4.0;
	protected const float PERCEPTION_FACTOR_VIGILANT = 5.0;
	protected const float PERCEPTION_FACTOR_ALERTED = 6.0; 
	protected const float PERCEPTION_FACTOR_THREATENED = 5.0;
	protected const float PERCEPTION_FACTOR_PINNED = 4.0;
	protected const float PERCEPTION_FACTOR_EXHAUSTED = 4.0;
	
	protected const float PERCEPTION_FACTOR_EQUIPMENT_BINOCULARS = 3.0;
	protected const float PERCEPTION_FACTOR_EQUIPMENT_NONE = 1.0;
	
	protected static const float TARGET_MAX_LAST_SEEN_DIRECT_ATTACK = 1.6;
	
<<<<<<< HEAD
	protected static const float TARGET_SCORE_RETREAT = 75.0;
	
	protected static const float TARGET_INVISIBLE_TIME = 8.0;
	
	static const float LONG_RANGE_FIRE_DISTANCE = 100.0;
=======
	static const float LONG_RANGE_FIRE_DISTANCE = 300.0;
>>>>>>> Reforger_1.1

	override protected void EOnInit(IEntity owner)
	{
		GetAiAgent();
		
		super.EOnInit(owner);
		
		if (m_Agent)
		{
			m_ControlledEntity = m_Agent.GetControlledEntity();
			
			m_SCR_ChimeraAIAgent = SCR_ChimeraAIAgent.Cast(m_Agent);
			
			m_DCO_AIInfoComponent = DCO_AIInfoComponent.Cast(m_Agent.FindComponent(DCO_AIInfoComponent));
		}
	}
	
	override bool IsFriendlyInAim()
	{
		IEntity friendlyEntInAim = m_Perception.GetFriendlyInLineOfFire();
#ifdef WORKBENCH
		if (friendlyEntInAim && DiagMenu.GetBool(SCR_DebugMenuID.DEBUGUI_AI_SHOW_FRIENDLY_IN_AIM))
			m_FriendlyAimShape = Shape.CreateSphere(COLOR_RED, ShapeFlags.NOOUTLINE|ShapeFlags.NOZBUFFER|ShapeFlags.TRANSP, friendlyEntInAim.GetOrigin() + Vector(0, 2, 0), 0.1);	
		else 
			m_FriendlyAimShape = null;
#endif		
		m_bFriendlyAimLastResult = friendlyEntInAim != null;
				
		return m_bFriendlyAimLastResult;
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
		
		perceptionComp.SetPerceptionFactor(perceptionFactor);
	}
	
<<<<<<< HEAD
	override void SetHoldFire(bool isHoldFire)
	{
		#ifdef AI_DEBUG
		AddDebugMessage(string.Format("SetHoldFire: %1", isHoldFire));
		#endif
		
		if (isHoldFire)
		{ 
			SetActionAllowed(EAICombatActions.HOLD_FIRE,true);
			SetActionAllowed(EAICombatActions.BURST_FIRE,false);
			SetActionAllowed(EAICombatActions.SUPPRESSIVE_FIRE,false);
		}
		else
		{
			SetCombatType(m_eCombatType);
		}
=======
	//------------------------------------------------------------------------------------------------
	protected static const float DISTANCE_MAX = 500; 
	protected static const float DISTANCE_MIN = 5; // Minimal distance when movement is allowed
	private static const float NEAR_PROXIMITY = 2;
	// TODO: add possibility to get cover towards custom position
	//------------------------------------------------------------------------------------------------
	override vector FindNextCoverPosition()
	{
		if (!m_SelectedTarget)
			return vector.Zero;
		
		vector ownerPos = GetOwner().GetOrigin();
		vector lastSeenPos = m_SelectedTarget.GetLastSeenPosition();
		float distanceToTarget = vector.Distance(ownerPos, lastSeenPos);

		if (m_StopDistance > distanceToTarget)
			return vector.Zero;
		
		// Create randomized position
		SCR_ChimeraAIAgent agent = GetAiAgent();
		SCR_DefendWaypoint defendWp = SCR_DefendWaypoint.Cast(agent.m_GroupWaypoint);
		vector direction;
		bool standardAttack = true;
		float nextCoverDistance;
		
		// If target is outside defend waypoint, run towards center of it
		if (defendWp)
		{
			if (!defendWp.IsWithinCompletionRadius(lastSeenPos) &&
				!defendWp.IsWithinCompletionRadius(ownerPos))
			{
				direction = vector.Direction(ownerPos, defendWp.GetOrigin());	// Direction towards center of defend wp
				
				if (vector.Distance(defendWp.GetOrigin(), ownerPos) < DISTANCE_MIN)
					nextCoverDistance = 0;
				else	
					nextCoverDistance = DISTANCE_MIN;
				
				standardAttack = false;
			}
		}
		
		if (standardAttack)
		{
			nextCoverDistance = Math.RandomFloat(DISTANCE_MIN, DISTANCE_MAX);

			// If close enough, get directly to the target
			if (nextCoverDistance > (distanceToTarget - DISTANCE_MIN))
				nextCoverDistance = distanceToTarget - DISTANCE_MIN;
			
			direction = vector.Direction(ownerPos, m_SelectedTarget.GetLastSeenPosition());
		}
			
		direction.Normalize();
		vector newPositionCenter = direction * nextCoverDistance + ownerPos, newPosition;
		// yes possibly it could lead to end up in target position but lets ignore it for now
		
		newPosition = s_AIRandomGenerator.GenerateRandomPointInRadius(0, NEAR_PROXIMITY, newPositionCenter, true);
		newPosition[1] = newPositionCenter[1];
		return newPosition;
>>>>>>> Reforger_1.1
	}
	
	override void SetCombatType(EAICombatType combatType)
	{
		#ifdef AI_DEBUG
		AddDebugMessage(string.Format("SetCombatType: %1", typename.EnumToString(EAICombatType, combatType)));
		#endif
		
		switch (combatType)
		{
			case EAICombatType.NONE:
			{
				SetActionAllowed(EAICombatActions.HOLD_FIRE,true);
				SetActionAllowed(EAICombatActions.MOVEMENT_WHEN_FIRE,false);
				SetActionAllowed(EAICombatActions.SUPPRESSIVE_FIRE,false);
				SetActionAllowed(EAICombatActions.MOVEMENT_TO_LAST_SEEN,false);
				break;
			}
			case EAICombatType.NORMAL:
			{
				SetActionAllowed(EAICombatActions.HOLD_FIRE,false);
				SetActionAllowed(EAICombatActions.MOVEMENT_WHEN_FIRE,true);
				SetActionAllowed(EAICombatActions.SUPPRESSIVE_FIRE,true);
				SetActionAllowed(EAICombatActions.MOVEMENT_TO_LAST_SEEN,true);
				break;
			}
			case EAICombatType.SUPPRESSIVE:
			{
				SetActionAllowed(EAICombatActions.HOLD_FIRE,false);
				SetActionAllowed(EAICombatActions.MOVEMENT_WHEN_FIRE,false);
				SetActionAllowed(EAICombatActions.SUPPRESSIVE_FIRE,true);
				SetActionAllowed(EAICombatActions.MOVEMENT_TO_LAST_SEEN,true);
				break;
			}
			case EAICombatType.RETREAT:
			{
				SetActionAllowed(EAICombatActions.HOLD_FIRE,false);
				SetActionAllowed(EAICombatActions.MOVEMENT_WHEN_FIRE,true);
				SetActionAllowed(EAICombatActions.SUPPRESSIVE_FIRE,true);
				SetActionAllowed(EAICombatActions.MOVEMENT_TO_LAST_SEEN,false);
				break;
			}
			case EAICombatType.SINGLE_SHOT:
			{
				SetActionAllowed(EAICombatActions.HOLD_FIRE,false);
				SetActionAllowed(EAICombatActions.MOVEMENT_WHEN_FIRE,false);
				SetActionAllowed(EAICombatActions.SUPPRESSIVE_FIRE,false);
				SetActionAllowed(EAICombatActions.MOVEMENT_TO_LAST_SEEN,false);
				break;
			}
		}
		m_eCombatType = combatType;
#ifdef WORKBENCH
		SCR_AIDebugVisualization.VisualizeMessage(GetOwner(), typename.EnumToString(EAICombatType,m_eCombatType), EAIDebugCategory.COMBAT, 5);
#endif
	}
<<<<<<< HEAD
	
	override void ResetCombatType()
	{
		#ifdef AI_DEBUG
		AddDebugMessage("ResetCombatType");
		#endif
		
		SetCombatType(m_eDefaultCombatType);
	}
=======
>>>>>>> Reforger_1.1
};