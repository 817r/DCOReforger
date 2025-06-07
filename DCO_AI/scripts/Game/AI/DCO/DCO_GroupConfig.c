class SCR_DCO_AIGroupConfigComponentClass : ScriptComponentClass
{
}

enum DCO_GroupTactics
{
	AGGRESSIVE,
	DEFENSIVE,
	EVASIVE,
	BALANCED
}

class SCR_DCO_AIGroupConfigComponent : ScriptComponent
{
	[Attribute("0", UIWidgets.ComboBox, "AI Tactics in combat", "", ParamEnumArray.FromEnum(DCO_GroupTactics), category: "AI Tactics to use", )]
	DCO_GroupTactics m_Tactics;
	
	static SCR_DCO_AIGroupConfigComponent GetDCOAIConfigComponent(IEntity unit)
	{		
		AIControlComponent ctrl = AIControlComponent.Cast(unit.FindComponent(AIControlComponent));
		if (ctrl)
		{
			SCR_ChimeraAIAgent agent = SCR_ChimeraAIAgent.Cast(ctrl.GetAIAgent());
			if (agent)
			{
				AIGroup grp = agent.GetParentGroup();
				if (grp)
					return SCR_DCO_AIGroupConfigComponent.Cast(grp.FindComponent(SCR_DCO_AIGroupConfigComponent));
			}
		}
		return null;
	}

	override void OnPostInit(IEntity owner)
    {		
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);
    }
}