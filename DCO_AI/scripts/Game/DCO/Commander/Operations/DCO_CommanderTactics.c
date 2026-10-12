enum DCO_ETactic
{
	MOVEMENT_TO_CONTACT,
	RECON_IN_FORCE,
	FIRE_AND_MANEUVER,
	FLANKING,
	HAMMER_AND_ANVIL,
	PINCER,
	INFILTRATION,
	SIEGE,
	BYPASS,
	RAID,
	ATTACK_BY_FIRE,
	PURSUIT,
	DELAY
}

enum DCO_ETacticFlag
{
	MOVEMENT_TO_CONTACT	= 1,
	RECON_IN_FORCE		= 2,
	FIRE_AND_MANEUVER	= 4,
	FLANKING			= 8,
	HAMMER_AND_ANVIL	= 16,
	PINCER				= 32,
	INFILTRATION		= 64,
	SIEGE				= 128,
	BYPASS				= 256,
	RAID				= 512,
	ATTACK_BY_FIRE		= 1024,
	PURSUIT				= 2048,
	DELAY				= 4096
}

class DCO_TacticPlan
{
	CMD_AICommanderObjectiveComponent m_Obj;
	DCO_ETactic m_eTactic;
	int m_iPhase;
	float m_fStart;
	float m_fPhaseStart;
	float m_fLastEval;
	float m_fLastFire;
	float m_fRatio;
	bool m_bHasRetreat;
	vector m_vRetreat;
	vector m_vBlock;
	vector m_vTarget;
	int m_iFlankSide = 1;
	float m_fIntelConf0;
	float m_fArrivedAt = -1;
	string m_sLearnCtx;
	string m_sLearnCtxTR;
	string m_sLearnVs;
	string m_sLearnFk;
	bool m_bReleaseLogged;
	ref map<DCO_GroupUtilityComponent, float> m_mBestDist = new map<DCO_GroupUtilityComponent, float>();
	ref map<DCO_GroupUtilityComponent, float> m_mProgressAt = new map<DCO_GroupUtilityComponent, float>();
	ref set<DCO_GroupUtilityComponent> m_sConsolidated = new set<DCO_GroupUtilityComponent>();
	float m_fConsolCheckAt;
	ref array<DCO_GroupUtilityComponent> m_aGroups = {};
	ref array<int> m_aRoles = {};
	ref array<int> m_aStartUnits = {};
	ref array<vector> m_aGoals = {};
	ref map<int, int> m_mPlayerRoles = new map<int, int>();
	ref map<int, vector> m_mPlayerGoals = new map<int, vector>();

	int PlayerWithRole(int role)
	{
		foreach (int id, int r : m_mPlayerRoles)
		{
			if (r == role)
				return id;
		}
		return -1;
	}

	DCO_GroupUtilityComponent m_VehFix;
	IEntity m_VehFixVeh;
	vector m_vVehFixPos;
	float m_fVehFixMoveAt;
	float m_fVehFixOutAt = -1;
	float m_fVehFixAt;
	int m_iVehFixReplans;
	bool m_bVehFixOff;
	DCO_GroupUtilityComponent m_VehFixBad;
	float m_fVehFixBadUntil;

	void RemoveAt(int i)
	{
		DCO_GroupUtilityComponent rg = m_aGroups[i];
		if (rg)
		{
			m_mBestDist.Remove(rg);
			m_mProgressAt.Remove(rg);
			m_sConsolidated.RemoveItem(rg);
		}
		m_aGroups.Remove(i);
		m_aRoles.Remove(i);
		m_aStartUnits.Remove(i);
		m_aGoals.Remove(i);
	}

	void AddGroup(DCO_GroupUtilityComponent g, int role, vector goal)
	{
		m_aGroups.Insert(g);
		m_aRoles.Insert(role);
		m_aStartUnits.Insert(g.GetUnitCount());
		m_aGoals.Insert(goal);
	}

	DCO_GroupUtilityComponent FirstWithRole(int role)
	{
		foreach (int i, DCO_GroupUtilityComponent g : m_aGroups)
		{
			if (g && m_aRoles[i] == role)
				return g;
		}
		return null;
	}
}

class DCO_TacticSituation
{
	int m_iGroups;
	int m_iSmallGroups;
	float m_fRatio;
	float m_fConfidence;
	bool m_bEverSeen;
	bool m_bNight;
	bool m_bUrban;
	bool m_bForest;
	bool m_bRetreat;
	vector m_vRetreat;
	string m_sRetreatSrc = "none";
	bool m_bMortar;
	bool m_bFireSupport;
	bool m_bOtherObjective;
	float m_fMinRatio;
	bool m_bEnemyMG;
	bool m_bEnemyInbound;
	int m_iIntel;
	bool m_bDugIn;
	bool m_bEnemyArmor;
	int m_iSides;
	int m_iTime;
	int m_iBand;
}

class DCO_CommanderTactics
{
	static const int ROLE_FIX = 0;
	static const int ROLE_BLOCK = 2;
	static const int ROLE_ASSAULT = 3;

	protected static float TICK_S() { return 5; }

	protected ref map<CMD_AICommanderObjectiveComponent, ref DCO_TacticPlan> m_mPlans = new map<CMD_AICommanderObjectiveComponent, ref DCO_TacticPlan>();
	protected ref map<CMD_AICommanderObjectiveComponent, int> m_mFailedMask = new map<CMD_AICommanderObjectiveComponent, int>();
	protected ref map<CMD_AICommanderObjectiveComponent, float> m_mFailedAt = new map<CMD_AICommanderObjectiveComponent, float>();
	protected ref array<ref DCO_TacticPlan> m_aPursuits = {};
	protected float m_fTimer;

	static int Bit(int t)
	{
		return 1 << t;
	}

	static string Name(int t)
	{
		return typename.EnumToString(DCO_ETactic, t);
	}

	static void Event(string line)
	{
		Print("[DCO_Tactics] " + line);
		DCO_BenchmarkLoggerComponent.Event(line);
	}

	static bool IsAllowed(AICommander_BaseComponent cmd, int t)
	{
		return (cmd.GetAllowedTactics() & Bit(t)) != 0;
	}

	DCO_TacticPlan GetPlan(CMD_AICommanderObjectiveComponent obj)
	{
		return m_mPlans.Get(obj);
	}

	bool ControlsObjective(CMD_AICommanderObjectiveComponent obj)
	{
		DCO_TacticPlan p = m_mPlans.Get(obj);
		if (!p)
			return false;

		switch (p.m_eTactic)
		{
			case DCO_ETactic.MOVEMENT_TO_CONTACT:
			case DCO_ETactic.RECON_IN_FORCE:
			case DCO_ETactic.SIEGE:
			case DCO_ETactic.BYPASS:
			case DCO_ETactic.RAID:
			case DCO_ETactic.ATTACK_BY_FIRE:
				return true;
		}
		return false;
	}

	bool AllowsFlankSwing(CMD_AICommanderObjectiveComponent obj)
	{
		DCO_TacticPlan p = m_mPlans.Get(obj);
		return !p || p.m_eTactic != DCO_ETactic.FIRE_AND_MANEUVER;
	}

	int NextFlankSide(CMD_AICommanderObjectiveComponent obj)
	{
		DCO_TacticPlan p = m_mPlans.Get(obj);
		if (!p || p.m_eTactic != DCO_ETactic.PINCER)
			return 0;
		p.m_iFlankSide = -p.m_iFlankSide;
		return p.m_iFlankSide;
	}

	bool HoldRelease(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, float now)
	{
		DCO_TacticPlan p = m_mPlans.Get(obj);
		if (!p || p.m_eTactic != DCO_ETactic.HAMMER_AND_ANVIL)
			return false;

		DCO_GroupUtilityComponent anvil = p.FirstWithRole(ROLE_BLOCK);
		bool hasAnvil = anvil != null;
		bool ready = anvil && vector.DistanceXZ(anvil.GetOwner().GetOrigin(), p.m_vBlock) <= 60.0;
		int playerAnvil = p.PlayerWithRole(ROLE_BLOCK);
		if (!anvil && playerAnvil >= 0)
		{
			DCO_PlayerTasking tasking = cmd.GetPlayerTasking();
			hasAnvil = tasking.IsRoleWaited(playerAnvil);
			ready = tasking.IsRoleArrived(playerAnvil);
		}

		if (ready)
		{
			if (p.m_iPhase == 0)
			{
				p.m_iPhase = 1;
				Event(string.Format("tactic_phase obj=%1 tactic=%2 phase=anvil_ready", obj.GetOwner().GetName(), Name(p.m_eTactic)));
			}
			return false;
		}

		if (now - p.m_fStart > 240.0 || !hasAnvil)
		{
			p.m_eTactic = DCO_ETactic.FLANKING;
			string why = "anvil_timeout";
			if (!hasAnvil)
				why = "no_anvil";
			Event(string.Format("tactic_phase obj=%1 tactic=HAMMER_AND_ANVIL phase=convert_flanking reason=%2", obj.GetOwner().GetName(), why));
			return false;
		}
		return true;
	}

	DCO_TacticPlan Begin(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, float now, int mask = -1)
	{
		m_mPlans.Remove(obj);

		DCO_TacticSituation s = Assess(cmd, obj, now);
		if (DCO_TacticTest.ForceSelect(cmd, obj))
			mask = Bit(DCO_TacticTest.Tactic());
		int usable = mask & cmd.GetAllowedTactics();
		if (usable == 0)
			usable = cmd.GetAllowedTactics();

		bool mett = DCO_MettTable.On() && !DCO_TacticTest.ForceSelect(cmd, obj);
		if (mett)
			DCO_MettTable.Read(cmd, obj, s);
		array<float> scores = {};
		for (int t = 0; t <= DCO_ETactic.DELAY; t++)
		{
			if ((usable & Bit(t)) == 0)
				scores.Insert(0);
			else if (mett)
				scores.Insert(DCO_MettTable.Score(cmd, obj, t, s, RecentlyFailed(obj, t)));
			else
				scores.Insert(Score(cmd, obj, t, s));
		}

		string lCtx;
		string lCtxTR;
		string lVs;
		int learnPick = -1;
		if (!DCO_TacticTest.ForceSelect(cmd, obj))
			learnPick = DCO_CmdLearn.ApplyTactics(cmd, obj, s, scores, lCtx, lCtxTR, lVs);

		int best = -1;
		string pick = "score";
		if (learnPick >= 0 && (usable & Bit(learnPick)) != 0)
		{
			best = learnPick;
			pick = "learn_explore";
		}
		else if (mett)
		{
			best = DCO_MettTable.Pick(obj, scores);
			pick = "mett";
			if (best >= 0)
				DCO_MettTable.NotePick(cmd, best);
		}
		else
		{
			float bestScore = 0;
			for (int i = 0; i < scores.Count(); i++)
			{
				if (scores[i] > bestScore)
				{
					bestScore = scores[i];
					best = i;
				}
			}
		}

		if (best < 0)
		{
			pick = "fallback";
			if ((usable & Bit(DCO_ETactic.FIRE_AND_MANEUVER)) != 0)
				best = DCO_ETactic.FIRE_AND_MANEUVER;
			else
			{
				for (int f = 0; f < DCO_ETactic.PURSUIT; f++)
				{
					if ((usable & Bit(f)) != 0)
					{
						best = f;
						break;
					}
				}
			}
			if (best < 0)
			{
				Event(string.Format("tactic_none cmd=%1 obj=%2 mask=%3 allowed=%4", cmd.GetCommanderUID(), obj.GetOwner().GetName(), mask, cmd.GetAllowedTactics()));
				return null;
			}
		}

		if (!s.m_bRetreat)
		{
			vector far = obj.GetOwner().GetOrigin() - cmd.GetOwner().GetOrigin();
			far[1] = 0;
			if (far.LengthSq() > 1)
			{
				far.Normalize();
				s.m_vRetreat = far;
				s.m_sRetreatSrc = "far_side";
			}
		}

		DCO_TacticPlan p = new DCO_TacticPlan();
		p.m_Obj = obj;
		p.m_eTactic = best;
		p.m_fStart = now;
		p.m_fPhaseStart = now;
		p.m_fLastEval = now;
		p.m_fRatio = s.m_fRatio;
		p.m_bHasRetreat = s.m_bRetreat;
		p.m_vRetreat = s.m_vRetreat;
		p.m_fIntelConf0 = s.m_fConfidence;
		p.m_sLearnCtx = lCtx;
		p.m_sLearnCtxTR = lCtxTR;
		p.m_sLearnVs = lVs;
		p.m_sLearnFk = cmd.GetCommanderFactionKey();
		m_mPlans.Set(obj, p);

		Event(string.Format("tactic_chosen cmd=%1 obj=%2 tactic=%3 ratio=%4 groups=%5 conf=%6 night=%7 urban=%8 forest=%9",
			cmd.GetCommanderUID(), obj.GetOwner().GetName(), Name(best), s.m_fRatio.ToString(-1, 2), s.m_iGroups,
			s.m_fConfidence.ToString(-1, 2), s.m_bNight, s.m_bUrban, s.m_bForest) + string.Format(" retreat=%1 dir=%2 mask=%3 pick=%4", s.m_sRetreatSrc, s.m_vRetreat, usable, pick));
		Event(string.Format("tactic_scores obj=%1 top=%2", obj.GetOwner().GetName(), TopScores(scores)));

		Start(cmd, p, now);
		return p;
	}

