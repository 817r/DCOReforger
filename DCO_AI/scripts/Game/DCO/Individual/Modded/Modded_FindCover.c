modded class SCR_AIFindCover
{

	protected ref array<vector> m_aDCOMatePos = {};
	protected ref array<int> m_aDCOBlocked = {};
	protected SCR_AIUtilityComponent m_DCOUtil;

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
		bool found;
		int dcoFindT = DCO_Perf.Begin();
		if (DCO_CoverEval.On())
		{
			if (!m_DCOUtil)
				m_DCOUtil = SCR_AIUtilityComponent.Cast(owner.FindComponent(SCR_AIUtilityComponent));
			found = DCO_CoverEval.Pick(owner, m_DCOUtil, m_CoverMgr, m_PathfindingComp, queryProps, coverPos, coverTallestPos, tilex, tiley, coverId);
		}
		else
			found = DCO_GetSpacedCover(owner, queryProps, coverPos, coverTallestPos, tilex, tiley, coverId);
		DCO_Perf.End("find_cover", dcoFindT);
		if (!found)
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

		if (!m_DCOUtil)
			m_DCOUtil = SCR_AIUtilityComponent.Cast(owner.FindComponent(SCR_AIUtilityComponent));
		DCO_SquadSpacing.CollectMatePositions(owner, m_aDCOMatePos);
		if (DCO_IndLearn.PickOn() && !DCO_IndLearn.Explore())
			return DCO_PickLearnedCover(queryProps, coverPos, coverTallestPos, tilex, tiley, coverId);
		if (DCO_SquadSpacing.NearestDistance(coverPos, m_aDCOMatePos) >= 5.0 && DCO_IndLearn.CoverOk(m_DCOUtil, coverPos))
			return true;

		vector firstPos = coverPos;
		vector firstTallest = coverTallestPos;
		int firstX = tilex;
		int firstY = tiley;
		int firstId = coverId;

		m_aDCOBlocked.Clear();
		bool spaced = false;
		for (int i = 0; i < 3; i++)
		{
			m_CoverMgr.SetOccupiedCover(tilex, tiley, coverId, true);
			m_aDCOBlocked.Insert(tilex);
			m_aDCOBlocked.Insert(tiley);
			m_aDCOBlocked.Insert(coverId);

			if (!m_CoverMgr.GetBestCover("Soldiers", m_PathfindingComp, queryProps, coverPos, coverTallestPos, tilex, tiley, coverId))
				break;

			if (DCO_SquadSpacing.NearestDistance(coverPos, m_aDCOMatePos) >= 5.0 && DCO_IndLearn.CoverOk(m_DCOUtil, coverPos))
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

	protected bool DCO_PickLearnedCover(CoverQueryProperties queryProps, inout vector coverPos, inout vector coverTallestPos, inout int tilex, inout int tiley, inout int coverId)
	{
		vector firstPos = coverPos;
		vector firstTallest = coverTallestPos;
		int firstX = tilex;
		int firstY = tiley;
		int firstId = coverId;
		vector bestPos = coverPos;
		vector bestTallest = coverTallestPos;
		int bestX = tilex;
		int bestY = tiley;
		int bestId = coverId;
		string info;
		bool have = DCO_SquadSpacing.NearestDistance(coverPos, m_aDCOMatePos) >= 5.0;
		float bestM = -1;
		float firstM = 1.0;
		if (have)
		{
			firstM = DCO_IndLearn.Mult(m_DCOUtil, coverPos, info);
			bestM = firstM;
		}
		int chosen = 0;
		int n = 1;
		m_aDCOBlocked.Clear();
		for (int i = 0; i < 2; i++)
		{
			m_CoverMgr.SetOccupiedCover(tilex, tiley, coverId, true);
			m_aDCOBlocked.Insert(tilex);
			m_aDCOBlocked.Insert(tiley);
			m_aDCOBlocked.Insert(coverId);
			if (!m_CoverMgr.GetBestCover("Soldiers", m_PathfindingComp, queryProps, coverPos, coverTallestPos, tilex, tiley, coverId))
				break;
			n++;
			if (DCO_SquadSpacing.NearestDistance(coverPos, m_aDCOMatePos) < 5.0)
				continue;
			string ci;
			float m = DCO_IndLearn.Mult(m_DCOUtil, coverPos, ci);
			if (!have || m > bestM + 0.03)
			{
				have = true;
				bestM = m;
				info = ci;
				chosen = i + 1;
				bestPos = coverPos;
				bestTallest = coverTallestPos;
				bestX = tilex;
				bestY = tiley;
				bestId = coverId;
			}
		}
		for (int b = 0; b < m_aDCOBlocked.Count(); b += 3)
			m_CoverMgr.SetOccupiedCover(m_aDCOBlocked[b], m_aDCOBlocked[b + 1], m_aDCOBlocked[b + 2], false);
		if (!have)
		{
			coverPos = firstPos;
			coverTallestPos = firstTallest;
			tilex = firstX;
			tiley = firstY;
			coverId = firstId;
			return true;
		}
		coverPos = bestPos;
		coverTallestPos = bestTallest;
		tilex = bestX;
		tiley = bestY;
		coverId = bestId;
		DCO_IndLearn.NotePick(m_DCOUtil, n, chosen, firstM, bestM, info);
		return true;
	}
}
