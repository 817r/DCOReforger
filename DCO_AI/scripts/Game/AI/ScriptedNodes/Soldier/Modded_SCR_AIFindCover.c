modded class SCR_AIFindCover : AITaskScripted
{
	private bool m_bAbort;
	
	private bool m_IsAgentNearby;
	private IEntity m_AgentEntityNearby;
	
	private BaseTarget m_CurrentTarget;
	
	private float m_ThreatSuppression;
	private float m_CoverSearchDistance;
	
	private AIGroup m_ParentGroup;
	private IEntity m_OwnerEntity;
	private SCR_ChimeraAIAgent m_SCR_ChimeraAIAgent;
	
	private AIFormationComponent m_FormationComponent;
	
	private DCO_AIInfoComponent m_DCO_AIInfoComponent;
	private SCR_AIUtilityComponent m_SCR_AIUtilityComponent;
	private DCO_AIInfoGroupComponent m_DCO_AIGroupInfoComponent;
	
	//override private const vector
	private const vector PRONE_OFFSET = Vector(0, 0.30, 0);
	
	private vector m_CurrentCoverPosition;
	
	private ref array<ref vector> m_CoverPositions = DCO_AIFindCover.GetCoverPositions();
	
	//modded protected static ref TStringArray s_aVarsIn = {
	//	PORT_FRIENDLY__REF
	//};	
	
	//------------------------------------------------------------------------------------------------
	override void OnInit(AIAgent owner)
	{
		m_ParentGroup = owner.GetParentGroup();
		
		m_OwnerEntity = owner.GetControlledEntity();
		
		m_SCR_ChimeraAIAgent = SCR_ChimeraAIAgent.Cast(owner);
		
		m_SCR_AIUtilityComponent = SCR_AIUtilityComponent.Cast(owner.FindComponent(SCR_AIUtilityComponent));
		
		m_DCO_AIInfoComponent = m_SCR_AIUtilityComponent.m_DCO_AIInfoComponent;
		
		if (m_ParentGroup)
		{
			m_FormationComponent = AIFormationComponent.Cast(m_ParentGroup.FindComponent(AIFormationComponent));
		}
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnEnter(AIAgent owner)
	{
		m_bAbort = true;
		
		m_CoverSearchDistance++;
		
		if (m_CoverSearchDistance > 30)
			m_CoverSearchDistance = 0;
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnAbort(AIAgent owner, Node nodeCausingAbort)
	{
		if (owner && m_bAbort)
		{
			m_bAbort = false;
			
			int coverPositionIndex = m_CoverPositions.Find(m_CurrentCoverPosition);
			
			if (coverPositionIndex > -1)
				m_CoverPositions.Remove(coverPositionIndex);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		if (!m_PathfindingComponent)
		{
			m_PathfindingComponent = AIPathfindingComponent.Cast( owner.FindComponent(AIPathfindingComponent));
			
			if (!m_PathfindingComponent)
				NodeError(this, owner, "Missing pathfinding component");
		}
		
		m_World = owner.GetWorld(); 
		bool directionProvided = false;
		
		if (!GetVariableIn(PORT_ENEMY, m_Enemy))
		{
			if (!GetVariableIn(PORT_DANGER_POS, m_DangerPosition))
				return ENodeResult.FAIL;
			else
				directionProvided = true;
		}
		
		ChimeraCharacter enemyCharacter = ChimeraCharacter.Cast(m_Enemy);
		if (!enemyCharacter && !directionProvided)
			return ENodeResult.RUNNING;
		
#ifdef WORKBENCH		
		ClearDebug();
#endif
		
		bool inCoverNow;
		if(!GetVariableIn(PORT_IN_COVER, inCoverNow))
			inCoverNow = false;
		
		IEntity ownerEntity = owner.GetControlledEntity();
		vector enemyAimPos;
		if (!directionProvided)
			enemyAimPos = enemyCharacter.AimingPosition();
		else 
			enemyAimPos = m_DangerPosition;
		
		GetVariableIn(PORT_DANGER_POS, m_DangerPosition);
		
		enemyAimPos = m_DangerPosition;
		
		if (enemyCharacter)
			enemyAimPos = enemyCharacter.AimingPosition();
		
		float distanceIsDanger = Math.RandomFloat(50,100);
		
		float threatSuppressionIsDanger = Math.RandomFloat(0.3,0.7);
		
		float distanceToDanger = vector.Distance(m_DangerPosition, ownerEntity.GetOrigin());
		
		vector traceOrigin;
		if (inCoverNow)
			traceOrigin = ownerEntity.CoordToParent(IN_COVER_OFFSET);
		else
			traceOrigin = ownerEntity.GetOrigin();
		
		bool isEndangering;
		
		float offsetDistanceX = Math.RandomFloat(0,15);
		
		offsetDistanceX = Math.RandomFloat(5,30);
		
		if (m_ThreatSuppression > threatSuppressionIsDanger)
			offsetDistanceX = m_CoverSearchDistance;
		
		AIAgent leaderAgent = m_ParentGroup.GetLeaderAgent();
					
		IEntity leaderEntity = m_ParentGroup.GetLeaderEntity();
					
		if (owner == leaderAgent)
			traceOrigin = leaderEntity.GetOrigin();
					else
					{
						if (m_FormationComponent)
						{
							AIFormationDefinition formation = m_FormationComponent.GetFormation();
							
							if (formation)
							{
								array<AIAgent> agents = {};
								
								m_ParentGroup.GetAgents(agents);
								
								int formationOffsetIndex = agents.Find(owner);
								
								vector offsetPosition = formation.GetOffsetPosition(formationOffsetIndex);
								
								vector offsetWorldPosition = leaderEntity.CoordToParent(offsetPosition);
								
								traceOrigin = offsetWorldPosition;
							}
						}
					}
		
	/*	if (m_DCO_AIInfoComponent.GetHoldPosition())
		{
			int holdPositionRadius = m_DCO_AIInfoComponent.GetHoldPositionRadius();
			
			if (holdPositionRadius < 1)
				holdPositionRadius = 1;
			
			offsetDistanceX = holdPositionRadius;
			
			traceOrigin = m_DCO_AIInfoComponent.GetHoldPositionOrigin();
		} */
		
		float offsetDistanceZ = offsetDistanceX;
		
		if (Math.RandomFloat(0,100) < 50 && m_CoverSearchDistance > 5)
			offsetDistanceZ = 0;
		
		
		vector searchCoverOffset = Vector(0, 0, 0);
		
		if (distanceIsDanger > distanceToDanger)
		{
			searchCoverOffset = Vector(0, 0, -7);
		}
		else
		{
			if (m_ThreatSuppression > threatSuppressionIsDanger)
			{				
				searchCoverOffset = Vector(0, 0, 0);
			}
			else
			{
				float searchCoverRandomOffset = Math.RandomFloat(-7,7);
				
				searchCoverOffset = Vector(offsetDistanceX, 0, offsetDistanceX);
			}
		}
		
		traceOrigin = ownerEntity.CoordToParent(searchCoverOffset);
		
		m_CurrentTarget = m_SCR_AIUtilityComponent.m_CombatComponent.GetCurrentTarget();

		if (m_CurrentTarget)
			isEndangering = m_CurrentTarget.IsEndangering();
		
		if (m_ThreatSuppression > 0.5 || distanceIsDanger > distanceToDanger)
		{
			offsets =
			{
				Vector(0, GROUND_HEIGHT, -offsetDistanceZ),
				Vector(offsetDistanceX, GROUND_HEIGHT, -offsetDistanceZ),
				Vector(-offsetDistanceX, GROUND_HEIGHT, -offsetDistanceZ)
			};
		}
		else
		{
			offsets =
			{
				Vector(0, GROUND_HEIGHT, offsetDistanceZ),
				Vector(offsetDistanceX, GROUND_HEIGHT, offsetDistanceZ),
				Vector(-offsetDistanceX, GROUND_HEIGHT, offsetDistanceZ)
			};
		}
	
		ECharacterStance stance;
		
		vector hitNavmeshPos;
		bool coverFound;		
		
#ifdef WORKBENCH
		if (DiagMenu.GetBool(SCR_DebugMenuID.DEBUGUI_AI_DEBUG_COVERS))
			m_DebugShapes.Insert(Shape.CreateSphere(Color.PINK, m_SphereFlags, traceOrigin, DEBUGSPHERE_RADIUS));
#endif		
		foreach (vector offsetLocal : offsets)
		{
			vector traceEndWorld = ownerEntity.CoordToParent(offsetLocal);
			
			bool holeInNavmesh = !m_PathfindingComponent.RayTrace(traceOrigin, traceEndWorld, hitNavmeshPos);
			
#ifdef WORKBENCH
			if (DiagMenu.GetBool(SCR_DebugMenuID.DEBUGUI_AI_DEBUG_COVERS))
			{ 
				int color = COLOR_BLUE;
				if (holeInNavmesh)
					color = COLOR_BLUE_A;
				m_DebugShapes.Insert(Shape.CreateSphere(color, m_SphereFlags, traceEndWorld, DEBUGSPHERE_RADIUS));
			}
#endif
			
			if (!holeInNavmesh)
				continue;
			
			if (hitNavmeshPos == vector.Zero)
			{			
				continue;
			}
						
			if (m_ThreatSuppression > threatSuppressionIsDanger)
			
			{

			}
			else
			{
				float distanceToCover = vector.Distance(hitNavmeshPos, ownerEntity.GetOrigin());
				
				if (m_CurrentTarget)
				{
					if (distanceToCover < 1)
						continue;
				}
				else
				{
					if (distanceToCover < 3)
						continue;
				}
			}
			
			hitNavmeshPos[1] = m_World.GetSurfaceY(hitNavmeshPos[0], hitNavmeshPos[2]);
			
			coverFound = IsCover(ownerEntity, hitNavmeshPos, enemyAimPos, isEndangering, stance);
			
			if (coverFound)
				break;
		}
		
		if (!coverFound)
			return ENodeResult.FAIL; 
		
		if (IsCoverAvailable(hitNavmeshPos))
		{
			if (IsAgentNearby(hitNavmeshPos))
				return ENodeResult.FAIL; 
			
			m_CoverSearchDistance = 0;
			
			float coverDistaceOffset = Math.RandomFloat(1,2);
			
			vector direction = (hitNavmeshPos - m_DangerPosition).Normalized();
			
			vector coverPosition = hitNavmeshPos + direction * coverDistaceOffset;
			
			int coverPositionIndex = m_CoverPositions.Find(m_CurrentCoverPosition);
			
			#ifdef WORKBENCH
			
			#endif
			
			if (coverPositionIndex > -1)
			{
				m_CoverPositions.Remove(coverPositionIndex);
				
				#ifdef WORKBENCH
				
				#endif
			}
			
			m_CoverPositions.Insert(coverPosition);
			
			m_CurrentCoverPosition = coverPosition;
			
			SetVariableOut(PORT_STANCE, stance);
			SetVariableOut(PORT_POSITION, coverPosition);
			
			return ENodeResult.SUCCESS;
		}
		
		return ENodeResult.FAIL;
	}
	
	//------------------------------------------------------------------------------------------------		
	bool IsCover(IEntity ownerEntity, vector posGround, vector end, bool isEndangering, out ECharacterStance stance)
	{
		float hit;
		
		float hitPrecision = 1;
		
		hitPrecision -= m_ThreatSuppression;
		
		if (m_CurrentTarget == null)
		{
			hitPrecision = Math.RandomFloat(0.3,0.5);
			
			hitPrecision = Math.RandomFloat(0.3,0.7);
		}
		
		if (isEndangering)
			hitPrecision -= Math.RandomFloat(0.1,0.3);
		
		if (hitPrecision < 0)
			hitPrecision = 0;
		
		if (!m_TraceParams)
		{
			m_TraceParams = new TraceParam();		
			m_TraceParams.Flags = TraceFlags.ENTS | TraceFlags.WORLD;
			m_TraceParams.LayerMask =  EPhysicsLayerDefs.Projectile;
		}
		
		ref array<IEntity> excludeArray = {m_Enemy, ownerEntity};
		m_TraceParams.ExcludeArray = excludeArray;
		m_TraceParams.End = end;
		
		m_TraceParams.Start = posGround + PRONE_OFFSET;
		hit = m_World.TraceMove(m_TraceParams, null);
		
#ifdef WORKBENCH
		DrawDebugTrace(hit);
#endif	
		if (hit >= hitPrecision)
		{
			stance = ECharacterStance.PRONE;			
			return true;
		}
		
		m_TraceParams.Start = posGround + KNEEL_OFFSET;
		hit = m_World.TraceMove(m_TraceParams, null);
		
#ifdef WORKBENCH
		DrawDebugTrace(hit);
#endif
		if (hit >= hitPrecision)
		{
			stance = ECharacterStance.CROUCH;			
			return true;
		}
		
		m_TraceParams.Start = posGround + STAND_OFFSET;
		hit = m_World.TraceMove(m_TraceParams, null);
		
#ifdef WORKBENCH
		DrawDebugTrace(hit);
#endif
		if (hit >= hitPrecision)
		{
			stance = ECharacterStance.STAND;
			return true;
		}
		
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	bool IsAgentNearby(vector hitNavmeshPos)
	{
		m_IsAgentNearby = false;
		
		m_AgentEntityNearby = null;
		
		GetGame().GetWorld().QueryEntitiesBySphere(hitNavmeshPos, 1, GetNearbyAgent);
		
		return m_IsAgentNearby;
	}
	
	//------------------------------------------------------------------------------------------------
	bool GetNearbyAgent(IEntity entity)
	{
		typename entityType = entity.Type();
		
		EntityPrefabData prefabData = entity.GetPrefabData();

#ifdef DCO_DEVELOPMENT
		
		string className = entity.ClassName();
		
		string entityString = entity.ToString();
		
		if (prefabData)
		{
			BaseContainer prefab = prefabData.GetPrefab();
			
			ResourceName prefabName = prefabData.GetPrefabName();
		}
		
#endif
		
		if (entityType == SCR_ChimeraCharacter)
		{
			if (entity == m_OwnerEntity)
				return true;
			
			m_IsAgentNearby = true;
			
			m_AgentEntityNearby = entity;
			
			return false;
		}
		else
		{
			vector min, max;
			
			ResourceName prefabName;
			
			entity.GetBounds(min, max);
			
			if (prefabData)
				prefabName = prefabData.GetPrefabName();
			
			if (m_ThreatSuppression < 0.9 && entityType == SCR_DestructibleEntity)
			{
				SCR_DestructibleEntityClass destructibleEntityPrefabData = SCR_DestructibleEntityClass.Cast(prefabData);
				
				if (destructibleEntityPrefabData)
				{
					if (destructibleEntityPrefabData.m_eMaterialSoundType == 15)
					{
						m_IsAgentNearby = true;
						
						m_AgentEntityNearby = entity;
					}
				}
			}
			
			return false;
		}
	}
	
	//------------------------------------------------------------------------------------------------
	bool IsCoverAvailable(vector hitNavmeshPos)
	{
		bool coverAvailable;
		
		SCR_DefendWaypoint defendWaypoint = SCR_DefendWaypoint.Cast(m_SCR_ChimeraAIAgent.m_GroupWaypoint);
		
		if (defendWaypoint)
		{
			if (defendWaypoint.IsWithinCompletionRadius(hitNavmeshPos))
				coverAvailable = true;
			else
				return false;
		}
		
		coverAvailable = true;
		
		float targetDistance = vector.Distance(hitNavmeshPos, m_DangerPosition);
		
		if (targetDistance < 50)
			return false;
		
		if (coverAvailable)
		{
			float coverDistance;
			
			vector coverPosition;
			
			if (m_CoverPositions.IsEmpty())
				return true;
			
			vector currentPosition = m_OwnerEntity.GetOrigin();
			
			int coverCount = m_CoverPositions.Count();
			
			for (int i = 0; i < coverCount; ++i)
			{
				coverPosition = m_CoverPositions[i];
				
				coverDistance = vector.Distance(coverPosition, hitNavmeshPos);
				
				if (coverDistance < 3)
				{					
					if (coverPosition == m_CurrentCoverPosition && m_ThreatSuppression > 0.1)
						continue;
					else
						return false;
				}
			}
		}
		
		return true;
	}
};