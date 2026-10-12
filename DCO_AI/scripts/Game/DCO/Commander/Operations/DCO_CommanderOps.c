class DCO_ObjectiveIntel
{
	float m_fDefenders;
	bool m_bArmor;
	float m_fConfidence;
	float m_fUpdated = -1;
	bool m_bEverSeen;
	int m_iRaw;
	float m_fLogAt = -1;
	float m_fUpdLogAt = -1;
	float m_fDecayAt = -1;
	string m_sLastHow;
	float m_fSeed;
	float m_fLastCount;
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
	float m_fFailAt;
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
	DCO_GroupUtilityComponent m_Garrison;
	bool m_bBlockLogged;
	float m_fHold = 480;
	int m_iStep;
	vector m_vAxis;
}

class DCO_ArmorSupport
{
	DCO_GroupUtilityComponent m_Armor;
	CMD_AICommanderObjectiveComponent m_Obj;
	vector m_vOverwatch;
	int m_iState;
	float m_fTerrainAt = -1000;
	bool m_bClose;
	bool m_bSbfLogged;
	bool m_bShift;
	bool m_bLift;
	int m_iSopPrev;
	string m_sMoveWhy = "register";
	float m_fMoveLogAt;
}

class DCO_RemnantEntry
{
	DCO_GroupUtilityComponent m_G;
	float m_fAt;
	float m_fMoveAt;
	string m_sName;
}

class DCO_MergeJob
{
	DCO_GroupUtilityComponent m_From;
	DCO_GroupUtilityComponent m_To;
	float m_fStart;
}

class DCO_CommanderOps
{
	static float Now()
	{
		return GetGame().GetWorld().GetWorldTime() / 1000.0;
	}


	protected ref map<CMD_AICommanderObjectiveComponent, ref DCO_ObjectiveIntel> m_mIntel = new map<CMD_AICommanderObjectiveComponent, ref DCO_ObjectiveIntel>();
	protected ref map<CMD_AICommanderObjectiveComponent, ref DCO_ObjectiveMemory> m_mMemory = new map<CMD_AICommanderObjectiveComponent, ref DCO_ObjectiveMemory>();
	protected ref map<CMD_AICommanderObjectiveComponent, ref DCO_PrepFire> m_mPrep = new map<CMD_AICommanderObjectiveComponent, ref DCO_PrepFire>();
	protected ref map<CMD_AICommanderObjectiveComponent, bool> m_mOwned = new map<CMD_AICommanderObjectiveComponent, bool>();
	protected ref array<ref DCO_Consolidation> m_aConsolidations = {};
	protected ref array<ref DCO_ArmorSupport> m_aArmor = {};
	protected ref array<ref DCO_MergeJob> m_aMerges = {};
	protected ref array<vector> m_aPerceived = {};

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


	static void Event(string line)
	{
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", line);
	}

