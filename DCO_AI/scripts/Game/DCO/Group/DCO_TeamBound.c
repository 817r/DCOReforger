class DCO_TeamBoundState : Managed
{
	int m_iTurn = -1;
	float m_fTurnStart;
	bool m_bTurnMoved;
	float m_fWaitStart = -1;
	float m_fNextEval;
	int m_iFm;
	int m_iBase = -1;
	int m_iMan = -1;
	vector m_vFlank;
	vector m_vEnemy;
	float m_fEnemySeen;
	float m_fFmGateAt = -1;
	SCR_AIGroupFireteam m_BaseFt;
	SCR_AIGroupFireteam m_ManFt;
	SCR_AIGroupFireteam m_TurnFt;
	ref array<SCR_AIGroupFireteam> m_aTeams = {};
	ref map<int, float> m_mLastEnd = new map<int, float>();
}

class DCO_TeamBound
{
	protected static ref map<SCR_AIGroupFireteamManager, ref DCO_TeamBoundState> s_mState;
	protected static ref map<AIAgent, vector> s_mPos;
	protected static ref map<AIAgent, float> s_mMovedAt;

	static bool On(SCR_AIGroup group)
	{
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetFor(group);
		return g && g.DCO_TuneF("tb.on", 1.0) > 0;
	}

	protected static float T(SCR_AIGroup group, string key, float def)
	{
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetFor(group);
		if (!g)
			return def;
		return g.DCO_TuneF(key, def);
	}

	static bool Turn(SCR_AIGroupFireteamManager mgr, SCR_AIGroup group, array<ref SCR_AIGroupFireteam> fireteams, SCR_AIGroupFireteam mine, out bool outBounding)
	{
		outBounding = false;
		if (!s_mState)
		{
			s_mState = new map<SCR_AIGroupFireteamManager, ref DCO_TeamBoundState>();
			s_mPos = new map<AIAgent, vector>();
			s_mMovedAt = new map<AIAgent, float>();
		}
		if (s_mState.Count() > 400)
			s_mState.Clear();
		if (s_mPos.Count() > 3000)
		{
			s_mPos.Clear();
			s_mMovedAt.Clear();
		}

		DCO_TeamBoundState st = s_mState.Get(mgr);
		if (!st)
		{
			st = new DCO_TeamBoundState();
			s_mState.Set(mgr, st);
		}
		float now = GetGame().GetWorld().GetWorldTime() * 0.001;
		if (now >= st.m_fNextEval)
		{
			st.m_fNextEval = now + 0.5;
			st.m_aTeams.Clear();
			foreach (SCR_AIGroupFireteam ft : fireteams)
			{
				if (ft && ft.Type() == SCR_AIGroupFireteam && AliveCount(ft) > 0)
					st.m_aTeams.Insert(ft);
			}
			if (st.m_iTurn >= 0)
				st.m_iTurn = st.m_aTeams.Find(st.m_TurnFt);
			if (st.m_iFm != 0)
			{
				st.m_iBase = st.m_aTeams.Find(st.m_BaseFt);
				st.m_iMan = st.m_aTeams.Find(st.m_ManFt);
				if (st.m_iBase < 0 || st.m_iMan < 0 || st.m_aTeams.Count() < 2)
					FmEnd(group, st, "team_lost");
			}
			if (st.m_aTeams.Count() >= 2)
				Eval(group, st, st.m_aTeams, now);
		}
		int myIndex = st.m_aTeams.Find(mine);
		if (st.m_aTeams.Count() < 2 || myIndex < 0)
			return true;
		outBounding = true;
		return st.m_iTurn == myIndex || st.m_iTurn == -2;
	}

	protected static int AliveCount(SCR_AIGroupFireteam ft)
	{
		array<AIAgent> agents = {};
		ft.GetMembers(agents);
		int n;
		foreach (AIAgent a : agents)
		{
			if (a && DCO_Util.IsAlive(a.GetControlledEntity()))
				n++;
		}
		return n;
	}

