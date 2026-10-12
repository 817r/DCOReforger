class DCO_ForceEval
{
	float m_fRaw;
	float m_fPosture;
	string m_sPosture;
	float m_fEff;
	int m_iBand;
	int m_iBaseBand;
	float m_fConf;
	float m_fOwn;
	float m_fEnemy;
	float m_fShift;
	float m_fAt;
	float m_fLogAt;

	void DCO_ForceEval()
	{
		m_fPosture = 1;
		m_sPosture = "open";
		m_iBand = DCO_EForceBand.UNKNOWN;
		m_iBaseBand = DCO_EForceBand.UNKNOWN;
		m_fLogAt = -1000;
	}

	string DebugText()
	{
		string text = "Raw " + m_fRaw.ToString(-1, 1) + " : 1  ·  Posture x" + m_fPosture.ToString(-1, 1) + " (" + m_sPosture + ")  ·  Eff " + m_fEff.ToString(-1, 1) + " : 1  ·  " + DCO_Doctrine.BandName(m_iBand) + "  ·  Conf " + Math.Round(m_fConf * 100).ToString() + "%";
		string sign = "";
		if (m_fShift > 0)
			sign = "+";
		text += "  ·  Shift " + sign + m_fShift.ToString(-1, 2);
		return text;
	}
}

class DCO_DoctrineRuntime
{
	protected ref map<CMD_AICommanderObjectiveComponent, ref DCO_ForceEval> m_mEval = new map<CMD_AICommanderObjectiveComponent, ref DCO_ForceEval>();
	protected ref map<CMD_AICommanderObjectiveComponent, float> m_mEnemyHeldSince = new map<CMD_AICommanderObjectiveComponent, float>();
	protected float m_fTimer;
	protected ref DCO_BattlePlanner m_Planner = new DCO_BattlePlanner();
	protected ref DCO_DoctrineSupport m_Support = new DCO_DoctrineSupport();
	protected ref DCO_TerrainObjectives m_Terrain = new DCO_TerrainObjectives();
	protected bool m_bSeeded;

	DCO_BattlePlanner GetPlanner()
	{
		return m_Planner;
	}

	DCO_DoctrineSupport GetSupport()
	{
		return m_Support;
	}

	DCO_TerrainObjectives GetTerrain()
	{
		return m_Terrain;
	}

