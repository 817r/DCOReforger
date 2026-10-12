class DCO_LearnDefPos
{
	string m_sKind;
	string m_sKey;
	int m_iKills;
	bool m_bEarlyBreach;
}

class DCO_LearnDefEpisode
{
	CMD_AICommanderObjectiveComponent m_Obj;
	string m_sFaction;
	float m_fStart;
	float m_fLastContact;
	vector m_vAttackSum;
	vector m_vVehicleSum;
	int m_iBreachSector = -1;
	float m_fBreachAt = -1;
	bool m_bVsPlayer;
	int m_iOurDead;
	int m_iEnemyDead;
	ref map<string, ref DCO_LearnDefPos> m_mPos = new map<string, ref DCO_LearnDefPos>();
	ref map<AIGroup, string> m_mGroupKey = new map<AIGroup, string>();
	ref map<IEntity, string> m_mUnitKey = new map<IEntity, string>();
}

class DCO_LearnDefense
{
	protected static ref DCO_LearnDefense s_Inst;

	protected ref map<string, vector> m_mStat = new map<string, vector>();
	protected ref map<string, ref DCO_LearnDefEpisode> m_mEp = new map<string, ref DCO_LearnDefEpisode>();
	protected ref map<string, bool> m_mSummary = new map<string, bool>();
	protected ref map<AIGroup, string> m_mPlaced = new map<AIGroup, string>();
	protected string m_sFile;
	protected int m_iSessions;
	protected float m_fNextSave;
	protected bool m_bDirty;
	protected bool m_bKillHook;
	protected BaseWorld m_World;

	static DCO_LearnDefense Get()
	{
		if (!Replication.IsServer())
			return null;
		if (s_Inst && s_Inst.m_World != GetGame().GetWorld())
		{
			SaveAll();
			Event("learn_world_switch store=defense");
			s_Inst = null;
		}
		if (!s_Inst)
		{
			s_Inst = new DCO_LearnDefense();
			s_Inst.m_World = GetGame().GetWorld();
			s_Inst.Load();
		}
		return s_Inst;
	}

	static void SaveAll()
	{
		if (s_Inst)
			s_Inst.Save("end");
	}

	static int ResetAll()
	{
		DCO_LearnDefense inst = Get();
		if (!inst)
			return 0;
		int n = inst.m_mStat.Count() + inst.m_mEp.Count();
		inst.m_mStat.Clear();
		inst.m_mEp.Clear();
		inst.m_mSummary.Clear();
		inst.m_bDirty = true;
		return n;
	}

	static void Event(string line)
	{
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", line);
	}

	static string SectorName(int s)
	{
		switch (s)
		{
			case 0: return "N";
			case 1: return "NE";
			case 2: return "E";
			case 3: return "SE";
			case 4: return "S";
			case 5: return "SW";
			case 6: return "W";
			case 7: return "NW";
		}
		return "-";
	}

	static int SectorOfDir(vector dir)
	{
		if (dir[0] * dir[0] + dir[2] * dir[2] < 0.0001)
			return -1;
		float deg = Math.Atan2(dir[0], dir[2]) * Math.RAD2DEG;
		if (deg < 0)
			deg += 360.0;
		int s = Math.Floor((deg + 22.5) / 45.0);
		return s % 8;
	}

	static vector SectorDir(int s)
	{
		float a = s * 45.0 * Math.DEG2RAD;
		return Vector(Math.Sin(a), 0, Math.Cos(a));
	}

	static string PosKey(vector p)
	{
		return DCO_Util.XZ(Vector(Math.Round(p[0] / 10.0) * 10.0, 0, Math.Round(p[2] / 10.0) * 10.0));
	}

	protected static string K(CMD_AICommanderObjectiveComponent obj, string fk, string vs, string kind, string key)
	{
		return obj.GetLogName() + "|" + fk + "|" + vs + "|" + kind + "|" + key;
	}

	protected float Post(string k, out float n)
	{
		vector s;
		if (!m_mStat.Find(k, s))
		{
			n = 0;
			return 0.5;
		}
		n = s[0];
		return (s[1] + 0.5 * 4.0) / (s[0] + 4.0);
	}

	protected float Mult(string k, out float n)
	{
		float p = Post(k, n);
		return 1.0 + 0.25 * Math.Clamp((p - 0.5) * 2.0, -1.0, 1.0);
	}

