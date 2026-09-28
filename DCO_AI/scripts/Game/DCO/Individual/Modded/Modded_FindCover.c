modded class SCR_AIFindCover
{
	static const int DCO_SPACING_RETRIES = 3;

	protected ref array<vector> m_aDCOMatePos = {};
	protected ref array<int> m_aDCOBlocked = {};

	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		IEntity ownerEntity = owner.GetControlledEntity();
		if (!ownerEntity || !m_State || !m_CoverMgr || !m_PathfindingComp)
			return ENodeResult.FAIL;

		CoverQueryProperties queryProps;
		GetVariableIn(PORT_COVER_QUERY_PROPERTIES, queryProps);
		if (!queryProps)
			return ENodeResult.FAIL;

		vector coverPos, coverTallestPos;
		int tilex, tiley, coverId;
		if (!DCO_GetSpacedCover(owner, queryProps, coverPos, coverTallestPos, tilex, tiley, coverId))
		{
			ClearVariable(PORT_COVER_LOCK);
			return ENodeResult.FAIL;
		}

		m_State.AssignCover(new SCR_AICoverLock(tilex, tiley, coverId, coverPos, coverTallestPos));
		SetVariableOut(PORT_COVER_LOCK, m_State.GetAssignedCover());
		return ENodeResult.SUCCESS;
	}

	protected bool DCO_GetSpacedCover(AIAgent owner, CoverQueryProperties queryProps, out vector coverPos, out vector coverTallestPos, out int tilex, out int tiley, out int coverId)
	{
		if (!m_CoverMgr.GetBestCover("Soldiers", m_PathfindingComp, queryProps, coverPos, coverTallestPos, tilex, tiley, coverId))
			return false;

		DCO_SquadSpacing.CollectMatePositions(owner, m_aDCOMatePos);
		if (DCO_SquadSpacing.NearestDistance(coverPos, m_aDCOMatePos) >= DCO_SquadSpacing.MIN_SPACING)
			return true;

		vector firstPos = coverPos;
		vector firstTallest = coverTallestPos;
		int firstX = tilex;
		int firstY = tiley;
		int firstId = coverId;

		m_aDCOBlocked.Clear();
		bool spaced = false;
		for (int i = 0; i < DCO_SPACING_RETRIES; i++)
		{
			m_CoverMgr.SetOccupiedCover(tilex, tiley, coverId, true);
			m_aDCOBlocked.Insert(tilex);
			m_aDCOBlocked.Insert(tiley);
			m_aDCOBlocked.Insert(coverId);

			if (!m_CoverMgr.GetBestCover("Soldiers", m_PathfindingComp, queryProps, coverPos, coverTallestPos, tilex, tiley, coverId))
				break;

			if (DCO_SquadSpacing.NearestDistance(coverPos, m_aDCOMatePos) >= DCO_SquadSpacing.MIN_SPACING)
			{
				spaced = true;
				break;
			}
		}

		for (int b = 0; b < m_aDCOBlocked.Count(); b += 3)
			m_CoverMgr.SetOccupiedCover(m_aDCOBlocked[b], m_aDCOBlocked[b + 1], m_aDCOBlocked[b + 2], false);

		if (!spaced)
		{
			coverPos = firstPos;
			coverTallestPos = firstTallest;
			tilex = firstX;
			tiley = firstY;
			coverId = firstId;
		}
		return true;
	}
}
