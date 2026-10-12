enum DCO_EDefTactic
{
	AREA,
	DEPTH,
	REVERSE,
	PERIMETER,
	DELAY
}

enum DCO_EDefTacticFlag
{
	AREA = 1,
	DEPTH = 2,
	REVERSE_SLOPE = 4,
	PERIMETER = 8,
	DELAY = 16,
	SPOILING_ATTACK = 32,
	COUNTERATTACK = 64,
	ENGAGEMENT_AREA = 128
}

enum DCO_EDefRole
{
	NONE,
	MAIN,
	FORWARD,
	FINAL,
	RESERVE,
	OP,
	DELAY,
	SPOIL_ABF,
	SPOIL_RAID,
	COUNTER
}

class DCO_DefensePlan
{
	CMD_AICommanderObjectiveComponent m_Obj;
	int m_eTactic = -1;
	float m_fStart;
	float m_fChosenAt;
	float m_fThreatSeen;
	float m_fPerimSince = -1;
	vector m_vAxis;
	vector m_vThreat;
	float m_fRatio;
	int m_iAxes;
	float m_fMainShare;
	bool m_bArmor;
	bool m_bIDF;
	int m_iBand = -1;
	int m_iBandPending = -1;
	float m_fBandSince = -1;
	bool m_bUrban;
	int m_iSwitches;
	bool m_bMainConfirmed;
	float m_fShareSince = -1;
	DCO_GroupUtilityComponent m_Economy;
	ref array<DCO_GroupUtilityComponent> m_aGroups = {};
	ref array<int> m_aStartUnits = {};
	DCO_GroupUtilityComponent m_Reserve;
	vector m_vReservePos;
	vector m_vFwd;
	vector m_vMain;
	vector m_vFinal;
	int m_iFallbacks;
	bool m_bCrest;
	string m_sCrestLog;
	vector m_vCrest;
	float m_fCrestDist;
	float m_fLastForwardFire = -1000;
	float m_fTrigger;
	bool m_bFired;
	bool m_bEA;
	vector m_vEACenter;
	bool m_bEAFire;
	DCO_GroupUtilityComponent m_Counter;
	float m_fCounterAt;
	float m_fCounterWin;
	float m_fCounterCd;
	int m_iCounters;
	DCO_GroupUtilityComponent m_Spoil;
	int m_iSpoilKind;
	float m_fSpoilAt;
	float m_fSpoilContact = -1;
	float m_fSpoilCd;
	vector m_vSpoilBack;
	int m_iSpoils;
	float m_fFPFAt = -1000;
	int m_iDelayStep;
	float m_fNextBound;
	float m_fLastReeval;
	int m_iDocAction = -1;
	int m_iDocBand = -1;
	int m_iDocPending = -1;
	float m_fDocSince = -1;
	float m_fDocConf;
	bool m_bEnded;
	ref map<DCO_GroupUtilityComponent, float> m_mSuppSince = new map<DCO_GroupUtilityComponent, float>();
	ref map<DCO_GroupUtilityComponent, float> m_mAltAt = new map<DCO_GroupUtilityComponent, float>();

	int StartUnits(DCO_GroupUtilityComponent g)
	{
		int i = m_aGroups.Find(g);
		if (i < 0)
			return g.GetUnitCount();
		return m_aStartUnits[i];
	}

	void Track(DCO_GroupUtilityComponent g)
	{
		if (m_aGroups.Contains(g))
			return;
		m_aGroups.Insert(g);
		m_aStartUnits.Insert(Math.Max(g.GetUnitCount(), 1));
	}
}

class DCO_DefenseTactics : Managed
{
	protected ref map<CMD_AICommanderObjectiveComponent, ref DCO_DefensePlan> m_mPlans = new map<CMD_AICommanderObjectiveComponent, ref DCO_DefensePlan>();
	protected ref map<CMD_ThreatEntry, float> m_mStillSince = new map<CMD_ThreatEntry, float>();
	protected ref map<CMD_ThreatEntry, vector> m_mStillPos = new map<CMD_ThreatEntry, vector>();
	protected ref map<CMD_AICommanderObjectiveComponent, float> m_mRetreatUntil = new map<CMD_AICommanderObjectiveComponent, float>();
	protected float m_fTimer;
	protected int m_iStandTo = 25;

	static string Name(int t)
	{
		if (t < 0)
			return "NONE";
		return typename.EnumToString(DCO_EDefTactic, t);
	}

	static string RoleName(int r)
	{
		return typename.EnumToString(DCO_EDefRole, r);
	}

	static void Event(string line)
	{
		DCO_Log.Write(DCO_ELogChannel.TACTIC, DCO_ELogLevel.INFO, "-", line);
	}

	static bool Allowed(AICommander_BaseComponent cmd, int bit)
	{
		return (cmd.GetAllowedDefenseTactics() & bit) != 0;
	}

	DCO_DefensePlan GetPlan(CMD_AICommanderObjectiveComponent obj)
	{
		return m_mPlans.Get(obj);
	}

	void Tick(AICommander_BaseComponent cmd, float timeSlice)
	{
		m_fTimer += timeSlice;
		if (m_fTimer < 5.0)
			return;
		m_fTimer = 0;
		int learnT = DCO_Perf.Begin();
		DCO_LearnDefense.Tick(cmd, DCO_Util.NowS());
		DCO_Perf.End("cmd_learn_def", learnT);
		int cmdLearnT = DCO_Perf.Begin();
		DCO_CmdLearn.Tick(cmd, DCO_Util.NowS());
		DCO_Perf.End("cmd_learn", cmdLearnT);
		if (cmd.GetAllowedDefenseTactics() == 0 && m_mPlans.IsEmpty())
			return;

		float now = DCO_Util.NowS();
		FactionKey fk = cmd.GetCommanderFactionKey();
		CMD_ThreatResponseComponent tr = cmd.GetThreatResponseComponent();
		UpdateStill(tr, now);
		UpdateStandTo(cmd, tr, now);

		array<CMD_AICommanderObjectiveComponent> ended = {};
		foreach (CMD_AICommanderObjectiveComponent obj, DCO_DefensePlan p : m_mPlans)
		{
			if (!obj || !p || UpdatePlan(cmd, p, tr, now))
				ended.Insert(obj);
		}
		foreach (CMD_AICommanderObjectiveComponent e : ended)
			m_mPlans.Remove(e);

		if (cmd.GetAllowedDefenseTactics() == 0)
			return;
		foreach (CMD_AICommanderObjectiveComponent o : cmd.DCO_AllObjectives())
		{
			if (!o || !o.GetOwner() || m_mPlans.Contains(o) || !o.IsCapturedBy(fk))
				continue;
			if (m_mRetreatUntil.Contains(o) && now < m_mRetreatUntil.Get(o))
				continue;
			array<DCO_GroupUtilityComponent> defs = {};
			Defenders(cmd, o, defs);
			if (defs.IsEmpty())
				continue;
			array<CMD_ThreatEntry> near = {};
			if (NearThreats(tr, o, now, near) == 0)
				continue;
			DCO_DefensePlan np = new DCO_DefensePlan();
			np.m_Obj = o;
			np.m_fStart = now;
			np.m_fThreatSeen = now;
			np.m_fLastReeval = now;
			foreach (DCO_GroupUtilityComponent d : defs)
				np.Track(d);
			m_mPlans.Set(o, np);
			DCO_Perf.Count("def:plan_start");
			Select(cmd, np, near, defs, now, "first_threat", "none");
			if (np.m_bEnded)
				m_mPlans.Remove(o);
		}
	}

	protected void Defenders(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, notnull array<DCO_GroupUtilityComponent> outDefs)
	{
		foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
		{
			if (!g || !g.GetOwner() || g.IsPlayerGroup() || g.IsMortar() || g.IsDedicatedTransport() || g.GetGroupObjective() != obj)
				continue;
			int task = g.GetTask();
			if (task == DCO_EGroupTask.DEFEND || task == DCO_EGroupTask.GARRISON || g.m_iDCODefRole != DCO_EDefRole.NONE)
				outDefs.Insert(g);
		}
	}

	protected int NearThreats(CMD_ThreatResponseComponent tr, CMD_AICommanderObjectiveComponent obj, float now, notnull array<CMD_ThreatEntry> outNear)
	{
		if (!tr)
			return 0;
		vector objPos = obj.GetOwner().GetOrigin();
		float r = obj.GetRadius() + 600.0;
		foreach (CMD_ThreatEntry t : tr.GetThreats())
		{
			if (t && now - t.m_fLastUpdateTime < 180.0 && vector.DistanceXZ(t.m_vBelievedPos, objPos) <= r)
				outNear.Insert(t);
		}
		return outNear.Count();
	}

	protected static float ThreatWeight(CMD_ThreatEntry t)
	{
		float w = Math.Max(t.m_iEstimatedEnemyCount, 1);
		if (t.m_iArmor > 0 || t.m_bArmorSeen)
			w += 8.0;
		return w + t.m_iMG;
	}

	protected void UpdateStill(CMD_ThreatResponseComponent tr, float now)
	{
		if (!tr)
			return;
		array<CMD_ThreatEntry> gone = {};
		foreach (CMD_ThreatEntry k, float v : m_mStillSince)
		{
			if (!k || !tr.GetThreats().Contains(k))
				gone.Insert(k);
		}
		foreach (CMD_ThreatEntry gk : gone)
		{
			m_mStillSince.Remove(gk);
			m_mStillPos.Remove(gk);
		}
		foreach (CMD_ThreatEntry t : tr.GetThreats())
		{
			if (!t)
				continue;
			vector at;
			if (!m_mStillPos.Find(t, at) || vector.DistanceXZ(at, t.m_vBelievedPos) > 20.0)
			{
				m_mStillPos.Set(t, t.m_vBelievedPos);
				m_mStillSince.Set(t, now);
			}
		}
	}

	protected float StillFor(CMD_ThreatEntry t, float now)
	{
		float since;
		if (!m_mStillSince.Find(t, since))
			return 0;
		return now - since;
	}

	protected static bool InPosition(DCO_GroupUtilityComponent g, CMD_AICommanderObjectiveComponent obj)
	{
		if (!g || !obj || g.GetPhase() == DCO_ETaskPhase.MOVING || g.m_DCOBreak)
			return false;
		return vector.DistanceXZ(g.GetOwner().GetOrigin(), obj.GetOwner().GetOrigin()) <= obj.GetRadius() + 400.0;
	}

	protected static float EnemyDistTo(array<CMD_ThreatEntry> near, vector pos, out CMD_ThreatEntry nearest)
	{
		float best = float.MAX;
		nearest = null;
		foreach (CMD_ThreatEntry t : near)
		{
			float d = vector.DistanceXZ(t.m_vBelievedPos, pos);
			if (d < best)
			{
				best = d;
				nearest = t;
			}
		}
		return best;
	}

	protected static bool CloseContact(DCO_GroupUtilityComponent g, array<CMD_ThreatEntry> near)
	{
		CMD_ThreatEntry t;
		return EnemyDistTo(near, g.GetOwner().GetOrigin(), t) < 125.0;
	}

