class DCO_FindIndoorPosition: AITaskScripted
{
	static const string PORT_CENTER_OF_SEARCH	= "OriginIn";
	static const string PORT_RADIUS				= "RadiusIn";
	static const string PORT_VECTOR_BOOL		= "Is Position Found";

	[Attribute("0", UIWidgets.EditBox)]
	protected float m_fRadius;

	[Attribute("6", UIWidgets.EditBox, "Candidates evaluated per BT tick. Raise for faster results, lower if the server feels heavy.", category: "Search Budget")]
	protected int m_iCandidatesPerTick;

	[Attribute("150", UIWidgets.EditBox, "Maximum total candidates before giving up. The search also stops once every cell of the building has been tried.", category: "Search Budget")]
	protected int m_iMaxCandidates;

	[Attribute("20", UIWidgets.EditBox, "Maximum ticks to wait for a navmesh tile to load before giving up. Waiting for a tile does not count as an attempt.", category: "Search Budget")]
	protected int m_iMaxTileWaitTicks;

	[Attribute("0.72", UIWidgets.Range, "Score treated as good enough: a candidate at this score is taken at once without searching further.\nNOTE: the theoretical maximum score is only about 0.869 (cover and fire line pull against each other). Do not set above about 0.80 or the shortcut will never be used.", params: "0 1 0.01", category: "Search Budget")]
	protected float m_fGoodEnoughScore;

	[Attribute("15.0", UIWidgets.EditBox, "How long (s) a search result stays valid before searching again.", category: "Search Budget")]
	protected float m_fResultTTL;

	[Attribute("2.5", UIWidgets.EditBox, "Horizontal cell size (m) for stratified sampling. Smaller = more precise but more candidates.", category: "Sampling")]
	protected float m_fCellSize;

	[Attribute("3.0", UIWidgets.EditBox, "Cell height (m), roughly the height of one floor.", category: "Sampling")]
	protected float m_fCellHeight;

	[Attribute("1.0", UIWidgets.EditBox, "Candidates that snap (navmesh) this close to an already evaluated candidate are discarded without scoring.", category: "Sampling")]
	protected float m_fDedupeRadius;

	[Attribute("3", UIWidgets.EditBox, "Number of best candidates kept for final verification (if #1 is occupied, take #2, and so on).", category: "Sampling")]
	protected int m_iTopCandidates;

	[Attribute("1.6", UIWidgets.EditBox, "Minimum ceiling height (m). Rejects crawlspaces.", category: "Hard Filter")]
	protected float m_fMinHeadroom;

	[Attribute("3.0", UIWidgets.EditBox, "Radius (m) to check for other characters. A position is rejected if someone is this close. This AI itself is not counted.", category: "Hard Filter")]
	protected float m_fOccupancyRadius;

	[Attribute("1.2", UIWidgets.EditBox, "Radius (m) to check for doors. Keep SMALL: it only stops AI from standing in a doorway.", category: "Hard Filter")]
	protected float m_fDoorBlockRadius;

	[Attribute("2.5", UIWidgets.EditBox, "Radius (m) of the door avoid zone. Inside it the score is reduced, more so closer to the door. Outside the hard filter m_fDoorBlockRadius.", category: "Door Avoidance")]
	protected float m_fDoorAvoidRadius;

	[Attribute("0.3", UIWidgets.Range, "Maximum score reduction (right at the door). Falls linearly to 0 at m_fDoorAvoidRadius.", params: "0 1 0.05", category: "Door Avoidance")]
	protected float m_fDoorAvoidPenalty;

	[Attribute("1", UIWidgets.CheckBox, "Reject positions in narrow gaps (door frames without a door, curved corridors). Uses existing rays, no extra traces.", category: "Door Avoidance")]
	protected bool m_bRejectChokepoints;

	[Attribute("1.4", UIWidgets.EditBox, "Width (m) below which a spot counts as a narrow gap: two opposite rays both hit a wall within this total.", category: "Door Avoidance")]
	protected float m_fChokepointWidth;

	[Attribute("3.0", UIWidgets.EditBox, "Minimum distance (m) to positions booked by other AI.", category: "Hard Filter")]
	protected float m_fMinBookedDistance;

	[Attribute("1", UIWidgets.CheckBox, "Require at least one fire direction (out of the building or a long interior line). Turn off if AI often fail to find a position in dense buildings.", category: "Hard Filter")]
	protected bool m_bRequireFireLine;

	[Attribute("0.35", UIWidgets.Range, "Weight: how protected the position is (walls around it).", params: "0 1 0.01", category: "Scoring")]
	protected float m_fWeightCover;

	[Attribute("0.35", UIWidgets.Range, "Weight: how good the fire direction is.", params: "0 1 0.01", category: "Scoring")]
	protected float m_fWeightFireLine;

	[Attribute("0.15", UIWidgets.Range, "Weight: how far from other AI (so they do not pile up in one corner).", params: "0 1 0.01", category: "Scoring")]
	protected float m_fWeightSpread;

	[Attribute("0.15", UIWidgets.Range, "Weight: how close to the point requested by the BT.", params: "0 1 0.01", category: "Scoring")]
	protected float m_fWeightProximity;

