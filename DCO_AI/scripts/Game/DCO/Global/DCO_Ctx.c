enum DCO_ERangeBand
{
	CONTACT,
	CLOSE,
	MEDIUM,
	LONG,
	EXTREME
}

enum DCO_EEnv
{
	INDOOR,
	TRENCH,
	URBAN,
	FOREST,
	OPEN
}

enum DCO_ECtxRel
{
	EVEN,
	HOLD,
	ASSAULT
}

enum DCO_ECtxKind
{
	NONE,
	CQB_INDOOR,
	HARD_STANDOFF,
	CQB_URBAN,
	FOREST_CLOSE,
	TRENCH_CLEAR,
	OPEN_MEETING,
	HOLD_INDOOR,
	HOLD_TRENCH,
	HOLD_URBAN,
	HOLD_FOREST,
	ASSAULT_INDOOR,
	ASSAULT_TRENCH,
	ASSAULT_URBAN,
	ASSAULT_FOREST
}

enum DCO_ECtxParam
{
	FIRE,
	AIM,
	ACC,
	SUP,
	FRAG,
	MOVE,
	BMIN,
	BMAX,
	CMAX,
	PACE,
	UNIT,
	CRAWL,
	PEEK,
	C2C,
	CONC,
	SMOKE,
	THREAT,
	REACT,
	COUNT
}

class DCO_CtxState
{
	float m_fAt = -1;
	float m_fDist;
	int m_iBand = -1;
	int m_iMe = -1;
	int m_iPend = -1;
	int m_iTgt = -1;
	int m_iRel;
	int m_iKind;
	bool m_bSameBld;
	ref array<float> m_aP = {};
}

class DCO_EnvCell
{
	int m_iEnv;
	int m_iBld;
	int m_iTree;
	int m_iFort;
}

class DCO_Ctx
{
	protected static ref map<SCR_AIUtilityComponent, ref DCO_CtxState> s_mState;
	protected static ref map<int, ref DCO_EnvCell> s_mCells;
	protected static ref array<ref array<float>> s_aRows;
	protected static ref array<ref array<float>> s_aEnvMul;
	protected static ref array<ref array<float>> s_aEnvSet;
	protected static ref array<IEntity> s_aRoots;
	protected static ref array<string> s_aFortKeys;
	protected static vector s_vQ;
	protected static float s_fQBldR;
	protected static float s_fQTreeR;
	protected static int s_iQBld;
	protected static int s_iQTree;
	protected static int s_iQFort;
	protected static float s_fBudgetAt;
	protected static int s_iBudget;
	protected static bool s_bInitLogged;

	protected static array<IEntity> Roots()
	{
		if (!s_aRoots)
			s_aRoots = new array<IEntity>();
		return s_aRoots;
	}

	protected static array<vector> Trees()
	{
		if (!s_aTrees)
			s_aTrees = new array<vector>();
		return s_aTrees;
	}

	protected static array<string> FortKeys()
	{
		if (!s_aFortKeys)
			s_aFortKeys = {"sandbag", "trench", "bunker", "fortif", "hesco", "foxhole", "dugout", "revetment"};
		return s_aFortKeys;
	}

	static bool On(SCR_AIUtilityComponent u)
	{
		return u && DCO_Stress.T(u, "ctx.on", 1.0) > 0;
	}

	static bool OnGlobal()
	{
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetInstance();
		return g && g.DCO_TuneF("ctx.on", 1.0) > 0;
	}

	static float TuneG(string key, float def)
	{
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetInstance();
		if (!g)
			return def;
		return g.DCO_TuneF(key, def);
	}

	static float TuneFor(IEntity e, string key, float def)
	{
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetFor(e);
		if (!g)
			return def;
		return g.DCO_TuneF(key, def);
	}

	static float TuneForU(SCR_AIUtilityComponent u, string key, float def)
	{
		if (!u)
			return TuneG(key, def);
		return TuneFor(u.m_OwnerEntity, key, def);
	}

	static float TuneForGU(SCR_AIGroupUtilityComponent gu, string key, float def)
	{
		if (!gu)
			return TuneG(key, def);
		return TuneFor(gu.m_Owner, key, def);
	}

	static float TuneForCmd(AICommander_BaseComponent cmd, string key, float def)
	{
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetForCommander(cmd);
		if (!g)
			return def;
		return g.DCO_TuneF(key, def);
	}

	static int Band(float d)
	{
		if (d < TuneG("ctx.band_contact_m", 10.0))
			return DCO_ERangeBand.CONTACT;
		if (d < TuneG("ctx.band_close_m", 50.0))
			return DCO_ERangeBand.CLOSE;
		if (d < TuneG("ctx.band_medium_m", 200.0))
			return DCO_ERangeBand.MEDIUM;
		if (d < TuneG("ctx.band_long_m", 400.0))
			return DCO_ERangeBand.LONG;
		return DCO_ERangeBand.EXTREME;
	}

