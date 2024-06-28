modded class SCR_AIGroupPerception : Managed
{
	// Target is considered lost if noone has seen it for this time.
	// This value should be consistent with actual duration of AI combat behavior (see TARGET_MAX_LAST_SEEN)
	const float TARGET_LOST_THRESHOLD_S = 60.0;
	
	// Time till target is totally forgotten and is removed from memory.
	// This doesn't need to be much longer than MAX_CLUSTER_AGE_S
	const float TARGET_FORGET_THRESHOLD_S = 170.0;

	//---------------------------------------------------------------------------------------------------
	override protected void UpdateTargetsFromMembers()
	{
		IEntity leaderEntity = m_Group.GetLeaderEntity();
		
		// Makes no sense without a leader
		if (!leaderEntity)
			return;
		
		// Update all targets from our group members
		array<AIAgent> agents = {};
		m_Group.GetAgents(agents);
		
		array<BaseTarget> targets = {};
		
		bool targetIsNew;
		SCR_AITargetInfo targetInfo;
		bool invokedEvent = false;
		
		foreach (SCR_AIInfoComponent infoComp : m_Utility.m_aInfoComponents)
		{
			PerceptionComponent perception = infoComp.m_Perception;
			
			perception.GetTargetsList(targets, ETargetCategory.DETECTED);
			foreach (BaseTarget baseTarget : targets)
			{
				targetInfo = AddOrUpdateTarget(baseTarget, targetIsNew);
				
				if (targetIsNew && !invokedEvent)
				{
					if (Event_OnEnemyDetectedFiltered)
					{
						AIAgent reporter = AIAgent.Cast(infoComp.GetOwner());
						Event_OnEnemyDetectedFiltered.Invoke(m_Group, targetInfo, reporter);
					}
					invokedEvent = true;
				}
			}
			
			perception.GetTargetsList(targets, ETargetCategory.ENEMY);
			foreach (BaseTarget baseTarget : targets)
			{
				targetInfo = AddOrUpdateTarget(baseTarget, targetIsNew);
				
				if (targetIsNew && !invokedEvent)
				{
					if (Event_OnEnemyDetectedFiltered)
					{
						AIAgent reporter = AIAgent.Cast(infoComp.GetOwner());
						Event_OnEnemyDetectedFiltered.Invoke(m_Group, targetInfo, reporter);
					}
					invokedEvent = true;
				}
			}
		}
	}
}