	protected void LearnEnd(DCO_TacticPlan p, string result, float now)
	{
		if (!p || p.m_sLearnCtx.IsEmpty() || !p.m_Obj || !p.m_Obj.GetOwner())
			return;
		int start;
		int alive;
		foreach (int i, DCO_GroupUtilityComponent g : p.m_aGroups)
		{
			if (i < p.m_aStartUnits.Count())
				start += p.m_aStartUnits[i];
			if (g)
				alive += g.GetUnitCount();
		}
		int losses = Math.Max(start - alive, 0);
		int kills = DCO_CmdLearn.DeathsNear(p.m_Obj.GetOwner().GetOrigin(), p.m_Obj.GetRadius() + 300.0, p.m_sLearnFk, p.m_fStart);
		DCO_CmdLearn.TacticEpisode(p.m_Obj, p.m_eTactic, p.m_sLearnCtx, p.m_sLearnCtxTR, p.m_sLearnVs, result, now - p.m_fStart, losses, kills, false);
		p.m_sLearnCtx = "";
	}

	protected string TopScores(array<float> scores)
	{
		array<int> order = {};
		for (int i = 0; i < scores.Count(); i++)
		{
			int at = 0;
			while (at < order.Count() && scores[order[at]] >= scores[i])
				at++;
			order.InsertAt(i, at);
		}

		string s;
		for (int j = 0; j < 3 && j < order.Count(); j++)
			s += string.Format(" %1=%2", Name(order[j]), scores[order[j]].ToString(-1, 1));
		return s;
	}

	protected DCO_TacticSituation Assess(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, float now)
	{
		DCO_TacticSituation s = new DCO_TacticSituation();
		vector objPos = obj.GetOwner().GetOrigin();

		DCO_StrengthInfo info = new DCO_StrengthInfo();
		float friendly = 0;
		foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
		{
			if (!IsUsable(cmd, g))
				continue;
			s.m_iGroups++;
			if (g.GetUnitCount() <= 6.0)
				s.m_iSmallGroups++;
			if (g.HasMG() || g.IsArmor())
				s.m_bFireSupport = true;
			DCO_Strength.OfGroup(SCR_AIGroup.Cast(g.GetOwner()), false, info);
			friendly += info.m_fStrength;
		}

		DCO_CommanderOps ops = cmd.GetOps();
		DCO_ObjectiveIntel intel = ops.GetIntel(obj);
		float enemy = ops.EstimateDefenders(obj);
		if (intel.m_bArmor)
			enemy += 8.0;
		s.m_fRatio = friendly / Math.Max(enemy, 1);
		s.m_fConfidence = intel.m_fConfidence;
		s.m_bEverSeen = intel.m_bEverSeen;
		s.m_bNight = CMD_ThreatResponseComponent.IsNight();
		s.m_fMinRatio = cmd.GetMinAttackRatio();

		DCO_GarrisonRegistry reg = DCO_GarrisonRegistry.GetInstance();
		if (reg)
		{
			array<IEntity> buildings = {};
			reg.FindBuildings(objPos, obj.GetRadius(), buildings);
			s.m_bUrban = buildings.Count() >= 4;
		}

		array<DCO_TerrainPoint> pts = {};
		s.m_bForest = DCO_TerrainCache.Query(DCO_ETerrainFlag.FOREST_EDGE, objPos, obj.GetRadius() + 200, pts) >= 3;

		CMD_ThreatResponseComponent threat = cmd.GetThreatResponseComponent();
		if (threat)
		{
			float nearSq = (obj.GetRadius() + 300.0) * (obj.GetRadius() + 300.0);
			foreach (CMD_ThreatEntry te : threat.GetThreats())
			{
				if (!te || vector.DistanceSqXZ(te.m_vPosition, objPos) > nearSq)
					continue;
				if (te.m_iMG > 0)
					s.m_bEnemyMG = true;
				if (te.m_bMoving && vector.DistanceXZ(te.m_vPosition + te.m_vVelocity * 30, objPos) < vector.DistanceXZ(te.m_vPosition, objPos))
					s.m_bEnemyInbound = true;
			}
		}

		CMD_ArtillerySupport arty = cmd.GetArtySupport();
		s.m_bMortar = arty && arty.HasRegisteredUnits();
		if (s.m_bMortar)
			s.m_bFireSupport = true;

		vector retreat;
		string retreatSrc;
		s.m_bRetreat = FindRetreat(cmd, obj, retreat, retreatSrc);
		s.m_sRetreatSrc = retreatSrc;
		s.m_vRetreat = retreat;

		FactionKey fk = cmd.GetCommanderFactionKey();
		foreach (CMD_AICommanderObjectiveComponent o : cmd.GetObjectiveList())
		{
			if (o && o != obj && !o.IsCapturedBy(fk) && !cmd.IsObjectiveOnCooldown(o))
			{
				s.m_bOtherObjective = true;
				break;
			}
		}
		return s;
	}

	protected bool FindRetreat(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, out vector dir, out string src)
	{
		FactionKey fk = cmd.GetCommanderFactionKey();
		vector objPos = obj.GetOwner().GetOrigin();
		float minDist = obj.GetRadius() + 50;
		float best = float.MAX;
		bool found = false;
		src = "none";
		foreach (CMD_AICommanderObjectiveComponent o : cmd.GetObjectiveList())
		{
			if (!o || o == obj || !o.GetOwner())
				continue;
			FactionKey owner = o.GetOwnerFaction();
			if (owner.IsEmpty() || owner == fk)
				continue;

			float d = vector.DistanceXZ(o.GetOwner().GetOrigin(), objPos);
			if (d < best)
			{
				best = d;
				dir = o.GetOwner().GetOrigin() - objPos;
				found = true;
				src = "objective";
			}
		}

		if (!found)
		{
			AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
			if (mgr)
			{
				foreach (AICommander_BaseComponent enemy : mgr.m_aCommander)
				{
					if (!enemy || enemy == cmd || enemy.GetCommanderFactionKey().IsEmpty() || enemy.GetCommanderFactionKey() == fk)
						continue;
					string hubSrc;
					vector hub = enemy.GetHubFor(objPos, hubSrc);
					if (hub == vector.Zero)
						hub = enemy.GetOwner().GetOrigin();
					float d2 = vector.DistanceXZ(hub, objPos);
					if (d2 > minDist && d2 < best)
					{
						best = d2;
						dir = hub - objPos;
						found = true;
						src = "enemy_hub";
					}
				}
			}
		}

		if (!found)
			found = FindRoadOut(cmd, obj, dir, src);

		if (!found)
			return false;
		dir[1] = 0;
		dir.Normalize();
		return true;
	}

	protected bool FindRoadOut(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, out vector dir, out string src)
	{
		SCR_AIWorld aiWorld = SCR_AIWorld.Cast(GetGame().GetAIWorld());
		if (!aiWorld || !aiWorld.GetRoadNetworkManager())
			return false;
		RoadNetworkManager roads = aiWorld.GetRoadNetworkManager();

		vector objPos = obj.GetOwner().GetOrigin();
		vector away = objPos - cmd.GetOwner().GetOrigin();
		float awayYaw = Math.Atan2(away[0], away[2]);
		float ring = obj.GetRadius() + 150;
		for (int i = 0; i < 7; i++)
		{
			float offset = ((i + 1) / 2) * 30;
			if (i % 2 == 0)
				offset = -offset;
			float yaw = awayYaw + offset * Math.DEG2RAD;
			vector p = objPos + Vector(Math.Sin(yaw), 0, Math.Cos(yaw)) * ring;
			BaseRoad road;
			float dist;
			roads.GetClosestRoad(p, road, dist);
			if (road && dist <= 50.0)
			{
				dir = p - objPos;
				src = "road";
				return true;
			}
		}
		return false;
	}

