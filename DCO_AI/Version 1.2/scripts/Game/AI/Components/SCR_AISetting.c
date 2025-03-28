[ComponentEditorProps(category: "GameScripted/AI", description: "Utility for DCO AI")]
class SCRDCO_AIConfigComponentClass : ScriptComponentClass
{
}

enum DCO_ForceStance{
	STAND,
	CROUCH,
	PRONE
}

class SCRDCO_AIConfigComponent : ScriptComponent
{
	[Attribute( defvalue: "1", uiwidget: UIWidgets.CheckBox, desc: "Alow movement" )]
	bool m_EnableMovement;
	
	[Attribute("0", UIWidgets.ComboBox, "Force Stances In AI", "", ParamEnumArray.FromEnum(DCO_ForceStance) )]
	DCO_ForceStance m_Estance;	
	
	[Attribute("800", UIWidgets.Auto, "Investigation Move")]
	float InvestigationDistance;	
	
	[Attribute("30", UIWidgets.Auto, "Info Share Delay in Second")]
	float InfoShareTime;
	
	[Attribute("3", UIWidgets.Auto, "Investigation Delayed Before They Move")]
	float InvestigateDelay;
	
	protected SCR_AIUtilityComponent m_Utility;
	protected SCR_AIInfoComponent m_InfoComp;
	
	static bool SetEnableMovement(IEntity unit, bool rank)
	{
		if (!unit)
			return true;
		
		SCRDCO_AIConfigComponent comp = GetDCOAIConfig(unit);
		
		if (!comp)
			return false;
		
		return comp.SetMovement(rank);
	}

	static bool GetEnableMovement(IEntity unit)
	{
		if (!unit)
			return true;
		
		SCRDCO_AIConfigComponent comp = GetDCOAIConfig(unit);
		
		if (!comp)
			return false;
		
		return comp.GetEnableMovement();
	}
	
	static bool SetInfoshareDelay(IEntity unit, float rank)
	{
		if (!unit)
			return true;
		
		SCRDCO_AIConfigComponent comp = GetDCOAIConfig(unit);
		
		if (!comp)
			return false;
		
		return comp.SetInfoTime(rank);
	}

	static float GetInfoshareDelay(IEntity unit)
	{
		if (!unit)
			return 1;
		
		SCRDCO_AIConfigComponent comp = GetDCOAIConfig(unit);
		
		if (!comp)
			return 1;
		
		return comp.GetInfoShare();
	}
	
	static float SetInvestigationDist(IEntity unit, float rank)
	{
		if (!unit)
			return 1;
		
		SCRDCO_AIConfigComponent comp = GetDCOAIConfig(unit);
		
		if (!comp)
			return 1;
		
		return comp.SetInvestigationDist(rank);
	}
	
	static float GetInvestigateDelay(IEntity unit)
	{
		if (!unit)
			return 1;
		
		SCRDCO_AIConfigComponent comp = GetDCOAIConfig(unit);
		
		if (!comp)
			return 1;
		
		return comp.GetInvestigationDelay();
	}
	
	static float SetInvestigationDelay(IEntity unit, float rank)
	{
		if (!unit)
			return 1;
		
		SCRDCO_AIConfigComponent comp = GetDCOAIConfig(unit);
		
		if (!comp)
			return 1;
		
		return comp.SetInvestigationDelay(rank);
	}

	static float GetInvestigationDist(IEntity unit)
	{
		if (!unit)
			return true;
		
		SCRDCO_AIConfigComponent comp = GetDCOAIConfig(unit);
		
		if (!comp)
			return false;
		
		return comp.GetInvestigationDist();
	}
	
	static DCO_ForceStance SetStances(IEntity unit, DCO_ForceStance rank)
	{
		if (!unit)
			return true;
		
		SCRDCO_AIConfigComponent comp = GetDCOAIConfig(unit);
		
		if (!comp)
			return false;
		
		return comp.SetStances(rank);
	}

	static DCO_ForceStance GetStances(IEntity unit)
	{
		if (!unit)
			return true;
		
		SCRDCO_AIConfigComponent comp = GetDCOAIConfig(unit);
		
		if (!comp)
			return false;
		
		return comp.GetStances();
	}
	
	static SCRDCO_AIConfigComponent GetDCOAIConfig(IEntity unit)
	{
		return SCRDCO_AIConfigComponent.Cast(unit.FindComponent(SCRDCO_AIConfigComponent));
	}

	protected bool SetMovement(bool rank)
	{
		m_EnableMovement = rank;
		m_Utility.restartBehaviour();
		return rank;
	}
	
	protected bool GetEnableMovement()
	{
		return m_EnableMovement;
	}
	
	protected float SetInvestigationDist(float rank)
	{
		InvestigationDistance = rank;
		return rank;
	}
	
	protected float GetInvestigationDist()
	{
		return InvestigationDistance;
	}
	
	protected float SetInvestigationDelay(float rank)
	{
		InvestigateDelay = rank;
		return rank;
	}
	
	protected float GetInvestigationDelay()
	{
		return InvestigateDelay;
	}
	
	protected float SetInfoTime(float rank)
	{
		InfoShareTime = rank;
		m_Utility.SetInfoshareTimer(rank);
		return rank;
	}
	
	protected float GetInfoShare()
	{
		return InfoShareTime;
	}
	
	protected DCO_ForceStance SetStances(DCO_ForceStance rank)
	{
		m_Estance = rank;
		m_InfoComp.SetForceStance(ConvertDCOToStanceChange(m_Estance));
		return rank;
	}

	ECharacterStance ConvertDCOToStanceChange(DCO_ForceStance stance)
	{
		switch (stance)
		{
			case DCO_ForceStance.STAND: return ECharacterStance.STAND;
			case DCO_ForceStance.CROUCH: return ECharacterStance.CROUCH;
			case DCO_ForceStance.PRONE: return ECharacterStance.PRONE;
		}
		return 0;
	}
	
	DCO_ForceStance ConvertVanToDCOChange(ECharacterStance stance)
	{
		switch (stance)
		{
			case ECharacterStance.STAND: return DCO_ForceStance.STAND;
			case ECharacterStance.CROUCH: return DCO_ForceStance.CROUCH;
			case ECharacterStance.PRONE: return DCO_ForceStance.PRONE;
		}
		return 0;
	}
	
	protected DCO_ForceStance GetStances()
	{
		m_Estance = ConvertVanToDCOChange(m_InfoComp.GetStance());
		return m_Estance;
	}
	
	override void EOnInit(IEntity owner)
	{
		AIControlComponent ctrl = AIControlComponent.Cast(owner.FindComponent(AIControlComponent));
		if (ctrl)
		{
			SCR_ChimeraAIAgent agent = SCR_ChimeraAIAgent.Cast(ctrl.GetAIAgent());
			if (agent)
			{
				m_Utility = SCR_AIUtilityComponent.Cast(agent.FindComponent(SCR_AIUtilityComponent));
				if (m_Utility)
				{
					m_InfoComp = m_Utility.m_AIInfo;
					m_Estance = m_Utility.getCharCon().GetStance();
				}
				
			}
		}
	}
	
	override void OnPostInit(IEntity owner)
	{
		SetEventMask(owner, EntityEvent.INIT);
	}
}
