class DCO_AICreateSuppressionVolumeAt : AITaskScripted
{
	protected static const string SUPPRESSION_VOLUME = "SuppressionVolume";
	
	protected static const string LAST_SEEN_POSITION_PORT = "LastSeenPosition";
	
	[Attribute("3", UIWidgets.Slider, "Radius Of Suppression Volume", "0 100 1" )]
	protected float m_fRadiusSuppression;
	
	ref SCR_AITargetInfo m_TargetInfo;
	ref SCR_AISuppressionVolumeBaseTargetBox m_SuppressionVolume;
	vector v_TargetPos;
	
	protected ref TStringArray s_aVarsIn = {
		LAST_SEEN_POSITION_PORT,
	};
	protected ref TStringArray s_aVarsOut = {SUPPRESSION_VOLUME};
	override TStringArray GetVariablesIn() { return s_aVarsIn; }
	
	override TStringArray GetVariablesOut() { return s_aVarsOut; }	
	
	static override bool VisibleInPalette() { return true; }
	
	static override string GetOnHoverDescription() { return "Create SCR_TargetInfo object from its member variables"; };
	
	//----------------------------------------------------------------------------------------------------------------
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		GetVariableIn("LAST_SEEN_POSITION_PORT", v_TargetPos);
		
		m_TargetInfo = new SCR_AITargetInfo();
		float shooterDistance = vector.Distance(m_TargetInfo.m_Entity.GetOrigin(), owner.GetControlledEntity().GetOrigin());
		float radius = Math.Map(shooterDistance, 50, SCR_AICombatComponent.LONG_RANGE_COMBAT_DISTANCE, 2, 5);
		vector bbMax, bbMin;
		m_TargetInfo.m_Entity.GetBounds(bbMin, bbMax);
		m_SuppressionVolume = new SCR_AISuppressionVolumeBaseTargetBox(bbMin * radius, bbMax * radius * 1.5);
		
		return ENodeResult.SUCCESS;
	}
}