	protected float Ratio(AICommander_BaseComponent cmd, DCO_DefensePlan p, array<DCO_GroupUtilityComponent> defs, array<CMD_ThreatEntry> near)
	{
		DCO_StrengthInfo info = new DCO_StrengthInfo();
		float mine = 0;
		foreach (DCO_GroupUtilityComponent g : defs)
		{
			DCO_Strength.OfGroup(SCR_AIGroup.Cast(g.GetOwner()), false, info);
			float factor = 1.0;
			if (InPosition(g, p.m_Obj))
			{
				factor = 1.3;
				DCO_AIGarrisonActivity act = g.GetGarrisonActivity();
				if (act && act.IsActive())
					factor = 2.0;
			}
			mine += info.m_fStrength * factor;
		}
		vector objPos = p.m_Obj.GetOwner().GetOrigin();
		foreach (DCO_GroupUtilityComponent rg : cmd.GetOwnedGroups())
		{
			if (!rg || !rg.GetOwner() || defs.Contains(rg) || rg.IsPlayerGroup() || rg.IsMortar() || !rg.IsInfantry() || !rg.IsAvailableReserve())
				continue;
			if (vector.DistanceXZ(rg.GetOwner().GetOrigin(), objPos) > 450.0)
				continue;
			DCO_Strength.OfGroup(SCR_AIGroup.Cast(rg.GetOwner()), false, info);
			mine += info.m_fStrength;
		}
		float enemy = 0;
		foreach (CMD_ThreatEntry t : near)
			enemy += ThreatWeight(t);
		return mine / Math.Max(enemy, 1.0);
	}

	protected void Axes(DCO_DefensePlan p, array<CMD_ThreatEntry> near, out bool perimeterAxes, out vector secondAxis)
	{
		vector objPos = p.m_Obj.GetOwner().GetOrigin();
		array<vector> dirs = {};
		array<float> weights = {};
		float total = 0;
		foreach (CMD_ThreatEntry t : near)
		{
			vector d = DCO_Specialist.Flat(t.m_vBelievedPos - objPos);
			float w = ThreatWeight(t);
			total += w;
			bool merged = false;
			for (int i = 0; i < dirs.Count(); i++)
			{
				vector cn = DCO_Specialist.Flat(dirs[i]);
				if (vector.Dot(cn, d) >= 0.707)
				{
					dirs[i] = dirs[i] + d * w;
					weights[i] = weights[i] + w;
					merged = true;
					break;
				}
			}
			if (!merged)
			{
				dirs.Insert(d * w);
				weights.Insert(w);
			}
		}
		perimeterAxes = false;
		secondAxis = vector.Zero;
		p.m_iAxes = 0;
		if (dirs.IsEmpty())
			return;
		int mainIdx = 0;
		foreach (int k, float kw : weights)
		{
			if (kw > weights[mainIdx])
				mainIdx = k;
		}
		p.m_vAxis = DCO_Specialist.Flat(dirs[mainIdx]);
		p.m_fMainShare = weights[mainIdx] / Math.Max(total, 1.0);
		float secondW = 0;
		foreach (int j, vector jd : dirs)
		{
			if (weights[j] * 3.0 < total)
				continue;
			p.m_iAxes++;
			if (j == mainIdx)
				continue;
			vector jn = DCO_Specialist.Flat(jd);
			if (vector.Dot(jn, p.m_vAxis) <= DCO_Ctx.TuneG("def.perim_dot", -0.17))
				perimeterAxes = true;
			if (weights[j] > secondW)
			{
				secondW = weights[j];
				secondAxis = jn;
			}
		}
		float perimNow = DCO_Util.NowS();
		if (!perimeterAxes)
			p.m_fPerimSince = -1;
		else
		{
			if (p.m_fPerimSince < 0)
			{
				p.m_fPerimSince = perimNow;
				DCO_Log.Write(DCO_ELogChannel.TACTIC, DCO_ELogLevel.INFO, "-", string.Format("def_perimeter_axis obj=%1 axes=%2 main_share=%3 confirm_s=%4", p.m_Obj.GetOwner().GetName(), p.m_iAxes, p.m_fMainShare.ToString(-1, 2), Math.Round(DCO_Ctx.TuneG("def.perim_confirm_s", 60.0))));
			}
			if (perimNow - p.m_fPerimSince < DCO_Ctx.TuneG("def.perim_confirm_s", 60.0))
			{
				perimeterAxes = false;
				DCO_Perf.Count("def:perim_wait");
			}
			else
				DCO_Perf.Count("def:perim_confirmed");
		}
		CMD_ThreatEntry nearest;
		EnemyDistTo(near, objPos, nearest);
		if (nearest)
			p.m_vThreat = nearest.m_vBelievedPos;
	}

	protected bool FindCrest(DCO_DefensePlan p)
	{
		vector objPos = p.m_Obj.GetOwner().GetOrigin();
		BaseWorld world = GetGame().GetWorld();
		float objH = world.GetSurfaceY(objPos[0], objPos[2]);
		float bestH = -float.MAX;
		float bestD = -1;
		vector bestDir = p.m_vAxis;
		float fan = DCO_Ctx.TuneG("def.crest_fan", 0);
		int fans = 1;
		if (fan > 0)
			fans = 3;
		for (int f = 0; f < fans; f++)
		{
			float deg = 0;
			if (f == 1)
				deg = 25.0;
			else if (f == 2)
				deg = -25.0;
			float rad = deg * Math.DEG2RAD;
			vector ax = Vector(p.m_vAxis[0] * Math.Cos(rad) - p.m_vAxis[2] * Math.Sin(rad), 0, p.m_vAxis[0] * Math.Sin(rad) + p.m_vAxis[2] * Math.Cos(rad));
			for (int i = 0; i <= 10; i++)
			{
				float d = 50.0 + i * 25.0;
				vector s = objPos + ax * d;
				float h = world.GetSurfaceY(s[0], s[2]);
				vector ahead = objPos + ax * (d + 100.0);
				float ah = world.GetSurfaceY(ahead[0], ahead[2]);
				if (h >= objH + 3.0 && h >= ah + 2.0 && h > bestH)
				{
					bestH = h;
					bestD = d;
					bestDir = ax;
				}
			}
		}
		p.m_bCrest = bestD > 0;
		if (p.m_bCrest)
		{
			p.m_fCrestDist = bestD;
			p.m_vCrest = DCO_Util.Surface(objPos + bestDir * bestD);
		}
		if (fans > 1)
		{
			float cross = bestDir[0] * p.m_vAxis[2] - bestDir[2] * p.m_vAxis[0];
			float offDeg = Math.Atan2(cross, vector.Dot(bestDir, p.m_vAxis)) * Math.RAD2DEG;
			float rise = 0;
			if (p.m_bCrest)
				rise = bestH - objH;
			string crestLine = string.Format("def_crest obj=%1 found=%2 dist=%3 off_deg=%4 rise=%5", p.m_Obj.GetLogName(), p.m_bCrest, Math.Round(bestD), Math.Round(offDeg), Math.Round(rise));
			if (crestLine != p.m_sCrestLog)
			{
				p.m_sCrestLog = crestLine;
				Event(crestLine);
			}
		}
		return p.m_bCrest;
	}

	protected static bool Isolated(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj)
	{
		FactionKey fk = cmd.GetCommanderFactionKey();
		vector objPos = obj.GetOwner().GetOrigin();
		foreach (CMD_AICommanderObjectiveComponent o : cmd.DCO_AllObjectives())
		{
			if (o && o != obj && o.GetOwner() && o.IsCapturedBy(fk) && vector.DistanceXZ(o.GetOwner().GetOrigin(), objPos) < 1500.0)
				return false;
		}
		return true;
	}

	protected static int Band(float ratio)
	{
		if (ratio < 0.33)
			return 0;
		if (ratio < 0.67)
			return 1;
		if (ratio <= 2.0)
			return 2;
		return 3;
	}

	protected static int BandHyst(float ratio, int cur)
	{
		array<float> edges = {0.33, 0.67, 2.0};
		int raw = Band(ratio);
		if (cur < 0 || raw == cur)
			return raw;
		if (raw < cur && ratio < edges[cur - 1] * 0.8)
			return raw;
		if (raw > cur && ratio > edges[cur] * 1.2)
			return raw;
		return cur;
	}

	protected bool Assess(AICommander_BaseComponent cmd, DCO_DefensePlan p, array<CMD_ThreatEntry> near, array<DCO_GroupUtilityComponent> defs, float now, out bool perimeterAxes, out vector secondAxis)
	{
		p.m_fRatio = Ratio(cmd, p, defs, near);
		Axes(p, near, perimeterAxes, secondAxis);
		bool armor = false;
		foreach (CMD_ThreatEntry t : near)
		{
			if (t.m_iArmor > 0 || t.m_bArmorSeen)
				armor = true;
		}
		bool idf = cmd.GetDefense() && now - cmd.GetDefense().GetLastEnemyIDF() < 600.0;
		bool newHeavy = (armor && !p.m_bArmor) || (idf && !p.m_bIDF);
		p.m_bArmor = p.m_bArmor || armor;
		p.m_bIDF = p.m_bIDF || idf;
		return newHeavy;
	}

