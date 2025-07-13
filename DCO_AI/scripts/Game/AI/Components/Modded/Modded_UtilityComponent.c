modded class SCR_AIUtilityComponent : SCR_AIBaseUtilityComponent
{
	SCR_DCO_AIConfigComponent DCO_ConfComponent;
	DCO_AIMoraleSystemComponent DCO_MoraleSystem;
	DCO_AIDetectionSystemComponent DCO_AIDetection;
	
	bool takeWeaponAlready = false;
		
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
	protected bool MagazineHandling()
	{
		BaseWeaponComponent weap = m_CombatComponent.GetCurrentWeaponManager().GetCurrentWeapon();
		if (!weap) return false;
		bool isReplenishable = false;
		//SCR_AIDebugVisualization.VisualizeMessage(m_OwnerEntity, weap.GetUIInfo().GetName(), EAIDebugCategory.INFO, 1.4, Color.White);		
		if (weap.GetWeaponType() == EWeaponType.WT_RIFLE || weap.GetWeaponType() == EWeaponType.WT_MACHINEGUN || weap.GetWeaponType() == EWeaponType.WT_HANDGUN) isReplenishable = true;
		
		int magCount = m_CombatComponent.getInventoryStorageMan().GetMagazineCountByWeapon(weap);
		int lowMagThreshold = m_CombatComponent.GetWeaponLowMagThreshold(weap);
		
		if (magCount < lowMagThreshold && isReplenishable)	
		{
			BaseMagazineWell m_sMagazineWellType = weap.GetCurrentMagazine().GetMagazineWell();
			m_CombatComponent.getInventoryStorageMan().ResupplyMagazines(lowMagThreshold + 1);
			m_CombatComponent.getCharacterController().ReloadWeapon();		
			return true;
		}		
		
		return false;
	}
	
	protected bool Emergency()
	{
		if (takeWeaponAlready) return false;
		array<BaseWeaponComponent> weap = {};
		array<BaseMagazineComponent> mag = {};
		m_CombatComponent.GetCurrentWeaponManager().GetWeapons(weap);
		
		if (!weap) return false;
		int isLowAmmo = 0;
		
		//SCR_AIDebugVisualization.VisualizeMessage(m_OwnerEntity, weap.GetUIInfo().GetName(), EAIDebugCategory.INFO, 1.4, Color.White);
		
		foreach (BaseWeaponComponent e : weap)
		{
			bool replens = false;
			if (e.GetWeaponType() == EWeaponType.WT_RIFLE || e.GetWeaponType() == EWeaponType.WT_MACHINEGUN || e.GetWeaponType() == EWeaponType.WT_HANDGUN)
			{
				replens = true;
			} 
			
			int magCount = m_CombatComponent.getInventoryStorageMan().GetMagazineCountByWeapon(e);
			int lowMagThreshold = m_CombatComponent.GetWeaponLowMagThreshold(e);

			if (magCount < lowMagThreshold && replens && e.GetWeaponType() != EWeaponType.WT_FRAGGRENADE)	
			{
				isLowAmmo++;
			}
		}
		
		if (isLowAmmo > 2)	
		{
			return true;
		}		
		
		return false;
	}
	
	bool isScavangeWeapon()
	{
		return (Emergency()) || (MagazineHandling() && m_ThreatSystem.GetSuppressionMeasure() < 0.7 && DCO_MoraleSystem.GetMoraleValue() > 40);
	}
	
	bool isResupply()
	{
		return (MagazineHandling() && m_ThreatSystem.GetState() < EAIThreatState.THREATENED);
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
		m_AIInfo.RegisterUtility(this);
	}
	
	bool IsAimImproved()
	{
		return DCO_ConfComponent.EnableAimImprovement();
	}
}