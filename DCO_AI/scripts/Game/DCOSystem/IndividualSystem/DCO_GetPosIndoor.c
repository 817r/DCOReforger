// === REWRITE NOTES ===
// PERUBAHAN BESAR: dari "ambil posisi PERTAMA yang lolos filter" jadi "kumpulin
// kandidat, nilai pake skor, ambil yang TERBAIK". Versi lama gak punya konsep
// posisi bagus vs posisi jelek -- asal lolos 4 filter langsung dipake, jadi AI
// sering berdiri di tengah ruangan kosong tanpa arah tembak.
//
// Bug yang dibenerin:
// 1. BUG RUANG KOORDINAT di GetRandomPosInBounds(). localPos masih koordinat LOKAL
//    (dari GetBounds), tapi dilempar ke GetSurfaceY() yang minta koordinat DUNIA.
//    Buat gedung di (5000, 0, 3000), klem tanahnya nanya ketinggian di titik (0,0)
//    peta -- tempat yang sama sekali lain. Sekarang konversi ke dunia DULU, baru
//    diklem.
// 2. corridorWidth sebenernya ngukur TINGGI LANGIT-LANGIT, bukan lebar koridor --
//    kedua trace-nya vertikal (dir = End - Start = 8.45 * Up, murni ke atas).
//    Dan dua cek params.TraceEnt.GetRootParent() di dalemnya gak ngapa-ngapain:
//    TraceEnt masih hasil trace dari baris sebelumnya (belum ada trace baru setelah
//    params.End diubah) dan udah divalidasi sama persis beberapa baris di atas, jadi
//    satu selalu false satu selalu true. Diganti MeasureHeadroom() yang jujur
//    namanya dan jalan di SEMUA lantai, bukan cuma yang miring.
// 3. IsPositionOccupied nolak posisi kalau ada PINTU APAPUN dalam 3m. Di gedung
//    Reforger pintu ada di mana-mana -- kamar kecil bisa ketolak seluruhnya. Ini
//    kemungkinan besar alasan kenapa butuh sampe 1000 attempt. Sekarang pintu punya
//    radius sendiri yang jauh lebih kecil (cuma nyegah AI berdiri MENGHALANGI pintu),
//    kepisah dari radius occupancy karakter.
// 4. Baris 158 & 164 lama manggil GetRandomPosInBounds() dan nyimpen ke
//    m_vCurrentQueryPos, padahal nilainya ditimpa lagi di awal RandomQueryStep()
//    berikutnya -- dua pemanggilan sia-sia tiap attempt gagal. Dihapus.
// 5. "if (attempt > maxAttempt) { return; } return;" -- dua cabang yang dua-duanya
//    cuma return. Dihapus.
// 6. Null-check hilang: buildPosComp.GetBuildingEntity(), params.TraceEnt,
//    SCR_CoverManagerComponent.GetInstance(), dan m_Building yang dicache lintas
//    tick (gedung bisa hancur di tengah pencarian).
// 7. Satu attempt per tick BT x maxAttempt 1000 = AI bisa nyangkut di node ini
//    ratusan tick sebelum nyerah. Sekarang beberapa kandidat per tick (configurable)
//    dan attempt budget jauh lebih kecil karena filternya gak lagi over-reject.
// 8. Hasil lama di-cache selamanya selama searchPos gak pindah -- node yang
//    dijalankan ulang balikin posisi SAMA yang mungkin udah keisi orang lain.
//    Sekarang ada TTL.
//
// PORT SENGAJA GAK DIUBAH -- nama maupun urutannya di s_aVarsIn/s_aVarsOut persis
// sama kayak sebelumnya, jadi .bt yang udah ada gak perlu disentuh dan gak ada
// resiko port index geser.
// === END REWRITE NOTES ===