	protected void Select(AICommander_BaseComponent cmd, DCO_DefensePlan p, array<CMD_ThreatEntry> near, array<DCO_GroupUtilityComponent> defs, float now, string reason, string trigger)
	{
		bool perimeterAxes;
		vector secondAxis;
		Assess(cmd, p, near, defs, now, perimeterAxes, secondAxis);
		vector objPos = p.m_Obj.GetOwner().GetOrigin();
		DCO_GarrisonRegistry reg = DCO_GarrisonRegistry.GetInstance();
		if (reg)
		{
			array<IEntity> buildings = {};
			reg.FindBuildings(objPos, p.m_Obj.GetRadius(), buildings);
			p.m_bUrban = buildings.Count() >= 4;
		}
		FindCrest(p);

		bool contact = false;
		bool hasty = false;
		foreach (DCO_GroupUtilityComponent g : defs)
		{
			if (g.HasState(DCO_EGroupState.IN_CONTACT))
				contact = true;
			if (!InPosition(g, p.m_Obj))
				hasty = true;
		}
		bool forwardOpen = false;
		if (p.m_bCrest)
		{
			array<DCO_TerrainPoint> pts = {};
			forwardOpen = DCO_TerrainCache.Query(DCO_ETerrainFlag.FOREST_EDGE, p.m_vCrest + p.m_vAxis * 150.0, 100.0, pts) < 2;
		}

		float r = p.m_fRatio;
		array<float> scores = {0.0, 0.0, 0.0, 0.0, 0.0};
		if (r >= 0.67)
			scores[DCO_EDefTactic.AREA] = 1.0;
		if (defs.Count() >= 2 && (r < 0.67 || p.m_bIDF))
		{
			scores[DCO_EDefTactic.DEPTH] = 1.0;
			if (p.m_bIDF)
				scores[DCO_EDefTactic.DEPTH] = 1.3;
		}
		if (p.m_bCrest && (p.m_bArmor || p.m_bIDF || forwardOpen || (hasty && contact)))
			scores[DCO_EDefTactic.REVERSE] = 1.2;
		if (perimeterAxes)
			scores[DCO_EDefTactic.PERIMETER] = 1.3;
		else if (Isolated(cmd, p.m_Obj))
			scores[DCO_EDefTactic.PERIMETER] = 0.6;
		if (r < 0.33)
			scores[DCO_EDefTactic.DELAY] = 1.5;

		if (DCO_Radio.PersonalityOf(cmd) == 2)
		{
			scores[DCO_EDefTactic.DEPTH] = scores[DCO_EDefTactic.DEPTH] * 1.2;
			scores[DCO_EDefTactic.REVERSE] = scores[DCO_EDefTactic.REVERSE] * 1.2;
			scores[DCO_EDefTactic.PERIMETER] = scores[DCO_EDefTactic.PERIMETER] * 1.2;
		}

		if (DCO_MettTable.On())
		{
			if (p.m_bUrban)
			{
				scores[DCO_EDefTactic.AREA] = scores[DCO_EDefTactic.AREA] * 1.3;
				scores[DCO_EDefTactic.REVERSE] = scores[DCO_EDefTactic.REVERSE] * 0.6;
				scores[DCO_EDefTactic.PERIMETER] = scores[DCO_EDefTactic.PERIMETER] * 1.3;
			}
			if (hasty)
			{
				scores[DCO_EDefTactic.AREA] = scores[DCO_EDefTactic.AREA] * 1.3;
				scores[DCO_EDefTactic.DEPTH] = scores[DCO_EDefTactic.DEPTH] * 0.6;
			}
			string mettLine = string.Format("mett_def obj=%1 urban=%2 hasty=%3", p.m_Obj.GetOwner().GetName(), p.m_bUrban, hasty);
			mettLine += " area=" + scores[0].ToString(-1, 2) + " depth=" + scores[1].ToString(-1, 2) + " reverse=" + scores[2].ToString(-1, 2);
			mettLine += " perimeter=" + scores[3].ToString(-1, 2) + " delay=" + scores[4].ToString(-1, 2);
			DCO_CommanderTactics.Event(mettLine);
		}

		int docBand;
		float docConf;
		int act = DocAction(cmd, p, defs, near, perimeterAxes, now, docBand, docConf);
		if (act >= 0)
		{
			bool docChanged = act != p.m_iDocAction || docBand != p.m_iDocBand;
			p.m_iDocAction = act;
			p.m_iDocBand = docBand;
			p.m_fDocConf = docConf;
			p.m_iDocPending = -1;
			if (docChanged)
				DCO_DoctrineDefense.Log(cmd, p.m_Obj, DCO_DoctrineDefense.ActionName(act), docBand, r, docConf, " reason=" + reason + " trigger=" + trigger);
			if (act == DCO_EDefAction.FALLBACK || act == DCO_EDefAction.WITHDRAW)
			{
				DoctrineRetreat(cmd, p, defs, near, act == DCO_EDefAction.FALLBACK, now);
				return;
			}
			if (act == DCO_EDefAction.DELAY)
			{
				for (int dt = 0; dt < scores.Count(); dt++)
				{
					if (dt != DCO_EDefTactic.DELAY)
						scores[dt] = 0;
				}
				scores[DCO_EDefTactic.DELAY] = 1.5;
			}
			else
				scores[DCO_EDefTactic.DELAY] = 0;
			if (docBand == DCO_EForceBand.UNKNOWN && DCO_DoctrineDefense.Allows(cmd, docBand, DCO_EDefenseCol.OPLP) && cmd.GetDefense())
				cmd.GetDefense().RequestDoctrineOP(cmd, p.m_Obj, p.m_vAxis, now);
		}

		float sum = 0;
		for (int t = 0; t < scores.Count(); t++)
		{
			if (!Allowed(cmd, 1 << t))
				scores[t] = 0;
			sum += scores[t];
		}

		int chosen = -1;
		int onlyAllowed = -1;
		int allowedCount = 0;
		for (int ab = 0; ab <= DCO_EDefTactic.DELAY; ab++)
		{
			if (Allowed(cmd, 1 << ab))
			{
				allowedCount++;
				onlyAllowed = ab;
			}
		}
		if (allowedCount == 1)
		{
			chosen = onlyAllowed;
			DCO_Perf.Count("def:select_mask");
		}
		else if (sum > 0)
		{
			float roll = Math.RandomFloat(0, sum);
			for (int k = 0; k < scores.Count(); k++)
			{
				if (scores[k] <= 0)
					continue;
				roll -= scores[k];
				chosen = k;
				if (roll <= 0)
					break;
			}
		}
		else if (Allowed(cmd, DCO_EDefTacticFlag.AREA))
		{
			chosen = DCO_EDefTactic.AREA;
			reason = reason + "_fallback";
		}

		string all;
		for (int s = 0; s < scores.Count(); s++)
			all += string.Format(" %1:%2", Name(s), scores[s].ToString(-1, 2));

		int prev = p.m_eTactic;
		p.m_iBand = BandHyst(r, p.m_iBand);
		p.m_iBandPending = -1;
		p.m_fBandSince = -1;
		p.m_fLastReeval = now;
		if (reason == "first_threat")
		{
			DCO_Perf.Count("def:select:" + Name(chosen));
			Event(string.Format("def_select cmd=%1 obj=%2 scores=[%3 ] chosen=%4 ratio=%5 axes=%6 reason=%7 urban=%8 crest=%9",
				cmd.GetCommanderUID(), p.m_Obj.GetLogName(), all, Name(chosen), r.ToString(-1, 2), p.m_iAxes, reason, p.m_bUrban, p.m_bCrest));
		}
		else
		{
			if (chosen == prev)
				return;
			p.m_iSwitches++;
			DCO_Perf.Count("def:switch:" + trigger);
			Event(string.Format("def_switch obj=%1 from=%2 to=%3 trigger=%4 held_s=%5 ratio=%6 scores=[%7 ]",
				p.m_Obj.GetLogName(), Name(prev), Name(chosen), trigger, Math.Round(now - p.m_fChosenAt), r.ToString(-1, 2), all));
		}
		p.m_eTactic = chosen;
		p.m_fChosenAt = now;
		if (chosen < 0)
			return;
		Apply(cmd, p, defs, near, secondAxis, now);
		if (reason == "first_threat")
			DCO_Briefing.Defense(cmd, p, DCO_EBriefKind.OPORD);
		else
			DCO_Briefing.Defense(cmd, p, DCO_EBriefKind.FRAGO, "brief_frago_" + trigger);
	}

	protected int DocAction(AICommander_BaseComponent cmd, DCO_DefensePlan p, array<DCO_GroupUtilityComponent> defs, array<CMD_ThreatEntry> near, bool perimeterAxes, float now, out int band, out float conf)
	{
		bool prepared = true;
		foreach (DCO_GroupUtilityComponent g : defs)
		{
			if (!InPosition(g, p.m_Obj))
				prepared = false;
		}
		conf = DCO_DoctrineDefense.Confidence(near, now);
		bool encircled = perimeterAxes && Isolated(cmd, p.m_Obj);
		return DCO_DoctrineDefense.Decide(cmd, p.m_Obj, p.m_fRatio, conf, prepared, encircled, Allowed(cmd, DCO_EDefTacticFlag.DELAY), p.m_iDocBand, band);
	}

	protected void DoctrineRetreat(AICommander_BaseComponent cmd, DCO_DefensePlan p, array<DCO_GroupUtilityComponent> defs, array<CMD_ThreatEntry> near, bool fallback, float now)
	{
		string target;
		vector to = DCO_DoctrineDefense.RetreatTarget(cmd, p.m_Obj, p.m_vAxis, fallback, target);
		int moved;
		int smoked;
		foreach (DCO_GroupUtilityComponent g : defs)
		{
			if (!g || !g.GetOwner() || g.IsInTransport())
				continue;
			CMD_ThreatEntry nt;
			if (EnemyDistTo(near, g.GetOwner().GetOrigin(), nt) < 400.0 && nt)
			{
				CMD_ThreatResponseComponent.ThrowSmoke(g, nt.m_vBelievedPos);
				smoked++;
			}
			g.DCO_SetHold(0);
			g.CompleteAllWaypoints();
			g.SetTask(DCO_EGroupTask.NONE);
			g.SetGroupObjective(null);
			if (cmd.SpawnMoveRoute(g, g.GetOwner().GetOrigin(), to, now))
				moved++;
		}
		p.m_bEnded = true;
		m_mRetreatUntil.Set(p.m_Obj, now + 300.0);
		string action = "withdraw";
		if (fallback)
			action = "fallback";
		DCO_Perf.Count("def:doctrine_" + action);
		Event(string.Format("def_phase obj=%1 tactic=%2 phase=doctrine_%3 groups=%4 moved=%5 smoke=%6 target=%7 pos=%8", p.m_Obj.GetLogName(), Name(p.m_eTactic), action, defs.Count(), moved, smoked, target, to));
		Result(cmd, p, "withdrawn", "doctrine_" + action, now);
	}

	protected void Place(AICommander_BaseComponent cmd, DCO_GroupUtilityComponent g, vector pos, vector look, bool hold, float now)
	{
		g.CompleteAllWaypoints();
		vector face = look - pos;
		DCO_GarrisonWaypoint gw = cmd.SpawnGarrisonWP(DCO_Util.Surface(pos), Math.Atan2(face[2], face[0]), DCO_Ctx.TuneForCmd(cmd, "def.place_r", 45.0), hold);
		if (gw)
			g.MoveTo(gw, now);
		if (hold)
			g.DCO_SetHold(now + 3600);
		string kind = "sector";
		if (g.m_iDCODefRole == DCO_EDefRole.RESERVE)
			kind = "reserve";
		else if (g.m_iDCODefRole == DCO_EDefRole.OP)
			kind = "op";
		else if (g.m_iDCODefRole == DCO_EDefRole.COUNTER)
			kind = "qrf";
		else if (g.m_iDCODefRole == DCO_EDefRole.SPOIL_ABF || g.m_iDCODefRole == DCO_EDefRole.SPOIL_RAID)
			kind = "patrol";
		string defName = "-";
		DCO_DefensePlan dp = m_mPlans.Get(g.GetGroupObjective());
		if (dp)
			defName = Name(dp.m_eTactic);
		DCO_Battle.Place(cmd, g.GetGroupObjective(), AIGroup.Cast(g.GetOwner()), kind, pos, Math.Atan2(face[0], face[2]) * Math.RAD2DEG, "-", "-", RoleName(g.m_iDCODefRole), defName);
	}

	protected void SetRole(DCO_DefensePlan p, DCO_GroupUtilityComponent g, int role)
	{
		p.Track(g);
		g.m_iDCODefRole = role;
	}

	protected DCO_GroupUtilityComponent LeastEngaged(DCO_DefensePlan p, array<DCO_GroupUtilityComponent> groups, array<CMD_ThreatEntry> near)
	{
		DCO_GroupUtilityComponent best;
		float bestD = -1;
		foreach (DCO_GroupUtilityComponent g : groups)
		{
			if (!g.IsInfantry() || g.HasState(DCO_EGroupState.IN_CONTACT))
				continue;
			CMD_ThreatEntry t;
			float d = EnemyDistTo(near, g.GetOwner().GetOrigin(), t);
			if (d > bestD)
			{
				bestD = d;
				best = g;
			}
		}
		return best;
	}

