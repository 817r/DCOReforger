class DCO_AIGetSuppressionVolumeRandomPosition : AITaskScripted
{
	protected static const string CENTER_POS_PORT = "RandomPos";
	protected static const string DISTANCE_PORT = "Distance_m";
		
	protected static const string SUPPRESSION_VOLUME = "SuppressionVolume";
	
	protected ref TStringArray s_aVarsOut = {CENTER_POS_PORT,DISTANCE_PORT};
	protected ref TStringArray s_aVarsIn = {SUPPRESSION_VOLUME};
	override TStringArray GetVariablesIn() { return s_aVarsIn; }
	override TStringArray GetVariablesOut() { return s_aVarsOut; }
			
	static override bool VisibleInPalette() { return true; }
	
	static override string GetOnHoverDescription() { return "Returns center position of given suppression volume"; };
	
	//---------------------------------------------------------------------------------------
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		SCR_AISuppressionVolumeBase volume;
		GetVariableIn(SUPPRESSION_VOLUME, volume);
		
		if (!volume)
			return NodeError(this, owner, "No suppression volume provided!");
			
		vector centerPos = volume.GetCenterPosition();
		SetVariableOut(CENTER_POS_PORT, centerPos);
		
		float distance;
		IEntity m_CharacterEntity = owner.GetControlledEntity();
		if (m_CharacterEntity)
			distance = vector.Distance(m_CharacterEntity.GetOrigin(), centerPos);
		
		SetVariableOut(DISTANCE_PORT, distance);
		
		return ENodeResult.SUCCESS;
	};
}