	protected void SeedPreBattle(AICommander_BaseComponent cmd, AICommander_ManagerComponent mgr, float now)
	{
		int level = cmd.GetPreBattleIntel();
		string levelName = typename.EnumToString(DCO_EPreBattleIntel, level);
		if (level == DCO_EPreBattleIntel.NONE)
		{
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(cmd), "prebattle_intel cmd=" + cmd.GetCommanderUID() + " level=" + levelName + " seeded=0");
			return;
		}
		float conf = 0.55;
		float err = 0.3;
		if (level == DCO_EPreBattleIntel.ACCURATE)
		{
			conf = 0.8;
			err = 0.1;
		}
		else if (level == DCO_EPreBattleIntel.WEAK)
		{
			conf = 0.35;
			err = 0.6;
		}
		FactionKey fk = cmd.GetCommanderFactionKey();
		int seeded;
		foreach (CMD_AICommanderObjectiveComponent obj : mgr.m_aObjective)
		{
			if (!obj || obj.IsCapturedBy(fk))
				continue;
			int friendly;
			int enemy;
			obj.CountNearbyUnitsCached(obj.GetRadius() + 150, fk, friendly, enemy);
			if (enemy <= 0)
				continue;
			DCO_ObjectiveIntel intel = cmd.GetOps().GetIntel(obj);
			if (intel.m_bEverSeen)
				continue;
			intel.m_fDefenders = Math.Max(1, Math.Round(enemy * (1 + Math.RandomFloat(-err, err))));
			intel.m_fSeed = intel.m_fDefenders;
			intel.m_fConfidence = conf;
			intel.m_fUpdated = now;
			intel.m_bEverSeen = true;
			seeded++;
		}
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(cmd), "prebattle_intel cmd=" + cmd.GetCommanderUID() + " level=" + levelName + " seeded=" + seeded.ToString() + " conf=" + conf.ToString(-1, 2));
	}

	DCO_ForceEval GetEval(CMD_AICommanderObjectiveComponent obj)
	{
		return m_mEval.Get(obj);
	}

	int GetBand(CMD_AICommanderObjectiveComponent obj)
	{
		DCO_ForceEval e = m_mEval.Get(obj);
		if (!e)
			return DCO_EForceBand.UNKNOWN;
		return e.m_iBand;
	}

	void Tick(AICommander_BaseComponent cmd, float timeSlice)
	{
		m_fTimer += timeSlice;
		if (m_fTimer < 15.0)
			return;
		m_fTimer = 0;
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!cmd || !mgr)
			return;
		float now = DCO_Util.NowS();
		m_Support.Tick(cmd, now);
		m_Terrain.Update(cmd, m_Planner, now);
		float idle;
		float reserveFloor = cmd.GetReserveFloor();
		foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
		{
			if (!Countable(g))
				continue;
			DCO_EGroupTask task = g.GetTask();
			if (task == DCO_EGroupTask.NONE || task == DCO_EGroupTask.PATROL)
				idle += GroupStrength(g, false);
		}
		float committable = Math.Max(0, idle - reserveFloor);
		if (!m_bSeeded)
		{
			m_bSeeded = true;
			SeedPreBattle(cmd, mgr, now);
		}
		FactionKey fk = cmd.GetCommanderFactionKey();
		foreach (CMD_AICommanderObjectiveComponent obj : mgr.m_aObjective)
		{
			if (!obj)
				continue;
			if (obj.IsCapturedBy(fk))
			{
				m_mEnemyHeldSince.Remove(obj);
				continue;
			}
			Evaluate(cmd, obj, committable, now);
		}
	}

	protected static bool Countable(DCO_GroupUtilityComponent g)
	{
		return g && !g.IsPlayerGroup() && !g.IsDedicatedTransport() && !g.IsMortar() && g.GetUnitCount() > 0;
	}

	protected static float GroupStrength(DCO_GroupUtilityComponent g, bool enemyArmor)
	{
		float s = g.GetUnitCount();
		if (g.HasMG())
			s += 0.5;
		if (g.HasAT())
			s += 0.5;
		if (g.IsArmor())
			s += 6;
		if (g.GetGroupMorale() > 1.5)
			s *= 0.7;
		if (enemyArmor && !g.HasAT() && !g.IsArmor())
			s *= 0.1;
		return s;
	}

	protected void Evaluate(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, float committable, float now)
	{
		DCO_ObjectiveIntel intel = cmd.GetOps().GetIntel(obj);
		DCO_ForceEval e = m_mEval.Get(obj);
		if (!e)
		{
			e = new DCO_ForceEval();
			m_mEval.Set(obj, e);
		}
		int oldBand = e.m_iBand;
		vector p = obj.GetOwner().GetOrigin();
		float r = obj.GetRadius() + 150;

		float onObj;
		foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
		{
			if (Countable(g) && g.GetGroupObjective() == obj)
				onObj += GroupStrength(g, intel.m_bArmor);
		}
		float scale = 1;
		if (intel.m_bArmor)
			scale = 0.6;
		e.m_fOwn = onObj + committable * scale;

		float enemy = intel.m_fDefenders;
		bool moving = false;
		CMD_ThreatResponseComponent tr = cmd.GetThreatResponseComponent();
		if (tr)
		{
			foreach (CMD_ThreatEntry t : tr.GetThreats())
			{
				if (!t || now - t.m_fLastUpdateTime > 300 || vector.DistanceXZ(t.m_vBelievedPos, p) > r)
					continue;
				enemy += 0.5 * (t.m_iMG + t.m_iAT) + 4 * t.m_iVehicle + 8 * t.m_iArmor;
				if (t.m_vVelocity.Length() > 1.0)
					moving = true;
			}
		}
		if (intel.m_bArmor && enemy < intel.m_fDefenders + 8)
			enemy += 8;
		e.m_fEnemy = enemy;
		e.m_fConf = intel.m_fConfidence;

		FactionKey owner = obj.GetOwnerFaction();
		bool enemyHeld = !owner.IsEmpty() && owner != cmd.GetCommanderFactionKey();
		float heldSince;
		if (enemyHeld)
		{
			if (!m_mEnemyHeldSince.Find(obj, heldSince))
			{
				heldSince = now;
				m_mEnemyHeldSince.Set(obj, now);
			}
		}
		else
		{
			m_mEnemyHeldSince.Remove(obj);
		}
		bool urban = false;
		DCO_GarrisonRegistry reg = DCO_GarrisonRegistry.GetInstance();
		if (reg && enemyHeld)
		{
			array<IEntity> buildings = {};
			reg.FindBuildings(p, obj.GetRadius(), buildings);
			urban = buildings.Count() >= 4;
		}
		e.m_fPosture = 1.3;
		e.m_sPosture = "hasty";
		if (moving)
		{
			e.m_fPosture = 1.0;
			e.m_sPosture = "moving";
		}
		else if (enemyHeld && urban)
		{
			e.m_fPosture = 2.0;
			e.m_sPosture = "buildings";
		}
		else if (enemyHeld && now - heldSince >= 300)
		{
			e.m_fPosture = 1.5;
			e.m_sPosture = "dug_in";
		}

		e.m_fRaw = e.m_fOwn / Math.Max(enemy, 1);
		e.m_fEff = e.m_fRaw / e.m_fPosture;
		e.m_fShift = cmd.GetDoctrineShift();
		e.m_fAt = now;

		bool stale = intel.m_fUpdated < 0 || now - intel.m_fUpdated > 300;
		if (!intel.m_bEverSeen || intel.m_fConfidence < 0.4 || stale)
		{
			e.m_iBaseBand = DCO_EForceBand.UNKNOWN;
			e.m_iBand = DCO_EForceBand.UNKNOWN;
		}
		else
		{
			int baseBand = BandOf(e.m_fEff);
			if (oldBand != DCO_EForceBand.UNKNOWN)
				baseBand = Hysteresis(oldBand, baseBand, e.m_fEff);
			int shifted = BandOf(e.m_fEff * (1 + e.m_fShift));
			e.m_iBaseBand = baseBand;
			e.m_iBand = Math.ClampInt(shifted, baseBand - 1, baseBand + 1);
		}

		if (e.m_iBand != oldBand)
			DCO_Perf.Count("doctrine:band:" + DCO_Doctrine.BandName(e.m_iBand));
		if (e.m_iBand != oldBand || now - e.m_fLogAt >= 60)
		{
			e.m_fLogAt = now;
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(cmd), string.Format("force_condition cmd=%1 obj=%2 raw=%3 posture=%4 eff=%5 band=%6 conf=%7 own=%8 enemy=%9", cmd.GetCommanderUID(), obj.GetLogName(), e.m_fRaw.ToString(-1, 2), e.m_fPosture.ToString(-1, 1), e.m_fEff.ToString(-1, 2), DCO_Doctrine.BandName(e.m_iBand), e.m_fConf.ToString(-1, 2), e.m_fOwn.ToString(-1, 1), e.m_fEnemy.ToString(-1, 1)) + " shift=" + e.m_fShift.ToString(-1, 2) + " posture_kind=" + e.m_sPosture);
		}
	}

	static int BandOf(float ratio)
	{
		if (ratio >= 2.0)
			return DCO_EForceBand.STRONG;
		if (ratio >= 1.5)
			return DCO_EForceBand.FAVORABLE;
		if (ratio >= 0.67)
			return DCO_EForceBand.EQUAL;
		return DCO_EForceBand.WEAKER;
	}

	static float LowerEdge(int band)
	{
		if (band == DCO_EForceBand.STRONG)
			return 2.0;
		if (band == DCO_EForceBand.FAVORABLE)
			return 1.5;
		if (band == DCO_EForceBand.EQUAL)
			return 0.67;
		return 0;
	}

	static int Hysteresis(int oldBand, int newBand, float ratio)
	{
		if (newBand == oldBand)
			return oldBand;
		if (newBand < oldBand)
		{
			if (ratio >= LowerEdge(newBand) * 1.12)
				return newBand;
			return oldBand;
		}
		if (ratio < LowerEdge(newBand - 1) * 0.88)
			return newBand;
		return oldBand;
	}

	string DebugLines(AICommander_BaseComponent cmd)
	{
		string text;
		foreach (CMD_AICommanderObjectiveComponent obj, DCO_ForceEval e : m_mEval)
		{
			if (!obj || !e)
				continue;
			if (!text.IsEmpty())
				text += "\n";
			text += obj.GetLogName() + ": " + e.DebugText();
		}
		return text;
	}
}