	protected void Apply(AICommander_BaseComponent cmd, DCO_DefensePlan p, array<DCO_GroupUtilityComponent> defs, array<CMD_ThreatEntry> near, vector secondAxis, float now)
	{
		vector objPos = p.m_Obj.GetOwner().GetOrigin();
		float R = p.m_Obj.GetRadius();
		vector side = Vector(-p.m_vAxis[2], 0, p.m_vAxis[0]);
		array<DCO_GroupUtilityComponent> free = {};
		foreach (DCO_GroupUtilityComponent g : defs)
		{
			if (g == p.m_Counter || g == p.m_Spoil)
				continue;
			if (CloseContact(g, near))
			{
				if (g.m_iDCODefRole == DCO_EDefRole.NONE)
					SetRole(p, g, DCO_EDefRole.MAIN);
				DCO_Perf.Count("def:keep_close_contact");
				continue;
			}
			free.Insert(g);
		}
		p.m_bFired = false;
		p.m_fTrigger = R + DCO_Ctx.TuneForCmd(cmd, "def.trigger_extra", 150.0);
		p.m_Reserve = null;
		p.m_Economy = null;

		if (p.m_eTactic != DCO_EDefTactic.DELAY && p.m_eTactic != DCO_EDefTactic.REVERSE && defs.Count() >= 3 && free.Count() >= 2 && DCO_Force.Reserve(cmd, defs.Count()) >= 1)
		{
			p.m_Reserve = LeastEngaged(p, free, near);
			if (p.m_Reserve)
			{
				free.RemoveItem(p.m_Reserve);
				SetRole(p, p.m_Reserve, DCO_EDefRole.RESERVE);
				p.m_vReservePos = DCO_LearnDefense.ReservePos(cmd, p.m_Obj, objPos - p.m_vAxis * Math.Min(R * 0.5, 80.0));
				Place(cmd, p.m_Reserve, p.m_vReservePos, objPos + p.m_vAxis * R, p.m_eTactic == DCO_EDefTactic.PERIMETER, now);
			}
		}
		if (!p.m_bMainConfirmed && secondAxis != vector.Zero && free.Count() >= 3)
		{
			float bestDot = -2;
			foreach (DCO_GroupUtilityComponent eg : free)
			{
				float dt = vector.Dot(DCO_Specialist.Flat(eg.GetOwner().GetOrigin() - objPos), secondAxis);
				if (dt > bestDot)
				{
					bestDot = dt;
					p.m_Economy = eg;
				}
			}
			if (p.m_Economy)
			{
				free.RemoveItem(p.m_Economy);
				SetRole(p, p.m_Economy, DCO_EDefRole.MAIN);
				DCO_AIGarrisonActivity ea = p.m_Economy.GetGarrisonActivity();
				if (ea && ea.IsActive())
					ea.DCO_Reface(secondAxis, false);
				Event(string.Format("def_phase obj=%1 tactic=%2 phase=hold_secondary line=economy grp=%3", p.m_Obj.GetLogName(), Name(p.m_eTactic), DCO_Log.IdGroup(AIGroup.Cast(p.m_Economy.GetOwner()))));
			}
		}

		switch (p.m_eTactic)
		{
			case DCO_EDefTactic.AREA: ApplyArea(cmd, p, free, objPos, R, side, now); break;
			case DCO_EDefTactic.DEPTH: ApplyDepth(cmd, p, free, objPos, R, side, now); break;
			case DCO_EDefTactic.REVERSE: ApplyReverse(cmd, p, free, objPos, R, side, now); break;
			case DCO_EDefTactic.PERIMETER: ApplyPerimeter(cmd, p, free, objPos, R, now); break;
			case DCO_EDefTactic.DELAY: ApplyDelay(cmd, p, free, objPos, now); break;
		}
		Event(string.Format("def_phase obj=%1 tactic=%2 phase=setup line=all groups=%3 moved=%4 reserve=%5 trigger_m=%6", p.m_Obj.GetLogName(), Name(p.m_eTactic), defs.Count(), free.Count(), p.m_Reserve != null, Math.Round(p.m_fTrigger)));
	}

	protected void ApplyArea(AICommander_BaseComponent cmd, DCO_DefensePlan p, array<DCO_GroupUtilityComponent> free, vector objPos, float R, vector side, float now)
	{
		float depth = R * 0.7;
		if (p.m_bUrban)
			depth = R * 0.5;
		int n = free.Count();
		foreach (int i, DCO_GroupUtilityComponent g : free)
		{
			SetRole(p, g, DCO_EDefRole.MAIN);
			vector rel = DCO_Specialist.Flat(g.GetOwner().GetOrigin() - objPos);
			DCO_AIGarrisonActivity act = g.GetGarrisonActivity();
			if (vector.Dot(rel, p.m_vAxis) < 0.0 && (n < 3 || i < n - 1))
			{
				float lat = (1.0 * i - 0.5 * (n - 1)) * 45.0;
				Place(cmd, g, objPos + p.m_vAxis * depth + side * lat, objPos + p.m_vAxis * (R + 300.0), false, now);
			}
			else if (act && act.IsActive())
				act.DCO_Reface(p.m_vAxis, false);
		}
	}

	protected vector FinalLine(DCO_DefensePlan p, vector objPos, float R)
	{
		float back = Math.Max(100.0, R * 0.6);
		vector guess = objPos - p.m_vAxis * back;
		DCO_GarrisonRegistry reg = DCO_GarrisonRegistry.GetInstance();
		if (reg)
		{
			array<IEntity> buildings = {};
			reg.FindBuildings(guess, 80.0, buildings);
			if (!buildings.IsEmpty())
				return buildings[0].GetOrigin();
		}
		vector cover = DCO_TerrainCache.QueryBest(DCO_ETerrainFlag.FOREST_EDGE | DCO_ETerrainFlag.HILLTOP, guess, 80.0, string.Empty, guess);
		if (cover != vector.Zero)
			return cover;
		return guess;
	}

	protected void ApplyDepth(AICommander_BaseComponent cmd, DCO_DefensePlan p, array<DCO_GroupUtilityComponent> free, vector objPos, float R, vector side, float now)
	{
		p.m_vFwd = DCO_Util.Surface(objPos + p.m_vAxis * (R + 150.0));
		p.m_vMain = DCO_Util.Surface(objPos + p.m_vAxis * R);
		p.m_vFinal = DCO_Util.Surface(FinalLine(p, objPos, R));
		bool small = R <= 60.0;
		DCO_GroupUtilityComponent fwd;
		DCO_GroupUtilityComponent fin;
		float bestF = float.MAX;
		foreach (DCO_GroupUtilityComponent g : free)
		{
			float df = vector.DistanceXZ(g.GetOwner().GetOrigin(), p.m_vFwd);
			if (df < bestF)
			{
				bestF = df;
				fwd = g;
			}
		}
		int frontRaw = Math.Round(DCO_Force.Share(cmd, "defend_front") * free.Count());
		int frontN = Math.ClampInt(frontRaw, DCO_Force.Min(cmd, "defend_front"), free.Count());
		int depthN = free.Count() - frontN;
		Event(string.Format("force_size cmd=%1 mission=defend_front avail=%2 front=%3 depth=%4 obj=%5", cmd.GetCommanderUID(), free.Count(), frontN, depthN, p.m_Obj.GetLogName()));
		if (free.Count() >= 3 && !small && depthN >= 1)
		{
			float bestB = float.MAX;
			foreach (DCO_GroupUtilityComponent gb : free)
			{
				if (gb == fwd)
					continue;
				float db = vector.DistanceXZ(gb.GetOwner().GetOrigin(), p.m_vFinal);
				if (db < bestB)
				{
					bestB = db;
					fin = gb;
				}
			}
		}
		int mains = 0;
		foreach (DCO_GroupUtilityComponent gm : free)
		{
			if (gm == fwd)
			{
				SetRole(p, gm, DCO_EDefRole.FORWARD);
				Place(cmd, gm, p.m_vFwd, p.m_vFwd + p.m_vAxis * 300.0, false, now);
				continue;
			}
			if (gm == fin)
			{
				SetRole(p, gm, DCO_EDefRole.FINAL);
				Place(cmd, gm, p.m_vFinal, p.m_vMain, false, now);
				continue;
			}
			SetRole(p, gm, DCO_EDefRole.MAIN);
			float lat = mains * 45.0 - 22.0;
			mains++;
			Place(cmd, gm, p.m_vMain + side * lat, p.m_vFwd, false, now);
		}
		Event(string.Format("def_phase obj=%1 tactic=DEPTH phase=setup line=lines fwd_m=%2 main_m=%3 final_m=%4 small=%5", p.m_Obj.GetLogName(), Math.Round(R + 150.0), Math.Round(R), Math.Round(vector.DistanceXZ(p.m_vFinal, objPos)), small));
	}

	protected void ApplyReverse(AICommander_BaseComponent cmd, DCO_DefensePlan p, array<DCO_GroupUtilityComponent> free, vector objPos, float R, vector side, float now)
	{
		if (!p.m_bCrest && !FindCrest(p))
		{
			DCO_Perf.Count("def:reverse_nocrest");
			Event(string.Format("def_reverse_nocrest obj=%1 axis=%2 fan=%3 fallback=AREA", p.m_Obj.GetLogName(), p.m_vAxis, DCO_Ctx.TuneForCmd(cmd, "def.crest_fan", 0)));
			ApplyArea(cmd, p, free, objPos, R, side, now);
			return;
		}
		DCO_Perf.Count("def:reverse_crest");
		p.m_fTrigger = p.m_fCrestDist;
		array<vector> eyes = {};
		array<float> fronts = {200.0, 300.0, 400.0};
		foreach (float fd : fronts)
			eyes.Insert(DCO_Util.Surface(p.m_vCrest + p.m_vAxis * fd) + Vector(0, 1.7, 0));
		eyes.Insert(DCO_Util.Surface(p.m_vCrest + p.m_vAxis * 300.0 + side * 80.0) + Vector(0, 1.7, 0));
		eyes.Insert(DCO_Util.Surface(p.m_vCrest + p.m_vAxis * 300.0 - side * 80.0) + Vector(0, 1.7, 0));

		DCO_GroupUtilityComponent op;
		if (free.Count() >= 2)
		{
			int fewest = int.MAX;
			foreach (DCO_GroupUtilityComponent go : free)
			{
				if (go.GetUnitCount() < fewest)
				{
					fewest = go.GetUnitCount();
					op = go;
				}
			}
		}
		int n = 0;
		int hidden = 0;
		array<float> backs = {150.0, 100.0, 200.0, 250.0, 75.0};
		foreach (DCO_GroupUtilityComponent g : free)
		{
			if (g == op)
			{
				SetRole(p, g, DCO_EDefRole.OP);
				Place(cmd, g, p.m_vCrest, p.m_vCrest + p.m_vAxis * 300.0, false, now);
				continue;
			}
			SetRole(p, g, DCO_EDefRole.MAIN);
			float lat = (n - (free.Count() - 1) * 0.5) * 50.0;
			n++;
			vector pick = p.m_vCrest - p.m_vAxis * 150.0 + side * lat;
			foreach (float b : backs)
			{
				vector c = DCO_Util.Surface(p.m_vCrest - p.m_vAxis * b + side * lat);
				bool seen = false;
				foreach (vector eye : eyes)
				{
					if (DCO_CommanderDefense.HasLOS(eye - Vector(0, 1.2, 0), c))
					{
						seen = true;
						break;
					}
				}
				if (!seen && DCO_CommanderDefense.HasLOS(c, p.m_vCrest))
				{
					pick = c;
					hidden++;
					break;
				}
			}
			Place(cmd, g, pick, p.m_vCrest, false, now);
		}
		Event(string.Format("def_phase obj=%1 tactic=REVERSE phase=setup line=reverse crest_m=%2 positions=%3 hidden=%4 op=%5", p.m_Obj.GetLogName(), Math.Round(p.m_fCrestDist), n, hidden, op != null));
	}