	protected float Score(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, int t, DCO_TacticSituation s)
	{
		if (!IsAllowed(cmd, t) || t == DCO_ETactic.PURSUIT || t == DCO_ETactic.DELAY)
			return 0;

		float aggression = cmd.GetAggression();
		if (cmd.DCO_AggProfile())
			aggression = Math.Clamp(aggression * cmd.DCO_AggrA(), 0, 1.5);
		float strategy = cmd.GetAdaptability();
		float intelTrait = 1 - cmd.GetRiskTaking();
		float support = cmd.GetPatience();
		bool weak = s.m_fRatio < s.m_fMinRatio;
		bool lowIntel = !s.m_bEverSeen || s.m_fConfidence < 0.2;

		float v = 0;
		switch (t)
		{
			case DCO_ETactic.MOVEMENT_TO_CONTACT:
				if (!lowIntel || s.m_iGroups < 1)
					return 0;
				v = 3 + 1.5 * aggression;
				break;
			case DCO_ETactic.RECON_IN_FORCE:
				if (s.m_fConfidence >= 0.4 || s.m_iGroups < 2)
					return 0;
				v = 2 + 2 * intelTrait;
				break;
			case DCO_ETactic.FIRE_AND_MANEUVER:
				if (s.m_iGroups < 2)
					return 0;
				v = 2 + 1.5 * aggression;
				if (!s.m_bUrban && !s.m_bForest)
					v += 1.5;
				break;
			case DCO_ETactic.FLANKING:
				if (s.m_iGroups < 2)
					return 0;
				v = 2.5 + 1.5 * strategy;
				if (s.m_bForest)
					v += 0.5;
				break;
			case DCO_ETactic.HAMMER_AND_ANVIL:
				if (s.m_iGroups < 3 || s.m_fRatio < 1.5 || !s.m_bRetreat)
					return 0;
				v = 2 + aggression + strategy;
				if (s.m_fRatio >= 2)
					v += 1;
				break;
			case DCO_ETactic.PINCER:
				if (s.m_iGroups < 3 || s.m_fRatio < 2)
					return 0;
				v = 2 + 1.5 * strategy;
				if (s.m_fRatio >= 3)
					v += 1;
				break;
			case DCO_ETactic.INFILTRATION:
				if (s.m_iSmallGroups < 1 || s.m_iGroups < 2)
					return 0;
				v = 1 + strategy + 0.5 * intelTrait;
				if (s.m_bNight)
					v += 2;
				if (s.m_bUrban || s.m_bForest)
					v += 1.5;
				break;
			case DCO_ETactic.SIEGE:
				if (!weak || (s.m_iGroups < 3 && !s.m_bMortar))
					return 0;
				v = 2 + 2 * support;
				if (s.m_bMortar)
					v += 1;
				break;
			case DCO_ETactic.BYPASS:
				if (s.m_fRatio >= s.m_fMinRatio * 0.7 || !s.m_bOtherObjective || s.m_iGroups < 1)
					return 0;
				v = 1.5 + strategy;
				break;
			case DCO_ETactic.RAID:
				if (!weak || s.m_iGroups < 1)
					return 0;
				v = 1 + aggression + cmd.GetRiskTaking();
				break;
			case DCO_ETactic.ATTACK_BY_FIRE:
				if (!weak || !s.m_bFireSupport)
					return 0;
				v = 2 + support;
				if (s.m_bMortar)
					v += 1;
				break;
		}

		if (weak && (t == DCO_ETactic.FIRE_AND_MANEUVER || t == DCO_ETactic.FLANKING || t == DCO_ETactic.INFILTRATION))
			v -= 2;
		if (!weak && !lowIntel && s.m_fRatio >= s.m_fMinRatio * 1.5 && t == DCO_ETactic.MOVEMENT_TO_CONTACT)
			v -= 2;
		if (s.m_bEnemyInbound && (t == DCO_ETactic.HAMMER_AND_ANVIL || t == DCO_ETactic.SIEGE))
			v += 1;
		if (s.m_bEnemyMG && (t == DCO_ETactic.FLANKING || t == DCO_ETactic.ATTACK_BY_FIRE))
			v += 0.5;
		if (RecentlyFailed(obj, t))
			v -= 3;

		v += Math.RandomFloat(0, 1);
		return Math.Max(v, 0.01);
	}

	protected bool RecentlyFailed(CMD_AICommanderObjectiveComponent obj, int t)
	{
		float at;
		if (!m_mFailedAt.Find(obj, at) || DCO_CommanderOps.Now() - at > 900.0)
			return false;
		return (m_mFailedMask.Get(obj) & Bit(t)) != 0;
	}

	protected void MarkFailed(CMD_AICommanderObjectiveComponent obj, int t)
	{
		m_mFailedMask.Set(obj, m_mFailedMask.Get(obj) | Bit(t));
		m_mFailedAt.Set(obj, DCO_CommanderOps.Now());
	}

	protected bool IsUsable(AICommander_BaseComponent cmd, DCO_GroupUtilityComponent g)
	{
		return g && !g.IsPlayerGroup() && !g.IsDedicatedTransport() && !g.IsMortar() && !g.IsInTransport()
			&& g.IsAvailableReserve() && g.CanItHaveOrder() && cmd.CanCommitGroup(g);
	}

	protected int RoleJob(int role)
	{
		if (role == ROLE_FIX)
			return 4;
		if (role == ROLE_BLOCK)
			return 16;
		if (role == 1)
			return 9;
		return 1;
	}

	protected DCO_GroupUtilityComponent PickGroup(AICommander_BaseComponent cmd, vector pos, bool small, bool fireSupport, int job = 0)
	{
		int jm = job;
		if (jm == 0)
			jm = DCO_GroupJobs.AnyCombat();
		DCO_GroupUtilityComponent best;
		float bestD = float.MAX;
		foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
		{
			if (!IsUsable(cmd, g))
				continue;
			if (!DCO_GroupJobs.Gate(cmd, g, jm, "tac_pick"))
				continue;
			if (small && g.GetUnitCount() > 6.0)
				continue;
			if (fireSupport && !g.HasMG() && !g.IsArmor())
				continue;
			float d = vector.DistanceSqXZ(g.GetOwner().GetOrigin(), pos);
			if ((jm & 3) != 0 && g.IsFirstToFight() && cmd.DCO_FtfFit(g))
				d = d / 2.25;
			if (d < bestD)
			{
				bestD = d;
				best = g;
			}
		}
		return best;
	}

	static int RoleBit(DCO_ETactic t, int role)
	{
		switch (role)
		{
			case ROLE_FIX:
				return 2;
			case ROLE_BLOCK:
				return 4;
			case 1:
				if (t == DCO_ETactic.INFILTRATION)
					return 8;
				break;
		}
		return 1;
	}

	static bool IsDecisive(int role)
	{
		return role == ROLE_ASSAULT || role == 1 || role == ROLE_BLOCK || role == 5;
	}

	static string RoleName(int role)
	{
		switch (role)
		{
			case ROLE_FIX:		return "FIX";
			case 1:	return "MANEUVER";
			case ROLE_BLOCK:	return "BLOCK";
			case ROLE_ASSAULT:	return "ASSAULT";
			case 4:	return "RESERVE";
			case 5:		return "LEAD";
		}
		return "NONE";
	}

	static float RoleScore(int units, bool mg, bool vehicle, float dist, int role, bool small, bool fireSupport)
	{
		if (small && units > 6.0)
			return -1000;

		float v = 5 - dist / 250;
		if (fireSupport)
		{
			if (mg)
				v += 1.5;
			else
				v -= 2;
		}
		if (small && units <= 4)
			v += 1;
		if (vehicle && dist > 500)
			v += 1;
		if (role == ROLE_ASSAULT || role == ROLE_BLOCK)
			v += Math.Min(units, 8) * 0.15;
		return v;
	}

	protected bool TryPlayerRole(AICommander_BaseComponent cmd, DCO_TacticPlan p, int role, vector pos, bool small, bool fireSupport, float now)
	{
		if (!cmd.IsPlayerTaskingEnabled())
			return false;

		DCO_PlayerTasking tasking = cmd.GetPlayerTasking();
		float playerScore;
		int id = tasking.BestRoleCandidate(cmd, p.m_Obj, RoleBit(p.m_eTactic, role), pos, role, small, fireSupport, playerScore);
		if (id < 0)
			return false;

		DCO_GroupUtilityComponent ai = PickGroup(cmd, pos, small, fireSupport, RoleJob(role));
		if (ai)
		{
			float aiScore = RoleScore(ai.GetUnitCount(), ai.HasMG(), ai.GetGroupVehicle() != null, vector.DistanceXZ(ai.GetOwner().GetOrigin(), pos), role, small, fireSupport);
			if (aiScore >= playerScore)
				return false;
		}

		vector goal = pos;
		if (role == ROLE_FIX)
		{
			goal = cmd.GetOps().FindOverwatch(p.m_Obj, tasking.GroupLeaderPos(id));
			if (goal == vector.Zero)
				goal = cmd.TacticStagingPos(p.m_Obj);
		}

		if (!tasking.AssignRole(cmd, id, p.m_Obj, p.m_eTactic, role, goal, p.m_vRetreat))
			return false;

		p.m_mPlayerRoles.Set(id, role);
		p.m_mPlayerGoals.Set(id, goal);
		return true;
	}

	bool HasPlayerRole(CMD_AICommanderObjectiveComponent obj, int groupID, int role)
	{
		DCO_TacticPlan p = m_mPlans.Get(obj);
		int r;
		return p && p.m_mPlayerRoles.Find(groupID, r) && r == role;
	}

	void RefillRole(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, int groupID, float now)
	{
		DCO_TacticPlan p = m_mPlans.Get(obj);
		int role;
		if (!p || !p.m_mPlayerRoles.Find(groupID, role))
			return;

		vector pos = p.m_mPlayerGoals.Get(groupID);
		p.m_mPlayerRoles.Remove(groupID);
		p.m_mPlayerGoals.Remove(groupID);

		vector objPos = obj.GetOwner().GetOrigin();
		DCO_GroupUtilityComponent g;
		switch (role)
		{
			case ROLE_FIX:
			{
				SendFix(cmd, p, 1, now, false);
				g = p.FirstWithRole(ROLE_FIX);
				break;
			}
			case ROLE_BLOCK:
			{
				g = PickGroup(cmd, pos, false, false, 16);
				if (g && cmd.TacticCommit(g, obj, DCO_EGroupTask.DEFEND, CoveredVia(g.GetOwner().GetOrigin(), pos), now))
				{
					cmd.TacticAppendMove(g, pos, now);
					p.AddGroup(g, ROLE_BLOCK, pos);
				}
				else
					g = null;
				break;
			}
			case 1:
			{
				g = PickGroup(cmd, objPos, true, false, 9);
				if (g && cmd.TacticCommit(g, obj, DCO_EGroupTask.RECON, CoveredVia(g.GetOwner().GetOrigin(), pos), now))
				{
					cmd.TacticAppendMove(g, pos, now);
					p.AddGroup(g, 1, pos);
				}
				else
					g = null;
				break;
			}
			default:
			{
				g = PickGroup(cmd, objPos, false, false, 1);
				if (g)
				{
					cmd.TacticAssault(g, obj, now);
					p.AddGroup(g, role, objPos);
				}
				break;
			}
		}

		string name = "none";
		if (g)
			name = g.GetOwner().GetName();
		Event(string.Format("tactic_role_refill obj=%1 tactic=%2 role=%3 from_player_group=%4 to=%5", obj.GetOwner().GetName(), Name(p.m_eTactic), RoleName(role), groupID, name));
	}

