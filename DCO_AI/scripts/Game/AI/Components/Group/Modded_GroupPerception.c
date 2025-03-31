modded class SCR_AIGroupPerception : Managed
{
	// Target is considered lost if noone has seen it for this time.
	// This value should be consistent with actual duration of AI combat behavior (see TARGET_MAX_LAST_SEEN)
	const float TARGET_LOST_THRESHOLD_S = 30.0;
	
	// Time till target is totally forgotten and is removed from memory.
	// This doesn't need to be much longer than MAX_CLUSTER_AGE_S
	const float TARGET_FORGET_THRESHOLD_S = 180.0;	
	
	void UpdateFromFriendlys(BaseTarget target,SCR_AIInfoComponent infoComp)
	{
		if (!target) return;
		int id = m_Utility.m_aInfoComponents.Find(infoComp);
		IEntity enemy = target.GetTargetEntity();
		bool targetIsNew;
		SCR_AITargetInfo targetInfo = AddOrUpdateTarget(target, targetIsNew);
		bool invokedEvent = false;
		
		if (targetIsNew && !invokedEvent)
		{
			if (Event_OnEnemyDetectedFiltered)
			{
				AIAgent reporter = AIAgent.Cast(infoComp.GetOwner());
				Event_OnEnemyDetectedFiltered.Invoke(m_Group, targetInfo, reporter);
			}
				
			invokedEvent = true;
		}			
		else return; 
	
	}
}