	protected void ApplyPerimeter(AICommander_BaseComponent cmd, DCO_DefensePlan p, array<DCO_GroupUtilityComponent> free, vector objPos, float R, float now)
	{
		float rad = R * 0.75;
		if (p.m_bUrban)
			rad = R * 0.55;
		float axisAng = Math.Atan2(p.m_vAxis[2], p.m_vAxis[0]);
		int n = free.Count();
		array<float> angs = {};
		if (n >= 3)
		{
			angs.Insert(axisAng + 35.0 * Math.DEG2RAD);
			angs.Insert(axisAng - 35.0 * Math.DEG2RAD);
			int rest = n - 2;
			for (int i = 0; i < rest; i++)
				angs.Insert(axisAng + Math.PI * 0.5 + (i + 0.5) * Math.PI / rest);
		}
		else if (n == 2)
		{
			angs.Insert(axisAng);
			angs.Insert(axisAng + Math.PI);
		}
		else
			angs.Insert(axisAng);
		foreach (int k, DCO_GroupUtilityComponent g : free)
		{
			SetRole(p, g, DCO_EDefRole.MAIN);
			vector dir = Vector(Math.Cos(angs[k]), 0, Math.Sin(angs[k]));
			Place(cmd, g, objPos + dir * rad, objPos + dir * (R + 300.0), true, now);
		}
		DCO_CommanderDefense def = cmd.GetDefense();
		if (def && cmd.IsOPEnabled() && def.OPCount() < DCO_Force.Max(cmd, "op", cmd.GetMaxOP()))
		{
			vector opPos = DCO_Util.Surface(objPos + p.m_vAxis * Math.Min(R + 300.0, rad + 450.0));
			def.OpenOP(cmd, opPos, objPos + p.m_vAxis * (R + 700.0), "perimeter", now);
		}
	}

	protected void ApplyDelay(AICommander_BaseComponent cmd, DCO_DefensePlan p, array<DCO_GroupUtilityComponent> free, vector objPos, float now)
	{
		p.m_iDelayStep = 0;
		p.m_fNextBound = now + 60.0;
		foreach (DCO_GroupUtilityComponent g : free)
			SetRole(p, g, DCO_EDefRole.DELAY);
		if (free.Count() >= 2)
		{
			DCO_GroupUtilityComponent rear = free[free.Count() - 1];
			Place(cmd, rear, objPos - p.m_vAxis * 150.0, objPos + p.m_vAxis * 300.0, false, now);
		}
	}

	protected bool UpdatePlan(AICommander_BaseComponent cmd, DCO_DefensePlan p, CMD_ThreatResponseComponent tr, float now)
	{
		CMD_AICommanderObjectiveComponent obj = p.m_Obj;
		FactionKey fk = cmd.GetCommanderFactionKey();
		if (!obj.IsCapturedBy(fk))
		{
			Result(cmd, p, "lost", "objective_lost", now);
			return true;
		}
		array<DCO_GroupUtilityComponent> defs = {};
		Defenders(cmd, obj, defs);
		foreach (DCO_GroupUtilityComponent tg : p.m_aGroups)
		{
			if (tg && !defs.Contains(tg) && tg != p.m_Counter && tg != p.m_Spoil && tg.m_iDCODefRole != DCO_EDefRole.NONE)
				tg.m_iDCODefRole = DCO_EDefRole.NONE;
		}
		array<CMD_ThreatEntry> near = {};
		NearThreats(tr, obj, now, near);
		if (near.IsEmpty())
		{
			if (now - p.m_fThreatSeen > 300.0)
			{
				Result(cmd, p, "held", "threat_gone", now);
				return true;
			}
			return false;
		}
		p.m_fThreatSeen = now;
		if (defs.IsEmpty())
		{
			Result(cmd, p, "withdrawn", "no_defenders", now);
			return true;
		}

		bool perimeterAxes;
		vector secondAxis;
		vector oldAxis = p.m_vAxis;
		bool newHeavy = Assess(cmd, p, near, defs, now, perimeterAxes, secondAxis);
		UpdateFeint(cmd, p, defs, near, secondAxis, now);

		string trigger = "none";
		if (newHeavy)
			trigger = "armor_idf";
		else if (p.m_iAxes >= 2 && vector.Dot(oldAxis, p.m_vAxis) <= 0.26 && p.m_eTactic != DCO_EDefTactic.PERIMETER)
			trigger = "new_axis";
		else
		{
			CMD_ThreatEntry inner;
			if (EnemyDistTo(near, obj.GetOwner().GetOrigin(), inner) < obj.GetRadius() * 0.5 && p.m_eTactic != DCO_EDefTactic.DELAY)
				trigger = "breach";
		}
		if (trigger == "none")
		{
			int band = BandHyst(p.m_fRatio, p.m_iBand);
			if (band != p.m_iBand)
			{
				if (band != p.m_iBandPending)
				{
					p.m_iBandPending = band;
					p.m_fBandSince = now;
				}
				else if (now - p.m_fBandSince >= 60.0)
					trigger = "band";
			}
			else
				p.m_iBandPending = -1;
		}
		if (trigger == "none" && p.m_iDocAction >= 0)
		{
			int docBand;
			float docConf;
			int act = DocAction(cmd, p, defs, near, perimeterAxes, now, docBand, docConf);
			if (act == p.m_iDocAction)
			{
				p.m_iDocBand = docBand;
				p.m_fDocConf = docConf;
				p.m_iDocPending = -1;
			}
			else if (act != p.m_iDocPending)
			{
				p.m_iDocPending = act;
				p.m_fDocSince = now;
			}
			else if (now - p.m_fDocSince >= 60.0)
				trigger = "doctrine";
		}
		if (trigger == "none" && now - p.m_fChosenAt >= 300.0 && now - p.m_fLastReeval >= 300.0)
			trigger = "reeval";
		if (trigger != "none" && (trigger != "breach" || now - p.m_fChosenAt >= 60.0))
			Select(cmd, p, near, defs, now, "switch", trigger);
		if (p.m_bEnded)
			return true;
		if (p.m_iBand < 0)
			p.m_iBand = Band(p.m_fRatio);

		switch (p.m_eTactic)
		{
			case DCO_EDefTactic.DEPTH: UpdateDepth(cmd, p, defs, near, now); break;
			case DCO_EDefTactic.REVERSE: UpdateReverse(cmd, p, near, now); break;
			case DCO_EDefTactic.DELAY:
			{
				if (UpdateDelay(cmd, p, defs, near, now))
				{
					Result(cmd, p, "delayed", "bounds_done", now);
					return true;
				}
				break;
			}
		}
		UpdateFireControl(cmd, p, defs, near, now);
		UpdateEA(cmd, p, defs, near, now);
		UpdateCounter(cmd, p, defs, near, now);
		UpdateSpoil(cmd, p, defs, now);
		UpdateFPF(cmd, p, defs, near, now);
		UpdateAlternates(cmd, p, defs, near, now);
		if (!p.m_Reserve && p.m_eTactic != DCO_EDefTactic.DELAY && p.m_eTactic != DCO_EDefTactic.REVERSE && defs.Count() >= 3 && !p.m_Counter && DCO_Force.Reserve(cmd, defs.Count()) >= 1)
		{
			array<DCO_GroupUtilityComponent> cand = {};
			foreach (DCO_GroupUtilityComponent cg : defs)
			{
				if (cg != p.m_Spoil && !CloseContact(cg, near))
					cand.Insert(cg);
			}
			p.m_Reserve = LeastEngaged(p, cand, near);
			if (p.m_Reserve)
			{
				SetRole(p, p.m_Reserve, DCO_EDefRole.RESERVE);
				vector objPos = obj.GetOwner().GetOrigin();
				p.m_vReservePos = DCO_LearnDefense.ReservePos(cmd, obj, objPos - p.m_vAxis * Math.Min(obj.GetRadius() * 0.5, 80.0));
				Place(cmd, p.m_Reserve, p.m_vReservePos, objPos + p.m_vAxis * obj.GetRadius(), false, now);
				Event(string.Format("def_phase obj=%1 tactic=%2 phase=reserve_rebuilt line=reserve grp=%3", obj.GetLogName(), Name(p.m_eTactic), DCO_Log.IdGroup(AIGroup.Cast(p.m_Reserve.GetOwner()))));
			}
		}
		return false;
	}

	protected void UpdateFeint(AICommander_BaseComponent cmd, DCO_DefensePlan p, array<DCO_GroupUtilityComponent> defs, array<CMD_ThreatEntry> near, vector secondAxis, float now)
	{
		if (p.m_bMainConfirmed)
			return;
		if (p.m_fMainShare >= 0.6)
		{
			if (p.m_fShareSince < 0)
				p.m_fShareSince = now;
			if (now - p.m_fShareSince >= 90.0)
			{
				p.m_bMainConfirmed = true;
				DCO_Perf.Count("def:main_confirmed");
				Event(string.Format("def_phase obj=%1 tactic=%2 phase=main_confirmed line=all share=%3", p.m_Obj.GetLogName(), Name(p.m_eTactic), p.m_fMainShare.ToString(-1, 2)));
				if (p.m_Economy && !CloseContact(p.m_Economy, near))
				{
					DCO_AIGarrisonActivity act = p.m_Economy.GetGarrisonActivity();
					if (act && act.IsActive())
						act.DCO_Reface(p.m_vAxis, false);
					p.m_Economy = null;
				}
			}
		}
		else
			p.m_fShareSince = -1;
	}

	protected void Fallback(AICommander_BaseComponent cmd, DCO_DefensePlan p, DCO_GroupUtilityComponent g, int toRole, vector to, CMD_ThreatEntry threat, string trigger, bool ready, float now)
	{
		string from = RoleName(g.m_iDCODefRole);
		DCO_Perf.Count("def:fallback");
		Event(string.Format("def_fallback obj=%1 grp=%2 from_line=%3 to_line=%4 trigger=%5 next_ready=%6", p.m_Obj.GetLogName(), DCO_Log.IdGroup(AIGroup.Cast(g.GetOwner())), from, RoleName(toRole), trigger, ready));
		if (!ready)
			return;
		p.m_iFallbacks++;
		g.m_iDCODefRole = toRole;
		vector threatPos = p.m_vThreat;
		if (threat)
			threatPos = threat.m_vBelievedPos;
		if (!DCO_BreakContact.Start(g, threatPos, to, "def_fallback"))
			Place(cmd, g, to, p.m_Obj.GetOwner().GetOrigin() + p.m_vAxis * 300.0, false, now);
		Event(string.Format("def_alt obj=%1 grp=%2 kind=subsequent reason=depth", p.m_Obj.GetLogName(), DCO_Log.IdGroup(AIGroup.Cast(g.GetOwner()))));
		DCO_Perf.Count("def:alt:subsequent");
		DCO_Briefing.Defense(cmd, p, DCO_EBriefKind.PHASE, "brief_phase_fallback", DCO_Radio.P("line", from, "to", RoleName(toRole), "obj", p.m_Obj.GetDisplayName()));
	}