	[Attribute("0.5", UIWidgets.Range, "Value of a fire line that only sweeps the interior (corridor or hall), compared with one that leaves the building through a window or door (= 1.0).", params: "0 1 0.05", category: "Scoring")]
	protected float m_fInteriorLineValue;

	[Attribute("0.05", UIWidgets.Range, "Score bonus for the position this AI has already booked. Stops AI moving every TTL when the new position is only slightly better.", params: "0 0.3 0.01", category: "Scoring")]
	protected float m_fStickinessBonus;


	protected IEntity m_Building;
	protected IEntity m_BuildingKey;
	protected AIGroup m_MyGroup;

	protected int m_iCoverPref;
	protected bool m_bOutdoorDecided;
	protected bool m_bOutdoorWins;
	protected static ref array<string> s_aFortKeywords;
	protected static array<string> GetFortKeywords()
	{
		if (!s_aFortKeywords)
			s_aFortKeywords = {"sandbag", "trench", "bunker", "fortif", "hesco", "foxhole", "dugout", "revetment"};
		return s_aFortKeywords;
	}
	protected ref array<IEntity> m_aFortQuery = {};
	protected IEntity m_BestFort;

	protected bool FindFortPosition(vector center, out vector outPos)
	{
		if (!m_pNavmesh)
			m_pNavmesh = GetGame().GetAIWorld().GetNavmeshWorldComponent("Soldiers");
		if (!m_pNavmesh)
			return false;

		vector fort = m_BestFort.GetOrigin();
		vector back = center - fort;
		back[1] = 0;
		if (back.LengthSq() < 1)
			back = Vector(Math.RandomFloat(-1, 1), 0, Math.RandomFloat(-1, 1));
		back.Normalize();
		vector side = Vector(-back[2], 0, back[0]);

		SCR_CoverManagerComponent coverMgr = SCR_CoverManagerComponent.GetInstance();
		array<float> offsets = {0, 2, -2, 4, -4};
		foreach (float o : offsets)
		{
			vector p = fort + back * 1.2 + side * o;
			vector snapped;
			if (!m_pNavmesh.IsTileLoaded(p) || !m_pNavmesh.GetReachablePoint(p, 2, snapped))
				continue;
			if (coverMgr)
			{
				float booked = coverMgr.GetNearestBookedDistanceXZ(snapped);
				if (booked > 0 && booked < m_fMinBookedDistance)
					continue;
			}
			if (IsOccupiedByCharacterNow(snapped))
				continue;
			outPos = snapped;
			return true;
		}
		return false;
	}

	protected int GetCoverPreference(AIAgent owner)
	{
		AIGroup grp = owner.GetParentGroup();
		if (grp)
		{
			DCO_GroupConfigComponent cfg = DCO_GroupConfigComponent.Cast(grp.FindComponent(DCO_GroupConfigComponent));
			if (cfg)
				return cfg.GetCoverPreference();
		}
		DCO_GlobalAIComponent global = DCO_GlobalAIComponent.GetFor(owner);
		if (global)
			return global.GetCoverPreference();
		return 0;
	}

	protected static ref map<int, ref array<IEntity>> s_mFortCells;
	protected static ref map<int, float> s_mFortCellAt;

	protected float OutdoorScore(vector center, float radius)
	{
		float r = Math.Min(radius, 60.0);
		if (!s_mFortCells)
		{
			s_mFortCells = new map<int, ref array<IEntity>>();
			s_mFortCellAt = new map<int, float>();
		}
		float now = GetGame().GetWorld().GetWorldTime();
		int cx = Math.Floor(center[0] / 16.0);
		int cy = Math.Floor(center[1] / 8.0);
		int cz = Math.Floor(center[2] / 16.0);
		int key = (cx * 1024 + cz) * 256 + cy;
		array<IEntity> cell = s_mFortCells.Get(key);
		float cellAt;
		if (!cell || !s_mFortCellAt.Find(key, cellAt) || now - cellAt > 60000.0)
		{
			if (s_mFortCells.Count() > 2048)
			{
				s_mFortCells.Clear();
				s_mFortCellAt.Clear();
			}
			m_aFortQuery.Clear();
			DCO_Perf.Count("q:DCO_FindIndoorPosition");
			vector cc = Vector((cx + 0.5) * 16.0, (cy + 0.5) * 8.0, (cz + 0.5) * 16.0);
			GetGame().GetWorld().QueryEntitiesBySphere(cc, 74.0, FortQueryCallback, null, EQueryEntitiesFlags.STATIC);
			cell = new array<IEntity>();
			cell.Copy(m_aFortQuery);
			s_mFortCells.Set(key, cell);
			s_mFortCellAt.Set(key, now);
		}
		else
			DCO_Perf.Count("indoor:fort_cell_hit");

		float best = 0.45;
		m_BestFort = null;
		float r2 = r * r;
		foreach (IEntity e : cell)
		{
			if (!e)
				continue;
			vector fmins, fmaxs;
			e.GetWorldBounds(fmins, fmaxs);
			float ox = Math.Max(Math.Max(fmins[0] - center[0], 0.0), center[0] - fmaxs[0]);
			float oy = Math.Max(Math.Max(fmins[1] - center[1], 0.0), center[1] - fmaxs[1]);
			float oz = Math.Max(Math.Max(fmins[2] - center[2], 0.0), center[2] - fmaxs[2]);
			if (ox * ox + oy * oy + oz * oz > r2)
				continue;
			float d = vector.Distance(e.GetOrigin(), center);
			float sc = 0.8 + 0.1 * (1 - d / Math.Max(r, 1));
			if (sc > best)
			{
				best = sc;
				m_BestFort = e;
			}
		}
		m_aFortQuery.Clear();
		return best;
	}

