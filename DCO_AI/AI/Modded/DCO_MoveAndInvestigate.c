modded class SCR_AIMoveAndInvestigateBehavior : SCR_AIMoveBehaviorBase
{
	protected ref Shape m_DebugShape;
	
	protected const float MAX_THREAT_THRESHOLD = 0.2000;
	
	protected DCO_AIInfoGroupComponent m_DCO_AIGroupInfoComponent;

	override void OnActionSelected()
	{
		bool investigate;
		
		int investigateRadius;
		
		super.OnActionSelected();
		
		SCR_ChimeraAIAgent chimeraAIAgent = m_Utility.m_ChimeraAIAgent;
		
		DCO_AIInfoComponent aiInfoComponen = m_Utility.m_DCO_AIInfoComponent;
		
		float threatEndangered = m_Utility.m_ThreatSystem.GetThreatEndangered();
		
		if (threatEndangered > 0)
		{
			IEntity controlledEntity = chimeraAIAgent.GetControlledEntity();
			
			if (controlledEntity)
			{
				vector origin = controlledEntity.GetOrigin();
				
				float threatTotal = m_Utility.m_ThreatSystem.GetThreatTotal();
				
				float distance = vector.Distance(origin, m_vPosition.m_Value);
				
				vector direction = vector.Direction(origin, m_vPosition.m_Value);
				
				float investigateDistanceModifier = 1;
				
				investigateDistanceModifier += threatTotal;
				
				investigateDistanceModifier += threatEndangered;
				
				distance /= 2;
				
				direction.Normalize();
				
				vector investigateCenterPosition = origin + distance * direction;
				
				investigateRadius = vector.Distance(m_vPosition.m_Value, investigateCenterPosition);
				
				vector investigateMovePosition = s_AIRandomGenerator.GenerateRandomPointInRadius(5, 15, investigateCenterPosition, true);
				
				investigateMovePosition = s_AIRandomGenerator.GenerateRandomPointInRadius(0, investigateRadius, investigateCenterPosition, true);
				
				investigateMovePosition[1] = GetGame().GetWorld().GetSurfaceY(investigateMovePosition[0], investigateMovePosition[2]);
				
				m_fRadius.Init(this, investigateRadius);
				
				m_vPosition.Init(this, investigateMovePosition);
			}
		}
		
		if (aiInfoComponen)
		{
			bool holdPosition = aiInfoComponen.GetHoldPosition();
			
			if (holdPosition)
			{
				Fail();
				
				return;
			}
			
			bool disableMovementControls = aiInfoComponen.GetDisableMovementControls();
			
			if (disableMovementControls)
			{
				Fail();
				return;
			}
		}
		
		if (chimeraAIAgent)
		{
			AIGroup parentGroup = chimeraAIAgent.GetParentGroup();
			
			if (chimeraAIAgent)
			{
				m_DCO_AIGroupInfoComponent = chimeraAIAgent.m_DCO_AIGroupInfoComponent;
				
				if (m_DCO_AIGroupInfoComponent)
				{
					investigate = m_DCO_AIGroupInfoComponent.GetInvestigate();
					
					if (investigate)
						investigateRadius = m_DCO_AIGroupInfoComponent.GetInvestigateRadius();
				}
			}

		}

		if (investigate)
		{
			float minRadius, maxRadius;
			
			GetInvestigationRadius(minRadius, maxRadius);
			
			float radius = m_fRadius.m_Value;
			
			if (investigateRadius > -1)
			{
				if (investigateRadius == 0)
					investigateRadius = 1;
				
				radius = investigateRadius;
			}
			
			m_fRadius.Init(this, radius);
			
			vector investigatePositionCenter = m_vPosition.m_Value;
			
			threatEndangered = m_Utility.m_ThreatSystem.GetThreatEndangered();
			
			vector investigatePositionMove = s_AIRandomGenerator.GenerateRandomPointInRadius(minRadius, maxRadius, investigatePositionCenter, true);

			investigatePositionMove[1] = GetGame().GetWorld().GetSurfaceY(investigatePositionCenter[0], investigatePositionCenter[2]);
			
			m_vPosition.Init(this, investigatePositionMove);

		}
		else
		{
			Fail();
		}
	}
	
	//------------------------------------------------------------------------------------------------
	void GetInvestigationRadius(out float minRadius, out float maxRadius)
	{
		string baseTargetCategory = "NONE";
		
		minRadius = Math.RandomFloat(0,5);
		
		maxRadius = Math.RandomFloat(5,15);

		PerceptionComponent perceptionComponent = m_Utility.m_PerceptionComponent;
		
		if (perceptionComponent)
		{
			BaseTarget baseTarget = perceptionComponent.GetLastSeenTarget(ETargetCategory.ENEMY, float.MAX);
			
			if (baseTarget)
			{
				baseTargetCategory = "ENEMY";
				
				minRadius = Math.RandomFloat(20,30);
				
				maxRadius = Math.RandomFloat(35,70);
			}
			else
			{
				baseTarget = perceptionComponent.GetLastSeenTarget(ETargetCategory.DETECTED, float.MAX);
				
				if (baseTarget)
				{
					baseTargetCategory = "DETECTED";
					
					minRadius = Math.RandomFloat(5,15);
					
					maxRadius = Math.RandomFloat(15,40);
				}
				else
				{
					baseTarget = perceptionComponent.GetLastSeenTarget(ETargetCategory.UNKNOWN, float.MAX);
					
					if (baseTarget)
					{
						baseTargetCategory = "UNKNOWN";
						
						minRadius += Math.RandomFloat(0,5);
						
						maxRadius += Math.RandomFloat(5,15);
					}
				}
			}
		}
	}
};