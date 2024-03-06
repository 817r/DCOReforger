//------------------------------------------------------------------------------------------------
modded class SCR_AICombatComponent : ScriptComponent
{
	protected DCO_ECombatMovementType m_CombatMovementType;
	
	protected SCR_AIGroupUtilityComponent m_SCR_AIGroupUtilityComponent;
	protected ref SCR_AIGroupFireteamManager m_SCR_AIGroupFireteamManager;
	
	
	//------------------------------------------------------------------------------------------------
	vector FindNextMovePosition(SCR_AIGroupUtilityComponent aiGroupUtilityComponent)
	{
		if (m_SelectedTarget)
		{
			vector direction;
			float nextMoveDistance;
			bool standardAttack = true;
			
			float defendWaypointCompletionRadius;
			
			vector ownerPos = GetOwner().GetOrigin();
			
			vector lastSeenPosition = m_SelectedTarget.GetLastSeenPosition();
			float distanceToTarget = vector.Distance(ownerPos, lastSeenPosition);

			SCR_DefendWaypoint defendWaypoint = SCR_DefendWaypoint.Cast(m_SCR_ChimeraAIAgent.m_GroupWaypoint);
			
			if (defendWaypoint)
			{
				defendWaypointCompletionRadius = defendWaypoint.GetCompletionRadius();
				
				if (defendWaypoint.IsWithinCompletionRadius(ownerPos) && defendWaypoint.IsWithinCompletionRadius(lastSeenPosition))
					standardAttack = true;
				else
				{
					standardAttack = false;
					
					direction = vector.Direction(ownerPos, defendWaypoint.GetOrigin());
					
					float defendWaypointDistance = vector.DistanceXZ(ownerPos, defendWaypoint.GetOrigin());
					
					if (defendWaypointDistance < defendWaypointCompletionRadius)
						nextMoveDistance = 0;
					else
						nextMoveDistance = defendWaypointDistance;
				}
			}
			
			if (standardAttack)
				direction = vector.Direction(ownerPos, m_SelectedTarget.GetLastSeenPosition());
			
			direction.Normalize();
			
			vector nextMovePosition;
			
			if (standardAttack)
				nextMovePosition = GetStandardMovePosition(ownerPos, lastSeenPosition, distanceToTarget);
			else
			{
				float nearProximity = Math.RandomFloat(5,15);
				
				nearProximity = defendWaypointCompletionRadius;
				
				if (nearProximity < 1)
					nearProximity = 1;
				
				vector newPositionCenter = direction * nextMoveDistance + ownerPos, newPosition;
				
				newPositionCenter = defendWaypoint.GetOrigin();
				
				newPosition = s_AIRandomGenerator.GenerateRandomPointInRadius(0, nearProximity, newPositionCenter, true);
								
				nextMovePosition = newPosition;
			}
			return nextMovePosition;
		}
		
		return vector.Zero;
	}
	
	vector GetStandardMovePosition(vector ownerPos, vector lastSeenPosition, float distanceToTarget)
	{	
		float degreeBase;
		
		float combatDefendChance;
		
		float degreeAngle = Math.RandomFloat(0,65);
		
		float moveDistance = Math.RandomFloat(10,25);
		
		BaseTarget currentTarget = GetCurrentTarget();
		
		EWeaponType currentWeaponType = GetCurrentWeaponType();
		
		degreeBase = 180;

		if (Math.RandomFloat(0,100) < 50)
			degreeBase -= degreeAngle;
		else
			degreeBase += degreeAngle;
		
		vector newPosition = DCO_Math.GetPositionFromYaw(degreeBase, ownerPos, lastSeenPosition, moveDistance);
		
		return newPosition;
	}
};