	protected void Start(AICommander_BaseComponent cmd, DCO_TacticPlan p, float now)
	{
		CMD_AICommanderObjectiveComponent obj = p.m_Obj;
		vector objPos = obj.GetOwner().GetOrigin();

		switch (p.m_eTactic)
		{
			case DCO_ETactic.MOVEMENT_TO_CONTACT:
			{
				DCO_GroupUtilityComponent lead = PickGroup(cmd, objPos, false, false, 1);
				if (lead && cmd.TacticCommit(lead, obj, DCO_EGroupTask.ATTACK, objPos, now))
					p.AddGroup(lead, 5, objPos);
				DCO_GroupUtilityComponent follow = PickGroup(cmd, objPos, false, false, 1);
				if (follow && lead)
				{
					vector fpos = FollowPos(lead.GetOwner().GetOrigin(), objPos);
					if (cmd.TacticCommit(follow, obj, DCO_EGroupTask.ATTACK, fpos, now))
						p.AddGroup(follow, 4, fpos);
				}
				break;
			}
			case DCO_ETactic.RECON_IN_FORCE:
			case DCO_ETactic.RAID:
			{
				int count = 1;
				if (p.m_eTactic == DCO_ETactic.RAID && p.m_fRatio > 0.5)
					count = 2;
				for (int i = 0; i < count; i++)
				{
					if (TryPlayerRole(cmd, p, ROLE_ASSAULT, objPos, false, false, now))
						continue;
					DCO_GroupUtilityComponent g = PickGroup(cmd, objPos, false, false, 1);
					if (!g)
						break;
					cmd.TacticAssault(g, obj, now);
					p.AddGroup(g, ROLE_ASSAULT, objPos);
				}
				break;
			}
			case DCO_ETactic.HAMMER_AND_ANVIL:
			{
				p.m_vBlock = BlockPos(obj, p.m_vRetreat);
				if (TryPlayerRole(cmd, p, ROLE_BLOCK, p.m_vBlock, false, false, now))
					break;
				DCO_GroupUtilityComponent anvil = PickGroup(cmd, p.m_vBlock, false, false, 16);
				if (anvil && cmd.TacticCommit(anvil, obj, DCO_EGroupTask.DEFEND, CoveredVia(anvil.GetOwner().GetOrigin(), p.m_vBlock), now))
				{
					cmd.TacticAppendMove(anvil, p.m_vBlock, now);
					p.AddGroup(anvil, ROLE_BLOCK, p.m_vBlock);
				}
				break;
			}
			case DCO_ETactic.INFILTRATION:
			{
				p.m_vTarget = KeyPoint(obj);
				if (TryPlayerRole(cmd, p, 1, p.m_vTarget, true, false, now))
					break;
				DCO_GroupUtilityComponent inf = PickGroup(cmd, objPos, true, false, 9);
				if (inf && cmd.TacticCommit(inf, obj, DCO_EGroupTask.RECON, CoveredVia(inf.GetOwner().GetOrigin(), p.m_vTarget), now))
				{
					cmd.TacticAppendMove(inf, p.m_vTarget, now);
					p.AddGroup(inf, 1, p.m_vTarget);
				}
				break;
			}
			case DCO_ETactic.SIEGE:
			{
				if (p.m_bHasRetreat)
				{
					p.m_vBlock = BlockPos(obj, p.m_vRetreat);
					if (!TryPlayerRole(cmd, p, ROLE_BLOCK, p.m_vBlock, false, false, now))
					{
						DCO_GroupUtilityComponent blocker = PickGroup(cmd, p.m_vBlock, false, false, 16);
						if (blocker && cmd.TacticCommit(blocker, obj, DCO_EGroupTask.DEFEND, CoveredVia(blocker.GetOwner().GetOrigin(), p.m_vBlock), now))
						{
							cmd.TacticAppendMove(blocker, p.m_vBlock, now);
							p.AddGroup(blocker, ROLE_BLOCK, p.m_vBlock);
						}
					}
				}
				SendFix(cmd, p, 2, now);
				Harass(cmd, p, now);
				break;
			}
			case DCO_ETactic.ATTACK_BY_FIRE:
			{
				SendFix(cmd, p, 3, now);
				Harass(cmd, p, now);
				break;
			}
			case DCO_ETactic.BYPASS:
			{
				SendFix(cmd, p, 1, now);
				cmd.SetObjectiveCooldown(obj, 600.0);
				p.m_iPhase = 1;
				break;
			}
		}
	}

	static bool HasGunner(IEntity veh)
	{
		if (!veh)
			return false;
		BaseCompartmentManagerComponent cm = BaseCompartmentManagerComponent.Cast(veh.FindComponent(BaseCompartmentManagerComponent));
		if (!cm)
			return false;
		array<BaseCompartmentSlot> slots = {};
		cm.GetCompartments(slots);
		foreach (BaseCompartmentSlot slot : slots)
		{
			if (!TurretCompartmentSlot.Cast(slot))
				continue;
			IEntity occ = slot.GetOccupant();
			if (occ && DCO_Util.IsAlive(occ))
				return true;
		}
		return false;
	}

	protected float FixLosShort(vector site, vector objPos, float r)
	{
		vector eye = site + Vector(0, 1.5, 0);
		vector aim = objPos + Vector(0, 2.0, 0);
		float len = vector.Distance(eye, aim);
		return len * (1.0 - DCO_Util.TraceFrac(eye, aim, null, TraceFlags.WORLD | TraceFlags.ENTS, -1, "t:fix_los")) - (r + 10.0);
	}

	protected bool FixLosOk(vector site, vector objPos, float r)
	{
		return FixLosShort(site, objPos, r) <= 0;
	}

	protected vector FixLosSite(CMD_AICommanderObjectiveComponent obj, vector from, vector pos)
	{
		vector objPos = obj.GetOwner().GetOrigin();
		float r = obj.GetRadius();
		if (pos != vector.Zero && FixLosOk(pos, objPos, r))
		{
			DCO_Perf.Count("fix_los:ok");
			return pos;
		}
		vector side = from - objPos;
		side[1] = 0;
		if (side.LengthSq() < 1)
			side = Vector(0, 0, 1);
		side.Normalize();
		float baseAng = Math.Atan2(side[0], side[2]);
		vector best = vector.Zero;
		float bestS = -1000000.0;
		int tried = 0;
		float minShort = 1000000.0;
		int water = 0;
		for (int ai = -4; ai <= 4; ai++)
		{
			for (int di = 0; di < 3; di++)
			{
				float ang = baseAng + ai * 15.0 * Math.DEG2RAD;
				float d = r + 120.0 + di * 80.0;
				vector c = DCO_Util.Surface(objPos + Vector(Math.Sin(ang) * d, 0, Math.Cos(ang) * d));
				tried++;
				if (c[1] < 0.5)
				{
					water++;
					continue;
				}
				float shortM = FixLosShort(c, objPos, r);
				if (shortM < minShort)
					minShort = shortM;
				if (shortM > 0)
					continue;
				float sc = -Math.AbsFloat(ai) * 2.0 - vector.DistanceXZ(c, from) / 100.0;
				if (sc > bestS)
				{
					bestS = sc;
					best = c;
				}
			}
		}
		string result = "none_keep_old";
		if (best != vector.Zero)
			result = "found";
		DCO_Perf.Count("fix_los:" + result);
		string bestTxt = "-";
		if (best != vector.Zero)
			bestTxt = DCO_Util.XZ(best);
		Event(string.Format("fix_site_los obj=%1 old=%2 tried=%3 result=%4 pos=%5 closest_short_m=%6 water=%7 r=%8", obj.GetOwner().GetName(), DCO_Util.XZ(pos), tried, result, bestTxt, Math.Round(minShort), water, Math.Round(r)));
		if (best != vector.Zero)
			return best;
		return pos;
	}

	vector VehSbfSite(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, DCO_GroupUtilityComponent g, int cls, out string why, out string band, out string at)
	{
		why = "";
		band = "-";
		vector objPos = obj.GetOwner().GetOrigin();
		float r = obj.GetRadius();
		DCO_TacticSituation s = Assess(cmd, obj, DCO_CommanderOps.Now());
		int atLevel = 0;
		if (s.m_bUrban || s.m_bForest)
			atLevel = 1;
		if (s.m_bEnemyArmor || cmd.GetOps().GetIntel(obj).m_bArmor)
			atLevel = 2;
		at = "none";
		if (atLevel == 1)
			at = "rpg_likely";
		else if (atLevel == 2)
			at = "armor";
		bool screen = s.m_iGroups >= 2;
		if (atLevel == 2 && cls < 3)
		{
			why = "armor";
			return vector.Zero;
		}
		if (cls == 1 && (atLevel >= 1 || s.m_bEnemyMG))
		{
			why = "at_or_mg_soft";
			return vector.Zero;
		}
		if (atLevel >= 1 && !screen)
		{
			why = "no_screen";
			return vector.Zero;
		}
		float minD = r + DCO_Ctx.TuneForCmd(cmd, "veh.sbf_apc_min_m", 300.0);
		float maxD = r + DCO_Ctx.TuneForCmd(cmd, "veh.sbf_apc_max_m", 550.0);
		if (cls == 3)
		{
			minD = r + DCO_Ctx.TuneForCmd(cmd, "veh.sbf_tank_min_m", 350.0);
			maxD = r + DCO_Ctx.TuneForCmd(cmd, "veh.sbf_tank_max_m", 650.0);
		}
		else if (cls == 1)
		{
			minD = r + DCO_Ctx.TuneForCmd(cmd, "veh.sbf_soft_min_m", 450.0);
			maxD = r + DCO_Ctx.TuneForCmd(cmd, "veh.sbf_soft_max_m", 700.0);
		}
		if (atLevel >= 1 && cls >= 2)
		{
			minD = Math.Max(minD, r + DCO_Ctx.TuneForCmd(cmd, "veh.sbf_at_floor_m", 500.0));
			maxD = Math.Max(maxD, minD + 200.0);
		}
		if (!screen)
		{
			minD += 100.0;
			maxD += 100.0;
		}
		vector from = g.GetOwner().GetOrigin();
		vector pos = cmd.GetOps().FindOverwatch(obj, from, minD, maxD, (minD + maxD) * 0.5);
		bool losOk = pos != vector.Zero && FixLosOk(pos, objPos, r);
		band = string.Format("%1-%2", Math.Round(minD - r), Math.Round(maxD - r));
		if (!losOk && screen && atLevel == 0)
		{
			pos = cmd.GetOps().FindOverwatch(obj, from, minD - 100.0, maxD - 100.0, (minD + maxD) * 0.5 - 100.0);
			losOk = pos != vector.Zero && FixLosOk(pos, objPos, r);
			band = string.Format("%1-%2", Math.Round(minD - 100.0 - r), Math.Round(maxD - 100.0 - r));
		}
		if (!losOk)
		{
			why = "no_los";
			return vector.Zero;
		}
		return pos;
	}