class DCO_FindIndoorPosition: AITaskScripted
{
	static const string PORT_CENTER_OF_SEARCH		= "OriginIn";
	static const string PORT_RADIUS					= "RadiusIn";
	static const string PORT_VECTOR_BOOL			= "Is Position Found";
	static const string PORT_VECTOR_POS				= "Position Found";

	[Attribute("0", UIWidgets.EditBox)]
	protected float m_fRadius;

	// === ADDED: budget pencarian ===
	[Attribute("6", UIWidgets.EditBox, "Berapa kandidat dievaluasi per tick BT. Naikin buat hasil lebih cepat, turunin kalau server kerasa berat.", category: "Search Budget")]
	protected int m_iCandidatesPerTick;

	[Attribute("150", UIWidgets.EditBox, "Total kandidat maksimum sebelum nyerah. Jauh lebih kecil dari 1000 lama karena filternya gak lagi over-reject.", category: "Search Budget")]
	protected int m_iMaxCandidates;

	[Attribute("20", UIWidgets.EditBox, "Maksimum tick nunggu navmesh tile ke-load sebelum nyerah. Nunggu tile gak dihitung sebagai attempt.", category: "Search Budget")]
	protected int m_iMaxTileWaitTicks;

	[Attribute("0.72", UIWidgets.Range, "Skor yang dianggap 'udah bagus banget' -- kandidat segini langsung diambil tanpa nyari lagi.\nCATATAN: skor MAKSIMUM teoretis cuma 0.869 (butuh tepat 5 dinding + 3 arah tembak + gak ada AI lain + persis di titik pusat), karena cover dan fireLine saling nekan -- mau 3 arah tembak berarti maksimal 5 dari 8 arah boleh ketutup, jadi coverScore mentok 0.625. Jangan set di atas ~0.80, jalan pintasnya gak akan pernah kepake dan pencarian selalu ngabisin budget penuh.", params: "0 1 0.01", category: "Search Budget")]
	protected float m_fGoodEnoughScore;

	[Attribute("15.0", UIWidgets.EditBox, "Berapa detik hasil pencarian dianggap masih valid sebelum dicari ulang.", category: "Search Budget")]
	protected float m_fResultTTL;
	// === END ADDED ===

	// === ADDED: filter keras (hard reject) ===
	[Attribute("1.6", UIWidgets.EditBox, "Tinggi langit-langit minimum (m). Nolak crawlspace / kolong. Ini pengganti 'corridorWidth' lama yang sebenernya emang ngukur ini.", category: "Hard Filter")]
	protected float m_fMinHeadroom;

	[Attribute("3.0", UIWidgets.EditBox, "Radius (m) ngecek karakter lain -- posisi ditolak kalau ada orang segini deket.", category: "Hard Filter")]
	protected float m_fOccupancyRadius;

	[Attribute("1.2", UIWidgets.EditBox, "Radius (m) ngecek pintu. KECIL -- tujuannya cuma nyegah AI berdiri MENGHALANGI pintu, bukan ngusir dia dari seluruh ruangan yang punya pintu (bug versi lama pake 3m).", category: "Hard Filter")]
	protected float m_fDoorBlockRadius;

	[Attribute("3.0", UIWidgets.EditBox, "Jarak minimum (m) ke posisi yang udah di-book AI lain.", category: "Hard Filter")]
	protected float m_fMinBookedDistance;

	[Attribute("1", UIWidgets.CheckBox, "Wajib punya minimal satu arah tembak keluar. Matiin kalau AI kebanyakan gagal dapet posisi di gedung padat.", category: "Hard Filter")]
	protected bool m_bRequireFireLine;
	// === END ADDED ===

	// === ADDED: bobot skor. Gak harus total 1 -- dinormalisasi otomatis. ===
	[Attribute("0.35", UIWidgets.Range, "Bobot: seberapa terlindungi posisinya (banyak dinding di sekitar).", params: "0 1 0.01", category: "Scoring")]
	protected float m_fWeightCover;

