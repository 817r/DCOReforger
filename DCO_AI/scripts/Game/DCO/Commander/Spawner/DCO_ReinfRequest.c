class DCO_ReinfRequest
{
	int m_iId;
	int m_iKind;
	string m_sType;
	string m_sWhy;
	string m_sStyle;
	int m_iPrio;
	float m_fSize;
	int m_iGroups;
	int m_iOrdered;
	int m_iArrived;
	string m_sSpawner;
	float m_fCreated;
	int m_iBandAt;
	bool m_bCancelled;
	string m_sCancelWhy;
	vector m_vPos;
	float m_fRatio;
	float m_fThr;
	float m_fConf;
	string m_sObj;
	CMD_AICommanderObjectiveComponent m_DefObj;
	float m_fLastTrigger;
	float m_fThreatDist = 99999;
	int m_iGap;
	int m_iGapOrig;

	static string PrioName(int p)
	{
		if (p >= 2)
			return "URGENT";
		if (p == 1)
			return "PRIORITY";
		return "ROUTINE";
	}

	static float PrioMul(int p)
	{
		if (p >= 2)
			return 0.5;
		if (p == 1)
			return 1.0;
		return 2.0;
	}

	string KindName()
	{
		if (m_iKind == 2)
			return "capability";
		if (m_iKind == 3)
			return "defense";
		return "strength";
	}

	bool IsOpen()
	{
		return !m_bCancelled && (m_iOrdered < m_iGroups || m_iArrived < m_iOrdered);
	}

	string Text()
	{
		string t = KindName();
		if (m_iKind == 2)
			t = m_sType;
		t.ToUpper();
		return string.Format("%1 %2 · %3 %4", t, PrioName(m_iPrio), m_iGroups, DCO_CommanderSpawnerComponent.Plural(m_iGroups, "group", "groups"));
	}
}

class DCO_ReinfBoard
{
	protected static int s_iNextId;
	protected static ref array<DCO_ReinfBoard> s_aAll;
	protected static array<DCO_ReinfBoard> GetAll()
	{
		if (!s_aAll)
			s_aAll = {};
		return s_aAll;
	}

	string m_sUID;
	float m_fPeak;
	float m_fNow;
	float m_fCE = 1;
	int m_iBand;
	int m_iPendBand;
	float m_fPendSince = -1;
	string m_sStyle = "US";
	int m_iFilled;
	float m_fNextTick;
	float m_fRatioBelowSince = -1;
	float m_fRatioAboveSince = -1;
	bool m_bRatioActive;
	float m_fCulminatedAt = -1000;
	float m_fFilledAt = -1000;
	ref DCO_ReinfRequest m_Strength;
	ref DCO_ReinfRequest m_Capability;
	protected ref map<DCO_GroupUtilityComponent, float> m_mCounted = new map<DCO_GroupUtilityComponent, float>();
	protected ref map<DCO_GroupUtilityComponent, IEntity> m_mLeader = new map<DCO_GroupUtilityComponent, IEntity>();
	protected ref map<DCO_GroupUtilityComponent, float> m_mLeaderLostAt = new map<DCO_GroupUtilityComponent, float>();
	protected ref map<DCO_GroupUtilityComponent, float> m_mLeaderLostDur = new map<DCO_GroupUtilityComponent, float>();
	protected ref map<string, float> m_mSuppLog = new map<string, float>();

	void DCO_ReinfBoard(string uid)
	{
		m_sUID = uid;
		GetAll().Insert(this);
	}

	void ~DCO_ReinfBoard()
	{
		if (GetAll())
			GetAll().RemoveItem(this);
	}

	static string BandName(int b)
	{
		if (b == 0)
			return "GREEN";
		if (b == 1)
			return "AMBER";
		if (b == 2)
			return "RED";
		return "BLACK";
	}

	static string CensusLine()
	{
		string line = "CENSUS reinf";
		int n;
		foreach (DCO_ReinfBoard b : GetAll())
		{
			if (!b)
				continue;
			int open;
			if (b.m_Strength && b.m_Strength.IsOpen())
				open++;
			if (b.m_Capability && b.m_Capability.IsOpen())
				open++;
			line += string.Format(" cmd=%1 ce=%2 band=%3 open=%4 filled=%5;", b.m_sUID, Math.Round(b.m_fCE * 100), BandName(b.m_iBand), open, b.m_iFilled);
			n++;
		}
		if (n == 0)
			line += " none";
		return line;
	}

	string StatusText()
	{
		string band = string.Format("%1 %2%", BandName(m_iBand), Math.Round(m_fCE * 100));
		string t;
		if (m_Strength && m_Strength.IsOpen())
			t = m_Strength.Text();
		if (m_Capability && m_Capability.IsOpen())
		{
			if (!t.IsEmpty())
				t += " + ";
			t += m_Capability.Text();
		}
		if (t.IsEmpty())
			return "No request (" + band + ")";
		return "Request: " + t + " (" + band + ")";
	}