	protected void Add(string k, float value)
	{
		vector s;
		if (!m_mStat.Find(k, s))
			s = vector.Zero;
		s[0] = s[0] + 1.0;
		s[1] = s[1] + value;
		m_mStat.Set(k, s);
		m_bDirty = true;
	}

	static string VsKey(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj)
	{
		PlayerManager pm = GetGame().GetPlayerManager();
		FactionManager fm = GetGame().GetFactionManager();
		if (!pm || !fm || !obj)
			return "ai";
		Faction mine = fm.GetFactionByKey(cmd.GetCommanderFactionKey());
		vector objPos = obj.GetOwner().GetOrigin();
		float r = obj.GetRadius() + 600.0;
		array<int> players = {};
		pm.GetPlayers(players);
		foreach (int pid : players)
		{
			IEntity pe = pm.GetPlayerControlledEntity(pid);
			if (!pe || vector.DistanceXZ(pe.GetOrigin(), objPos) > r)
				continue;
			Faction pf = fm.GetFactionByKey(DCO_Util.FactionKeyOf(pe, true));
			if (pf && mine && !pf.IsFactionFriendly(mine))
				return "player";
		}
		return "ai";
	}

	static bool Explore(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, string kind)
	{
		if (Math.RandomFloat01() >= 0.12)
			return false;
		DCO_Perf.Count("learn_def:explore");
		Event(string.Format("learn_def_explore obj=%1 kind=%2 key=-", obj.GetLogName(), kind));
		return true;
	}

	static void LogUse(CMD_AICommanderObjectiveComponent obj, string kind, string key, float baseScore, float mult, float samples)
	{
		DCO_Perf.Count("learn_def:use:" + kind);
		Event(string.Format("learn_def_use obj=%1 kind=%2 key=%3 base=%4 mult=%5 samples=%6", obj.GetLogName(), kind, key, baseScore.ToString(-1, 2), mult.ToString(-1, 2), Math.Round(samples)));
	}

	static float SectorMult(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, float mathAngle, out float samples)
	{
		samples = 0;
		DCO_LearnDefense L = Get();
		if (!L || !obj)
			return 1.0;
		int s = SectorOfDir(Vector(Math.Cos(mathAngle), 0, Math.Sin(mathAngle)));
		if (s < 0)
			return 1.0;
		string fk = cmd.GetCommanderFactionKey();
		string vs = VsKey(cmd, obj);
		float na, nb;
		float ma = L.Mult(K(obj, fk, vs, "attack", s.ToString()), na);
		float mb = L.Mult(K(obj, fk, vs, "breach", s.ToString()), nb);
		samples = na + nb;
		return (ma + mb) * 0.5;
	}

	protected bool TopSector(CMD_AICommanderObjectiveComponent obj, string fk, string vs, string kind, out int best, out float bestPost, out float n)
	{
		best = -1;
		bestPost = 0;
		n = 0;
		for (int s = 0; s < 8; s++)
		{
			float sn = 0;
			float p = Post(K(obj, fk, vs, kind, s.ToString()), sn);
			if (sn > 0 && p > bestPost)
			{
				bestPost = p;
				best = s;
				n = sn;
			}
		}
		return best >= 0;
	}

	static float ThreatAngle(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, float fallback)
	{
		DCO_LearnDefense L = Get();
		if (!L || !obj)
			return fallback;
		int s;
		float p, n;
		if (!L.TopSector(obj, cmd.GetCommanderFactionKey(), VsKey(cmd, obj), "attack", s, p, n) || p < 0.6 || n < 2)
			return fallback;
		vector d = SectorDir(s);
		return Math.Atan2(d[2], d[0]);
	}