	protected static int BandHyst(float d, int prev)
	{
		int b = Band(d);
		if (prev < 0 || b == prev)
			return b;
		float h = 2.0;
		if (Band(d + h) == prev || Band(d - h) == prev)
			return prev;
		return b;
	}

	static string BandName(int b)
	{
		return typename.EnumToString(DCO_ERangeBand, b);
	}

	static string EnvName(int e)
	{
		return typename.EnumToString(DCO_EEnv, e);
	}

	static string KindName(int k)
	{
		return typename.EnumToString(DCO_ECtxKind, k);
	}

	static string RelName(int r)
	{
		return typename.EnumToString(DCO_ECtxRel, r);
	}

	static int Rank(int env)
	{
		switch (env)
		{
			case DCO_EEnv.INDOOR: return TuneG("ctx.rank_indoor", 3.0);
			case DCO_EEnv.TRENCH: return TuneG("ctx.rank_trench", 3.0);
			case DCO_EEnv.URBAN: return TuneG("ctx.rank_urban", 2.0);
			case DCO_EEnv.FOREST: return TuneG("ctx.rank_forest", 1.0);
		}
		return TuneG("ctx.rank_open", 0.0);
	}

	static int Rel(int me, int tgt)
	{
		int a = Rank(me);
		int b = Rank(tgt);
		if (a > b)
			return DCO_ECtxRel.HOLD;
		if (a < b)
			return DCO_ECtxRel.ASSAULT;
		return DCO_ECtxRel.EVEN;
	}

	static int Kind(int band, int me, int tgt, bool sameBld)
	{
		if (band != DCO_ERangeBand.CONTACT && band != DCO_ERangeBand.CLOSE)
			return DCO_ECtxKind.NONE;
		int rel = Rel(me, tgt);
		if (rel == DCO_ECtxRel.HOLD)
		{
			switch (me)
			{
				case DCO_EEnv.INDOOR: return DCO_ECtxKind.HOLD_INDOOR;
				case DCO_EEnv.TRENCH: return DCO_ECtxKind.HOLD_TRENCH;
				case DCO_EEnv.URBAN: return DCO_ECtxKind.HOLD_URBAN;
			}
			return DCO_ECtxKind.HOLD_FOREST;
		}
		if (rel == DCO_ECtxRel.ASSAULT)
		{
			switch (tgt)
			{
				case DCO_EEnv.INDOOR: return DCO_ECtxKind.ASSAULT_INDOOR;
				case DCO_EEnv.TRENCH: return DCO_ECtxKind.ASSAULT_TRENCH;
				case DCO_EEnv.URBAN: return DCO_ECtxKind.ASSAULT_URBAN;
			}
			return DCO_ECtxKind.ASSAULT_FOREST;
		}
		if (me != tgt)
			return DCO_ECtxKind.HARD_STANDOFF;
		switch (me)
		{
			case DCO_EEnv.INDOOR:
			{
				if (sameBld)
					return DCO_ECtxKind.CQB_INDOOR;
				return DCO_ECtxKind.HARD_STANDOFF;
			}
			case DCO_EEnv.TRENCH:
			{
				if (band == DCO_ERangeBand.CONTACT)
					return DCO_ECtxKind.TRENCH_CLEAR;
				return DCO_ECtxKind.HARD_STANDOFF;
			}
			case DCO_EEnv.URBAN: return DCO_ECtxKind.CQB_URBAN;
			case DCO_EEnv.FOREST: return DCO_ECtxKind.FOREST_CLOSE;
		}
		return DCO_ECtxKind.OPEN_MEETING;
	}

	static bool IsCQBKind(int kind)
	{
		return kind == DCO_ECtxKind.CQB_INDOOR || kind == DCO_ECtxKind.CQB_URBAN || kind == DCO_ECtxKind.TRENCH_CLEAR;
	}

	static int EnvAtPos(vector pos)
	{
		string method;
		if (SCR_CoverManagerComponent.DCO_GetBuildingAtPos(pos, null, method))
			return DCO_EEnv.INDOOR;
		return CellEnv(pos, true);
	}

	static int EnvOf(IEntity ent)
	{
		if (!ent)
			return DCO_EEnv.OPEN;
		if (SCR_CoverManagerComponent.IsEntityInsideBuilding(ent))
			return DCO_EEnv.INDOOR;
		return CellEnv(ent.GetOrigin(), true);
	}

	static int CellEnv(vector pos, bool trenchCheck)
	{
		DCO_EnvCell c = Cell(pos);
		if (!c)
			return DCO_EEnv.OPEN;
		if (trenchCheck && c.m_iFort > 0 && FortNear(pos, TuneG("ctx.trench_radius_m", 3.0)))
			return DCO_EEnv.TRENCH;
		return c.m_iEnv;
	}

