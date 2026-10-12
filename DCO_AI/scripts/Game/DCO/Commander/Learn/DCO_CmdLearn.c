class DCO_CmdLearnLeg
{
	AICommander_BaseComponent m_Cmd;
	CMD_AICommanderObjectiveComponent m_Obj;
	string m_sKey;
	int m_iStart;
}

class DCO_CmdLearnMortar
{
	AICommander_BaseComponent m_Cmd;
	string m_sCell;
	vector m_vPos;
	bool m_bFired;
	float m_fFiredAt;
}

class DCO_CmdLearnOw
{
	AICommander_BaseComponent m_Cmd;
	DCO_GroupUtilityComponent m_Grp;
	vector m_vPos;
	vector m_vLast;
	float m_fStillSince;
	float m_fAt;
	int m_iStart;
	bool m_bSet;
}

class DCO_CmdLearnDeath
{
	AIGroup m_KillerGrp;
	vector m_vPos;
	string m_sFaction;
	float m_fAt;
}

class DCO_CmdLearn
{
	protected static ref DCO_CmdLearn s_Inst;
	protected BaseWorld m_World;

	protected ref DCO_LearnStore m_Tactics;
	protected ref DCO_LearnStore m_Terrain;
	protected ref map<AIGroup, ref DCO_CmdLearnLeg> m_mLegs = new map<AIGroup, ref DCO_CmdLearnLeg>();
	protected ref map<AIGroup, ref DCO_CmdLearnMortar> m_mMortars = new map<AIGroup, ref DCO_CmdLearnMortar>();
	protected ref map<AIGroup, ref DCO_CmdLearnOw> m_mOw = new map<AIGroup, ref DCO_CmdLearnOw>();
	protected ref map<CMD_AICommanderObjectiveComponent, float> m_mStageDone = new map<CMD_AICommanderObjectiveComponent, float>();
	protected ref array<ref DCO_CmdLearnDeath> m_aDeaths = {};
	protected ref map<string, bool> m_mSummary = new map<string, bool>();
	protected float m_fNextSave;
	protected bool m_bKillHook;

	static DCO_CmdLearn Get()
	{
		if (!Replication.IsServer())
			return null;
		if (s_Inst && s_Inst.m_World != GetGame().GetWorld())
		{
			SaveAll();
			Event("learn_world_switch store=cmd");
			s_Inst = null;
		}
		if (!s_Inst)
		{
			s_Inst = new DCO_CmdLearn();
			s_Inst.m_World = GetGame().GetWorld();
			string dir = "$profile:DCO/CommanderLearn/";
			if (DCO_TestCaseComponent.Active())
				dir = "$profile:DCO/CommanderLearn/_test/";
			s_Inst.m_Tactics = new DCO_LearnStore(dir + "Tactics.json");
			s_Inst.m_Terrain = new DCO_LearnStore(dir + "Terrain/" + DCO_LearnStore.WorldName() + ".json");
			s_Inst.m_fNextSave = DCO_Util.NowS() + 300.0;
		}
		return s_Inst;
	}

	static int ResetAll()
	{
		DCO_CmdLearn inst = Get();
		if (!inst)
			return 0;
		int n = inst.m_Tactics.Clear() + inst.m_Terrain.Clear();
		inst.m_mLegs.Clear();
		inst.m_mMortars.Clear();
		inst.m_mOw.Clear();
		inst.m_aDeaths.Clear();
		inst.m_mSummary.Clear();
		return n;
	}

	static void SaveAll()
	{
		if (!s_Inst)
			return;
		s_Inst.m_Tactics.Save("end");
		s_Inst.m_Terrain.Save("end");
	}

	static void Event(string line)
	{
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", line);
	}

	static string VsAt(vector pos, float radius)
	{
		PlayerManager pm = GetGame().GetPlayerManager();
		if (!pm)
			return "ai";
		array<int> players = {};
		pm.GetPlayers(players);
		foreach (int pid : players)
		{
			IEntity pe = pm.GetPlayerControlledEntity(pid);
			if (pe && DCO_Util.IsAlive(pe) && vector.DistanceXZ(pe.GetOrigin(), pos) <= radius)
				return "player";
		}
		return "ai";
	}

