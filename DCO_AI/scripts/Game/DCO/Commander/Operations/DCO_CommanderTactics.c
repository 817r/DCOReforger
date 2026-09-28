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
}

class DCO_CommanderTactics
{
	static const float ROAD_SNAP_M = 50;
	static const int ROLE_FIX = 0;
	static const int ROLE_MANEUVER = 1;
	static const int ROLE_BLOCK = 2;
	static const int ROLE_ASSAULT = 3;
	static const int ROLE_RESERVE = 4;
	static const int ROLE_LEAD = 5;
	static const int ALL_TACTICS = 8191;

	protected static const float TICK_S = 5;
	protected static const float MTC_FOLLOW_M = 250;
	protected static const float MTC_TIMEOUT_S = 600;
	protected static const float PROBE_S = 150;
	protected static const float PROBE_LOSS = 0.25;
	protected static const float ANVIL_TIMEOUT_S = 240;
	protected static const float ANVIL_ARRIVE_M = 60;
	protected static const float BLOCK_MIN_M = 150;
	protected static const float BLOCK_MAX_M = 300;
	protected static const float REEVAL_S = 120;
	protected static const float SIEGE_TIMEOUT_S = 900;
	protected static const float BYPASS_COOLDOWN_S = 600;
	protected static const float RAID_COOLDOWN_S = 300;
	protected static const float FAIL_MEMORY_S = 900;
	protected static const float BOUND_STEP_M = 125;
	protected static const float BOUND_ARRIVE_M = 30;
	protected static const float BOUND_FINAL_M = 150;
	protected static const float PURSUIT_M = 400;
	protected static const float PURSUIT_S = 180;
	protected static const float INFIL_MAX_UNITS = 6;
	protected static const float HARASS_S = 120;
	protected static const float ENEMY_WATCH_M = 300;
	protected static const int HARASS_SHELLS = 3;

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
		bool ready = anvil && vector.DistanceXZ(anvil.GetOwner().GetOrigin(), p.m_vBlock) <= ANVIL_ARRIVE_M;
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

		if (now - p.m_fStart > ANVIL_TIMEOUT_S || !hasAnvil)
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

