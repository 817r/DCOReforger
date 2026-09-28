class DCO_ObjectiveIntel
{
	float m_fDefenders;
	bool m_bArmor;
	float m_fConfidence;
	float m_fUpdated = -1;
	bool m_bEverSeen;
}

class DCO_AttackAttempt
{
	float m_fBearing;
	int m_iGroups;
	float m_fStrength;
	float m_fStart;
}

class DCO_ObjectiveMemory
{
	ref array<float> m_aFailedBearings = {};
	int m_iFails;
	ref DCO_AttackAttempt m_Active;
}

class DCO_PrepFire
{
	bool m_bFired;
	bool m_bLifted;
	float m_fUntil;
	vector m_vImpact;
	float m_fSafeRadius;
}

class DCO_Consolidation
{
	CMD_AICommanderObjectiveComponent m_Obj;
	float m_fStart;
	float m_fStrengthStart;
	ref array<DCO_GroupUtilityComponent> m_aGroups = {};
}

class DCO_ArmorSupport
{
	DCO_GroupUtilityComponent m_Armor;
	CMD_AICommanderObjectiveComponent m_Obj;
	vector m_vOverwatch;
	int m_iState;
}

class DCO_MergeJob
{
	DCO_GroupUtilityComponent m_From;
	DCO_GroupUtilityComponent m_To;
	float m_fStart;
}

class DCO_CommanderOps
{
	protected static const float TICK_S = 5;
	protected static const float INTEL_HALF_LIFE_S = 300;
	protected static const float PRIOR_DEFENDERS = 8;
	protected static const float UNCERTAINTY_MARGIN = 0.5;
	protected static const float RATIO_CONFIDENCE_MIN = 0.2;
	protected static const int OPS_MAX_GROUPS = 6;
	protected static const int PREP_SHELLS = 6;
	protected static const float PREP_DURATION_S = 45;
	protected static const float SMOKE_OPEN_DIST = 150;
	protected static const float CONSOLIDATION_S = 120;
	protected static const float EXPLOIT_STRENGTH = 0.6;
	protected static const float PAUSE_S = 180;
	protected static const float MERGE_DIST = 300;
	protected static const float MERGE_JOIN_DIST = 25;
	protected static const float MERGE_TIMEOUT_S = 180;
	protected static const float AVOID_BEARING_DEG = 45;
	protected static const int FAILS_BEFORE_COOLDOWN = 3;
	protected static const float ATTEMPT_TIMEOUT_S = 900;
	protected static const float SUPPORT_OBJ_DIST = 700;
	protected static const float ARMOR_INFANTRY_RADIUS = 80;
	protected static const float ARMOR_THREAT_RADIUS = 250;

	protected ref map<CMD_AICommanderObjectiveComponent, ref DCO_ObjectiveIntel> m_mIntel = new map<CMD_AICommanderObjectiveComponent, ref DCO_ObjectiveIntel>();
	protected ref map<CMD_AICommanderObjectiveComponent, ref DCO_ObjectiveMemory> m_mMemory = new map<CMD_AICommanderObjectiveComponent, ref DCO_ObjectiveMemory>();
	protected ref map<CMD_AICommanderObjectiveComponent, ref DCO_PrepFire> m_mPrep = new map<CMD_AICommanderObjectiveComponent, ref DCO_PrepFire>();
	protected ref map<CMD_AICommanderObjectiveComponent, bool> m_mOwned = new map<CMD_AICommanderObjectiveComponent, bool>();
	protected ref array<ref DCO_Consolidation> m_aConsolidations = {};
	protected ref array<ref DCO_ArmorSupport> m_aArmor = {};
	protected ref array<ref DCO_MergeJob> m_aMerges = {};

	protected CMD_AICommanderObjectiveComponent m_Main;
	protected CMD_AICommanderObjectiveComponent m_Support;
	protected CMD_AICommanderObjectiveComponent m_ExploitHint;
	protected DCO_GroupUtilityComponent m_SupportGroup;
	protected float m_fPauseUntil;
	protected float m_fTimer;

	protected ref map<CMD_AICommanderObjectiveComponent, float> m_mFeint = new map<CMD_AICommanderObjectiveComponent, float>();
	protected DCO_GroupUtilityComponent m_FeintGroup;
	protected vector m_vFeintHome;
	protected float m_fFeintEnd;

	CMD_AICommanderObjectiveComponent GetMainEffort()	{ return m_Main; }
	protected float m_fLastIntelDecay;

	static float Now()
	{
		return GetGame().GetWorld().GetWorldTime() / 1000.0;
	}

	static void Event(string line)
	{
		Print("[DCO_Ops] " + line);
		DCO_BenchmarkLoggerComponent.Event(line);
	}

	void Tick(AICommander_BaseComponent cmd, float timeSlice)
	{
		m_fTimer += timeSlice;
		if (m_fTimer < TICK_S)
			return;
		m_fTimer = 0;

		float now = Now();
		UpdateIntel(cmd, now);
		DetectCaptures(cmd, now);
		UpdateConsolidations(cmd, now);
		UpdateAttempts(cmd, now);
		UpdateLift(cmd, now);
		UpdateArmor(cmd, now);
		UpdateMerges(cmd, now);
		UpdateSupportingEffort(cmd, now);
		if (m_FeintGroup && now > m_fFeintEnd)
			EndFeint(cmd, "timeout", now);
	}

	DCO_ObjectiveIntel GetIntel(CMD_AICommanderObjectiveComponent obj)
	{
		DCO_ObjectiveIntel i = m_mIntel.Get(obj);
		if (!i)
		{
			i = new DCO_ObjectiveIntel();
			m_mIntel.Set(obj, i);
		}
		return i;
	}

	protected void UpdateIntel(AICommander_BaseComponent cmd, float now)
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		CMD_ThreatResponseComponent tr = cmd.GetThreatResponseComponent();
		if (!mgr || !tr)
			return;

		float decay = 1;
		if (m_fLastIntelDecay > 0)
			decay = Math.Pow(0.5, (now - m_fLastIntelDecay) / INTEL_HALF_LIFE_S);
		m_fLastIntelDecay = now;