	static int Sector16(vector from, vector to)
	{
		vector d = to - from;
		if (d[0] * d[0] + d[2] * d[2] < 0.0001)
			return 0;
		float deg = Math.Atan2(d[0], d[2]) * Math.RAD2DEG;
		if (deg < 0)
			deg += 360.0;
		int s = Math.Floor((deg + 11.25) / 22.5);
		return s % 16;
	}

	static bool Explore()
	{
		return Math.RandomFloat01() < 0.12;
	}

	static void Context(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, DCO_TacticSituation s, out string ctx, out string ctxTR)
	{
		string type = "open";
		if (s.m_bUrban)
			type = "urban";
		else if (s.m_bForest)
			type = "forest";
		string band = "r2";
		if (s.m_fRatio < 1.0)
			band = "r0";
		else if (s.m_fRatio < 2.0)
			band = "r1";
		vector objPos = obj.GetOwner().GetOrigin();
		bool eat = false;
		bool inside = false;
		CMD_ThreatResponseComponent tr = cmd.GetThreatResponseComponent();
		if (tr)
		{
			foreach (CMD_ThreatEntry t : tr.GetThreats())
			{
				if (!t)
					continue;
				float d = vector.DistanceXZ(t.m_vPosition, objPos);
				if (d > obj.GetRadius() + 500.0)
					continue;
				if (t.m_bATSeen || t.m_bArmorSeen || t.m_iAT > 0 || t.m_iArmor > 0)
					eat = true;
				if (d <= obj.GetRadius())
					inside = true;
			}
		}
		bool omg = false;
		bool oveh = false;
		foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
		{
			if (!g || g.IsPlayerGroup())
				continue;
			if (g.HasMG())
				omg = true;
			if (g.GetGroupVehicle())
				oveh = true;
		}
		ctxTR = type + "_" + band;
		ctx = string.Format("%1_%2_m%3a%4M%5V%6n%7i%8", type, band, DCO_Util.B01(s.m_bEnemyMG), DCO_Util.B01(eat), DCO_Util.B01(omg), DCO_Util.B01(oveh), DCO_Util.B01(s.m_bNight), DCO_Util.B01(s.m_bUrban && inside));
	}

	protected float TacticMult(string vs, string ctx, string ctxTR, string name, out float n, out string level)
	{
		float m = m_Tactics.Mult("tactic|" + vs + "|x|" + ctx + "|" + name, 0.5, n);
		if (n >= 5)
		{
			level = "exact";
			return m;
		}
		m = m_Tactics.Mult("tactic|" + vs + "|t|" + ctxTR + "|" + name, 0.5, n);
		if (n >= 5)
		{
			level = "type_ratio";
			return m;
		}
		m = m_Tactics.Mult("tactic|" + vs + "|g|" + name, 0.5, n);
		if (n >= 5)
		{
			level = "global";
			return m;
		}
		level = "prior";
		return 1.0;
	}

