class DCO_FindIndoorRelocatePosition : DCO_FindIndoorPosition
{
	static const string PORT_REQUEST = "Request";

	[Attribute("1.0", UIWidgets.EditBox, "(DEFEND) Toleransi (m): kandidat yang lebih deket ke ancaman dari posisi awal lebih dari segini ditolak.", category: "Relocate")]
	protected float m_fTowardThreatTolerance;

	[Attribute("0.6", UIWidgets.Range, "(DEFEND) Bobot skor ancaman vs skor induk (cover/fire line umum, spread, proximity).", params: "0 1 0.05", category: "Relocate Scoring")]
	protected float m_fThreatWeightDefend;

	[Attribute("0.7", UIWidgets.Range, "(FIRE_POSITION) Bobot skor ancaman vs skor induk.", params: "0 1 0.05", category: "Relocate Scoring")]
	protected float m_fThreatWeightFire;

	[Attribute("2.5", UIWidgets.EditBox, "Jarak (m) titik sample ancaman ke kiri/kanan dari posisi ancaman. 0 = cuma satu titik.", category: "Relocate Scoring")]
	protected float m_fThreatSampleSpread;

	//--------------------------------------------------------------------
	protected static const float HIDE_HEIGHT        = 0.7;  //! Dada waktu jongkok
	protected static const float PEEK_HEIGHT        = 1.2;  //! Mata waktu jongkok
	protected static const float THREAT_EYE_HEIGHT  = 1.5;  //! Mata ancaman
	protected static const float LOS_CLEAR_FRACTION = 0.99; //! Trace bersih segini = kelihatan

	// Skor per klasifikasi, per intent
	protected static const float DEFEND_SCORE_FIRE    = 1.0;
	protected static const float DEFEND_SCORE_HIDDEN  = 0.8;
	protected static const float DEFEND_SCORE_EXPOSED = -1.0;
	protected static const float DEFEND_MAX_EXPOSED   = 0.5;  //! Fraksi sample EXPOSED >= ini -> ditolak

	protected static const float FIRE_SCORE_FIRE      = 1.0;
	protected static const float FIRE_SCORE_EXPOSED   = 0.35; //! Bisa nembak tapi gak ada cover
	protected static const float FIRE_MIN_CAN_SEE     = 0.5;  //! Fraksi sample yang kelihatan dari PEEK < ini -> ditolak

	// Pintu (DEFEND)
	protected static const float DOOR_FUNNEL_DIST     = 2.0;  //! Lebih deket dari ini ke pintu sisi ancaman = berdiri di jalur masuk
	protected static const float DOOR_WATCH_MIN       = 3.0;  //! Jarak jagain pintu yang enak
	protected static const float DOOR_WATCH_MAX       = 8.0;
	protected static const float DOOR_FUNNEL_PENALTY  = 0.3;
	protected static const float DOOR_WATCH_BONUS     = 0.2;
	protected static const int   DOOR_MAX_LOS_CHECKS  = 2;    //! Batas trace pintu per kandidat
	protected static const float DOOR_LOS_STOP_SHORT  = 0.5;  //! Trace LOS ke pintu berhenti segini sebelum pintu

	//--------------------------------------------------------------------
	protected DCO_AICombatMoveRequest_IndoorRelocate m_CurrentRq; // weak, request dipegang combat move state
	protected IEntity m_LockedBuilding;
	protected vector  m_vRelocateStart;
	protected vector  m_vRelocateThreat;
	protected float   m_fRelocateMinMove;
	protected DCO_EIndoorRelocateIntent m_eIntent;
	protected float   m_fAttrRadius = -1;

	protected ref array<vector> m_aThreatSamples = {};
	protected ref TraceParam    m_LosTrace;
	protected bool              m_bSuppressBasePrune;

	//------------------------------------------------------------------------------------------------
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		if (!owner)
			return ENodeResult.FAIL;

		SCR_AICombatMoveRequestBase rqBase;
		GetVariableIn(PORT_REQUEST, rqBase);
		DCO_AICombatMoveRequest_IndoorRelocate rq = DCO_AICombatMoveRequest_IndoorRelocate.Cast(rqBase);
		if (!rq || !rq.m_Building)
		{
			SetVariableOut(PORT_VECTOR_BOOL, false);
			return ENodeResult.FAIL;
		}

		// Simpan nilai attribute asli sekali, karena m_fRadius dipake ulang per request.
		if (m_fAttrRadius < 0)
			m_fAttrRadius = m_fRadius;

