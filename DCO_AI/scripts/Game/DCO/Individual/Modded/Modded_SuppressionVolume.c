modded class SCR_AISuppressionVolumeBase
{
	protected static const float MAX_X_ANGLE_DEG = 14;
	protected static const float MIN_X_ANGLE_DEG = 2.2;
	protected static const float MAX_Y_ANGLE_DEG = 3;
	protected static const float MIN_Y_ANGLE_DEG = 0.5;
	protected static const float MIN_SURFACE_Y = 0.2;
	protected static const float CHANCE_FOR_OPPOSITE_DIR = 0.18;






	static bool CreateSuppressionBox(vector center, float sizeXZ, float height, out vector bbMin, out vector bbMax)
	{
	    float half = sizeXZ * 0.5;

	    bbMin = center - Vector(half, 0, half);
	    bbMax = center + Vector(half, height, half);
	    return true;
	}
}
modded class SCR_AIGetSuppressionVolumeCenterPosition
{
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		SCR_AISuppressionVolumeBase volume;
		GetVariableIn("SuppressionVolume", volume);
		if (!volume)
			return ENodeResult.FAIL;
		return super.EOnTaskSimulate(owner, dt);
	}
}

modded class SCR_AIGetSuppressionVolumeLine
{
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		SCR_AISuppressionVolumeBase volume;
		GetVariableIn("SuppressionVolume", volume);
		if (!volume)
			return ENodeResult.FAIL;
		return super.EOnTaskSimulate(owner, dt);
	}
}

modded class SCR_AISuppressionVolumeClusterBox
{
	override void SetClusterState(SCR_AITargetClusterState clusterState, bool scaleTarget = true)
	{
		super.SetClusterState(clusterState, false);
		if (!m_ClusterState || !scaleTarget)
			return;

		float dist = m_ClusterState.m_fDistMin;
		float extra = 3.0 + dist * 0.04;
		if (m_ClusterState.m_iCountIdentified <= 0)
			extra *= 1.5;
		float cap = Math.Clamp(dist * 0.2126, 15.0, 60.0);

		for (int i = 0; i < 3; i += 2)
		{
			float c = (m_vBBMin[i] + m_vBBMax[i]) * 0.5;
			float raw = m_vBBMax[i] - m_vBBMin[i];
			float w = Math.Max(Math.Min(raw + extra * 2.0, cap), raw);
			m_vBBMin[i] = c - w * 0.5;
			m_vBBMax[i] = c + w * 0.5;
		}

		if (m_vBBMax[1] - m_vBBMin[1] < 4.0)
		{
			m_vBBMin[1] = m_vBBMin[1] - 2.0;
			m_vBBMax[1] = m_vBBMax[1] + 2.0;
		}

#ifdef WORKBENCH
		UpdateDebug();
#endif
	}
}