	static int ApplyTactics(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, DCO_TacticSituation s, notnull array<float> scores, out string ctx, out string ctxTR, out string vs)
	{
		DCO_CmdLearn L = Get();
		vs = DCO_LearnDefense.VsKey(cmd, obj);
		Context(cmd, obj, s, ctx, ctxTR);
		if (!L)
			return -1;
		string before;
		string after;
		int best = -1;
		float bestScore = 0;
		float bestBase = 0;
		float bestMult = 1;
		float bestN = 0;
		string bestLevel = "prior";
		int leastIdx = -1;
		float leastN = float.MAX;
		for (int t = 0; t < scores.Count(); t++)
		{
			if (scores[t] <= 0)
				continue;
			string name = DCO_CommanderTactics.Name(t);
			float n = 0;
			string level = "prior";
			float m = L.TacticMult(vs, ctx, ctxTR, name, n, level);
			float raw = scores[t];
			scores[t] = raw * m;
			before += " " + name + ":" + raw.ToString(-1, 2);
			after += " " + name + ":" + scores[t].ToString(-1, 2);
			float exactN = 0;
			L.m_Tactics.Post("tactic|" + vs + "|x|" + ctx + "|" + name, 0.5, exactN);
			if (exactN < leastN)
			{
				leastN = exactN;
				leastIdx = t;
			}
			if (scores[t] > bestScore)
			{
				bestScore = scores[t];
				best = t;
				bestBase = raw;
				bestMult = m;
				bestN = n;
				bestLevel = level;
			}
		}
		Event(string.Format("tactic_select_learn obj=%1 ctx=%2 vs=%3 before=[%4 ] after=[%5 ]", obj.GetLogName(), ctx, vs, before, after));
		if (leastIdx >= 0 && leastIdx != best && Explore())
		{
			DCO_Perf.Count("learn:explore");
			Event(string.Format("learn_explore kind=tactic chosen=%1 reason=explore obj=%2", DCO_CommanderTactics.Name(leastIdx), obj.GetLogName()));
			return leastIdx;
		}
		if (best >= 0)
		{
			DCO_Perf.Count("learn:use:tactic");
			Event(string.Format("learn_use kind=tactic key=%1|%2 base=%3 mult=%4 samples=%5 level=%6", ctx, DCO_CommanderTactics.Name(best), bestBase.ToString(-1, 2), bestMult.ToString(-1, 2), Math.Round(bestN), bestLevel));
		}
		return -1;
	}

	static void TacticEpisode(CMD_AICommanderObjectiveComponent obj, int tactic, string ctx, string ctxTR, string vs, string result, float duration, int losses, int kills, bool control)
	{
		DCO_CmdLearn L = Get();
		if (!L || ctx.IsEmpty() || result == "replanned" || result == "bypass_end")
			return;
		float value = 0;
		if (result == "captured" && duration <= 1800.0)
			value = 1.0;
		else if (result == "raid_done" && kills > losses)
			value = 1.0;
		else if ((result == "timeout" || result == "ended") && control)
			value = 0.3;
		float ratio = 1.0 * losses / Math.Max(kills, 0.5);
		if (losses > 0 && ratio > 3.0)
			value = value * 0.3;
		else if (losses > 0 && ratio > 1.5)
			value = value * 0.6;
		string name = DCO_CommanderTactics.Name(tactic);
		L.m_Tactics.Add("tactic|" + vs + "|x|" + ctx + "|" + name, value);
		L.m_Tactics.Add("tactic|" + vs + "|t|" + ctxTR + "|" + name, value);
		L.m_Tactics.Add("tactic|" + vs + "|g|" + name, value);
		DCO_Perf.Count("learn:episode:tactic");
		Event(string.Format("learn_episode kind=tactic key=%1|%2 value=%3 vs=%4 reason=%5 obj=%6 losses=%7 kills=%8 control=%9", ctx, name, value.ToString(-1, 2), vs, result, obj.GetLogName(), losses, kills, DCO_Util.B01(control)));
	}

	static void Terrain(string kind, vector pos, float cell, float value, string reason, string objName)
	{
		DCO_CmdLearn L = Get();
		if (!L)
			return;
		string vs = VsAt(pos, 800.0);
		string key = DCO_LearnStore.PosKey(pos, cell);
		L.m_Terrain.Add(kind + "|" + vs + "|" + key, value);
		DCO_Perf.Count("learn:episode:" + kind);
		Event(string.Format("learn_episode kind=%1 key=%2 value=%3 vs=%4 reason=%5 obj=%6", kind, key, value.ToString(-1, 2), vs, reason, objName));
	}

	static float TerrainMult(string kind, vector pos, float cell, out float n)
	{
		n = 0;
		DCO_CmdLearn L = Get();
		if (!L)
			return 1.0;
		return L.m_Terrain.Mult(kind + "|" + VsAt(pos, 800.0) + "|" + DCO_LearnStore.PosKey(pos, cell), 0.5, n);
	}

