class DCO_FindIndoorRelocatePosition : DCO_FindIndoorPosition
{

	[Attribute("1.0", UIWidgets.EditBox, "(DEFEND) Tolerance (m): candidates closer to the threat than the start position by more than this are rejected.", category: "Relocate")]
	protected float m_fTowardThreatTolerance;

	[Attribute("0.6", UIWidgets.Range, "(DEFEND) Weight of the threat score vs the parent score (general cover/fire line, spread, proximity).", params: "0 1 0.05", category: "Relocate Scoring")]
	protected float m_fThreatWeightDefend;

	[Attribute("0.7", UIWidgets.Range, "(FIRE_POSITION) Weight of the threat score vs the parent score.", params: "0 1 0.05", category: "Relocate Scoring")]
	protected float m_fThreatWeightFire;

	[Attribute("2.5", UIWidgets.EditBox, "Distance (m) of the threat sample points to the left and right of the threat position. 0 = a single point.", category: "Relocate Scoring")]
	protected float m_fThreatSampleSpread;





	protected DCO_AICombatMoveRequest_IndoorRelocate m_CurrentRq;
	protected IEntity m_LockedBuilding;
	protected vector  m_vRelocateStart;
	protected vector  m_vRelocateThreat;
	protected float   m_fRelocateMinMove;
	protected DCO_EIndoorRelocateIntent m_eIntent;
	protected float   m_fAttrRadius = -1;

	protected ref array<vector> m_aThreatSamples = {};
	protected bool              m_bSuppressBasePrune;

	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		if (!owner)
			return ENodeResult.FAIL;

		SCR_AICombatMoveRequestBase rqBase;
		GetVariableIn("Request", rqBase);
		DCO_AICombatMoveRequest_IndoorRelocate rq = DCO_AICombatMoveRequest_IndoorRelocate.Cast(rqBase);
		if (!rq || !rq.m_Building)
		{
			SetVariableOut(PORT_VECTOR_BOOL, false);
			return ENodeResult.FAIL;
		}

		if (m_fAttrRadius < 0)
			m_fAttrRadius = m_fRadius;

		if (rq != m_CurrentRq)
			BeginRequest(owner, rq);

