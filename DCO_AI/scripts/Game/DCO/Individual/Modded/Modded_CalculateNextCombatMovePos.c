modded class SCR_AICalculateNextCombatMovePos
{

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
		if (bestGap >= 5.0)
			return best;

		float spreadRadius = Math.Max(radius, 5.0);
		for (int i = 0; i < 4; i++)
		{
			vector candidate = s_AIRandomGenerator.GenerateRandomPointInRadius(0, spreadRadius, centerPos, true);
			candidate[1] = centerPos[1];
			float gap = DCO_SquadSpacing.NearestDistance(candidate, m_aDCOMatePos);
			if (gap > bestGap)
			{
				best = candidate;
				bestGap = gap;
				if (gap >= 5.0)
					break;
			}
		}
		return best;
	}
}