	protected bool TryVehicleSbf(AICommander_BaseComponent cmd, DCO_TacticPlan p, float now)
	{
		CMD_AICommanderObjectiveComponent obj = p.m_Obj;
		if (p.m_bVehFixOff)
		{
			DCO_Perf.Count("veh_sbf:skip_replan_cap");
			return false;
		}
		vector objPos = obj.GetOwner().GetOrigin();
		float r = obj.GetRadius();
		DCO_TacticSituation s = Assess(cmd, obj, now);
		int atLevel = 0;
		if (s.m_bUrban || s.m_bForest)
			atLevel = 1;
		if (s.m_bEnemyArmor || cmd.GetOps().GetIntel(obj).m_bArmor)
			atLevel = 2;
		bool screen = s.m_iGroups >= 2;
		float search = DCO_Ctx.TuneForCmd(cmd, "veh.sbf_search_m", 1500.0);
		DCO_GroupUtilityComponent best;
		int bestCls = 0;
		float bestD = search;
		int skippedArmor = 0;
		int skippedSoft = 0;
		int skippedCrew = 0;
		foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
		{
			if (!g || g.IsPlayerGroup() || g.IsDedicatedTransport() || g.GetUnitCount() <= 0 || !g.GetOwner())
				continue;
			if (g == p.m_VehFixBad && now < p.m_fVehFixBadUntil)
				continue;
			if (!g.IsAvailableReserve() || !g.CanItHaveOrder() || !cmd.CanCommitGroup(g))
				continue;
			if (!g.GetGroupVehicle())
				continue;
			if (!DCO_ArmorPair.ArmedCrew(g) || !HasGunner(g.GetGroupVehicle()) || !DCO_GroupJobs.Gate(cmd, g, 4, "veh_sbf"))
			{
				skippedCrew++;
				continue;
			}
			int cls = DCO_VehicleActive.VehClass(g.GetGroupVehicle());
			if (cls == 0)
				continue;
			if (atLevel == 2 && cls < 3)
			{
				skippedArmor++;
				continue;
			}
			if (cls == 1 && (atLevel >= 1 || s.m_bEnemyMG))
			{
				skippedSoft++;
				continue;
			}
			float d = vector.DistanceXZ(g.GetOwner().GetOrigin(), objPos);
			if (d > search)
				continue;
			if (cls > bestCls || (cls == bestCls && d < bestD))
			{
				bestCls = cls;
				bestD = d;
				best = g;
			}
		}
		string at = "none";
		if (atLevel == 1)
			at = "rpg_likely";
		else if (atLevel == 2)
			at = "armor";
		if (!best)
		{
			string why = "no_vehicle";
			if (skippedArmor > 0)
				why = "armor";
			else if (skippedSoft > 0)
				why = "at_or_mg_soft";
			else if (skippedCrew > 0)
				why = "crew_or_gunner_or_job";
			DCO_Perf.Count("veh_sbf:skip_" + why);
			Event(string.Format("veh_sbf_skip obj=%1 reason=%2 at=%3", obj.GetOwner().GetName(), why, at));
			return false;
		}
		string vWhy;
		string band;
		string vAt;
		vector pos = VehSbfSite(cmd, obj, best, bestCls, vWhy, band, vAt);
		if (pos == vector.Zero)
		{
			DCO_Perf.Count("veh_sbf:skip_" + vWhy);
			Event(string.Format("veh_sbf_skip obj=%1 reason=%2 at=%3 veh=%4 band=%5", obj.GetOwner().GetName(), vWhy, vAt, best.GetOwner().GetName(), band));
			return false;
		}
		if (!cmd.TacticCommit(best, obj, DCO_EGroupTask.SUPPORT_BY_FIRE, pos, now))
		{
			DCO_Perf.Count("veh_sbf:skip_commit");
			return false;
		}
		SCR_AIWaypoint sup = cmd.SpawnSuppressWP(objPos);
		if (sup)
			best.MoveTo(sup, now);
		p.AddGroup(best, ROLE_FIX, pos);
		p.m_VehFix = best;
		p.m_VehFixVeh = best.GetGroupVehicle();
		p.m_vVehFixPos = best.GetOwner().GetOrigin();
		if (p.m_VehFixVeh)
			p.m_vVehFixPos = p.m_VehFixVeh.GetOrigin();
		p.m_fVehFixMoveAt = now;
		p.m_fVehFixOutAt = -1;
		p.m_fVehFixAt = now;
		DCO_Perf.Count("veh_sbf:pick_" + DCO_VehicleActive.ClassName(bestCls));
		Event(string.Format("veh_sbf_pick obj=%1 veh=%2 class=%3 band=%4 los_ok=1 screen=%5 at_level=%6 pos=%7 dist=%8", obj.GetOwner().GetName(), best.GetOwner().GetName(), DCO_VehicleActive.ClassName(bestCls), band, DCO_Util.B01(screen), at, DCO_Util.XZ(pos), Math.Round(bestD)));
		return true;
	}

	protected void VehFixClear(DCO_TacticPlan p, string why)
	{
		string gname = "-";
		if (p.m_VehFix && p.m_VehFix.GetOwner())
			gname = p.m_VehFix.GetOwner().GetName();
		p.m_VehFix = null;
		p.m_VehFixVeh = null;
		Event(string.Format("veh_sbf_clear obj=%1 grp=%2 why=%3", p.m_Obj.GetOwner().GetName(), gname, why));
	}

	protected void VehFixCheck(AICommander_BaseComponent cmd, DCO_TacticPlan p, float now)
	{
		if (!p.m_VehFixVeh && !p.m_VehFix)
			return;
		if (p.m_bReleaseLogged)
		{
			VehFixClear(p, "assault_released");
			return;
		}
		DCO_GroupUtilityComponent g = p.m_VehFix;
		IEntity veh = p.m_VehFixVeh;
		int idx = -1;
		if (g)
			idx = p.m_aGroups.Find(g);
		if (g && idx < 0)
		{
			VehFixClear(p, "not_in_plan");
			return;
		}
		if (g && g.GetOwner() && g.GetUnitCount() > 0 && (g.GetTask() != DCO_EGroupTask.SUPPORT_BY_FIRE || p.m_sConsolidated.Contains(g)))
		{
			VehFixClear(p, "task_changed");
			return;
		}
		string why;
		if (!g || !g.GetOwner() || g.GetUnitCount() <= 0)
			why = "crew_lost";
		else if (!veh)
			why = "vehicle_gone";
		else
		{
			DamageManagerComponent dmg = DamageManagerComponent.Cast(veh.FindComponent(DamageManagerComponent));
			if (dmg && dmg.GetState() == EDamageState.DESTROYED)
				why = "destroyed";
		}
		if (why.IsEmpty())
		{
			if (!g.IsMountedIn(veh))
			{
				if (p.m_fVehFixOutAt < 0)
					p.m_fVehFixOutAt = now;
				else if (now - p.m_fVehFixOutAt > 45.0)
					why = "dismounted";
			}
			else
			{
				p.m_fVehFixOutAt = -1;
				if (!HasGunner(veh))
					why = "no_gunner";
			}
		}
		if (why.IsEmpty() && p.m_fVehFixOutAt < 0)
		{
			vector vp = veh.GetOrigin();
			bool nearGoal = idx < p.m_aGoals.Count() && vector.DistanceXZ(vp, p.m_aGoals[idx]) <= 80.0;
			if (nearGoal || vector.DistanceXZ(vp, p.m_vVehFixPos) > 15.0)
			{
				p.m_vVehFixPos = vp;
				p.m_fVehFixMoveAt = now;
			}
			else if (now - p.m_fVehFixMoveAt > 90.0)
				why = "stuck";
		}
		if (why.IsEmpty())
			return;
		string vname = "-";
		if (veh)
			vname = veh.GetName();
		string gname = "-";
		if (g && g.GetOwner())
			gname = g.GetOwner().GetName();
		DCO_Perf.Count("veh_sbf:lost_" + why);
		Event(string.Format("veh_sbf_lost obj=%1 grp=%2 veh=%3 why=%4 after_s=%5 replans=%6", p.m_Obj.GetOwner().GetName(), gname, vname, why, Math.Round(now - p.m_fVehFixAt), p.m_iVehFixReplans));
		p.m_VehFix = null;
		p.m_VehFixVeh = null;
		if (idx >= 0)
			p.RemoveAt(idx);
		if (g && g.GetOwner() && g.GetUnitCount() > 0)
		{
			cmd.TacticRelease(g, p.m_Obj);
			p.m_VehFixBad = g;
			p.m_fVehFixBadUntil = now + 180.0;
		}
		p.m_iVehFixReplans++;
		if (p.m_iVehFixReplans >= 2)
		{
			p.m_bVehFixOff = true;
			Event(string.Format("veh_sbf_replan_skip obj=%1 replans=%2 -- vehicle SBF off for this plan, infantry FIX only", p.m_Obj.GetOwner().GetName(), p.m_iVehFixReplans));
		}
		SendFix(cmd, p, 1, now, false);
		DCO_GroupUtilityComponent nf = p.FirstWithRole(ROLE_FIX);
		string nfName = "none";
		if (nf && nf.GetOwner())
			nfName = nf.GetOwner().GetName();
		Event(string.Format("veh_sbf_replan obj=%1 why=%2 new_fix=%3 replans=%4", p.m_Obj.GetOwner().GetName(), why, nfName, p.m_iVehFixReplans));
	}

	protected void SendFix(AICommander_BaseComponent cmd, DCO_TacticPlan p, int count, float now, bool allowPlayers = true)
	{
		vector objPos = p.m_Obj.GetOwner().GetOrigin();
		for (int i = 0; i < count; i++)
		{
			if (allowPlayers && TryPlayerRole(cmd, p, ROLE_FIX, objPos, false, true, now))
				continue;
			if (i == 0 && DCO_Ctx.TuneForCmd(cmd, "veh.sbf", 0) > 0 && TryVehicleSbf(cmd, p, now))
				continue;
			DCO_GroupUtilityComponent g = PickGroup(cmd, objPos, false, true, 4);
			if (!g)
				g = PickGroup(cmd, objPos, false, false, 4);
			if (!g)
				return;

			vector pos = cmd.GetOps().FindOverwatch(p.m_Obj, g.GetOwner().GetOrigin());
			if (pos == vector.Zero)
				pos = cmd.TacticStagingPos(p.m_Obj);
			if (DCO_Ctx.TuneForCmd(cmd, "tac.fix_los", 1) > 0)
				pos = FixLosSite(p.m_Obj, g.GetOwner().GetOrigin(), pos);
			if (!cmd.TacticCommit(g, p.m_Obj, DCO_EGroupTask.SUPPORT_BY_FIRE, pos, now))
				return;

			SCR_AIWaypoint sup = cmd.SpawnSuppressWP(objPos);
			if (sup)
				g.MoveTo(sup, now);
			p.AddGroup(g, ROLE_FIX, pos);
		}
	}

	protected void Harass(AICommander_BaseComponent cmd, DCO_TacticPlan p, float now)
	{
		CMD_ArtillerySupport arty = cmd.GetArtySupport();
		if (!arty || !arty.HasRegisteredUnits() || now - p.m_fLastFire < 120.0)
			return;

		p.m_fLastFire = now;
		CMD_FireMissionRequest req = new CMD_FireMissionRequest(p.m_Obj.GetOwner().GetOrigin(), SCR_EAIArtilleryAmmoType.HIGH_EXPLOSIVE, now, 3);
		arty.ApplyTier(req, "area", p.m_Obj.GetRadius(), "tactic_" + Name(p.m_eTactic));
		if (arty.HasFriendlyNearRequest(req, now))
		{
			CMD_ArtillerySupport.LogDenied(req, "friendly");
			return;
		}
		arty.RequestShellImpact(req, now, 3);
	}

	protected vector FollowPos(vector leadPos, vector objPos)
	{
		vector back = leadPos - objPos;
		back[1] = 0;
		if (back.Length() < 1)
			return leadPos;
		back.Normalize();
		vector pos = leadPos + back * 250.0;
		pos[1] = GetGame().GetWorld().GetSurfaceY(pos[0], pos[2]);
		return pos;
	}

	protected vector BlockPos(CMD_AICommanderObjectiveComponent obj, vector dir)
	{
		vector objPos = obj.GetOwner().GetOrigin();
		float dist = obj.GetRadius() + (150.0 + 300.0) * 0.5;
		vector pos = objPos + dir * dist;
		vector cover = DCO_TerrainCache.QueryBest(DCO_ETerrainFlag.FOREST_EDGE | DCO_ETerrainFlag.HILLTOP | DCO_ETerrainFlag.OVERWATCH, pos, (300.0 - 150.0) * 0.5 + 50, string.Empty, pos);
		if (cover != vector.Zero)
			pos = cover;
		pos[1] = GetGame().GetWorld().GetSurfaceY(pos[0], pos[2]);
		return pos;
	}

	protected vector CoveredVia(vector from, vector to)
	{
		vector mid = (from + to) * 0.5;
		float r = Math.Max(vector.DistanceXZ(from, to) * 0.35, 100);
		vector cover = DCO_TerrainCache.QueryBest(DCO_ETerrainFlag.FOREST_EDGE, mid, r, string.Empty, mid);
		if (cover == vector.Zero)
			return to;
		return cover;
	}

