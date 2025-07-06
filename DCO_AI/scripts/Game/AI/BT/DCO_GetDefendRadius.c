//! Returns a position of entity with local space offset
class DCO_GetDefendRadius : AITaskScripted
{	
	protected static const string DEFEND_RADIUS = "Radius";
		SCR_DCO_AIGroupConfigComponent groupConfig;
	
	static override bool VisibleInPalette() { return true; }
	
	protected override void OnInit(AIAgent owner)
	{
		AIGroup grp = owner.GetParentGroup();
		groupConfig = SCR_DCO_AIGroupConfigComponent.Cast(grp.FindComponent(SCR_DCO_AIGroupConfigComponent));
	}
	
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		if (!groupConfig)
		{
			return false;
		};
		float rad = groupConfig.GetDefendRadius();
		SetVariableOut(DEFEND_RADIUS, rad);
		
		return ENodeResult.SUCCESS;
	}
	
	protected static ref TStringArray s_aVarsOut = { DEFEND_RADIUS };
	override TStringArray GetVariablesOut() { return s_aVarsOut; }
}