		FactionKey fk = cmd.GetCommanderFactionKey();
		foreach (CMD_AICommanderObjectiveComponent obj : mgr.m_aObjective)
		{
			if (!obj || obj.IsCapturedBy(fk))
				continue;

			DCO_ObjectiveIntel intel = GetIntel(obj);
			intel.m_fConfidence *= decay;

			vector p = obj.GetOwner().GetOrigin();
			float r = obj.GetRadius() + 150;
			int count = 0;
			bool armor = false;
			float conf = 0;
			foreach (CMD_ThreatEntry t : tr.GetThreats())
			{
				if (!t || vector.DistanceXZ(t.m_vBelievedPos, p) > r)
					continue;
				count += t.m_iEstimatedEnemyCount;
				armor = armor || t.m_bArmorSeen;
				float fresh = Math.Clamp(1 - (now - t.m_fLastUpdateTime) / 120.0, 0, 1);
				conf = Math.Max(conf, t.m_fReportQuality * fresh * NightIntelScale());
			}

			if (count > 0 && conf > 0)
			{
				intel.m_fDefenders = count;
				intel.m_bArmor = intel.m_bArmor || armor;
				intel.m_fConfidence = Math.Max(intel.m_fConfidence, conf);
				intel.m_fUpdated = now;
				intel.m_bEverSeen = true;
			}
		}
	}

	protected static float NightIntelScale()
	{
		if (DCO_Night.IsActive())
			return 0.7;
		return 1.0;
	}

	float EstimateDefenders(CMD_AICommanderObjectiveComponent obj)
	{
		DCO_ObjectiveIntel i = GetIntel(obj);
		if (!i.m_bEverSeen)
			return PRIOR_DEFENDERS;
		return i.m_fDefenders;
	}

	float GetTargetRatio(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj)
	{
		float ratio = cmd.GetMinAttackRatio() + 0.25 * cmd.GetPatience() + 0.25 * cmd.GetResilience();
		DCO_ObjectiveMemory m = m_mMemory.Get(obj);
		if (m)
			ratio += 0.5 * m.m_iFails;
		return ratio;
	}

	int RequiredGroups(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, float avgGroupSize)
	{
		int floor = obj.GetRequiredGroupCount();
		DCO_ObjectiveIntel intel = GetIntel(obj);
		float est = EstimateDefenders(obj);
		float margin = UNCERTAINTY_MARGIN * (1 - intel.m_fConfidence) * (1 - 0.6 * cmd.GetRiskTaking());
		int need = Math.Ceil(est * GetTargetRatio(cmd, obj) * (1 + margin) / Math.Max(avgGroupSize, 2));

		DCO_ObjectiveMemory mem = m_mMemory.Get(obj);
		if (mem)
			need += mem.m_iFails;

		return Math.ClampInt(need, floor, Math.Max(floor, OPS_MAX_GROUPS));
	}

	bool IsForceSufficient(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj)
	{
		DCO_ObjectiveIntel intel = GetIntel(obj);
		if (intel.m_bEverSeen && intel.m_fConfidence >= RATIO_CONFIDENCE_MIN)
		{
			if (cmd.GetAttackStrength(obj) < GetTargetRatio(cmd, obj) * intel.m_fDefenders)
				return false;
		}

		if (intel.m_bArmor)
		{
			foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
			{
				if (g && g.GetGroupObjective() == obj && (g.HasAT() || g.IsArmor()))
					return true;
			}
			return false;
		}
		return true;
	}

	bool ReadyToRelease(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, float now)
	{
		float feintUntil;
		if (!m_mFeint.Find(obj, feintUntil))
		{
			if (TryFeint(cmd, obj, now))
				return false;
		}
		else if (now < feintUntil)
			return false;

		DCO_PrepFire prep = m_mPrep.Get(obj);
		if (prep && prep.m_bFired)
			return now >= prep.m_fUntil;

		CMD_ArtillerySupport arty = cmd.GetArtySupport();
		if (!arty || !arty.HasRegisteredUnits())
			return true;

		DCO_ObjectiveMemory mem = m_mMemory.Get(obj);
		bool mandatory = mem && mem.m_iFails > 0;

		vector target;
		float bestUnc = float.MAX;
		float lastSeen = now;
		float quality = 0.5;
		CMD_ThreatResponseComponent tr = cmd.GetThreatResponseComponent();
		if (tr)
		{
			vector p = obj.GetOwner().GetOrigin();
			foreach (CMD_ThreatEntry t : tr.GetThreats())
			{
				if (!t || vector.DistanceXZ(t.m_vBelievedPos, p) > obj.GetRadius() + 50 || t.m_fBelievedUncertainty >= bestUnc)
					continue;
				bestUnc = t.m_fBelievedUncertainty;
				target = t.m_vBelievedPos;
				lastSeen = t.m_fLastUpdateTime;
				quality = t.m_fReportQuality;
			}
		}

		string tier = arty.ResolveTier(bestUnc);
		if (tier == "none")
		{
			if (!mandatory)
				return true;
			target = obj.GetOwner().GetOrigin();
			bestUnc = obj.GetRadius();
			tier = "area";
		}

		CMD_FireMissionRequest req = new CMD_FireMissionRequest(target, SCR_EAIArtilleryAmmoType.HIGH_EXPLOSIVE, now, PREP_SHELLS, lastSeen, quality);
		arty.ApplyTier(req, tier, bestUnc, "prep");
		if (arty.HasFriendlyNearRequest(req, now))
		{
			CMD_ArtillerySupport.LogDenied(req, "friendly");
			return true;
		}

		arty.RequestShellImpact(req, now, PREP_SHELLS);
		prep = new DCO_PrepFire();
		prep.m_bFired = true;
		prep.m_fUntil = now + PREP_DURATION_S;
		prep.m_vImpact = target;
		prep.m_fSafeRadius = req.m_fSafeRadius;
		m_mPrep.Set(obj, prep);
		Event(string.Format("ops_prep obj=%1 unc=%2 mandatory=%3 shells=%4 safe_r=%5", obj.GetOwner().GetName(), Math.Round(bestUnc), mandatory, PREP_SHELLS, Math.Round(req.m_fSafeRadius)));
		return false;
	}

	void OnAssaultReleased(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, vector stagingPos, float now)
	{
		vector objPos = obj.GetOwner().GetOrigin();
		DCO_ObjectiveMemory mem = GetMemory(obj);
		DCO_AttackAttempt a = new DCO_AttackAttempt();
		a.m_fBearing = Bearing(objPos, stagingPos);
		a.m_fStart = now;
		foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
		{
			if (g && g.GetGroupObjective() == obj && AICommander_BaseComponent.TaskCountsSlot(g.GetTask()))
				a.m_iGroups++;
		}
		a.m_fStrength = cmd.GetAttackStrength(obj);
		mem.m_Active = a;
		m_mFeint.Remove(obj);
		EndFeint(cmd, "main_released", now);
		Event(string.Format("ops_release obj=%1 bearing=%2 groups=%3 strength=%4 est=%5 conf=%6", obj.GetOwner().GetName(),
			Math.Round(a.m_fBearing), a.m_iGroups, a.m_fStrength, EstimateDefenders(obj), GetIntel(obj).m_fConfidence.ToString(-1, 2)));

		TrySmoke(cmd, obj, stagingPos, now);
	}

	protected void TrySmoke(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, vector stagingPos, float now)
	{
		CMD_ArtillerySupport arty = cmd.GetArtySupport();
		if (!arty || !arty.HasRegisteredUnits())
			return;

		vector objPos = obj.GetOwner().GetOrigin();
		float dist = vector.DistanceXZ(objPos, stagingPos);
		if (dist < SMOKE_OPEN_DIST)
			return;

		int open;
		for (int i = 1; i <= 5; i++)
		{
			vector s = stagingPos + (objPos - stagingPos) * (i / 6.0);
			array<DCO_TerrainPoint> pts = {};
			if (DCO_TerrainCache.Query(DCO_ETerrainFlag.FLAT_OPEN, s, 35, pts) > 0)
				open++;
		}
		if (open < 3)
			return;

		vector dir = stagingPos - objPos;
		dir[1] = 0;
		dir.Normalize();
		vector smokePos = objPos + dir * (obj.GetRadius() + 40);
		smokePos[1] = GetGame().GetWorld().GetSurfaceY(smokePos[0], smokePos[2]);
		CMD_FireMissionRequest req = new CMD_FireMissionRequest(smokePos, SCR_EAIArtilleryAmmoType.SMOKE, now, 3);
		req.m_sSource = "smoke";
		arty.RequestShellImpact(req, now, 3);
		Event(string.Format("ops_smoke obj=%1 open_samples=%2 pos=%3", obj.GetOwner().GetName(), open, smokePos));
	}

	protected void UpdateLift(AICommander_BaseComponent cmd, float now)
	{
		CMD_ArtillerySupport arty = cmd.GetArtySupport();
		foreach (CMD_AICommanderObjectiveComponent obj, DCO_PrepFire prep : m_mPrep)
		{
			if (!obj || !prep || prep.m_bLifted)
				continue;
			foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
			{
				if (!g || g.GetGroupObjective() != obj || !AICommander_BaseComponent.TaskCountsSlot(g.GetTask()))
					continue;
				if (vector.DistanceXZ(g.GetOwner().GetOrigin(), prep.m_vImpact) > prep.m_fSafeRadius)
					continue;
				prep.m_bLifted = true;
				int lifted = 0;
				if (arty)
					lifted = arty.LiftFiresNear(prep.m_vImpact, prep.m_fSafeRadius);
				Event(string.Format("ops_lift obj=%1 group=%2 cancelled=%3", obj.GetOwner().GetName(), g.GetOwner().GetName(), lifted));
				break;
			}
		}
	}

	vector FindOverwatch(CMD_AICommanderObjectiveComponent obj, vector from)
	{
		vector objPos = obj.GetOwner().GetOrigin();
		float r = obj.GetRadius();
		array<DCO_TerrainPoint> pts = {};
		DCO_TerrainCache.Query(DCO_ETerrainFlag.OVERWATCH | DCO_ETerrainFlag.HILLTOP, objPos, r + 400, pts, obj.GetOwner().GetName());

		vector side = from - objPos;
		side[1] = 0;
		side.Normalize();
		float best = -float.MAX;
		vector bestPos;
		foreach (DCO_TerrainPoint p : pts)
		{
			float d = vector.DistanceXZ(p.m_vPos, objPos);
			if (d < r + 150 || d > r + 400)
				continue;
			vector to = p.m_vPos - objPos;
			to[1] = 0;
			to.Normalize();
			float facing = to[0] * side[0] + to[2] * side[2];
			if (facing < 0)
				continue;
			float s = p.m_fRelElev + facing * 5 - vector.DistanceXZ(p.m_vPos, from) / 200;
			if (s > best)
			{
				best = s;
				bestPos = p.m_vPos;
			}
		}
		if (best > -float.MAX)
			return bestPos;

		return CMD_ReconSpotFinder.FindBestReconSpot(from, objPos, r + 60.0, r + 220.0, 12);
	}

	protected void DetectCaptures(AICommander_BaseComponent cmd, float now)
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
			return;
		FactionKey fk = cmd.GetCommanderFactionKey();
		foreach (CMD_AICommanderObjectiveComponent obj : mgr.m_aObjective)
		{
			if (!obj)
				continue;
			bool isOwned = obj.IsCapturedBy(fk);
			bool was = false;
			bool known = m_mOwned.Find(obj, was);
			m_mOwned.Set(obj, isOwned);
			if (known && isOwned && !was)
				BeginConsolidation(cmd, obj, now);
			else if (known && was && !isOwned && cmd.GetDefense())
				cmd.GetDefense().OnObjectiveLost(cmd, obj, now);
		}
	}

	protected void BeginConsolidation(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, float now)
	{
		DCO_Consolidation c = new DCO_Consolidation();
		c.m_Obj = obj;
		c.m_fStart = now;

		vector objPos = obj.GetOwner().GetOrigin();
		float gatherR = obj.GetRadius() + 400;
		DCO_GroupUtilityComponent garrison;
		foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
		{
			if (!g || g.IsPlayerGroup() || g.IsDedicatedTransport() || g.IsMortar() || g.IsInTransport())
				continue;
			if (g.GetGroupObjective() != obj && vector.DistanceXZ(g.GetOwner().GetOrigin(), objPos) > gatherR)
				continue;
			DCO_EGroupTask t = g.GetTask();
			if (t != DCO_EGroupTask.ATTACK && t != DCO_EGroupTask.FLANK && t != DCO_EGroupTask.SUPPORT_BY_FIRE && t != DCO_EGroupTask.NONE)
				continue;

			c.m_aGroups.Insert(g);
			c.m_fStrengthStart += g.GetUnitCount();
			if (!g.IsArmor() && (!garrison || g.GetUnitCount() > garrison.GetUnitCount()))
				garrison = g;
		}

		foreach (DCO_GroupUtilityComponent g2 : c.m_aGroups)
		{
			if (g2 == garrison)
				continue;
			g2.CompleteAllWaypoints();
			g2.SetGroupObjective(null);
			g2.SetTask(DCO_EGroupTask.NONE);
			g2.DCO_SetHold(now + CONSOLIDATION_S);
			SCR_AIWaypoint wp = cmd.SpawnMoveWP(objPos);
			if (wp)
				g2.MoveTo(wp, now);
		}

		string gname = "none";
		if (garrison)
		{
			cmd.StartObjectiveGarrison(garrison, obj, now);
			c.m_aGroups.RemoveItem(garrison);
			gname = garrison.GetOwner().GetName();
		}

		if (cmd.GetLogistics())
			cmd.GetLogistics().ForceScan();

		m_mPrep.Remove(obj);
		m_mFeint.Remove(obj);
		DCO_ObjectiveMemory mem = m_mMemory.Get(obj);
		if (mem && mem.m_Active)
		{
			Event(string.Format("ops_attempt obj=%1 result=success bearing=%2 time=%3s", obj.GetOwner().GetName(), Math.Round(mem.m_Active.m_fBearing), Math.Round(now - mem.m_Active.m_fStart)));
			mem.m_Active = null;
			mem.m_iFails = 0;
			mem.m_aFailedBearings.Clear();
		}

		m_aConsolidations.Insert(c);
		Event(string.Format("ops_consolidate obj=%1 garrison=%2 gathering=%3 strength=%4", obj.GetOwner().GetName(), gname, c.m_aGroups.Count(), c.m_fStrengthStart));
	}

	protected void UpdateConsolidations(AICommander_BaseComponent cmd, float now)
	{
		for (int i = m_aConsolidations.Count() - 1; i >= 0; i--)
		{
			DCO_Consolidation c = m_aConsolidations[i];
			if (!c.m_Obj)
			{
				m_aConsolidations.Remove(i);
				continue;
			}
			if (now - c.m_fStart < CONSOLIDATION_S)
				continue;

			float strength = 0;
			foreach (DCO_GroupUtilityComponent g : c.m_aGroups)
			{
				if (g)
				{
					strength += g.GetUnitCount();
					g.DCO_SetHold(0);
				}
			}

			float ratio = strength / Math.Max(c.m_fStrengthStart, 1);
			string decision;
			if (ratio >= EXPLOIT_STRENGTH && cmd.GetAggression() >= 0.5)
			{
				m_ExploitHint = NextOnAxis(cmd, c.m_Obj);
				m_Main = null;
				decision = "exploit";
			}
			else
			{
				m_fPauseUntil = now + PAUSE_S;
				decision = "pause";
			}

			string next = "none";
			if (m_ExploitHint && decision == "exploit")
				next = m_ExploitHint.GetOwner().GetName();
			Event(string.Format("ops_after_capture obj=%1 decision=%2 strength_ratio=%3 next=%4", c.m_Obj.GetOwner().GetName(), decision, ratio.ToString(-1, 2), next));
			m_aConsolidations.Remove(i);
		}
	}

	protected CMD_AICommanderObjectiveComponent NextOnAxis(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent from)
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
			return null;
		vector p = from.GetOwner().GetOrigin();
		vector axis = p - cmd.GetOwner().GetOrigin();
		axis[1] = 0;
		axis.Normalize();

		CMD_AICommanderObjectiveComponent best;
		float bestScore = float.MAX;
		foreach (CMD_AICommanderObjectiveComponent o : mgr.m_aObjective)
		{
			if (!o || o == from || o.IsCapturedBy(cmd.GetCommanderFactionKey()) || cmd.IsObjectiveOnCooldown(o))
				continue;
			vector to = o.GetOwner().GetOrigin() - p;
			to[1] = 0;
			float d = to.Length();
			if (d < 1)
				continue;
			float dot = (to[0] * axis[0] + to[2] * axis[2]) / d;
			if (dot < 0.3)
				continue;
			float score = d * (2 - dot);
			if (score < bestScore)
			{
				bestScore = score;
				best = o;
			}
		}
		return best;
	}

	void PlanAxis(AICommander_BaseComponent cmd, notnull array<CMD_AICommanderObjectiveComponent> ranked, float now)
	{
		if (now < m_fPauseUntil)
		{
			ranked.Clear();
			return;
		}

		FactionKey fk = cmd.GetCommanderFactionKey();
		if (m_Main && (m_Main.IsCapturedBy(fk) || cmd.IsObjectiveOnCooldown(m_Main)))
			m_Main = null;

		CMD_AICommanderObjectiveComponent prevMain = m_Main;
		if (!m_Main && m_ExploitHint && !m_ExploitHint.IsCapturedBy(fk) && !cmd.IsObjectiveOnCooldown(m_ExploitHint))
			m_Main = m_ExploitHint;
		m_ExploitHint = null;
		if (!m_Main && !ranked.IsEmpty())
			m_Main = ranked[0];
		if (!m_Main)
			return;

		vector anchor = AxisAnchor(cmd, m_Main);
		vector mainPos = m_Main.GetOwner().GetOrigin();
		vector axis = mainPos - anchor;
		axis[1] = 0;
		float axisLen = axis.Length();
		if (axisLen > 1)
			axis = axis * (1.0 / axisLen);

		array<CMD_AICommanderObjectiveComponent> onAxis = {};
		array<float> alongs = {};
		array<CMD_AICommanderObjectiveComponent> opportunity = {};
		foreach (CMD_AICommanderObjectiveComponent o : ranked)
		{
			if (!o || o == m_Main)
				continue;
			vector rel = o.GetOwner().GetOrigin() - anchor;
			rel[1] = 0;
			float d = rel.Length();
			float along = rel[0] * axis[0] + rel[2] * axis[2];
			bool inCone = d > 1 && along / d >= 0.7;
			if (inCone)
			{
				int at = 0;
				while (at < alongs.Count() && alongs[at] <= along)
					at++;
				onAxis.InsertAt(o, at);
				alongs.InsertAt(along, at);
				continue;
			}

			DCO_ObjectiveIntel intel = GetIntel(o);
			if (intel.m_bEverSeen && intel.m_fConfidence >= 0.5 && intel.m_fDefenders <= 3 && d <= 800)
				opportunity.Insert(o);
		}

		CMD_AICommanderObjectiveComponent support;
		float bestD = SUPPORT_OBJ_DIST;
		foreach (CMD_AICommanderObjectiveComponent s : ranked)
		{
			if (!s || s == m_Main || onAxis.Contains(s))
				continue;
			float sd = vector.DistanceXZ(s.GetOwner().GetOrigin(), mainPos);
			if (sd < bestD)
			{
				bestD = sd;
				support = s;
			}
		}
		if (support != m_Support)
			ReleaseSupportGroup();
		m_Support = support;

		ranked.Clear();
		ranked.Insert(m_Main);
		ranked.InsertAll(onAxis);
		ranked.InsertAll(opportunity);

		if (m_Main != prevMain)
		{
			string sname = "none";
			if (m_Support)
				sname = m_Support.GetOwner().GetName();
			Event(string.Format("ops_axis main=%1 support=%2 phase_lines=%3 opportunities=%4", m_Main.GetOwner().GetName(), sname, onAxis.Count(), opportunity.Count()));
		}
	}

	protected vector AxisAnchor(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent main)
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		vector mainPos = main.GetOwner().GetOrigin();
		vector anchor = cmd.GetOwner().GetOrigin();
		float best = vector.DistanceXZ(anchor, mainPos);
		if (!mgr)
			return anchor;
		foreach (CMD_AICommanderObjectiveComponent o : mgr.m_aObjective)
		{
			if (!o || !o.IsCapturedBy(cmd.GetCommanderFactionKey()))
				continue;
			float d = vector.DistanceXZ(o.GetOwner().GetOrigin(), mainPos);
			if (d < best)
			{
				best = d;
				anchor = o.GetOwner().GetOrigin();
			}
		}
		return anchor;
	}

	protected void UpdateSupportingEffort(AICommander_BaseComponent cmd, float now)
	{
		bool mainActive = m_Main && HasOperationGroups(cmd, m_Main);
		if (!m_Support || !mainActive || m_Support.IsCapturedBy(cmd.GetCommanderFactionKey()))
		{
			ReleaseSupportGroup();
			return;
		}

		if (m_SupportGroup && m_SupportGroup.GetTask() == DCO_EGroupTask.SUPPORT_BY_FIRE)
			return;
		m_SupportGroup = null;

		vector sp = m_Support.GetOwner().GetOrigin();
		DCO_GroupUtilityComponent g = cmd.FindBestIdleGroupForTask_Public(DCO_EGroupTask.SUPPORT_BY_FIRE, sp);
		if (!g || g.IsPlayerGroup() || !cmd.CanCommitGroup(g))
			return;

		vector pos = FindOverwatch(m_Support, g.GetOwner().GetOrigin());
		if (pos == vector.Zero)
			return;

		g.CompleteAllWaypoints();
		g.SetTask(DCO_EGroupTask.SUPPORT_BY_FIRE);
		if (!cmd.SpawnMoveRoute(g, g.GetOwner().GetOrigin(), pos, now))
			return;
		SCR_AIWaypoint sup = cmd.SpawnSuppressWP(sp);
		if (sup)
			g.MoveTo(sup, now);
		m_SupportGroup = g;
		Event(string.Format("ops_support_fire support=%1 main=%2 group=%3", m_Support.GetOwner().GetName(), m_Main.GetOwner().GetName(), g.GetOwner().GetName()));
	}

	protected void ReleaseSupportGroup()
	{
		if (m_SupportGroup && m_SupportGroup.GetTask() == DCO_EGroupTask.SUPPORT_BY_FIRE && !m_SupportGroup.GetGroupObjective())
		{
			m_SupportGroup.CompleteAllWaypoints();
			m_SupportGroup.SetTask(DCO_EGroupTask.NONE);
		}
		m_SupportGroup = null;
	}

	protected bool HasOperationGroups(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj)
	{
		foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
		{
			if (g && g.GetGroupObjective() == obj && AICommander_BaseComponent.TaskCountsSlot(g.GetTask()))
				return true;
		}
		return false;
	}

	DCO_ObjectiveMemory GetMemory(CMD_AICommanderObjectiveComponent obj)
	{
		DCO_ObjectiveMemory m = m_mMemory.Get(obj);
		if (!m)
		{
			m = new DCO_ObjectiveMemory();
			m_mMemory.Set(obj, m);
		}
		return m;
	}

	protected void UpdateAttempts(AICommander_BaseComponent cmd, float now)
	{
		FactionKey fk = cmd.GetCommanderFactionKey();
		foreach (CMD_AICommanderObjectiveComponent obj, DCO_ObjectiveMemory mem : m_mMemory)
		{
			if (!obj || !mem || !mem.m_Active || obj.IsCapturedBy(fk))
				continue;

			bool exhausted = !HasOperationGroups(cmd, obj);
			if (!exhausted && now - mem.m_Active.m_fStart < ATTEMPT_TIMEOUT_S)
				continue;

			float casualties = mem.m_Active.m_fStrength - cmd.GetAttackStrength(obj);
			mem.m_iFails++;
			mem.m_aFailedBearings.Insert(mem.m_Active.m_fBearing);
			Event(string.Format("ops_attempt obj=%1 result=fail why=%2 bearing=%3 groups=%4 casualties=%5 fails=%6", obj.GetOwner().GetName(),
				exhausted, Math.Round(mem.m_Active.m_fBearing), mem.m_Active.m_iGroups, Math.Round(casualties), mem.m_iFails));
			mem.m_Active = null;
			m_mPrep.Remove(obj);
			m_mFeint.Remove(obj);

			if (mem.m_iFails >= FAILS_BEFORE_COOLDOWN)
			{
				float minutes = Math.Lerp(5, 15, cmd.GetPatience());
				cmd.SetObjectiveCooldown(obj, minutes * 60);
				if (m_Main == obj)
					m_Main = null;
				Event(string.Format("ops_deprioritize obj=%1 fails=%2 minutes=%3", obj.GetOwner().GetName(), mem.m_iFails, Math.Round(minutes)));
				mem.m_iFails = 0;
			}
		}
	}

	vector AdjustApproachAxis(CMD_AICommanderObjectiveComponent obj, vector axis)
	{
		DCO_ObjectiveMemory mem = m_mMemory.Get(obj);
		if (!mem || mem.m_aFailedBearings.IsEmpty())
			return axis;

		array<float> rots = {0, 70, -70, 110, -110, 180};
		foreach (float rot : rots)
		{
			vector a = RotateXZ(axis, rot);
			float approach = BearingOfDir(-a);
			bool clear = true;
			foreach (float fb : mem.m_aFailedBearings)
			{
				if (AngleDiff(approach, fb) < AVOID_BEARING_DEG)
				{
					clear = false;
					break;
				}
			}
			if (clear)
			{
				if (rot != 0)
					Event(string.Format("ops_avoid_bearing obj=%1 rotate=%2 new_bearing=%3", obj.GetOwner().GetName(), rot, Math.Round(approach)));
				return a;
			}
		}
		return axis;
	}

	static float Bearing(vector from, vector to)
	{
		return BearingOfDir(to - from);
	}

	static float BearingOfDir(vector d)
	{
		float b = Math.Atan2(d[0], d[2]) * Math.RAD2DEG;
		if (b < 0)
			b += 360;
		return b;
	}

	static float AngleDiff(float a, float b)
	{
		float d = Math.AbsFloat(a - b);
		while (d > 360)
			d -= 360;
		if (d > 180)
			d = 360 - d;
		return d;
	}

	static vector RotateXZ(vector dir, float deg)
	{
		float r = deg * Math.DEG2RAD;
		float c = Math.Cos(r);
		float s = Math.Sin(r);
		return Vector(dir[0] * c - dir[2] * s, 0, dir[0] * s + dir[2] * c);
	}

	void RegisterArmor(AICommander_BaseComponent cmd, DCO_GroupUtilityComponent armor, CMD_AICommanderObjectiveComponent obj, vector overwatch)
	{
		DCO_ArmorSupport s = new DCO_ArmorSupport();
		s.m_Armor = armor;
		s.m_Obj = obj;
		s.m_vOverwatch = overwatch;
		m_aArmor.Insert(s);
		Event(string.Format("ops_armor_overwatch obj=%1 armor=%2 pos=%3", obj.GetOwner().GetName(), armor.GetOwner().GetName(), overwatch));
	}

	protected void UpdateArmor(AICommander_BaseComponent cmd, float now)
	{
		for (int i = m_aArmor.Count() - 1; i >= 0; i--)
		{
			DCO_ArmorSupport s = m_aArmor[i];
			if (!s.m_Armor || !s.m_Obj || s.m_Armor.GetGroupObjective() != s.m_Obj)
			{
				m_aArmor.Remove(i);
				continue;
			}

			vector apos = s.m_Armor.GetOwner().GetOrigin();
			vector objPos = s.m_Obj.GetOwner().GetOrigin();

			CMD_ThreatEntry danger = NearestThreat(cmd, apos, ARMOR_THREAT_RADIUS, true);
			if (danger && s.m_iState != 3 && s.m_iState != 0)
			{
				s.m_iState = 3;
				MoveGroup(cmd, s.m_Armor, s.m_vOverwatch, now);
				DCO_GroupUtilityComponent inf = NearestInfantry(cmd, s.m_Obj, danger.m_vBelievedPos);
				if (inf)
					MoveGroup(cmd, inf, danger.m_vBelievedPos, now);
				string reason = "at";
				if (danger.m_bArmorSeen)
					reason = "armor";
				Event(string.Format("ops_armor_pullback obj=%1 armor=%2 reason=%3 escort=%4", s.m_Obj.GetOwner().GetName(), s.m_Armor.GetOwner().GetName(), reason, inf != null));
				continue;
			}
			if (s.m_iState == 3)
			{
				if (!danger)
					s.m_iState = 0;
				continue;
			}

			CMD_ThreatEntry infThreat = NearestThreat(cmd, apos, ARMOR_THREAT_RADIUS, false);
			if (infThreat && s.m_iState != 4)
			{
				s.m_iState = 4;
				MoveGroup(cmd, s.m_Armor, ArmorEngagePos(apos, infThreat.m_vBelievedPos, objPos, s.m_Obj.GetRadius()), now);
				Event(string.Format("ops_armor_engage_inf obj=%1 armor=%2 dist=%3", s.m_Obj.GetOwner().GetName(), s.m_Armor.GetOwner().GetName(), Math.Round(vector.DistanceXZ(apos, infThreat.m_vBelievedPos))));
				continue;
			}
			if (s.m_iState == 4)
			{
				if (!infThreat)
					s.m_iState = 0;
				continue;
			}

			bool atKnown = ObjectiveHasATOrArmor(cmd, s.m_Obj);
			if (s.m_iState == 0)
			{
				if (!cmd.IsAssaultReleased(s.m_Obj))
					continue;
				if (atKnown && !InfantryWithin(cmd, s.m_Obj, objPos, s.m_Obj.GetRadius()))
					continue;
				if (EnemyStrengthAt(cmd, s.m_Obj) > FriendlyStrengthAt(cmd, s.m_Obj))
					continue;
				s.m_iState = 1;
				MoveGroup(cmd, s.m_Armor, objPos, now);
				if (atKnown)
					Event(string.Format("ops_armor_enter obj=%1 armor=%2", s.m_Obj.GetOwner().GetName(), s.m_Armor.GetOwner().GetName()));
				else
					Event(string.Format("ops_armor_with_assault obj=%1 armor=%2", s.m_Obj.GetOwner().GetName(), s.m_Armor.GetOwner().GetName()));
				continue;
			}

			float stopDist = ARMOR_INFANTRY_RADIUS;
			if (!atKnown)
				stopDist = ARMOR_ASSAULT_LEASH;
			bool escorted = InfantryWithin(cmd, s.m_Obj, apos, stopDist);
			if (s.m_iState == 1 && !escorted)
			{
				s.m_iState = 2;
				s.m_Armor.CompleteAllWaypoints();
				Event(string.Format("ops_armor_wait obj=%1 armor=%2", s.m_Obj.GetOwner().GetName(), s.m_Armor.GetOwner().GetName()));
			}
			else if (s.m_iState == 2 && InfantryWithin(cmd, s.m_Obj, apos, ARMOR_INFANTRY_RADIUS))
			{
				s.m_iState = 1;
				MoveGroup(cmd, s.m_Armor, objPos, now);
			}
		}
	}

	protected static const float ARMOR_ASSAULT_LEASH = 150;
	protected static const float ARMOR_ENGAGE_STANDOFF = 60;
	protected static const float ARMOR_ENGAGE_LEASH = 150;

	protected static vector ArmorEngagePos(vector apos, vector threatPos, vector objPos, float objRadius)
	{
		vector dir = apos - threatPos;
		dir[1] = 0;
		if (dir.Length() < 1)
			dir = objPos - threatPos;
		dir[1] = 0;
		dir.Normalize();

		vector pos = threatPos + dir * ARMOR_ENGAGE_STANDOFF;
		vector fromObj = pos - objPos;
		fromObj[1] = 0;
		float leash = objRadius + ARMOR_ENGAGE_LEASH;
		if (fromObj.Length() > leash)
			pos = objPos + fromObj.Normalized() * leash;
		pos[1] = GetGame().GetWorld().GetSurfaceY(pos[0], pos[2]);
		return pos;
	}

	protected static bool IsArmorDanger(CMD_ThreatEntry t)
	{
		return t.m_bArmorSeen || t.m_bATSeen;
	}

	protected static bool ObjectiveHasATOrArmor(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj)
	{
		CMD_ThreatResponseComponent tr = cmd.GetThreatResponseComponent();
		if (!tr)
			return false;

		vector p = obj.GetOwner().GetOrigin();
		float r = obj.GetRadius() + 100;
		foreach (CMD_ThreatEntry t : tr.GetThreats())
		{
			if (t && IsArmorDanger(t) && vector.DistanceXZ(t.m_vBelievedPos, p) <= r)
				return true;
		}
		return false;
	}

	protected static float EnemyStrengthAt(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj)
	{
		CMD_ThreatResponseComponent tr = cmd.GetThreatResponseComponent();
		if (!tr)
			return 0;

		vector p = obj.GetOwner().GetOrigin();
		float r = obj.GetRadius() + 100;
		float total = 0;
		foreach (CMD_ThreatEntry t : tr.GetThreats())
		{
			if (!t || vector.DistanceXZ(t.m_vBelievedPos, p) > r)
				continue;
			total += t.m_iEstimatedEnemyCount * DCO_Strength.W_MEMBER;
			if (t.m_bATSeen)
				total += DCO_Strength.W_AT_VS_VEHICLES;
			if (t.m_bArmorSeen)
				total += DCO_Strength.W_TANK;
		}
		return total;
	}

	protected static float FriendlyStrengthAt(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj)
	{
		vector p = obj.GetOwner().GetOrigin();
		float r = obj.GetRadius() + 300;
		float total = 0;
		DCO_StrengthInfo info = new DCO_StrengthInfo();
		foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
		{
			if (!g || g.GetGroupObjective() != obj || vector.DistanceXZ(g.GetOwner().GetOrigin(), p) > r)
				continue;
			DCO_Strength.OfGroup(SCR_AIGroup.Cast(g.GetOwner()), false, info);
			total += info.m_fStrength;
		}
		return total;
	}

	protected static void MoveGroup(AICommander_BaseComponent cmd, DCO_GroupUtilityComponent g, vector pos, float now)
	{
		g.CompleteAllWaypoints();
		SCR_AIWaypoint wp = cmd.SpawnMoveWP(pos);
		if (wp)
			g.MoveTo(wp, now);
	}

	protected static CMD_ThreatEntry NearestThreat(AICommander_BaseComponent cmd, vector pos, float radius, bool armorDanger)
	{
		CMD_ThreatResponseComponent tr = cmd.GetThreatResponseComponent();
		if (!tr)
			return null;
		CMD_ThreatEntry best;
		float bestD = radius;
		foreach (CMD_ThreatEntry t : tr.GetThreats())
		{
			if (!t || IsArmorDanger(t) != armorDanger)
				continue;
			float d = vector.DistanceXZ(t.m_vBelievedPos, pos);
			if (d < bestD)
			{
				bestD = d;
				best = t;
			}
		}
		return best;
	}

	protected static bool InfantryWithin(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, vector pos, float radius)
	{
		foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
		{
			if (g && !g.IsArmor() && g.GetGroupObjective() == obj && vector.DistanceXZ(g.GetOwner().GetOrigin(), pos) <= radius)
				return true;
		}
		return false;
	}

	protected static DCO_GroupUtilityComponent NearestInfantry(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, vector pos)
	{
		DCO_GroupUtilityComponent best;
		float bestD = float.MAX;
		foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
		{
			if (!g || g.IsArmor() || g.GetGroupObjective() != obj || g.IsInTransport())
				continue;
			float d = vector.DistanceSqXZ(g.GetOwner().GetOrigin(), pos);
			if (d < bestD)
			{
				bestD = d;
				best = g;
			}
		}
		return best;
	}

	protected void UpdateMerges(AICommander_BaseComponent cmd, float now)
	{
		for (int i = m_aMerges.Count() - 1; i >= 0; i--)
		{
			DCO_MergeJob j = m_aMerges[i];
			if (!j.m_From || !j.m_To || now - j.m_fStart > MERGE_TIMEOUT_S || j.m_From.HasState(DCO_EGroupState.IN_CONTACT) || j.m_To.HasState(DCO_EGroupState.IN_CONTACT))
			{
				if (j.m_From)
					j.m_From.DCO_SetHold(0);
				if (j.m_To)
					j.m_To.DCO_SetHold(0);
				m_aMerges.Remove(i);
				continue;
			}

			if (vector.DistanceXZ(j.m_From.GetOwner().GetOrigin(), j.m_To.GetOwner().GetOrigin()) > MERGE_JOIN_DIST)
				continue;

			ExecuteMerge(cmd, j);
			m_aMerges.Remove(i);
		}

		array<DCO_GroupUtilityComponent> weak = {};
		foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
		{
			if (!g || g.IsPlayerGroup() || g.IsDedicatedTransport() || g.IsMortar() || g.IsArmor() || g.IsInTransport())
				continue;
			if (!g.IsAvailableReserve() || g.HasState(DCO_EGroupState.IN_CONTACT) || IsMerging(g))
				continue;
			int units = g.GetUnitCount();
			if (units > 0 && units * 2 < Math.Max(g.DCO_GetPeakStrength(), 2))
				weak.Insert(g);
		}

		for (int a = 0; a < weak.Count(); a++)
		{
			for (int b = a + 1; b < weak.Count(); b++)
			{
				DCO_GroupUtilityComponent ga = weak[a];
				DCO_GroupUtilityComponent gb = weak[b];
				if (!ga || !gb || IsMerging(ga) || IsMerging(gb))
					continue;
				if (vector.DistanceXZ(ga.GetOwner().GetOrigin(), gb.GetOwner().GetOrigin()) > MERGE_DIST)
					continue;

				DCO_MergeJob j = new DCO_MergeJob();
				j.m_To = ga;
				j.m_From = gb;
				if (gb.GetUnitCount() > ga.GetUnitCount())
				{
					j.m_To = gb;
					j.m_From = ga;
				}
				j.m_fStart = now;
				j.m_From.DCO_SetHold(now + MERGE_TIMEOUT_S);
				j.m_To.DCO_SetHold(now + MERGE_TIMEOUT_S);
				j.m_To.CompleteAllWaypoints();
				MoveGroup(cmd, j.m_From, j.m_To.GetOwner().GetOrigin(), now);
				m_aMerges.Insert(j);
				Event(string.Format("ops_merge_start from=%1 (%2) to=%3 (%4)", j.m_From.GetOwner().GetName(), j.m_From.GetUnitCount(), j.m_To.GetOwner().GetName(), j.m_To.GetUnitCount()));
			}
		}
	}

	void RequestMerge(AICommander_BaseComponent cmd, DCO_GroupUtilityComponent from, DCO_GroupUtilityComponent to, float now)
	{
		if (!from || !to || IsMerging(from))
			return;
		DCO_MergeJob j = new DCO_MergeJob();
		j.m_From = from;
		j.m_To = to;
		j.m_fStart = now;
		from.DCO_SetHold(now + MERGE_TIMEOUT_S);
		MoveGroup(cmd, from, to.GetOwner().GetOrigin(), now);
		m_aMerges.Insert(j);
		Event(string.Format("ops_merge_start from=%1 (%2) to=%3 (%4) reason=op_return", from.GetOwner().GetName(), from.GetUnitCount(), to.GetOwner().GetName(), to.GetUnitCount()));
	}

	protected bool TryFeint(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, float now)
	{
		m_mFeint.Set(obj, 0);
		if (!cmd.IsFeintEnabled() || !cmd.IsAttackRatioMet(obj))
			return false;
		if (Math.RandomFloat01() > 0.15 + 0.35 * cmd.GetAggression() + 0.35 * cmd.GetRiskTaking())
			return false;

		vector objPos = obj.GetOwner().GetOrigin();
		string how;
		if (m_Support && m_Support != obj && !m_Support.IsCapturedBy(cmd.GetCommanderFactionKey()))
		{
			vector sp = m_Support.GetOwner().GetOrigin();
			DCO_GroupUtilityComponent g = m_SupportGroup;
			if (!g)
			{
				g = cmd.FindBestIdleGroupForTask_Public(DCO_EGroupTask.FLANK, sp);
				if (g && (g.IsPlayerGroup() || !cmd.CanCommitGroup(g)))
					g = null;
			}
			if (g)
			{
				vector gp = g.GetOwner().GetOrigin();
				vector toG = gp - sp;
				toG[1] = 0;
				toG.Normalize();
				m_FeintGroup = g;
				m_vFeintHome = gp;
				g.SetTask(DCO_EGroupTask.SUPPORT_BY_FIRE);
				MoveGroup(cmd, g, sp + toG * (m_Support.GetRadius() + 30), now);
				how = "fake_attack:" + g.GetOwner().GetName();
			}
		}

		CMD_ArtillerySupport arty = cmd.GetArtySupport();
		vector st = vector.Zero;
		if (arty && arty.HasRegisteredUnits() && cmd.GetStagingPos(obj, st))
		{
			vector dir = st - objPos;
			dir[1] = 0;
			dir.Normalize();
			vector smokePos = objPos + RotateXZ(dir, 120) * (obj.GetRadius() + 50);
			smokePos[1] = GetGame().GetWorld().GetSurfaceY(smokePos[0], smokePos[2]);
			CMD_FireMissionRequest req = new CMD_FireMissionRequest(smokePos, SCR_EAIArtilleryAmmoType.SMOKE, now, 3);
			req.m_sSource = "smoke";
			arty.RequestShellImpact(req, now, 3);
			how = how + " smoke_far_side";
		}
		if (how == "")
			return false;

		float lead = cmd.GetFeintLead();
		m_mFeint.Set(obj, now + lead);
		m_fFeintEnd = now + lead * 2;
		string sname = "none";
		if (m_Support)
			sname = m_Support.GetOwner().GetName();
		Event(string.Format("ops_feint_start main=%1 support=%2 how=%3 lead_s=%4", obj.GetOwner().GetName(), sname, how, Math.Round(lead)));
		return true;
	}

	protected void EndFeint(AICommander_BaseComponent cmd, string why, float now)
	{
		if (!m_FeintGroup)
			return;
		if (m_FeintGroup.GetTask() == DCO_EGroupTask.SUPPORT_BY_FIRE)
		{
			MoveGroup(cmd, m_FeintGroup, m_vFeintHome, now);
			if (m_FeintGroup != m_SupportGroup)
				m_FeintGroup.SetTask(DCO_EGroupTask.NONE);
			else if (m_Support)
			{
				SCR_AIWaypoint sup = cmd.SpawnSuppressWP(m_Support.GetOwner().GetOrigin());
				if (sup)
					m_FeintGroup.MoveTo(sup, now);
			}
		}
		Event(string.Format("ops_feint_end why=%1 group=%2", why, m_FeintGroup.GetOwner().GetName()));
		m_FeintGroup = null;
	}

	protected bool IsMerging(DCO_GroupUtilityComponent g)
	{
		foreach (DCO_MergeJob j : m_aMerges)
		{
			if (j.m_From == g || j.m_To == g)
				return true;
		}
		return false;
	}

	protected void ExecuteMerge(AICommander_BaseComponent cmd, DCO_MergeJob j)
	{
		SCR_AIGroup from = SCR_AIGroup.Cast(j.m_From.GetOwner());
		SCR_AIGroup to = SCR_AIGroup.Cast(j.m_To.GetOwner());
		if (!from || !to)
			return;

		array<AIAgent> agents = {};
		from.GetAgents(agents);
		foreach (AIAgent a : agents)
		{
			from.RemoveAgent(a);
			to.AddAgent(a);
		}
		j.m_To.DCO_SetHold(0);
		string fromName = from.GetName();
		cmd.ReleaseGroup(j.m_From);
		SCR_EntityHelper.DeleteEntityAndChildren(from);
		Event(string.Format("ops_merge_done from=%1 moved=%2 to=%3 now=%4", fromName, agents.Count(), to.GetName(), j.m_To.GetUnitCount()));
	}
}