	protected vector KeyPoint(CMD_AICommanderObjectiveComponent obj)
	{
		vector objPos = obj.GetOwner().GetOrigin();
		DCO_GarrisonRegistry reg = DCO_GarrisonRegistry.GetInstance();
		if (!reg)
			return objPos;

		array<IEntity> buildings = {};
		reg.FindBuildings(objPos, obj.GetRadius(), buildings);
		float best = float.MAX;
		vector pos = objPos;
		foreach (IEntity b : buildings)
		{
			float d = vector.DistanceXZ(b.GetOrigin(), objPos);
			if (d < best)
			{
				best = d;
				pos = b.GetOrigin();
			}
		}
		return pos;
	}

	void Tick(AICommander_BaseComponent cmd, float timeSlice)
	{
		m_fTimer += timeSlice;
		if (m_fTimer < TICK_S())
			return;
		m_fTimer = 0;

		float now = DCO_CommanderOps.Now();
		array<CMD_AICommanderObjectiveComponent> done = {};
		foreach (CMD_AICommanderObjectiveComponent obj, DCO_TacticPlan p : m_mPlans)
		{
			if (!obj || !p || UpdatePlan(cmd, p, now))
				done.Insert(obj);
		}
		foreach (CMD_AICommanderObjectiveComponent o : done)
			m_mPlans.Remove(o);

		UpdatePursuits(cmd, now);
	}

	protected bool UpdatePlan(AICommander_BaseComponent cmd, DCO_TacticPlan p, float now)
	{
		CMD_AICommanderObjectiveComponent obj = p.m_Obj;
		FactionKey fk = cmd.GetCommanderFactionKey();
		CMD_EObjectiveState st = obj.GetObjectiveState(fk);
		if (obj.IsCapturedBy(fk))
		{
			m_mFailedMask.Remove(obj);
			m_mFailedAt.Remove(obj);
			DCO_Perf.Count("tactic:captured");
			Event(string.Format("tactic_result obj=%1 tactic=%2 result=captured via=owner_flip after_s=%3", obj.GetOwner().GetName(), Name(p.m_eTactic), Math.Round(now - p.m_fStart)));
			LearnEnd(p, "captured", now);
			if (IsAllowed(cmd, DCO_ETactic.PURSUIT))
				TryPursuit(cmd, obj, now);
			return true;
		}
		if (st == CMD_EObjectiveState.COMPLETED || st == CMD_EObjectiveState.FAILED)
		{
			if (st == CMD_EObjectiveState.FAILED)
				LearnEnd(p, "failed", now);
			return true;
		}

		VehFixCheck(cmd, p, now);

		if (!p.m_bReleaseLogged && !ControlsObjective(obj) && cmd.IsAssaultReleased(obj))
		{
			p.m_bReleaseLogged = true;
			int committed;
			foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
			{
				if (g && g.GetGroupObjective() == obj)
					committed++;
			}
			Event(string.Format("tactic_phase obj=%1 tactic=%2 phase=assault_released after_s=%3 groups=%4 roles=%5", obj.GetOwner().GetName(), Name(p.m_eTactic), Math.Round(now - p.m_fStart), committed, p.m_aGroups.Count()));
		}
		if (p.m_bReleaseLogged)
		{
			PushStalled(cmd, p, now);
			Consolidate(cmd, p, now);
		}

		switch (p.m_eTactic)
		{
			case DCO_ETactic.MOVEMENT_TO_CONTACT: return UpdateMTC(cmd, p, now);
			case DCO_ETactic.RECON_IN_FORCE: return UpdateProbe(cmd, p, now, true);
			case DCO_ETactic.RAID: return UpdateProbe(cmd, p, now, false);
			case DCO_ETactic.SIEGE: return UpdateStandoff(cmd, p, now, true);
			case DCO_ETactic.ATTACK_BY_FIRE: return UpdateStandoff(cmd, p, now, false);
			case DCO_ETactic.BYPASS: return UpdateBypass(cmd, p, now);
			case DCO_ETactic.FIRE_AND_MANEUVER: UpdateBounding(cmd, p, now); break;
			case DCO_ETactic.INFILTRATION: UpdateInfiltration(cmd, p, now); break;
		}
		return false;
	}

	protected float AggTempo(AICommander_BaseComponent cmd)
	{
		if (cmd.DCO_K2())
			return cmd.DCO_K2Tempo();
		if (cmd.DCO_AggProfile())
			return Math.Clamp(Math.Pow(cmd.DCO_AggrA(), 0.5), 0.7, 2.0);
		return 1.0;
	}

	protected float AggWait(AICommander_BaseComponent cmd, string key, float def, float floor)
	{
		float x = DCO_Ctx.TuneForCmd(cmd, key, def);
		float t = AggTempo(cmd);
		if (t == 1.0)
			return x;
		DCO_Perf.Count("agg:p7_cmd");
		return Math.Max(x / t, Math.Min(x, floor));
	}

	protected void PushStalled(AICommander_BaseComponent cmd, DCO_TacticPlan p, float now)
	{
		CMD_AICommanderObjectiveComponent obj = p.m_Obj;
		vector objPos = obj.GetOwner().GetOrigin();
		foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
		{
			if (!g || g.GetGroupObjective() != obj || g.IsInTransport() || g.IsArmor() || g.IsPlayerGroup())
				continue;
			DCO_EGroupTask t = g.GetTask();
			if (t != DCO_EGroupTask.ATTACK && t != DCO_EGroupTask.FLANK)
				continue;
			float d = vector.DistanceXZ(g.GetOwner().GetOrigin(), objPos);
			if (d <= obj.GetRadius() * 0.8)
				continue;
			float best;
			if (!p.m_mBestDist.Find(g, best) || d < best - 15.0)
			{
				p.m_mBestDist.Set(g, d);
				p.m_mProgressAt.Set(g, now);
				continue;
			}
			float limit = AggWait(cmd, "tac.stall_s", 60.0, 30.0);
			if (g.HasState(DCO_EGroupState.IN_CONTACT))
				limit = AggWait(cmd, "tac.stall_contact_s", 120.0, 30.0);
			if (now - p.m_mProgressAt.Get(g) < limit)
				continue;
			string how = "reroute";
			if (DCO_Ctx.TuneForCmd(cmd, "tac.push_keep_route", 0) > 0 && g.IsGroupHaveWaypoint())
			{
				how = "keep_route";
				SCR_AIGroupUtilityComponent pu = SCR_AIGroupUtilityComponent.Cast(g.GetOwner().FindComponent(SCR_AIGroupUtilityComponent));
				if (pu)
					pu.DCO_ForcePosture(DCO_GroupTactics.AGGRESIVE, GetGame().GetWorld().GetWorldTime(), 90000.0, false, "tactic_push");
			}
			else
				cmd.TacticAssault(g, obj, now);
			float drive = DCO_AssaultDrive.Start(g);
			DCO_Perf.Count("tactic_push:" + how);
			p.m_mBestDist.Set(g, d);
			p.m_mProgressAt.Set(g, now);
			DCO_Perf.Count("tactic_push");
			Event(string.Format("tactic_push obj=%1 tactic=%2 grp=%3 task=%4 dist=%5 contact=%6 drive_s=%7", obj.GetOwner().GetName(), Name(p.m_eTactic), g.GetOwner().GetName(), typename.EnumToString(DCO_EGroupTask, t), Math.Round(d), g.HasState(DCO_EGroupState.IN_CONTACT), Math.Round(drive)) + " how=" + how);
		}
	}

	protected void Consolidate(AICommander_BaseComponent cmd, DCO_TacticPlan p, float now)
	{
		CMD_AICommanderObjectiveComponent obj = p.m_Obj;
		FactionKey fk = cmd.GetCommanderFactionKey();
		string why = "neutral";
		if (!obj.GetOwnerFaction().IsEmpty() && obj.GetControl(fk) <= 0)
		{
			if (now < p.m_fConsolCheckAt)
				return;
			p.m_fConsolCheckAt = now + 10.0;
			int friendlyNear;
			int enemyNear;
			obj.CountNearbyUnitsCached(obj.GetRadius() + DCO_Ctx.TuneForCmd(cmd, "tactic.consol_ring_m", 150.0), fk, friendlyNear, enemyNear);
			if (enemyNear > DCO_Ctx.TuneForCmd(cmd, "tactic.consol_enemy_max", 1.0))
				return;
			why = "enemy_thin";
		}
		vector objPos = obj.GetOwner().GetOrigin();
		foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
		{
			if (!g || p.m_sConsolidated.Contains(g) || g.GetGroupObjective() != obj || g.IsInTransport() || g.IsArmor() || g.IsPlayerGroup() || g.IsMortar())
				continue;
			DCO_EGroupTask t = g.GetTask();
			if (t != DCO_EGroupTask.ATTACK && t != DCO_EGroupTask.FLANK && t != DCO_EGroupTask.SUPPORT_BY_FIRE)
				continue;
			p.m_sConsolidated.Insert(g);
			float d = vector.DistanceXZ(g.GetOwner().GetOrigin(), objPos);
			if (d <= obj.GetRadius() * 0.8)
				continue;
			cmd.TacticAssault(g, obj, now);
			DCO_AssaultDrive.Start(g);
			p.m_mBestDist.Set(g, d);
			p.m_mProgressAt.Set(g, now);
			DCO_Perf.Count("tactic_consolidate");
			Event(string.Format("tactic_consolidate obj=%1 tactic=%2 grp=%3 from_task=%4 dist=%5 owner=%6 control=%7 why=%8", obj.GetOwner().GetName(), Name(p.m_eTactic), g.GetOwner().GetName(), typename.EnumToString(DCO_EGroupTask, t), Math.Round(d), obj.GetOwnerFaction(), Math.Round(obj.GetControl(fk)), why));
		}
	}

	protected void Rechoose(AICommander_BaseComponent cmd, DCO_TacticPlan p, string why)
	{
		int freed;
		foreach (DCO_GroupUtilityComponent g : p.m_aGroups)
		{
			if (!g || g.IsInTransport())
				continue;
			cmd.TacticRelease(g, p.m_Obj);
			freed++;
		}
		MarkFailed(p.m_Obj, p.m_eTactic);
		DCO_TacticTest.NoteHandOff(cmd, p.m_Obj, why);
		Event(string.Format("tactic_phase obj=%1 tactic=%2 phase=rechoose reason=%3 freed=%4", p.m_Obj.GetOwner().GetName(), Name(p.m_eTactic), why, freed));
		cmd.TacticRestart(p.m_Obj);
	}

	protected bool UpdateMTC(AICommander_BaseComponent cmd, DCO_TacticPlan p, float now)
	{
		DCO_GroupUtilityComponent lead = p.FirstWithRole(5);
		DCO_ObjectiveIntel intel = cmd.GetOps().GetIntel(p.m_Obj);
		DCO_GroupUtilityComponent follow = p.FirstWithRole(4);
		bool contact = (lead && lead.HasState(DCO_EGroupState.IN_CONTACT)) || (follow && follow.HasState(DCO_EGroupState.IN_CONTACT))
			|| (intel.m_bEverSeen && intel.m_fConfidence >= Math.Max(0.3, p.m_fIntelConf0 + 0.2));
		if (contact || !lead || now - p.m_fStart > 600.0)
		{
			string why = "contact";
			if (!lead)
				why = "lead_lost";
			else if (!contact)
				why = "timeout";
			Rechoose(cmd, p, why);
			return true;
		}

		if (follow && !follow.HasState(DCO_EGroupState.IN_CONTACT))
		{
			vector fpos = FollowPos(lead.GetOwner().GetOrigin(), p.m_Obj.GetOwner().GetOrigin());
			if (vector.DistanceXZ(follow.GetOwner().GetOrigin(), fpos) > 80)
				cmd.TacticMove(follow, fpos, now);
		}
		return false;
	}