	static vector MainThreatDir(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, out string source)
	{
		source = "nearest_fallback";
		DCO_LearnDefense L = Get();
		if (!obj)
			return vector.Zero;
		vector objPos = obj.GetOwner().GetOrigin();
		string fk = cmd.GetCommanderFactionKey();
		if (L)
		{
			int s;
			float p, n;
			if (L.TopSector(obj, fk, VsKey(cmd, obj), "attack", s, p, n) && p >= 0.6 && n >= 2)
			{
				source = "learned";
				return SectorDir(s);
			}
		}
		vector sum = vector.Zero;
		CMD_ThreatResponseComponent tr = cmd.GetThreatResponseComponent();
		float now = DCO_Util.NowS();
		if (tr)
		{
			foreach (CMD_ThreatEntry t : tr.GetThreats())
			{
				if (!t || now - t.m_fLastUpdateTime > 180.0 || vector.DistanceXZ(t.m_vBelievedPos, objPos) > obj.GetRadius() + 1500.0)
					continue;
				sum = sum + DCO_Specialist.Flat(t.m_vBelievedPos - objPos) * Math.Max(1, t.m_iEstimatedEnemyCount);
			}
		}
		if (sum.LengthSq() > 0.001)
		{
			source = "threat";
			return DCO_Specialist.Flat(sum);
		}
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (mgr)
		{
			float bestD = float.MAX;
			vector bestDir = vector.Zero;
			foreach (CMD_AICommanderObjectiveComponent other : mgr.m_aObjective)
			{
				if (!other || other == obj || !other.GetOwner() || other.IsCapturedBy(fk))
					continue;
				float d = vector.DistanceXZ(other.GetOwner().GetOrigin(), objPos);
				if (d < bestD)
				{
					bestD = d;
					bestDir = other.GetOwner().GetOrigin() - objPos;
				}
			}
			if (bestDir.LengthSq() > 0.001)
			{
				source = "frontline";
				return DCO_Specialist.Flat(bestDir);
			}
		}
		return vector.Zero;
	}

	static vector RoadDir(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, vector threatDir, out string source)
	{
		source = "nearest_fallback";
		DCO_LearnDefense L = Get();
		if (!obj)
			return vector.Zero;
		if (L)
		{
			int s;
			float p, n;
			if (L.TopSector(obj, cmd.GetCommanderFactionKey(), VsKey(cmd, obj), "vehicle", s, p, n) && p >= 0.6 && n >= 2)
			{
				source = "learned";
				return SectorDir(s);
			}
		}
		RoadNetworkManager roads = DCO_CommanderDefense.Roads();
		if (!roads)
			return vector.Zero;
		vector objPos = obj.GetOwner().GetOrigin();
		float r = obj.GetRadius() + 80.0;
		float bestDot = -2.0;
		vector best = vector.Zero;
		for (int i = 0; i < 8; i++)
		{
			vector dir = SectorDir(i);
			BaseRoad br;
			float rd = 1000;
			roads.GetClosestRoad(objPos + dir * r, br, rd);
			if (!br || rd > 40.0)
				continue;
			float dot = vector.Dot(dir, threatDir);
			if (threatDir == vector.Zero)
				dot = 0;
			if (dot > bestDot)
			{
				bestDot = dot;
				best = dir;
			}
		}
		if (best != vector.Zero)
			source = "road";
		return best;
	}

	static vector SpecialistDir(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, bool at, out string source)
	{
		vector threat = MainThreatDir(cmd, obj, source);
		if (!at)
			return threat;
		string rs;
		vector road = RoadDir(cmd, obj, threat, rs);
		source = rs;
		return road;
	}

	static int BestSlot(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, bool at, vector dir, notnull array<DCO_GarrisonBuilding> buildings, notnull array<int> slots, notnull array<int> used, out string key)
	{
		key = "-";
		DCO_LearnDefense L = Get();
		string kind = "mg";
		if (at)
			kind = "at";
		bool explore = Explore(cmd, obj, kind);
		string fk = cmd.GetCommanderFactionKey();
		string vs = VsKey(cmd, obj);
		int best = -1;
		float bestScore = -1;
		float bestBase = 0;
		float bestMult = 1;
		float bestN = 0;
		foreach (int rank, DCO_GarrisonBuilding b : buildings)
		{
			if (!b || used.Contains(rank) || rank >= slots.Count())
				continue;
			int si = slots[rank];
			if (si < 0 || si >= b.m_aSlots.Count())
				continue;
			DCO_GarrisonSlot slot = b.m_aSlots[si];
			if (!slot || slot.m_bUnreachable || slot.m_eType == DCO_EGarrisonSlotType.INTERIOR)
				continue;
			float dot = vector.Dot(DCO_Specialist.Flat(slot.m_vWorldDir), dir);
			if (dot < 0.3)
				continue;
			float baseScore = (dot + 1.0) * 0.5;
			string sk = PosKey(slot.m_vWorldPos);
			float n = 0;
			float m = 1.0;
			if (L && !explore)
				m = L.Mult(K(obj, fk, vs, kind, sk), n);
			float score = baseScore * m;
			if (score > bestScore)
			{
				bestScore = score;
				best = rank;
				bestBase = baseScore;
				bestMult = m;
				bestN = n;
				key = sk;
			}
		}
		if (best >= 0 && !explore)
			LogUse(obj, kind, key, bestBase, bestMult, bestN);
		return best;
	}