	protected static DCO_EnvCell Cell(vector pos)
	{
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return null;
		if (!s_mCells)
			s_mCells = new map<int, ref DCO_EnvCell>();
		float cellM = Math.Max(TuneG("ctx.cell_m", 25.0), 5.0);
		int cx = Math.Floor(pos[0] / cellM);
		int cz = Math.Floor(pos[2] / cellM);
		int key = cx * 100000 + cz;
		DCO_EnvCell c = s_mCells.Get(key);
		if (c)
		{
			DCO_Perf.Count("env:hit");
			return c;
		}
		DCO_Perf.Count("env:miss");
		float now = world.GetWorldTime();
		if (now != s_fBudgetAt)
		{
			s_fBudgetAt = now;
			s_iBudget = 0;
		}
		if (s_iBudget >= 4)
		{
			DCO_Perf.Count("env:defer");
			if (DCO_Log.On(DCO_ELogChannel.SYS, DCO_ELogLevel.TRACE))
				DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.TRACE, "-", string.Format("envcell_defer cell=%1,%2", cx, cz));
			return null;
		}
		s_iBudget++;
		LogInitOnce();

		vector center = Vector((cx + 0.5) * cellM, 0, (cz + 0.5) * cellM);
		center[1] = world.GetSurfaceY(center[0], center[2]);
		s_vQ = center;
		s_fQBldR = TuneG("ctx.urban_radius_m", 30.0);
		s_fQTreeR = TuneG("ctx.forest_radius_m", 20.0);
		s_iQBld = 0;
		s_iQTree = 0;
		s_iQFort = 0;
		Roots().Clear();
		DCO_Perf.Count("q:DCO_Env");
		world.QueryEntitiesBySphere(center, Math.Max(s_fQBldR, s_fQTreeR), CountCell, null, EQueryEntitiesFlags.STATIC);
		Roots().Clear();

