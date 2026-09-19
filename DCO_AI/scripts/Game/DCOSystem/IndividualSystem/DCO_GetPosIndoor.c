class DCO_FindIndoorPosition: AITaskScripted
{
	static const string PORT_CENTER_OF_SEARCH	= "OriginIn";
	static const string PORT_RADIUS				= "RadiusIn";
	static const string PORT_VECTOR_BOOL		= "Is Position Found";
	static const string PORT_VECTOR_POS			= "Position Found";

	[Attribute("0", UIWidgets.EditBox)]
	protected float m_fRadius;

	// === Budget pencarian ===
	[Attribute("6", UIWidgets.EditBox, "Berapa kandidat dievaluasi per tick BT. Naikin buat hasil lebih cepat, turunin kalau server kerasa berat.", category: "Search Budget")]
	protected int m_iCandidatesPerTick;

	[Attribute("150", UIWidgets.EditBox, "Total kandidat maksimum sebelum nyerah. Pencarian juga berhenti sendiri begitu semua cell gedung udah dicoba.", category: "Search Budget")]
	protected int m_iMaxCandidates;

	[Attribute("20", UIWidgets.EditBox, "Maksimum tick nunggu navmesh tile ke-load sebelum nyerah. Nunggu tile gak dihitung sebagai attempt.", category: "Search Budget")]
	protected int m_iMaxTileWaitTicks;

	[Attribute("0.72", UIWidgets.Range, "Skor yang dianggap 'udah bagus banget' -- kandidat segini langsung diambil tanpa nyari lagi.\nCATATAN: skor MAKSIMUM teoretis cuma ~0.869 (cover dan fireLine saling nekan). Jangan set di atas ~0.80, jalan pintasnya gak akan pernah kepake.", params: "0 1 0.01", category: "Search Budget")]
	protected float m_fGoodEnoughScore;

	[Attribute("15.0", UIWidgets.EditBox, "Berapa detik hasil pencarian dianggap masih valid sebelum dicari ulang.", category: "Search Budget")]
	protected float m_fResultTTL;

	// === Sampling ===
	[Attribute("2.5", UIWidgets.EditBox, "Ukuran cell horizontal (m) buat sampling stratified. Kecil = lebih teliti tapi lebih banyak kandidat.", category: "Sampling")]
	protected float m_fCellSize;

	[Attribute("3.0", UIWidgets.EditBox, "Tinggi cell (m) -- kira-kira tinggi satu lantai.", category: "Sampling")]
	protected float m_fCellHeight;

	[Attribute("1.0", UIWidgets.EditBox, "Kandidat yang nge-snap (navmesh) sedekat ini ke kandidat yang udah dievaluasi dibuang tanpa di-score.", category: "Sampling")]
	protected float m_fDedupeRadius;

	[Attribute("3", UIWidgets.EditBox, "Jumlah kandidat terbaik yang disimpan buat verifikasi akhir (kalau yang #1 keisi orang, ambil #2, dst).", category: "Sampling")]
	protected int m_iTopCandidates;

	// === Filter keras (hard reject) ===
	[Attribute("1.6", UIWidgets.EditBox, "Tinggi langit-langit minimum (m). Nolak crawlspace / kolong.", category: "Hard Filter")]
	protected float m_fMinHeadroom;

	[Attribute("3.0", UIWidgets.EditBox, "Radius (m) ngecek karakter lain -- posisi ditolak kalau ada orang segini deket. AI ini sendiri gak dihitung.", category: "Hard Filter")]
	protected float m_fOccupancyRadius;

	[Attribute("1.2", UIWidgets.EditBox, "Radius (m) ngecek pintu. KECIL -- cuma nyegah AI berdiri MENGHALANGI pintu.", category: "Hard Filter")]
	protected float m_fDoorBlockRadius;

	[Attribute("3.0", UIWidgets.EditBox, "Jarak minimum (m) ke posisi yang udah di-book AI lain.", category: "Hard Filter")]
	protected float m_fMinBookedDistance;

	[Attribute("1", UIWidgets.CheckBox, "Wajib punya minimal satu arah tembak (keluar gedung atau interior panjang). Matiin kalau AI kebanyakan gagal dapet posisi di gedung padat.", category: "Hard Filter")]
	protected bool m_bRequireFireLine;

	// === Bobot skor. Gak harus total 1 -- dinormalisasi otomatis. ===
	[Attribute("0.35", UIWidgets.Range, "Bobot: seberapa terlindungi posisinya (banyak dinding di sekitar).", params: "0 1 0.01", category: "Scoring")]
	protected float m_fWeightCover;

	[Attribute("0.35", UIWidgets.Range, "Bobot: seberapa bagus arah tembaknya.", params: "0 1 0.01", category: "Scoring")]
	protected float m_fWeightFireLine;

	[Attribute("0.15", UIWidgets.Range, "Bobot: seberapa jauh dari posisi AI lain (biar gak numpuk di satu sudut).", params: "0 1 0.01", category: "Scoring")]
	protected float m_fWeightSpread;

	[Attribute("0.15", UIWidgets.Range, "Bobot: seberapa deket ke titik yang diminta BT.", params: "0 1 0.01", category: "Scoring")]
	protected float m_fWeightProximity;

	[Attribute("0.5", UIWidgets.Range, "Nilai fire line yang cuma nyapu interior (koridor/aula) dibanding yang keluar gedung lewat jendela/pintu (= 1.0).", params: "0 1 0.05", category: "Scoring")]
	protected float m_fInteriorLineValue;

	[Attribute("0.05", UIWidgets.Range, "Bonus skor buat posisi yang lagi di-booking AI ini. Nyegah AI pindah-pindah tiap TTL habis kalau posisi barunya cuma sedikit lebih bagus.", params: "0 0.3 0.01", category: "Scoring")]
	protected float m_fStickinessBonus;

	//--------------------------------------------------------------------
	// Konstanta geometri penilaian
	protected static const int   RAY_COUNT        = 8;     //! Jumlah arah horizontal yang di-probe
	protected static const float RAY_COUNT_F      = 8.0;   //! Kembaran float-nya -- hindari integer division
	protected static const float FIRE_LINE_IDEAL  = 3.0;   //! Nilai fire line yang udah dianggap cukup
	protected static const float RAY_MAX_DIST     = 15.0;  //! Panjang maksimum probe
	protected static const float WALL_NEAR_DIST   = 2.5;   //! Kena sesuatu di bawah ini = dianggap "ada dinding/cover"
	protected static const float FIRE_LINE_DIST   = 8.0;   //! Bersih sejauh ini = dianggap arah tembak valid
	protected static const float CHEST_HEIGHT     = 1.0;   //! Tinggi probe horizontal
	protected static const float EYE_POS          = 1.55;
	protected static const float HEADROOM_PROBE   = 6.0;   //! Panjang probe vertikal ke atas
	protected static const float SPREAD_IDEAL_DIST = 8.0;  //! Jarak ke AI lain yang dianggap "udah cukup nyebar"
	protected static const float MIN_BUILDING_HALF_WIDTH = 4.0;
	protected static const float EXIT_MARGIN      = 0.5;   //! Ray harus bersih segini lewat batas gedung buat dianggap "keluar"
	protected static const float ACQUIRE_RETRY_MS = 2000.0; //! Jeda sebelum nyari gedung lagi setelah gagal di searchPos yang sama
	protected static const float BOUNDS_MARGIN    = 0.5;   //! Toleransi cek "posisi masih di dalam bounds gedung"

	//--------------------------------------------------------------------
	protected IEntity m_Building;
	protected IEntity m_OwnerEntity;
	protected vector  m_vLastSearchPos = vector.Zero;
	protected vector  m_vLocalMins, m_vLocalMaxs;

	protected ref array<IEntity> m_aQueryFoundBuilding = {};
	protected ref array<IEntity> m_aQueryCharacters    = {};
	protected ref array<IEntity> m_aQueryDoors         = {};

	protected ref array<int>    m_aCells     = {};
	protected ref array<vector> m_aEvaluated = {};
	protected ref array<vector> m_aTopPos    = {};
	protected ref array<float>  m_aTopScore  = {};

	protected ref TraceParam m_Trace;

	protected NavmeshWorldComponent m_pNavmesh;

	protected int    m_iCellsX, m_iCellsY, m_iCellsZ;
	protected int    m_iCellCursor;

	protected int    m_iAttempt;
	protected int    m_iTileWaitTicks;
	protected bool   m_bSearchDone;
	protected vector m_vBestPos;
	protected float  m_fBestScore;
	protected float  m_fResultTime_ms;

	protected vector m_vPrevBookedPos;
	protected bool   m_bPendingPrev;

	protected vector m_vAcquireFailPos;
	protected float  m_fAcquireFailTime_ms;

	//------------------------------------------------------------------------------------------------
	static override bool VisibleInPalette() { return true; }

	//------------------------------------------------------------------------------------------------
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		if (!owner)
			return ENodeResult.FAIL;

		vector searchPos;
		float  searchRad;

		GetVariableIn(PORT_CENTER_OF_SEARCH, searchPos);
		if (!GetVariableIn(PORT_RADIUS, searchRad))
			searchRad = m_fRadius;

		if (searchPos == vector.Zero)
			return ENodeResult.FAIL;

		float now_ms = GetGame().GetWorld().GetWorldTime();

		// Baru aja gagal nemu gedung di titik yang sama -- jangan query ulang tiap tick.
		if (!m_Building && m_fAcquireFailTime_ms > 0
			&& vector.DistanceSq(searchPos, m_vAcquireFailPos) <= 1.0
			&& (now_ms - m_fAcquireFailTime_ms) < ACQUIRE_RETRY_MS)
		{
			SetVariableOut(PORT_VECTOR_BOOL, false);
			return ENodeResult.FAIL;
		}

		bool needNewSearch = !m_Building
			|| vector.DistanceSq(searchPos, m_vLastSearchPos) > 1.0
			|| (m_bSearchDone && (now_ms - m_fResultTime_ms) > (m_fResultTTL * 1000.0));

		if (needNewSearch)
		{
			ResetSearch(searchPos, owner.GetControlledEntity());

			if (!AcquireBuilding(searchPos, searchRad))
			{
				m_vAcquireFailPos     = searchPos;
				m_fAcquireFailTime_ms = now_ms;
				SetVariableOut(PORT_VECTOR_BOOL, false);
				return ENodeResult.FAIL;
			}

			m_fAcquireFailTime_ms = 0;
			PrepareSearch(searchPos, searchRad);
		}

		if (!m_Building)
		{
			SetVariableOut(PORT_VECTOR_BOOL, false);
			return ENodeResult.FAIL;
		}

		if (!m_bSearchDone)
		{
			RunSearchBatch(searchPos, searchRad);

			if (!m_bSearchDone)
				return ENodeResult.RUNNING;
		}

		if (m_fBestScore <= 0)
		{
			SetVariableOut(PORT_VECTOR_BOOL, false);
			return ENodeResult.FAIL;
		}

		SCR_CoverManagerComponent coverMgr = SCR_CoverManagerComponent.GetInstance();
		if (coverMgr)
			coverMgr.RegisterPosition(owner.GetControlledEntity(), m_vBestPos);

		SetVariableOut(PORT_VECTOR_POS, m_vBestPos);
		SetVariableOut(PORT_VECTOR_BOOL, true);
		return ENodeResult.SUCCESS;
	}

	//------------------------------------------------------------------------------------------------
	protected void ResetSearch(vector searchPos, IEntity ownerEntity)
	{
		m_vLastSearchPos = searchPos;
		m_OwnerEntity    = ownerEntity;
		m_bSearchDone    = false;
		m_iAttempt       = 0;
		m_iTileWaitTicks = 0;
		m_fBestScore     = 0;
		m_vBestPos       = vector.Zero;
		m_Building       = null;
		m_iCellCursor    = 0;
		m_bPendingPrev   = false;
		m_vPrevBookedPos = vector.Zero;

		m_aCells.Clear();
		m_aEvaluated.Clear();
		m_aTopPos.Clear();
		m_aTopScore.Clear();
	}

	//------------------------------------------------------------------------------------------------
	protected bool AcquireBuilding(vector searchPos, float searchRad)
	{
		m_aQueryFoundBuilding.Clear();
		GetGame().GetWorld().QueryEntitiesBySphere(searchPos, searchRad, QueryCallback);

		IEntity nearestEntity = null;
		float smallestDistSq = float.MAX;

		foreach (IEntity e : m_aQueryFoundBuilding)
		{
			if (!e)
				continue;

			float distSq = vector.DistanceSq(e.GetOrigin(), searchPos);
			if (distSq < smallestDistSq)
			{
				nearestEntity  = e;
				smallestDistSq = distSq;
			}
		}

		if (!nearestEntity)
			return false;

		DCO_BuildingPositionComponent buildPosComp = DCO_BuildingPositionComponent.Cast(nearestEntity.FindComponent(DCO_BuildingPositionComponent));
		if (!buildPosComp)
			return false;

		m_Building = buildPosComp.GetBuildingEntity();
		if (!m_Building)
			return false;

		m_Building.GetBounds(m_vLocalMins, m_vLocalMaxs);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Sekali per pencarian: bikin grid cell, snapshot karakter+pintu, siapin posisi lama.
	protected void PrepareSearch(vector searchPos, float searchRad)
	{
		BuildCells(searchPos, searchRad);
		SnapshotOccupants();

		SCR_CoverManagerComponent coverMgr = SCR_CoverManagerComponent.GetInstance();
		if (!coverMgr || !m_OwnerEntity)
			return;

		// Booking lama AI ini sendiri dilepas supaya gak ngeblok dirinya sendiri
		// (spread + min booked distance). Nanti di-register ulang pas SUCCESS.
		vector prev = coverMgr.GetBookedPosition(m_OwnerEntity);
		coverMgr.ReleasePosition(m_OwnerEntity);

		if (prev != vector.Zero && IsInsideBuildingBounds(prev))
		{
			m_vPrevBookedPos = prev;
			m_bPendingPrev   = true;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void BuildCells(vector searchPos, float searchRad)
	{
		vector size = m_vLocalMaxs - m_vLocalMins;

		float cellXZ = Math.Max(m_fCellSize, 0.5);
		float cellY  = Math.Max(m_fCellHeight, 1.0);

		m_iCellsX = Math.Ceil(size[0] / cellXZ);
		m_iCellsY = Math.Ceil(size[1] / cellY);
		m_iCellsZ = Math.Ceil(size[2] / cellXZ);

		if (m_iCellsX < 1) m_iCellsX = 1;
		if (m_iCellsY < 1) m_iCellsY = 1;
		if (m_iCellsZ < 1) m_iCellsZ = 1;

		int total = m_iCellsX * m_iCellsY * m_iCellsZ;

		// Skip cell yang pusatnya jelas di luar radius pencarian.
		if (searchRad > 0)
		{
			float cw = size[0] / m_iCellsX;
			float ch = size[1] / m_iCellsY;
			float cd = size[2] / m_iCellsZ;
			float halfDiag = 0.5 * Math.Sqrt(cw * cw + ch * ch + cd * cd);
			float limit    = searchRad + halfDiag;
			float limitSq  = limit * limit;

			for (int idx = 0; idx < total; idx++)
			{
				vector localCenter = GetCellLocalPoint(idx, 0.5, 0.5, 0.5);
				vector worldCenter = m_Building.CoordToParent(localCenter);

				if (vector.DistanceSq(worldCenter, searchPos) <= limitSq)
					m_aCells.Insert(idx);
			}
		}

		// Radius 0 atau semua cell kefilter -> pake semua cell.
		if (m_aCells.IsEmpty())
		{
			for (int j = 0; j < total; j++)
			{
				m_aCells.Insert(j);
			}
		}

		// Fisher-Yates: urutan acak tapi tiap cell kebagian tepat sekali.
		for (int k = m_aCells.Count() - 1; k > 0; k--)
		{
			int r = Math.RandomInt(0, k + 1);
			int tmp = m_aCells[k];
			m_aCells[k] = m_aCells[r];
			m_aCells[r] = tmp;
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Titik lokal di dalam cell. fx/fy/fz = posisi relatif 0..1 di dalam cell.
	protected vector GetCellLocalPoint(int idx, float fx, float fy, float fz)
	{
		int ix   = idx % m_iCellsX;
		int rest = idx / m_iCellsX; // integer division memang disengaja (decode index)
		int iz   = rest % m_iCellsZ;
		int iy   = rest / m_iCellsZ;

		vector size = m_vLocalMaxs - m_vLocalMins;

		vector local;
		local[0] = m_vLocalMins[0] + (ix + fx) * (size[0] / m_iCellsX);
		local[1] = m_vLocalMins[1] + (iy + fy) * (size[1] / m_iCellsY);
		local[2] = m_vLocalMins[2] + (iz + fz) * (size[2] / m_iCellsZ);
		return local;
	}

	//------------------------------------------------------------------------------------------------
	protected vector SampleCell(int idx)
	{
		vector local    = GetCellLocalPoint(idx, Math.RandomFloat01(), Math.RandomFloat01(), Math.RandomFloat01());
		vector worldPos = m_Building.CoordToParent(local);

		float groundHeight = GetGame().GetWorld().GetSurfaceY(worldPos[0], worldPos[2]);
		if (worldPos[1] < groundHeight)
			worldPos[1] = groundHeight;

		return worldPos;
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsInsideBuildingBounds(vector worldPos)
	{
		if (!m_Building)
			return false;

		vector local = m_Building.CoordToLocal(worldPos);
		for (int a = 0; a < 3; a++)
		{
			if (local[a] < m_vLocalMins[a] - BOUNDS_MARGIN || local[a] > m_vLocalMaxs[a] + BOUNDS_MARGIN)
				return false;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected void RunSearchBatch(vector searchPos, float searchRad)
	{
		if (!m_pNavmesh)
			m_pNavmesh = GetGame().GetAIWorld().GetNavmeshWorldComponent("Soldiers");

		if (!m_pNavmesh)
		{
			FinalizeResult();
			return;
		}

		for (int i = 0; i < m_iCandidatesPerTick; i++)
		{
			bool cellsExhausted = !m_bPendingPrev && m_iCellCursor >= m_aCells.Count();
			if (m_iAttempt >= m_iMaxCandidates || cellsExhausted)
			{
				FinalizeResult();
				return;
			}

			vector queryPos;
			float  bonus  = 0;
			bool   isPrev = m_bPendingPrev;

			if (isPrev)
			{
				queryPos = m_vPrevBookedPos;
				bonus    = m_fStickinessBonus;
			}
			else
			{
				queryPos = SampleCell(m_aCells[m_iCellCursor]);
			}

			vector validPos;
			float  nearestBooked;

			DCO_BuildingPosCreation result = ValidateCandidate(queryPos, validPos, nearestBooked);

			if (result == DCO_BuildingPosCreation.RUNNING)
			{
				// Navmesh tile lagi di-load. Cell gak dimajuin -- tick depan dicoba lagi.
				m_iTileWaitTicks++;

				if (m_iTileWaitTicks > m_iMaxTileWaitTicks)
					FinalizeResult();

				return;
			}

			if (isPrev)
				m_bPendingPrev = false;
			else
				m_iCellCursor++;

			m_iAttempt++;

			if (result != DCO_BuildingPosCreation.SUCCESS)
				continue;

			float score = ScoreCandidate(validPos, searchPos, searchRad, nearestBooked, bonus);
			if (score < 0)
				continue;

			InsertTopCandidate(validPos, score);

			// Udah bagus banget -- gak usah nyari lagi.
			if (m_fBestScore >= m_fGoodEnoughScore)
			{
				FinalizeResult();
				return;
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Ambil kandidat top-K pertama yang masih kosong. Karakter bisa masuk area selama
	//! pencarian (snapshot-nya diambil di awal), jadi dicek ulang di sini.
	protected void FinalizeResult()
	{
		m_bSearchDone    = true;
		m_fResultTime_ms = GetGame().GetWorld().GetWorldTime();

		m_fBestScore = 0;
		m_vBestPos   = vector.Zero;

		for (int i = 0; i < m_aTopPos.Count(); i++)
		{
			if (IsOccupiedByCharacterNow(m_aTopPos[i]))
				continue;

			m_fBestScore = m_aTopScore[i];
			m_vBestPos   = m_aTopPos[i];
			return;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void InsertTopCandidate(vector pos, float score)
	{
		int maxKeep = Math.Max(1, m_iTopCandidates);

		int insertAt = m_aTopScore.Count();
		for (int i = 0; i < m_aTopScore.Count(); i++)
		{
			if (score > m_aTopScore[i])
			{
				insertAt = i;
				break;
			}
		}

		if (insertAt >= maxKeep)
			return;

		m_aTopScore.InsertAt(score, insertAt);
		m_aTopPos.InsertAt(pos, insertAt);

		while (m_aTopScore.Count() > maxKeep)
		{
			int last = m_aTopScore.Count() - 1;
			m_aTopScore.Remove(last);
			m_aTopPos.Remove(last);
		}

		m_fBestScore = m_aTopScore[0];
		m_vBestPos   = m_aTopPos[0];
	}

	//------------------------------------------------------------------------------------------------
	//! Skor minimum buat masuk top-K. -1 = list belum penuh, jangan prune.
	protected float GetPruneThreshold()
	{
		int maxKeep = Math.Max(1, m_iTopCandidates);
		if (m_aTopScore.Count() < maxKeep)
			return -1;

		return m_aTopScore[maxKeep - 1];
	}

	//------------------------------------------------------------------------------------------------
	bool QueryCallback(IEntity e)
	{
		if (!e)
			return true;

		DCO_BuildingPositionComponent comp = DCO_BuildingPositionComponent.Cast(e.FindComponent(DCO_BuildingPositionComponent));
		if (!comp || !comp.GetOwner())
			return true;

		vector mins, maxs;
		comp.GetOwner().GetBounds(mins, maxs);

		float halfWidth = 0.5 * (maxs[0] - mins[0]);
		if (halfWidth > MIN_BUILDING_HALF_WIDTH)
			m_aQueryFoundBuilding.Insert(e);

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Urutan: murah dulu (navmesh, dedupe, occupancy = math), trace belakangan.
	protected DCO_BuildingPosCreation ValidateCandidate(vector queryPos, out vector outPos, out float nearestBooked)
	{
		nearestBooked = -1;

		if (m_pNavmesh.IsTileRequested(queryPos))
			return DCO_BuildingPosCreation.RUNNING;

		if (!m_pNavmesh.IsTileLoaded(queryPos))
		{
			m_pNavmesh.LoadTileIn(queryPos);
			return DCO_BuildingPosCreation.RUNNING;
		}

		if (!m_pNavmesh.GetReachablePoint(queryPos, 2, outPos))
			return DCO_BuildingPosCreation.FAIL;

		if (IsDuplicate(outPos))
			return DCO_BuildingPosCreation.FAIL;

		m_aEvaluated.Insert(outPos);

		if (!IsInsideBuildingBounds(outPos))
			return DCO_BuildingPosCreation.FAIL;

		if (IsPositionOccupied(outPos, nearestBooked))
			return DCO_BuildingPosCreation.FAIL;

		// Lantai harus bagian dari gedung ini.
		float floorFrac = TraceFraction(outPos + EYE_POS * vector.Up, outPos - 5 * vector.Up);
		if (floorFrac >= 0.9)
			return DCO_BuildingPosCreation.FAIL;

		IEntity floorEnt = m_Trace.TraceEnt;
		if (!floorEnt)
			return DCO_BuildingPosCreation.FAIL;

		if (floorEnt.GetRootParent() != m_Building.GetRootParent())
			return DCO_BuildingPosCreation.FAIL;

		if (MeasureHeadroom(outPos) < m_fMinHeadroom)
			return DCO_BuildingPosCreation.FAIL;

		return DCO_BuildingPosCreation.SUCCESS;
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsDuplicate(vector pos)
	{
		float dedupeSq = m_fDedupeRadius * m_fDedupeRadius;
		if (dedupeSq <= 0)
			return false;

		foreach (vector p : m_aEvaluated)
		{
			if (vector.DistanceSq(p, pos) < dedupeSq)
				return true;
		}
		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! Trace pake TraceParam yang dipake ulang. Hasil entity ada di m_Trace.TraceEnt.
	protected float TraceFraction(vector start, vector end)
	{
		if (!m_Trace)
			m_Trace = new TraceParam();

		m_Trace.Flags    = TraceFlags.ENTS;
		m_Trace.Start    = start;
		m_Trace.End      = end;
		m_Trace.TraceEnt = null;

		return GetGame().GetWorld().TraceMove(m_Trace, null);
	}

	//------------------------------------------------------------------------------------------------
	protected float MeasureHeadroom(vector floorPos)
	{
		float frac = TraceFraction(floorPos + 0.1 * vector.Up, floorPos + HEADROOM_PROBE * vector.Up);
		return frac * HEADROOM_PROBE;
	}

	//------------------------------------------------------------------------------------------------
	//! Jarak dari origin sampai ray keluar dari bounds lokal gedung (slab method).
	protected float GetBoundsExitDistance(vector worldOrigin, vector worldDir)
	{
		vector o = m_Building.CoordToLocal(worldOrigin);
		vector d = m_Building.VectorToLocal(worldDir);

		float tExit = float.MAX;

		for (int a = 0; a < 3; a++)
		{
			if (Math.AbsFloat(d[a]) < 0.0001)
				continue;

			float t1 = (m_vLocalMins[a] - o[a]) / d[a];
			float t2 = (m_vLocalMaxs[a] - o[a]) / d[a];
			float tFar = Math.Max(t1, t2);

			if (tFar < tExit)
				tExit = tFar;
		}

		if (tExit < 0)
			return 0;

		return tExit;
	}

	//------------------------------------------------------------------------------------------------
	protected float ScoreCandidate(vector pos, vector searchPos, float searchRad, float nearestBooked, float bonus)
	{
		// Komponen murah dulu -- dipake juga buat upper bound early-out.
		float spreadScore = 1.0;
		if (nearestBooked > 0)
			spreadScore = Math.Clamp(nearestBooked / SPREAD_IDEAL_DIST, 0.0, 1.0);

		float proximityScore = 1.0;
		if (searchRad > 0)
			proximityScore = 1.0 - Math.Clamp(vector.Distance(pos, searchPos) / searchRad, 0.0, 1.0);

		float totalWeight = m_fWeightCover + m_fWeightFireLine + m_fWeightSpread + m_fWeightProximity;
		float pruneAt     = GetPruneThreshold();
		float cheapPart   = (m_fWeightSpread * spreadScore) + (m_fWeightProximity * proximityScore);

		vector probeOrigin = pos + CHEST_HEIGHT * vector.Up;

		int   blockedDirections = 0;
		float fireValue         = 0;

		for (int i = 0; i < RAY_COUNT; i++)
		{
			float angleRad = (i * 360.0 / RAY_COUNT_F) * Math.DEG2RAD;
			vector dir = Vector(Math.Cos(angleRad), 0, Math.Sin(angleRad));

			float hitDist = TraceFraction(probeOrigin, probeOrigin + dir * RAY_MAX_DIST) * RAY_MAX_DIST;

			if (hitDist < WALL_NEAR_DIST)
			{
				blockedDirections++;
			}
			else if (hitDist >= FIRE_LINE_DIST)
			{
				float exitDist = GetBoundsExitDistance(probeOrigin, dir);
				if (hitDist >= exitDist + EXIT_MARGIN)
					fireValue += 1.0;               // tembus keluar gedung (jendela/pintu)
				else
					fireValue += m_fInteriorLineValue; // cuma nyapu interior
			}

			// Early-out: anggap semua ray sisa ideal. Kalau tetap gak masuk top-K, stop.
			if (pruneAt >= 0 && totalWeight > 0)
			{
				int remaining = RAY_COUNT - i - 1;
				float coverUB = (blockedDirections + remaining) / RAY_COUNT_F;
				float fireUB  = Math.Min(fireValue + remaining, FIRE_LINE_IDEAL) / FIRE_LINE_IDEAL;
				float upper   = ((m_fWeightCover * coverUB) + (m_fWeightFireLine * fireUB) + cheapPart) / totalWeight + bonus;

				if (upper <= pruneAt)
					return -1;
			}
		}

		if (blockedDirections >= RAY_COUNT)
			return -1;

		if (m_bRequireFireLine && fireValue <= 0)
			return -1;

		float coverScore = blockedDirections / RAY_COUNT_F;
		float fireScore  = Math.Min(fireValue, FIRE_LINE_IDEAL) / FIRE_LINE_IDEAL;

		if (totalWeight <= 0)
			return coverScore + bonus;

		float score = (m_fWeightCover * coverScore) + (m_fWeightFireLine * fireScore) + cheapPart;

		return score / totalWeight + bonus;
	}

	//------------------------------------------------------------------------------------------------
	//! Snapshot karakter + pintu di area gedung, sekali per pencarian.
	protected void SnapshotOccupants()
	{
		m_aQueryCharacters.Clear();
		m_aQueryDoors.Clear();

		vector center = m_Building.CoordToParent((m_vLocalMins + m_vLocalMaxs) * 0.5);
		float  radius = 0.5 * vector.Distance(m_vLocalMins, m_vLocalMaxs) + Math.Max(m_fOccupancyRadius, m_fDoorBlockRadius);

		GetGame().GetWorld().QueryEntitiesBySphere(center, radius, QueryCallbackC);
	}

	//------------------------------------------------------------------------------------------------
	bool QueryCallbackC(IEntity e)
	{
		if (!e || e == m_OwnerEntity)
			return true;

		// Cast murah dulu -- FindComponent damage manager cuma buat karakter beneran.
		if (ChimeraCharacter.Cast(e))
		{
			SCR_CharacterDamageManagerComponent charComp = SCR_CharacterDamageManagerComponent.Cast(e.FindComponent(SCR_CharacterDamageManagerComponent));
			if (charComp && !charComp.IsDestroyed())
				m_aQueryCharacters.Insert(e);

			return true;
		}

		BaseDoorComponent doorComp = BaseDoorComponent.Cast(e.FindComponent(BaseDoorComponent));
		if (doorComp)
			m_aQueryDoors.Insert(e);

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Per kandidat: cuma cek jarak ke snapshot (posisi karakter dibaca live). Gak ada query.
	protected bool IsPositionOccupied(vector pos, out float nearestBooked)
	{
		nearestBooked = -1;

		float occupancyRadSq = m_fOccupancyRadius * m_fOccupancyRadius;
		foreach (IEntity e : m_aQueryCharacters)
		{
			if (e && vector.DistanceSq(e.GetOrigin(), pos) < occupancyRadSq)
				return true;
		}

		float doorRadSq = m_fDoorBlockRadius * m_fDoorBlockRadius;
		foreach (IEntity d : m_aQueryDoors)
		{
			if (d && vector.DistanceSq(d.GetOrigin(), pos) < doorRadSq)
				return true;
		}

		SCR_CoverManagerComponent coverMgr = SCR_CoverManagerComponent.GetInstance();
		if (coverMgr)
		{
			nearestBooked = coverMgr.GetNearestBookedDistanceXZ(pos);
			if (nearestBooked > 0 && nearestBooked < m_fMinBookedDistance)
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! Verifikasi akhir: query kecil live, cuma karakter (pintu gak pindah).
	protected bool IsOccupiedByCharacterNow(vector pos)
	{
		m_aQueryCharacters.Clear();
		m_aQueryDoors.Clear();

		GetGame().GetWorld().QueryEntitiesBySphere(pos, m_fOccupancyRadius, QueryCallbackC);

		float occupancyRadSq = m_fOccupancyRadius * m_fOccupancyRadius;
		foreach (IEntity e : m_aQueryCharacters)
		{
			if (e && vector.DistanceSq(e.GetOrigin(), pos) < occupancyRadSq)
				return true;
		}
		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected static ref TStringArray s_aVarsOut = {
		PORT_VECTOR_POS,
		PORT_VECTOR_BOOL
	};
	override TStringArray GetVariablesOut()
	{
		return s_aVarsOut;
	}

	//------------------------------------------------------------------------------------------------
	protected static ref TStringArray s_aVarsIn = {
		PORT_CENTER_OF_SEARCH,
		PORT_RADIUS
	};
	override TStringArray GetVariablesIn()
	{
		return s_aVarsIn;
	}

	//------------------------------------------------------------------------------------------------
	static override string GetOnHoverDescription()
	{
		return "Find Indoor Position (stratified sampling, scored: cover + fire line keluar gedung + spread + proximity).";
	}
};