	DCO_TacticPlan Begin(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, float now)
	{
		m_mPlans.Remove(obj);

		DCO_TacticSituation s = Assess(cmd, obj, now);

		array<float> scores = {};
		for (int t = 0; t <= DCO_ETactic.DELAY; t++)
			scores.Insert(Score(cmd, obj, t, s));

		int best = -1;
		float bestScore = 0;
		for (int i = 0; i < scores.Count(); i++)
		{
			if (scores[i] > bestScore)
			{
				bestScore = scores[i];
				best = i;
			}
		}

		if (best < 0)
		{
			if (IsAllowed(cmd, DCO_ETactic.FIRE_AND_MANEUVER))
				best = DCO_ETactic.FIRE_AND_MANEUVER;
			else
				return null;
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
		m_mPlans.Set(obj, p);

		Event(string.Format("tactic_chosen cmd=%1 obj=%2 tactic=%3 ratio=%4 groups=%5 conf=%6 night=%7 urban=%8 forest=%9",
			cmd.GetCommanderUID(), obj.GetOwner().GetName(), Name(best), s.m_fRatio.ToString(-1, 2), s.m_iGroups,
			s.m_fConfidence.ToString(-1, 2), s.m_bNight, s.m_bUrban, s.m_bForest) + string.Format(" retreat=%1 dir=%2", s.m_sRetreatSrc, s.m_vRetreat));
		Event(string.Format("tactic_scores obj=%1 top=%2", obj.GetOwner().GetName(), TopScores(scores)));

		Start(cmd, p, now);
		return p;
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
			if (g.GetUnitCount() <= INFIL_MAX_UNITS)
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
			enemy += DCO_Strength.W_TANK;
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
			float nearSq = (obj.GetRadius() + ENEMY_WATCH_M) * (obj.GetRadius() + ENEMY_WATCH_M);
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
			if (road && dist <= ROAD_SNAP_M)
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
		if (!m_mFailedAt.Find(obj, at) || DCO_CommanderOps.Now() - at > FAIL_MEMORY_S)
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

	protected DCO_GroupUtilityComponent PickGroup(AICommander_BaseComponent cmd, vector pos, bool small, bool fireSupport)
	{
		DCO_GroupUtilityComponent best;
		float bestD = float.MAX;
		foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
		{
			if (!IsUsable(cmd, g))
				continue;
			if (small && g.GetUnitCount() > INFIL_MAX_UNITS)
				continue;
			if (fireSupport && !g.HasMG() && !g.IsArmor())
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

	static int RoleBit(DCO_ETactic t, int role)
	{
		switch (role)
		{
			case ROLE_FIX:
				return DCO_PlayerTasking.ROLE_BIT_FIX;
			case ROLE_BLOCK:
				return DCO_PlayerTasking.ROLE_BIT_BLOCK;
			case ROLE_MANEUVER:
				if (t == DCO_ETactic.INFILTRATION)
					return DCO_PlayerTasking.ROLE_BIT_RECON;
				break;
		}
		return DCO_PlayerTasking.ROLE_BIT_ASSAULT;
	}

	static bool IsDecisive(int role)
	{
		return role == ROLE_ASSAULT || role == ROLE_MANEUVER || role == ROLE_BLOCK || role == ROLE_LEAD;
	}

	static string RoleName(int role)
	{
		switch (role)
		{
			case ROLE_FIX:		return "FIX";
			case ROLE_MANEUVER:	return "MANEUVER";
			case ROLE_BLOCK:	return "BLOCK";
			case ROLE_ASSAULT:	return "ASSAULT";
			case ROLE_RESERVE:	return "RESERVE";
			case ROLE_LEAD:		return "LEAD";
		}
		return "NONE";
	}

	static float RoleScore(int units, bool mg, bool vehicle, float dist, int role, bool small, bool fireSupport)
	{
		if (small && units > INFIL_MAX_UNITS)
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

		DCO_GroupUtilityComponent ai = PickGroup(cmd, pos, small, fireSupport);
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
				g = PickGroup(cmd, pos, false, false);
				if (g && cmd.TacticCommit(g, obj, DCO_EGroupTask.DEFEND, CoveredVia(g.GetOwner().GetOrigin(), pos), now))
				{
					cmd.TacticAppendMove(g, pos, now);
					p.AddGroup(g, ROLE_BLOCK, pos);
				}
				else
					g = null;
				break;
			}
			case ROLE_MANEUVER:
			{
				g = PickGroup(cmd, objPos, true, false);
				if (g && cmd.TacticCommit(g, obj, DCO_EGroupTask.RECON, CoveredVia(g.GetOwner().GetOrigin(), pos), now))
				{
					cmd.TacticAppendMove(g, pos, now);
					p.AddGroup(g, ROLE_MANEUVER, pos);
				}
				else
					g = null;
				break;
			}
			default:
			{
				g = PickGroup(cmd, objPos, false, false);
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
				DCO_GroupUtilityComponent lead = PickGroup(cmd, objPos, false, false);
				if (lead && cmd.TacticCommit(lead, obj, DCO_EGroupTask.ATTACK, objPos, now))
					p.AddGroup(lead, ROLE_LEAD, objPos);
				DCO_GroupUtilityComponent follow = PickGroup(cmd, objPos, false, false);
				if (follow && lead)
				{
					vector fpos = FollowPos(lead.GetOwner().GetOrigin(), objPos);
					if (cmd.TacticCommit(follow, obj, DCO_EGroupTask.ATTACK, fpos, now))
						p.AddGroup(follow, ROLE_RESERVE, fpos);
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
					DCO_GroupUtilityComponent g = PickGroup(cmd, objPos, false, false);
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
				DCO_GroupUtilityComponent anvil = PickGroup(cmd, p.m_vBlock, false, false);
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
				if (TryPlayerRole(cmd, p, ROLE_MANEUVER, p.m_vTarget, true, false, now))
					break;
				DCO_GroupUtilityComponent inf = PickGroup(cmd, objPos, true, false);
				if (inf && cmd.TacticCommit(inf, obj, DCO_EGroupTask.RECON, CoveredVia(inf.GetOwner().GetOrigin(), p.m_vTarget), now))
				{
					cmd.TacticAppendMove(inf, p.m_vTarget, now);
					p.AddGroup(inf, ROLE_MANEUVER, p.m_vTarget);
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
						DCO_GroupUtilityComponent blocker = PickGroup(cmd, p.m_vBlock, false, false);
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
				cmd.SetObjectiveCooldown(obj, BYPASS_COOLDOWN_S);
				p.m_iPhase = 1;
				break;
			}
		}
	}

	protected void SendFix(AICommander_BaseComponent cmd, DCO_TacticPlan p, int count, float now, bool allowPlayers = true)
	{
		vector objPos = p.m_Obj.GetOwner().GetOrigin();
		for (int i = 0; i < count; i++)
		{
			if (allowPlayers && TryPlayerRole(cmd, p, ROLE_FIX, objPos, false, true, now))
				continue;
			DCO_GroupUtilityComponent g = PickGroup(cmd, objPos, false, true);
			if (!g)
				g = PickGroup(cmd, objPos, false, false);
			if (!g)
				return;

			vector pos = cmd.GetOps().FindOverwatch(p.m_Obj, g.GetOwner().GetOrigin());
			if (pos == vector.Zero)
				pos = cmd.TacticStagingPos(p.m_Obj);
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
		if (!arty || !arty.HasRegisteredUnits() || now - p.m_fLastFire < HARASS_S)
			return;

		p.m_fLastFire = now;
		CMD_FireMissionRequest req = new CMD_FireMissionRequest(p.m_Obj.GetOwner().GetOrigin(), SCR_EAIArtilleryAmmoType.HIGH_EXPLOSIVE, now, HARASS_SHELLS);
		arty.ApplyTier(req, "area", p.m_Obj.GetRadius(), "tactic_" + Name(p.m_eTactic));
		if (arty.HasFriendlyNearRequest(req, now))
		{
			CMD_ArtillerySupport.LogDenied(req, "friendly");
			return;
		}
		arty.RequestShellImpact(req, now, HARASS_SHELLS);
	}

	protected vector FollowPos(vector leadPos, vector objPos)
	{
		vector back = leadPos - objPos;
		back[1] = 0;
		if (back.Length() < 1)
			return leadPos;
		back.Normalize();
		vector pos = leadPos + back * MTC_FOLLOW_M;
		pos[1] = GetGame().GetWorld().GetSurfaceY(pos[0], pos[2]);
		return pos;
	}

	protected vector BlockPos(CMD_AICommanderObjectiveComponent obj, vector dir)
	{
		vector objPos = obj.GetOwner().GetOrigin();
		float dist = obj.GetRadius() + (BLOCK_MIN_M + BLOCK_MAX_M) * 0.5;
		vector pos = objPos + dir * dist;
		vector cover = DCO_TerrainCache.QueryBest(DCO_ETerrainFlag.FOREST_EDGE | DCO_ETerrainFlag.HILLTOP | DCO_ETerrainFlag.OVERWATCH, pos, (BLOCK_MAX_M - BLOCK_MIN_M) * 0.5 + 50, string.Empty, pos);
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
		if (m_fTimer < TICK_S)
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
		if (obj.IsCapturedBy(fk) || st == CMD_EObjectiveState.COMPLETED || st == CMD_EObjectiveState.FAILED)
			return true;

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

	protected bool UpdateMTC(AICommander_BaseComponent cmd, DCO_TacticPlan p, float now)
	{
		DCO_GroupUtilityComponent lead = p.FirstWithRole(ROLE_LEAD);
		DCO_ObjectiveIntel intel = cmd.GetOps().GetIntel(p.m_Obj);
		bool contact = (lead && lead.HasState(DCO_EGroupState.IN_CONTACT)) || (intel.m_bEverSeen && intel.m_fConfidence >= 0.3);
		if (contact || !lead || now - p.m_fStart > MTC_TIMEOUT_S)
		{
			string why = "contact";
			if (!lead)
				why = "lead_lost";
			else if (!contact)
				why = "timeout";
			Event(string.Format("tactic_phase obj=%1 tactic=%2 phase=rechoose reason=%3", p.m_Obj.GetOwner().GetName(), Name(p.m_eTactic), why));
			MarkFailed(p.m_Obj, p.m_eTactic);
			cmd.TacticRestart(p.m_Obj);
			return true;
		}

		DCO_GroupUtilityComponent follow = p.FirstWithRole(ROLE_RESERVE);
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
			foreach (int i, DCO_GroupUtilityComponent g : p.m_aGroups)
			{
				if (!g)
					continue;
				units += g.GetUnitCount();
				start += p.m_aStartUnits[i];
			}

			bool losses = start > 0 && units <= start * (1 - PROBE_LOSS);
			if (now - p.m_fStart < PROBE_S && !losses && (units > 0 || !p.m_mPlayerRoles.IsEmpty()))
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
			Event(string.Format("tactic_phase obj=%1 tactic=%2 phase=withdraw reason=%3 units=%4/%5", p.m_Obj.GetOwner().GetName(), Name(p.m_eTactic), why, units, start));
			return false;
		}

		if (now - p.m_fPhaseStart < 60)
			return false;

		if (rechoose)
		{
			MarkFailed(p.m_Obj, p.m_eTactic);
			Event(string.Format("tactic_phase obj=%1 tactic=%2 phase=rechoose reason=intel", p.m_Obj.GetOwner().GetName(), Name(p.m_eTactic)));
			cmd.TacticRestart(p.m_Obj);
			return true;
		}

		foreach (DCO_GroupUtilityComponent g3 : p.m_aGroups)
		{
			if (g3)
				cmd.TacticRelease(g3, p.m_Obj);
		}
		cmd.TacticAbort(p.m_Obj, now);
		cmd.SetObjectiveCooldown(p.m_Obj, RAID_COOLDOWN_S);
		Event(string.Format("tactic_result obj=%1 tactic=%2 result=raid_done", p.m_Obj.GetOwner().GetName(), Name(p.m_eTactic)));
		return true;
	}

	protected bool UpdateStandoff(AICommander_BaseComponent cmd, DCO_TacticPlan p, float now, bool siege)
	{
		Harass(cmd, p, now);
		if (now - p.m_fLastEval < REEVAL_S)
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

		if (current >= s.m_fMinRatio)
		{
			Event(string.Format("tactic_phase obj=%1 tactic=%2 phase=escalate ratio=%3", p.m_Obj.GetOwner().GetName(), Name(p.m_eTactic), current.ToString(-1, 2)));
			MarkFailed(p.m_Obj, p.m_eTactic);
			cmd.TacticRestart(p.m_Obj);
			return true;
		}

		if (now - p.m_fStart < SIEGE_TIMEOUT_S)
			return false;

		foreach (DCO_GroupUtilityComponent g2 : p.m_aGroups)
		{
			if (g2)
				cmd.TacticRelease(g2, p.m_Obj);
		}
		MarkFailed(p.m_Obj, p.m_eTactic);
		cmd.TacticAbort(p.m_Obj, now);
		Event(string.Format("tactic_result obj=%1 tactic=%2 result=timeout ratio=%3", p.m_Obj.GetOwner().GetName(), Name(p.m_eTactic), current.ToString(-1, 2)));
		return true;
	}

	protected bool UpdateBypass(AICommander_BaseComponent cmd, DCO_TacticPlan p, float now)
	{
		if (now - p.m_fStart < BYPASS_COOLDOWN_S)
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
		float finalDist = obj.GetRadius() + BOUND_FINAL_M;

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
		DCO_GroupUtilityComponent mover = p.FirstWithRole(ROLE_MANEUVER);
		if (mover && groups.Contains(mover))
		{
			int mi = p.m_aGroups.Find(mover);
			if (vector.DistanceXZ(mover.GetOwner().GetOrigin(), p.m_aGoals[mi]) > BOUND_ARRIVE_M && now - p.m_fPhaseStart < 90)
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
		float step = Math.Min(BOUND_STEP_M, Math.Max(rearDist - obj.GetRadius(), 20));
		vector goal = rear.GetOwner().GetOrigin() + dir * step;
		goal[1] = GetGame().GetWorld().GetSurfaceY(goal[0], goal[2]);

		int ri = p.m_aGroups.Find(rear);
		if (ri < 0)
		{
			p.AddGroup(rear, ROLE_MANEUVER, goal);
		}
		else
		{
			p.m_aRoles[ri] = ROLE_MANEUVER;
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

		DCO_GroupUtilityComponent inf = p.FirstWithRole(ROLE_MANEUVER);
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
			tactic = Name(p.m_eTactic);
		m_mPlans.Remove(obj);
		m_mFailedMask.Remove(obj);
		m_mFailedAt.Remove(obj);
		Event(string.Format("tactic_result obj=%1 tactic=%2 result=captured", obj.GetOwner().GetName(), tactic));

		if (IsAllowed(cmd, DCO_ETactic.PURSUIT))
			TryPursuit(cmd, obj, now);
	}

	void OnAborted(CMD_AICommanderObjectiveComponent obj, string reason)
	{
		DCO_TacticPlan p = m_mPlans.Get(obj);
		if (!p)
			return;
		MarkFailed(obj, p.m_eTactic);
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
			if (g.GetUnitCount() > bestUnits && !g.HasState(DCO_EGroupState.IN_CONTACT))
			{
				bestUnits = g.GetUnitCount();
				best = g;
			}
		}
		if (!best || friendly < enemies * 1.5)
			return;

		away[1] = 0;
		away.Normalize();
		vector goal = objPos + away * (obj.GetRadius() + PURSUIT_M);
		goal[1] = GetGame().GetWorld().GetSurfaceY(goal[0], goal[2]);
		cmd.TacticMove(best, goal, now);

		DCO_TacticPlan p = new DCO_TacticPlan();
		p.m_Obj = obj;
		p.m_eTactic = DCO_ETactic.PURSUIT;
		p.m_fStart = now;
		p.AddGroup(best, ROLE_RESERVE, goal);
		m_aPursuits.Insert(p);
		Event(string.Format("tactic_phase obj=%1 tactic=PURSUIT phase=start grp=%2 enemies=%3", obj.GetOwner().GetName(), best.GetOwner(), enemies));
	}

	protected void UpdatePursuits(AICommander_BaseComponent cmd, float now)
	{
		for (int i = m_aPursuits.Count() - 1; i >= 0; i--)
		{
			DCO_TacticPlan p = m_aPursuits[i];
			DCO_GroupUtilityComponent g = p.FirstWithRole(ROLE_RESERVE);
			if (!g || !p.m_Obj)
			{
				m_aPursuits.Remove(i);
				continue;
			}

			bool arrived = vector.DistanceXZ(g.GetOwner().GetOrigin(), p.m_aGoals[0]) <= 50;
			if (!arrived && now - p.m_fStart < PURSUIT_S)
				continue;

			cmd.TacticMove(g, p.m_Obj.GetOwner().GetOrigin(), now);
			Event(string.Format("tactic_phase obj=%1 tactic=PURSUIT phase=consolidate grp=%2", p.m_Obj.GetOwner().GetName(), g.GetOwner()));
			m_aPursuits.Remove(i);
		}
	}
}
