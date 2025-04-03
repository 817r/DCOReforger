modded class SCR_AIUtilityComponent : SCR_AIBaseUtilityComponent
{
	SCR_DCO_AIConfigComponent DCO_ConfComponent;
	DCO_AIMoraleSystemComponent DCO_MoraleSystem;
	DCO_AIDetectionSystemComponent DCO_AIDetection;
		
	ref array<BaseTarget> targets = new array<BaseTarget>;
	ref array<IEntity> dedAlly;
	//------------------------------------------------------------------------------------------------
	//!
	//! \param[in] unknownTarget
	//! \return
	override SCR_AIBehaviorBase EvaluateBehavior(BaseTarget unknownTarget)
	{
		super.EvaluateBehavior(unknownTarget);
		
		MaintainTarget();
		MagazineHandling();
		return m_CurrentBehavior;
	}
	
	protected void MaintainTarget()
	{
		if(targets.Count() < 1) return;
		
		foreach(BaseTarget targ : targets)
		{
			if(!targ) return;
			
			if(targ.IsDisarmed())
			{
				targets.RemoveItem(targ);
			}
			else if	(targ.GetTimeSinceDetected() > 30)
			{
				targets.RemoveItem(targ);
			} else if (targ.GetDamageManagerComponent().IsDestroyed())
			{
				targets.RemoveItem(targ);
			}
		}
	}

	protected void InfoShare()
	{
		SCR_AIGroupUtilityComponent groupUtilityComp = SCR_AIGroupUtilityComponent.Cast(GetAIAgent().GetParentGroup().FindComponent(SCR_AIGroupUtilityComponent));
		if (groupUtilityComp)
		{
			PerceptionManager pm = GetGame().GetPerceptionManager();
			if (pm)
			{					
				for (int i = targets.Count()-1; i >= 0; i--)
				{
					if (!targets[i]) return;
					float timeNow = pm.GetTime();		
					groupUtilityComp.m_Perception.UpdateFromFriendlys(targets[i], m_AIInfo);
				}
			}
		}
	}
	
	// Temporary for Magic Magazine
	protected void MagazineHandling()
	{
		BaseWeaponComponent weap = m_CombatComponent.GetCurrentWeaponManager().GetCurrentWeapon();
		if (!weap) return;
		bool isReplenishable = false;
		//SCR_AIDebugVisualization.VisualizeMessage(m_OwnerEntity, weap.GetUIInfo().GetName(), EAIDebugCategory.INFO, 1.4, Color.White);		
		if (weap.GetWeaponType() == EWeaponType.WT_RIFLE || weap.GetWeaponType() == EWeaponType.WT_MACHINEGUN || weap.GetWeaponType() == EWeaponType.WT_HANDGUN) isReplenishable = true;
		
		int magCount = m_CombatComponent.getInventoryStorageMan().GetMagazineCountByWeapon(weap);
		int lowMagThreshold = m_CombatComponent.GetWeaponLowMagThreshold(weap);
		
		if (magCount < lowMagThreshold && isReplenishable)	
		{
			BaseMagazineWell m_sMagazineWellType = weap.GetCurrentMagazine().GetMagazineWell();
			m_CombatComponent.getInventoryStorageMan().ResupplyMagazines(lowMagThreshold + 1);
			//m_CombatComponent.getCharacterController().ReloadWeapon();		
		}		
	}
	
	protected void TakeGoodWeaponAndMagazine()
	{
		DCO_AIDetection.GetDeadAllies(dedAlly);
		array<IEntity> itemss;
		array<IEntity> Weapon;
		array<IEntity> Magazine;

		foreach (IEntity e : dedAlly)
		{
			array<typename> compToFind;
			compToFind.Insert(BaseWeaponComponent);
			compToFind.Insert(BaseMagazineComponent);
			SCR_InventoryStorageManagerComponent m_Inventory = SCR_InventoryStorageManagerComponent.Cast(e.FindComponent(SCR_InventoryStorageManagerComponent));	
			m_Inventory.FindItemsWithComponents(itemss, compToFind);
			
			foreach(IEntity wp : itemss)
			{
				BaseWeaponComponent wpC = BaseWeaponComponent.Cast(wp.FindComponent(BaseWeaponComponent));
				BaseMagazineComponent mgC = BaseMagazineComponent.Cast(wp.FindComponent(BaseMagazineComponent));
				if (!wpC || !mgC) return;
				
				if (wpC)
				{
					EWeaponType Wtype = wpC.GetWeaponType();
					if (Wtype == EWeaponType.WT_ROCKETLAUNCHER)
					{
						Weapon.Insert(wp);
						break;
					} else if (Wtype == EWeaponType.WT_MACHINEGUN)
					{
						Weapon.Insert(wp);
						break;
					} else if (Wtype == EWeaponType.WT_GRENADELAUNCHER)
					{
						Weapon.Insert(wp);
						break;					
					}
				} else if (mgC)
				{
					Magazine.Insert(wp);
				}
			}
			
			itemss.Clear();
		}
		
		EWeaponType myType = m_CombatComponent.GetCurrentWeaponManager().GetCurrentWeapon().GetWeaponType();
		
		switch(myType)
		{
			case EWeaponType.WT_RIFLE:
			{
				break;
			}
		}
	}
	
	void SearchWeapon(array<IEntity> itemss)
	{
		
	}
	
	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		AIAgent agent = GetOwner();
		if (!agent)
			return;	
		
		m_OwnerEntity = GenericEntity.Cast(agent.GetControlledEntity());
		if (!m_OwnerEntity)
			return;
		
		DCO_ConfComponent = SCR_DCO_AIConfigComponent.Cast(owner.FindComponent(SCR_DCO_AIConfigComponent));
		DCO_MoraleSystem = DCO_AIMoraleSystemComponent.Cast(m_OwnerEntity.FindComponent(DCO_AIMoraleSystemComponent));
		DCO_AIDetection = DCO_AIDetectionSystemComponent.Cast(m_OwnerEntity.FindComponent(DCO_AIDetectionSystemComponent));
	}
	
	bool IsAimImproved()
	{
		return DCO_ConfComponent.EnableAimImprovement();
	}
}