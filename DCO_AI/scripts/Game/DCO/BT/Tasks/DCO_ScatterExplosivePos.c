class DCO_AIScatterExplosivePos : AITaskScripted
{

	[Attribute("1", UIWidgets.CheckBox, "ON = grenade launcher (slider GL Accuracy), OFF = frag (slider Aim Accuracy)")]
	protected bool m_bGrenadeLauncher;

	protected SCR_AIUtilityComponent m_Utility;

	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		vector pos;
		if (!GetVariableIn("PositionIn", pos))
			return ENodeResult.FAIL;

		if (!m_Utility)
			m_Utility = SCR_AIUtilityComponent.Cast(owner.FindComponent(SCR_AIUtilityComponent));

		SetVariableOut("PositionOut", DCO_AimUtility.ScatterExplosive(m_Utility, pos, m_bGrenadeLauncher));
		if (m_bGrenadeLauncher && m_Utility && m_Utility.m_OwnerEntity)
		{
			DCO_Perf.Count("spec:gl_shot");
			DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(m_Utility.m_OwnerEntity), string.Format("spec_gl_aim unit=%1 dist=%2", DCO_Log.IdUnit(m_Utility.m_OwnerEntity), Math.Round(vector.Distance(m_Utility.m_OwnerEntity.GetOrigin(), pos))));
		}
		return ENodeResult.SUCCESS;
	}

	protected static ref TStringArray s_aVarsIn;
	protected static TStringArray GetVarsIn()
	{
		if (!s_aVarsIn)
			s_aVarsIn = { "PositionIn" };
		return s_aVarsIn;
	}
	override TStringArray GetVariablesIn() { return GetVarsIn(); }

	protected static ref TStringArray s_aVarsOut;
	protected static TStringArray GetVarsOut()
	{
		if (!s_aVarsOut)
			s_aVarsOut = { "PositionOut" };
		return s_aVarsOut;
	}
	override TStringArray GetVariablesOut() { return GetVarsOut(); }

	static override bool VisibleInPalette() { return true; }

	static override string GetOnHoverDescription() { return "DCO: randomise the GL or grenade aim point by skill and accuracy. Set once per shot, outside the aim loop."; }
}