	[Attribute("0.35", UIWidgets.Range, "Bobot: seberapa bagus arah tembak keluarnya (jendela/pintu/ruang terbuka).", params: "0 1 0.01", category: "Scoring")]
	protected float m_fWeightFireLine;

	[Attribute("0.15", UIWidgets.Range, "Bobot: seberapa jauh dari posisi AI lain (biar gak numpuk di satu sudut).", params: "0 1 0.01", category: "Scoring")]
	protected float m_fWeightSpread;

	[Attribute("0.15", UIWidgets.Range, "Bobot: seberapa deket ke titik yang diminta BT.", params: "0 1 0.01", category: "Scoring")]
	protected float m_fWeightProximity;
	// === END ADDED ===

	//--------------------------------------------------------------------
	// Konstanta geometri penilaian
	protected static const int   RAY_COUNT        = 8;     //! Jumlah arah horizontal yang di-probe
	protected static const float RAY_COUNT_F      = 8.0;   //! Kembaran float-nya -- hindari integer division pas ngitung skor
	protected static const float FIRE_LINE_IDEAL  = 3.0;   //! Jumlah arah tembak yang udah dianggap cukup
	protected static const float RAY_MAX_DIST     = 15.0;  //! Panjang maksimum probe
	protected static const float WALL_NEAR_DIST   = 2.5;   //! Kena sesuatu di bawah ini = dianggap "ada dinding/cover"
	protected static const float FIRE_LINE_DIST   = 8.0;   //! Bersih sejauh ini = dianggap arah tembak valid
	protected static const float CHEST_HEIGHT     = 1.0;   //! Tinggi probe horizontal (setinggi dada, bukan mata)
	protected static const float EYE_POS          = 1.55;
	protected static const float HEADROOM_PROBE   = 6.0;   //! Panjang probe vertikal ke atas
	protected static const float SPREAD_IDEAL_DIST = 8.0;  //! Jarak ke AI lain yang dianggap "udah cukup nyebar"
	protected static const float MIN_BUILDING_HALF_WIDTH = 4.0;

	//--------------------------------------------------------------------
	protected IEntity m_Building;
	protected vector  m_vLastSearchPos = vector.Zero;
	protected vector  m_vLocalMins, m_vLocalMaxs;

	protected ref array<IEntity> m_aQueryFoundBuilding = {};
	protected ref array<IEntity> m_aQueryCharacters    = {};
	protected ref array<IEntity> m_aQueryDoors         = {};

	protected NavmeshWorldComponent m_pNavmesh;

	protected int    m_iAttempt;
	protected int    m_iTileWaitTicks;
	protected bool   m_bSearchDone;
	protected vector m_vBestPos;
	protected float  m_fBestScore;
	protected float  m_fResultTime_ms;

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

		bool needNewSearch = !m_Building
			|| vector.DistanceSq(searchPos, m_vLastSearchPos) > 1.0
			|| (m_bSearchDone && (now_ms - m_fResultTime_ms) > (m_fResultTTL * 1000.0));

