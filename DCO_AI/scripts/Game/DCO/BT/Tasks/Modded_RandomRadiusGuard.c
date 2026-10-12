modded class SCR_AIGetRandomLookPosition : AITaskScripted
{
	protected bool m_bDCORngLogged;

	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		IEntity controlledEntity = owner.GetControlledEntity();
		if (!controlledEntity)
			return ENodeResult.FAIL;

		vector inPos;
		if (!GetVariableIn(PORT_IN_POS, inPos))
			return ENodeResult.FAIL;

		float radius = vector.Distance(controlledEntity.GetOrigin(), inPos) * m_fTangent;
		if (radius < 0.5)
		{
			DCO_Perf.Count("rng_zero_radius_guard:look_position");
			if (!m_bDCORngLogged)
			{
				m_bDCORngLogged = true;
				DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.WARN, DCO_Log.IdUnit(controlledEntity), string.Format("rng_zero_radius_guard caller=SCR_AIGetRandomLookPosition unit=%1 radius=%2 pos=%3", DCO_Log.IdUnit(controlledEntity), radius.ToString(-1, 2), DCO_Log.Pos(inPos)));
			}
			SetVariableOut(PORT_OUT_POS, inPos);
			return ENodeResult.SUCCESS;
		}

		vector outPos = s_AIRandomGenerator.GenerateRandomPointInRadius(0, radius, inPos, true);
		outPos[1] = inPos[1];
		SetVariableOut(PORT_OUT_POS, outPos);
		return ENodeResult.SUCCESS;
	}
}

modded class SCR_AIGetRandomPointWithExclude : AITaskScripted
{
	protected bool m_bDCORngLogged;

	override protected bool FindPosition2D(out vector randomPos, vector randomSphereOrigin, float randomSphereRadius, vector excludeSphereOrigin = vector.Zero, float excludeRadius = 0, int iterationCount = 50)
	{
		float minR = 0.5;
		if (randomSphereOrigin == excludeSphereOrigin || excludeRadius < 1.0e-8)
			minR = excludeRadius + 0.5;
		if (randomSphereRadius < minR)
		{
			DCO_Perf.Count("rng_zero_radius_guard:point_with_exclude");
			if (!m_bDCORngLogged)
			{
				m_bDCORngLogged = true;
				DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.WARN, "-", string.Format("rng_zero_radius_guard caller=SCR_AIGetRandomPointWithExclude radius=%1 exclude=%2 clamp_to=%3 pos=%4", randomSphereRadius.ToString(-1, 2), excludeRadius.ToString(-1, 2), minR.ToString(-1, 2), DCO_Log.Pos(randomSphereOrigin)));
			}
			randomSphereRadius = minR;
		}
		return super.FindPosition2D(randomPos, randomSphereOrigin, randomSphereRadius, excludeSphereOrigin, excludeRadius, iterationCount);
	}
}
