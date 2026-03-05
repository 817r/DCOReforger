[BaseContainerProps()]
modded class SCR_AIDangerReaction_WeaponFired
{
	override bool PerformReaction(notnull SCR_AIUtilityComponent utility, notnull SCR_AIThreatSystem threatSystem, AIDangerEvent dangerEvent, int dangerEventCount)
	{
		AIDangerEventWeaponFire eventWeaponFire = AIDangerEventWeaponFire.Cast(dangerEvent);		
		IEntity shooter = eventWeaponFire.GetObject();
		
		if (!shooter || !eventWeaponFire)
			return false;
		
		IEntity instigatorEntity = eventWeaponFire.GetInstigatorEntity();
		if (!instigatorEntity)
			return false;
		
		// Check faction relations, ignore if not enemy or there is no faction
		Faction instigatorFaction = SCR_AIFactionHandling.GetEntityPerceivedFaction(instigatorEntity);
		
		if (!instigatorFaction)
			return false;
		
		SCR_ChimeraAIAgent agent = SCR_ChimeraAIAgent.Cast(utility.GetOwner());
		
		bool myFactionIsMilitary = utility.IsMilitary();
		if (myFactionIsMilitary && !agent.IsEnemy(instigatorFaction))
			return false;
		
		// Get root entity of shooter, in case it is turret in vehicle hierarchy
		vector shotPos = eventWeaponFire.GetPosition();
		vector shotDir = eventWeaponFire.GetDirection();
		bool isShotSuppressed = eventWeaponFire.IsSuppressed();
		
		vector myOrigin = utility.m_OwnerEntity.GetOrigin();
		float distance = vector.Distance(myOrigin, shotPos);
		
		// Is it a flyby?
		bool isFlyby = IsFlyby(myOrigin, shotPos, shotDir, distance);
		
		bool isAudible = IsAudiable(distance, isShotSuppressed);
		
		
		float timeTillFlyby_s = float.MAX;
		float timeTillGunshotHeard_s = float.MAX;
		if (isFlyby)
		{
			float projectileSpeed = eventWeaponFire.GetInitialSpeed();
			WorldTimestamp eventTimestamp = eventWeaponFire.GetTimestamp();
			float flightTime_s = distance / projectileSpeed;
			WorldTimestamp flybyTimestamp = eventTimestamp.PlusSeconds(flightTime_s + PROJECTILE_FLYBY_DELAY_S);
			timeTillFlyby_s = flybyTimestamp.DiffSeconds(GetGame().GetWorld().GetTimestamp());
			
			if (timeTillFlyby_s < 0)
				OnProjectileFlyby(utility, dangerEventCount, shotPos);
			else
			{
				utility.GetCallqueue().CallLater(OnProjectileFlyby, 1000*timeTillFlyby_s, false,
					utility, dangerEventCount, shotPos);
			}
		}
		
		if (isAudible)
		{
			WorldTimestamp eventTimestamp = eventWeaponFire.GetTimestamp();
			float wavefrontTravelTime_s = distance / SOUND_SPEED_MS;
			WorldTimestamp wavefrontArrivalTimestamp = eventTimestamp.PlusSeconds(wavefrontTravelTime_s);
			timeTillGunshotHeard_s = wavefrontArrivalTimestamp.DiffSeconds(GetGame().GetWorld().GetTimestamp());
			
			// Ignore the gunshot sound if projectile flies by sooner than sound of gunshot.
			// This is general case for majority of weapons. We don't want to notify the threat system twice.
			// Threat system was not tuned to be notified twice in such cases.
			bool ignoreGunshotHeard = isFlyby && timeTillFlyby_s < timeTillGunshotHeard_s;
			
			if (!ignoreGunshotHeard)
			{
				if (timeTillGunshotHeard_s < 0)
					OnGunshotHeard(utility, distance, dangerEventCount, shotPos);
				else
				{
					utility.GetCallqueue().CallLater(OnGunshotHeard, 1000*timeTillGunshotHeard_s, false,
						utility, distance, dangerEventCount, shotPos);
				}
			}
		}
		
		if (isFlyby || isAudible)
		{
			// Notify our group, only if we are a leader
			bool endangeringForGroup = isFlyby || distance < ENDANGERING_FOR_GROUP_RADIUS;
			
			AIGroup myGroup = utility.GetOwner().GetParentGroup();
			if (myGroup && myGroup.GetLeaderAgent() == agent)
			{
				float timeTillGroupNotified_s = Math.Min(timeTillFlyby_s, timeTillGunshotHeard_s);
				
				if (timeTillGroupNotified_s < 0)
					NotifyGroup(myGroup, shooter, instigatorEntity, instigatorFaction, shotPos, endangeringForGroup);
				else
				{
					utility.GetCallqueue().CallLater(NotifyGroup, 1000*timeTillGroupNotified_s, false,
						myGroup, shooter, instigatorEntity, instigatorFaction, shotPos, endangeringForGroup);
				}
			}
			
			if (endangeringForGroup)
			{
				float radius = Math.Map(distance, 50, SCR_AICombatComponent.LONG_RANGE_COMBAT_DISTANCE, 3, 10);
				SCR_AISuppressionVolumeSphere createSupp = new SCR_AISuppressionVolumeSphere(shotPos, radius);
				
				if (utility.m_CombatComponent.GetSelectedWeaponType() == EWeaponType.WT_MACHINEGUN)
				{
					SCR_AISuppressBehavior supp = new SCR_AISuppressBehavior(utility, null, createSupp, 3 * radius, 2.5, 2000);
					utility.AddAction(supp);
				}
			}
			
		}
		
		if (isShotSuppressed && distance < 50)
		{
			auto investigaste = new SCR_AIMoveAndInvestigateBehavior(utility, null, shotPos,
			SCR_AIActionBase.PRIORITY_BEHAVIOR_MOVE_AND_INVESTIGATE, SCR_AIActionBase.PRIORITY_LEVEL_NORMAL, isDangerous: true, radius: 25, targetUnitType: EAIUnitType.UnitType_Infantry, duration: 150); 
			
			utility.AddAction(investigaste);
		}
		
		return true;
	}
	
	protected bool IsAudiable(float dist, bool isSuppressed)
	{
		float maxAudibleDistance;
		if (isSuppressed)
			maxAudibleDistance = AUDIBLE_DISTANCE_SUPPRESSED;
		else
			maxAudibleDistance = AUDIBLE_DISTANCE_NORMAL;
		
		if (isSuppressed && dist > 70)
			return Math.RandomFloat01() > 0.65;
		
		return dist < maxAudibleDistance;
	}
}