	protected bool LineReady(DCO_DefensePlan p, array<DCO_GroupUtilityComponent> defs, int role, vector linePos, vector from)
	{
		foreach (DCO_GroupUtilityComponent g : defs)
		{
			if (g.m_iDCODefRole != role || g.m_DCOBreak || g.GetPhase() == DCO_ETaskPhase.MOVING)
				continue;
			if (vector.DistanceXZ(g.GetOwner().GetOrigin(), linePos) <= 80.0 && DCO_CommanderDefense.HasLOS(g.GetOwner().GetOrigin(), from))
				return true;
		}
		return false;
	}

	protected void UpdateDepth(AICommander_BaseComponent cmd, DCO_DefensePlan p, array<DCO_GroupUtilityComponent> defs, array<CMD_ThreatEntry> near, float now)
	{
		foreach (DCO_GroupUtilityComponent g : defs)
		{
			if (g.m_DCOBreak)
				continue;
			vector gp = g.GetOwner().GetOrigin();
			CMD_ThreatEntry t;
			float d = EnemyDistTo(near, gp, t);
			if (g.m_iDCODefRole == DCO_EDefRole.FORWARD)
			{
				bool manoeuvre = t && t.m_bMoving && d < 400.0;
				if (d <= 300.0 || manoeuvre)
				{
					string why = "enemy_close";
					if (d > 300.0)
						why = "manoeuvre";
					Fallback(cmd, p, g, DCO_EDefRole.MAIN, p.m_vMain, t, why, true, now);
				}
				continue;
			}
			if (g.m_iDCODefRole != DCO_EDefRole.MAIN)
				continue;
			string trig = string.Empty;
			if (d <= 150.0)
				trig = "release_line";
			else if (t && d < 300.0 && Math.AbsFloat(vector.Dot(DCO_Specialist.Flat(t.m_vBelievedPos - gp), p.m_vAxis)) < 0.4)
				trig = "flank";
			else if (g.HasState(DCO_EGroupState.SUPPRESSED) && d < 100.0)
				trig = "suppressed_assault";
			else if (g.GetUnitCount() <= 1.0 * p.StartUnits(g) * 0.7)
				trig = "losses";
			else if (t && g.GetUnitCount() < ThreatWeight(t) * 0.5)
				trig = "ratio";
			if (trig.IsEmpty())
				continue;
			bool ready = LineReady(p, defs, DCO_EDefRole.FINAL, p.m_vFinal, gp);
			if (!ready && g.m_fDCODefHoldLog > now)
				continue;
			g.m_fDCODefHoldLog = now + 60.0;
			Fallback(cmd, p, g, DCO_EDefRole.FINAL, p.m_vFinal, t, trig, ready, now);
		}
	}

	protected void UpdateReverse(AICommander_BaseComponent cmd, DCO_DefensePlan p, array<CMD_ThreatEntry> near, float now)
	{
		if (!p.m_bCrest)
			return;
		CMD_ThreatEntry t;
		float dCrest = EnemyDistTo(near, p.m_vCrest, t);
		foreach (DCO_GroupUtilityComponent g : p.m_aGroups)
		{
			if (!g || g.m_iDCODefRole != DCO_EDefRole.OP || g.m_DCOBreak)
				continue;
			if (dCrest <= 300.0)
			{
				g.m_iDCODefRole = DCO_EDefRole.MAIN;
				vector back = p.m_vCrest - p.m_vAxis * 150.0;
				if (!DCO_BreakContact.Start(g, t.m_vBelievedPos, back, "def_op_withdraw"))
					Place(cmd, g, back, p.m_vCrest, false, now);
				Event(string.Format("def_phase obj=%1 tactic=REVERSE phase=fallback line=op grp=%2 enemy_m=%3", p.m_Obj.GetLogName(), DCO_Log.IdGroup(AIGroup.Cast(g.GetOwner())), Math.Round(dCrest)));
			}
		}
		if (t && now - p.m_fLastForwardFire > 120.0 && dCrest < 400.0 && vector.Dot(DCO_Specialist.Flat(t.m_vBelievedPos - p.m_vCrest), p.m_vAxis) > 0.3)
		{
			p.m_fLastForwardFire = now;
			if (Mortar(cmd, t.m_vBelievedPos, 3, "def_reverse_slope"))
				Event(string.Format("def_phase obj=%1 tactic=REVERSE phase=forward_slope_fire line=mortar enemy_m=%2", p.m_Obj.GetLogName(), Math.Round(dCrest)));
		}
	}

	protected bool UpdateDelay(AICommander_BaseComponent cmd, DCO_DefensePlan p, array<DCO_GroupUtilityComponent> defs, array<CMD_ThreatEntry> near, float now)
	{
		array<DCO_GroupUtilityComponent> line = {};
		foreach (DCO_GroupUtilityComponent g : defs)
		{
			if (g.m_iDCODefRole == DCO_EDefRole.DELAY)
				line.Insert(g);
		}
		if (line.IsEmpty())
			return p.m_iDelayStep > 0;
		DCO_GroupUtilityComponent front;
		float frontD = float.MAX;
		CMD_ThreatEntry ft;
		foreach (DCO_GroupUtilityComponent lg : line)
		{
			CMD_ThreatEntry lt;
			float ld = EnemyDistTo(near, lg.GetOwner().GetOrigin(), lt);
			if (ld < frontD)
			{
				frontD = ld;
				front = lg;
				ft = lt;
			}
		}
		if (!front || front.m_DCOBreak)
			return false;
		if (frontD > 250.0 && now < p.m_fNextBound)
			return false;
		if (frontD > 450.0)
			return false;
		if (p.m_iDelayStep >= 3)
			return true;
		vector objPos = p.m_Obj.GetOwner().GetOrigin();
		p.m_iDelayStep++;
		vector next = DCO_Util.Surface(objPos - p.m_vAxis * (150.0 * (p.m_iDelayStep + 1)));
		p.m_fNextBound = now + 90.0;
		int moveN = Math.Round(DCO_Force.Share(cmd, "delay_move") * line.Count());
		if (moveN < 1)
			moveN = 1;
		array<DCO_GroupUtilityComponent> movers = {front};
		while (movers.Count() < moveN)
		{
			DCO_GroupUtilityComponent nx;
			float nd = float.MAX;
			foreach (DCO_GroupUtilityComponent cand : line)
			{
				if (movers.Contains(cand) || cand.m_DCOBreak)
					continue;
				CMD_ThreatEntry ct;
				float cd = EnemyDistTo(near, cand.GetOwner().GetOrigin(), ct);
				if (cd < nd)
				{
					nd = cd;
					nx = cand;
				}
			}
			if (!nx)
				break;
			movers.Insert(nx);
		}
		string mode = "subsequent";
		if (line.Count() >= 2 && movers.Count() < line.Count())
			mode = "alternating";
		else if (line.Count() >= 2)
			mode = "together";
		Event(string.Format("force_size cmd=%1 mission=delay_move avail=%2 result=%3 obj=%4", cmd.GetCommanderUID(), line.Count(), movers.Count(), p.m_Obj.GetLogName()));
		DCO_Perf.Count("def:delay_bound");
		Event(string.Format("def_phase obj=%1 tactic=DELAY phase=fallback line=%2 grp=%3 step=%4 enemy_m=%5", p.m_Obj.GetLogName(), mode, DCO_Log.IdGroup(AIGroup.Cast(front.GetOwner())), p.m_iDelayStep, Math.Round(frontD)));
		vector threatPos = p.m_vThreat;
		if (ft)
			threatPos = ft.m_vBelievedPos;
		foreach (DCO_GroupUtilityComponent mv : movers)
		{
			if (!DCO_BreakContact.Start(mv, threatPos, next, "def_delay"))
				Place(cmd, mv, next, objPos + p.m_vAxis * 300.0, false, now);
		}
		return false;
	}

	protected void UpdateFireControl(AICommander_BaseComponent cmd, DCO_DefensePlan p, array<DCO_GroupUtilityComponent> defs, array<CMD_ThreatEntry> near, float now)
	{
		if (p.m_bFired || p.m_eTactic == DCO_EDefTactic.DELAY)
			return;
		vector objPos = p.m_Obj.GetOwner().GetOrigin();
		CMD_ThreatEntry t;
		float d = EnemyDistTo(near, objPos, t);
		bool crossed = d <= p.m_fTrigger;
		foreach (DCO_GroupUtilityComponent g : defs)
		{
			int role = g.m_iDCODefRole;
			if (role != DCO_EDefRole.MAIN && role != DCO_EDefRole.FINAL && role != DCO_EDefRole.FORWARD && role != DCO_EDefRole.RESERVE)
				continue;
			if (crossed || g.HasState(DCO_EGroupState.SUPPRESSED) || CloseContact(g, near))
				DCO_CommanderDefense.SetFire(g, true);
			else
				DCO_CommanderDefense.SetFire(g, false);
		}
		if (!crossed)
			return;
		p.m_bFired = true;
		string line = "trigger_line";
		if (p.m_eTactic == DCO_EDefTactic.REVERSE)
			line = "crest";
		Event(string.Format("def_phase obj=%1 tactic=%2 phase=hold line=%3 fire=open enemy_m=%4", p.m_Obj.GetLogName(), Name(p.m_eTactic), line, Math.Round(d)));
		DCO_Briefing.Defense(cmd, p, DCO_EBriefKind.PHASE, "brief_phase_fire", DCO_Radio.P("dist", DCO_Radio.N(d), "obj", p.m_Obj.GetDisplayName()));
	}

	static bool Mortar(AICommander_BaseComponent cmd, vector target, int rounds, string source)
	{
		CMD_ArtillerySupport arty = cmd.GetArtySupport();
		if (!arty || !arty.HasRegisteredUnits())
			return false;
		CMD_FireMissionRequest req = new CMD_FireMissionRequest(target, SCR_EAIArtilleryAmmoType.HIGH_EXPLOSIVE, DCO_Util.NowS(), rounds);
		arty.ApplyTier(req, "area", 50.0, source);
		if (arty.HasFriendlyNearRequest(req, DCO_Util.NowS()))
		{
			CMD_ArtillerySupport.LogDenied(req, "friendly");
			return false;
		}
		arty.RequestShellImpact(req, DCO_Util.NowS(), rounds);
		return true;
	}