	protected bool UpdateProbe(AICommander_BaseComponent cmd, DCO_TacticPlan p, float now, bool rechoose)
	{
		if (p.m_iPhase == 0)
		{
			int units, start;
			bool engaged;
			vector objPos = p.m_Obj.GetOwner().GetOrigin();
			float reach = p.m_Obj.GetRadius() + 75.0;
			foreach (int i, DCO_GroupUtilityComponent g : p.m_aGroups)
			{
				if (!g)
					continue;
				units += g.GetUnitCount();
				start += p.m_aStartUnits[i];
				if (g.HasState(DCO_EGroupState.IN_CONTACT) || vector.DistanceXZ(g.GetOwner().GetOrigin(), objPos) <= reach)
					engaged = true;
			}
			if (engaged && p.m_fArrivedAt < 0)
			{
				p.m_fArrivedAt = now;
				Event(string.Format("tactic_phase obj=%1 tactic=%2 phase=strike after_s=%3", p.m_Obj.GetOwner().GetName(), Name(p.m_eTactic), Math.Round(now - p.m_fStart)));
			}

			bool losses = start > 0 && units <= start * (1 - 0.25);
			bool struck = p.m_fArrivedAt >= 0 && now - p.m_fArrivedAt >= 150.0;
			bool noContact = p.m_fArrivedAt < 0 && now - p.m_fStart >= 600.0;
			if (!struck && !noContact && !losses && (units > 0 || !p.m_mPlayerRoles.IsEmpty()))
				return false;

			p.m_iPhase = 1;
			p.m_fPhaseStart = now;
			vector back = cmd.TacticStagingPos(p.m_Obj);
			cmd.GetPlayerTasking().RolePhase(cmd, p.m_Obj, p.m_mPlayerRoles, "role_withdraw", back);
			foreach (DCO_GroupUtilityComponent g2 : p.m_aGroups)
			{
				if (g2)
					cmd.TacticMove(g2, back, now);
			}
			string why = "time";
			if (losses)
				why = "losses";
			else if (noContact)
				why = "no_contact";
			Event(string.Format("tactic_phase obj=%1 tactic=%2 phase=withdraw reason=%3 units=%4/%5", p.m_Obj.GetOwner().GetName(), Name(p.m_eTactic), why, units, start));
			return false;
		}

		if (now - p.m_fPhaseStart < 60)
			return false;

		if (rechoose)
		{
			Rechoose(cmd, p, "intel");
			return true;
		}

		foreach (DCO_GroupUtilityComponent g3 : p.m_aGroups)
		{
			if (g3)
				cmd.TacticRelease(g3, p.m_Obj);
		}
		cmd.TacticAbort(p.m_Obj, now);
		cmd.SetObjectiveCooldown(p.m_Obj, 300.0);
		Event(string.Format("tactic_result obj=%1 tactic=%2 result=raid_done", p.m_Obj.GetOwner().GetName(), Name(p.m_eTactic)));
		LearnEnd(p, "raid_done", now);
		return true;
	}

	protected bool UpdateStandoff(AICommander_BaseComponent cmd, DCO_TacticPlan p, float now, bool siege)
	{
		Harass(cmd, p, now);
		if (now - p.m_fLastEval < 120.0)
			return false;
		p.m_fLastEval = now;

		DCO_TacticSituation s = Assess(cmd, p.m_Obj, now);
		float current = s.m_fRatio;
		DCO_StrengthInfo info = new DCO_StrengthInfo();
		foreach (DCO_GroupUtilityComponent g : p.m_aGroups)
		{
			if (!g)
				continue;
			DCO_Strength.OfGroup(SCR_AIGroup.Cast(g.GetOwner()), false, info);
			current += info.m_fStrength / Math.Max(cmd.GetOps().EstimateDefenders(p.m_Obj), 1);
		}

		if (current >= s.m_fMinRatio && DCO_Ctx.TuneForCmd(cmd, "tac.standoff_fix", 0) > 0)
		{
			float minS = DCO_Ctx.TuneForCmd(cmd, "tac.standoff_min_s", 240.0);
			float minConf = DCO_Ctx.TuneForCmd(cmd, "tac.standoff_conf", 0.5);
			bool lowConf = s.m_fConfidence < minConf && now - p.m_fStart < DCO_Ctx.TuneForCmd(cmd, "tac.standoff_max_s", 600.0);
			if (now - p.m_fStart < minS || lowConf)
			{
				string why = "too_early";
				if (now - p.m_fStart >= minS)
					why = "low_conf";
				DCO_Perf.Count("standoff:hold_" + why);
				Event(string.Format("tactic_standoff_hold obj=%1 tactic=%2 why=%3 ratio=%4 conf=%5 age_s=%6", p.m_Obj.GetOwner().GetName(), Name(p.m_eTactic), why, current.ToString(-1, 2), s.m_fConfidence.ToString(-1, 2), Math.Round(now - p.m_fStart)));
				return false;
			}
		}
		if (current >= s.m_fMinRatio)
		{
			Event(string.Format("tactic_phase obj=%1 tactic=%2 phase=escalate ratio=%3", p.m_Obj.GetOwner().GetName(), Name(p.m_eTactic), current.ToString(-1, 2)));
			Rechoose(cmd, p, "escalate");
			return true;
		}

		if (now - p.m_fStart < 900.0)
			return false;

		foreach (DCO_GroupUtilityComponent g2 : p.m_aGroups)
		{
			if (g2)
				cmd.TacticRelease(g2, p.m_Obj);
		}
		MarkFailed(p.m_Obj, p.m_eTactic);
		cmd.TacticAbort(p.m_Obj, now);
		Event(string.Format("tactic_result obj=%1 tactic=%2 result=timeout ratio=%3", p.m_Obj.GetOwner().GetName(), Name(p.m_eTactic), current.ToString(-1, 2)));
		LearnEnd(p, "timeout", now);
		return true;
	}

	protected bool UpdateBypass(AICommander_BaseComponent cmd, DCO_TacticPlan p, float now)
	{
		if (now - p.m_fStart < 600.0)
			return false;

		foreach (DCO_GroupUtilityComponent g : p.m_aGroups)
		{
			if (g)
				cmd.TacticRelease(g, p.m_Obj);
		}
		Event(string.Format("tactic_result obj=%1 tactic=%2 result=bypass_end", p.m_Obj.GetOwner().GetName(), Name(p.m_eTactic)));
		cmd.TacticRestart(p.m_Obj);
		return true;
	}

	protected void UpdateBounding(AICommander_BaseComponent cmd, DCO_TacticPlan p, float now)
	{
		CMD_AICommanderObjectiveComponent obj = p.m_Obj;
		if (!cmd.IsAssaultReleased(obj))
			return;

		vector objPos = obj.GetOwner().GetOrigin();
		float finalDist = obj.GetRadius() + 150.0;

		array<DCO_GroupUtilityComponent> groups = {};
		foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
		{
			if (g && g.GetGroupObjective() == obj && !g.IsInTransport() && !g.IsArmor()
				&& (g.GetTask() == DCO_EGroupTask.ATTACK || g.GetTask() == DCO_EGroupTask.FLANK))
				groups.Insert(g);
		}
		if (groups.Count() < 2 || p.m_iPhase == 2)
			return;

		bool allClose = true;
		foreach (DCO_GroupUtilityComponent g2 : groups)
		{
			if (vector.DistanceXZ(g2.GetOwner().GetOrigin(), objPos) > finalDist)
				allClose = false;
		}
		if (allClose)
		{
			p.m_iPhase = 2;
			foreach (DCO_GroupUtilityComponent g3 : groups)
				cmd.TacticAssault(g3, obj, now);
			Event(string.Format("tactic_phase obj=%1 tactic=%2 phase=final_assault groups=%3", obj.GetOwner().GetName(), Name(p.m_eTactic), groups.Count()));
			return;
		}

		p.m_iPhase = 1;
		DCO_GroupUtilityComponent mover = p.FirstWithRole(1);
		if (mover && groups.Contains(mover))
		{
			int mi = p.m_aGroups.Find(mover);
			if (vector.DistanceXZ(mover.GetOwner().GetOrigin(), p.m_aGoals[mi]) > 30.0 && now - p.m_fPhaseStart < 90)
				return;
			p.m_aRoles[mi] = ROLE_FIX;
			SCR_AIWaypoint sup = cmd.SpawnSuppressWP(objPos);
			mover.CompleteAllWaypoints();
			if (sup)
				mover.MoveTo(sup, now);
		}

		DCO_GroupUtilityComponent rear;
		float rearDist = -1;
		foreach (DCO_GroupUtilityComponent g4 : groups)
		{
			if (g4 == mover)
				continue;
			float d = vector.DistanceXZ(g4.GetOwner().GetOrigin(), objPos);
			if (d > rearDist)
			{
				rearDist = d;
				rear = g4;
			}
		}
		if (!rear)
			return;

		vector dir = objPos - rear.GetOwner().GetOrigin();
		dir[1] = 0;
		dir.Normalize();
		float step = Math.Min(125.0, Math.Max(rearDist - obj.GetRadius(), 20));
		vector goal = rear.GetOwner().GetOrigin() + dir * step;
		goal[1] = GetGame().GetWorld().GetSurfaceY(goal[0], goal[2]);

		int ri = p.m_aGroups.Find(rear);
		if (ri < 0)
		{
			p.AddGroup(rear, 1, goal);
		}
		else
		{
			p.m_aRoles[ri] = 1;
			p.m_aGoals[ri] = goal;
		}
		foreach (DCO_GroupUtilityComponent fixer : groups)
		{
			if (fixer == rear)
				continue;
			int fi = p.m_aGroups.Find(fixer);
			if (fi >= 0 && p.m_aRoles[fi] == ROLE_FIX)
				continue;
			if (fi < 0)
				p.AddGroup(fixer, ROLE_FIX, fixer.GetOwner().GetOrigin());
			else
				p.m_aRoles[fi] = ROLE_FIX;

			SCR_AIWaypoint fixWp = cmd.SpawnSuppressWP(objPos);
			fixer.CompleteAllWaypoints();
			if (fixWp)
				fixer.MoveTo(fixWp, now);
		}

		p.m_fPhaseStart = now;
		cmd.TacticMove(rear, goal, now);
		Event(string.Format("tactic_phase obj=%1 tactic=%2 phase=bound grp=%3 dist=%4", obj.GetOwner().GetName(), Name(p.m_eTactic), rear.GetOwner(), Math.Round(rearDist)));
	}

	protected void UpdateInfiltration(AICommander_BaseComponent cmd, DCO_TacticPlan p, float now)
	{
		if (p.m_iPhase != 0)
			return;

		DCO_GroupUtilityComponent inf = p.FirstWithRole(1);
		if (!inf)
		{
			p.m_iPhase = 1;
			return;
		}

		bool arrived = vector.DistanceXZ(inf.GetOwner().GetOrigin(), p.m_vTarget) <= 40;
		if (!arrived && !inf.HasState(DCO_EGroupState.IN_CONTACT) && !cmd.IsAssaultReleased(p.m_Obj))
			return;

		p.m_iPhase = 1;
		cmd.TacticAssault(inf, p.m_Obj, now);
		string why = "arrived";
		if (!arrived)
			why = "contact";
		Event(string.Format("tactic_phase obj=%1 tactic=%2 phase=strike reason=%3", p.m_Obj.GetOwner().GetName(), Name(p.m_eTactic), why));
	}