	protected static void Sample(SCR_AIGroupFireteam ft, float now, float setS, float fireS, out bool moving, out bool fired, out bool enemy, out vector centroid, out vector enemyPos, out bool mg, float setFrac)
	{
		moving = false;
		fired = false;
		enemy = false;
		mg = false;
		centroid = vector.Zero;
		enemyPos = vector.Zero;
		int alive;
		int movers;
		float bestEnemy = float.MAX;
		array<AIAgent> agents = {};
		ft.GetMembers(agents);
		float nowMs = now * 1000.0;
		foreach (AIAgent a : agents)
		{
			if (!a)
				continue;
			IEntity ent = a.GetControlledEntity();
			if (!DCO_Util.IsAlive(ent))
				continue;
			vector p = ent.GetOrigin();
			centroid = centroid + p;
			alive++;
			vector last;
			if (s_mPos.Find(a, last))
			{
				if (vector.DistanceXZ(p, last) > 0.6)
				{
					s_mMovedAt.Set(a, now);
					s_mPos.Set(a, p);
				}
			}
			else
				s_mPos.Set(a, p);
			float movedAt;
			if (s_mMovedAt.Find(a, movedAt) && now - movedAt < setS)
				movers++;
			if (nowMs - DCO_ShotCounter.LastShotAt(ent) < fireS * 1000.0)
				fired = true;
			SCR_AIUtilityComponent u = SCR_AIUtilityComponent.Cast(a.FindComponent(SCR_AIUtilityComponent));
			DCO_EngageInfo info = DCO_Engage.Info(u);
			if (info && info.m_fDistFresh >= 0)
			{
				enemy = true;
				if (info.m_fDistFresh < bestEnemy)
				{
					bestEnemy = info.m_fDistFresh;
					enemyPos = info.m_vNearest;
				}
			}
			if (u && (DCO_Roles.Get(u) & 1))
				mg = true;
		}
		if (alive > 0)
			centroid = centroid * (1.0 / alive);
		moving = movers > (int)(alive * setFrac);
	}

	protected static void Eval(SCR_AIGroup group, DCO_TeamBoundState st, array<SCR_AIGroupFireteam> teams, float now)
	{
		float setS = T(group, "tb.set_s", 1.5);
		float fireS = T(group, "tb.fire_s", 4.0);
		float setFrac = T(group, "tb.set_frac", 0.34);
		int n = teams.Count();
		array<bool> moving = {};
		array<bool> fired = {};
		array<vector> cents = {};
		array<bool> mgs = {};
		bool enemy = false;
		vector enemyPos = vector.Zero;
		foreach (SCR_AIGroupFireteam ft : teams)
		{
			bool mv;
			bool fr;
			bool en;
			vector cen;
			vector ep;
			bool hasMg;
			Sample(ft, now, setS, fireS, mv, fr, en, cen, ep, hasMg, setFrac);
			moving.Insert(mv);
			fired.Insert(fr);
			cents.Insert(cen);
			mgs.Insert(hasMg);
			if (en)
			{
				enemy = true;
				if (enemyPos == vector.Zero)
					enemyPos = ep;
			}
		}
		if (FmEval(group, st, cents, mgs, fired, enemy, enemyPos, now))
			return;

		if (st.m_iTurn >= n)
			st.m_iTurn = -1;
		if (st.m_iTurn >= 0)
		{
			int t = st.m_iTurn;
			string endWhy;
			if (moving[t])
			{
				st.m_bTurnMoved = true;
				if (now - st.m_fTurnStart > T(group, "tb.bound_max_s", 12.0))
					endWhy = "timeout";
			}
			else if (st.m_bTurnMoved)
				endWhy = "set";
			else if (now - st.m_fTurnStart > T(group, "tb.idle_s", 4.0))
				endWhy = "idle";
			if (endWhy.IsEmpty())
				return;
			st.m_mLastEnd.Set(t, now);
			st.m_iTurn = -1;
			st.m_fWaitStart = now;
			DCO_Perf.Count("tb:end_" + endWhy);
			if (DCO_Log.On(DCO_ELogChannel.SQUAD, DCO_ELogLevel.DEBUG))
				DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.DEBUG, DCO_Log.IdGroup(group), string.Format("tb_end grp=%1 team=%2 why=%3 bound_s=%4", DCO_Log.IdGroup(group), t, endWhy, (now - st.m_fTurnStart).ToString(-1, 1)));
		}