	protected bool FortQueryCallback(IEntity e)
	{
		if (!e || e.GetParent() || BaseTree.Cast(e))
			return true;
		string prefab = SCR_ResourceNameUtils.GetPrefabName(e);
		if (prefab.IsEmpty())
			return true;
		prefab.ToLower();
		foreach (string k : GetFortKeywords())
		{
			if (prefab.Contains(k))
			{
				m_aFortQuery.Insert(e);
				return true;
			}
		}
		return true;
	}
	protected IEntity m_OwnerEntity;
	protected vector  m_vLastSearchPos = vector.Zero;
	protected vector  m_vLocalMins, m_vLocalMaxs;

	protected ref array<IEntity> m_aQueryFoundBuilding = {};
	protected ref array<IEntity> m_aQueryCharacters    = {};
	protected ref array<IEntity> m_aQueryDoors         = {};
	protected ref array<vector>  m_aDoorPoints         = {};
	protected ref array<float>   m_aRayHitDist         = {};

	protected ref array<int>    m_aCells     = {};
	protected ref array<vector> m_aEvaluated = {};
	protected ref array<vector> m_aTopPos    = {};
	protected ref array<float>  m_aTopScore  = {};


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

	protected DCO_GarrisonBuilding m_GB;
	protected bool   m_bFromCache;
	protected int    m_iCacheAdded;
	protected float  m_fLastCover;
	protected float  m_fLastFire;
	protected float  m_fLastMinExit = -1;
	protected vector m_vLastFireDir;
	protected ref array<DCO_GarrisonSlot> m_aTopSlot = {};

	protected bool DCO_UseSlotCache()
	{
		return true;
	}