	void Tick(AICommander_BaseComponent cmd, float timeSlice)
	{
		m_fTimer += timeSlice;
		if (m_fTimer < 5.0)
			return;
		m_fTimer = 0;

		float now = DCO_Util.NowS();
		BuildPerceived(cmd);
		UpdateIntel(cmd, now);
		DetectCaptures(cmd, now);
		UpdateConsolidations(cmd, now);
		UpdateAttempts(cmd, now);
		UpdateLift(cmd, now);
		UpdateArmor(cmd, now);
		UpdateMerges(cmd, now);
		UpdateRemnants(cmd, now);
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
			decay = Math.Pow(0.5, (now - m_fLastIntelDecay) / 300.0);
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
			bool armor = false;
			float conf = 0;
			foreach (CMD_ThreatEntry t : tr.GetThreats())
			{
				if (!t || vector.DistanceXZ(t.m_vBelievedPos, p) > r)
					continue;
				armor = armor || t.m_bArmorSeen;
				float fresh = Math.Clamp(1 - (now - t.m_fLastUpdateTime) / 120.0, 0, 1);
				conf = Math.Max(conf, t.m_fReportQuality * fresh * NightIntelScale());
			}

			int raw;
			int shared;
			float count = EstimateObj(cmd, mgr, obj, p, r, now, raw, shared);
			int seen = PerceivedNear(p, r);
			if (raw > 0 && now - intel.m_fLogAt >= 60.0 && (Math.AbsFloat(raw - count) >= 1 || shared > 0))
			{
				intel.m_fLogAt = now;
				Event(string.Format("est obj=%1 est_raw=%2 est_dedup=%3 perceived=%4 shared_skip=%5", obj.GetLogName(), raw, count.ToString(-1, 1), seen, shared));
			}
			intel.m_iRaw = raw;

			if (count > 0 && conf > 0)
			{
				float oldDef = intel.m_fDefenders;
				float newDef = count;
				string how = "set";
				if (DCO_Ctx.TuneForCmd(cmd, "intel.est_blend", 1) > 0 && oldDef > 0)
				{
					if (count < oldDef)
					{
						float dt = 5.0;
						if (intel.m_fUpdated >= 0)
							dt = Math.Clamp(now - intel.m_fUpdated, 0.0, 60.0);
						float k = Math.Clamp(conf * dt / 120.0, 0.0, 1.0);
						newDef = oldDef - (oldDef - count) * k;
						how = "blend_down";
					}
					else if (count > oldDef && seen == 0)
					{
						newDef = oldDef;
						how = "hold_no_eyes";
					}
				}
				bool howChanged = how != intel.m_sLastHow;
				intel.m_sLastHow = how;
				if (Math.AbsFloat(newDef - oldDef) >= 1.0 || (how == "hold_no_eyes" && howChanged))
				{
					DCO_Perf.Count("intel:est_" + how);
					if (now - intel.m_fUpdLogAt >= 30.0 || howChanged)
					{
						intel.m_fUpdLogAt = now;
						Event(string.Format("est_update obj=%1 old=%2 new=%3 count=%4 perceived=%5 conf=%6 how=%7", obj.GetLogName(), oldDef.ToString(-1, 1), newDef.ToString(-1, 1), count.ToString(-1, 1), seen, conf.ToString(-1, 2), how));
					}
				}
				intel.m_fDefenders = newDef;
				intel.m_bArmor = intel.m_bArmor || armor;
				intel.m_fConfidence = Math.Max(intel.m_fConfidence, conf);
				intel.m_fUpdated = now;
				intel.m_fDecayAt = now;
				intel.m_fLastCount = count;
				intel.m_bEverSeen = true;
			}
			else if (seen == 0 && intel.m_fDefenders > 0 && DCO_Ctx.TuneForCmd(cmd, "intel.est_blend", 1) > 0)
			{
				if (intel.m_fDecayAt < 0)
					intel.m_fDecayAt = now;
				float ddt = now - intel.m_fDecayAt;
				intel.m_fDecayAt = now;
				float oldD = intel.m_fDefenders;
				float floorD = 0;
				if (intel.m_fSeed > 0)
				{
					floorD = intel.m_fSeed;
					if (intel.m_fLastCount > 0)
						floorD = Math.Min(floorD, intel.m_fLastCount);
					floorD = floorD * 0.5;
				}
				float newD = Math.Max(oldD * Math.Pow(0.5, ddt / 600.0), floorD);
				if (newD < 0.5)
					newD = 0;
				intel.m_fDefenders = newD;
				if (Math.Floor(oldD) != Math.Floor(newD))
				{
					DCO_Perf.Count("intel:est_decay");
					intel.m_sLastHow = "decay";
					Event(string.Format("est_update obj=%1 old=%2 new=%3 count=0 perceived=0 conf=%4 how=decay half_life_s=600 floor=%5", obj.GetLogName(), oldD.ToString(-1, 1), newD.ToString(-1, 1), intel.m_fConfidence.ToString(-1, 2), floorD.ToString(-1, 1)));
				}
			}
		}
	}

	protected void BuildPerceived(AICommander_BaseComponent cmd)
	{
		m_aPerceived.Clear();
		PerceptionManager pm = GetGame().GetPerceptionManager();
		if (!pm)
			return;
		float pmNow = pm.GetTime();
		set<IEntity> seen = new set<IEntity>();
		foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
		{
			if (!g || !g.GetGroupUtilityComponent())
				continue;
			SCR_AIGroupPerception perc = g.GetGroupUtilityComponent().GetPercGroupComp();
			if (!perc)
				continue;
			foreach (SCR_AITargetInfo t : perc.m_aTargets)
			{
				if (!t || !t.m_Entity || pmNow - t.m_fTimestamp > 300.0)
					continue;
				if (t.m_eCategory != EAITargetInfoCategory.DETECTED && t.m_eCategory != EAITargetInfoCategory.IDENTIFIED)
					continue;
				if (seen.Contains(t.m_Entity))
					continue;
				seen.Insert(t.m_Entity);
				m_aPerceived.Insert(t.m_vWorldPos);
			}
		}
	}

	int PerceivedNear(vector center, float radius)
	{
		int n;
		float rSq = radius * radius;
		foreach (vector v : m_aPerceived)
		{
			if (vector.DistanceSqXZ(v, center) <= rSq)
				n++;
		}
		return n;
	}

	static float DedupEntries(array<CMD_ThreatEntry> entries, float now, bool armorWeight, float minFresh, out int raw)
	{
		raw = 0;
		array<vector> anchors = {};
		array<float> values = {};
		foreach (CMD_ThreatEntry t : entries)
		{
			if (!t)
				continue;
			float age = now - t.m_fLastUpdateTime;
			float w = Math.Clamp(1 - (age - 60.0) / 240.0, minFresh, 1);
			if (w <= 0)
				continue;
			raw += t.m_iEstimatedEnemyCount;
			float v = 1.0 * t.m_iEstimatedEnemyCount * w;
			if (armorWeight && t.m_bArmorSeen)
				v += 3 * Math.Max(t.m_iArmor, 1);

			int hit = -1;
			foreach (int i, vector a : anchors)
			{
				if (vector.DistanceSqXZ(a, t.m_vBelievedPos) <= 10000)
				{
					hit = i;
					break;
				}
			}
			if (hit < 0)
			{
				anchors.Insert(t.m_vBelievedPos);
				values.Insert(v);
			}
			else
				values[hit] = Math.Max(values[hit], v);
		}

		float total = 0;
		foreach (float val : values)
			total += val;
		return total;
	}

	float EstimateObj(AICommander_BaseComponent cmd, AICommander_ManagerComponent mgr, CMD_AICommanderObjectiveComponent obj, vector center, float radius, float now, out int raw, out int shared)
	{
		raw = 0;
		shared = 0;
		CMD_ThreatResponseComponent tr = cmd.GetThreatResponseComponent();
		if (!tr)
			return 0;
		array<CMD_ThreatEntry> near = {};
		float rSq = radius * radius;
		foreach (CMD_ThreatEntry t : tr.GetThreats())
		{
			if (!t || vector.DistanceSqXZ(t.m_vBelievedPos, center) > rSq)
				continue;
			float mine = vector.DistanceXZ(t.m_vBelievedPos, center) - obj.GetRadius();
			bool other = false;
			foreach (CMD_AICommanderObjectiveComponent o : mgr.m_aObjective)
			{
				if (!o || o == obj || !o.GetOwner())
					continue;
				if (vector.DistanceXZ(t.m_vBelievedPos, o.GetOwner().GetOrigin()) - o.GetRadius() < mine)
				{
					other = true;
					break;
				}
			}
			if (other)
			{
				shared++;
				continue;
			}
			near.Insert(t);
		}
		if (shared > 0)
			DCO_Perf.Count("intel:est_shared_skip");
		float est = DedupEntries(near, now, false, 0, raw);
		int perceived = 0;
		float pSq = radius * radius;
		foreach (vector v : m_aPerceived)
		{
			if (vector.DistanceSqXZ(v, center) > pSq)
				continue;
			float vm = vector.DistanceXZ(v, center) - obj.GetRadius();
			bool vOther = false;
			foreach (CMD_AICommanderObjectiveComponent po : mgr.m_aObjective)
			{
				if (po && po != obj && po.GetOwner() && vector.DistanceXZ(v, po.GetOwner().GetOrigin()) - po.GetRadius() < vm)
				{
					vOther = true;
					break;
				}
			}
			if (!vOther)
				perceived++;
		}
		if (perceived > 0)
			est = Math.Min(est, Math.Ceil(1.0 * perceived * 1.5));
		return est;
	}

	float Estimate(AICommander_BaseComponent cmd, vector center, float radius, float now, bool armorWeight, out int raw)
	{
		raw = 0;
		CMD_ThreatResponseComponent tr = cmd.GetThreatResponseComponent();
		if (!tr)
			return 0;

		array<CMD_ThreatEntry> near = {};
		float rSq = radius * radius;
		foreach (CMD_ThreatEntry t : tr.GetThreats())
		{
			if (t && vector.DistanceSqXZ(t.m_vBelievedPos, center) <= rSq)
				near.Insert(t);
		}

		float est = DedupEntries(near, now, armorWeight, 0, raw);
		int perceived = PerceivedNear(center, radius);
		if (perceived > 0)
			est = Math.Min(est, Math.Ceil(1.0 * perceived * 1.5));
		return est;
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
			return 8.0;
		return i.m_fDefenders;
	}

	float GetTargetRatio(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj)
	{
		float ratio = cmd.GetMinAttackRatio() + 0.25 * cmd.GetPatience() + 0.25 * cmd.GetResilience();
		DCO_ObjectiveMemory m = m_mMemory.Get(obj);
		if (m)
			ratio += Math.Min(0.5 * m.m_iFails, 1.0);
		return ratio;
	}

	int RequiredGroups(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, float avgGroupSize)
	{
		int floor = obj.GetRequiredGroupCount();
		DCO_ObjectiveIntel intel = GetIntel(obj);
		float est = EstimateDefenders(obj);
		float margin = 0.5 * (1 - intel.m_fConfidence) * (1 - 0.6 * cmd.GetRiskTaking());
		int need = Math.Ceil(est * GetTargetRatio(cmd, obj) * (1 + margin) / Math.Max(avgGroupSize, 2));

		DCO_ObjectiveMemory mem = m_mMemory.Get(obj);
		if (mem)
			need += Math.Min(mem.m_iFails, 2);

		int hi = DCO_Force.AttackMax(cmd);
		int lo = Math.Min(Math.Max(floor, DCO_Force.Min(cmd, "attack")), hi);
		return Math.ClampInt(need, lo, Math.Max(lo, hi));
	}

	protected ref map<CMD_AICommanderObjectiveComponent, int> m_mDCORatioOkBand = new map<CMD_AICommanderObjectiveComponent, int>();

	protected int DCO_LowestAttackBand(AICommander_BaseComponent cmd, DCO_Doctrine d)
	{
		int lowest = -1;
		for (int b = DCO_EForceBand.STRONG; b <= DCO_EForceBand.WEAKER; b++)
		{
			if ((DCO_BattlePlanner.AttackMask(d, b) & cmd.GetAllowedTactics()) != 0)
				lowest = b;
		}
		return lowest;
	}

	string DCO_ForceText(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj)
	{
		DCO_Doctrine d = cmd.GetDoctrine();
		DCO_DoctrineRuntime rt = cmd.GetDoctrineRuntime();
		DCO_ForceEval e;
		if (d && rt)
			e = rt.GetEval(obj);
		if (e && e.m_iBand != DCO_EForceBand.UNKNOWN)
		{
			int low = DCO_LowestAttackBand(cmd, d);
			float need = -1;
			if (low >= 0)
				need = e.m_fEnemy * e.m_fPosture * DCO_DoctrineRuntime.LowerEdge(low);
			return string.Format(" strength=%1 need=%2 defenders=%3 src=doctrine band=%4 eff=%5", e.m_fOwn.ToString(-1, 1), need.ToString(-1, 1), e.m_fEnemy.ToString(-1, 1), DCO_Doctrine.BandName(e.m_iBand), e.m_fEff.ToString(-1, 2));
		}
		DCO_ObjectiveIntel intel = GetIntel(obj);
		return string.Format(" strength=%1 need=%2 defenders=%3 src=intel", cmd.GetAttackStrength(obj), (GetTargetRatio(cmd, obj) * intel.m_fDefenders).ToString(-1, 1), intel.m_fDefenders.ToString(-1, 1));
	}

	bool IsForceSufficient(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj)
	{
		if (DCO_TacticTest.Forces(cmd, obj))
		{
			DCO_TacticTest.NoteBypass(cmd, obj, "ratio");
			return true;
		}
		DCO_ObjectiveIntel intel = GetIntel(obj);
		DCO_Doctrine d = cmd.GetDoctrine();
		DCO_DoctrineRuntime rt = cmd.GetDoctrineRuntime();
		DCO_ForceEval e;
		if (d && rt)
			e = rt.GetEval(obj);
		if (e && e.m_iBand != DCO_EForceBand.UNKNOWN)
		{
			if ((DCO_BattlePlanner.AttackMask(d, e.m_iBand) & cmd.GetAllowedTactics()) != 0)
				m_mDCORatioOkBand.Set(obj, e.m_iBand);
			else
			{
				int okBand;
				if (!m_mDCORatioOkBand.Find(obj, okBand) || e.m_fEff < DCO_DoctrineRuntime.LowerEdge(okBand) * 0.85)
				{
					m_mDCORatioOkBand.Remove(obj);
					return false;
				}
				DCO_Perf.Count("force_ok:hysteresis");
			}
		}
		else if (intel.m_bEverSeen && intel.m_fConfidence >= 0.2)
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

		int prepMode = -1;
		if (cmd.GetDoctrineRuntime())
			prepMode = cmd.GetDoctrineRuntime().GetPlanner().PrepMode(cmd, obj);
		if (prepMode == 0)
			return true;

		CMD_ArtillerySupport arty = cmd.GetArtySupport();
		if (!arty || !arty.HasRegisteredUnits())
			return true;

		DCO_ObjectiveMemory mem = m_mMemory.Get(obj);
		bool mandatory = (mem && mem.m_iFails > 0) || prepMode == 1;

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

		CMD_FireMissionRequest req = new CMD_FireMissionRequest(target, SCR_EAIArtilleryAmmoType.HIGH_EXPLOSIVE, now, 6, lastSeen, quality);
		arty.ApplyTier(req, tier, bestUnc, "prep");
		if (arty.HasFriendlyNearRequest(req, now))
		{
			CMD_ArtillerySupport.LogDenied(req, "friendly");
			return true;
		}

		arty.RequestShellImpact(req, now, 6);
		prep = new DCO_PrepFire();
		prep.m_bFired = true;
		prep.m_fUntil = now + 45.0;
		prep.m_vImpact = target;
		prep.m_fSafeRadius = req.m_fSafeRadius;
		m_mPrep.Set(obj, prep);
		Event(string.Format("ops_prep obj=%1 unc=%2 mandatory=%3 shells=%4 safe_r=%5", obj.GetLogName(), Math.Round(bestUnc), mandatory, 6, Math.Round(req.m_fSafeRadius)));
		if (prepMode == 1)
			cmd.GetDoctrineRuntime().GetPlanner().Phase(cmd, obj, "barrage", "doctrine", now);
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
		Event(string.Format("ops_release obj=%1 bearing=%2 groups=%3 strength=%4 est=%5 conf=%6", obj.GetLogName(),
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
		if (dist < 150.0)
			return;

		int open;
		for (int i = 1; i <= 5; i++)
		{
			vector s = stagingPos + (objPos - stagingPos) * (1.0 * i / 6.0);
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
		smokePos = DCO_Util.Surface(smokePos);
		CMD_FireMissionRequest req = new CMD_FireMissionRequest(smokePos, SCR_EAIArtilleryAmmoType.SMOKE, now, 3);
		req.m_sSource = "smoke";
		arty.RequestShellImpact(req, now, 3);
		Event(string.Format("ops_smoke obj=%1 open_samples=%2 pos=%3", obj.GetLogName(), open, smokePos));
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
				Event(string.Format("ops_lift obj=%1 group=%2 cancelled=%3", obj.GetLogName(), g.GetOwner().GetName(), lifted));
				break;
			}
		}
	}

	protected string NearLiveObjective(vector p, CMD_AICommanderObjectiveComponent obj, FactionKey selfFk, out float dist)
	{
		dist = -1;
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (selfFk.IsEmpty() || !mgr)
			return "";
		foreach (CMD_AICommanderObjectiveComponent y : mgr.m_aObjective)
		{
			if (!y || y == obj || !y.GetOwner())
				continue;
			FactionKey owner = y.GetOwnerFaction();
			if (owner.IsEmpty() || owner == selfFk)
				continue;
			float d = vector.DistanceXZ(p, y.GetOwner().GetOrigin());
			if (d <= y.GetRadius() + 200.0)
			{
				dist = d;
				return y.GetLogName();
			}
		}
		return "";
	}

	vector FindOverwatch(CMD_AICommanderObjectiveComponent obj, vector from, float minD = -1, float maxD = -1, float prefD = -1, FactionKey selfFk = "")
	{
		vector objPos = obj.GetOwner().GetOrigin();
		float r = obj.GetRadius();
		if (minD < 0)
			minD = r + 150;
		if (maxD < 0)
			maxD = r + 350;
		if (prefD < 0)
			prefD = r + 250;
		int rejected = 0;
		string nearName = "-";
		float nearD = -1;
		vector found = FindOverwatchBand(obj, from, minD, maxD, prefD, selfFk, rejected, nearName, nearD);
		string band = string.Format("%1-%2", Math.Round(minD - r), Math.Round(maxD - r));
		if (found == vector.Zero && rejected > 0)
		{
			found = FindOverwatchBand(obj, from, maxD, maxD + 200.0, maxD + 100.0, selfFk, rejected, nearName, nearD);
			band = string.Format("%1-%2", Math.Round(maxD - r), Math.Round(maxD + 200.0 - r));
		}
		if (found == vector.Zero)
		{
			found = CMD_ReconSpotFinder.FindBestReconSpot(from, objPos, minD, maxD, 12);
			float fd;
			string fn = NearLiveObjective(found, obj, selfFk, fd);
			if (found != vector.Zero && !fn.IsEmpty())
			{
				rejected++;
				nearName = fn;
				nearD = fd;
				found = vector.Zero;
			}
			band = "recon";
		}
		if (!selfFk.IsEmpty() && rejected > 0)
		{
			DCO_Perf.Count("ow_site:rejected_near_live");
			if (found == vector.Zero)
				Event(string.Format("op_fix_site obj=%1 result=no_safe_site rejected_blocked=%2 nearest_live=%3:%4 band=%5", obj.GetLogName(), rejected, nearName, Math.Round(nearD), band));
			else
				Event(string.Format("op_fix_site obj=%1 pos=%2 rejected_blocked=%3 nearest_live=%4:%5 band=%6", obj.GetLogName(), DCO_Util.XZ(found), rejected, nearName, Math.Round(nearD), band));
		}
		return found;
	}

	protected vector FindOverwatchBand(CMD_AICommanderObjectiveComponent obj, vector from, float minD, float maxD, float prefD, FactionKey selfFk, inout int rejected, inout string nearName, inout float nearD)
	{
		vector objPos = obj.GetOwner().GetOrigin();
		float r = obj.GetRadius();
		array<DCO_TerrainPoint> pts = {};
		DCO_TerrainCache.Query(DCO_ETerrainFlag.OVERWATCH | DCO_ETerrainFlag.HILLTOP | DCO_ETerrainFlag.FOREST_EDGE, objPos, maxD + 50, pts, obj.GetOwner().GetName());

		vector side = from - objPos;
		side[1] = 0;
		side.Normalize();
		float best = -float.MAX;
		vector bestPos;
		float owRaw = 0;
		float owMult = 1.0;
		float owN = 0;
		foreach (DCO_TerrainPoint p : pts)
		{
			float d = vector.DistanceXZ(p.m_vPos, objPos);
			if (d < minD || d > maxD)
				continue;
			vector to = p.m_vPos - objPos;
			to[1] = 0;
			to.Normalize();
			float facing = to[0] * side[0] + to[2] * side[2];
			if (facing < 0.5)
				continue;
			vector eye = p.m_vPos + Vector(0, 1.5, 0);
			vector aim = objPos + Vector(0, 2.0, 0);
			float losLen = vector.Distance(eye, aim);
			if (losLen * (1.0 - DCO_Util.TraceFrac(eye, aim, null, TraceFlags.WORLD | TraceFlags.ENTS, -1, "t:overwatch_los")) > r + 10.0)
				continue;
			float ld;
			string ln = NearLiveObjective(p.m_vPos, obj, selfFk, ld);
			if (!ln.IsEmpty())
			{
				rejected++;
				if (nearD < 0 || ld < nearD)
				{
					nearD = ld;
					nearName = ln;
				}
				continue;
			}
			float s = p.m_fRelElev + facing * 10 - vector.DistanceXZ(p.m_vPos, from) / 100 - Math.AbsFloat(d - prefD) / 50;
			if ((p.m_iFlags & (DCO_ETerrainFlag.OVERWATCH | DCO_ETerrainFlag.HILLTOP)) != 0)
				s += 3;
			float pn = 0;
			float pm = DCO_CmdLearn.TerrainMult("overwatch", p.m_vPos, 25.0, pn);
			float sRaw = s;
			s = DCO_CmdLearn.ApplyScore(s, pm);
			if (s > best)
			{
				best = s;
				bestPos = p.m_vPos;
				owRaw = sRaw;
				owMult = pm;
				owN = pn;
			}
		}
		if (best > -float.MAX)
		{
			if (owN > 0)
				DCO_CmdLearn.LogUse("overwatch", DCO_LearnStore.PosKey(bestPos, 25.0), owRaw, owMult, owN);
			return bestPos;
		}
		return vector.Zero;
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
			if (cmd.DCO_PlanGuard(g, "ops_consolidate", obj))
				continue;

			c.m_aGroups.Insert(g);
			c.m_fStrengthStart += g.GetUnitCount();
			if (!g.IsArmor() && DCO_GroupJobs.Allows(g, DCO_EGroupJob.GARRISON) && (!garrison || g.GetUnitCount() > garrison.GetUnitCount()))
				garrison = g;
		}

		c.m_fHold = 480;
		DCO_Doctrine cdoc = cmd.GetDoctrine();
		if (cdoc)
			c.m_fHold = cdoc.Number(8);
		if (cmd.DCO_AggProfile())
			c.m_fHold = Math.Max(c.m_fHold / Math.Pow(cmd.DCO_AggrA(), 0.5), Math.Min(c.m_fHold, 120.0));
		c.m_vAxis = ConsolidateAxis(cmd, obj);
		int slot;
		foreach (DCO_GroupUtilityComponent g2 : c.m_aGroups)
		{
			if (g2 == garrison)
				continue;
			g2.CompleteAllWaypoints();
			g2.SetGroupObjective(null);
			g2.SetTask(DCO_EGroupTask.NONE);
			g2.DCO_SetHold(now + c.m_fHold);
			float off = ((slot + 1) / 2) * 50.0;
			if (slot % 2 == 1)
				off = -off;
			slot++;
			float yaw = Math.Atan2(c.m_vAxis[0], c.m_vAxis[2]) + off * Math.DEG2RAD;
			vector sec = DCO_Util.Surface(objPos + Vector(Math.Sin(yaw), 0, Math.Cos(yaw)) * Math.Max(obj.GetRadius() * 0.8, 30.0));
			SCR_AIWaypoint wp = cmd.SpawnMoveWP(sec);
			if (wp)
				g2.MoveTo(wp, now);
		}
		string gname = "none";
		if (garrison)
		{
			cmd.StartObjectiveGarrison(garrison, obj, now);
			c.m_aGroups.RemoveItem(garrison);
			c.m_Garrison = garrison;
			gname = garrison.GetOwner().GetName();
		}

		DCO_CommanderDefense cdef = cmd.GetDefense();
		if (cdef && cmd.IsOPEnabled() && cdef.OPCount() < DCO_Force.Max(cmd, "op", cmd.GetMaxOP()))
			cdef.OpenOP(cmd, DCO_Util.Surface(objPos + c.m_vAxis * (obj.GetRadius() + 150.0)), objPos + c.m_vAxis * (obj.GetRadius() + 500.0), "consolidate", now);
		c.m_iStep = 1;
		Event(string.Format("consolidate_step obj=%1 step=1 name=security t_s=0 groups=%2 axis=%3 op=%4 garrison_first=%5", obj.GetLogName(), c.m_aGroups.Count(), DCO_Util.XZ(objPos + c.m_vAxis * 100.0), DCO_Util.B01(cdef != null), gname));

		if (cmd.GetLogistics())
			cmd.GetLogistics().ForceScan();

		m_mPrep.Remove(obj);
		m_mFeint.Remove(obj);
		DCO_ObjectiveMemory mem = m_mMemory.Get(obj);
		if (mem && mem.m_Active)
		{
			Event(string.Format("ops_attempt obj=%1 result=success bearing=%2 time=%3s", obj.GetLogName(), Math.Round(mem.m_Active.m_fBearing), Math.Round(now - mem.m_Active.m_fStart)));
			mem.m_Active = null;
			mem.m_iFails = 0;
			mem.m_aFailedBearings.Clear();
		}

		m_aConsolidations.Insert(c);
		Event(string.Format("ops_consolidate obj=%1 garrison=%2 gathering=%3 strength=%4 hold_s=%5", obj.GetLogName(), gname, c.m_aGroups.Count(), c.m_fStrengthStart, Math.Round(c.m_fHold)));
	}

	protected vector ConsolidateAxis(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj)
	{
		vector objPos = obj.GetOwner().GetOrigin();
		FactionKey fk = cmd.GetCommanderFactionKey();
		float best = float.MAX;
		vector dir = objPos - cmd.DCO_AttackerCentroid();
		foreach (CMD_AICommanderObjectiveComponent o : cmd.DCO_AllObjectives())
		{
			if (!o || o == obj || !o.GetOwner() || o.IsCapturedBy(fk))
				continue;
			float d = vector.DistanceXZ(o.GetOwner().GetOrigin(), objPos);
			if (d < best)
			{
				best = d;
				dir = o.GetOwner().GetOrigin() - objPos;
			}
		}
		dir[1] = 0;
		if (dir.Length() < 1.0)
			dir = Vector(0, 0, 1);
		dir.Normalize();
		return dir;
	}

	protected void ConsolidateSteps(AICommander_BaseComponent cmd, DCO_Consolidation c, float age)
	{
		if (c.m_iStep == 1 && age >= 60.0)
		{
			c.m_iStep = 2;
			Event(string.Format("consolidate_step obj=%1 step=2 name=clear t_s=%2", c.m_Obj.GetLogName(), Math.Round(age)));
		}
		else if (c.m_iStep == 2 && age >= 150.0)
		{
			c.m_iStep = 3;
			int alive;
			int low;
			foreach (DCO_GroupUtilityComponent g : c.m_aGroups)
			{
				if (!g)
					continue;
				alive += g.GetUnitCount();
				if (g.HasState(DCO_EGroupState.COMBAT_INEFFECTIVE))
					low++;
			}
			Event(string.Format("consolidate_step obj=%1 step=3 name=lace t_s=%2 alive=%3 start=%4 groups_ineffective=%5", c.m_Obj.GetLogName(), Math.Round(age), alive, Math.Round(c.m_fStrengthStart), low));
		}
		else if (c.m_iStep == 3 && age >= 240.0)
		{
			c.m_iStep = 4;
			if (cmd.GetLogistics())
				cmd.GetLogistics().ForceScan();
			Event(string.Format("consolidate_step obj=%1 step=4 name=redistribute t_s=%2", c.m_Obj.GetLogName(), Math.Round(age)));
		}
		else if (c.m_iStep == 4 && age >= Math.Min(300.0, c.m_fHold - 30.0))
		{
			c.m_iStep = 5;
			Event(string.Format("consolidate_step obj=%1 step=5 name=ready_counterattack t_s=%2", c.m_Obj.GetLogName(), Math.Round(age)));
		}
	}

	protected void TryLateGarrison(AICommander_BaseComponent cmd, DCO_Consolidation c, float now)
	{
		vector objPos = c.m_Obj.GetOwner().GetOrigin();
		float gatherR = c.m_Obj.GetRadius() + 400;
		DCO_GroupUtilityComponent best;
		float bestD = float.MAX;
		foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
		{
			if (!g || g.IsPlayerGroup() || g.IsDedicatedTransport() || g.IsMortar() || g.IsInTransport() || g.IsArmor())
				continue;
			float d = vector.DistanceXZ(g.GetOwner().GetOrigin(), objPos);
			if (g.GetGroupObjective() != c.m_Obj && d > gatherR)
				continue;
			DCO_EGroupTask t = g.GetTask();
			if (t != DCO_EGroupTask.ATTACK && t != DCO_EGroupTask.FLANK && t != DCO_EGroupTask.SUPPORT_BY_FIRE && t != DCO_EGroupTask.NONE)
				continue;
			if (cmd.DCO_PlanGuard(g, "ops_consolidate_late", c.m_Obj))
				continue;
			if (!DCO_GroupJobs.Allows(g, DCO_EGroupJob.GARRISON))
				continue;
			if (d < bestD)
			{
				bestD = d;
				best = g;
			}
		}
		if (!best)
			return;
		cmd.StartObjectiveGarrison(best, c.m_Obj, now);
		c.m_aGroups.RemoveItem(best);
		c.m_Garrison = best;
		DCO_Perf.Count("ops:consolidate_late_garrison");
		Event(string.Format("ops_consolidate_garrison obj=%1 grp=%2 delay_s=%3 dist=%4", c.m_Obj.GetLogName(), best.GetOwner().GetName(), Math.Round(now - c.m_fStart), Math.Round(bestD)));
	}

	protected bool EnemyNearObjective(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, float now)
	{
		CMD_ThreatResponseComponent tr = cmd.GetThreatResponseComponent();
		if (!tr || !obj.GetOwner())
			return false;
		vector objPos = obj.GetOwner().GetOrigin();
		float r = obj.GetRadius() + 400.0;
		FactionKey fk = cmd.GetCommanderFactionKey();
		foreach (CMD_ThreatEntry t : tr.GetThreats())
		{
			if (!t || now - t.m_fLastUpdateTime >= 60.0 || vector.DistanceXZ(t.m_vBelievedPos, objPos) > r)
				continue;
			bool atOther;
			foreach (CMD_AICommanderObjectiveComponent o : cmd.DCO_AllObjectives())
			{
				if (!o || o == obj || !o.GetOwner() || o.IsCapturedBy(fk))
					continue;
				if (vector.DistanceXZ(t.m_vBelievedPos, o.GetOwner().GetOrigin()) <= o.GetRadius() + 100.0)
				{
					atOther = true;
					break;
				}
			}
			if (atOther)
			{
				DCO_Perf.Count("ops:consolidate_ignore_other_obj");
				continue;
			}
			return true;
		}
		return false;
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
			if (!c.m_Garrison)
				TryLateGarrison(cmd, c, now);
			float age = now - c.m_fStart;
			ConsolidateSteps(cmd, c, age);
			if (age < c.m_fHold)
				continue;

			bool enemyNear = EnemyNearObjective(cmd, c.m_Obj, now);
			if (enemyNear && age < 900.0)
			{
				foreach (DCO_GroupUtilityComponent hg : c.m_aGroups)
				{
					if (hg)
						hg.DCO_SetHold(now + 60.0);
				}
				if (!c.m_bBlockLogged)
				{
					c.m_bBlockLogged = true;
					DCO_Perf.Count("ops:consolidate_blocked");
					Event(string.Format("ops_consolidate_release obj=%1 hold_s=%2 release=blocked_enemy_near", c.m_Obj.GetLogName(), Math.Round(age)));
				}
				continue;
			}

			float strength = 0;
			foreach (DCO_GroupUtilityComponent g : c.m_aGroups)
			{
				if (g)
				{
					strength += g.GetUnitCount();
					g.DCO_SetHold(0);
				}
			}
			string release = "reserve";
			if (enemyNear)
				release = "cap";
			string gname = "none";
			if (c.m_Garrison && c.m_Garrison.GetOwner())
				gname = c.m_Garrison.GetOwner().GetName();
			c.m_iStep = 6;
			Event(string.Format("consolidate_step obj=%1 step=6 name=report t_s=%2", c.m_Obj.GetLogName(), Math.Round(age)));
			DCO_Perf.Count("ops:consolidate_release_" + release);
			Event(string.Format("ops_consolidate_release obj=%1 hold_s=%2 release=%3 garrison=%4 released=%5", c.m_Obj.GetLogName(), Math.Round(age), release, gname, c.m_aGroups.Count()));

			float ratio = strength / Math.Max(c.m_fStrengthStart, 1);
			CMD_AICommanderObjectiveComponent nextObj = NextOnAxis(cmd, c.m_Obj);
			string decision;
			string why = "ok";
			if (ratio >= 0.6 && cmd.DCO_EffAggression() >= 0.5 && nextObj && !enemyNear)
			{
				m_ExploitHint = nextObj;
				m_Main = null;
				decision = "exploit";
			}
			else
			{
				m_fPauseUntil = now + 180.0;
				decision = "pause";
				if (ratio < 0.6)
					why = "weak";
				else if (cmd.DCO_EffAggression() < 0.5)
					why = "cautious";
				else if (!nextObj)
					why = "no_next";
				else
					why = "enemy_near";
			}

			string next = "none";
			if (nextObj)
				next = nextObj.GetLogName();
			Event(string.Format("ops_after_capture obj=%1 decision=%2 strength_ratio=%3 next=%4 why=%5", c.m_Obj.GetLogName(), decision, ratio.ToString(-1, 2), next, why));
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
			if (!o || o == from || o.IsCapturedBy(cmd.GetCommanderFactionKey()) || cmd.IsObjectiveOnCooldown(o) || !cmd.DCO_MayAttack(o))
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

	void DCO_ForceMain(CMD_AICommanderObjectiveComponent o)
	{
		if (!o || m_Main == o)
			return;
		string before = "none";
		if (m_Main)
			before = m_Main.GetLogName();
		m_Main = o;
		Event(string.Format("ops_axis_focus main=%1 before=%2", o.GetLogName(), before));
	}

	void PlanAxis(AICommander_BaseComponent cmd, notnull array<CMD_AICommanderObjectiveComponent> ranked, float now)
	{
		if (now < m_fPauseUntil)
		{
			ranked.Clear();
			return;
		}

		FactionKey fk = cmd.GetCommanderFactionKey();
		if (m_Main && (m_Main.IsCapturedBy(fk) || cmd.IsObjectiveOnCooldown(m_Main) || !cmd.DCO_MayAttack(m_Main)))
			m_Main = null;

		CMD_AICommanderObjectiveComponent prevMain = m_Main;
		if (!m_Main && m_ExploitHint && !m_ExploitHint.IsCapturedBy(fk) && !cmd.IsObjectiveOnCooldown(m_ExploitHint) && cmd.DCO_MayAttack(m_ExploitHint))
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
		float bestD = 700.0;
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
				sname = m_Support.GetLogName();
			Event(string.Format("ops_axis main=%1 support=%2 phase_lines=%3 opportunities=%4", m_Main.GetLogName(), sname, onAxis.Count(), opportunity.Count()));
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

		vector pos = FindOverwatch(m_Support, g.GetOwner().GetOrigin(), -1, -1, -1, cmd.GetCommanderFactionKey());
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
		Event(string.Format("ops_support_fire support=%1 main=%2 group=%3", m_Support.GetLogName(), m_Main.GetLogName(), g.GetOwner().GetName()));
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
			DCO_CmdLearn.LoadFailBearings(obj, m.m_aFailedBearings);
			m_mMemory.Set(obj, m);
		}
		return m;
	}

	protected void UpdateAttempts(AICommander_BaseComponent cmd, float now)
	{
		DecayFails(now);
		FactionKey fk = cmd.GetCommanderFactionKey();
		foreach (CMD_AICommanderObjectiveComponent obj, DCO_ObjectiveMemory mem : m_mMemory)
		{
			if (!obj || !mem || !mem.m_Active || obj.IsCapturedBy(fk))
				continue;

			bool exhausted = !HasOperationGroups(cmd, obj);
			if (!exhausted && now - mem.m_Active.m_fStart < 900.0)
				continue;

			float casualties = mem.m_Active.m_fStrength - cmd.GetAttackStrength(obj);
			mem.m_aFailedBearings.Insert(mem.m_Active.m_fBearing);
			DCO_CmdLearn.NoteFailBearing(obj, mem.m_Active.m_fBearing);
			Event(string.Format("ops_attempt obj=%1 result=fail why=%2 bearing=%3 groups=%4 casualties=%5 fails=%6", obj.GetLogName(),
				exhausted, Math.Round(mem.m_Active.m_fBearing), mem.m_Active.m_iGroups, Math.Round(casualties), mem.m_iFails + 1));
			mem.m_Active = null;
			AddFail(cmd, obj, mem, now);
		}
	}

	void NoteFail(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, float now, string why)
	{
		if (!obj)
			return;
		DCO_ObjectiveMemory mem = GetMemory(obj);
		Event(string.Format("ops_attempt obj=%1 result=fail why=%2 fails=%3", obj.GetLogName(), why, mem.m_iFails + 1));
		mem.m_Active = null;
		AddFail(cmd, obj, mem, now);
	}

	protected void AddFail(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, DCO_ObjectiveMemory mem, float now)
	{
		mem.m_iFails++;
		mem.m_fFailAt = now;
		m_mPrep.Remove(obj);
		m_mFeint.Remove(obj);
		if (mem.m_iFails < 3)
			return;

		float minutes = Math.Lerp(5, 15, cmd.GetPatience());
		cmd.SetObjectiveCooldown(obj, minutes * 60);
		if (m_Main == obj)
			m_Main = null;
		Event(string.Format("ops_deprioritize obj=%1 fails=%2 minutes=%3", obj.GetLogName(), mem.m_iFails, Math.Round(minutes)));
		mem.m_iFails = 0;
	}

	protected void DecayFails(float now)
	{
		foreach (CMD_AICommanderObjectiveComponent obj, DCO_ObjectiveMemory mem : m_mMemory)
		{
			if (!obj || !mem || mem.m_iFails <= 0 || now - mem.m_fFailAt < 900.0)
				continue;
			mem.m_iFails--;
			mem.m_fFailAt = now;
			Event(string.Format("ops_fail_decay obj=%1 fails=%2", obj.GetLogName(), mem.m_iFails));
		}
	}

	vector AdjustApproachAxis(CMD_AICommanderObjectiveComponent obj, vector axis)
	{
		if (!obj)
			return axis;
		DCO_ObjectiveMemory mem = GetMemory(obj);
		bool explore = DCO_CmdLearn.Explore();
		array<float> rots = {0, 70, -70, 110, -110, 180};
		vector bestA = axis;
		float bestScore = -1;
		float bestRot = 0;
		float bestBase = 0;
		float bestMult = 1.0;
		float bestN = 0;
		float bestApproach = BearingOfDir(-axis);
		foreach (float rot : rots)
		{
			vector a = RotateXZ(axis, rot);
			float approach = BearingOfDir(-a);
			bool clear = true;
			foreach (float fb : mem.m_aFailedBearings)
			{
				if (AngleDiff(approach, fb) < 45.0)
				{
					clear = false;
					break;
				}
			}
			if (!clear)
				continue;
			float rotBase = 1.0 - Math.AbsFloat(rot) / 360.0;
			float n = 0;
			float m = 1.0;
			if (!explore)
				m = DCO_CmdLearn.ApproachMult(obj, approach, n);
			float sc = rotBase * m;
			if (sc > bestScore)
			{
				bestScore = sc;
				bestA = a;
				bestRot = rot;
				bestBase = rotBase;
				bestMult = m;
				bestN = n;
				bestApproach = approach;
			}
		}
		if (bestScore < 0)
			return axis;
		if (bestRot != 0)
			Event(string.Format("ops_avoid_bearing obj=%1 rotate=%2 new_bearing=%3", obj.GetLogName(), bestRot, Math.Round(bestApproach)));
		if (explore)
			DCO_CmdLearn.LogExplore("approach", obj.GetLogName() + "|" + Math.Round(bestApproach).ToString());
		else
			DCO_CmdLearn.LogUse("approach", obj.GetLogName() + "|" + Math.Round(bestApproach).ToString(), bestBase, bestMult, bestN);
		return bestA;
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
		s.m_vOverwatch = DCO_ArmorPair.HullDown(overwatch, obj.GetOwner().GetOrigin());
		m_aArmor.Insert(s);
		Event(string.Format("ops_armor_overwatch obj=%1 armor=%2 pos=%3", obj.GetLogName(), armor.GetOwner().GetName(), overwatch));
	}

	void DbgArmor(notnull array<vector> outPos)
	{
		foreach (DCO_ArmorSupport s : m_aArmor)
		{
			if (s && s.m_Armor && s.m_vOverwatch != vector.Zero)
				outPos.Insert(s.m_vOverwatch);
		}
	}

	void MoveArmorOverwatch(DCO_GroupUtilityComponent armor, vector pos)
	{
		foreach (DCO_ArmorSupport s : m_aArmor)
		{
			if (!s || s.m_Armor != armor)
				continue;
			s.m_vOverwatch = pos;
			s.m_bSbfLogged = false;
			s.m_sMoveWhy = "fix_bound";
			Event(string.Format("ops_armor_overwatch obj=%1 armor=%2 pos=%3 reason=fix_bound", s.m_Obj.GetLogName(), armor.GetOwner().GetName(), DCO_Util.XZ(pos)));
		}
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
			if (DCO_GlobalAIComponent.BehaviorOnCommander(cmd, DCO_EBehavior.COMBINED_ARMS))
			{
				DCO_GroupUtilityComponent spotter = NearestInfantry(cmd, s.m_Obj, apos);
				if (spotter && vector.DistanceXZ(spotter.GetOwner().GetOrigin(), apos) < 400.0)
					DCO_ArmorPair.MarkTargets(spotter.GetGroupUtilityComponent(), s.m_Armor.GetGroupUtilityComponent(), now);
			}

			bool fmHold = false;
			if (fmHold && FixDrift(cmd, s, apos, now))
				continue;
			float pullR = 150.0;
			IEntity sveh = s.m_Armor.GetGroupVehicle();
			int scls = DCO_VehicleActive.VehClass(sveh);
			SCR_AIGroup sgrp = SCR_AIGroup.Cast(s.m_Armor.GetOwner());
			SCR_ChimeraAIAgent sla;
			if (sgrp)
				sla = SCR_ChimeraAIAgent.Cast(sgrp.GetLeaderAgent());
			if (scls > 0 && sla && sla.m_UtilityComponent)
				pullR = DCO_VehicleActive.HideDist(sla.m_UtilityComponent, scls);
			CMD_ThreatEntry danger = NearestThreat(cmd, apos, pullR, true);
			if (danger && s.m_iState != 3 && s.m_iState != 0)
			{
				s.m_iState = 3;
				s.m_sMoveWhy = "pullback";
				MoveGroup(cmd, s.m_Armor, s.m_vOverwatch, now);
				if (fmHold)
					Event(string.Format("tr_fix_vehicle_move grp=%1 obj=%2 reason=pullback dist_ow=%3 to=overwatch", s.m_Armor.GetOwner().GetName(), s.m_Obj.GetLogName(), Math.Round(vector.DistanceXZ(apos, s.m_vOverwatch))));
				DCO_GroupUtilityComponent inf = NearestInfantry(cmd, s.m_Obj, danger.m_vBelievedPos);
				if (inf)
					MoveGroup(cmd, inf, danger.m_vBelievedPos, now);
				string reason = "at";
				if (danger.m_bArmorSeen)
					reason = "armor";
				DCO_Perf.Count("veh:pullback_at");
				Event(string.Format("ops_armor_pullback obj=%1 armor=%2 reason=%3 escort=%4", s.m_Obj.GetLogName(), s.m_Armor.GetOwner().GetName(), reason, inf != null));
				continue;
			}
			if (s.m_iState == 3)
			{
				if (!danger)
					s.m_iState = 0;
				continue;
			}

			CMD_ThreatEntry infThreat = NearestThreat(cmd, apos, 250.0, false);
			if (infThreat && s.m_iState != 4 && fmHold)
			{
				s.m_iState = 4;
				DCO_Perf.Count("tr_fix_vehicle_move:engage_inf_held");
				Event(string.Format("tr_fix_vehicle_move grp=%1 obj=%2 reason=engage_inf action=held_hull_down threat_dist=%3", s.m_Armor.GetOwner().GetName(), s.m_Obj.GetLogName(), Math.Round(vector.DistanceXZ(apos, infThreat.m_vBelievedPos))));
				continue;
			}
			if (infThreat && s.m_iState != 4)
			{
				s.m_iState = 4;
				s.m_sMoveWhy = "engage_inf";
				MoveGroup(cmd, s.m_Armor, DCO_ArmorPair.HullDown(ArmorEngagePos(apos, infThreat.m_vBelievedPos, objPos, s.m_Obj.GetRadius()), infThreat.m_vBelievedPos), now);
				Event(string.Format("ops_armor_engage_inf obj=%1 armor=%2 dist=%3", s.m_Obj.GetLogName(), s.m_Armor.GetOwner().GetName(), Math.Round(vector.DistanceXZ(apos, infThreat.m_vBelievedPos))));
				continue;
			}
			if (s.m_iState == 4)
			{
				if (!infThreat)
					s.m_iState = 0;
				continue;
			}

			bool atKnown = ObjectiveHasATOrArmor(cmd, s.m_Obj);
			if (s.m_bLift && !fmHold)
			{
				s.m_bLift = false;
				if (s.m_Armor.GetSOPOverride() == DCO_ESOPAction.OBSERVE)
					s.m_Armor.SetSOPOverride(s.m_iSopPrev);
			}
			if (s.m_iState == 0 && (!cmd.IsAssaultReleased(s.m_Obj) || fmHold))
			{
				AIGroup aag = AIGroup.Cast(s.m_Armor.GetOwner());
				float owDist = vector.DistanceXZ(apos, s.m_vOverwatch);
				if (owDist > 40.0 && aag && !aag.GetCurrentWaypoint() && !s.m_Armor.IsMoving())
				{
					s.m_sMoveWhy = "reorder";
					MoveGroup(cmd, s.m_Armor, s.m_vOverwatch, now);
					DCO_Perf.Count("veh:sbf_reorder");
					Event(string.Format("task_no_wp grp=%1 task=SUPPORT_BY_FIRE action=reorder obj=%2 dist=%3", s.m_Armor.GetOwner().GetName(), s.m_Obj.GetLogName(), Math.Round(owDist)));
				}
				else if (owDist <= 40.0 && !s.m_bSbfLogged)
				{
					s.m_bSbfLogged = true;
					Event(string.Format("veh_sbf grp=%1 obj=%2 pos=%3 dist=%4 state=in_position", s.m_Armor.GetOwner().GetName(), s.m_Obj.GetLogName(), DCO_Util.XZ(apos), Math.Round(vector.DistanceXZ(apos, objPos))));
				}
				continue;
			}
			if (s.m_iState == 0)
			{
				if (atKnown)
				{
					if (!cmd.IsAssaultReleased(s.m_Obj))
						continue;
					if (!InfantryWithin(cmd, s.m_Obj, objPos, s.m_Obj.GetRadius()))
						continue;
					float enemyStr = EnemyStrengthAt(cmd, s.m_Obj);
					float friendStr = FriendlyStrengthAt(cmd, s.m_Obj);
					if (friendStr < enemyStr * 0.8)
						continue;
					DCO_Perf.Count("veh:push_ratio");
					Event(string.Format("ops_armor_push_ratio obj=%1 armor=%2 friend=%3 enemy=%4 need=0.8", s.m_Obj.GetLogName(), s.m_Armor.GetOwner().GetName(), friendStr.ToString(-1, 1), enemyStr.ToString(-1, 1)));
				}
				else if (!HasOperationGroups(cmd, s.m_Obj))
					continue;
				s.m_iState = 1;
				s.m_sMoveWhy = "push_after_release";
				MoveGroup(cmd, s.m_Armor, objPos, now);
				if (atKnown)
					Event(string.Format("ops_armor_enter obj=%1 armor=%2", s.m_Obj.GetLogName(), s.m_Armor.GetOwner().GetName()));
				else
				{
					DCO_Perf.Count("veh:push_no_at");
					Event(string.Format("ops_armor_with_assault obj=%1 armor=%2", s.m_Obj.GetLogName(), s.m_Armor.GetOwner().GetName()));
				}
				continue;
			}

			bool close = IsCloseTerrain(s, apos, now);
			bool escorted;
			string waitWhy;
			if (atKnown)
			{
				escorted = InfantryWithin(cmd, s.m_Obj, apos, 80.0);
				waitWhy = "at_active";
			}
			else if (close)
			{
				escorted = InfantryAhead(cmd, s.m_Obj, apos, objPos, 30.0);
				waitWhy = "close_terrain";
			}
			else
			{
				escorted = InfantryWithin(cmd, s.m_Obj, apos, 150.0) || InfantryAhead(cmd, s.m_Obj, apos, objPos, 0);
				waitWhy = "lead_limit";
			}

			if (s.m_iState == 1 && !escorted)
			{
				s.m_iState = 2;
				s.m_Armor.CompleteAllWaypoints();
				DCO_Perf.Count("veh:wait_" + waitWhy);
				Event(string.Format("ops_armor_wait obj=%1 armor=%2 why=%3", s.m_Obj.GetLogName(), s.m_Armor.GetOwner().GetName(), waitWhy));
			}
			else if (s.m_iState == 2 && escorted)
			{
				s.m_iState = 1;
				MoveGroup(cmd, s.m_Armor, objPos, now);
			}
		}
	}


	protected bool FixDrift(AICommander_BaseComponent cmd, DCO_ArmorSupport s, vector apos, float now)
	{
		float d = vector.DistanceXZ(apos, s.m_vOverwatch);
		if (d <= 50.0 && s.m_sMoveWhy == "drift_return")
			s.m_sMoveWhy = "held";
		if (!s.m_bSbfLogged || d <= 50.0 || !s.m_Armor.IsMoving())
			return false;
		if (now >= s.m_fMoveLogAt)
		{
			s.m_fMoveLogAt = now + 20.0;
			AIGroup ag = AIGroup.Cast(s.m_Armor.GetOwner());
			string wp = "none";
			if (ag && ag.GetCurrentWaypoint())
				wp = ag.GetCurrentWaypoint().ClassName();
			DCO_Perf.Count("tr_fix_vehicle_move:" + s.m_sMoveWhy);
			Event(string.Format("tr_fix_vehicle_move grp=%1 obj=%2 reason=%3 dist_ow=%4 wp=%5 task=%6 action=return_hull_down", s.m_Armor.GetOwner().GetName(), s.m_Obj.GetLogName(), s.m_sMoveWhy, Math.Round(d), wp, typename.EnumToString(DCO_EGroupTask, s.m_Armor.GetTask())));
		}
		if (s.m_sMoveWhy == "drift_return")
			return false;
		s.m_sMoveWhy = "drift_return";
		MoveGroup(cmd, s.m_Armor, s.m_vOverwatch, now);
		return true;
	}

	protected static vector ArmorEngagePos(vector apos, vector threatPos, vector objPos, float objRadius)
	{
		vector dir = apos - threatPos;
		dir[1] = 0;
		if (dir.Length() < 1)
			dir = objPos - threatPos;
		dir[1] = 0;
		dir.Normalize();

		vector pos = threatPos + dir * 60.0;
		vector fromObj = pos - objPos;
		fromObj[1] = 0;
		float leash = objRadius + 150.0;
		if (fromObj.Length() > leash)
			pos = objPos + fromObj.Normalized() * leash;
		pos = DCO_Util.Surface(pos);
		return pos;
	}

	protected static bool IsArmorDanger(CMD_ThreatEntry t)
	{
		return (t.m_bArmorSeen || t.m_bATSeen) && DCO_Util.NowS() - t.m_fLastUpdateTime <= 60.0;
	}

	protected static bool IsCloseTerrain(DCO_ArmorSupport s, vector pos, float now)
	{
		if (now - s.m_fTerrainAt < 20.0)
			return s.m_bClose;
		s.m_fTerrainAt = now;
		s.m_bClose = false;
		DCO_GarrisonRegistry reg = DCO_GarrisonRegistry.GetInstance();
		if (reg)
		{
			array<IEntity> buildings = {};
			reg.FindBuildings(pos, 60.0, buildings);
			s.m_bClose = buildings.Count() >= 4;
		}
		if (!s.m_bClose)
		{
			array<DCO_TerrainPoint> pts = {};
			s.m_bClose = DCO_TerrainCache.Query(DCO_ETerrainFlag.FOREST_EDGE, pos, 80.0, pts) >= 3;
		}
		return s.m_bClose;
	}

	protected static bool InfantryAhead(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, vector apos, vector objPos, float lead)
	{
		float armorToObj = vector.DistanceXZ(apos, objPos);
		foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
		{
			if (!g || g.IsArmor() || g.GetGroupObjective() != obj || g.IsInTransport())
				continue;
			vector gp = g.GetOwner().GetOrigin();
			if (vector.DistanceXZ(gp, apos) <= 150.0 && vector.DistanceXZ(gp, objPos) <= armorToObj - lead)
				return true;
		}
		return false;
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
		int raw;
		float total = cmd.GetOps().Estimate(cmd, p, r, DCO_Util.NowS(), false, raw);
		bool at, armor;
		foreach (CMD_ThreatEntry t : tr.GetThreats())
		{
			if (!t || vector.DistanceXZ(t.m_vBelievedPos, p) > r)
				continue;
			at = at || t.m_bATSeen;
			armor = armor || t.m_bArmorSeen;
		}
		if (at)
			total += 2.0;
		if (armor)
			total += 8.0;
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
		float nowS = DCO_Util.NowS();
		foreach (CMD_ThreatEntry t : tr.GetThreats())
		{
			if (!t || IsArmorDanger(t) != armorDanger || nowS - t.m_fLastUpdateTime > 60.0)
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
			if (!j.m_From || !j.m_To || now - j.m_fStart > 180.0 || j.m_From.HasState(DCO_EGroupState.IN_CONTACT) || j.m_To.HasState(DCO_EGroupState.IN_CONTACT))
			{
				if (j.m_From)
					j.m_From.DCO_SetHold(0);
				if (j.m_To)
					j.m_To.DCO_SetHold(0);
				m_aMerges.Remove(i);
				continue;
			}

			if (vector.DistanceXZ(j.m_From.GetOwner().GetOrigin(), j.m_To.GetOwner().GetOrigin()) > 25.0)
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
			if (cmd.DCO_PlanGuard(g, "ops_merge"))
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
				if (vector.DistanceXZ(ga.GetOwner().GetOrigin(), gb.GetOwner().GetOrigin()) > 300.0)
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
				j.m_From.DCO_SetHold(now + 180.0);
				j.m_To.DCO_SetHold(now + 180.0);
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
		from.DCO_SetHold(now + 180.0);
		MoveGroup(cmd, from, to.GetOwner().GetOrigin(), now);
		m_aMerges.Insert(j);
		Event(string.Format("ops_merge_start from=%1 (%2) to=%3 (%4) reason=op_return", from.GetOwner().GetName(), from.GetUnitCount(), to.GetOwner().GetName(), to.GetUnitCount()));
	}

	protected bool TryFeint(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, float now)
	{
		m_mFeint.Set(obj, 0);
		if (!cmd.IsFeintEnabled() || !cmd.IsAttackRatioMet(obj))
			return false;
		if (DCO_Force.Max(cmd, "feint") < 1)
		{
			DCO_Force.LogSkip(cmd, "feint", 1, 1);
			return false;
		}
		if (Math.RandomFloat01() > 0.15 + 0.35 * cmd.DCO_EffAggression() + 0.35 * cmd.GetRiskTaking())
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
			smokePos = DCO_Util.Surface(smokePos);
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
			sname = m_Support.GetLogName();
		Event(string.Format("ops_feint_start main=%1 support=%2 how=%3 lead_s=%4", obj.GetLogName(), sname, how, Math.Round(lead)));
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

	protected ref array<ref DCO_RemnantEntry> m_aRemnants;
	protected ref map<DCO_GroupUtilityComponent, int> m_mRemnantSkip;

	protected static bool HasSpawner(AICommander_BaseComponent cmd)
	{
		foreach (DCO_CommanderSpawnerComponent sp : DCO_CommanderSpawnerComponent.GetAll())
		{
			if (!sp)
				continue;
			if (sp.GetCommanderUID() == cmd.GetCommanderUID() || sp.GetFaction() == cmd.GetCommanderFactionKey())
				return true;
		}
		return false;
	}

	protected void UpdateRemnants(AICommander_BaseComponent cmd, float now)
	{
		if (!m_aRemnants)
		{
			m_aRemnants = {};
			m_mRemnantSkip = new map<DCO_GroupUtilityComponent, int>();
		}
		string src;
		bool spawner = HasSpawner(cmd);
		foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
		{
			if (!g || !g.GetOwner() || g.IsPlayerGroup() || g.IsDedicatedTransport() || g.IsMortar() || g.IsArmor() || IsRemnant(g) || IsMerging(g))
				continue;
			int peak = g.DCO_GetPeakStrength();
			int units = g.GetUnitCount();
			int skipUnits;
			if (m_mRemnantSkip.Find(g, skipUnits))
			{
				if (units <= skipUnits)
					continue;
				m_mRemnantSkip.Remove(g);
			}
			bool thrHit = units > 0 && units < peak && units * 2 <= peak && units <= cmd.DCO_GT_RetreatThreshold() && DCO_Ctx.TuneForCmd(cmd, "cmd.retreat_thr", 1.0) > 0;
			if (peak < 4 || units <= 0 || (!g.HasState(DCO_EGroupState.COMBAT_INEFFECTIVE) && !thrHit))
				continue;
			if (thrHit && !g.HasState(DCO_EGroupState.COMBAT_INEFFECTIVE))
			{
				DCO_Perf.Count("ops:remnant_thr");
				Event(string.Format("remnant_thr grp=%1 units=%2 peak=%3 thr=%4", g.GetOwner().GetName(), units, peak, Math.Round(cmd.DCO_GT_RetreatThreshold())));
			}
			vector gpos = g.GetOwner().GetOrigin();
			vector hub = cmd.GetHubFor(gpos, src);
			if (!spawner)
			{
				m_mRemnantSkip.Set(g, units);
				DCO_Perf.Count("ops:remnant_skip_no_spawner");
				Event(string.Format("remnant_skip grp=%1 units=%2 peak=%3 reason=no_spawner action=stay", g.GetOwner().GetName(), units, peak));
				continue;
			}
			if (vector.DistanceXZ(gpos, hub) <= 60.0 && !g.IsInTransport() && !g.HasState(DCO_EGroupState.IN_CONTACT))
			{
				if (cmd.GetTactics())
					DCO_TacticsCompat.DropFromPlans(cmd.GetTactics(), g);
				cmd.DetachFromObjective(g);
				ReturnToPool(cmd, g, 0, src);
				continue;
			}
			int dropped = 0;
			if (cmd.GetTactics())
				dropped = DCO_TacticsCompat.DropFromPlans(cmd.GetTactics(), g);
			string task = typename.EnumToString(DCO_EGroupTask, g.GetTask());
			cmd.DetachFromObjective(g);
			g.DCO_SetHold(now + 900.0);
			DCO_RemnantEntry re = new DCO_RemnantEntry();
			re.m_G = g;
			re.m_fAt = now;
			re.m_fMoveAt = now;
			re.m_sName = g.GetOwner().GetName();
			m_aRemnants.Insert(re);
			string how = RemnantMove(cmd, g, hub, now);
			DCO_Perf.Count("ops:remnant_pull");
			Event(string.Format("remnant_pull grp=%1 units=%2 peak=%3 task=%4 plans=%5 hub=%6 hub_src=%7 dist=%8 how=%9", g.GetOwner().GetName(), units, peak, task, dropped, DCO_Util.XZ(hub), src, Math.Round(vector.DistanceXZ(gpos, hub)), how));
		}

		for (int ri = m_aRemnants.Count() - 1; ri >= 0; ri--)
		{
			DCO_RemnantEntry e = m_aRemnants[ri];
			DCO_GroupUtilityComponent rg = e.m_G;
			float at = e.m_fAt;
			if (!rg || !rg.GetOwner() || rg.GetUnitCount() <= 0)
			{
				DCO_Perf.Count("ops:remnant_lost");
				Event(string.Format("remnant_lost grp=%1 travel_s=%2 reason=destroyed", e.m_sName, Math.Round(now - at)));
				m_aRemnants.Remove(ri);
				continue;
			}
			vector rpos = rg.GetOwner().GetOrigin();
			vector rhub = cmd.GetHubFor(rpos, src);
			float d = vector.DistanceXZ(rpos, rhub);
			if (d <= 60.0 && !rg.IsInTransport() && !rg.HasState(DCO_EGroupState.IN_CONTACT))
			{
				ReturnToPool(cmd, rg, now - at, src);
				m_aRemnants.Remove(ri);
				continue;
			}
			if (now - at > 900.0)
			{
				rg.DCO_SetHold(0);
				m_mRemnantSkip.Set(rg, rg.GetUnitCount());
				DCO_Perf.Count("ops:remnant_timeout");
				Event(string.Format("remnant_timeout grp=%1 units=%2 dist=%3 action=release", rg.GetOwner().GetName(), rg.GetUnitCount(), Math.Round(d)));
				m_aRemnants.Remove(ri);
				continue;
			}
			float lastMove = e.m_fMoveAt;
			AIGroup ag = AIGroup.Cast(rg.GetOwner());
			if (rg.IsBreakingContact() || rg.IsInTransport() || !ag)
				continue;
			bool stalled = !ag.GetCurrentWaypoint() || rg.HasState(DCO_EGroupState.IN_CONTACT);
			if (now - lastMove > 30.0 && stalled)
			{
				e.m_fMoveAt = now;
				string rhow = RemnantMove(cmd, rg, rhub, now);
				DCO_Perf.Count("ops:remnant_reorder");
				Event(string.Format("remnant_reorder grp=%1 dist=%2 how=%3 contact=%4", rg.GetOwner().GetName(), Math.Round(d), rhow, rg.HasState(DCO_EGroupState.IN_CONTACT)));
			}
		}
	}

	protected bool IsRemnant(DCO_GroupUtilityComponent g)
	{
		foreach (DCO_RemnantEntry e : m_aRemnants)
		{
			if (e.m_G == g)
				return true;
		}
		return false;
	}

	protected string RemnantMove(AICommander_BaseComponent cmd, DCO_GroupUtilityComponent g, vector hub, float now)
	{
		if (g.IsInTransport())
			return "transport";
		vector gpos = g.GetOwner().GetOrigin();
		if (g.HasState(DCO_EGroupState.IN_CONTACT))
		{
			CMD_ThreatEntry t = NearestThreat(cmd, gpos, 400.0, false);
			if (!t)
				t = NearestThreat(cmd, gpos, 400.0, true);
			if (t)
			{
				vector dir = hub - gpos;
				dir[1] = 0;
				float len = dir.Length();
				vector rally = hub;
				if (len > 150.0)
					rally = DCO_Util.Surface(gpos + dir * (150.0 / len));
				if (DCO_BreakContact.Start(g, t.m_vBelievedPos, rally, "op_remnant"))
					return "break_contact";
			}
		}
		MoveGroup(cmd, g, hub, now);
		return "move";
	}

	protected void ReturnToPool(AICommander_BaseComponent cmd, DCO_GroupUtilityComponent g, float travelS, string hubSrc)
	{
		SCR_AIGroup grp = SCR_AIGroup.Cast(g.GetOwner());
		if (!grp)
			return;
		string name = grp.GetName();
		int men = g.GetUnitCount();
		if (!HasSpawner(cmd))
		{
			g.DCO_SetHold(0);
			if (m_mRemnantSkip)
				m_mRemnantSkip.Set(g, men);
			DCO_Perf.Count("ops:remnant_kept");
			Event(string.Format("remnant_kept grp=%1 men=%2 reason=no_spawner hub_src=%3 travel_s=%4", name, men, hubSrc, Math.Round(travelS)));
			return;
		}
		array<AIAgent> agents = {};
		grp.GetAgents(agents);
		foreach (AIAgent a : agents)
		{
			if (!a)
				continue;
			IEntity ent = a.GetControlledEntity();
			if (ent && !SCR_CharacterHelper.IsPlayer(ent))
				SCR_EntityHelper.DeleteEntityAndChildren(ent);
		}
		cmd.RefundStock(men);
		cmd.RefundPoints(men);
		SCR_EntityHelper.DeleteEntityAndChildren(grp);
		DCO_Perf.CountN("ops:remnant_returned_men", men);
		Event(string.Format("remnant_returned grp=%1 men=%2 stock=%3 points=%4 hub_src=%5 travel_s=%6", name, men, cmd.GetReinforcementStock(), cmd.GetPointsStock(), hubSrc, Math.Round(travelS)));
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