	static void LogPlace(CMD_AICommanderObjectiveComponent obj, AIGroup grp, bool at, string slot, vector dir, string source, vector slotPos = vector.Zero)
	{
		string role = "mg";
		if (at)
			role = "at";
		if (source == "nearest_fallback")
			DCO_Perf.Count("def_mgat:fallback");
		else
			DCO_Perf.Count("def_mgat:" + role + "_placed");
		Event(string.Format("def_mgat_place obj=%1 grp=%2 role=%3 slot=%4 facing=%5 source=%6", obj.GetLogName(), DCO_Log.IdGroup(grp), role, slot, SectorName(SectorOfDir(dir)), source));
		vector gp = slotPos;
		if (gp == vector.Zero && slot != "-")
		{
			array<string> parts = {};
			slot.Split(",", parts, true);
			if (parts.Count() == 2)
				gp = Vector(parts[0].ToFloat(), 0, parts[1].ToFloat());
		}
		if (gp == vector.Zero && grp)
			gp = grp.GetOrigin();
		DCO_Battle.Place(null, obj, grp, role, gp, Math.Atan2(dir[0], dir[2]) * Math.RAD2DEG, "-", slot, "-", source);
	}

	static vector ReservePos(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, vector basePos)
	{
		DCO_LearnDefense L = Get();
		if (!L || !obj || !cmd)
			return basePos;
		vector objPos = obj.GetOwner().GetOrigin();
		vector baseDir = DCO_Specialist.Flat(basePos - objPos);
		float r = vector.DistanceXZ(basePos, objPos);
		if (r < 1.0)
			return basePos;
		if (Explore(cmd, obj, "reserve"))
			return basePos;
		string fk = cmd.GetCommanderFactionKey();
		string vs = VsKey(cmd, obj);
		int best = -1;
		float bestScore = -1;
		float bestBase = 0;
		float bestMult = 1;
		float bestN = 0;
		for (int s = 0; s < 8; s++)
		{
			vector d = SectorDir(s);
			float sectorBase = (vector.Dot(d, baseDir) + 1.0) * 0.5;
			float n = 0;
			float m = L.Mult(K(obj, fk, vs, "breach", s.ToString()), n);
			float score = sectorBase * m;
			if (score > bestScore)
			{
				bestScore = score;
				best = s;
				bestBase = sectorBase;
				bestMult = m;
				bestN = n;
			}
		}
		if (best < 0 || best == SectorOfDir(baseDir))
			return basePos;
		LogUse(obj, "reserve", SectorName(best), bestBase, bestMult, bestN);
		return DCO_Util.Surface(objPos + SectorDir(best) * r);
	}

	static void Tick(AICommander_BaseComponent cmd, float now)
	{
		DCO_LearnDefense L = Get();
		if (L && cmd)
			L.DoTick(cmd, now);
	}

