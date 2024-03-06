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
			
			if (m_DCO_AIInfoComponent && m_DCO_AIInfoComponent.GetHoldPosition())
				nextMovePosition = GetHoldMovePosition(ownerPos, nextMovePosition);
			
			nextMovePosition[1] = GetOwner().GetWorld().GetSurfaceY(nextMovePosition[0], nextMovePosition[2]);
			
			return nextMovePosition;
		}
		
		return vector.Zero;
	}
	
	//------------------------------------------------------------------------------------------------
	vector GetStandardMovePosition(vector ownerPos, vector lastSeenPosition, float distanceToTarget)
	{
		int shapeColor;
		
		float degreeBase;
		
		float combatDefendChance;
		
		float degreeAngle = Math.RandomFloat(0,65);
		
		float moveDistance = Math.RandomFloat(5,15);
		
		BaseTarget currentTarget = GetCurrentTarget();
		
		EWeaponType currentWeaponType = GetCurrentWeaponType();
		
		
		if (currentWeaponType == EWeaponType.WT_MACHINEGUN || currentWeaponType == EWeaponType.WT_SNIPERRIFLE)
		{
			
			if (currentTarget)
			{
				float timeSinceSeen = currentTarget.GetTimeSinceSeen();
			}
		}
		
		degreeBase = 180;

		if (Math.RandomFloat(0,100) < 50)
			degreeBase -= degreeAngle;
		else
			degreeBase += degreeAngle;
		
		vector newPosition = DCO_Math.GetPositionFromYaw(degreeBase, ownerPos, lastSeenPosition, moveDistance);
		
		return newPosition;
	}
	
	//------------------------------------------------------------------------------------------------
	vector GetHoldMovePosition(vector ownerPos, vector nextMovePosition)
	{
		int holdPositionRadius = 80;
		
		if (holdPositionRadius < 1)
			holdPositionRadius = 1;
		
		vector holdPositionOrigin = m_DCO_AIInfoComponent.GetHoldPositionOrigin();
		
		float holdDistanceOwnerPosition = vector.DistanceXZ(ownerPos, holdPositionOrigin);
		
		float holdDistanceNextMovePosition = vector.DistanceXZ(nextMovePosition, holdPositionOrigin);
		
		if (holdPositionRadius < holdDistanceOwnerPosition || holdPositionRadius < holdDistanceNextMovePosition)
		{
			vector newPosition = s_AIRandomGenerator.GenerateRandomPointInRadius(0, holdPositionRadius, holdPositionOrigin, true);
			
			nextMovePosition = newPosition;
		}
		else
		{
			nextMovePosition = holdPositionOrigin;
		}
		
		return nextMovePosition;
	}
	
/*	//------------------------------------------------------------------------------------------------
	void GetFireTeamMovePosition(out float degreeBase, out float moveDistance)
	{
		AIAgent fireteamLeader;
		
		SCR_AIGroupFireteam fireteam = m_SCR_ChimeraAIAgent.GetFireteam();
		
		if (fireteam)
			fireteamLeader = fireteam.GetLeader();
		
		if (fireteamLeader && fireteamLeader == m_SCR_ChimeraAIAgent)
		{			
			if (m_SCR_AIGroupFireteamManager == null)
			{
				AIGroup parentGroup = m_SCR_ChimeraAIAgent.m_ParentGroup;
				
				SCR_AIGroupUtilityComponent aiGroupUtilityComponent = SCR_AIGroupUtilityComponent.Cast(parentGroup.FindComponent(SCR_AIGroupUtilityComponent));
				
				if (aiGroupUtilityComponent)
					m_SCR_AIGroupFireteamManager = aiGroupUtilityComponent.m_FireteamMgr;
			}
			
			if (m_SCR_AIGroupFireteamManager)
			{				
				array<ref SCR_AIGroupFireteam> fireteams = m_SCR_AIGroupFireteamManager.GetFT();
				
				if (fireteams.IsEmpty())
					return;
				
				int fireteamsCount = fireteams.Count();
				
				int fireteamIndex = fireteams.Find(fireteam);
				
				IEntity fireTeamLeaderEntity = fireteamLeader.GetControlledEntity();
				
				IEntity fireTeamZeroLeaderEntity = fireteams[0].GetLeader().GetControlledEntity();
				
				switch (fireteamIndex)
				{
					case 0:
					{
						degreeBase = 0;
						
						moveDistance = Math.RandomFloat(3,20);
						
						if (fireteamsCount > 1)
						{
							IEntity fireteamOneLeaderEntity = fireteams[1].GetLeader().GetControlledEntity();
							
							if (fireteamOneLeaderEntity)
							{
								float fireteamOneDistance = vector.Distance(fireTeamZeroLeaderEntity.GetOrigin(), fireteamOneLeaderEntity.GetOrigin());
								
								if (fireteamsCount > 2)
								{
									IEntity fireteamTwoLeaderEntity = fireteams[2].GetLeader().GetControlledEntity();
									
									if (fireteamTwoLeaderEntity)
									{
										float fireteamTwoDistance = vector.Distance(fireTeamZeroLeaderEntity.GetOrigin(), fireteamTwoLeaderEntity.GetOrigin());
										
										if (fireteamOneDistance > fireteamTwoDistance)
											degreeBase = 80;
										else
											degreeBase = -80;
									}
								}
							}
						}
						
						break;
					}
					case 1:
					{
						degreeBase = 80;
						
						moveDistance = Math.RandomFloat(3,15);
						
						break;
					}
					case 2:
					{
						degreeBase = -80;
						
						moveDistance = Math.RandomFloat(3,15);
						
						break;
					}
				}
				
				if (degreeBase > 0)
					degreeBase += Math.RandomFloat(0,15);
				else if (degreeBase < 0)
					degreeBase -= Math.RandomFloat(0,15);
			}
		}
	}*/
};