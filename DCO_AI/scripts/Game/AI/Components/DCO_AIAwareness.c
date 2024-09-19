class DCO_AIAwarenessClass : ScriptComponentClass
{
}

class DCO_AIAwareness : ScriptComponent
{
	protected SCR_AIInfoComponent m_InfoComp;
	protected SCR_AIUtilityComponent m_UtilityComp;
	protected SCR_AICombatComponent m_CombatComp;
	protected PerceptionComponent m_Perceptions;
	//protected CharacterVicinityComponent m_charVic;
	protected SCR_InventoryStorageManagerComponent inventorys;
	
	ref array<ref SCR_AITargetInfo> friendly = {};
	ref array<ref SCR_AITargetInfo> hostile = {};
	ref array<IEntity> friendlyBTarget = {};
	ref array<IEntity> hostileEnt = {};
	
	[Attribute(defvalue: "150", uiwidget: UIWidgets.Auto, desc: "Awareness Friendly Radius")]
	float searchRad;
	
	[Attribute(defvalue: "200", uiwidget: UIWidgets.Auto, desc: "Awareness Hostile Presence Radius")]
	float searchRadH;
	
	protected void getFriendlyEvaluation()
	{
		ref array<BaseTarget> detected = new array<BaseTarget>;
		m_Perceptions.GetTargetsList(detected, ETargetCategory.FRIENDLY);
		foreach (BaseTarget target : detected)
		{
			IEntity ent = target.GetTargetEntity();
			if(!ent) 
				return;
			
			SCR_AITargetInfo targetInfo = new SCR_AITargetInfo();
			targetInfo.InitFromBaseTarget(target);
			
			if(!targetInfo)
				return;
			
			if (target.GetDistance() < searchRad)
			{
				if (!friendlyBTarget.Contains(ent))
				{
					friendlyBTarget.Insert(ent);
					friendly.Insert(targetInfo);
				}
								
			} else if (target.GetDistance() > searchRad + 50)
			{
				if (friendlyBTarget.Contains(ent))
				{
					friendlyBTarget.RemoveItem(ent);
					friendly.RemoveItem(targetInfo);
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
				friendlyBTarget.Remove(i);
				return;
			}		
			
			bool destroyed = tarinfo.m_DamageManager.IsDestroyed();
			if (destroyed)
			{
				friendly.Remove(i);
				friendlyBTarget.Remove(i);
				return;
			}
			
			if (vector.Distance(tarinfo.m_Entity.GetOrigin(), m_UtilityComp.m_OwnerEntity.GetOrigin()) > searchRad + 50)
			{
				friendlyBTarget.Remove(i);
				friendly.Remove(i);
				return;
			}
		}
	}		
	
	protected void enemyInfos()
	{
		ref array<BaseTarget> hostiles = new array<BaseTarget>;
		m_Perceptions.GetTargetsList(hostiles, ETargetCategory.ENEMY);
		foreach (BaseTarget target : hostiles)
		{
			IEntity ent = target.GetTargetEntity();
			if(!ent) 
				return;
			
			SCR_AITargetInfo targetInfo = new SCR_AITargetInfo();
			targetInfo.InitFromBaseTarget(target);
			
			if(!targetInfo)
				return;
			
			if (target.GetDistance() < searchRadH)
			{
				if (!hostileEnt.Contains(ent))
				{
					hostileEnt.Insert(ent);
					hostile.Insert(targetInfo);
				}
								
			} else if (target.GetDistance() > searchRadH + 50)
			{
				if (hostileEnt.Contains(ent))
				{
					hostileEnt.RemoveItem(ent);
					hostile.RemoveItem(targetInfo);
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
				hostile.Remove(i);
				hostileEnt.Remove(i);
				return;
			}		
			
			bool destroyed = tarinfo.m_DamageManager.IsDestroyed();
			if (destroyed)
			{
				hostile.Remove(i);
				hostileEnt.Remove(i);
				return;
			}
			
			if (vector.Distance(tarinfo.m_Entity.GetOrigin(), m_UtilityComp.m_OwnerEntity.GetOrigin()) > searchRad + 50)
			{
				hostileEnt.Remove(i);
				hostile.Remove(i);
				return;
			}
		}
	}
	
	void manageDetected()
	{
		
	}

	void initialize(SCR_AIUtilityComponent util)
	{
		m_UtilityComp = util;
		m_CombatComp = m_UtilityComp.m_CombatComponent;
		m_InfoComp = m_UtilityComp.m_AIInfo;
		m_Perceptions = m_UtilityComp.m_PerceptionComponent;
		//m_charVic = CharacterVicinityComponent.Cast(m_UtilityComp.GetOwner().GetControlledEntity().FindComponent(CharacterVicinityComponent));
		inventorys = SCR_InventoryStorageManagerComponent.Cast(m_UtilityComp.GetOwner().GetControlledEntity().FindComponent(SCR_InventoryStorageManagerComponent));
	}
	
	void Update()
	{
		getFriendlyEvaluation();
		MaintainFriendly();
		MaintainHostile();
		manageDetected();
	}
	
	float getNumberFriendlyRecognized()
	{
		return friendlyBTarget.Count();
	}
}