	protected void DoTick(AICommander_BaseComponent cmd, float now)
	{
		HookKills();
		string fk = cmd.GetCommanderFactionKey();
		CMD_ThreatResponseComponent tr = cmd.GetThreatResponseComponent();
		foreach (CMD_AICommanderObjectiveComponent o : cmd.DCO_AllObjectives())
		{
			if (!o || !o.GetOwner())
				continue;
			string ek = o.GetLogName() + "|" + fk;
			DCO_LearnDefEpisode ep = m_mEp.Get(ek);
			if (!o.IsCapturedBy(fk))
			{
				if (ep)
					EndEpisode(cmd, ep, false, "lost", now);
				continue;
			}
			if (!m_mSummary.Contains(ek))
			{
				m_mSummary.Set(ek, true);
				Summary(cmd, o);
			}
			vector objPos = o.GetOwner().GetOrigin();
			float R = o.GetRadius();
			int contacts = 0;
			vector sum = vector.Zero;
			vector vsum = vector.Zero;
			bool breach = false;
			vector breachPos = vector.Zero;
			if (tr)
			{
				foreach (CMD_ThreatEntry t : tr.GetThreats())
				{
					if (!t || now - t.m_fLastUpdateTime > 30.0)
						continue;
					float d = vector.DistanceXZ(t.m_vBelievedPos, objPos);
					if (d > R + 300.0)
						continue;
					contacts++;
					vector dir = DCO_Specialist.Flat(t.m_vBelievedPos - objPos);
					sum = sum + dir * Math.Max(1, t.m_iEstimatedEnemyCount);
					if (t.m_iVehicle > 0 || t.m_iArmor > 0 || t.m_bArmorSeen)
						vsum = vsum + dir;
					if (d <= R && !breach)
					{
						breach = true;
						breachPos = t.m_vBelievedPos;
					}
				}
			}
			if (!ep)
			{
				if (contacts == 0)
				{
					PlacePerimeter(cmd, o);
					continue;
				}
				ep = StartEpisode(cmd, o, fk, now);
			}
			if (contacts > 0)
			{
				ep.m_fLastContact = now;
				ep.m_vAttackSum = ep.m_vAttackSum + sum;
				ep.m_vVehicleSum = ep.m_vVehicleSum + vsum;
			}
			if (breach && ep.m_iBreachSector < 0)
			{
				ep.m_iBreachSector = SectorOfDir(breachPos - objPos);
				ep.m_fBreachAt = now;
			}
			TrackPositions(cmd, ep);
			PlacePerimeter(cmd, o);
			if (now - ep.m_fLastContact > 300.0)
				EndEpisode(cmd, ep, true, "no_contact", now);
		}
		if (now >= m_fNextSave)
		{
			m_fNextSave = now + 300.0;
			if (m_bDirty)
				Save("timer");
		}
	}

	protected DCO_LearnDefEpisode StartEpisode(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent o, string fk, float now)
	{
		DCO_LearnDefEpisode ep = new DCO_LearnDefEpisode();
		ep.m_Obj = o;
		ep.m_sFaction = fk;
		ep.m_fStart = now;
		ep.m_fLastContact = now;
		ep.m_bVsPlayer = VsKey(cmd, o) == "player";
		m_mEp.Set(o.GetLogName() + "|" + fk, ep);
		DCO_Perf.Count("learn_def:episode_start");
		return ep;
	}

	protected void TrackPositions(AICommander_BaseComponent cmd, DCO_LearnDefEpisode ep)
	{
		vector objPos = ep.m_Obj.GetOwner().GetOrigin();
		foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
		{
			if (!g || !g.GetOwner() || g.IsPlayerGroup())
				continue;
			AIGroup ag = AIGroup.Cast(g.GetOwner());
			if (!ag)
				continue;
			string gk;
			bool known = ep.m_mGroupKey.Find(ag, gk);
			bool defending = g.GetGroupObjective() == ep.m_Obj && (g.GetTask() == DCO_EGroupTask.DEFEND || g.GetTask() == DCO_EGroupTask.GARRISON || g.m_iDCODefRole != DCO_EDefRole.NONE);
			if (known)
			{
				DCO_LearnDefPos kp = ep.m_mPos.Get(gk);
				if (kp && (!defending || g.GetUnitCount() <= 0))
					kp.m_bEarlyBreach = true;
				continue;
			}
			if (!defending)
				continue;
			string kind = "perimeter";
			string key = PosKey(ag.GetOrigin());
			if (g.GetGarrisonActivity())
				kind = "garrison";
			else if (g.m_iDCODefRole == DCO_EDefRole.OP)
				kind = "op";
			else if (g.m_iDCODefRole == DCO_EDefRole.RESERVE)
			{
				kind = "reserve";
				key = SectorName(SectorOfDir(ag.GetOrigin() - objPos));
			}
			string pk = kind + "|" + key;
			ep.m_mGroupKey.Set(ag, pk);
			if (!ep.m_mPos.Contains(pk))
			{
				DCO_LearnDefPos np = new DCO_LearnDefPos();
				np.m_sKind = kind;
				np.m_sKey = key;
				ep.m_mPos.Set(pk, np);
			}
			array<AIAgent> agents = {};
			ag.GetAgents(agents);
			foreach (AIAgent a : agents)
			{
				SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
				if (!ca || !ca.m_UtilityComponent || !ca.GetControlledEntity())
					continue;
				int roles = DCO_Roles.Get(ca.m_UtilityComponent);
				string uk = "";
				if (roles & 1)
					uk = "mg";
				else if (roles & 2)
					uk = "at";
				if (uk.IsEmpty())
					continue;
				string upk = uk + "|" + PosKey(ca.GetControlledEntity().GetOrigin());
				ep.m_mUnitKey.Set(ca.GetControlledEntity(), upk);
				if (!ep.m_mPos.Contains(upk))
				{
					DCO_LearnDefPos up = new DCO_LearnDefPos();
					up.m_sKind = uk;
					up.m_sKey = PosKey(ca.GetControlledEntity().GetOrigin());
					ep.m_mPos.Set(upk, up);
				}
			}
		}
	}