	void OnCaptured(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, float now)
	{
		DCO_TacticPlan p = m_mPlans.Get(obj);
		string tactic = "none";
		if (p)
		{
			tactic = Name(p.m_eTactic);
			LearnEnd(p, "captured", now);
		}
		m_mPlans.Remove(obj);
		m_mFailedMask.Remove(obj);
		m_mFailedAt.Remove(obj);
		DCO_Perf.Count("tactic:captured");
		Event(string.Format("tactic_result obj=%1 tactic=%2 result=captured", obj.GetOwner().GetName(), tactic));

		if (IsAllowed(cmd, DCO_ETactic.PURSUIT))
			TryPursuit(cmd, obj, now);
	}

	void OnOwnerFlip(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, float now)
	{
		if (m_mPlans.Contains(obj) || !m_mFailedAt.Contains(obj))
			return;
		int mask = m_mFailedMask.Get(obj);
		m_mFailedMask.Remove(obj);
		m_mFailedAt.Remove(obj);
		DCO_Perf.Count("tactic:captured");
		Event(string.Format("tactic_result obj=%1 tactic=none result=captured via=owner_flip_noplan failed_mask=%2", obj.GetOwner().GetName(), mask));
		if (IsAllowed(cmd, DCO_ETactic.PURSUIT))
			TryPursuit(cmd, obj, now);
	}

	void OnAborted(CMD_AICommanderObjectiveComponent obj, string reason)
	{
		DCO_TacticPlan p = m_mPlans.Get(obj);
		if (!p)
			return;
		MarkFailed(obj, p.m_eTactic);
		LearnEnd(p, "abort", DCO_Util.NowS());
		m_mPlans.Remove(obj);
		Event(string.Format("tactic_abort obj=%1 tactic=%2 reason=%3", obj.GetOwner().GetName(), Name(p.m_eTactic), reason));
	}

	void OnReset(CMD_AICommanderObjectiveComponent obj)
	{
		DCO_TacticPlan p = m_mPlans.Get(obj);
		if (p && !ControlsObjective(obj))
			m_mPlans.Remove(obj);
	}

	protected void TryPursuit(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, float now)
	{
		CMD_ThreatResponseComponent tr = cmd.GetThreatResponseComponent();
		if (!tr)
			return;

		vector objPos = obj.GetOwner().GetOrigin();
		vector away = vector.Zero;
		int enemies = 0;
		foreach (CMD_ThreatEntry t : tr.GetThreats())
		{
			if (!t || vector.DistanceXZ(t.m_vBelievedPos, objPos) > obj.GetRadius() + 500)
				continue;
			vector rel = t.m_vBelievedPos - objPos;
			rel[1] = 0;
			if (vector.Dot(t.m_vVelocity, rel) <= 0)
				continue;
			away = away + rel;
			enemies += t.m_iEstimatedEnemyCount;
		}
		if (enemies == 0 || away.Length() < 1)
			return;

		DCO_GroupUtilityComponent best;
		int bestUnits = 0;
		float friendly = 0;
		DCO_StrengthInfo info = new DCO_StrengthInfo();
		foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
		{
			if (!g || g.GetGroupObjective() != obj || g.IsPlayerGroup() || g.IsInTransport())
				continue;
			DCO_Strength.OfGroup(SCR_AIGroup.Cast(g.GetOwner()), false, info);
			friendly += info.m_fStrength;
			DCO_EGroupTask pt = g.GetTask();
			if (pt == DCO_EGroupTask.DEFEND || pt == DCO_EGroupTask.GARRISON)
			{
				DCO_Perf.Count("pursuit:skip_holding");
				DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("pursuit_skip cmd=%1 obj=%2 grp=%3 reason=holds_objective task=%4", cmd.GetCommanderUID(), obj.GetLogName(), DCO_Log.IdGroup(SCR_AIGroup.Cast(g.GetOwner())), typename.EnumToString(DCO_EGroupTask, pt)));
				continue;
			}
			if (g.GetUnitCount() > bestUnits && !g.HasState(DCO_EGroupState.IN_CONTACT) && DCO_GroupJobs.Gate(cmd, g, 1, "pursuit"))
			{
				bestUnits = g.GetUnitCount();
				best = g;
			}
		}
		float pr = DCO_Ctx.TuneForCmd(cmd, "tac.pursuit_ratio", 1.5);
		float ptp = AggTempo(cmd);
		if (ptp != 1.0)
			pr = Math.Max(pr / ptp, Math.Min(pr, 1.2));
		if (!best || friendly < enemies * pr)
			return;

		away[1] = 0;
		away.Normalize();
		vector goal = objPos + away * (obj.GetRadius() + 400.0);
		goal[1] = GetGame().GetWorld().GetSurfaceY(goal[0], goal[2]);
		cmd.TacticMove(best, goal, now);

		DCO_TacticPlan p = new DCO_TacticPlan();
		p.m_Obj = obj;
		p.m_eTactic = DCO_ETactic.PURSUIT;
		p.m_fStart = now;
		p.AddGroup(best, 4, goal);
		m_aPursuits.Insert(p);
		Event(string.Format("tactic_phase obj=%1 tactic=PURSUIT phase=start grp=%2 enemies=%3", obj.GetOwner().GetName(), best.GetOwner(), enemies));
		DCO_Perf.Count("pursuit_start");
	}

	protected bool TrackPursuit(AICommander_BaseComponent cmd, DCO_TacticPlan p, DCO_GroupUtilityComponent g, float now, out string why)
	{
		vector objPos = p.m_Obj.GetOwner().GetOrigin();
		vector gPos = g.GetOwner().GetOrigin();
		float leash = p.m_Obj.GetRadius() + DCO_Ctx.TuneForCmd(cmd, "tac.pursuit_leash_m", 600.0);
		if (now - p.m_fStart > DCO_Ctx.TuneForCmd(cmd, "tac.pursuit_max_s", 300.0))
		{
			why = "max_time";
			return true;
		}
		if (vector.DistanceXZ(gPos, objPos) > leash)
		{
			why = "leash";
			return true;
		}
		CMD_ThreatResponseComponent tr = cmd.GetThreatResponseComponent();
		CMD_ThreatEntry best;
		float bestD = float.MAX;
		if (tr)
		{
			foreach (CMD_ThreatEntry t : tr.GetThreats())
			{
				if (!t || vector.DistanceXZ(t.m_vBelievedPos, objPos) > leash)
					continue;
				float d = vector.DistanceXZ(t.m_vBelievedPos, gPos);
				if (d < bestD)
				{
					bestD = d;
					best = t;
				}
			}
		}
		if (!best)
		{
			if (p.m_fArrivedAt < 0)
				p.m_fArrivedAt = now;
			if (now - p.m_fArrivedAt >= AggWait(cmd, "tac.lost_contact_s", 60.0, 30.0))
			{
				why = "lost_contact";
				return true;
			}
			return false;
		}
		p.m_fArrivedAt = -1;
		if (now - p.m_fLastEval < DCO_Ctx.TuneForCmd(cmd, "tac.pursuit_retarget_s", 30.0))
			return false;
		p.m_fLastEval = now;
		vector goal = best.m_vBelievedPos;
		goal[1] = GetGame().GetWorld().GetSurfaceY(goal[0], goal[2]);
		p.m_aGoals[0] = goal;
		cmd.TacticMove(g, goal, now);
		DCO_Perf.Count("pursuit_retarget");
		Event(string.Format("pursuit_retarget obj=%1 grp=%2 enemy_m=%3 from_obj_m=%4 t=%5", p.m_Obj.GetOwner().GetName(), g.GetOwner(), Math.Round(bestD), Math.Round(vector.DistanceXZ(goal, objPos)), Math.Round(now - p.m_fStart)));
		return false;
	}

	protected void UpdatePursuits(AICommander_BaseComponent cmd, float now)
	{
		for (int i = m_aPursuits.Count() - 1; i >= 0; i--)
		{
			DCO_TacticPlan p = m_aPursuits[i];
			DCO_GroupUtilityComponent g = p.FirstWithRole(4);
			if (!g || !p.m_Obj)
			{
				m_aPursuits.Remove(i);
				continue;
			}

			bool arrived = vector.DistanceXZ(g.GetOwner().GetOrigin(), p.m_aGoals[0]) <= 50;
			string why = "timeout";
			if (arrived)
				why = "arrived";
			if (DCO_Ctx.TuneForCmd(cmd, "tac.pursuit_track", 0) > 0)
			{
				if (!TrackPursuit(cmd, p, g, now, why))
					continue;
			}
			else if (!arrived && now - p.m_fStart < AggWait(cmd, "tac.pursuit_consol_s", 180.0, 120.0))
				continue;

			cmd.TacticMove(g, p.m_Obj.GetOwner().GetOrigin(), now);
			Event(string.Format("tactic_phase obj=%1 tactic=PURSUIT phase=consolidate grp=%2 arrived=%3 why=%4 after_s=%5", p.m_Obj.GetOwner().GetName(), g.GetOwner(), arrived, why, Math.Round(now - p.m_fStart)));
			DCO_Perf.Count("pursuit_done");
			m_aPursuits.Remove(i);
		}
	}
}

class DCO_AssaultDrive
{
	protected static ref map<DCO_GroupUtilityComponent, float> s_mUntil;

	static float Start(DCO_GroupUtilityComponent g)
	{
		float dur = DCO_Ctx.TuneG("tactic.drive_s", 90.0);
		if (!g || dur <= 0)
			return 0;
		if (!s_mUntil)
			s_mUntil = new map<DCO_GroupUtilityComponent, float>();
		s_mUntil.Set(g, DCO_Util.NowS() + dur);
		DCO_Perf.Count("assault:drive_start");
		return dur;
	}

	static bool Active(DCO_GroupUtilityComponent g)
	{
		float until;
		if (!g || !s_mUntil || !s_mUntil.Find(g, until))
			return false;
		if (DCO_Util.NowS() < until)
			return true;
		s_mUntil.Remove(g);
		return false;
	}
}

modded class SCR_AIObserveThreatSystemBehavior
{
	protected float m_fDCODriveCheckAt;
	protected bool m_bDCODrive;

	override float CustomEvaluate()
	{
		float prio = super.CustomEvaluate();
		if (prio <= 0 || !m_Utility || !m_Utility.GetAIAgent())
			return prio;
		if ((m_eCurrentSectorFlags & SCR_EAIThreatSectorFlags.CAUSED_DAMAGE) && IsInHighPriorityState())
			return prio;
		float now = GetGame().GetWorld().GetWorldTime();
		if (now >= m_fDCODriveCheckAt)
		{
			m_fDCODriveCheckAt = now + 1000.0;
			m_bDCODrive = false;
			AIGroup grp = m_Utility.GetAIAgent().GetParentGroup();
			if (grp)
			{
				SCR_AIGroupUtilityComponent gu = SCR_AIGroupUtilityComponent.Cast(grp.FindComponent(SCR_AIGroupUtilityComponent));
				if (gu)
					m_bDCODrive = DCO_AssaultDrive.Active(gu.DCO_GetUtil());
			}
		}
		if (!m_bDCODrive)
			return prio;
		DCO_Perf.Count("assault:drive_observe_skip");
		return 0;
	}
}
