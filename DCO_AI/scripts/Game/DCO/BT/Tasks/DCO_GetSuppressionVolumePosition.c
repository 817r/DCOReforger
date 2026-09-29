class DCO_AIGetSuppressionVolumeRandomPosition : AITaskScripted
{


	protected ref TStringArray s_aVarsOut = {"RandomPos","Distance_m"};
	protected ref TStringArray s_aVarsIn = {"SuppressionVolume"};
	override TStringArray GetVariablesIn() { return s_aVarsIn; }
	override TStringArray GetVariablesOut() { return s_aVarsOut; }

	static override bool VisibleInPalette() { return true; }

	static override string GetOnHoverDescription() { return "Returns center position of given suppression volume"; };

	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		SCR_AISuppressionVolumeBase volume;
		GetVariableIn("SuppressionVolume", volume);

		if (!volume)
			return NodeError(this, owner, "No suppression volume provided!");

		vector centerPos = volume.GetCenterPosition();
		SetVariableOut("RandomPos", centerPos);

		float distance;
		IEntity m_CharacterEntity = owner.GetControlledEntity();
		if (m_CharacterEntity)
			distance = vector.Distance(m_CharacterEntity.GetOrigin(), centerPos);

		SetVariableOut("Distance_m", distance);

		return ENodeResult.SUCCESS;
	};
}