	protected void PlacePerimeter(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent o)
	{
		string ok = o.GetLogName();
		foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
		{
			if (!g || !g.GetOwner() || g.IsPlayerGroup() || g.GetGroupObjective() != o || g.GetTask() != DCO_EGroupTask.DEFEND || g.GetGarrisonActivity())
				continue;
			AIGroup ag = AIGroup.Cast(g.GetOwner());
			if (!ag || g.HasState(DCO_EGroupState.IN_CONTACT))
				continue;
			string done;
			if (m_mPlaced.Find(ag, done) && done == ok)
				continue;
			if (ag.GetCurrentWaypoint() && vector.DistanceXZ(ag.GetOrigin(), ag.GetCurrentWaypoint().GetOrigin()) > 30.0)
				continue;
			m_mPlaced.Set(ag, ok);
			string srcMg;
			vector threat = MainThreatDir(cmd, o, srcMg);
			string srcAt;
			vector road = RoadDir(cmd, o, threat, srcAt);
			array<AIAgent> agents = {};
			ag.GetAgents(agents);
			bool mgDone = false;
			bool atDone = false;
			foreach (AIAgent a : agents)
			{
				SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
				if (!ca || !ca.m_UtilityComponent || !ca.GetControlledEntity())
					continue;
				int roles = DCO_Roles.Get(ca.m_UtilityComponent);
				if ((roles & 1) && !mgDone)
				{
					mgDone = true;
					if (threat == vector.Zero)
					{
						LogPlace(o, ag, false, "-", threat, "nearest_fallback");
						continue;
					}
					vector mp = DCO_Util.Surface(ag.GetOrigin() + threat * 8.0);
					DCO_Specialist.MoveIndividually(ca.m_UtilityComponent, mp);
					LogPlace(o, ag, false, PosKey(mp), threat, srcMg, mp);
				}
				else if ((roles & 2) && !atDone)
				{
					atDone = true;
					if (road == vector.Zero)
					{
						LogPlace(o, ag, true, "-", road, "nearest_fallback");
						continue;
					}
					vector apos = DCO_Util.Surface(ag.GetOrigin() + road * 8.0);
					DCO_Specialist.MoveIndividually(ca.m_UtilityComponent, apos);
					LogPlace(o, ag, true, PosKey(apos), road, srcAt, apos);
				}
			}
		}
	}

	protected void HookKills()
	{
		if (m_bKillHook)
			return;
		SCR_BaseGameMode gm = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (!gm)
			return;
		gm.GetOnControllableDestroyed().Insert(OnKill);
		m_bKillHook = true;
	}

	protected static AIGroup GroupOf(IEntity ent)
	{
		if (!ent)
			return null;
		AIControlComponent ctrl = AIControlComponent.Cast(ent.FindComponent(AIControlComponent));
		if (!ctrl || !ctrl.GetControlAIAgent())
			return null;
		return ctrl.GetControlAIAgent().GetParentGroup();
	}

	void OnKill(notnull SCR_InstigatorContextData data)
	{
		IEntity victim = data.GetVictimEntity();
		IEntity killer = data.GetKillerEntity();
		if (!victim || m_mEp.IsEmpty())
			return;
		string vf = DCO_Util.FactionKeyOf(victim, true);
		AIGroup kg = GroupOf(killer);
		foreach (string ek, DCO_LearnDefEpisode ep : m_mEp)
		{
			if (!ep || !ep.m_Obj || !ep.m_Obj.GetOwner())
				continue;
			if (vector.DistanceXZ(victim.GetOrigin(), ep.m_Obj.GetOwner().GetOrigin()) > ep.m_Obj.GetRadius() + 300.0)
				continue;
			if (vf == ep.m_sFaction)
			{
				ep.m_iOurDead++;
				string vk;
				if (ep.m_mUnitKey.Find(victim, vk) && ep.m_mPos.Contains(vk))
					ep.m_mPos.Get(vk).m_bEarlyBreach = true;
				continue;
			}
			ep.m_iEnemyDead++;
			string uk;
			if (killer && ep.m_mUnitKey.Find(killer, uk) && ep.m_mPos.Contains(uk))
				ep.m_mPos.Get(uk).m_iKills++;
			string gk;
			if (kg && ep.m_mGroupKey.Find(kg, gk) && ep.m_mPos.Contains(gk))
				ep.m_mPos.Get(gk).m_iKills++;
		}
	}

