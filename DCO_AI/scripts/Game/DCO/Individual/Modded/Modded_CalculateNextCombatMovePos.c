modded class SCR_AICalculateNextCombatMovePos
{
	static const int DCO_SPACING_SAMPLES = 4;

	protected AIAgent m_DCOOwner;
	protected ref array<vector> m_aDCOMatePos = {};

	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		m_DCOOwner = owner;
		return super.EOnTaskSimulate(owner, dt);
	}

	override protected vector RandomizeDestinationPos(float distance, vector centerPos)
	{
		float radius = 0.1 * distance;
		vector best = centerPos;
		if (radius > 0.01)
			best = super.RandomizeDestinationPos(distance, centerPos);

		if (!m_DCOOwner)
			return best;

		DCO_SquadSpacing.CollectMatePositions(m_DCOOwner, m_aDCOMatePos);
		float bestGap = DCO_SquadSpacing.NearestDistance(best, m_aDCOMatePos);
		if (bestGap >= DCO_SquadSpacing.MIN_SPACING)
			return best;

		float spreadRadius = Math.Max(radius, DCO_SquadSpacing.MIN_SPACING);
		for (int i = 0; i < DCO_SPACING_SAMPLES; i++)
		{
			vector candidate = s_AIRandomGenerator.GenerateRandomPointInRadius(0, spreadRadius, centerPos, true);
			candidate[1] = centerPos[1];
			float gap = DCO_SquadSpacing.NearestDistance(candidate, m_aDCOMatePos);
			if (gap > bestGap)
			{
				best = candidate;
				bestGap = gap;
				if (gap >= DCO_SquadSpacing.MIN_SPACING)
					break;
			}
		}
		return best;
	}
}