	static float ApplyScore(float score, float mult)
	{
		return score + Math.AbsFloat(score) * (mult - 1.0);
	}

	static void LogUse(string kind, string key, float baseScore, float mult, float samples)
	{
		DCO_Perf.Count("learn:use:" + kind);
		Event(string.Format("learn_use kind=%1 key=%2 base=%3 mult=%4 samples=%5 level=%6", kind, key, baseScore.ToString(-1, 2), mult.ToString(-1, 2), Math.Round(samples), LevelOf(samples)));
	}

	static void LogExplore(string kind, string chosen)
	{
		DCO_Perf.Count("learn:explore");
		Event(string.Format("learn_explore kind=%1 chosen=%2 reason=explore", kind, chosen));
	}

	protected static string LevelOf(float samples)
	{
		if (samples > 0)
			return "exact";
		return "prior";
	}

	static float ApproachMult(CMD_AICommanderObjectiveComponent obj, float bearingDeg, out float n)
	{
		n = 0;
		DCO_CmdLearn L = Get();
		if (!L || !obj)
			return 1.0;
		vector objPos = obj.GetOwner().GetOrigin();
		float a = bearingDeg * Math.DEG2RAD;
		int s = Sector16(objPos, objPos + Vector(Math.Sin(a), 0, Math.Cos(a)));
		return L.m_Terrain.Mult("approach|" + VsAt(objPos, obj.GetRadius() + 1000.0) + "|" + obj.GetLogName() + "|" + s.ToString(), 0.5, n);
	}

	static void NoteFailBearing(CMD_AICommanderObjectiveComponent obj, float bearing)
	{
		DCO_CmdLearn L = Get();
		if (!L || !obj)
			return;
		int b = Math.Round(bearing / 22.5);
		L.m_Terrain.Add("fail|" + obj.GetLogName() + "|" + (b % 16).ToString(), 1.0);
	}

	static void LoadFailBearings(CMD_AICommanderObjectiveComponent obj, notnull array<float> outBearings)
	{
		DCO_CmdLearn L = Get();
		if (!L || !obj)
			return;
		for (int s = 0; s < 16; s++)
		{
			float n = 0;
			L.m_Terrain.Post("fail|" + obj.GetLogName() + "|" + s.ToString(), 0.0, n);
			if (n >= 1.0)
				outBearings.Insert(1.0 * s * 22.5);
		}
		if (!outBearings.IsEmpty())
			Event(string.Format("learn_use kind=fail key=%1 base=0 mult=1 samples=%2 level=exact", obj.GetLogName(), outBearings.Count()));
	}

	static int OwnDeathsNear(vector pos, float radius, string faction, float since)
	{
		DCO_CmdLearn L = Get();
		if (!L)
			return 0;
		int n = 0;
		foreach (DCO_CmdLearnDeath d : L.m_aDeaths)
		{
			if (d && d.m_fAt >= since && d.m_sFaction == faction && vector.DistanceXZ(d.m_vPos, pos) <= radius)
				n++;
		}
		return n;
	}

	static void StagingEpisode(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, vector st, float since, string why)
	{
		if (!cmd || !obj || st == vector.Zero)
			return;
		int dn = OwnDeathsNear(st, 150.0, cmd.GetCommanderFactionKey(), since);
		float v = 0;
		if (why == "go")
		{
			v = 1.0;
			if (dn >= 3)
				v = 0;
			else if (dn >= 1)
				v = 0.4;
		}
		else if (dn == 0)
			return;
		DCO_CmdLearn L = Get();
		if (!L)
			return;
		float done;
		if (L.m_mStageDone.Find(obj, done) && done == since)
		{
			DCO_Perf.Count("learn:staging_dup");
			return;
		}
		L.m_mStageDone.Set(obj, since);
		Terrain("staging", st, 50.0, v, why + "_deaths" + dn.ToString(), obj.GetLogName());
	}