		if (needNewSearch)
		{
			ResetSearch(searchPos);

			if (!AcquireBuilding(searchPos, searchRad))
			{
				SetVariableOut(PORT_VECTOR_BOOL, false);
				return ENodeResult.FAIL;
			}
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
	protected void ResetSearch(vector searchPos)
	{
		m_vLastSearchPos = searchPos;
		m_bSearchDone    = false;
		m_iAttempt       = 0;
		m_iTileWaitTicks = 0;
		m_fBestScore     = 0;
		m_vBestPos       = vector.Zero;
		m_Building       = null;
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

	protected void RunSearchBatch(vector searchPos, float searchRad)
	{
		if (!m_pNavmesh)
			m_pNavmesh = GetGame().GetAIWorld().GetNavmeshWorldComponent("Soldiers");

		if (!m_pNavmesh)
		{
			m_bSearchDone = true;
			return;
		}

		for (int i = 0; i < m_iCandidatesPerTick; i++)
		{
			if (m_iAttempt >= m_iMaxCandidates)
			{
				m_bSearchDone    = true;
				m_fResultTime_ms = GetGame().GetWorld().GetWorldTime();
				return;
			}

			vector queryPos = GetRandomPosInBounds();
			vector validPos;

			DCO_BuildingPosCreation result = ValidateCandidate(queryPos, validPos);

			if (result == DCO_BuildingPosCreation.RUNNING)
			{
				// Navmesh tile lagi di-load. Berhenti batch ini, coba lagi tick depan.
				m_iTileWaitTicks++;

				if (m_iTileWaitTicks > m_iMaxTileWaitTicks)
				{
					m_bSearchDone    = true;
					m_fResultTime_ms = GetGame().GetWorld().GetWorldTime();
				}

				return;
			}

			m_iAttempt++;

			if (result != DCO_BuildingPosCreation.SUCCESS)
				continue;

			float score = ScoreCandidate(validPos, searchPos, searchRad);
			if (score < 0)
				continue;

			if (score > m_fBestScore)
			{
				m_fBestScore = score;
				m_vBestPos   = validPos;
			}

			// Udah bagus banget -- gak usah nyari lagi.
			if (m_fBestScore >= m_fGoodEnoughScore)
			{
				m_bSearchDone    = true;
				m_fResultTime_ms = GetGame().GetWorld().GetWorldTime();
				return;
			}
		}
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

	protected vector GetRandomPosInBounds()
	{
		vector localPos;
		for (int i = 0; i < 3; i++)
		{
			localPos[i] = Math.RandomFloatInclusive(m_vLocalMins[i], m_vLocalMaxs[i]);
		}

		vector worldPos = m_Building.CoordToParent(localPos);

		float groundHeight = GetGame().GetWorld().GetSurfaceY(worldPos[0], worldPos[2]);
		if (worldPos[1] < groundHeight)
			worldPos[1] = groundHeight;

		return worldPos;
	}

	protected DCO_BuildingPosCreation ValidateCandidate(vector queryPos, out vector outPos)
	{
		if (m_pNavmesh.IsTileRequested(queryPos))
			return DCO_BuildingPosCreation.RUNNING;

		if (!m_pNavmesh.IsTileLoaded(queryPos))
		{
			m_pNavmesh.LoadTileIn(queryPos);
			return DCO_BuildingPosCreation.RUNNING;
		}

		if (!m_pNavmesh.GetReachablePoint(queryPos, 2, outPos))
			return DCO_BuildingPosCreation.FAIL;

		if (GetGame().GetWorld().GetSurfaceY(outPos[0], outPos[2]) < 0)
			return DCO_BuildingPosCreation.FAIL;

		TraceParam floorTrace = new TraceParam();
		floorTrace.Flags = TraceFlags.ENTS;
		floorTrace.Start = outPos + EYE_POS * vector.Up;
		floorTrace.End   = outPos - 5 * vector.Up;

		if (GetGame().GetWorld().TraceMove(floorTrace, null) >= 0.9)
			return DCO_BuildingPosCreation.FAIL;

		if (!floorTrace.TraceEnt)
			return DCO_BuildingPosCreation.FAIL;

		if (floorTrace.TraceEnt.GetRootParent() != m_Building.GetRootParent())
			return DCO_BuildingPosCreation.FAIL;

		if (MeasureHeadroom(outPos) < m_fMinHeadroom)
			return DCO_BuildingPosCreation.FAIL;

		if (IsPositionOccupied(outPos))
			return DCO_BuildingPosCreation.FAIL;

		return DCO_BuildingPosCreation.SUCCESS;
	}

	protected float MeasureHeadroom(vector floorPos)
	{
		TraceParam up = new TraceParam();
		up.Flags = TraceFlags.ENTS;
		up.Start = floorPos + 0.1 * vector.Up;
		up.End   = floorPos + HEADROOM_PROBE * vector.Up;

		float frac = GetGame().GetWorld().TraceMove(up, null);
		return frac * HEADROOM_PROBE;
	}

	protected float ScoreCandidate(vector pos, vector searchPos, float searchRad)
	{
		vector probeOrigin = pos + CHEST_HEIGHT * vector.Up;

		int blockedDirections = 0;
		int fireLines         = 0;

		for (int i = 0; i < RAY_COUNT; i++)
		{
			float angleRad = (i * 360.0 / RAY_COUNT) * Math.DEG2RAD;
			vector dir = Vector(Math.Cos(angleRad), 0, Math.Sin(angleRad));

			TraceParam ray = new TraceParam();
			ray.Flags = TraceFlags.ENTS;
			ray.Start = probeOrigin;
			ray.End   = probeOrigin + dir * RAY_MAX_DIST;

			float hitDist = GetGame().GetWorld().TraceMove(ray, null) * RAY_MAX_DIST;

			if (hitDist < WALL_NEAR_DIST)
				blockedDirections++;
			else if (hitDist >= FIRE_LINE_DIST)
				fireLines++;
		}

		if (blockedDirections >= RAY_COUNT)
			return -1;

		if (m_bRequireFireLine && fireLines == 0)
			return -1;

		float coverScore = blockedDirections / RAY_COUNT_F;

		float fireScore = Math.Min(fireLines, FIRE_LINE_IDEAL) / FIRE_LINE_IDEAL;

		float spreadScore = 1.0;
		SCR_CoverManagerComponent coverMgr = SCR_CoverManagerComponent.GetInstance();
		if (coverMgr)
		{
			float nearestBooked = coverMgr.GetNearestBookedDistanceXZ(pos);
			if (nearestBooked > 0)
				spreadScore = Math.Clamp(nearestBooked / SPREAD_IDEAL_DIST, 0.0, 1.0);
		}

		float proximityScore = 1.0;
		if (searchRad > 0)
			proximityScore = 1.0 - Math.Clamp(vector.Distance(pos, searchPos) / searchRad, 0.0, 1.0);

		float totalWeight = m_fWeightCover + m_fWeightFireLine + m_fWeightSpread + m_fWeightProximity;
		if (totalWeight <= 0)
			return coverScore;

		float score = (m_fWeightCover     * coverScore)
		            + (m_fWeightFireLine  * fireScore)
		            + (m_fWeightSpread    * spreadScore)
		            + (m_fWeightProximity * proximityScore);

		return score / totalWeight;
	}

	//------------------------------------------------------------------------------------------------
	bool QueryCallbackC(IEntity e)
	{
		if (!e)
			return true;

		SCR_CharacterDamageManagerComponent charComp = SCR_CharacterDamageManagerComponent.Cast(e.FindComponent(SCR_CharacterDamageManagerComponent));
		if (charComp && !charComp.IsDestroyed())
		{
			m_aQueryCharacters.Insert(e);
			return true;
		}

		BaseDoorComponent doorComp = BaseDoorComponent.Cast(e.FindComponent(BaseDoorComponent));
		if (doorComp)
			m_aQueryDoors.Insert(e);

		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsPositionOccupied(vector pos)
	{
		m_aQueryCharacters.Clear();
		m_aQueryDoors.Clear();

		float queryRadius = Math.Max(m_fOccupancyRadius, m_fDoorBlockRadius);
		GetGame().GetWorld().QueryEntitiesBySphere(pos, queryRadius, QueryCallbackC);

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
			float bookedDist = coverMgr.GetNearestBookedDistanceXZ(pos);
			if (bookedDist > 0 && bookedDist < m_fMinBookedDistance)
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
		return "Find Indoor Position (scored: cover + fire line + spread).";
	}
};