	protected static float T(AICommander_BaseComponent cmd, string key, float def)
	{
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetForCommander(cmd);
		if (!g)
			return def;
		return g.DCO_TuneF(key, def);
	}

	protected void Suppress(string why, float now)
	{
		DCO_Perf.Count("reinf:suppressed_" + why);
		float last;
		if (m_mSuppLog.Find(why, last) && now - last < 60.0)
			return;
		m_mSuppLog.Set(why, now);
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("reinf_suppressed cmd=%1 why=%2 band=%3 ce=%4", m_sUID, why, BandName(m_iBand), m_fCE.ToString(-1, 2)));
	}

	protected float UnitWeight(IEntity e, bool enemyArmor)
	{
		ChimeraCharacter ch = ChimeraCharacter.Cast(e);
		if (!ch)
			return 0;
		CharacterControllerComponent ctrl = ch.GetCharacterController();
		if (!ctrl || ctrl.GetLifeState() == ECharacterLifeState.DEAD)
			return 0;
		if (ctrl.IsUnconscious())
			return -0.5;
		float w = 1.0;
		SCR_CharacterDamageManagerComponent dm = SCR_CharacterDamageManagerComponent.Cast(e.FindComponent(SCR_CharacterDamageManagerComponent));
		if (dm && dm.GetHealthScaled() < 0.7)
			w = 0.8;
		SCR_AIUtilityComponent u;
		AIControlComponent ac = AIControlComponent.Cast(e.FindComponent(AIControlComponent));
		if (ac)
		{
			SCR_ChimeraAIAgent ag = SCR_ChimeraAIAgent.Cast(ac.GetControlAIAgent());
			if (ag)
				u = ag.m_UtilityComponent;
		}
		if (!u)
			return w;
		DCO_AIMoraleSystem ms = u.GetMoraleSystem();
		if (ms)
		{
			moraleState st = ms.GetState();
			if (st == moraleState.PANIC || st == moraleState.BREAK)
				return 0;
			if (st == moraleState.SHAKEN)
				w = Math.Min(w, 0.7);
		}
		if (u.m_CombatComponent)
		{
			typename well;
			int spare = DCO_Rearm.SpareMags(u.m_CombatComponent, e, well);
			if (spare >= 0 && spare <= 1)
				w *= 0.5;
		}
		int roles = DCO_Roles.Get(u);
		if (roles & 1)
			w *= 2.0;
		else if ((roles & 2) && enemyArmor)
			w *= 1.5;
		return w;
	}

	protected float GroupWeight(DCO_GroupUtilityComponent g, bool enemyArmor, float now)
	{
		SCR_AIGroup grp = SCR_AIGroup.Cast(g.GetOwner());
		if (!grp)
			return 0;
		array<AIAgent> agents = {};
		grp.GetAgents(agents);
		float w;
		foreach (AIAgent a : agents)
		{
			if (a)
				w += UnitWeight(a.GetControlledEntity(), enemyArmor);
		}
		IEntity leader = grp.GetLeaderEntity();
		IEntity last;
		if (m_mLeader.Find(g, last) && last && last != leader && !DCO_Util.IsAlive(last))
		{
			m_mLeaderLostAt.Set(g, now);
			m_mLeaderLostDur.Set(g, Math.RandomFloat(120.0, 300.0));
			DCO_Perf.Count("reinf:leader_lost");
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.DEBUG, "-", string.Format("reinf_leader_lost cmd=%1 grp=%2", m_sUID, DCO_Log.IdGroup(grp)));
		}
		m_mLeader.Set(g, leader);
		float lostAt;
		if (m_mLeaderLostAt.Find(g, lostAt))
		{
			if (now - lostAt < m_mLeaderLostDur.Get(g))
				w *= 0.8;
			else
				w *= 0.9;
		}
		IEntity veh = g.GetGroupVehicle();
		if (veh)
		{
			int cls = DCO_VehicleActive.VehClass(veh);
			float vw;
			if (cls == 1)
				vw = 2;
			else if (cls == 2)
				vw = 6;
			else if (cls == 3)
				vw = 12;
			SCR_VehicleDamageManagerComponent vd = SCR_VehicleDamageManagerComponent.Cast(veh.FindComponent(SCR_VehicleDamageManagerComponent));
			if (vd && vd.GetHealthScaled() < 0.35)
				vw = 0;
			w += vw;
		}
		return Math.Max(w, 0);
	}

	protected int NewId()
	{
		s_iNextId++;
		return s_iNextId;
	}

	protected ref map<string, float> m_mNoSourceUntil;
	protected float m_fNoSourceLogAt = -1;

	void MarkNoSource(string type, string spawner, float now)
	{
		if (!m_mNoSourceUntil)
			m_mNoSourceUntil = new map<string, float>();
		m_mNoSourceUntil.Set(type, now + 300.0);
		DCO_Perf.Count("reinf:no_source");
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.WARN, "-", string.Format("reinf_no_source cmd=%1 type=%2 spawner=%3 pause_s=300 (no spawner pool has a group of this role; the commander stops asking for it for 5 min)", m_sUID, type, spawner));
	}

	protected bool NoSource(string type, float now)
	{
		float until;
		if (!m_mNoSourceUntil || !m_mNoSourceUntil.Find(type, until) || now >= until)
			return false;
		if (m_fNoSourceLogAt < 0 || now - m_fNoSourceLogAt >= 120.0)
		{
			m_fNoSourceLogAt = now;
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("reinf_suppressed cmd=%1 why=no_source type=%2 left_s=%3", m_sUID, type, Math.Round(until - now)));
		}
		return true;
	}

	protected AICommander_BaseComponent FindCmd()
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
			return null;
		return mgr.FindCommanderByUID(m_sUID);
	}

	protected void Cancel(DCO_ReinfRequest r, string why)
	{
		if (!r || r.m_bCancelled)
			return;
		r.m_bCancelled = true;
		r.m_sCancelWhy = why;
		DCO_Perf.Count("reinf:cancel_" + why);
		string fill = "-";
		if (r.m_iOrdered == 0)
			fill = DCO_CommanderSpawnerComponent.FillReport(FindCmd());
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("reinf_cancel cmd=%1 id=%2 kind=%3 why=%4 ordered=%5 arrived=%6 spawner=%7 fillers=%8", m_sUID, r.m_iId, r.KindName(), why, r.m_iOrdered, r.m_iArrived, r.m_sSpawner, fill));
	}

	protected DCO_ReinfRequest Create(int kind, string type, string why, int prio, float size, int groups, vector pos, float now)
	{
		DCO_ReinfRequest r = new DCO_ReinfRequest();
		r.m_iId = NewId();
		r.m_iKind = kind;
		r.m_sType = type;
		r.m_sWhy = why;
		r.m_sStyle = m_sStyle;
		r.m_iPrio = prio;
		r.m_fSize = size;
		r.m_iGroups = groups;
		r.m_fCreated = now;
		r.m_iBandAt = m_iBand;
		r.m_vPos = pos;
		DCO_Perf.Count("reinf:request_" + r.KindName());
		return r;
	}

	protected void LogRequest(DCO_ReinfRequest r)
	{
		string whyKey = r.m_sWhy;
		whyKey.Replace("+", "_");
		DCO_Perf.Count("reinf:why_" + whyKey);
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("reinf_request cmd=%1 id=%2 kind=%3 type=%4 why=%5 band=%6 ce=%7 obj=%8 ratio=%9", m_sUID, r.m_iId, r.KindName(), r.m_sType, r.m_sWhy, BandName(m_iBand), m_fCE.ToString(-1, 2), r.m_sObj, r.m_fRatio.ToString(-1, 2))
			+ string.Format(" thr=%1 conf=%2 size=%3 groups=%4 prio=%5 style=%6 fillers=%7", r.m_fThr.ToString(-1, 2), r.m_fConf.ToString(-1, 2), Math.Round(r.m_fSize), r.m_iGroups, DCO_ReinfRequest.PrioName(r.m_iPrio), m_sStyle, DCO_CommanderSpawnerComponent.FillReport(FindCmd())));
		DCO_Events.Reinforcement(string.Format("cmd=%1 id=%2 kind=%3 type=%4 why=%5 obj=%6 groups=%7", m_sUID, r.m_iId, r.KindName(), r.m_sType, r.m_sWhy, r.m_sObj, r.m_iGroups));
	}

	protected void Update(DCO_ReinfRequest r, int prio, float size, int groups, string why, bool overrun, float now)
	{
		int np = r.m_iPrio;
		if (prio > r.m_iPrio)
		{
			string ew;
			if (r.m_iPrio == 0 && now - r.m_fCreated >= 600.0 && m_iBand > r.m_iBandAt)
				ew = "band_drop";
			if (prio >= 2 && m_iBand == 3)
				ew = "black";
			if (prio >= 2 && overrun)
				ew = "overrun";
			if (!ew.IsEmpty())
			{
				np = prio;
				DCO_Perf.Count("reinf:escalate");
				DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("reinf_escalate cmd=%1 id=%2 from=%3 to=%4 why=%5", m_sUID, r.m_iId, DCO_ReinfRequest.PrioName(r.m_iPrio), DCO_ReinfRequest.PrioName(np), ew));
				r.m_iBandAt = m_iBand;
			}
		}
		int ng = Math.Max(groups, r.m_iOrdered);
		if (ng != r.m_iGroups || np != r.m_iPrio || why != r.m_sWhy)
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("reinf_update cmd=%1 id=%2 groups_old=%3 groups=%4 prio_old=%5 prio=%6 why=%7 size=%8", m_sUID, r.m_iId, r.m_iGroups, ng, DCO_ReinfRequest.PrioName(r.m_iPrio), DCO_ReinfRequest.PrioName(np), why, Math.Round(size)));
		r.m_iPrio = np;
		r.m_iGroups = ng;
		r.m_fSize = size;
		r.m_sWhy = why;
	}

	protected int Squads(float size)
	{
		int groups = Math.Floor(size / 8.0);
		if (size - groups * 8.0 >= 4.8)
			groups++;
		return Math.Min(groups, 4);
	}

	protected void CheckFilled(DCO_ReinfRequest r, float now)
	{
		if (!r || r.m_bCancelled || r.m_iArrived <= 0 || r.m_iArrived < r.m_iGroups)
			return;
		m_iFilled++;
		if (r.m_iKind == 1)
			m_fFilledAt = now;
		DCO_Perf.Count("reinf:filled");
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("reinf_filled cmd=%1 id=%2 kind=%3 spawner=%4 groups=%5 waited_s=%6", m_sUID, r.m_iId, r.KindName(), r.m_sSpawner, r.m_iArrived, Math.Round(now - r.m_fCreated)));
		r.m_bCancelled = true;
		r.m_sCancelWhy = "filled";
	}

	protected ref map<CMD_AICommanderObjectiveComponent, ref DCO_ReinfRequest> m_mDefense;
	protected ref map<CMD_AICommanderObjectiveComponent, float> m_mDefCooldown;

	void DefenseNeed(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, int need, int have, string why, int prio, float threatDist, float now)
	{
		if (!cmd || !obj)
			return;
		if (!m_mDefense)
		{
			m_mDefense = new map<CMD_AICommanderObjectiveComponent, ref DCO_ReinfRequest>();
			m_mDefCooldown = new map<CMD_AICommanderObjectiveComponent, float>();
		}
		DCO_ReinfRequest r = m_mDefense.Get(obj);
		int gap = need - have;
		FactionKey fk = cmd.GetCommanderFactionKey();
		FactionKey owner = obj.GetOwnerFaction();
		if (!owner.IsEmpty() && owner != fk)
		{
			DCO_Perf.Count("def:reinf_skip_lost");
			if (r)
			{
				DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("def_reinf_skip cmd=%1 obj=%2 reason=lost owner=%3 (counterattack job, not reinforcement)", m_sUID, obj.GetLogName(), owner));
				DefCancel(r, "lost", now);
			}
			return;
		}
		if (gap <= 0)
		{
			if (r && now - r.m_fCreated >= 60.0)
				DefCancel(r, "gap_closed", now);
			return;
		}
		int groups = Math.Min(gap, 2);
		if (r)
		{
			int np = r.m_iPrio;
			string ew;
			if (prio >= 2 && r.m_iPrio < 2 && (threatDist < 500.0 || why == "threat"))
				ew = "enemy_close";
			else if (prio > r.m_iPrio)
				ew = "higher_report";
			else if (r.m_iPrio == 0 && now - r.m_fCreated >= 300.0 && gap > r.m_iGapOrig)
			{
				ew = "worse_after_5min";
				prio = 1;
			}
			if (!ew.IsEmpty())
			{
				np = Math.Max(prio, r.m_iPrio);
				DCO_Perf.Count("reinf:escalate");
				DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("reinf_escalate cmd=%1 id=%2 from=%3 to=%4 why=%5 obj=%6", m_sUID, r.m_iId, DCO_ReinfRequest.PrioName(r.m_iPrio), DCO_ReinfRequest.PrioName(np), ew, obj.GetLogName()));
			}
			int ng = Math.Max(groups, r.m_iOrdered);
			if (ng != r.m_iGroups || np != r.m_iPrio)
				DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("reinf_update cmd=%1 id=%2 kind=defense obj=%3 groups_old=%4 groups=%5 prio_old=%6 prio=%7 why=%8", m_sUID, r.m_iId, obj.GetLogName(), r.m_iGroups, ng, DCO_ReinfRequest.PrioName(r.m_iPrio), DCO_ReinfRequest.PrioName(np), why));
			r.m_iPrio = np;
			r.m_iGroups = ng;
			r.m_fSize = ng * 8.0;
			r.m_iGap = gap;
			r.m_fLastTrigger = now;
			r.m_fThreatDist = threatDist;
			return;
		}
		float cd;
		if (m_mDefCooldown.Find(obj, cd) && now < cd && prio < 2)
		{
			DCO_Perf.Count("def:reinf_cooldown");
			return;
		}
		if (NoSource("DEFENSE", now))
			return;
		r = Create(3, "DEFENSE", why, prio, groups * 8.0, groups, obj.GetOwner().GetOrigin(), now);
		r.m_DefObj = obj;
		r.m_sObj = obj.GetLogName();
		r.m_fLastTrigger = now;
		r.m_fThreatDist = threatDist;
		r.m_iGap = gap;
		r.m_iGapOrig = gap;
		m_mDefense.Set(obj, r);
		DCO_Perf.Count("reinf:defense_request");
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("def_reinf_request cmd=%1 id=%2 obj=%3 trigger=%4 gap=%5 size=%6 prio=%7 threat_m=%8", m_sUID, r.m_iId, obj.GetLogName(), why, gap, groups, DCO_ReinfRequest.PrioName(prio), Math.Round(threatDist)));
		LogRequest(r);
	}

	protected void DefCancel(DCO_ReinfRequest r, string why, float now)
	{
		if (!r)
			return;
		if (!r.m_bCancelled)
		{
			Cancel(r, why);
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("def_reinf_cancel cmd=%1 id=%2 obj=%3 reason=%4 ordered=%5 arrived=%6", m_sUID, r.m_iId, r.m_sObj, why, r.m_iOrdered, r.m_iArrived));
		}
		if (m_mDefense && r.m_DefObj)
		{
			m_mDefense.Remove(r.m_DefObj);
			m_mDefCooldown.Set(r.m_DefObj, now + DCO_Ctx.TuneForCmd(FindCmd(), "reinf.def_cooldown_s", 240.0));
		}
	}

	protected void DefenseTick(AICommander_BaseComponent cmd, FactionKey fk, float now)
	{
		if (!DCO_DefShortfall.On(cmd))
			return;
		bool floorOn = DCO_DefShortfall.FloorOn(cmd);
		foreach (CMD_AICommanderObjectiveComponent obj : cmd.DCO_AllObjectives())
		{
			if (!obj || !obj.GetOwner() || !obj.IsCapturedBy(fk) || obj.IsCommanderBlackListed(cmd.GetCommanderUID()))
				continue;
			int have = DCO_DefShortfall.Defenders(cmd, obj);
			if (have > 0)
				continue;
			float td;
			float enemy;
			DCO_DefShortfall.ThreatNear(cmd, obj.GetOwner().GetOrigin(), now, td, enemy);
			int need = 1;
			if (floorOn)
				need = DCO_DefShortfall.Floor(obj, enemy, false);
			int prio = 1;
			if (td < 800.0)
				prio = 2;
			DCO_DefShortfall.Report(cmd, obj, need, have, "empty", prio, td);
		}
		if (!m_mDefense)
			return;
		array<CMD_AICommanderObjectiveComponent> objs = {};
		foreach (CMD_AICommanderObjectiveComponent o, DCO_ReinfRequest dr : m_mDefense)
			objs.Insert(o);
		foreach (CMD_AICommanderObjectiveComponent o2 : objs)
		{
			DCO_ReinfRequest r = m_mDefense.Get(o2);
			if (!r)
				continue;
			CheckFilled(r, now);
			if (r.m_bCancelled)
			{
				if (r.m_sCancelWhy == "filled")
					DCO_Perf.Count("reinf:defense_filled");
				DefCancel(r, r.m_sCancelWhy, now);
				continue;
			}
			if (!o2 || !o2.GetOwner())
			{
				DefCancel(r, "objective_gone", now);
				continue;
			}
			FactionKey ow = o2.GetOwnerFaction();
			if (!ow.IsEmpty() && ow != fk)
			{
				DefCancel(r, "lost", now);
				continue;
			}
			if (now - r.m_fLastTrigger > 300.0 && r.m_iOrdered == 0)
				DefCancel(r, "no_threat_5min", now);
		}
	}

	DCO_ReinfRequest Claimable(string spawner)
	{
		DCO_ReinfRequest best;
		array<DCO_ReinfRequest> list = {m_Strength, m_Capability};
		if (m_mDefense)
		{
			foreach (CMD_AICommanderObjectiveComponent dobj, DCO_ReinfRequest dreq : m_mDefense)
				list.Insert(dreq);
		}
		foreach (DCO_ReinfRequest r : list)
		{
			if (!r || r.m_bCancelled || r.m_iOrdered >= r.m_iGroups)
				continue;
			if (!r.m_sSpawner.IsEmpty() && r.m_sSpawner != spawner)
				continue;
			if (r.m_iPrio == 0)
			{
				DCO_ReinfRequest other = m_Capability;
				if (r == m_Capability)
					other = m_Strength;
				if (other && !other.m_bCancelled && other.m_iPrio > 0 && other.m_iOrdered < other.m_iGroups)
					continue;
			}
			if (!best || r.m_iPrio > best.m_iPrio)
				best = r;
		}
		return best;
	}

	void Tick(AICommander_BaseComponent cmd)
	{
		float now = DCO_Util.NowS();
		if (now < m_fNextTick)
			return;
		m_fNextTick = now + 30.0;
		m_sUID = cmd.GetCommanderUID();
		FactionKey fk = cmd.GetCommanderFactionKey();
		m_sStyle = "US";
		if (fk == "USSR")
			m_sStyle = "USSR";
		else if (fk == "FIA")
			m_sStyle = "FIA";

		CheckFilled(m_Strength, now);
		CheckFilled(m_Capability, now);
		if (m_Strength && m_Strength.m_bCancelled)
			m_Strength = null;
		if (m_Capability && m_Capability.m_bCancelled)
			m_Capability = null;
		DefenseTick(cmd, fk, now);

		bool enemyArmor;
		int armorThreats;
		int unclear;
		array<CMD_ThreatEntry> armorList = {};
		CMD_ThreatResponseComponent tr = cmd.GetThreatResponseComponent();
		if (tr)
		{
			foreach (CMD_ThreatEntry t : tr.GetThreats())
			{
				if (!t || now - t.m_fLastUpdateTime > 300.0)
					continue;
				if (t.m_iArmor > 0 || t.m_bArmorSeen)
				{
					enemyArmor = true;
					armorThreats++;
					armorList.Insert(t);
				}
				if (t.m_bNeedsRecon && !t.m_bReconSent)
					unclear++;
			}
		}

		map<DCO_GroupUtilityComponent, float> seen = new map<DCO_GroupUtilityComponent, float>();
		float total;
		int atGroups;
		bool armorClose;
		foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
		{
			if (!g || g.IsPlayerGroup() || g.IsMortar() || g.IsDedicatedTransport())
				continue;
			float w = GroupWeight(g, enemyArmor, now);
			seen.Set(g, w);
			total += w;
			if (g.HasAT() || g.IsArmor())
				atGroups++;
			foreach (CMD_ThreatEntry at : armorList)
			{
				if (vector.DistanceXZ(at.m_vBelievedPos, g.GetOwner().GetOrigin()) < 300.0)
					armorClose = true;
			}
		}
		foreach (DCO_GroupUtilityComponent og, float ow : m_mCounted)
		{
			if (!og || seen.Contains(og))
				continue;
			if (og.GetUnitCount() > 0 && og.GetMyCommander() != cmd)
			{
				m_fPeak = Math.Max(m_fPeak - ow, 0);
				DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("reinf_peak_adjust cmd=%1 grp=%2 minus=%3 peak=%4 why=transfer", m_sUID, og.GetOwner().GetName(), ow.ToString(-1, 1), m_fPeak.ToString(-1, 1)));
			}
		}
		m_mCounted = seen;
		m_fNow = total;
		m_fPeak = Math.Max(m_fPeak, total);
		if (m_fPeak <= 0)
			return;
		m_fCE = total / m_fPeak;

		float off;
		if (m_sStyle == "FIA")
			off = 0.1;
		int raw = 3;
		float green = Math.Min(T(cmd, "reinf.green", 0.85) + off, 0.99);
		float amber = Math.Min(T(cmd, "reinf.amber", 0.70) + off, green);
		float red = Math.Min(T(cmd, "reinf.red", 0.50) + off, amber);
		if (m_fCE >= green)
			raw = 0;
		else if (m_fCE >= amber)
			raw = 1;
		else if (m_fCE >= red)
			raw = 2;
		float hold = T(cmd, "reinf.hold_s", 60.0);
		if (raw != m_iBand)
		{
			if (raw != m_iPendBand || m_fPendSince < 0)
			{
				m_iPendBand = raw;
				m_fPendSince = now;
			}
			if (now - m_fPendSince >= hold)
			{
				DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("reinf_eval cmd=%1 ce=%2 band=%3 from=%4 peak=%5 now=%6 style=%7", m_sUID, m_fCE.ToString(-1, 2), BandName(raw), BandName(m_iBand), m_fPeak.ToString(-1, 1), total.ToString(-1, 1), m_sStyle));
				m_iBand = raw;
				m_fPendSince = -1;
			}
		}
		else
		{
			m_iPendBand = raw;
			m_fPendSince = -1;
		}
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.DEBUG, "-", string.Format("reinf_eval cmd=%1 ce=%2 band=%3 raw=%4 peak=%5 now=%6 style=%7", m_sUID, m_fCE.ToString(-1, 2), BandName(m_iBand), BandName(raw), m_fPeak.ToString(-1, 1), total.ToString(-1, 1), m_sStyle));

		float attSize;
		int attPrio = -1;
		bool recon;
		bool reconstitute = m_fCE < 0.25;
		if (reconstitute)
		{
			Suppress("reconstitute", now);
			Cancel(m_Strength, "reconstitute");
			m_Strength = null;
		}
		else if (m_iBand >= 1)
		{
			if (m_sStyle == "USSR" && m_iBand == 1)
				Suppress("ussr_amber", now);
			else
			{
				attPrio = m_iBand - 1;
				if (m_sStyle == "USSR")
					attSize = 8.0;
				else
					attSize = 0.9 * m_fPeak - total;
			}
		}
		float confMin = T(cmd, "reinf.conf_min", 0.4);
		float margin = 1.3;
		if (m_sStyle == "USSR")
			margin = 1.1;
		bool anyBelow;
		bool allAbove = true;
		bool anyMission;
		bool overrun;
		float ratSize;
		int ratPrio = -1;
		CMD_AICommanderObjectiveComponent ratObj;
		float ratR;
		float ratThr;
		float ratConf;
		DCO_DoctrineRuntime rt = cmd.GetDoctrineRuntime();
		DCO_CommanderTactics tac = cmd.GetTactics();
		if (!reconstitute)
		{
			foreach (CMD_AICommanderObjectiveComponent obj : cmd.GetObjectiveList())
			{
				if (!obj || !obj.GetOwner())
					continue;
				vector op = obj.GetOwner().GetOrigin();
				float thr;
				float ratio;
				float conf = 1;
				float shortMen;
				int prio;
				if (!obj.IsCapturedBy(fk))
				{
					bool planned = tac && tac.GetPlan(obj);
					bool engaged = planned;
					bool nearCap;
					foreach (DCO_GroupUtilityComponent ag : cmd.GetOwnedGroups())
					{
						if (!ag || ag.GetGroupObjective() != obj || ag.GetTask() != DCO_EGroupTask.ATTACK)
							continue;
						engaged = true;
						if (vector.DistanceXZ(ag.GetOwner().GetOrigin(), op) < 100.0)
							nearCap = true;
					}
					if (!engaged || !rt)
						continue;
					DCO_ForceEval e = rt.GetEval(obj);
					if (!e || e.m_iBand == DCO_EForceBand.UNKNOWN)
						continue;
					anyMission = true;
					if (nearCap)
					{
						Suppress("near_capture", now);
						continue;
					}
					conf = e.m_fConf;
					if (conf < confMin)
					{
						Suppress("intel_possible", now);
						recon = true;
						continue;
					}
					thr = T(cmd, "reinf.ratio_hasty", 1.7);
					if (planned)
						thr = T(cmd, "reinf.ratio_attack", 2.0);
					ratio = e.m_fEff;
					if (conf < 0.6)
						ratio = ratio / (1.0 + 0.5 * (1.0 - conf));
					prio = 1;
					if (conf < 0.6)
						prio = 0;
					shortMen = e.m_fOwn * (thr / Math.Max(ratio, 0.05) - 1.0);
				}
				else
				{
					float enemy;
					float bestQ;
					bool contact;
					if (tr)
					{
						foreach (CMD_ThreatEntry dt : tr.GetThreats())
						{
							if (!dt || now - dt.m_fLastUpdateTime > 300.0 || vector.DistanceXZ(dt.m_vBelievedPos, op) > obj.GetRadius() + 300.0)
								continue;
							enemy += dt.m_iEstimatedEnemyCount + 4 * dt.m_iVehicle + 8 * dt.m_iArmor;
							bestQ = Math.Max(bestQ, dt.m_fReportQuality);
							if (now - dt.m_fLastUpdateTime < 60.0)
								contact = true;
						}
					}
					if (enemy <= 0)
						continue;
					float own;
					foreach (DCO_GroupUtilityComponent dg, float dw : seen)
					{
						if (dg && dg.GetGroupObjective() == obj)
							own += dw;
					}
					anyMission = true;
					conf = bestQ;
					if (conf < confMin)
					{
						Suppress("intel_possible", now);
						recon = true;
						continue;
					}
					if (conf < 0.6)
						enemy *= 1.0 + 0.5 * (1.0 - conf);
					thr = T(cmd, "reinf.ratio_defend", 0.5);
					ratio = own / enemy;
					prio = 0;
					if (contact && conf >= 0.6)
						prio = 1;
					if (ratio < 0.25 && contact)
					{
						prio = 2;
						overrun = true;
					}
					shortMen = thr * enemy - own;
				}
				if (ratio >= 1.1 * thr)
					continue;
				allAbove = false;
				if (ratio >= 0.9 * thr)
					continue;
				anyBelow = true;
				if (shortMen * margin > ratSize)
				{
					ratSize = shortMen * margin;
					ratPrio = Math.Max(ratPrio, prio);
					ratObj = obj;
					ratR = ratio;
					ratThr = thr;
					ratConf = conf;
				}
			}
		}

		string missionGone;
		if (anyBelow)
		{
			m_fRatioAboveSince = -1;
			if (m_fRatioBelowSince < 0)
				m_fRatioBelowSince = now;
			if (now - m_fRatioBelowSince >= hold)
				m_bRatioActive = true;
		}
		else
		{
			m_fRatioBelowSince = -1;
			if (!anyMission && m_bRatioActive)
			{
				m_bRatioActive = false;
				missionGone = "mission_changed";
			}
			else if (allAbove)
			{
				if (m_fRatioAboveSince < 0)
					m_fRatioAboveSince = now;
				if (now - m_fRatioAboveSince >= hold)
					m_bRatioActive = false;
			}
		}

		bool want = attPrio >= 0 || m_bRatioActive;
		float size = attSize;
		int prioAll = attPrio;
		string why = "attrition";
		if (attPrio < 0)
			why = "ratio";
		if (m_bRatioActive)
		{
			if (attPrio >= 0)
				why = "attrition+ratio";
			if (ratPrio >= 0)
			{
				size = Math.Max(size, ratSize);
				prioAll = Math.Max(prioAll, ratPrio);
			}
			else if (m_Strength)
			{
				size = Math.Max(size, m_Strength.m_fSize);
				prioAll = Math.Max(prioAll, m_Strength.m_iPrio);
			}
			else if (prioAll < 0)
				want = false;
		}
		if (prioAll > 0 && now - m_fCulminatedAt < 600.0)
		{
			if (overrun)
				DCO_Perf.Count("reinf:culminated_overrun");
			else
			{
				prioAll = 0;
				Suppress("culminated", now);
			}
		}
		int groups = Squads(size);
		if (m_sStyle == "USSR" && want)
			groups = 1;
		if (want && groups < 1)
		{
			Suppress("reorganize", now);
			want = false;
		}
		if (want && m_Strength && m_Strength.m_iOrdered > m_Strength.m_iArrived && m_Strength.m_iOrdered >= groups)
			Suppress("en_route", now);
		if (want && !m_Strength && now - m_fFilledAt < 120.0 && prioAll < 2)
		{
			Suppress("integrating", now);
			want = false;
		}

		if (want)
		{
			if (!m_Strength)
			{
				vector pos = cmd.GetOwner().GetOrigin();
				if (ratObj)
					pos = ratObj.GetOwner().GetOrigin();
				else if (!seen.IsEmpty())
				{
					vector sum;
					int cnt;
					foreach (DCO_GroupUtilityComponent pg, float pw : seen)
					{
						if (pg)
						{
							sum += pg.GetOwner().GetOrigin();
							cnt++;
						}
					}
					if (cnt > 0)
						pos = sum * (1.0 / cnt);
				}
				m_Strength = Create(1, "-", why, prioAll, size, groups, pos, now);
				if (ratObj)
				{
					m_Strength.m_sObj = ratObj.GetLogName();
					m_Strength.m_fRatio = ratR;
					m_Strength.m_fThr = ratThr;
					m_Strength.m_fConf = ratConf;
				}
				LogRequest(m_Strength);
			}
			else
				Update(m_Strength, prioAll, size, groups, why, overrun, now);
		}
		else if (m_Strength && !reconstitute)
		{
			string cw = "recovered";
			if (!missionGone.IsEmpty() && attPrio < 0)
				cw = missionGone;
			Cancel(m_Strength, cw);
			m_Strength = null;
		}

		string capType;
		int capPrio = -1;
		int capGroups;
		if (armorThreats > atGroups)
		{
			capType = "AT";
			capPrio = 1;
			if (armorClose)
				capPrio = 2;
			capGroups = 2;
		}
		else if (unclear > 0 || recon)
		{
			capType = "RECON";
			capPrio = 0;
			capGroups = 1;
		}
		if (!capType.IsEmpty() && !m_Capability && NoSource(capType, now))
			capType = string.Empty;
		if (!capType.IsEmpty())
		{
			if (m_Capability && m_Capability.m_sType != capType)
			{
				Cancel(m_Capability, "gap_closed");
				m_Capability = null;
			}
			if (!m_Capability)
			{
				m_Capability = Create(2, capType, "capability", capPrio, capGroups * 8.0, capGroups, cmd.GetOwner().GetOrigin(), now);
				if (capType == "AT" && !armorList.IsEmpty())
					m_Capability.m_vPos = armorList[0].m_vBelievedPos;
				LogRequest(m_Capability);
			}
			else
				Update(m_Capability, capPrio, capGroups * 8.0, capGroups, "capability", armorClose, now);
		}
		else if (m_Capability)
		{
			Cancel(m_Capability, "gap_closed");
			m_Capability = null;
		}
	}
}