		c = new DCO_EnvCell();
		c.m_iBld = s_iQBld;
		c.m_iTree = s_iQTree;
		c.m_iFort = s_iQFort;
		c.m_iEnv = DCO_EEnv.OPEN;
		if (s_iQBld >= TuneG("ctx.urban_bld_min", 3.0))
			c.m_iEnv = DCO_EEnv.URBAN;
		else if (s_iQTree >= TuneG("ctx.forest_tree_min", 12.0))
			c.m_iEnv = DCO_EEnv.FOREST;
		if (s_mCells.Count() > 20000)
			s_mCells.Clear();
		s_mCells.Set(key, c);
		if (DCO_Log.On(DCO_ELogChannel.SYS, DCO_ELogLevel.DEBUG))
			DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.DEBUG, "-", string.Format("envcell cell=%1,%2 pos=%3 bld=%4 tree=%5 fort=%6 result=%7",
				cx, cz, DCO_Log.Pos(center), c.m_iBld, c.m_iTree, c.m_iFort, EnvName(c.m_iEnv)));
		return c;
	}

	protected static bool CountCell(IEntity e)
	{
		if (!e)
			return true;
		vector p = e.GetOrigin();
		float d = vector.DistanceXZ(p, s_vQ);
		if (Tree.Cast(e))
		{
			if (d <= s_fQTreeR)
				s_iQTree++;
			return true;
		}
		IEntity root = e.GetRootParent();
		if (!root)
			root = e;
		if (Roots().Contains(root))
			return true;
		Roots().Insert(root);
		if (IsFort(root))
		{
			s_iQFort++;
			return true;
		}
		if (d <= s_fQBldR && IsBuilding(root))
			s_iQBld++;
		return true;
	}

	protected static bool IsBuilding(IEntity root)
	{
		if (root.FindComponent(DCO_BuildingPositionComponent))
			return true;
		EntityPrefabData pd = root.GetPrefabData();
		if (!pd)
			return false;
		string path = pd.GetPrefabName();
		return path.Contains("/Buildings/") || path.Contains("Structures/Houses");
	}

	protected static bool IsFort(IEntity e)
	{
		EntityPrefabData pd = e.GetPrefabData();
		if (!pd)
			return false;
		string n = pd.GetPrefabName();
		n.ToLower();
		foreach (string k : FortKeys())
		{
			if (n.Contains(k))
				return true;
		}
		return false;
	}

	protected static bool FortNear(vector pos, float r)
	{
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return false;
		s_iQFort = 0;
		Roots().Clear();
		DCO_Perf.Count("q:DCO_EnvFort");
		world.QueryEntitiesBySphere(pos, r, CountFort, null, EQueryEntitiesFlags.STATIC);
		Roots().Clear();
		return s_iQFort > 0;
	}

	protected static bool CountFort(IEntity e)
	{
		if (!e)
			return true;
		IEntity root = e.GetRootParent();
		if (!root)
			root = e;
		if (IsFort(root) || IsFort(e))
		{
			s_iQFort++;
			return false;
		}
		return true;
	}

	protected static void LogInitOnce()
	{
		if (s_bInitLogged)
			return;
		s_bInitLogged = true;
		DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", string.Format("ctx_init on=%1 legacy close_move=%2 very_long_move=%3 close_combat=%4 long_combat=%5 cell_m=%6 urban_bld_min=%7 forest_tree_min=%8",
			DCO_Util.B01(OnGlobal()), SCR_AICombatMoveUtils.CLOSE_RANGE_COMBAT_DIST, SCR_AICombatMoveUtils.VERY_LONG_RANGE_COMBAT_DIST, SCR_AICombatComponent.CLOSE_RANGE_COMBAT_DISTANCE,
			SCR_AICombatComponent.LONG_RANGE_COMBAT_DISTANCE, TuneG("ctx.cell_m", 25.0), TuneG("ctx.urban_bld_min", 3.0), TuneG("ctx.forest_tree_min", 12.0)));
	}

	static DCO_CtxState Get(SCR_AIUtilityComponent u)
	{
		if (!On(u) || !u.m_OwnerEntity || !u.m_CombatComponent || !GetGame().GetWorld())
			return null;
		BaseTarget t = u.m_CombatComponent.GetCurrentTarget();
		if (!t)
			return null;
		if (!s_mState)
			s_mState = new map<SCR_AIUtilityComponent, ref DCO_CtxState>();
		float now = GetGame().GetWorld().GetWorldTime();
		DCO_CtxState s = s_mState.Get(u);
		if (s && s.m_fAt >= 0 && now >= s.m_fAt && now - s.m_fAt < 1000.0)
			return s;
		if (!s)
		{
			if (s_mState.Count() > 600)
				s_mState.Clear();
			s = new DCO_CtxState();
			s_mState.Set(u, s);
		}
		s.m_fAt = now;
		DCO_Perf.Count("ctx:update");
		Refresh(u, s, t);
		return s;
	}

	protected static void Refresh(SCR_AIUtilityComponent u, DCO_CtxState s, BaseTarget t)
	{
		IEntity me = u.m_OwnerEntity;
		IEntity te = t.GetTargetEntity();
		vector tpos = t.GetLastSeenPosition();
		float dist = t.GetDistance();
		if (dist <= 0)
			dist = vector.Distance(me.GetOrigin(), tpos);

		int band = BandHyst(dist, s.m_iBand);

		int meNow = EnvOf(me);
		int meEnv = s.m_iMe;
		if (meEnv < 0)
			meEnv = meNow;
		else if (meNow != meEnv)
		{
			if (s.m_iPend == meNow)
				meEnv = meNow;
			s.m_iPend = meNow;
		}
		if (meEnv == meNow)
			s.m_iPend = -1;

		int tgtEnv;
		IEntity tgtBld;
		if (te && t.GetTimeSinceSeen() < 2.0)
		{
			tgtEnv = EnvOf(te);
			if (tgtEnv == DCO_EEnv.INDOOR)
				tgtBld = SCR_CoverManagerComponent.DCO_GetBuildingAt(te);
		}
		else
		{
			string method;
			tgtBld = SCR_CoverManagerComponent.DCO_GetBuildingAtPos(tpos, null, method);
			if (tgtBld)
				tgtEnv = DCO_EEnv.INDOOR;
			else
				tgtEnv = CellEnv(tpos, true);
		}
		bool same;
		if (meEnv == DCO_EEnv.INDOOR && tgtEnv == DCO_EEnv.INDOOR && tgtBld)
			same = SCR_CoverManagerComponent.DCO_GetBuildingAt(me) == tgtBld;

		int rel = Rel(meEnv, tgtEnv);
		int kind = Kind(band, meEnv, tgtEnv, same);
		s.m_fDist = dist;

		bool changed = band != s.m_iBand || meEnv != s.m_iMe || tgtEnv != s.m_iTgt || kind != s.m_iKind || rel != s.m_iRel || s.m_aP.IsEmpty();
		if (!changed)
			return;

		if (s.m_iBand >= 0 && DCO_Log.On(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO))
			DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("ctx unit=%1 from=%2/%3 to=%4/%5 tgt_env=%6 rel=%7 kind=%8 dist=%9",
				DCO_Log.IdUnit(me), BandName(s.m_iBand), EnvName(s.m_iMe), BandName(band), EnvName(meEnv), EnvName(tgtEnv), RelName(rel), KindName(kind), Math.Round(dist)));
		if (kind != s.m_iKind)
		{
			DCO_Perf.Count("ctx:kind_change");
			DCO_Perf.Count("ctxkind:" + KindName(kind));
		}
		if (meEnv != s.m_iMe && s.m_iMe >= 0)
			DCO_Perf.Count("ctx:env_change");

		s.m_iBand = band;
		s.m_iMe = meEnv;
		s.m_iTgt = tgtEnv;
		s.m_iRel = rel;
		s.m_iKind = kind;
		s.m_bSameBld = same;
		Build(s);
	}

	static float P(DCO_CtxState s, DCO_ECtxParam p)
	{
		return s.m_aP[p];
	}

	protected static ref map<AIGroup, float> s_mMateMoveAt;
	protected static ref map<AIGroup, float> s_mMateCheckAt;

	static float MateMoveAge_s(SCR_AIUtilityComponent u)
	{
		if (!u || !u.GetAIAgent() || !u.GetAIAgent().GetParentGroup() || !GetGame().GetWorld())
			return 999.0;
		if (!s_mMateMoveAt)
		{
			s_mMateMoveAt = new map<AIGroup, float>();
			s_mMateCheckAt = new map<AIGroup, float>();
		}
		AIGroup g = u.GetAIAgent().GetParentGroup();
		float now = GetGame().GetWorld().GetWorldTime();
		float checkAt;
		if (!s_mMateCheckAt.Find(g, checkAt) || now - checkAt > 500.0)
		{
			if (s_mMateCheckAt.Count() > 300)
			{
				s_mMateCheckAt.Clear();
				s_mMateMoveAt.Clear();
			}
			s_mMateCheckAt.Set(g, now);
			array<AIAgent> agents = {};
			g.GetAgents(agents);
			foreach (AIAgent a : agents)
			{
				SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
				if (!ca || ca == u.GetAIAgent() || !ca.m_UtilityComponent || !ca.m_UtilityComponent.m_CombatMoveState)
					continue;
				if (ca.m_UtilityComponent.m_CombatMoveState.IsExecutingRequest())
				{
					s_mMateMoveAt.Set(g, now);
					break;
				}
			}
		}
		float movedAt;
		if (!s_mMateMoveAt.Find(g, movedAt))
			return 999.0;
		return (now - movedAt) * 0.001;
	}

	static bool HideOn(SCR_AIUtilityComponent u)
	{
		if (u)
			return DCO_Stress.T(u, "ctx.on", 1.0) > 0 && DCO_Stress.T(u, "ctx.hide_on", 1.0) > 0;
		return TuneG("ctx.on", 1.0) > 0 && TuneG("ctx.hide_on", 1.0) > 0;
	}

	static float HideCloseM(SCR_AIUtilityComponent u)
	{
		if (HideOn(u))
		{
			DCO_Perf.Count("ctx:hide_band");
			return TuneG("ctx.band_close_m", 50.0);
		}
		return SCR_AICombatMoveUtils.CLOSE_RANGE_COMBAT_DIST;
	}

	static float HideFarM(SCR_AIUtilityComponent u)
	{
		if (HideOn(u))
			return TuneG("ctx.band_long_m", 400.0);
		return SCR_AICombatMoveUtils.VERY_LONG_RANGE_COMBAT_DIST;
	}

	static int SpareMags(IEntity me, BaseWeaponComponent weapon)
	{
		if (!me || !weapon)
			return 99;
		InventoryStorageManagerComponent inv = InventoryStorageManagerComponent.Cast(me.FindComponent(InventoryStorageManagerComponent));
		if (!inv)
			return 99;
		return inv.GetMagazineCountByWeapon(weapon);
	}

	static float BaseP(int band, DCO_ECtxParam p)
	{
		Tables();
		return s_aRows[band][p];
	}

	static float ThreatW(SCR_AIUtilityComponent u, vector shooterPos)
	{
		if (shooterPos == vector.Zero || !On(u) || !u.m_OwnerEntity)
			return 1.0;
		float d = vector.Distance(u.m_OwnerEntity.GetOrigin(), shooterPos);
		float t = BaseP(Band(d), DCO_ECtxParam.THREAT);
		float k = DCO_Stress.T(u, "ctx.threat_k", 0.5);
		DCO_Perf.Count("ctx:threat_w");
		return Math.Clamp(1.0 + (t - 1.0) * k, 0.2, 3.0);
	}

	static float ReactDelay_s(SCR_AIUtilityComponent u, DCO_CtxState s)
	{
		return (P(s, DCO_ECtxParam.REACT) + P(s, DCO_ECtxParam.AIM)) * DCO_Stress.T(u, "ctx.react_scale", 0.0);
	}

	static bool BuddyTurn(AIAgent agent, float period_ms, out bool paired)
	{
		paired = false;
		if (!agent || !agent.GetParentGroup())
			return true;
		array<AIAgent> agents = {};
		agent.GetParentGroup().GetAgents(agents);
		int idx = agents.Find(agent);
		if (idx < 0)
			return true;
		int partner = idx + 1;
		if (idx % 2 == 1)
			partner = idx - 1;
		if (partner >= agents.Count() || !agents[partner] || !DCO_Util.IsAlive(agents[partner].GetControlledEntity()))
			return true;
		paired = true;
		int slot = (int)(GetGame().GetWorld().GetWorldTime() / period_ms) % 2;
		return idx % 2 == slot;
	}

	static bool FindCorner(IEntity me, vector target, float dmin, float dmax, out vector outPos)
	{
		DCO_GarrisonRegistry reg = DCO_GarrisonRegistry.GetInstance();
		if (!me || !reg)
			return false;
		vector p = me.GetOrigin();
		array<IEntity> buildings = {};
		reg.FindBuildings(p, dmax + 15.0, buildings);
		float myToT = vector.DistanceXZ(p, target);
		float want = 0.5 * (dmin + dmax);
		float best = -float.MAX;
		bool found;
		foreach (IEntity b : buildings)
		{
			if (!b)
				continue;
			vector mins, maxs;
			b.GetBounds(mins, maxs);
			for (int i = 0; i < 4; i++)
			{
				float lx = maxs[0] + 1.0;
				if (i == 1 || i == 2)
				{
					lx = mins[0] - 1.0;
				}
				float lz = maxs[2] + 1.0;
				if (i >= 2)
					lz = mins[2] - 1.0;
				vector c = b.CoordToParent(Vector(lx, 0, lz));
				float dMe = vector.DistanceXZ(p, c);
				if (dMe < Math.Max(dmin, 3.0) || dMe > dmax)
					continue;
				float progress = myToT - vector.DistanceXZ(c, target);
				if (progress < 3.0 || vector.DistanceXZ(c, target) < 8.0)
					continue;
				float score = progress - 0.3 * Math.AbsFloat(dMe - want);
				if (score > best)
				{
					best = score;
					outPos = c;
					found = true;
				}
			}
		}
		if (found)
			outPos = DCO_Util.Surface(outPos);
		return found;
	}

	static bool FindDeadAngle(IEntity me, vector target, float maxDist, out vector outPos)
	{
		if (!me)
			return false;
		string method;
		IEntity b = SCR_CoverManagerComponent.DCO_GetBuildingAtPos(target, null, method);
		if (!b)
			return false;
		vector p = me.GetOrigin();
		vector mins, maxs;
		b.GetBounds(mins, maxs);
		float best = float.MAX;
		bool found;
		for (int i = 0; i < 4; i++)
		{
			float lx = maxs[0] + 1.0;
			if (i == 1 || i == 2)
				lx = mins[0] - 1.0;
			float lz = maxs[2] + 1.0;
			if (i >= 2)
				lz = mins[2] - 1.0;
			vector c = b.CoordToParent(Vector(lx, 0, lz));
			float d = vector.DistanceXZ(p, c);
			if (d < 3.0 || d > maxDist)
				continue;
			if (d < best)
			{
				best = d;
				outPos = c;
				found = true;
			}
		}
		if (found)
			outPos = DCO_Util.Surface(outPos);
		return found;
	}

	protected static vector s_vTreeMe;
	protected static float s_fTreeR;
	protected static ref array<vector> s_aTrees;

	static bool FindConcealment(IEntity me, vector target, float maxDist, bool advance, out vector outPos)
	{
		BaseWorld world = GetGame().GetWorld();
		if (!me || !world)
			return false;
		vector p = me.GetOrigin();
		s_vTreeMe = p;
		s_fTreeR = maxDist + 2.0;
		Trees().Clear();
		DCO_Perf.Count("q:DCO_EnvConc");
		world.QueryEntitiesBySphere(p, s_fTreeR, CollectTree, null, EQueryEntitiesFlags.STATIC);
		float myToT = vector.DistanceXZ(p, target);
		float want = Math.Max(0.6 * maxDist, 3.0);
		float best = -float.MAX;
		bool found;
		foreach (vector t : Trees())
		{
			vector away = t - target;
			away[1] = 0;
			if (away.Length() < 1.0)
				continue;
			away.Normalize();
			vector spot = t + away * 0.9;
			float dMe = vector.DistanceXZ(p, spot);
			if (dMe < 2.0 || dMe > maxDist + 2.0)
				continue;
			float progress = myToT - vector.DistanceXZ(spot, target);
			if (advance && progress < -2.0)
				continue;
			float score = 0.5 * progress - Math.AbsFloat(dMe - want);
			if (score > best)
			{
				best = score;
				outPos = spot;
				found = true;
			}
		}
		Trees().Clear();
		if (found)
			outPos = DCO_Util.Surface(outPos);
		return found;
	}

	protected static bool CollectTree(IEntity e)
	{
		if (e && Tree.Cast(e) && Trees().Count() < 64)
			Trees().Insert(e.GetOrigin());
		return true;
	}

	static string Tag(DCO_CtxState s)
	{
		if (!s)
			return "off";
		return string.Format("%1/%2>%3 %4 %5", BandName(s.m_iBand), EnvName(s.m_iMe), EnvName(s.m_iTgt), RelName(s.m_iRel), KindName(s.m_iKind));
	}

	protected static bool FromTarget(int p)
	{
		return p == DCO_ECtxParam.FIRE || p == DCO_ECtxParam.AIM || p == DCO_ECtxParam.ACC || p == DCO_ECtxParam.SUP
			|| p == DCO_ECtxParam.FRAG || p == DCO_ECtxParam.THREAT || p == DCO_ECtxParam.REACT;
	}

	protected static void Build(DCO_CtxState s)
	{
		Tables();
		array<float> row;
		bool ovr = s.m_iKind != DCO_ECtxKind.NONE;
		if (ovr)
			row = s_aRows[5 + s.m_iKind - 1];
		else
			row = s_aRows[s.m_iBand];
		s.m_aP.Clear();
		for (int i = 0; i < DCO_ECtxParam.COUNT; i++)
		{
			float v = row[i];
			if (!ovr)
			{
				int env = s.m_iMe;
				if (FromTarget(i))
					env = s.m_iTgt;
				float sv = s_aEnvSet[env][i];
				if (sv >= 0)
					v = sv;
				else
					v = v * s_aEnvMul[env][i];
				if (s.m_iBand == DCO_ERangeBand.MEDIUM || s.m_iBand == DCO_ERangeBand.LONG)
				{
					if (s.m_iRel == DCO_ECtxRel.HOLD)
					{
						if (i == DCO_ECtxParam.MOVE)
							v = 8;
						if (i == DCO_ECtxParam.SUP)
							v = 0.8 * v;
					}
					else if (s.m_iRel == DCO_ECtxRel.ASSAULT)
					{
						if (i == DCO_ECtxParam.SMOKE)
							v = 1;
						if (i == DCO_ECtxParam.UNIT)
							v = 2;
						if (i == DCO_ECtxParam.SUP)
							v = 1.3 * v;
					}
				}
			}
			s.m_aP.Insert(v);
		}
	}

	static array<string> RowNames()
	{
		array<string> n = {};
		for (int b = 0; b < 5; b++)
			n.Insert(BandName(b));
		for (int k = 1; k <= 14; k++)
			n.Insert(KindName(k));
		for (int e = 0; e < 5; e++)
			n.Insert("ENVMUL_" + EnvName(e));
		for (int e2 = 0; e2 < 5; e2++)
			n.Insert("ENVSET_" + EnvName(e2));
		return n;
	}

	protected static void ApplyOverrides()
	{
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetInstance();
		if (!g)
			return;
		array<string> names = RowNames();
		int applied;
		foreach (int i, string name : names)
		{
			string ov = g.DCO_CtxRow(name);
			if (ov.IsEmpty())
				continue;
			array<string> parts = {};
			ov.Split(" ", parts, true);
			if (parts.Count() != DCO_ECtxParam.COUNT)
			{
				DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.WARN, "-", string.Format("ctx_profile_bad row=%1 values=%2 need=%3 action=keep_builtin", name, parts.Count(), DCO_ECtxParam.COUNT));
				continue;
			}
			array<float> row = R(ov);
			if (i < 19)
				s_aRows[i] = row;
			else if (i < 24)
				s_aEnvMul[i - 19] = row;
			else
				s_aEnvSet[i - 24] = row;
			applied++;
			DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", string.Format("ctx_profile_override row=%1 values=%2", name, ov));
		}
		DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", string.Format("ctx_profile_ready rows=%1 overrides=%2", names.Count(), applied));
	}

	protected static array<float> R(string row)
	{
		array<string> parts = {};
		row.Split(" ", parts, true);
		array<float> a = {};
		foreach (string p : parts)
			a.Insert(p.ToFloat());
		while (a.Count() < DCO_ECtxParam.COUNT)
			a.Insert(0);
		return a;
	}

	protected static array<float> Fill(float v)
	{
		array<float> a = {};
		for (int i = 0; i < DCO_ECtxParam.COUNT; i++)
			a.Insert(v);
		return a;
	}

	protected static void Tables()
	{
		if (s_aRows)
			return;
		s_aRows = {};
		s_aRows.Insert(R("2 0.2 1.0 0.1 0 1 0 3 3 2 0 0 0 0 0 0 2.0 0.3"));
		s_aRows.Insert(R("1 0.4 1.0 0.3 1 3 5 10 10 1 1 0.1 1 0 0 0 1.5 0.5"));
		s_aRows.Insert(R("1 0.8 1.0 0.4 0 3 15 25 20 1 2 0.2 1 0 0 0 1.0 0.7"));
		s_aRows.Insert(R("0 1.5 1.0 0.6 0 2 15 30 30 1 2 0.4 1 0 0 0 0.6 1.0"));
		s_aRows.Insert(R("3 2.0 1.0 0.0 0 0 0 0 30 0 3 0.3 1 0 0 0 0.3 1.2"));

		s_aRows.Insert(R("2 0.2 1.0 0.1 1 4 3 8 3 0 1 0 1 0 0 0 2.0 0.3"));
		s_aRows.Insert(R("1 0.4 0.95 0.7 1 2 5 10 5 1 2 0 1 0 0 1 1.2 0.5"));
		s_aRows.Insert(R("1 0.3 1.0 0.3 1 3 5 15 8 1 1 0 1 1 0 1 1.7 0.4"));
		s_aRows.Insert(R("1 0.3 0.9 0.4 0 3 3 8 8 1 1 0.2 1 0 1 0 1.6 0.4"));
		s_aRows.Insert(R("2 0.2 1.0 0.2 1 7 3 8 3 0 1 0 1 1 0 0 2.0 0.3"));
		s_aRows.Insert(R("2 0.3 1.0 0.5 1 1 2 10 15 2 0 0.5 0 0 1 1 2.0 0.3"));
		s_aRows.Insert(R("1 0.4 1.0 0.4 1 8 0 5 3 0 0 0 1 0 0 0 1.0 0.5"));
		s_aRows.Insert(R("1 0.4 1.0 0.4 1 8 0 5 3 0 0 0 1 0 0 0 1.0 0.5"));
		s_aRows.Insert(R("1 0.4 1.0 0.4 1 8 0 8 5 1 1 0 1 1 0 0 1.1 0.5"));
		s_aRows.Insert(R("1 0.4 0.95 0.3 0 8 0 6 5 0 1 0 1 0 1 0 1.1 0.5"));
		s_aRows.Insert(R("1 0.4 0.95 0.8 1 9 5 15 10 2 2 0.1 0 0 0 1 1.6 0.4"));
		s_aRows.Insert(R("1 0.4 0.95 0.8 1 9 5 12 10 2 2 0.3 0 0 0 1 1.6 0.4"));
		s_aRows.Insert(R("1 0.4 0.95 0.7 1 9 5 15 10 2 2 0.2 0 0 0 1 1.5 0.4"));
		s_aRows.Insert(R("1 0.4 0.9 0.8 0 2 5 15 10 1 2 0.2 0 0 0 1 1.5 0.4"));

		s_aEnvMul = {};
		s_aEnvSet = {};
		for (int e = 0; e < 5; e++)
		{
			s_aEnvMul.Insert(Fill(1.0));
			s_aEnvSet.Insert(Fill(-1.0));
		}
		s_aEnvMul[DCO_EEnv.INDOOR][DCO_ECtxParam.BMIN] = 0.5;
		s_aEnvMul[DCO_EEnv.INDOOR][DCO_ECtxParam.BMAX] = 0.5;
		s_aEnvMul[DCO_EEnv.INDOOR][DCO_ECtxParam.CMAX] = 0.5;
		s_aEnvMul[DCO_EEnv.INDOOR][DCO_ECtxParam.ACC] = 0.9;
		s_aEnvMul[DCO_EEnv.INDOOR][DCO_ECtxParam.SUP] = 1.3;
		s_aEnvSet[DCO_EEnv.INDOOR][DCO_ECtxParam.PACE] = 0;
		s_aEnvSet[DCO_EEnv.INDOOR][DCO_ECtxParam.PEEK] = 1;

		s_aEnvMul[DCO_EEnv.URBAN][DCO_ECtxParam.CMAX] = 0.7;
		s_aEnvSet[DCO_EEnv.URBAN][DCO_ECtxParam.C2C] = 1;
		s_aEnvSet[DCO_EEnv.URBAN][DCO_ECtxParam.SMOKE] = 1;
		s_aEnvSet[DCO_EEnv.URBAN][DCO_ECtxParam.PEEK] = 1;

		s_aEnvMul[DCO_EEnv.FOREST][DCO_ECtxParam.BMIN] = 0.7;
		s_aEnvMul[DCO_EEnv.FOREST][DCO_ECtxParam.BMAX] = 0.7;
		s_aEnvMul[DCO_EEnv.FOREST][DCO_ECtxParam.ACC] = 0.9;
		s_aEnvSet[DCO_EEnv.FOREST][DCO_ECtxParam.CONC] = 1;
		s_aEnvSet[DCO_EEnv.FOREST][DCO_ECtxParam.FRAG] = 0;

		s_aEnvMul[DCO_EEnv.TRENCH][DCO_ECtxParam.BMIN] = 0.6;
		s_aEnvMul[DCO_EEnv.TRENCH][DCO_ECtxParam.BMAX] = 0.6;
		s_aEnvSet[DCO_EEnv.TRENCH][DCO_ECtxParam.PACE] = 0;
		s_aEnvSet[DCO_EEnv.TRENCH][DCO_ECtxParam.FRAG] = 1;

		s_aEnvMul[DCO_EEnv.OPEN][DCO_ECtxParam.CMAX] = 1.5;
		s_aEnvMul[DCO_EEnv.OPEN][DCO_ECtxParam.CRAWL] = 1.5;
		s_aEnvMul[DCO_EEnv.OPEN][DCO_ECtxParam.BMAX] = 1.2;
		s_aEnvMul[DCO_EEnv.OPEN][DCO_ECtxParam.ACC] = 1.1;
		s_aEnvSet[DCO_EEnv.OPEN][DCO_ECtxParam.SMOKE] = 1;
		ApplyOverrides();
	}
}