	protected void EndEpisode(AICommander_BaseComponent cmd, DCO_LearnDefEpisode ep, bool held, string why, float now)
	{
		CMD_AICommanderObjectiveComponent obj = ep.m_Obj;
		string fk = ep.m_sFaction;
		m_mEp.Remove(obj.GetLogName() + "|" + fk);
		string vs = "ai";
		if (ep.m_bVsPlayer)
			vs = "player";
		int attack = SectorOfDir(ep.m_vAttackSum);
		int vehicle = SectorOfDir(ep.m_vVehicleSum);
		for (int s = 0; s < 8; s++)
		{
			float hitA = 0;
			if (s == attack)
				hitA = 1;
			float hitB = 0;
			if (s == ep.m_iBreachSector)
				hitB = 1;
			float hitV = 0;
			if (s == vehicle)
				hitV = 1;
			if (attack >= 0)
				Add(K(obj, fk, vs, "attack", s.ToString()), hitA);
			if (ep.m_iBreachSector >= 0)
				Add(K(obj, fk, vs, "breach", s.ToString()), hitB);
			if (vehicle >= 0)
				Add(K(obj, fk, vs, "vehicle", s.ToString()), hitV);
		}
		float heldV = 0;
		if (held)
			heldV = 1;
		Add(K(obj, fk, vs, "held", "x"), heldV);
		float breachS = -1;
		if (ep.m_fBreachAt >= 0)
			breachS = ep.m_fBreachAt - ep.m_fStart;
		float exchange = 1.0 * ep.m_iEnemyDead / Math.Max(ep.m_iOurDead, 1.0);
		DCO_Perf.Count("learn_def:episode");
		Event(string.Format("learn_def_episode obj=%1 held=%2 breach=%3 attack=%4 breach_s=%5 exchange=%6 vs=%7 why=%8 dur_s=%9",
			obj.GetLogName(), DCO_Util.B01(held), SectorName(ep.m_iBreachSector), SectorName(attack), Math.Round(breachS), exchange.ToString(-1, 2), vs, why, Math.Round(now - ep.m_fStart))
			+ string.Format(" vehicle=%1 our_dead=%2 enemy_dead=%3 positions=%4", SectorName(vehicle), ep.m_iOurDead, ep.m_iEnemyDead, ep.m_mPos.Count()));
		float heldBonus = -0.1;
		if (held)
			heldBonus = 0.1;
		foreach (string pk, DCO_LearnDefPos p : ep.m_mPos)
		{
			if (!p)
				continue;
			float early = 0;
			if (p.m_bEarlyBreach)
				early = 1;
			float value = Math.Clamp(0.5 + 0.15 * p.m_iKills - 0.4 * early + heldBonus, 0.0, 1.0);
			Add(K(obj, fk, vs, p.m_sKind, p.m_sKey), value);
			Event(string.Format("learn_def_pos obj=%1 kind=%2 key=%3 value=%4 kills=%5 early_breach=%6", obj.GetLogName(), p.m_sKind, p.m_sKey, value.ToString(-1, 2), p.m_iKills, DCO_Util.B01(p.m_bEarlyBreach)));
		}
	}