		ENodeResult r = super.EOnTaskSimulate(owner, dt);
		if (r == ENodeResult.SUCCESS)
			rq.m_vDCOGoal = m_vBestPos;
		if (r == ENodeResult.SUCCESS && DCO_Log.On(DCO_ELogChannel.UNIT, DCO_ELogLevel.DEBUG))
		{
			IEntity me = owner.GetControlledEntity();
			vector fromPos = rq.m_vStartPos;
			if (me)
				fromPos = me.GetOrigin();
			DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(me), string.Format("reloc_pos who=%1 from=%2,%3 to=%4,%5 dist=%6 score=%7",
				DCO_Log.IdUnit(me), Math.Round(fromPos[0]), Math.Round(fromPos[2]), Math.Round(m_vBestPos[0]), Math.Round(m_vBestPos[2]), Math.Round(vector.DistanceXZ(fromPos, m_vBestPos)), m_fBestScore));
		}
		return r;
	}

	protected void BeginRequest(AIAgent owner, DCO_AICombatMoveRequest_IndoorRelocate rq)
	{
		m_CurrentRq        = rq;
		m_LockedBuilding   = rq.m_Building;
		m_vRelocateThreat  = rq.m_vTargetPos;
		m_fRelocateMinMove = rq.m_fMinMoveDist;
		m_eIntent          = rq.m_eIntent;

		IEntity myEntity = owner.GetControlledEntity();
		if (myEntity)
			m_vRelocateStart = myEntity.GetOrigin();
		else
			m_vRelocateStart = vector.Zero;

		rq.m_vStartPos = m_vRelocateStart;

		if (rq.m_fCoverSearchDistMax > 0)
			m_fRadius = rq.m_fCoverSearchDistMax;
		else
			m_fRadius = m_fAttrRadius;

		BuildThreatSamples();

		m_Building            = null;
		m_fAcquireFailTime_ms = 0;
	}

	protected void BuildThreatSamples()
	{
		m_aThreatSamples.Clear();

		if (m_vRelocateThreat == vector.Zero)
			return;

		m_aThreatSamples.Insert(m_vRelocateThreat);

		if (m_fThreatSampleSpread <= 0 || m_vRelocateStart == vector.Zero)
			return;

		vector toThreat = m_vRelocateThreat - m_vRelocateStart;
		toThreat[1] = 0;

		float len = toThreat.Length();
		if (len < 0.5)
			return;

		vector fwd   = toThreat / len;
		vector right = Vector(fwd[2], 0, -fwd[0]);

		m_aThreatSamples.Insert(m_vRelocateThreat + right * m_fThreatSampleSpread);
		m_aThreatSamples.Insert(m_vRelocateThreat - right * m_fThreatSampleSpread);
	}

	override protected bool AcquireBuilding(vector searchPos, float searchRad)
	{
		if (!m_LockedBuilding)
			return false;

		m_Building = m_LockedBuilding;
		m_Building.GetBounds(m_vLocalMins, m_vLocalMaxs);
		return true;
	}

	override protected DCO_BuildingPosCreation ValidateCandidate(vector queryPos, out vector outPos, out float nearestBooked)
	{
		DCO_BuildingPosCreation result = super.ValidateCandidate(queryPos, outPos, nearestBooked);
		if (result != DCO_BuildingPosCreation.SUCCESS)
			return result;

		if (m_vRelocateStart == vector.Zero)
			return result;

		if (m_fRelocateMinMove > 0 && vector.DistanceXZ(outPos, m_vRelocateStart) < m_fRelocateMinMove)
			return DCO_BuildingPosCreation.FAIL;

		if (Math.AbsFloat(outPos[1] - m_vRelocateStart[1]) > 2.0)
		{
			DCO_Perf.Count("reloc:reject_floor");
			return DCO_BuildingPosCreation.FAIL;
		}

		if (m_eIntent == DCO_EIndoorRelocateIntent.RELOCATE_DEFEND && m_vRelocateThreat != vector.Zero)
		{
			float startToThreat = vector.DistanceXZ(m_vRelocateStart, m_vRelocateThreat);
			float candToThreat  = vector.DistanceXZ(outPos, m_vRelocateThreat);

			if (candToThreat < startToThreat - m_fTowardThreatTolerance)
				return DCO_BuildingPosCreation.FAIL;
		}

		return result;
	}

	override protected bool DCO_UseSlotCache()
	{
		return false;
	}

	override protected float GetPruneThreshold()
	{
		if (m_bSuppressBasePrune)
			return -1;

		return super.GetPruneThreshold();
	}

	override protected float ScoreCandidate(vector pos, vector searchPos, float searchRad, float nearestBooked, float bonus)
	{
		m_bSuppressBasePrune = true;
		float baseScore = super.ScoreCandidate(pos, searchPos, searchRad, nearestBooked, bonus);
		m_bSuppressBasePrune = false;

		if (baseScore < 0)
			return baseScore;

		if (m_aThreatSamples.IsEmpty())
			return baseScore;

		float w = m_fThreatWeightDefend;
		if (m_eIntent == DCO_EIndoorRelocateIntent.FIRE_POSITION)
			w = m_fThreatWeightFire;

		w = Math.Clamp(w, 0.0, 1.0);

		float pruneAt = super.GetPruneThreshold();
		if (pruneAt >= 0 && ((1.0 - w) * baseScore + w) <= pruneAt)
			return -1;

		float threatScore = ScoreThreat(pos);
		if (threatScore < 0)
			return -1;

		return (1.0 - w) * baseScore + w * threatScore;
	}

	protected float ScoreThreat(vector pos)
	{
		int nFire    = 0;
		int nHidden  = 0;
		int nExposed = 0;

		vector hidePoint = pos + 0.7 * vector.Up;
		vector peekPoint = pos + 1.2 * vector.Up;

		foreach (vector sample : m_aThreatSamples)
		{
			vector eye = sample + 1.5 * vector.Up;

			if (HasLOS(eye, hidePoint))
			{
				nExposed++;
				continue;
			}

			if (HasLOS(eye, peekPoint))
				nFire++;
			else
				nHidden++;
		}

		float count    = m_aThreatSamples.Count();
		float fFire    = 1.0 * nFire / count;
		float fHidden  = 1.0 * nHidden / count;
		float fExposed = 1.0 * nExposed / count;

		if (m_eIntent == DCO_EIndoorRelocateIntent.FIRE_POSITION)
		{
			if ((fFire + fExposed) < 0.5)
				return -1;

			return Math.Clamp(fFire * 1.0 + fExposed * 0.35, 0.0, 1.0);
		}

		if (fExposed >= 0.5)
			return -1;

		float score = fFire * 1.0 + fHidden * 0.8 + fExposed * (-1.0);
		score += ScoreDoors(pos, peekPoint);

		return Math.Clamp(score, 0.0, 1.0);
	}

	protected float ScoreDoors(vector pos, vector peekPoint)
	{
		if (m_aQueryDoors.IsEmpty() || m_vRelocateStart == vector.Zero)
			return 0;

		float startToThreat = vector.DistanceXZ(m_vRelocateStart, m_vRelocateThreat);

		float adjust    = 0;
		bool  watching  = false;
		int   losChecks = 0;

		foreach (IEntity door : m_aQueryDoors)
		{
			if (!door)
				continue;

			vector doorPos = door.GetOrigin();
			if (vector.DistanceXZ(doorPos, m_vRelocateThreat) >= startToThreat)
				continue;

			float d = vector.DistanceXZ(pos, doorPos);

			if (d < 2.0)
			{
				adjust -= 0.3;
				continue;
			}

			if (watching || losChecks >= 2)
				continue;

			if (d < 3.0 || d > 8.0)
				continue;

			losChecks++;

			vector doorPeek = doorPos + 1.2 * vector.Up;
			vector toDoor   = doorPeek - peekPoint;
			float  toDoorLen = toDoor.Length();
			if (toDoorLen > 0.5)
				doorPeek = peekPoint + toDoor * ((toDoorLen - 0.5) / toDoorLen);

			if (HasLOS(peekPoint, doorPeek))
			{
				adjust  += 0.2;
				watching = true;
			}
		}

		return adjust;
	}

	protected bool HasLOS(vector from, vector to)
	{
		return DCO_Util.TraceClear(from, to, m_OwnerEntity, 0.99, TraceFlags.WORLD | TraceFlags.ENTS, EPhysicsLayerDefs.Projectile, null, "t:DCO_FindIndoorRelocatePosition");
	}

	protected static ref TStringArray s_aVarsInRelocate;
	protected static TStringArray GetVarsInRelocate()
	{
		if (!s_aVarsInRelocate)
			s_aVarsInRelocate = { PORT_CENTER_OF_SEARCH, PORT_RADIUS, "Request" };
		return s_aVarsInRelocate;
	}
	override TStringArray GetVariablesIn()
	{
		return GetVarsInRelocate();
	}

	static override bool VisibleInPalette() { return true; }

	static override string GetOnHoverDescription()
	{
		return "Find Indoor Relocate Position: the building is locked from the INDOOR_RELOCATE request. Scoring is based on threat direction (LOS at two heights: hide/peek), DEFEND or FIRE_POSITION intent, and guarding doors on the threat side.";
	}
}
