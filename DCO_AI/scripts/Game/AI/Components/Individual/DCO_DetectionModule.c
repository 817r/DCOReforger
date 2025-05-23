class DCO_AIDetectionSystemComponentClass : ScriptComponentClass
{
}

class DCO_AIDetectionSystemComponent : ScriptComponent
{	
	[Attribute("100", UIWidgets.Slider, "AI Detection Radius", params: "0 200 1" )]
	protected float m_fDetectionRadius;
	
	protected PerceptionComponent 					Perc;
	protected SCR_AIUtilityComponent 				m_Utility;
	protected SCR_AIInfoComponent					m_AIInfo;
	protected SCR_ChimeraAIAgent 					agent;
	
	protected float IntervalUpdate = 5000;
	protected float TimeStamp;
	
	protected ref array<IEntity> enemies = {};
	protected ref array<IEntity> allies = {};
	protected ref array<IEntity> DeadAllies = {};
	
	ref array<BaseTarget> hostiless = {};
	
	ref array<ref SCR_AITargetInfo> friendly = {};
	ref array<ref SCR_AITargetInfo> hostile = {};

	protected IEntity m_OwnerEntity;
	
	override void OnPostInit(IEntity owner)
    {		
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);
        SetEventMask(owner, EntityEvent.FRAME);
    }
	
	override void EOnInit(IEntity owner)
	{
		m_OwnerEntity = owner;	
		AIControlComponent ctrl = AIControlComponent.Cast(owner.FindComponent(AIControlComponent));
		if (ctrl)
		{
			agent = SCR_ChimeraAIAgent.Cast(ctrl.GetAIAgent());
			if (agent)
			{
				m_AIInfo = agent.m_InfoComponent;
				m_Utility = SCR_AIUtilityComponent.Cast(agent.FindComponent(SCR_AIUtilityComponent));
			}
		}
		Perc = PerceptionComponent.Cast(owner.FindComponent(PerceptionComponent));
		if (!Perc) Print("NO PERCEPTION COMPONENT ON DETECTION MODULE");
	}

    override void EOnFrame(IEntity owner, float timeSlice)
    {
		UpdateDetection(owner, timeSlice);
    }
	
	protected void UpdateDetection(IEntity owner, float timeSlice)
    {
		if (!m_Utility || !m_AIInfo) return;
		if (m_AIInfo.HasUnitState(EUnitState.UNCONSCIOUS)) return;
		
		getFriendlyEvaluation();
		enemyInfos(); 

		if (allies.Count() > 0) MaintainFriendly();
		if (enemies.Count() > 0) MaintainHostile();
		
		if (hostiless.Count() > 0)
			InfoShareCondition(timeSlice);
		
		// SCR_AIDebugVisualization.VisualizeMessage(owner,"Allies : " + allies.Count().ToString() + " |Enemies : " + enemies.Count().ToString(), EAIDebugCategory.INFO, 1.4);
		// MaintainDetected(owner);
    }
	
	protected bool InfoShareCondition(float timeSlice)
	{
		TimeStamp += timeSlice;
		if (TimeStamp > IntervalUpdate)
		{
			InfoShare();
			return true;
		}
		return false;
	}
	
	protected void InfoShare()
	{
		foreach (IEntity friends : allies)
		{
			AIControlComponent ctrl = AIControlComponent.Cast(friends.FindComponent(AIControlComponent));
			SCR_ChimeraAIAgent agents = SCR_ChimeraAIAgent.Cast(ctrl.GetAIAgent());
			SCR_AIUtilityComponent UtilityComp = agents.m_UtilityComponent;
			for (int i = hostiless.Count()-1; i >= 0; i--)
			{
				UtilityComp.targets.Insert(hostiless[i]);
				if (!m_Utility.targets.Contains(hostiless[i]))
					m_Utility.targets.Insert(hostiless[i]);
			}
		}
	}

    /*array<IEntity> GetNearbyEnemies(IEntity owner)
    {
		array<BaseTarget> tmp = {};
        Perc.GetTargetsList(tmp, ETargetCategory.ENEMY);
		foreach(BaseTarget tgt : tmp)
		{
			IEntity ents = tgt.GetTargetEntity();
			bool isOnDist = vector.Distance(ents.GetOrigin(), owner.GetOrigin()) < m_fDetectionRadius;

			if (!enemies.Contains(ents) && isOnDist)
				enemies.Insert(ents);
		}
		
        return enemies;
    }
	
	array<IEntity> GetNearbyAllies(IEntity owner)
    {
		array<BaseTarget> tmp = {};
        Perc.GetTargetsList(tmp, ETargetCategory.FRIENDLY);
		foreach(BaseTarget tgt : tmp)
		{
			IEntity ents = tgt.GetTargetEntity();
			bool isOnDist = vector.Distance(ents.GetOrigin(), owner.GetOrigin()) < m_fDetectionRadius;

			if (!allies.Contains(ents) && isOnDist)
				allies.Insert(ents);
		}
		
        return allies;
    }
	
	void MaintainDetected(IEntity owner)
	{
		for(int i = allies.Count() - 1; i < 1; i--)
		{
			IEntity temp = allies.Get(i);
			bool isOutDist = vector.Distance(temp.GetOrigin(), owner.GetOrigin()) > m_fDetectionRadius;
			
			DamageManagerComponent eDamage = DamageManagerComponent.Cast(temp.FindComponent(DamageManagerComponent));
			
			if (eDamage.IsDestroyed() || isOutDist)
			{
				allies.Remove(i);
			}
		}
		
		for(int i = enemies.Count() - 1; i < 1; i--)
		{
			IEntity temp = enemies.Get(i);
			bool isOutDist = vector.Distance(temp.GetOrigin(), owner.GetOrigin()) > m_fDetectionRadius;
			
			DamageManagerComponent eDamage = DamageManagerComponent.Cast(temp.FindComponent(DamageManagerComponent));
			
			if (eDamage.IsDestroyed() || isOutDist)
			{
				enemies.Remove(i);
			}
		}
		
		SCR_AIDebugVisualization.VisualizeMessage(owner,"Allies : " + allies.Count().ToString() + " |Enemies : " + enemies.Count().ToString(), EAIDebugCategory.INFO, 1.4);
	}
	
	
	bool QueryCallbackFriendly()
	{
		if (e == OwnerEnt) return true;
		
		SCR_CharacterFactionAffiliationComponent ownerFaction = SCR_CharacterFactionAffiliationComponent.Cast(OwnerEnt.FindComponent(SCR_CharacterFactionAffiliationComponent));
		SCR_CharacterFactionAffiliationComponent comp = SCR_CharacterFactionAffiliationComponent.Cast(e.FindComponent(SCR_CharacterFactionAffiliationComponent));
		ChimeraAIControlComponent eCont = ChimeraAIControlComponent.Cast(e.FindComponent(ChimeraAIControlComponent));
		
		if (!comp || !eCont) return true;
		
		if (comp.GetAffiliatedFaction() == ownerFaction.GetAffiliatedFaction())
		{
			if (!allies.Contains(e))
				allies.Insert(e);
			
			DamageManagerComponent eDamage = DamageManagerComponent.Cast(e.FindComponent(DamageManagerComponent));
			
			if (eDamage.IsDestroyed())
				allies.RemoveItem(e);
			
			return true;
		}
		return true;
	}
	*/
	
	protected void getFriendlyEvaluation()
	{
		ref array<BaseTarget> detected = new array<BaseTarget>;
		Perc.GetTargetsList(detected, ETargetCategory.FRIENDLY);
		foreach (BaseTarget target : detected)
		{
			IEntity ent = target.GetTargetEntity();
			if(!ent) 
				return;
			
			SCR_AITargetInfo targetInfo = new SCR_AITargetInfo();
			targetInfo.InitFromBaseTarget(target);
			
			if(!targetInfo)
				return;
			
			if (target.GetDistance() < m_fDetectionRadius)
			{
				if (!allies.Contains(ent))
				{
					allies.Insert(ent);
					friendly.Insert(targetInfo);
				}				
			}
		}
	}
	
	protected void MaintainFriendly()
	{		
		for (int i = friendly.Count()-1; i >= 0; i--)
		{
			SCR_AITargetInfo tarinfo = friendly[i];
			if(!tarinfo.m_Entity)
			{
				friendly.Remove(i);
				allies.Remove(i);
				return;
			}		
			
			bool destroyed = tarinfo.m_DamageManager.IsDestroyed();
			if (destroyed)
			{
				if (!DeadAllies.Contains(allies.Get(i)))
				{
					DeadAllies.Insert(allies.Get(i));
					friendly.Remove(i);
					allies.Remove(i);
				}
				return;
			}
			
			if (vector.Distance(tarinfo.m_Entity.GetOrigin(), m_OwnerEntity.GetOrigin()) > m_fDetectionRadius + 50)
			{
				allies.Remove(i);
				friendly.Remove(i);
				return;
			}
		}
	}		
	
	protected void enemyInfos()
	{
		ref array<BaseTarget> hostiles = new array<BaseTarget>;
		Perc.GetTargetsList(hostiles, ETargetCategory.ENEMY);
		foreach (BaseTarget target : hostiles)
		{
			IEntity ent = target.GetTargetEntity();
			if(!ent) 
				return;
			
			SCR_AITargetInfo targetInfo = new SCR_AITargetInfo();
			targetInfo.InitFromBaseTarget(target);
			
			if(!targetInfo)
				return;
			
			if (target.GetDistance() < m_fDetectionRadius)
			{
				if (!enemies.Contains(ent))
				{
					hostiless.Insert(target);
					enemies.Insert(ent);
					hostile.Insert(targetInfo);
				}
			}
		}
	}
	
	protected void MaintainHostile()
	{		
		for (int i = hostile.Count()-1; i >= 0; i--)
		{
			SCR_AITargetInfo tarinfo = hostile[i];
			if(!tarinfo.m_Entity)
			{
				hostiless.Remove(i);
				hostile.Remove(i);
				enemies.Remove(i);
				return;
			}		
			
			bool destroyed = tarinfo.m_DamageManager.IsDestroyed();
			if (destroyed)
			{
				if (!DeadAllies.Contains(enemies.Get(i)))
				{
					DeadAllies.Insert(enemies.Get(i));
					hostiless.Remove(i);
					hostile.Remove(i);
					enemies.Remove(i);
				}
				return;
			}
			
			if (vector.Distance(tarinfo.m_Entity.GetOrigin(), m_OwnerEntity.GetOrigin()) > m_fDetectionRadius + 20)
			{
				hostiless.Remove(i);
				enemies.Remove(i);
				hostile.Remove(i);
				return;
			}
		}
	}	
	
	int GetFriendlyNumber()
	{
		return allies.Count();
	}
	
	int GetEnemiesNumber()
	{
		return enemies.Count();
	}
	
	bool GetDeadAllies(out array<IEntity> friendlys)
	{
		friendlys = DeadAllies;
		return true;
	}
}