	protected void PickFromCache(vector searchPos, float searchRad)
	{
		if (!m_pNavmesh)
			m_pNavmesh = GetGame().GetAIWorld().GetNavmeshWorldComponent("Soldiers");

		m_aTopPos.Clear();
		m_aTopScore.Clear();
		m_aTopSlot.Clear();
		array<float> mults = {};
		array<float> ns = {};

		float totalWeight = m_fWeightCover + m_fWeightFireLine + m_fWeightSpread + m_fWeightProximity;
		if (totalWeight <= 0)
			totalWeight = 1;
		vector threat;
		bool hasThreat = DCO_BldLearn.ThreatOf(m_OwnerEntity, threat);
		string use = DCO_BldLearn.UseFor(m_OwnerEntity);
		float radSq = searchRad * searchRad;
		int cands = 0;

		array<DCO_GarrisonSlot> preSlots = {};
		array<float> preScores = {};
		array<DCO_GarrisonSlot> allSlots = {};
		foreach (DCO_GarrisonSlot ps : m_GB.m_aSlots)
		{
			if (!ps || ps.m_bUnreachable || ps.m_eType == DCO_EGarrisonSlotType.ROOF)
				continue;
			if (searchRad > 0 && vector.DistanceSq(ps.m_vWorldPos, searchPos) > radSq)
				continue;
			if (m_bRequireFireLine && ps.m_fFire <= 0)
				continue;
			allSlots.Insert(ps);
			float pprox = 1.0;
			if (searchRad > 0)
				pprox = 1.0 - Math.Clamp(vector.Distance(ps.m_vWorldPos, searchPos) / searchRad, 0.0, 1.0);
			float pre = (m_fWeightCover * ps.m_fCover + m_fWeightFireLine * ps.m_fFire + m_fWeightSpread + m_fWeightProximity * pprox) / totalWeight;
			if (m_vPrevBookedPos != vector.Zero && vector.DistanceSq(ps.m_vWorldPos, m_vPrevBookedPos) < 1.0)
				pre += m_fStickinessBonus;
			int pat = 0;
			while (pat < preScores.Count() && preScores[pat] >= pre)
				pat++;
			if (pat >= 24)
				continue;
			preScores.InsertAt(pre, pat);
			preSlots.InsertAt(ps, pat);
			if (preScores.Count() > 24)
			{
				preScores.Remove(24);
				preSlots.Remove(24);
			}
		}
		bool preCut = allSlots.Count() > preSlots.Count();
		if (preCut)
			DCO_Perf.Count("indoor:pre_cut");

		for (int pass = 0; pass < 2; pass++)
		{
			array<DCO_GarrisonSlot> passSlots = preSlots;
			if (pass == 1)
			{
				if (!preCut || !m_aTopSlot.IsEmpty())
					break;
				DCO_Perf.Count("indoor:pre_retry_all");
				passSlots = allSlots;
			}
			foreach (DCO_GarrisonSlot s : passSlots)
			{
				if (pass == 1 && preSlots.Contains(s))
					continue;
				float nearestBooked = -1;
				if (IsPositionOccupied(s.m_vWorldPos, nearestBooked))
					continue;

				cands++;
				float spread = 1.0;
				if (nearestBooked > 0)
					spread = Math.Clamp(nearestBooked / 8.0, 0.0, 1.0);
				float prox = 1.0;
				if (searchRad > 0)
					prox = 1.0 - Math.Clamp(vector.Distance(s.m_vWorldPos, searchPos) / searchRad, 0.0, 1.0);

				float score = (m_fWeightCover * s.m_fCover + m_fWeightFireLine * s.m_fFire + m_fWeightSpread * spread + m_fWeightProximity * prox) / totalWeight;
				if (m_vPrevBookedPos != vector.Zero && vector.DistanceSq(s.m_vWorldPos, m_vPrevBookedPos) < 1.0)
					score += m_fStickinessBonus;
				score = ApplyDoorPenalty(score, s.m_vWorldPos);
				float n = 0;
				float mult = DCO_BldLearn.SlotMult(m_GB, s, threat, hasThreat, use, n);
				score *= mult;

				int at = 0;
				while (at < m_aTopScore.Count() && m_aTopScore[at] >= score)
					at++;
				if (at >= 5)
					continue;
				m_aTopScore.InsertAt(score, at);
				m_aTopPos.InsertAt(s.m_vWorldPos, at);
				m_aTopSlot.InsertAt(s, at);
				mults.InsertAt(mult, at);
				ns.InsertAt(n, at);
				if (m_aTopScore.Count() > 5)
				{
					m_aTopScore.Remove(5);
					m_aTopPos.Remove(5);
					m_aTopSlot.Remove(5);
					mults.Remove(5);
					ns.Remove(5);
				}
			}
		}

		if (hasThreat)
		{
			for (int i = 0; i < m_aTopPos.Count(); i++)
			{
				vector eye = m_aTopPos[i] + 1.0 * vector.Up;
				vector to = threat + 1.0 * vector.Up - eye;
				float len = to.Length();
				if (len < 1.0)
					continue;
				float reach = Math.Min(len, 30.0);
				float frac = TraceFraction(eye, eye + to * (reach / len));
				float adj = -0.05;
				if (use == "cover")
				{
					if (frac * reach < 2.0)
						adj = 0.05;
				}
				else if (frac >= 0.99 || frac * reach >= 8.0)
					adj = 0.05;
				m_aTopScore[i] = Math.Max(m_aTopScore[i] + adj, 0.01);
			}

			for (int a = 1; a < m_aTopScore.Count(); a++)
			{
				for (int b = a; b > 0 && m_aTopScore[b] > m_aTopScore[b - 1]; b--)
				{
					m_aTopScore.SwapItems(b, b - 1);
					m_aTopPos.SwapItems(b, b - 1);
					m_aTopSlot.SwapItems(b, b - 1);
					mults.SwapItems(b, b - 1);
					ns.SwapItems(b, b - 1);
				}
			}
		}

		bool explore = m_aTopPos.Count() > 1 && DCO_BldLearn.Explore();
		if (explore)
		{
			int k = Math.RandomInt(1, m_aTopPos.Count());
			m_aTopScore.SwapItems(0, k);
			m_aTopPos.SwapItems(0, k);
			m_aTopSlot.SwapItems(0, k);
			mults.SwapItems(0, k);
			ns.SwapItems(0, k);
			DCO_Perf.Count("bldcache:explore");
		}

		DCO_Perf.Count("bldcache:pick_cache");
		if (!m_aTopSlot.IsEmpty() && DCO_Log.On(DCO_ELogChannel.UNIT, DCO_ELogLevel.DEBUG))
		{
			string unit = DCO_Log.IdUnit(m_OwnerEntity);
			DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.DEBUG, unit, string.Format("bld_pick unit=%1 prefab=%2 slot=%3 use=%4 src=cache cands=%5 score=%6 mult=%7 n=%8 explore=%9",
				unit, DCO_BldCache.Name(m_GB.m_sPrefab), DCO_BldLearn.SlotKey(m_aTopSlot[0]), use, cands, m_aTopScore[0].ToString(-1, 2), mults[0].ToString(-1, 2), ns[0].ToString(-1, 1), DCO_Util.B01(explore))
				+ string.Format(" threat=%1", DCO_Util.B01(hasThreat)));
		}