	protected void UpdateEA(AICommander_BaseComponent cmd, DCO_DefensePlan p, array<DCO_GroupUtilityComponent> defs, array<CMD_ThreatEntry> near, float now)
	{
		if (!Allowed(cmd, DCO_EDefTacticFlag.ENGAGEMENT_AREA) || p.m_eTactic == DCO_EDefTactic.DELAY)
			return;
		vector objPos = p.m_Obj.GetOwner().GetOrigin();
		float R = p.m_Obj.GetRadius();
		if (!p.m_bEA)
		{
			vector probe = DCO_Util.Surface(objPos + p.m_vAxis * (R + 250.0));
			bool road = false;
			RoadNetworkManager roads = DCO_CommanderDefense.Roads();
			if (roads)
			{
				BaseRoad br;
				float rd;
				roads.GetClosestRoad(probe, br, rd);
				road = br && rd <= 60.0;
			}
			array<DCO_TerrainPoint> pts = {};
			bool open = DCO_TerrainCache.Query(DCO_ETerrainFlag.FOREST_EDGE, probe, 150.0, pts) < 2;
			if (!road && !open)
				return;
			p.m_bEA = true;
			p.m_vEACenter = probe;
			DCO_Battle.Place(cmd, p.m_Obj, null, "engagement_area", p.m_vEACenter, Math.Atan2(-p.m_vAxis[0], -p.m_vAxis[2]) * Math.RAD2DEG, "-", "-", "-", Name(p.m_eTactic));
			if (p.m_eTactic != DCO_EDefTactic.REVERSE)
				p.m_fTrigger = R + DCO_Ctx.TuneForCmd(cmd, "def.trigger_extra", 150.0);
			int mg = 0;
			int at = 0;
			foreach (DCO_GroupUtilityComponent g : defs)
			{
				if (g.HasMG())
					mg++;
				if (g.HasAT())
					at++;
			}
			DCO_CommanderDefense def = cmd.GetDefense();
			if (def)
				def.RequestApproachAmbush(cmd, p.m_Obj, p.m_vThreat, p.m_bArmor, now);
			bool mortar = cmd.GetArtySupport() && cmd.GetArtySupport().HasRegisteredUnits();
			DCO_Perf.Count("def:ea");
			string kind = "open";
			if (road)
				kind = "road";
			Event(string.Format("def_ea obj=%1 trigger_line=%2 mg=%3 at=%4 mortar=%5 fired=0 kind=%6", p.m_Obj.GetLogName(), Math.Round(p.m_fTrigger), mg, at, mortar, kind));
			return;
		}
		if (p.m_bEAFire)
			return;
		CMD_ThreatEntry t;
		if (EnemyDistTo(near, p.m_vEACenter, t) > 100.0)
			return;
		p.m_bEAFire = true;
		bool fired = Mortar(cmd, p.m_vEACenter, 4, "def_ea");
		Event(string.Format("def_ea obj=%1 trigger_line=%2 mg=0 at=0 mortar=%3 fired=1", p.m_Obj.GetLogName(), Math.Round(p.m_fTrigger), fired));
	}

	protected static bool SegCross(vector a, vector b, vector c, vector d)
	{
		float d1 = (b[0] - a[0]) * (c[2] - a[2]) - (b[2] - a[2]) * (c[0] - a[0]);
		float d2 = (b[0] - a[0]) * (d[2] - a[2]) - (b[2] - a[2]) * (d[0] - a[0]);
		float d3 = (d[0] - c[0]) * (a[2] - c[2]) - (d[2] - c[2]) * (a[0] - c[0]);
		float d4 = (d[0] - c[0]) * (b[2] - c[2]) - (d[2] - c[2]) * (b[0] - c[0]);
		return d1 * d2 < 0 && d3 * d4 < 0;
	}

	protected void UpdateCounter(AICommander_BaseComponent cmd, DCO_DefensePlan p, array<DCO_GroupUtilityComponent> defs, array<CMD_ThreatEntry> near, float now)
	{
		vector objPos = p.m_Obj.GetOwner().GetOrigin();
		float R = p.m_Obj.GetRadius();
		if (p.m_Counter)
		{
			DCO_GroupUtilityComponent c = p.m_Counter;
			string res = string.Empty;
			if (!c.GetOwner() || c.GetUnitCount() == 0)
				res = "aborted";
			else if (vector.DistanceXZ(c.GetOwner().GetOrigin(), objPos) > R + 250.0)
				res = "limit_reached";
			else if (now - p.m_fCounterAt > p.m_fCounterWin)
				res = "returned";
			if (res.IsEmpty())
				return;
			p.m_Counter = null;
			p.m_fCounterCd = now + 120.0;
			DCO_Perf.Count("def:counter:" + res);
			Event(string.Format("def_counter obj=%1 grp=%2 fixed_s=0 dist=0 result=%3", p.m_Obj.GetLogName(), DCO_Log.IdGroup(AIGroup.Cast(c.GetOwner())), res));
			if (c.GetOwner() && c.GetUnitCount() > 0)
			{
				c.SetTask(DCO_EGroupTask.DEFEND);
				SetRole(p, c, DCO_EDefRole.RESERVE);
				p.m_Reserve = c;
				Place(cmd, c, p.m_vReservePos, objPos + p.m_vAxis * R, false, now);
			}
			return;
		}
		if (!Allowed(cmd, DCO_EDefTacticFlag.COUNTERATTACK) || now < p.m_fCounterCd || p.m_eTactic == DCO_EDefTactic.DELAY)
			return;
		if (!DCO_DoctrineDefense.Allows(cmd, p.m_iDocBand, DCO_EDefenseCol.COUNTERATTACK))
			return;
		foreach (DCO_GroupUtilityComponent hg : defs)
		{
			if (hg.m_DCOBreak)
				return;
		}
		CMD_ThreatEntry fixedT;
		float fixedS = 0;
		foreach (CMD_ThreatEntry t : near)
		{
			if (!t.m_bFiringAtUs || vector.DistanceXZ(t.m_vBelievedPos, objPos) > R + 150.0)
				continue;
			float still = StillFor(t, now);
			if (still >= 60.0 && still > fixedS)
			{
				fixedS = still;
				fixedT = t;
			}
		}
		if (!fixedT)
			return;
		if (DCO_Force.Max(cmd, "counter") < 1)
			return;
		DCO_GroupUtilityComponent res2 = p.m_Reserve;
		if (!res2 && defs.Count() == 2)
			res2 = LeastEngaged(p, defs, near);
		if (!res2 || res2.HasState(DCO_EGroupState.IN_CONTACT))
			return;

		vector ep = fixedT.m_vBelievedPos;
		vector toE = DCO_Specialist.Flat(ep - objPos);
		vector flank = Vector(-toE[2], 0, toE[0]);
		vector from = res2.GetOwner().GetOrigin();
		vector best = vector.Zero;
		float bestCover = -1;
		array<float> signs = {1.0, -1.0};
		foreach (float sg : signs)
		{
			vector goal = DCO_Util.Surface(ep + flank * (150.0 * sg));
			bool crosses = false;
			foreach (DCO_GroupUtilityComponent fg : defs)
			{
				if (fg != res2 && SegCross(from, goal, fg.GetOwner().GetOrigin(), ep))
				{
					crosses = true;
					break;
				}
			}
			if (crosses)
				continue;
			array<DCO_TerrainPoint> cpts = {};
			float cover = DCO_TerrainCache.Query(DCO_ETerrainFlag.FOREST_EDGE, (from + goal) * 0.5, 120.0, cpts);
			if (cover > bestCover)
			{
				bestCover = cover;
				best = goal;
			}
		}
		if (best == vector.Zero)
		{
			DCO_Perf.Count("def:counter:no_route");
			return;
		}
		p.m_Counter = res2;
		if (p.m_Reserve == res2)
			p.m_Reserve = null;
		p.m_fCounterAt = now;
		p.m_fCounterWin = Math.RandomFloat(300.0, 600.0);
		p.m_iCounters++;
		SetRole(p, res2, DCO_EDefRole.COUNTER);
		res2.CompleteAllWaypoints();
		res2.SetTask(DCO_EGroupTask.FLANK);
		if (cmd.SpawnMoveRoute(res2, from, best, now))
		{
			SCR_AIWaypoint sweep = cmd.SpawnMoveWP(ep);
			if (sweep)
				res2.MoveTo(sweep, now);
		}
		DCO_Perf.Count("def:counter");
		Event(string.Format("def_counter obj=%1 grp=%2 fixed_s=%3 dist=%4 result=start cover=%5", p.m_Obj.GetLogName(), DCO_Log.IdGroup(AIGroup.Cast(res2.GetOwner())), Math.Round(fixedS), Math.Round(vector.DistanceXZ(from, best)), bestCover));
		if (cmd.GetDoctrine())
			DCO_DoctrineDefense.Log(cmd, p.m_Obj, "counterattack", p.m_iDocBand, p.m_fRatio, p.m_fDocConf, " scope=local window_s=" + Math.Round(p.m_fCounterWin).ToString());
	}

	protected void UpdateSpoil(AICommander_BaseComponent cmd, DCO_DefensePlan p, array<DCO_GroupUtilityComponent> defs, float now)
	{
		vector objPos = p.m_Obj.GetOwner().GetOrigin();
		if (p.m_Spoil)
		{
			DCO_GroupUtilityComponent s = p.m_Spoil;
			string res = string.Empty;
			if (!s.GetOwner() || s.GetUnitCount() == 0)
				res = "aborted";
			else
			{
				if (p.m_fSpoilContact < 0 && s.HasState(DCO_EGroupState.IN_CONTACT))
					p.m_fSpoilContact = now;
				if (p.m_fSpoilContact >= 0 && now - p.m_fSpoilContact > 180.0)
					res = "contact_limit";
				else if (now - p.m_fSpoilAt > 300.0)
					res = "returned";
			}
			if (res.IsEmpty())
				return;
			p.m_Spoil = null;
			p.m_fSpoilCd = now + 600.0;
			string kindName = "abf";
			if (p.m_iSpoilKind == DCO_EDefRole.SPOIL_RAID)
				kindName = "raid";
			Event(string.Format("def_spoil obj=%1 kind=%2 target_dist=0 ratio=0 groups_left=%3 result=%4", p.m_Obj.GetLogName(), kindName, defs.Count(), res));
			if (s.GetOwner() && s.GetUnitCount() > 0)
			{
				s.SetTask(DCO_EGroupTask.DEFEND);
				SetRole(p, s, DCO_EDefRole.MAIN);
				Place(cmd, s, p.m_vSpoilBack, objPos + p.m_vAxis * 300.0, false, now);
			}
			return;
		}
		if (!Allowed(cmd, DCO_EDefTacticFlag.SPOILING_ATTACK) || now < p.m_fSpoilCd)
			return;
		CMD_ThreatResponseComponent tr = cmd.GetThreatResponseComponent();
		if (!tr)
			return;
		CMD_ThreatEntry target;
		float targetD = 0;
		int others = 0;
		foreach (CMD_ThreatEntry t : tr.GetThreats())
		{
			if (!t)
				continue;
			float d = vector.DistanceXZ(t.m_vBelievedPos, objPos);
			if (d > p.m_Obj.GetRadius() + 600.0)
				continue;
			if (d < 300.0 || d > 800.0 || t.m_bFiringAtUs || t.m_iEstimatedEnemyCount < 3 || StillFor(t, now) < 90.0)
			{
				others++;
				continue;
			}
			if (!target || d < targetD)
			{
				target = t;
				targetD = d;
			}
		}
		if (!target)
			return;
		p.m_iSpoils++;
		p.m_fSpoilCd = now + 600.0;
		if (Mortar(cmd, target.m_vBelievedPos, 4, "def_spoil"))
		{
			DCO_Perf.Count("def:spoil:abf_mortar");
			Event(string.Format("def_spoil obj=%1 kind=abf target_dist=%2 ratio=%3 groups_left=%4 result=start via=mortar", p.m_Obj.GetLogName(), Math.Round(targetD), p.m_fRatio.ToString(-1, 2), defs.Count()));
			return;
		}
		DCO_StrengthInfo info = new DCO_StrengthInfo();
		DCO_GroupUtilityComponent raider;
		DCO_GroupUtilityComponent mgGroup;
		float bestS = 0;
		foreach (DCO_GroupUtilityComponent g : defs)
		{
			if (g.HasState(DCO_EGroupState.IN_CONTACT) || g.m_DCOBreak || g == p.m_Counter || !g.IsInfantry())
				continue;
			if (g.HasMG() && !mgGroup)
				mgGroup = g;
			DCO_Strength.OfGroup(SCR_AIGroup.Cast(g.GetOwner()), false, info);
			if (info.m_fStrength > bestS)
			{
				bestS = info.m_fStrength;
				raider = g;
			}
		}
		vector tp = target.m_vBelievedPos;
		bool covered = false;
		array<DCO_TerrainPoint> pts = {};
		covered = DCO_TerrainCache.Query(DCO_ETerrainFlag.FOREST_EDGE, (objPos + tp) * 0.5, 150.0, pts) >= 2;
		float localRatio = bestS / Math.Max(ThreatWeight(target), 1.0);
		bool raid = raider && (defs.Count() - 1 >= 2 || others == 0) && targetD <= 500.0 && covered && localRatio >= 1.0;
		DCO_GroupUtilityComponent use = mgGroup;
		int kind = DCO_EDefRole.SPOIL_ABF;
		if (raid)
		{
			use = raider;
			kind = DCO_EDefRole.SPOIL_RAID;
		}
		if (!use)
		{
			Event(string.Format("def_spoil obj=%1 kind=abf target_dist=%2 ratio=%3 groups_left=%4 result=no_group", p.m_Obj.GetLogName(), Math.Round(targetD), localRatio.ToString(-1, 2), defs.Count()));
			return;
		}
		p.m_Spoil = use;
		p.m_iSpoilKind = kind;
		p.m_fSpoilAt = now;
		p.m_fSpoilContact = -1;
		p.m_vSpoilBack = use.GetOwner().GetOrigin();
		SetRole(p, use, kind);
		use.CompleteAllWaypoints();
		string kindText = "abf";
		if (raid)
		{
			kindText = "raid";
			use.SetTask(DCO_EGroupTask.ATTACK);
			cmd.SpawnMoveRoute(use, use.GetOwner().GetOrigin(), tp, now);
		}
		else
		{
			vector toT = DCO_Specialist.Flat(tp - objPos);
			vector firePos = DCO_Util.Surface(tp - toT * 600.0);
			use.SetTask(DCO_EGroupTask.SUPPORT_BY_FIRE);
			Place(cmd, use, firePos, tp, false, now);
			SCR_AIWaypoint sup = cmd.SpawnSuppressWP(tp);
			if (sup)
				use.MoveTo(sup, now);
		}
		DCO_Perf.Count("def:spoil:" + kindText);
		Event(string.Format("def_spoil obj=%1 kind=%2 target_dist=%3 ratio=%4 groups_left=%5 result=start", p.m_Obj.GetLogName(), kindText, Math.Round(targetD), localRatio.ToString(-1, 2), defs.Count() - 1));
	}