	protected void Summary(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent o)
	{
		string fk = cmd.GetCommanderFactionKey();
		string vs = VsKey(cmd, o);
		int a, b, v;
		float pa, pb, pv, na, nb, nv;
		TopSector(o, fk, vs, "attack", a, pa, na);
		TopSector(o, fk, vs, "breach", b, pb, nb);
		TopSector(o, fk, vs, "vehicle", v, pv, nv);
		float nh = 0;
		float held = Post(K(o, fk, vs, "held", "x"), nh);
		string mg = BestKey(o, fk, vs, "mg");
		string at = BestKey(o, fk, vs, "at");
		Event(string.Format("learn_summary kind=defense obj=%1 vs=%2 episodes=%3 held=%4 top_attack=%5 top_breach=%6 top_vehicle=%7 mg=%8 at=%9",
			o.GetLogName(), vs, Math.Round(nh), held.ToString(-1, 2), SectorName(a), SectorName(b), SectorName(v), mg, at));
	}

	protected string BestKey(CMD_AICommanderObjectiveComponent o, string fk, string vs, string kind)
	{
		string prefix = K(o, fk, vs, kind, "");
		string best = "-";
		float bestP = 0.5;
		foreach (string k, vector s : m_mStat)
		{
			if (!k.StartsWith(prefix))
				continue;
			float p = (s[1] + 2.0) / (s[0] + 4.0);
			if (p > bestP)
			{
				bestP = p;
				best = k.Substring(prefix.Length(), k.Length() - prefix.Length());
			}
		}
		return best;
	}

	protected void Load()
	{
		string world = GetGame().GetWorldFile();
		array<string> bad = {"/", "{", "}", ".", ":", " "};
		foreach (string ch : bad)
			world.Replace(ch, "_");
		string defDir = "$profile:DCO/CommanderLearn/Defense/";
		if (DCO_TestCaseComponent.Active())
			defDir = "$profile:DCO/CommanderLearn/Defense/_test/";
		m_sFile = defDir + world + ".json";
		m_fNextSave = DCO_Util.NowS() + 300.0;
		if (!FileIO.FileExists(m_sFile))
		{
			m_iSessions = 1;
			Event(string.Format("learn_load kind=defense file=%1 entries=0 sessions=1 fresh=1", m_sFile));
			return;
		}
		SCR_JsonLoadContext ctx = new SCR_JsonLoadContext();
		array<string> keys = {};
		array<float> ns = {};
		array<float> vs = {};
		int sessions;
		if (!ctx.LoadFromFile(m_sFile) || !ctx.ReadValue("keys", keys) || !ctx.ReadValue("n", ns) || !ctx.ReadValue("v", vs) || keys.Count() != ns.Count() || keys.Count() != vs.Count())
		{
			m_iSessions = 1;
			DCO_Perf.Count("learn_def:load_fail");
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.WARN, "-", string.Format("learn_load_fail kind=defense file=%1 reason=parse", m_sFile));
			return;
		}
		if (!ctx.ReadValue("sessions", sessions))
			sessions = 0;
		m_iSessions = sessions + 1;
		bool decay = m_iSessions % 10 == 0;
		foreach (int i, string k : keys)
		{
			float n = ns[i];
			float v = vs[i];
			if (decay)
			{
				n = n * 0.5;
				v = v * 0.5;
			}
			m_mStat.Set(k, Vector(n, v, 0));
		}
		Event(string.Format("learn_load kind=defense file=%1 entries=%2 sessions=%3 decay=%4", m_sFile, keys.Count(), m_iSessions, DCO_Util.B01(decay)));
	}

	protected void Save(string why)
	{
		if (m_sFile.IsEmpty())
			return;
		FileIO.MakeDirectory("$profile:DCO");
		FileIO.MakeDirectory("$profile:DCO/CommanderLearn");
		FileIO.MakeDirectory("$profile:DCO/CommanderLearn/Defense");
		FileIO.MakeDirectory("$profile:DCO/CommanderLearn/Defense/_test");
		array<string> keys = {};
		array<float> ns = {};
		array<float> vs = {};
		foreach (string k, vector s : m_mStat)
		{
			keys.Insert(k);
			ns.Insert(s[0]);
			vs.Insert(s[1]);
		}
		SCR_JsonSaveContext ctx = new SCR_JsonSaveContext();
		ctx.WriteValue("version", 1);
		ctx.WriteValue("sessions", m_iSessions);
		ctx.WriteValue("keys", keys);
		ctx.WriteValue("n", ns);
		ctx.WriteValue("v", vs);
		bool ok = ctx.SaveToFile(m_sFile);
		m_bDirty = !ok;
		Event(string.Format("learn_save kind=defense file=%1 entries=%2 ok=%3 why=%4", m_sFile, keys.Count(), DCO_Util.B01(ok), why));
	}
}