		FinalizeResult();
	}

	protected bool DCO_CachedReachable(int i)
	{
		if (!m_bFromCache || !m_aTopSlot.IsIndexValid(i) || !m_pNavmesh || !m_GB)
			return true;
		vector p = m_aTopPos[i];
		if (!m_pNavmesh.IsTileLoaded(p))
			return true;
		vector snapped;
		if (m_pNavmesh.GetReachablePoint(p, 0.6, snapped))
			return true;
		DCO_GarrisonRegistry reg = DCO_GarrisonRegistry.GetInstance();
		int idx = m_GB.m_aSlots.Find(m_aTopSlot[i]);
		if (reg && idx >= 0)
			reg.MarkUnreachable(m_GB, idx);
		return false;
	}

	static override bool VisibleInPalette() { return true; }

	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		int pt = DCO_Perf.Begin();
		ENodeResult r = DCO_SimulateFindIndoor(owner, dt);
		DCO_Perf.End("bt_find_indoor", pt);
		return r;
	}

	protected ENodeResult DCO_SimulateFindIndoor(AIAgent owner, float dt)
	{
		if (!owner)
			return ENodeResult.FAIL;
		string ctrlLim;
		if (DCO_CtrlLock.Ask(owner, DCO_ECtrlReq.RELOC, ctrlLim) != DCO_ECtrlAns.ALLOW)
			return ENodeResult.FAIL;

		vector searchPos;
		float  searchRad;

		GetVariableIn(PORT_CENTER_OF_SEARCH, searchPos);
		if (!GetVariableIn(PORT_RADIUS, searchRad))
			searchRad = m_fRadius;

		if (searchPos == vector.Zero)
			return ENodeResult.FAIL;

		float now_ms = GetGame().GetWorld().GetWorldTime();

		if (!m_Building && m_fAcquireFailTime_ms > 0
			&& vector.DistanceSq(searchPos, m_vAcquireFailPos) <= 1.0
			&& (now_ms - m_fAcquireFailTime_ms) < 2000.0)
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

			m_iCoverPref = GetCoverPreference(owner);
			m_bOutdoorDecided = false;
			if (m_iCoverPref == 2)
			{
				SetVariableOut(PORT_VECTOR_BOOL, false);
				return ENodeResult.FAIL;
			}

			if (!AcquireBuilding(searchPos, searchRad))
			{
				m_vAcquireFailPos     = searchPos;
				m_fAcquireFailTime_ms = now_ms;
				SetVariableOut(PORT_VECTOR_BOOL, false);
				return ENodeResult.FAIL;
			}

			m_fAcquireFailTime_ms = 0;
			int acqT = DCO_Perf.Begin();
			PrepareSearch(searchPos, searchRad);
			DCO_Perf.End("bt_find_indoor_prepare", acqT);
			DCO_Perf.Count("indoor:phase_split");
			return ENodeResult.RUNNING;
		}

		if (!m_Building)
		{
			SetVariableOut(PORT_VECTOR_BOOL, false);
			return ENodeResult.FAIL;
		}

		bool ranBatch = false;
		if (!m_bSearchDone)
		{
			RunSearchBatch(searchPos, searchRad);
			ranBatch = true;

			if (!m_bSearchDone)
				return ENodeResult.RUNNING;
		}

		if (m_fBestScore <= 0)
		{
			SetVariableOut(PORT_VECTOR_BOOL, false);
			return ENodeResult.FAIL;
		}

		if (m_iCoverPref == 0 && !m_bOutdoorDecided && ranBatch && m_fBestScore > 0)
		{
			DCO_Perf.Count("indoor:phase_split");
			return ENodeResult.RUNNING;
		}
		if (m_iCoverPref == 0 && !m_bOutdoorDecided)
		{
			m_bOutdoorDecided = true;
			int osT = DCO_Perf.Begin();
			m_bOutdoorWins = OutdoorScore(searchPos, searchRad) > m_fBestScore;
			DCO_Perf.End("bt_find_indoor_outdoor", osT);
		}
		if (m_iCoverPref == 0 && m_bOutdoorWins)
		{
			vector fortPos;
			if (!m_BestFort || !FindFortPosition(searchPos, fortPos))
			{
				SetVariableOut(PORT_VECTOR_BOOL, false);
				return ENodeResult.FAIL;
			}
			SCR_CoverManagerComponent fortCoverMgr = SCR_CoverManagerComponent.GetInstance();
			if (fortCoverMgr)
				fortCoverMgr.RegisterPosition(owner.GetControlledEntity(), fortPos);
			SetVariableOut("Position Found", fortPos);
			SetVariableOut(PORT_VECTOR_BOOL, true);
			return ENodeResult.SUCCESS;
		}

		SCR_CoverManagerComponent coverMgr = SCR_CoverManagerComponent.GetInstance();
		if (coverMgr)
			coverMgr.RegisterPosition(owner.GetControlledEntity(), m_vBestPos);

		SetVariableOut("Position Found", m_vBestPos);
		SetVariableOut(PORT_VECTOR_BOOL, true);
		return ENodeResult.SUCCESS;
	}

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
		m_aTopSlot.Clear();
		m_GB             = null;
		m_bFromCache     = false;
		m_iCacheAdded    = 0;
	}

	protected bool AcquireBuilding(vector searchPos, float searchRad)
	{
		m_aQueryFoundBuilding.Clear();
		DCO_GarrisonRegistry acqReg = DCO_GarrisonRegistry.GetInstance();
		if (acqReg && searchRad <= 112.0)
		{
			DCO_Perf.Count("indoor:acquire_registry");
			array<IEntity> regFound = {};
			acqReg.FindBuildings(searchPos, searchRad, regFound);
			foreach (IEntity rf : regFound)
				QueryCallback(rf);
		}
		else
		{
			DCO_Perf.Count("q:DCO_FindIndoorPosition");
			GetGame().GetWorld().QueryEntitiesBySphere(searchPos, searchRad, QueryCallback);
		}

		IEntity nearestEntity = null;
		float smallestDistSq = float.MAX;

		AIGroup myGroup;
		ChimeraCharacter ownerChar = ChimeraCharacter.Cast(m_OwnerEntity);
		if (ownerChar)
		{
			AIControlComponent control = AIControlComponent.Cast(ownerChar.FindComponent(AIControlComponent));
			if (control && control.GetAIAgent())
				myGroup = control.GetAIAgent().GetParentGroup();
		}

		foreach (IEntity e : m_aQueryFoundBuilding)
		{
			if (!e)
				continue;

			if (DCO_GarrisonRegistry.IsBuildingUsedByOther(e, myGroup))
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

		m_BuildingKey = nearestEntity;
		m_MyGroup = myGroup;

		DCO_BuildingPositionComponent buildPosComp = DCO_BuildingPositionComponent.Cast(nearestEntity.FindComponent(DCO_BuildingPositionComponent));
		if (!buildPosComp)
			return false;

		m_Building = buildPosComp.GetBuildingEntity();
		if (!m_Building)
			return false;

		m_Building.GetBounds(m_vLocalMins, m_vLocalMaxs);
		return true;
	}

	protected void PrepareSearch(vector searchPos, float searchRad)
	{
		DCO_GarrisonRegistry gReg = DCO_GarrisonRegistry.GetInstance();
		if (gReg && m_Building)
		{
			m_GB = gReg.Request(m_Building, 0);
			m_bFromCache = DCO_UseSlotCache() && m_GB && m_GB.m_bReady && gReg.IsComplete(m_GB);
		}

		BuildCells(searchPos, searchRad);
		SnapshotOccupants();

		SCR_CoverManagerComponent coverMgr = SCR_CoverManagerComponent.GetInstance();
		if (!coverMgr || !m_OwnerEntity)
			return;

		vector prev = coverMgr.GetBookedPosition(m_OwnerEntity);
		coverMgr.ReleasePosition(m_OwnerEntity);

		if (prev != vector.Zero && IsInsideBuildingBounds(prev))
		{
			m_vPrevBookedPos = prev;
			m_bPendingPrev   = true;
		}
	}

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

		if (m_aCells.IsEmpty())
		{
			for (int j = 0; j < total; j++)
			{
				m_aCells.Insert(j);
			}
		}

		for (int k = m_aCells.Count() - 1; k > 0; k--)
		{
			int r = Math.RandomInt(0, k + 1);
			int tmp = m_aCells[k];
			m_aCells[k] = m_aCells[r];
			m_aCells[r] = tmp;
		}
	}

	protected vector GetCellLocalPoint(int idx, float fx, float fy, float fz)
	{
		int ix   = idx % m_iCellsX;
		int rest = idx / m_iCellsX;
		int iz   = rest % m_iCellsZ;
		int iy   = rest / m_iCellsZ;

		vector size = m_vLocalMaxs - m_vLocalMins;

		vector local;
		local[0] = m_vLocalMins[0] + (ix + fx) * (size[0] / m_iCellsX);
		local[1] = m_vLocalMins[1] + (iy + fy) * (size[1] / m_iCellsY);
		local[2] = m_vLocalMins[2] + (iz + fz) * (size[2] / m_iCellsZ);
		return local;
	}

	protected vector SampleCell(int idx)
	{
		vector local    = GetCellLocalPoint(idx, Math.RandomFloat01(), Math.RandomFloat01(), Math.RandomFloat01());
		vector worldPos = m_Building.CoordToParent(local);

		float groundHeight = GetGame().GetWorld().GetSurfaceY(worldPos[0], worldPos[2]);
		if (worldPos[1] < groundHeight)
			worldPos[1] = groundHeight;

		return worldPos;
	}

	protected bool IsInsideBuildingBounds(vector worldPos)
	{
		if (!m_Building)
			return false;

		vector local = m_Building.CoordToLocal(worldPos);
		for (int a = 0; a < 3; a++)
		{
			if (local[a] < m_vLocalMins[a] - 0.5 || local[a] > m_vLocalMaxs[a] + 0.5)
				return false;
		}
		return true;
	}

	protected void RunSearchBatch(vector searchPos, float searchRad)
	{
		if (m_bFromCache && m_GB)
		{
			PickFromCache(searchPos, searchRad);
			return;
		}

		if (!m_pNavmesh)
			m_pNavmesh = GetGame().GetAIWorld().GetNavmeshWorldComponent("Soldiers");

		if (!m_pNavmesh)
		{
			FinalizeResult();
			return;
		}

		int sliceStart = System.GetTickCount();
		for (int i = 0; i < m_iCandidatesPerTick; i++)
		{
			if (i > 0 && System.GetTickCount() - sliceStart >= 1)
			{
				DCO_Perf.Count("indoor:slice_cut");
				break;
			}
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

			if (m_GB && m_iCacheAdded < 10)
			{
				DCO_GarrisonRegistry sReg = DCO_GarrisonRegistry.GetInstance();
				if (sReg && sReg.AddSample(m_GB, validPos, m_fLastCover, m_fLastFire, m_vLastFireDir, m_fLastMinExit))
					m_iCacheAdded++;
			}

			if (m_fBestScore >= m_fGoodEnoughScore)
			{
				FinalizeResult();
				return;
			}
		}
	}

	protected void FinalizeResult()
	{
		m_bSearchDone    = true;
		m_fResultTime_ms = GetGame().GetWorld().GetWorldTime();

		m_fBestScore = 0;
		m_vBestPos   = vector.Zero;

		DCO_GarrisonRegistry fReg = DCO_GarrisonRegistry.GetInstance();
		if (!m_bFromCache && fReg && m_GB)
			fReg.EndSample(m_GB, m_iCacheAdded, DCO_Log.IdUnit(m_OwnerEntity));

		for (int i = 0; i < m_aTopPos.Count(); i++)
		{
			if (IsOccupiedByCharacterNow(m_aTopPos[i]))
				continue;
			if (!DCO_CachedReachable(i))
				continue;

			m_fBestScore = m_aTopScore[i];
			m_vBestPos   = m_aTopPos[i];
			DCO_GarrisonRegistry.MarkIndoorUse(m_BuildingKey, m_MyGroup);
			if (m_GB)
				DCO_BldLearn.OccupyGB(m_OwnerEntity, m_GB, m_vBestPos, DCO_BldLearn.UseFor(m_OwnerEntity));
			return;
		}
	}

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

	protected float GetPruneThreshold()
	{
		int maxKeep = Math.Max(1, m_iTopCandidates);
		if (m_aTopScore.Count() < maxKeep)
			return -1;

		return m_aTopScore[maxKeep - 1];
	}

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
		if (halfWidth > 4.0)
			m_aQueryFoundBuilding.Insert(e);

		return true;
	}

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

		float floorFrac = TraceFraction(outPos + 1.55 * vector.Up, outPos - 5 * vector.Up);
		if (floorFrac >= 0.9)
			return DCO_BuildingPosCreation.FAIL;

		IEntity floorEnt = DCO_Util.LastTraceEnt();
		if (!floorEnt)
			return DCO_BuildingPosCreation.FAIL;

		if (floorEnt.GetRootParent() != m_Building.GetRootParent())
			return DCO_BuildingPosCreation.FAIL;

		if (MeasureHeadroom(outPos) < m_fMinHeadroom)
			return DCO_BuildingPosCreation.FAIL;

		return DCO_BuildingPosCreation.SUCCESS;
	}

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

	protected float TraceFraction(vector start, vector end)
	{
		return DCO_Util.TraceFrac(start, end, null, TraceFlags.ENTS, -1, "t:DCO_FindIndoorPosition");
	}

	protected float MeasureHeadroom(vector floorPos)
	{
		float frac = TraceFraction(floorPos + 0.1 * vector.Up, floorPos + 6.0 * vector.Up);
		return frac * 6.0;
	}

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

	protected float ScoreCandidate(vector pos, vector searchPos, float searchRad, float nearestBooked, float bonus)
	{
		float spreadScore = 1.0;
		if (nearestBooked > 0)
			spreadScore = Math.Clamp(nearestBooked / 8.0, 0.0, 1.0);

		float proximityScore = 1.0;
		if (searchRad > 0)
			proximityScore = 1.0 - Math.Clamp(vector.Distance(pos, searchPos) / searchRad, 0.0, 1.0);

		float totalWeight = m_fWeightCover + m_fWeightFireLine + m_fWeightSpread + m_fWeightProximity;
		float pruneAt     = GetPruneThreshold();
		float cheapPart   = (m_fWeightSpread * spreadScore) + (m_fWeightProximity * proximityScore);

		vector probeOrigin = pos + 1.0 * vector.Up;

		int   blockedDirections = 0;
		float fireValue         = 0;
		float minExit           = float.MAX;

		m_aRayHitDist.Clear();
		vector bestDir = vector.Zero;
		float bestDirDist = -1;
		bool bestExit = false;

		for (int i = 0; i < 8; i++)
		{
			float angleRad = (i * 360.0 / 8.0) * Math.DEG2RAD;
			vector dir = Vector(Math.Cos(angleRad), 0, Math.Sin(angleRad));

			float hitDist = TraceFraction(probeOrigin, probeOrigin + dir * 15.0) * 15.0;
			m_aRayHitDist.Insert(hitDist);
			if (!bestExit && hitDist > bestDirDist)
			{
				bestDirDist = hitDist;
				bestDir = dir;
			}

			if (hitDist < 2.5)
			{
				blockedDirections++;
			}
			else if (hitDist >= 8.0)
			{
				float exitDist = GetBoundsExitDistance(probeOrigin, dir);
				if (hitDist >= exitDist + 0.5)
				{
					fireValue += 1.0;
					if (!bestExit || hitDist > bestDirDist)
					{
						bestExit = true;
						bestDirDist = hitDist;
						bestDir = dir;
					}
					minExit = Math.Min(minExit, exitDist);
				}
				else
					fireValue += m_fInteriorLineValue;
			}

			if (pruneAt >= 0 && totalWeight > 0)
			{
				int remaining = 8 - i - 1;
				float coverUB = (blockedDirections + remaining) / 8.0;
				float fireUB  = Math.Min(fireValue + remaining, 3.0) / 3.0;
				float upper   = ((m_fWeightCover * coverUB) + (m_fWeightFireLine * fireUB) + cheapPart) / totalWeight + bonus;

				if (upper <= pruneAt)
					return -1;
			}
		}

		if (blockedDirections >= 8)
			return -1;

		m_fLastCover = 1.0 * blockedDirections / 8.0;
		m_fLastFire = Math.Min(fireValue, 3.0) / 3.0;
		m_vLastFireDir = bestDir;
		m_fLastMinExit = -1;
		if (minExit < float.MAX)
			m_fLastMinExit = minExit;

		if (m_bRejectChokepoints && IsChokepoint())
			return -1;

		if (m_bRequireFireLine && fireValue <= 0)
			return -1;

		float coverScore = 1.0 * blockedDirections / 8.0;
		float fireScore  = Math.Min(fireValue, 3.0) / 3.0;

		if (totalWeight <= 0)
			return ApplyDoorPenalty(coverScore + bonus, pos);

		float score = (m_fWeightCover * coverScore) + (m_fWeightFireLine * fireScore) + cheapPart;
		float standoff = 0;
		if (minExit < float.MAX && DCO_GlobalAIComponent.BehaviorOn(DCO_EBehavior.AWARENESS))
		{
			if (minExit < 1.0)
				standoff = -0.08;
			else if (minExit <= 2.5)
				standoff = 0.04;
		}

		return ApplyDoorPenalty(score / totalWeight + bonus + standoff, pos);
	}

	protected void BuildDoorPoints()
	{
		m_aDoorPoints.Clear();

		foreach (IEntity d : m_aQueryDoors)
		{
			if (!d)
				continue;

			vector origin = d.GetOrigin();
			m_aDoorPoints.Insert(origin);

			vector mins, maxs;
			d.GetBounds(mins, maxs);

			vector center = d.CoordToParent((mins + maxs) * 0.5);
			center[1] = origin[1];

			if (vector.DistanceSq(center, origin) > 0.01)
				m_aDoorPoints.Insert(center);
		}
	}

	protected float GetNearestDoorDistXZ(vector pos)
	{
		float best = float.MAX;

		foreach (vector p : m_aDoorPoints)
		{
			if (Math.AbsFloat(p[1] - pos[1]) > 1.5)
				continue;

			float d = vector.DistanceXZ(p, pos);
			if (d < best)
				best = d;
		}

		return best;
	}

	protected float ApplyDoorPenalty(float score, vector pos)
	{
		if (m_fDoorAvoidRadius <= 0 || m_fDoorAvoidPenalty <= 0)
			return score;

		float d = GetNearestDoorDistXZ(pos);
		if (d >= m_fDoorAvoidRadius)
			return score;

		float t = 1.0 - (d / m_fDoorAvoidRadius);
		return Math.Max(score - m_fDoorAvoidPenalty * t, 0.01);
	}

	protected bool IsChokepoint()
	{
		int half = 8 / 2;
		if (m_aRayHitDist.Count() < 8)
			return false;

		for (int i = 0; i < half; i++)
		{
			if ((m_aRayHitDist[i] + m_aRayHitDist[i + half]) < m_fChokepointWidth)
				return true;
		}

		return false;
	}

	protected void SnapshotOccupants()
	{
		m_aQueryCharacters.Clear();
		m_aQueryDoors.Clear();

		vector center = m_Building.CoordToParent((m_vLocalMins + m_vLocalMaxs) * 0.5);
		float  radius = 0.5 * vector.Distance(m_vLocalMins, m_vLocalMaxs) + Math.Max(m_fOccupancyRadius, m_fDoorBlockRadius);

		DCO_Perf.Count("q:DCO_FindIndoorPosition");
		GetGame().GetWorld().QueryEntitiesBySphere(center, radius, QueryCallbackC);

		BuildDoorPoints();
	}

	bool QueryCallbackC(IEntity e)
	{
		if (!e || e == m_OwnerEntity)
			return true;

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

		if (GetNearestDoorDistXZ(pos) < m_fDoorBlockRadius)
			return true;

		SCR_CoverManagerComponent coverMgr = SCR_CoverManagerComponent.GetInstance();
		if (coverMgr)
		{
			nearestBooked = coverMgr.GetNearestBookedDistanceXZ(pos);
			if (nearestBooked > 0 && nearestBooked < m_fMinBookedDistance)
				return true;
		}

		return false;
	}

	protected bool IsOccupiedByCharacterNow(vector pos)
	{
		m_aQueryCharacters.Clear();
		m_aQueryDoors.Clear();

		DCO_Perf.Count("q:DCO_FindIndoorPosition");
		GetGame().GetWorld().QueryEntitiesBySphere(pos, m_fOccupancyRadius, QueryCallbackC);

		float occupancyRadSq = m_fOccupancyRadius * m_fOccupancyRadius;
		foreach (IEntity e : m_aQueryCharacters)
		{
			if (e && vector.DistanceSq(e.GetOrigin(), pos) < occupancyRadSq)
				return true;
		}
		return false;
	}

	protected static ref TStringArray s_aVarsOut;
	protected static TStringArray GetVarsOut()
	{
		if (!s_aVarsOut)
			s_aVarsOut = { "Position Found", PORT_VECTOR_BOOL };
		return s_aVarsOut;
	}
	override TStringArray GetVariablesOut()
	{
		return GetVarsOut();
	}

	protected static ref TStringArray s_aVarsIn;
	protected static TStringArray GetVarsIn()
	{
		if (!s_aVarsIn)
			s_aVarsIn = { PORT_CENTER_OF_SEARCH, PORT_RADIUS };
		return s_aVarsIn;
	}
	override TStringArray GetVariablesIn()
	{
		return GetVarsIn();
	}

	static override string GetOnHoverDescription()
	{
		return "Find Indoor Position (stratified sampling, scored by cover + fire line out of the building + spread + proximity).";
	}
};
