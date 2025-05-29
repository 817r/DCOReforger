modded class SCR_AICalculateCoverQueryProps_CombatMove : AITaskScripted
{
	//---------------------------------------------------------------
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		IEntity ownerEntity = owner.GetControlledEntity();
		if (!ownerEntity)
			return ENodeResult.FAIL;
		
		SCR_AICombatMoveRequestBase rqBase;
		GetVariableIn(PORT_COMBAT_MOVE_REQUEST, rqBase);
		SCR_AICombatMoveRequest_Move rq = SCR_AICombatMoveRequest_Move.Cast(rqBase);
		
		if (!rq)
			return SCR_AIErrorMessages.NodeErrorCombatMoveRequest(this, owner, rqBase);
		
		vector myPos = ownerEntity.GetOrigin();
		
		CoverQueryProperties query = m_CoverQueryProps;
		
		// Resolve direction of query
		if (rq.m_eDirection == SCR_EAICombatMoveDirection.ANYWHERE)
		{
			// Query in circle, don't use direction
			query.m_vSectorDir = vector.Zero;
			query.m_fScoreWeightDirection = 0;
			query.m_fQuerySectorAngleCosMin = -1.0;
		}
		else
		{
			// Directional query is used
			// Query in sector
			vector queryDirection = SCR_AICombatMoveUtils.CalculateMoveDirection(rq.m_eDirection, myPos, rq.m_vMovePos);
			query.m_vSectorDir = queryDirection;
			query.m_fQuerySectorAngleCosMin = Math.Cos(rq.m_fCoverSearchSectorHalfAngleRad);
			
			if (rq.m_bUseCoverSearchDirectivity)
				query.m_fScoreWeightDirection = 10.0;
			else
				query.m_fScoreWeightDirection = 1.0;
		}
		
		
		// Query sector properties
		query.m_vSectorPos = myPos;
		query.m_fSectorDistMin = rq.m_fCoverSearchDistMin;
		query.m_fSectorDistMax = rq.m_fCoverSearchDistMax;
		
		// Threat pos
		vector threatPos = ResolveThreatPos(myPos, rq);
		query.m_vThreatPos = threatPos;
		
		// Weight distance
		query.m_fScoreWeightDistance = 2.0;
		
		// Other
		query.m_vNearestPolyHalfExtend = SCR_AIFindCover.NEAREST_POLY_HALF_EXTEND;
		query.m_fNmAreaCostScale = SCR_AIFindCover.NAVMESH_AREA_COST_SCALE;
		query.m_fCoverHeightMin = 0;
		query.m_fCoverHeightMax = 10.0;
		query.m_iMaxCoversToCheck = SCR_AIFindCover.MAX_COVERS_HIGH_PRIORITY;
		
		
		query.m_fCoverToThreatAngleCosMin = m_fMinCoverToTargetAngleCos;
		query.m_bCheckVisibility = rq.m_bCheckCoverVisibility;
		query.m_bSelectHighestScore = false; // Select lowest score cover (nearest)
		
		SetVariableOut(PORT_COVER_QUERY_PROPERTIES, query);
		
		return ENodeResult.SUCCESS;
	}
}