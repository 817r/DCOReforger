modded class SCR_AIGroupPerception : Managed
{
	// Target is considered lost if noone has seen it for this time.
	// This value should be consistent with actual duration of AI combat behavior (see TARGET_MAX_LAST_SEEN)
	const float TARGET_LOST_THRESHOLD_S = 60.0;
	
	// Time till target is totally forgotten and is removed from memory.
	// This doesn't need to be much longer than MAX_CLUSTER_AGE_S
	const float TARGET_FORGET_THRESHOLD_S = 170.0;	
	
	override protected void RemoveTarget(IEntity enemy)
	{
		if (!enemy)
			return;
		
		int index = m_aTargetEntities.Find(enemy);
		
		if (index > -1)
		{
			m_aTargetEntities.Remove(index);
			m_aTargets.Remove(index);
			m_Utility.tempTarget.Remove(index);
			if (m_aTargetEntities.IsEmpty() && Event_OnNoEnemy)
				Event_OnNoEnemy.Invoke(m_Group);
		}
	}
	
	//---------------------------------------------------------------------------------------------------
	override protected void RemoveTarget(int id)
	{
		m_aTargetEntities.Remove(id);
		m_aTargets.Remove(id);
		m_Utility.tempTarget.Remove(id);
		
		if (m_aTargetEntities.IsEmpty() && Event_OnNoEnemy)
			Event_OnNoEnemy.Invoke(m_Group);
	}
	
	//---------------------------------------------------------------------------------------------------
	// Adds or updates target from BaseTarget
	override SCR_AITargetInfo AddOrUpdateTarget(notnull BaseTarget target, out bool outNewTarget)
	{
		IEntity enemy = target.GetTargetEntity();
		
		if (!enemy)
		{
			outNewTarget = false;
			return null;
		}
			
		int id = m_aTargetEntities.Find(enemy);
		if (id > -1)
		{
			SCR_AITargetInfo oldTargetInfo = m_aTargets[id];
			EAITargetInfoCategory oldCategory = m_aTargets[id].m_eCategory;
			
			// Ignore if destroyed or disarmed
			if (oldCategory == EAITargetInfoCategory.DESTROYED || oldCategory == EAITargetInfoCategory.DISARMED)
			{
				outNewTarget = false;
				return oldTargetInfo;
			}
				
			// This target was already found
			// Which newTimestamp to use? Depends on target category
			float newTimestamp;
			ETargetCategory targetCategory = target.GetTargetCategory();
			if (targetCategory == ETargetCategory.DETECTED)
				newTimestamp = target.GetTimeLastDetected();
			else
				newTimestamp = target.GetTimeLastSeen();
			
			if (oldTargetInfo.m_fTimestamp < newTimestamp)
			{
				// New information is newer
				// Is new data more relevant?
				if ((targetCategory == ETargetCategory.ENEMY) || // If enemy, always update
					((targetCategory == ETargetCategory.DETECTED) && (oldCategory != EAITargetInfoCategory.IDENTIFIED)) )
				{
					oldTargetInfo.UpdateFromBaseTarget(target);
				}
			}
			
			outNewTarget = false;
			return oldTargetInfo;
		}		
		
		// new enemy found
		
		// Ignore if disarmed
		if (target.IsDisarmed())
		{
			outNewTarget = false;
			return null;
		}
			
		SCR_AITargetInfo targetInfo = new SCR_AITargetInfo();
		targetInfo.InitFromBaseTarget(target);
				
		m_aTargetEntities.Insert(enemy);
		m_aTargets.Insert(targetInfo);
		m_Utility.tempTarget.Insert(enemy);
		
		if (Event_OnEnemyDetected)
		{
			Event_OnEnemyDetected.Invoke(m_Group, targetInfo);
		}
		
		outNewTarget = true;
		return targetInfo;
	}
	
	//---------------------------------------------------------------------------------------------------
	override void AddOrUpdateGunshot(notnull IEntity shooter, vector worldPos, float timestamp, bool endangering)
	{
		int id = m_aTargetEntities.Find(shooter);
		if (id > -1)
		{
			// Update data
			SCR_AITargetInfo oldTargetInfo = m_aTargets[id];
			if ((oldTargetInfo.m_eCategory != EAITargetInfoCategory.IDENTIFIED) &&	// Update only if it wasn't seen yet
				(timestamp > oldTargetInfo.m_fTimestamp))					// Update only if new data is newer
			{
				oldTargetInfo.UpdateFromGunshot(worldPos, timestamp, endangering);
			}
			
			// Update endangering flag
			oldTargetInfo.m_bEndangering |= endangering;
		}
		else
		{
			// Create new data
			PerceivableComponent perceivable = PerceivableComponent.Cast(shooter.FindComponent(PerceivableComponent));
			
			// Ignore aircrafts. Fix to prevent suppression of aircrafts,
			// but also there is no reason to track aircrafts in group perception.
			if (perceivable && perceivable.GetUnitType() == EAIUnitType.UnitType_Aircraft)
				return;
			
			SCR_AITargetInfo targetInfo = new SCR_AITargetInfo();
			targetInfo.InitFromGunshot(shooter, perceivable, worldPos, timestamp, endangering);
			
			m_aTargets.Insert(targetInfo);
			m_aTargetEntities.Insert(shooter);
			m_Utility.tempTarget.Insert(shooter);
		}
	}
	
	override void Update()
	{
		vanilla.Update();
	}
	
	void getTargetClusterArray(out array<SCR_AIGroupTargetCluster> gtcarr)
	{
		//gtcarr = m_aTargetClusters;
	}
}