	protected void UpdateFPF(AICommander_BaseComponent cmd, DCO_DefensePlan p, array<DCO_GroupUtilityComponent> defs, array<CMD_ThreatEntry> near, float now)
	{
		if (now - p.m_fFPFAt < 180.0 || p.m_eTactic == DCO_EDefTactic.DELAY)
			return;
		int finalRole = DCO_EDefRole.MAIN;
		if (p.m_eTactic == DCO_EDefTactic.DEPTH)
			finalRole = DCO_EDefRole.FINAL;
		int mg = 0;
		CMD_ThreatEntry assault;
		foreach (DCO_GroupUtilityComponent g : defs)
		{
			if (g.m_iDCODefRole != finalRole)
				continue;
			if (g.HasMG())
				mg++;
			CMD_ThreatEntry t;
			if (EnemyDistTo(near, g.GetOwner().GetOrigin(), t) <= 100.0 && t && t.m_bMoving)
				assault = t;
		}
		if (!assault)
			return;
		p.m_fFPFAt = now;
		bool fired = Mortar(cmd, assault.m_vBelievedPos, 4, "def_fpf");
		foreach (DCO_GroupUtilityComponent fg : defs)
		{
			if (fg.m_iDCODefRole == finalRole)
				DCO_CommanderDefense.SetFire(fg, true);
		}
		DCO_Perf.Count("def:fpf");
		Event(string.Format("def_fpf obj=%1 fpl_mg=%2 fpf_mortar=%3", p.m_Obj.GetLogName(), mg, fired));
	}

	protected void UpdateAlternates(AICommander_BaseComponent cmd, DCO_DefensePlan p, array<DCO_GroupUtilityComponent> defs, array<CMD_ThreatEntry> near, float now)
	{
		vector side = Vector(-p.m_vAxis[2], 0, p.m_vAxis[0]);
		foreach (DCO_GroupUtilityComponent g : defs)
		{
			int role = g.m_iDCODefRole;
			if (role != DCO_EDefRole.MAIN && role != DCO_EDefRole.FINAL && role != DCO_EDefRole.FORWARD)
				continue;
			if (!g.HasState(DCO_EGroupState.SUPPRESSED) || g.m_DCOBreak)
			{
				p.m_mSuppSince.Remove(g);
				continue;
			}
			float since;
			if (!p.m_mSuppSince.Find(g, since))
			{
				p.m_mSuppSince.Set(g, now);
				continue;
			}
			float lastAlt;
			if (p.m_mAltAt.Find(g, lastAlt) && now - lastAlt < 120.0)
				continue;
			CMD_ThreatEntry t;
			if (now - since < 30.0 || EnemyDistTo(near, g.GetOwner().GetOrigin(), t) < 60.0)
				continue;
			p.m_mAltAt.Set(g, now);
			p.m_mSuppSince.Remove(g);
			float sgn = 1.0;
			if (Math.RandomFloat01() < 0.5)
				sgn = -1.0;
			vector alt = g.GetOwner().GetOrigin() + side * (40.0 * sgn);
			Place(cmd, g, alt, alt + p.m_vAxis * 300.0, p.m_eTactic == DCO_EDefTactic.PERIMETER, now);
			DCO_Perf.Count("def:alt:alternate");
			Event(string.Format("def_alt obj=%1 grp=%2 kind=alternate reason=suppressed", p.m_Obj.GetLogName(), DCO_Log.IdGroup(AIGroup.Cast(g.GetOwner()))));
		}
	}

	protected void UpdateStandTo(AICommander_BaseComponent cmd, CMD_ThreatResponseComponent tr, float now)
	{
		int level = 25;
		if (!m_mPlans.IsEmpty())
			level = 100;
		else if (tr)
		{
			FactionKey fk = cmd.GetCommanderFactionKey();
			foreach (CMD_AICommanderObjectiveComponent o : cmd.DCO_AllObjectives())
			{
				if (!o || !o.GetOwner() || !o.IsCapturedBy(fk))
					continue;
				foreach (CMD_ThreatEntry t : tr.GetThreats())
				{
					if (t && vector.DistanceXZ(t.m_vBelievedPos, o.GetOwner().GetOrigin()) <= o.GetRadius() + 1500.0)
						level = 50;
				}
			}
		}
		string why = "threat";
		if (level < 100 && TwilightNear())
		{
			level = 100;
			why = "dawn_dusk";
		}
		else if (level == 25)
			why = "quiet";
		if (level == m_iStandTo)
			return;
		m_iStandTo = level;
		DCO_Perf.Count("def:standto:" + level.ToString());
		Event(string.Format("def_standto obj=all cmd=%1 level=%2 reason=%3", cmd.GetCommanderUID(), level, why));
	}

	int GetStandTo()
	{
		return m_iStandTo;
	}

	protected static bool TwilightNear()
	{
		ChimeraWorld world = ChimeraWorld.CastFrom(GetGame().GetWorld());
		if (!world)
			return false;
		TimeAndWeatherManagerEntity manager = world.GetTimeAndWeatherManager();
		if (!manager)
			return false;
		float rise, sunset;
		if (!manager.GetSunriseHour(rise) || !manager.GetSunsetHour(sunset))
			return false;
		float cur = manager.GetTimeOfTheDay();
		return Math.AbsFloat(cur - rise) <= 0.5 || Math.AbsFloat(cur - sunset) <= 0.5;
	}

	protected void Result(AICommander_BaseComponent cmd, DCO_DefensePlan p, string result, string reason, float now)
	{
		int start = 0;
		int alive = 0;
		foreach (int i, DCO_GroupUtilityComponent g : p.m_aGroups)
		{
			start += p.m_aStartUnits[i];
			if (g && g.GetOwner())
			{
				alive += g.GetUnitCount();
				g.m_iDCODefRole = DCO_EDefRole.NONE;
				g.DCO_SetHold(0);
				SCR_AIGroupUtilityComponent u = g.GetGroupUtilityComponent();
				if (u)
					u.SetCombatMode(EAIGroupCombatMode.RETURN_FIRE);
			}
		}
		float losses = 1.0 - 1.0 * alive / Math.Max(start, 1.0);
		DCO_Perf.Count("def:result:" + result);
		Event(string.Format("tactic_result obj=%1 tactic=DEF_%2 result=%3 reason=%4 duration_s=%5 switches=%6 losses=%7 fallbacks=%8 counters=%9",
			p.m_Obj.GetLogName(), Name(p.m_eTactic), result, reason, Math.Round(now - p.m_fStart), p.m_iSwitches, losses.ToString(-1, 2), p.m_iFallbacks, p.m_iCounters) + string.Format(" spoils=%1", p.m_iSpoils));
		DCO_Briefing.DefenseEnd(p, result);
	}

	static DCO_GroupTactics RolePosture(DCO_GroupUtilityComponent g, int stage, out string row)
	{
		switch (g.m_iDCODefRole)
		{
			case DCO_EDefRole.MAIN:
			case DCO_EDefRole.FINAL:
			{
				row = "D-D1";
				return DCO_GroupTactics.DEFENSIVE;
			}
			case DCO_EDefRole.FORWARD:
			case DCO_EDefRole.OP:
			{
				row = "D-D1";
				if (g.m_DCOBreak)
					return DCO_GroupTactics.DELAY;
				return DCO_GroupTactics.SNEAK;
			}
			case DCO_EDefRole.DELAY:
			{
				row = "D-D2";
				return DCO_GroupTactics.DELAY;
			}
			case DCO_EDefRole.RESERVE:
			{
				row = "D-D3";
				return DCO_GroupTactics.DEFENSIVE;
			}
			case DCO_EDefRole.COUNTER:
			{
				row = "D-D3";
				return DCO_GroupTactics.AGGRESIVE;
			}
			case DCO_EDefRole.SPOIL_ABF:
			{
				row = "D-D4 T-ABF";
				return DCO_GroupTactics.DEFENSIVE;
			}
			case DCO_EDefRole.SPOIL_RAID:
			{
				row = "D-D4 T-RA";
				if (stage == 1 || g.HasState(DCO_EGroupState.IN_CONTACT))
					return DCO_GroupTactics.AGGRESIVE;
				return DCO_GroupTactics.SNEAK;
			}
		}
		row = "D1";
		return DCO_GroupTactics.DEFENSIVE;
	}

	static bool InPreparedPosition(DCO_GroupUtilityComponent g, vector lp)
	{
		if (!g || (g.GetTask() != DCO_EGroupTask.DEFEND && g.GetTask() != DCO_EGroupTask.GARRISON) || g.GetPhase() == DCO_ETaskPhase.MOVING || g.m_DCOBreak)
			return false;
		CMD_AICommanderObjectiveComponent obj = g.GetGroupObjective();
		if (!obj || !obj.GetOwner())
			return false;
		return vector.DistanceXZ(lp, obj.GetOwner().GetOrigin()) <= obj.GetRadius() + 150.0;
	}
}