	static int KillsByGroup(AIGroup grp, float since)
	{
		DCO_CmdLearn L = Get();
		if (!L || !grp)
			return 0;
		int n = 0;
		foreach (DCO_CmdLearnDeath d : L.m_aDeaths)
		{
			if (d && d.m_fAt >= since && d.m_KillerGrp == grp)
				n++;
		}
		return n;
	}

	static int DeathsNear(vector pos, float radius, string notFaction, float since)
	{
		DCO_CmdLearn L = Get();
		if (!L)
			return 0;
		int n = 0;
		foreach (DCO_CmdLearnDeath d : L.m_aDeaths)
		{
			if (d && d.m_fAt >= since && d.m_sFaction != notFaction && vector.DistanceXZ(d.m_vPos, pos) <= radius)
				n++;
		}
		return n;
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

	void OnKill(notnull SCR_InstigatorContextData data)
	{
		IEntity victim = data.GetVictimEntity();
		if (!victim)
			return;
		DCO_CmdLearnDeath d = new DCO_CmdLearnDeath();
		d.m_vPos = victim.GetOrigin();
		d.m_sFaction = DCO_Util.FactionKeyOf(victim, true);
		d.m_fAt = DCO_Util.NowS();
		IEntity killer = data.GetKillerEntity();
		if (killer)
		{
			AIControlComponent kc = AIControlComponent.Cast(killer.FindComponent(AIControlComponent));
			if (kc && kc.GetAIAgent())
				d.m_KillerGrp = kc.GetAIAgent().GetParentGroup();
		}
		m_aDeaths.Insert(d);
		if (m_aDeaths.Count() > 400)
			m_aDeaths.RemoveOrdered(0);
	}

	static void Tick(AICommander_BaseComponent cmd, float now)
	{
		DCO_CmdLearn L = Get();
		if (L && cmd)
			L.DoTick(cmd, now);
	}

	protected void DoTick(AICommander_BaseComponent cmd, float now)
	{
		HookKills();
		string uid = cmd.GetCommanderUID();
		if (!m_mSummary.Contains(uid))
		{
			m_mSummary.Set(uid, true);
			Summary();
		}
		UpdateLegs(cmd, now);
		UpdateMortars(cmd, now);
		UpdateOverwatch(cmd, now);
		if (now >= m_fNextSave)
		{
			m_fNextSave = now + 300.0;
			if (m_Tactics.IsDirty())
				m_Tactics.Save("timer");
			if (m_Terrain.IsDirty())
				m_Terrain.Save("timer");
		}
	}

	protected void Summary()
	{
		array<string> kinds = {"tactic", "approach", "overwatch", "staging", "op", "ambush", "mortar", "fail"};
		foreach (string kind : kinds)
		{
			int keys = 0;
			float samples = 0;
			string top;
			if (kind == "tactic")
				top = m_Tactics.Summary("tactic|", keys, samples);
			else
				top = m_Terrain.Summary(kind + "|", keys, samples);
			if (keys > 0)
				Event(string.Format("learn_summary kind=%1 keys=%2 samples=%3 top=%4", kind, keys, Math.Round(samples), top));
		}
	}

	protected void UpdateLegs(AICommander_BaseComponent cmd, float now)
	{
		array<AIGroup> seen = {};
		foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
		{
			if (!g || !g.GetOwner() || g.IsPlayerGroup())
				continue;
			AIGroup ag = AIGroup.Cast(g.GetOwner());
			if (!ag)
				continue;
			seen.Insert(ag);
			DCO_CmdLearnLeg leg = m_mLegs.Get(ag);
			CMD_AICommanderObjectiveComponent obj = g.GetGroupObjective();
			bool attacking = obj && obj.GetOwner() && (g.GetTask() == DCO_EGroupTask.ATTACK || g.GetTask() == DCO_EGroupTask.FLANK);
			if (!leg)
			{
				if (!attacking)
					continue;
				vector objPos = obj.GetOwner().GetOrigin();
				if (vector.DistanceXZ(ag.GetOrigin(), objPos) <= obj.GetRadius() + 150.0)
					continue;
				DCO_CmdLearnLeg nl = new DCO_CmdLearnLeg();
				nl.m_Obj = obj;
				nl.m_Cmd = cmd;
				nl.m_iStart = Math.Max(g.GetUnitCount(), 1);
				nl.m_sKey = "approach|" + VsAt(objPos, obj.GetRadius() + 1000.0) + "|" + obj.GetLogName() + "|" + Sector16(objPos, ag.GetOrigin()).ToString();
				m_mLegs.Set(ag, nl);
				continue;
			}
			if (!leg.m_Obj || !leg.m_Obj.GetOwner())
			{
				m_mLegs.Remove(ag);
				continue;
			}
			if (!attacking || obj != leg.m_Obj || g.IsRetreating())
			{
				EndLeg(ag, leg, 0, "stopped");
				continue;
			}
			if (vector.DistanceXZ(ag.GetOrigin(), leg.m_Obj.GetOwner().GetOrigin()) <= leg.m_Obj.GetRadius())
			{
				float loss = 1.0 - g.GetUnitCount() / (leg.m_iStart * 1.0);
				float value = 1.0;
				if (loss >= 0.25)
					value = 0.5;
				EndLeg(ag, leg, value, "reached");
			}
		}
		array<AIGroup> gone = {};
		foreach (AIGroup lg, DCO_CmdLearnLeg ll : m_mLegs)
		{
			if (ll && ll.m_Cmd && ll.m_Cmd != cmd)
				continue;
			if (!lg || !seen.Contains(lg))
				gone.Insert(lg);
		}
		foreach (AIGroup dg : gone)
		{
			DCO_CmdLearnLeg dl = m_mLegs.Get(dg);
			if (dl && dl.m_Cmd && s_Inst == this)
				EndLeg(dg, dl, 0, "destroyed");
			else
				m_mLegs.Remove(dg);
		}
	}

	protected void EndLeg(AIGroup ag, DCO_CmdLearnLeg leg, float value, string reason)
	{
		m_mLegs.Remove(ag);
		m_Terrain.Add(leg.m_sKey, value);
		DCO_Perf.Count("learn:episode:approach");
		Event(string.Format("learn_episode kind=approach key=%1 value=%2 vs=%3 reason=%4", leg.m_sKey, value.ToString(-1, 2), VsOfKey(leg.m_sKey), reason));
	}

	protected static string VsOfKey(string key)
	{
		array<string> parts = {};
		key.Split("|", parts, false);
		if (parts.Count() > 1)
			return parts[1];
		return "ai";
	}

	protected void UpdateMortars(AICommander_BaseComponent cmd, float now)
	{
		array<AIGroup> seen = {};
		foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
		{
			if (!g || !g.GetOwner() || !g.IsMortar())
				continue;
			AIGroup ag = AIGroup.Cast(g.GetOwner());
			if (!ag || g.GetUnitCount() <= 0)
				continue;
			seen.Insert(ag);
			DCO_CmdLearnMortar m = m_mMortars.Get(ag);
			vector pos = ag.GetOrigin();
			if (!m)
			{
				m = new DCO_CmdLearnMortar();
				m.m_Cmd = cmd;
				m.m_vPos = pos;
				m.m_sCell = DCO_LearnStore.PosKey(pos, 20.0);
				m_mMortars.Set(ag, m);
			}
			if (vector.DistanceXZ(pos, m.m_vPos) > 60.0)
			{
				if (m.m_bFired)
					Terrain("mortar", m.m_vPos, 20.0, 0.4, "moved", "-");
				m.m_vPos = pos;
				m.m_sCell = DCO_LearnStore.PosKey(pos, 20.0);
				m.m_bFired = false;
				continue;
			}
			if (g.GetTask() == DCO_EGroupTask.FIRE_MISSION && !m.m_bFired)
			{
				m.m_bFired = true;
				m.m_fFiredAt = now;
			}
			else if (m.m_bFired && g.GetTask() != DCO_EGroupTask.FIRE_MISSION && now - m.m_fFiredAt > 120.0)
			{
				Terrain("mortar", m.m_vPos, 20.0, 1.0, "fired_safe", "-");
				m.m_bFired = false;
			}
		}
		array<AIGroup> gone = {};
		foreach (AIGroup mg, DCO_CmdLearnMortar mm : m_mMortars)
		{
			if (mm && mm.m_Cmd && mm.m_Cmd != cmd)
				continue;
			if (!mg || !seen.Contains(mg))
				gone.Insert(mg);
		}
		foreach (AIGroup dg : gone)
		{
			DCO_CmdLearnMortar dm = m_mMortars.Get(dg);
			if (dm && dm.m_Cmd)
				Terrain("mortar", dm.m_vPos, 20.0, 0, "destroyed", "-");
			m_mMortars.Remove(dg);
		}
	}

	protected void UpdateOverwatch(AICommander_BaseComponent cmd, float now)
	{
		array<AIGroup> seen = {};
		string fk = cmd.GetCommanderFactionKey();
		foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
		{
			if (!g || !g.GetOwner() || g.IsPlayerGroup() || g.GetTask() != DCO_EGroupTask.SUPPORT_BY_FIRE)
				continue;
			AIGroup ag = AIGroup.Cast(g.GetOwner());
			if (!ag || g.GetUnitCount() <= 0)
				continue;
			seen.Insert(ag);
			vector pos = ag.GetOrigin();
			DCO_CmdLearnOw o = m_mOw.Get(ag);
			if (!o)
			{
				o = new DCO_CmdLearnOw();
				o.m_Cmd = cmd;
				o.m_Grp = g;
				o.m_vLast = pos;
				o.m_fStillSince = now;
				o.m_iStart = g.GetUnitCount();
				m_mOw.Set(ag, o);
				continue;
			}
			if (!o.m_bSet)
			{
				if (vector.DistanceXZ(pos, o.m_vLast) > 15.0)
				{
					o.m_vLast = pos;
					o.m_fStillSince = now;
					o.m_iStart = g.GetUnitCount();
				}
				else if (now - o.m_fStillSince >= 30.0)
				{
					o.m_bSet = true;
					o.m_vPos = pos;
					o.m_fAt = now;
					o.m_iStart = g.GetUnitCount();
				}
				continue;
			}
			if (vector.DistanceXZ(pos, o.m_vPos) > 60.0)
			{
				EndOw(o, g.GetUnitCount(), fk, now, "moved");
				m_mOw.Remove(ag);
			}
		}
		array<AIGroup> gone = {};
		foreach (AIGroup og, DCO_CmdLearnOw oo : m_mOw)
		{
			if (oo && oo.m_Cmd && oo.m_Cmd != cmd)
				continue;
			if (!og || !seen.Contains(og))
				gone.Insert(og);
		}
		foreach (AIGroup dg : gone)
		{
			DCO_CmdLearnOw d = m_mOw.Get(dg);
			if (d && d.m_Cmd)
			{
				int units = 0;
				if (d.m_Grp && dg)
					units = d.m_Grp.GetUnitCount();
				string why = "task_end";
				if (units <= 0)
					why = "destroyed";
				EndOw(d, units, fk, now, why);
			}
			m_mOw.Remove(dg);
		}
	}

	protected void EndOw(DCO_CmdLearnOw o, int units, string fk, float now, string why)
	{
		if (!o.m_bSet || now - o.m_fAt < 30.0)
			return;
		float loss = 1.0 - units / Math.Max(o.m_iStart * 1.0, 1.0);
		int kills = 0;
		if (o.m_Grp)
			kills = KillsByGroup(AIGroup.Cast(o.m_Grp.GetOwner()), o.m_fAt);
		float value = 0.5;
		if (units <= 0)
			value = 0;
		else if (loss >= 0.25)
			value = 0.3;
		else if (kills > 0)
			value = 1.0;
		Terrain("overwatch", o.m_vPos, 25.0, value, why + "_k" + kills.ToString() + "_loss" + Math.Round(loss * 100.0).ToString(), "-");
	}
}