		if (rq != m_CurrentRq)
			BeginRequest(owner, rq);

		return super.EOnTaskSimulate(owner, dt);
	}

	//------------------------------------------------------------------------------------------------
	//! Request baru: kunci gedung, catat posisi awal, siapin sample ancaman, paksa pencarian ulang.
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

		// Induknya cache hasil per searchPos + TTL. searchPos (posisi unit) bisa sama persis
		// sama request sebelumnya, jadi paksa needNewSearch lewat m_Building = null.
		m_Building            = null;
		m_fAcquireFailTime_ms = 0;
	}

	//------------------------------------------------------------------------------------------------
	//! Titik tengah + kiri/kanan tegak lurus arah (unit -> ancaman). Posisi ancaman cuma perkiraan,
	//! jadi hasil LOS dirata-rata biar gak ditentuin satu ray yang kebetulan.
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

	//------------------------------------------------------------------------------------------------
	//! Gak pake query sphere sama sekali -- gedungnya udah pasti dari request.
	override protected bool AcquireBuilding(vector searchPos, float searchRad)
	{
		if (!m_LockedBuilding)
			return false;

		m_Building = m_LockedBuilding;
		m_Building.GetBounds(m_vLocalMins, m_vLocalMaxs);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override protected DCO_BuildingPosCreation ValidateCandidate(vector queryPos, out vector outPos, out float nearestBooked)
	{
		DCO_BuildingPosCreation result = super.ValidateCandidate(queryPos, outPos, nearestBooked);
		if (result != DCO_BuildingPosCreation.SUCCESS)
			return result;

		if (m_vRelocateStart == vector.Zero)
			return result;

		// Harus beneran pindah. Ini juga otomatis nolak kandidat "posisi booking lama" (stickiness).
		if (m_fRelocateMinMove > 0 && vector.DistanceXZ(outPos, m_vRelocateStart) < m_fRelocateMinMove)
			return DCO_BuildingPosCreation.FAIL;

		// DEFEND: jangan relocate ke arah musuh. FIRE_POSITION boleh -- kadang posisi tembak ada di depan.
		if (m_eIntent == DCO_EIndoorRelocateIntent.RELOCATE_DEFEND && m_vRelocateThreat != vector.Zero)
		{
			float startToThreat = vector.DistanceXZ(m_vRelocateStart, m_vRelocateThreat);
			float candToThreat  = vector.DistanceXZ(outPos, m_vRelocateThreat);

			if (candToThreat < startToThreat - m_fTowardThreatTolerance)
				return DCO_BuildingPosCreation.FAIL;
		}

		return result;
	}

	//------------------------------------------------------------------------------------------------
	//! Pruning induk dimatiin selama super.ScoreCandidate: upper bound-nya cuma ngitung skor induk,
	//! padahal skor akhir dicampur skor ancaman -- bisa kebuang kandidat yang sebenernya menang.
	override protected float GetPruneThreshold()
	{
		if (m_bSuppressBasePrune)
			return -1;

		return super.GetPruneThreshold();
	}

	//------------------------------------------------------------------------------------------------
	override protected float ScoreCandidate(vector pos, vector searchPos, float searchRad, float nearestBooked, float bonus)
	{
		m_bSuppressBasePrune = true;
		float baseScore = super.ScoreCandidate(pos, searchPos, searchRad, nearestBooked, bonus);
		m_bSuppressBasePrune = false;

		if (baseScore < 0)
			return baseScore;

		// Gak ada info ancaman -> cuma skor induk.
		if (m_aThreatSamples.IsEmpty())
			return baseScore;

		float w = m_fThreatWeightDefend;
		if (m_eIntent == DCO_EIndoorRelocateIntent.FIRE_POSITION)
			w = m_fThreatWeightFire;

		w = Math.Clamp(w, 0.0, 1.0);

		// Prune sendiri: skor ancaman maksimum 1. Kalau tetap gak masuk top-K, skip trace LOS.
		float pruneAt = super.GetPruneThreshold();
		if (pruneAt >= 0 && ((1.0 - w) * baseScore + w) <= pruneAt)
			return -1;

		float threatScore = ScoreThreat(pos);
		if (threatScore < 0)
			return -1;

		return (1.0 - w) * baseScore + w * threatScore;
	}

	//------------------------------------------------------------------------------------------------
	//! 0..1, atau -1 = ditolak (syarat keras intent gak kepenuhi).
	protected float ScoreThreat(vector pos)
	{
		int nFire    = 0;
		int nHidden  = 0;
		int nExposed = 0;

		vector hidePoint = pos + HIDE_HEIGHT * vector.Up;
		vector peekPoint = pos + PEEK_HEIGHT * vector.Up;

		foreach (vector sample : m_aThreatSamples)
		{
			vector eye = sample + THREAT_EYE_HEIGHT * vector.Up;

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
		float fFire    = nFire / count;
		float fHidden  = nHidden / count;
		float fExposed = nExposed / count;

		if (m_eIntent == DCO_EIndoorRelocateIntent.FIRE_POSITION)
		{
			// Wajib bisa lihat area target (FIRE atau EXPOSED sama-sama bersih di PEEK).
			if ((fFire + fExposed) < FIRE_MIN_CAN_SEE)
				return -1;

			return Math.Clamp(fFire * FIRE_SCORE_FIRE + fExposed * FIRE_SCORE_EXPOSED, 0.0, 1.0);
		}

		// RELOCATE_DEFEND
		if (fExposed >= DEFEND_MAX_EXPOSED)
			return -1;

		float score = fFire * DEFEND_SCORE_FIRE + fHidden * DEFEND_SCORE_HIDDEN + fExposed * DEFEND_SCORE_EXPOSED;
		score += ScoreDoors(pos, peekPoint);

		return Math.Clamp(score, 0.0, 1.0);
	}

	//------------------------------------------------------------------------------------------------
	//! Pintu "sisi ancaman" = pintu yang lebih deket ke ancaman daripada posisi awal unit.
	//! Berdiri di jalur masuknya -> penalti. Jagain dari jarak enak dengan LOS -> bonus (sekali, gak numpuk).
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

			if (d < DOOR_FUNNEL_DIST)
			{
				adjust -= DOOR_FUNNEL_PENALTY;
				continue;
			}

			if (watching || losChecks >= DOOR_MAX_LOS_CHECKS)
				continue;

			if (d < DOOR_WATCH_MIN || d > DOOR_WATCH_MAX)
				continue;

			losChecks++;

			// Berhenti 0.5m sebelum pintu: pintu tertutup sendiri gak boleh kebaca "gak ada LOS".
			vector doorPeek = doorPos + PEEK_HEIGHT * vector.Up;
			vector toDoor   = doorPeek - peekPoint;
			float  toDoorLen = toDoor.Length();
			if (toDoorLen > DOOR_LOS_STOP_SHORT)
				doorPeek = peekPoint + toDoor * ((toDoorLen - DOOR_LOS_STOP_SHORT) / toDoorLen);

			if (HasLOS(peekPoint, doorPeek))
			{
				adjust  += DOOR_WATCH_BONUS;
				watching = true;
			}
		}

		return adjust;
	}

	//------------------------------------------------------------------------------------------------
	//! LOS pake layer Projectile (sama kayak IsInOpenArea / GrenadeUtil / AimImprovement), supaya
	//! kaca/tirai/pagar diperlakuin sama kayak peluru. Trace induk (ENTS polos) sengaja gak diubah.
	protected bool HasLOS(vector from, vector to)
	{
		if (!m_LosTrace)
			m_LosTrace = new TraceParam();

		m_LosTrace.Flags     = TraceFlags.WORLD | TraceFlags.ENTS;
		m_LosTrace.LayerMask = EPhysicsLayerDefs.Projectile;
		m_LosTrace.Start     = from;
		m_LosTrace.End       = to;
		m_LosTrace.Exclude   = m_OwnerEntity;
		m_LosTrace.TraceEnt  = null;

		return GetGame().GetWorld().TraceMove(m_LosTrace, null) >= LOS_CLEAR_FRACTION;
	}

	//------------------------------------------------------------------------------------------------
	protected static ref TStringArray s_aVarsInRelocate = {
		PORT_CENTER_OF_SEARCH,
		PORT_RADIUS,
		PORT_REQUEST
	};
	override TStringArray GetVariablesIn()
	{
		return s_aVarsInRelocate;
	}

	//------------------------------------------------------------------------------------------------
	static override bool VisibleInPalette() { return true; }

	//------------------------------------------------------------------------------------------------
	static override string GetOnHoverDescription()
	{
		return "Find Indoor Relocate Position: gedung dikunci dari request INDOOR_RELOCATE. Scoring berbasis arah ancaman (LOS dua ketinggian: hide/peek), intent DEFEND atau FIRE_POSITION, penjagaan pintu sisi ancaman.";
	}
}