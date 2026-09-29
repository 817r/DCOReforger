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
		return ENodeResult.SUCCESS;
	}

	protected static ref TStringArray s_aVarsIn = { "PositionIn" };
	override TStringArray GetVariablesIn() { return s_aVarsIn; }

	protected static ref TStringArray s_aVarsOut = { "PositionOut" };
	override TStringArray GetVariablesOut() { return s_aVarsOut; }

	static override bool VisibleInPalette() { return true; }

	static override string GetOnHoverDescription() { return "DCO: acak titik bidik GL/granat sesuai skill & akurasi. Pasang sekali per tembakan, di luar loop aim."; }
}