		int pick = -1;
		float oldest = float.MAX;
		bool mgOw = enemy && n > 1 && T(group, "tb.mg_ow", 1.0) > 0;
		float mgLeash = T(group, "tb.mg_leash_m", 40.0);
		bool mgHeld = false;
		for (int i = 0; i < n; i++)
		{
			if (st.m_iFm == 1 && i != st.m_iMan)
				continue;
			if (mgOw && mgs[i] && NearestTeam(cents, i) <= mgLeash)
			{
				mgHeld = true;
				continue;
			}
			float lastEnd = 0;
			st.m_mLastEnd.Find(i, lastEnd);
			if (lastEnd < oldest)
			{
				oldest = lastEnd;
				pick = i;
			}
		}
		if (mgHeld)
			DCO_Perf.Count("tb:mg_hold");
		if (pick < 0)
			return;

		bool othersSet = true;
		bool cover = false;
		for (int j = 0; j < n; j++)
		{
			if (j == pick)
				continue;
			if (moving[j])
				othersSet = false;
			if (fired[j])
				cover = true;
		}
		if (st.m_fWaitStart < 0)
			st.m_fWaitStart = now;
		float waited = now - st.m_fWaitStart;
		string why;
		if (othersSet && (!enemy || cover))
		{
			why = "set_no_enemy";
			if (enemy)
				why = "set_covered";
		}
		else if (waited > T(group, "tb.deadlock_s", 10.0))
			why = "deadlock";
		if (why.IsEmpty())
		{
			if (!othersSet)
				DCO_Perf.Count("tb:wait_set");
			else
				DCO_Perf.Count("tb:wait_cover");
			return;
		}
		st.m_iTurn = pick;
		st.m_TurnFt = teams[pick];
		st.m_fTurnStart = now;
		st.m_bTurnMoved = false;
		st.m_fWaitStart = -1;
		if (enemy && cover)
			Callout(teams[pick]);
		DCO_Perf.Count("tb:turn_" + why);
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(group), string.Format("tb_turn grp=%1 team=%2 why=%3 waited_s=%4 teams=%5 enemy=%6 cover=%7",
			DCO_Log.IdGroup(group), pick, why, waited.ToString(-1, 1), n, DCO_Util.B01(enemy), DCO_Util.B01(cover)));
	}

	protected static void Callout(SCR_AIGroupFireteam ft)
	{
		if (!ft)
			return;
		array<AIAgent> agents = {};
		ft.GetMembers(agents);
		foreach (AIAgent a : agents)
		{
			if (!a || !DCO_Util.IsAlive(a.GetControlledEntity()))
				continue;
			SCR_AIUtilityComponent u = SCR_AIUtilityComponent.Cast(a.FindComponent(SCR_AIUtilityComponent));
			if (u)
			{
				DCO_Opp.BoundCallout(u);
				DCO_Perf.Count("tb:callout");
			}
			return;
		}
	}

	static bool Attacker(SCR_AIGroup group)
	{
		SCR_AIGroupUtilityComponent gu = group.GetGroupUtilityComponent();
		if (gu && gu.DCO_GetPosture() == DCO_GroupTactics.AGGRESIVE)
			return true;
		if (SCR_SearchAndDestroyWaypoint.Cast(group.GetCurrentWaypoint()))
			return true;
		DCO_GroupUtilityComponent g = DCO_GroupUtilityComponent.Cast(group.FindComponent(DCO_GroupUtilityComponent));
		if (!g)
			return false;
		DCO_EGroupTask t = g.GetTask();
		return t == DCO_EGroupTask.ATTACK || t == DCO_EGroupTask.FLANK || t == DCO_EGroupTask.REINFORCE;
	}

	protected static float NearestTeam(array<vector> cents, int i)
	{
		float best = float.MAX;
		foreach (int j, vector c : cents)
		{
			if (j == i)
				continue;
			float d = vector.DistanceXZ(c, cents[i]);
			if (d < best)
				best = d;
		}
		return best;
	}

	protected static void FmEnd(SCR_AIGroup group, DCO_TeamBoundState st, string why)
	{
		if (st.m_iTurn == -2)
			st.m_iTurn = -1;
		st.m_iFm = 0;
		st.m_fFmGateAt = -1;
		st.m_BaseFt = null;
		st.m_ManFt = null;
		DCO_Perf.Count("sfm:end_" + why);
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(group), string.Format("sfm_end grp=%1 why=%2", DCO_Log.IdGroup(group), why));
	}

	protected static bool FmEval(SCR_AIGroup group, DCO_TeamBoundState st, array<vector> cents, array<bool> mgs, array<bool> fired, bool enemy, vector enemyPos, float now)
	{
		if (T(group, "tb.fm", 1.0) <= 0)
		{
			if (st.m_iFm != 0)
				FmEnd(group, st, "off");
			return false;
		}
		int n = cents.Count();
		if (enemy && enemyPos != vector.Zero)
		{
			st.m_vEnemy = enemyPos;
			st.m_fEnemySeen = now;
		}
		if (st.m_iFm == 0)
		{
			if (!enemy || n < 2 || st.m_vEnemy == vector.Zero || !Attacker(group))
				return false;
			int base = 0;
			for (int i = 0; i < n; i++)
			{
				if (mgs[i])
				{
					base = i;
					break;
				}
			}
			int man = 1;
			if (base != 0)
				man = 0;
			float d = vector.DistanceXZ(cents[base], st.m_vEnemy);
			if (d < T(group, "tb.fm_min_m", 50.0) || d > T(group, "tb.fm_max_m", 300.0))
				return false;
			vector toE = st.m_vEnemy - cents[base];
			toE[1] = 0;
			toE.Normalize();
			vector perp = Vector(toE[2], 0, -toE[0]);
			float side = 1.0;
			if (vector.Dot(cents[man] - cents[base], perp) < 0)
				side = -1.0;
			float off = Math.Clamp(0.5 * d, 30.0, 80.0);
			st.m_vFlank = DCO_Util.Surface(st.m_vEnemy + perp * (side * off) - toE * 10.0);
			st.m_iFm = 1;
			st.m_iBase = base;
			st.m_iMan = man;
			st.m_BaseFt = st.m_aTeams[base];
			st.m_ManFt = st.m_aTeams[man];
			if (st.m_iTurn >= 0 && st.m_iTurn != man)
				st.m_iTurn = -1;
			DCO_Perf.Count("sfm:start");
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(group), string.Format("sfm_start grp=%1 base=%2 man=%3 base_mg=%4 dist=%5 side=%6 flank=%7 enemy=%8",
				DCO_Log.IdGroup(group), base, man, DCO_Util.B01(mgs[base]), Math.Round(d), side, DCO_Util.XZ(st.m_vFlank), DCO_Util.XZ(st.m_vEnemy)));
			return false;
		}
		if (now - st.m_fEnemySeen > T(group, "tb.fm_lost_s", 30.0))
		{
			FmEnd(group, st, "lost");
			return false;
		}
		if (st.m_iMan >= n || st.m_iBase >= n)
		{
			FmEnd(group, st, "team_lost");
			return false;
		}
		if (st.m_iFm == 1)
		{
			float manD = vector.DistanceXZ(cents[st.m_iMan], st.m_vEnemy);
			if (manD <= T(group, "tb.fm_assault_m", 40.0))
			{
				if (T(group, "tb.fm_gate", 1.0) > 0)
				{
					if (manD < T(group, "tb.fm_close_m", 15.0))
					{
						st.m_iFm = 2;
						st.m_iTurn = -2;
						DCO_Perf.Count("sfm:close");
						DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(group), string.Format("sfm_close grp=%1 man_dist=%2", DCO_Log.IdGroup(group), Math.Round(manD)));
						return true;
					}
					if (!fired[st.m_iBase])
					{
						if (st.m_fFmGateAt < 0)
						{
							st.m_fFmGateAt = now;
							DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(group), string.Format("sfm_gate_wait grp=%1 man_dist=%2 wait_max_s=%3", DCO_Log.IdGroup(group), Math.Round(manD), Math.Round(T(group, "tb.fm_gate_wait_s", 15.0))));
						}
						if (now - st.m_fFmGateAt < T(group, "tb.fm_gate_wait_s", 15.0))
						{
							DCO_Perf.Count("sfm:assault_wait_bof");
							return false;
						}
						DCO_Perf.Count("sfm:assault_gate_timeout");
						DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(group), string.Format("sfm_gate_timeout grp=%1 man_dist=%2 wait_s=%3", DCO_Log.IdGroup(group), Math.Round(manD), Math.Round(now - st.m_fFmGateAt)));
					}
				}
				st.m_fFmGateAt = -1;
				st.m_iFm = 2;
				st.m_iTurn = -2;
				DCO_Perf.Count("sfm:assault");
				if (st.m_iMan < st.m_aTeams.Count())
					Callout(st.m_aTeams[st.m_iMan]);
				DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(group), string.Format("sfm_assault grp=%1 man_dist=%2 bof=%3", DCO_Log.IdGroup(group), Math.Round(manD), DCO_Util.B01(fired[st.m_iBase])));
				return true;
			}
			st.m_fFmGateAt = -1;
			return false;
		}
		st.m_iTurn = -2;
		return true;
	}

	static bool FlankFor(SCR_AIUtilityComponent u, out vector flank)
	{
		flank = vector.Zero;
		if (!s_mState || !u || !u.GetAIAgent())
			return false;
		SCR_AIGroup group = SCR_AIGroup.Cast(u.GetAIAgent().GetParentGroup());
		if (!group || !group.GetGroupUtilityComponent())
			return false;
		SCR_AIGroupFireteamManager mgr = group.GetGroupUtilityComponent().m_FireteamMgr;
		if (!mgr)
			return false;
		DCO_TeamBoundState st = s_mState.Get(mgr);
		if (!st || st.m_iFm != 1)
			return false;
		if (st.m_aTeams.Find(mgr.FindFireteam(u.GetAIAgent())) != st.m_iMan)
			return false;
		flank = st.m_vFlank;
		return true;
	}

	static bool OverwatchFor(SCR_AIUtilityComponent u)
	{
		if (!s_mState || !u || !u.GetAIAgent())
			return false;
		SCR_AIGroup group = SCR_AIGroup.Cast(u.GetAIAgent().GetParentGroup());
		if (!group || !group.GetGroupUtilityComponent() || !On(group))
			return false;
		SCR_AIGroupFireteamManager mgr = group.GetGroupUtilityComponent().m_FireteamMgr;
		if (!mgr)
			return false;
		return Overwatching(mgr, mgr.DCO_Fireteams(), mgr.FindFireteam(u.GetAIAgent()));
	}

	static bool Overwatching(SCR_AIGroupFireteamManager mgr, array<ref SCR_AIGroupFireteam> fireteams, SCR_AIGroupFireteam mine)
	{
		if (!s_mState || !mgr || !mine)
			return false;
		DCO_TeamBoundState st = s_mState.Get(mgr);
		if (!st || st.m_iTurn < 0 || !st.m_bTurnMoved)
			return false;
		int myIndex = st.m_aTeams.Find(mine);
		return myIndex >= 0 && myIndex != st.m_iTurn;
	}
}
