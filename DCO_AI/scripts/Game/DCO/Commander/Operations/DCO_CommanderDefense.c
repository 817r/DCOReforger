class DCO_Ambush
{
	int m_iId;
	int m_iKind;
	vector m_vKill;
	vector m_vPos;
	vector m_vRally;
	bool m_bVehicle;
	string m_sReason;
	ref array<DCO_GroupUtilityComponent> m_aGroups = {};
	int m_iState;
	float m_fStateTime;
	float m_fLastTarget;
	float m_fOpenAt;
	int m_iStrength;
	int m_iMaxTargets;
}

class DCO_OPPost
{
	DCO_GroupUtilityComponent m_Group;
	DCO_GroupUtilityComponent m_Parent;
	vector m_vPos;
	vector m_vWatch;
	string m_sPurpose;
	float m_fStart;
	int m_iStrength;
	bool m_bOrdered;
}

class DCO_RoadTrack
{
	vector m_vPos;
	vector m_vDir;
	int m_iCount;
	float m_fLast;
	bool m_bUsed;
}

class DCO_CBTrack
{
	IEntity m_Source;
	vector m_vTruePos;
	int m_iHits;
	float m_fLast;
	float m_fLastFire;
	float m_fHunterStart;
	DCO_GroupUtilityComponent m_Hunter;
	bool m_bHunted;
	ref CMD_ThreatEntry m_Entry;
}

class DCO_CounterAttack
{
	CMD_AICommanderObjectiveComponent m_Obj;
	float m_fDeadline;
}

class DCO_CommanderDefense
{
	protected static const float TICK_S = 2;
	protected static const float PLAN_S = 60;
	protected static const float PLAN_COOLDOWN_S = 180;
	protected static const float SITE_MIN = 60;
	protected static const float SITE_MAX = 180;
	protected static const float SITE_RING = 110;
	protected static const float RALLY_DIST = 150;
	protected static const float OWN_OBJ_CLEAR = 100;
	protected static const float SET_DIST = 40;
	protected static const float MOVE_MAX_S = 300;
	protected static const float TARGET_FRESH_S = 5;
	protected static const float TARGET_GONE_S = 15;
	protected static const float WITHDRAW_MAX_S = 150;
	protected static const float OPEN_DELAY_S = 3;
	protected static const float MSR_WINDOW_S = 1200;
	protected static const float MSR_PASS_GAP_S = 90;
	protected static const float MSR_MATCH_DIST = 150;
	protected static const float MSR_MIN_SPEED = 4;
	protected static const int MSR_SIGHTINGS = 2;
	protected static const float DANGER_TTL_S = 1200;
	protected static const float DANGER_RADIUS = 150;
	protected static const float CB_NOTIFY_RADIUS = 150;
	protected static const float CB_TTL_S = 600;
	protected static const float CB_FIRE_COOLDOWN_S = 120;
	protected static const int CB_SHELLS = 4;
	protected static const float HUNTER_TIMEOUT_S = 600;
	protected static const int OP_TEAM = 3;
	protected static const float COUNTER_RANGE = 1200;
	protected static const float COUNTER_MIN_ENEMY = 4;

	protected ref array<ref DCO_Ambush> m_aAmbush = {};
	protected ref array<ref DCO_OPPost> m_aOP = {};
	protected ref array<ref DCO_RoadTrack> m_aRoads = {};
	protected ref array<ref DCO_CBTrack> m_aCB = {};
	protected ref array<ref DCO_CounterAttack> m_aCounter = {};
	protected ref array<ref CMD_ThreatEntry> m_aDanger = {};
	protected static int s_iAmbushId;
	protected float m_fTimer;
	protected float m_fPlanTimer;
	protected float m_fNextAmbush;

	static void Event(string line)
	{
		Print("[DCO_Defense] " + line);
		DCO_BenchmarkLoggerComponent.Event(line);
	}

	void Tick(AICommander_BaseComponent cmd, float timeSlice)
	{
		m_fTimer += timeSlice;
		if (m_fTimer < TICK_S)
			return;
		m_fTimer = 0;

		float now = DCO_CommanderOps.Now();
		UpdateAmbushes(cmd, now);
		UpdateOPs(cmd, now);
		UpdateCounterAttacks(cmd, now);
		UpdateCB(cmd, now);

		m_fPlanTimer += TICK_S;
		if (m_fPlanTimer < PLAN_S)
			return;
		m_fPlanTimer = 0;
		PurgeDangers(now);
		PlanMSR(cmd, now);
		PlanScreen(cmd, now);
		PlanOPs(cmd, now);
	}

	protected bool CanStartAmbush(AICommander_BaseComponent cmd, float now)
	{
		return cmd.IsAmbushEnabled() && now >= m_fNextAmbush && m_aAmbush.Count() < cmd.GetMaxAmbushes();
	}

	void RequestApproachAmbush(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, vector threatPos, bool vehicle, float now)
	{
		if (!obj || !CanStartAmbush(cmd, now))
			return;
		if (Math.RandomFloat01() > 0.4 + 0.3 * cmd.GetAggression() + 0.3 * cmd.GetRiskTaking())
			return;

		vector objPos = obj.GetOwner().GetOrigin();
		vector toObj = objPos - threatPos;
		toObj[1] = 0;
		float d = toObj.Length();
		float minKill = obj.GetRadius() + 150;
		if (d < minKill + 100)
			return;
		toObj = toObj * (1.0 / d);

		vector kill = SnapKillZone(objPos - toObj * Math.Max(minKill, d * 0.5), 80);
		StartAmbush(cmd, 0, kill, toObj, vehicle, "threat_projection", obj, now);
	}

	protected void PlanScreen(AICommander_BaseComponent cmd, float now)
	{
		if (!CanStartAmbush(cmd, now) || CountReserves(cmd) < 2)
			return;

		float w = 0.2 + 0.4 * cmd.GetAggression() + 0.4 * cmd.GetRiskTaking();
		if (cmd.GetCommanderMode() == CMD_ECommanderMode.OFFENSIVE)
			w *= 0.5;
		if (Math.RandomFloat01() > w * 0.5)
			return;

		CMD_AICommanderObjectiveComponent front;
		vector enemyDir;
		if (!FrontObjective(cmd, front, enemyDir))
			return;

		vector objPos = front.GetOwner().GetOrigin();
		vector kill = SnapKillZone(objPos + enemyDir * (front.GetRadius() + 250), 120);
		StartAmbush(cmd, 0, kill, -enemyDir, false, "approach_screen", front, now);
	}

	protected void PlanMSR(AICommander_BaseComponent cmd, float now)
	{
		if (!cmd.IsMSRAmbushEnabled())
			return;
		foreach (DCO_RoadTrack t : m_aRoads)
		{
			if (t.m_bUsed || t.m_iCount < MSR_SIGHTINGS || now - t.m_fLast > MSR_WINDOW_S)
				continue;
			if (!CanStartAmbush(cmd, now))
				return;
			if (Math.RandomFloat01() > 0.3 + 0.35 * cmd.GetAggression() + 0.35 * cmd.GetRiskTaking())
				continue;
			t.m_bUsed = true;
			StartAmbush(cmd, 1, SnapKillZone(t.m_vPos, 40), t.m_vDir, true, "msr_sightings_" + t.m_iCount, null, now);
		}
	}

	protected void StartAmbush(AICommander_BaseComponent cmd, int kind, vector kill, vector approachDir, bool vehicle, string reason, CMD_AICommanderObjectiveComponent obj, float now)
	{
		m_fNextAmbush = now + PLAN_COOLDOWN_S;
		string objName = "none";
		if (obj)
			objName = obj.GetOwner().GetName();

		vector site;
		if (!FindSite(cmd, kill, approachDir, site))
		{
			Event(string.Format("def_ambush_skip kind=%1 reason=%2 obj=%3 why=no_site kill=%4", kind, reason, objName, kill));
			return;
		}

		DCO_GroupUtilityComponent g1 = PickReserve(cmd, site, vehicle, !vehicle, null, 1500);
		if (!g1)
		{
			Event(string.Format("def_ambush_skip kind=%1 reason=%2 obj=%3 why=no_group need_at=%4", kind, reason, objName, vehicle));
			return;
		}

		DCO_Ambush a = new DCO_Ambush();
		a.m_iId = ++s_iAmbushId;
		a.m_iKind = kind;
		a.m_vKill = kill;
		a.m_vPos = site;
		a.m_bVehicle = vehicle;
		a.m_sReason = reason;
		a.m_fStateTime = now;
		a.m_aGroups.Insert(g1);

		vector away = site - kill;
		away[1] = 0;
		away.Normalize();
		a.m_vRally = Surface(site + away * RALLY_DIST);
		if (IsWet(a.m_vRally))
			a.m_vRally = site;

		MoveIntoPosition(cmd, g1, site, kill, now);

		if (vehicle || g1.GetUnitCount() < 6)
		{
			DCO_GroupUtilityComponent g2 = PickReserve(cmd, site, false, true, g1, 1000);
			if (g2)
			{
				a.m_aGroups.Insert(g2);
				MoveIntoPosition(cmd, g2, SecondSite(kill, site), kill, now);
			}
		}

		m_aAmbush.Insert(a);
		Event(string.Format("def_ambush_plan id=%1 kind=%2 reason=%3 obj=%4 kill=%5 site=%6 site_dist=%7 vehicle=%8 groups=%9",
			a.m_iId, kind, reason, objName, kill, site, Math.Round(vector.DistanceXZ(site, kill)), vehicle, a.m_aGroups.Count()));
	}

	protected bool FindSite(AICommander_BaseComponent cmd, vector kill, vector approachDir, out vector site)
	{
		array<vector> cands = {};
		array<float> bonus = {};
		array<DCO_TerrainPoint> pts = {};
		DCO_TerrainCache.Query(DCO_ETerrainFlag.FOREST_EDGE | DCO_ETerrainFlag.HILLTOP | DCO_ETerrainFlag.OVERWATCH, kill, SITE_MAX, pts);
		foreach (DCO_TerrainPoint p : pts)
		{
			float b = p.m_fRelElev * 0.1;
			if (p.m_iFlags & DCO_ETerrainFlag.FOREST_EDGE)
				b += 2;
			cands.Insert(p.m_vPos);
			bonus.Insert(b);
		}
		for (int i = 0; i < 12; i++)
		{
			float ang = i * 30 * Math.DEG2RAD;
			cands.Insert(kill + Vector(Math.Cos(ang), 0, Math.Sin(ang)) * SITE_RING);
			bonus.Insert(0);
		}

		bool found;
		float best = -float.MAX;
		foreach (int k, vector raw : cands)
		{
			vector c = Surface(raw);
			float d = vector.DistanceXZ(c, kill);
			if (d < SITE_MIN || d > SITE_MAX)
				continue;
			vector rel = c - kill;
			rel[1] = 0;
			rel = rel * (1.0 / d);
			float along = rel[0] * approachDir[0] + rel[2] * approachDir[2];
			if (along < -0.3)
				continue;
			if (IsWet(c) || NearOwnObjective(cmd, c) || NearThreat(cmd, c, 100) || !HasLOS(c, kill))
				continue;

			float s = bonus[k] + (1 - Math.AbsFloat(along)) * 2 - Math.AbsFloat(d - SITE_RING) / 60;
			if (s > best)
			{
				best = s;
				site = c;
				found = true;
			}
		}
		return found;
	}

	protected vector SecondSite(vector kill, vector site)
	{
		array<float> rots = {70, -70};
		foreach (float rot : rots)
		{
			vector c = Surface(kill + DCO_CommanderOps.RotateXZ(site - kill, rot));
			if (!IsWet(c) && HasLOS(c, kill))
				return c;
		}
		vector side = DCO_CommanderOps.RotateXZ(site - kill, 90);
		side.Normalize();
		return Surface(site + side * 25);
	}

	protected void MoveIntoPosition(AICommander_BaseComponent cmd, DCO_GroupUtilityComponent g, vector pos, vector look, float now)
	{
		g.CompleteAllWaypoints();
		g.SetGroupObjective(null);
		g.SetTask(DCO_EGroupTask.DEFEND);
		g.DCO_SetHold(now + 3600);
		SetFire(g, false);

		array<SCR_AIWaypoint> wps = {};
		SCR_AIWaypoint walk = cmd.SpawnMoveWP(pos, EMovementType.WALK);
		if (walk)
			wps.Insert(walk);
		vector face = look - pos;
		DCO_GarrisonWaypoint gw = cmd.SpawnGarrisonWP(pos, Math.Atan2(face[2], face[0]), 30, true);
		if (gw)
			wps.Insert(gw);
		g.MoveToRoute(wps, now);
	}

	protected void UpdateAmbushes(AICommander_BaseComponent cmd, float now)
	{
		for (int i = m_aAmbush.Count() - 1; i >= 0; i--)
		{
			DCO_Ambush a = m_aAmbush[i];
			for (int k = a.m_aGroups.Count() - 1; k >= 0; k--)
			{
				DCO_GroupUtilityComponent g = a.m_aGroups[k];
				if (!g || g.GetUnitCount() == 0 || (a.m_iState != 3 && (g.GetTask() != DCO_EGroupTask.DEFEND || g.GetGroupObjective())))
					a.m_aGroups.Remove(k);
			}
			if (a.m_aGroups.IsEmpty())
			{
				Event(string.Format("def_ambush_end id=%1 result=lost state=%2 targets=%3", a.m_iId, a.m_iState, a.m_iMaxTargets));
				m_aAmbush.Remove(i);
				continue;
			}

			bool vehicleSeen;
			int seen = CountTargets(a, a.m_vKill, cmd.GetAmbushKillRadius(), vehicleSeen);
			a.m_iMaxTargets = Math.Max(a.m_iMaxTargets, seen);
			bool hurt = Strength(a) < a.m_iStrength || AnySuppressed(a);

			switch (a.m_iState)
			{
				case 0:
				{
					if (AllNear(a, a.m_vPos, SET_DIST) || now - a.m_fStateTime > MOVE_MAX_S)
					{
						float moveS = now - a.m_fStateTime;
						a.m_iState = 1;
						a.m_fStateTime = now;
						a.m_iStrength = Strength(a);
						Event(string.Format("def_ambush_set id=%1 strength=%2 move_s=%3", a.m_iId, a.m_iStrength, Math.Round(moveS)));
					}
					else if (AnySuppressed(a))
						Withdraw(cmd, a, "discovered_moving", now);
					break;
				}
				case 1:
				{
					bool nearKill;
					int around = CountTargets(a, a.m_vKill, cmd.GetAmbushKillRadius() * 2, nearKill);
					if (seen > 0 && (!a.m_bVehicle || vehicleSeen))
						Spring(a, "target_in_killzone", seen, vehicleSeen, now);
					else if (hurt && around > 0)
						Spring(a, "discovered", around, nearKill, now);
					else if (hurt)
						Withdraw(cmd, a, "discovered_early", now);
					else if (now - a.m_fStateTime > cmd.GetAmbushMaxWait())
						Withdraw(cmd, a, "no_contact", now);
					break;
				}
				case 2:
				{
					if (now >= a.m_fOpenAt)
					{
						foreach (DCO_GroupUtilityComponent og : a.m_aGroups)
							SetFire(og, true);
					}
					if (seen > 0)
						a.m_fLastTarget = now;

					bool dummy;
					int enemies = CountTargets(a, a.m_vPos, 300, dummy);
					if (now - a.m_fStateTime > cmd.GetAmbushFireTime())
						Withdraw(cmd, a, "fire_time", now);
					else if (now - a.m_fLastTarget > TARGET_GONE_S)
						Withdraw(cmd, a, "target_gone", now);
					else if (enemies > Strength(a) * 1.5)
						Withdraw(cmd, a, "enemy_stronger", now);
					break;
				}
				case 3:
				{
					if (AllNear(a, a.m_vRally, SET_DIST) || now - a.m_fStateTime > WITHDRAW_MAX_S)
					{
						foreach (DCO_GroupUtilityComponent rg : a.m_aGroups)
						{
							rg.SetRetreating(false);
							rg.DCO_SetHold(0);
						}
						Event(string.Format("def_ambush_end id=%1 result=done kind=%2 max_targets=%3 casualties=%4", a.m_iId, a.m_iKind, a.m_iMaxTargets, Math.Max(a.m_iStrength - Strength(a), 0)));
						m_aAmbush.Remove(i);
					}
					break;
				}
			}
		}
	}

	protected void Spring(DCO_Ambush a, string trigger, int targets, bool vehicle, float now)
	{
		a.m_iState = 2;
		a.m_fStateTime = now;
		a.m_fLastTarget = now;
		a.m_fOpenAt = now + OPEN_DELAY_S;

		DCO_GroupUtilityComponent opener;
		foreach (DCO_GroupUtilityComponent g : a.m_aGroups)
		{
			if ((vehicle && g.HasAT()) || (!vehicle && g.HasMG()))
			{
				opener = g;
				break;
			}
		}
		if (!opener)
			a.m_fOpenAt = now;
		else
			SetFire(opener, true);

		string openerName = "all";
		if (opener)
			openerName = opener.GetOwner().GetName();
		Event(string.Format("def_ambush_spring id=%1 trigger=%2 targets=%3 vehicle=%4 opener=%5", a.m_iId, trigger, targets, vehicle, openerName));
	}

	protected void Withdraw(AICommander_BaseComponent cmd, DCO_Ambush a, string why, float now)
	{
		a.m_iState = 3;
		a.m_fStateTime = now;
		foreach (DCO_GroupUtilityComponent g : a.m_aGroups)
		{
			g.CompleteAllWaypoints();
			g.SetRetreating(true);
			SCR_AIWaypoint wp = cmd.SpawnMoveWP(a.m_vRally);
			if (wp)
				g.MoveTo(wp, now);
		}
		Event(string.Format("def_ambush_withdraw id=%1 why=%2 strength=%3/%4 max_targets=%5", a.m_iId, why, Strength(a), a.m_iStrength, a.m_iMaxTargets));
	}

	protected int CountTargets(DCO_Ambush a, vector center, float radius, out bool vehicleSeen)
	{
		vehicleSeen = false;
		PerceptionManager pm = GetGame().GetPerceptionManager();
		if (!pm)
			return 0;
		float pmNow = pm.GetTime();
		int most;
		foreach (DCO_GroupUtilityComponent g : a.m_aGroups)
		{
			SCR_AIGroupUtilityComponent u = g.GetGroupUtilityComponent();
			if (!u || !u.GetPercGroupComp())
				continue;
			int n = 0;
			foreach (SCR_AITargetInfo t : u.GetPercGroupComp().m_aTargets)
			{
				if (!t || pmNow - t.m_fTimestamp > TARGET_FRESH_S)
					continue;
				if (t.m_eCategory != EAITargetInfoCategory.DETECTED && t.m_eCategory != EAITargetInfoCategory.IDENTIFIED)
					continue;
				if (vector.DistanceXZ(t.m_vWorldPos, center) > radius)
					continue;
				n++;
				if (t.m_Entity && (Vehicle.Cast(t.m_Entity) || DCO_VehicleCombat.GetVehicle(t.m_Entity)))
					vehicleSeen = true;
			}
			most = Math.Max(most, n);
		}
		return most;
	}

	protected static int Strength(DCO_Ambush a)
	{
		int n;
		foreach (DCO_GroupUtilityComponent g : a.m_aGroups)
			n += g.GetUnitCount();
		return n;
	}

	protected static bool AnySuppressed(DCO_Ambush a)
	{
		foreach (DCO_GroupUtilityComponent g : a.m_aGroups)
		{
			if (g.HasState(DCO_EGroupState.SUPPRESSED))
				return true;
		}
		return false;
	}

	protected static bool AllNear(DCO_Ambush a, vector pos, float dist)
	{
		foreach (DCO_GroupUtilityComponent g : a.m_aGroups)
		{
			if (vector.DistanceXZ(g.GetOwner().GetOrigin(), pos) > dist + 30)
				return false;
		}
		return true;
	}

	protected static void SetFire(DCO_GroupUtilityComponent g, bool free)
	{
		SCR_AIGroupUtilityComponent u = g.GetGroupUtilityComponent();
		if (!u)
			return;
		if (free)
			u.SetCombatMode(EAIGroupCombatMode.FIRE_AT_WILL);
		else
			u.SetCombatMode(EAIGroupCombatMode.HOLD_FIRE);
	}

	void OnContactReport(AICommander_BaseComponent cmd, CMD_ContactReport report, DCO_GroupUtilityComponent grp)
	{
		float now = report.m_fInfoTime;
		vector vel = report.m_vVelocity;
		vel[1] = 0;
		if (vel.Length() >= MSR_MIN_SPEED)
			TrackRoadSighting(report.m_vPosition, vel, now);

		if (grp && grp.GetOwner() && grp.HasState(DCO_EGroupState.SUPPRESSED)
			&& (grp.HasState(DCO_EGroupState.EN_ROUTE) || grp.HasState(DCO_EGroupState.MOUNTED) || grp.IsInTransport() || grp.IsDedicatedTransport()))
			AddDanger(grp.GetOwner().GetOrigin(), now);
	}

	protected void TrackRoadSighting(vector pos, vector vel, float now)
	{
		RoadNetworkManager roads = Roads();
		if (!roads)
			return;
		BaseRoad road;
		float dist;
		roads.GetClosestRoad(pos, road, dist);
		if (!road || dist > 40)
			return;

		vector dir = vel.Normalized();
		foreach (DCO_RoadTrack t : m_aRoads)
		{
			if (now - t.m_fLast > MSR_WINDOW_S || vector.DistanceXZ(t.m_vPos, pos) > MSR_MATCH_DIST)
				continue;
			if (Math.AbsFloat(t.m_vDir[0] * dir[0] + t.m_vDir[2] * dir[2]) < 0.6)
				continue;
			if (now - t.m_fLast >= MSR_PASS_GAP_S)
			{
				t.m_iCount++;
				Event(string.Format("def_msr_sighting pos=%1 count=%2", t.m_vPos, t.m_iCount));
			}
			t.m_fLast = now;
			return;
		}

		DCO_RoadTrack n = new DCO_RoadTrack();
		n.m_vPos = pos;
		n.m_vDir = dir;
		n.m_iCount = 1;
		n.m_fLast = now;
		m_aRoads.Insert(n);
		if (m_aRoads.Count() > 32)
			m_aRoads.Remove(0);
	}

	protected void AddDanger(vector pos, float now)
	{
		foreach (CMD_ThreatEntry d : m_aDanger)
		{
			if (vector.DistanceXZ(d.m_vBelievedPos, pos) < DANGER_RADIUS)
			{
				d.m_fLastUpdateTime = now;
				return;
			}
		}
		CMD_ThreatEntry e = new CMD_ThreatEntry(pos, 4, now, null);
		e.m_fBelievedUncertainty = DANGER_RADIUS;
		e.m_fUncertainty = DANGER_RADIUS;
		m_aDanger.Insert(e);
		Event(string.Format("def_danger_zone pos=%1", pos));
	}

	protected void PurgeDangers(float now)
	{
		for (int i = m_aDanger.Count() - 1; i >= 0; i--)
		{
			if (now - m_aDanger[i].m_fLastUpdateTime > DANGER_TTL_S)
				m_aDanger.Remove(i);
		}
	}

	void GetDangers(notnull array<CMD_ThreatEntry> outList)
	{
		foreach (CMD_ThreatEntry d : m_aDanger)
			outList.Insert(d);
		foreach (DCO_CBTrack t : m_aCB)
		{
			if (t.m_Entry)
				outList.Insert(t.m_Entry);
		}
	}

	bool CrossesDanger(vector from, vector to)
	{
		foreach (CMD_ThreatEntry d : m_aDanger)
		{
			vector seg = to - from;
			seg[1] = 0;
			vector rel = d.m_vBelievedPos - from;
			float len2 = Math.Max(seg[0] * seg[0] + seg[2] * seg[2], 1);
			float tt = Math.Clamp((rel[0] * seg[0] + rel[2] * seg[2]) / len2, 0, 1);
			if (vector.DistanceXZ(from + seg * tt, d.m_vBelievedPos) < DANGER_RADIUS)
				return true;
		}
		return false;
	}

	bool IsOP(DCO_GroupUtilityComponent g)
	{
		foreach (DCO_OPPost p : m_aOP)
		{
			if (p.m_Group == g)
				return true;
		}
		return false;
	}

	protected void PlanOPs(AICommander_BaseComponent cmd, float now)
	{
		if (!cmd.IsOPEnabled() || m_aOP.Count() >= cmd.GetMaxOP())
			return;

		DCO_CommanderOps ops = cmd.GetOps();
		CMD_AICommanderObjectiveComponent main = ops.GetMainEffort();
		if (main && !main.IsCapturedBy(cmd.GetCommanderFactionKey()) && !HasOPPurpose("intel") && ops.GetIntel(main).m_fConfidence < 0.5)
		{
			vector mainPos = main.GetOwner().GetOrigin();
			vector pos = ops.FindOverwatch(main, cmd.GetOwner().GetOrigin());
			if (pos != vector.Zero)
			{
				OpenOP(cmd, pos, mainPos, "intel", now);
				return;
			}
		}

		if (HasOPPurpose("screen"))
			return;
		CMD_AICommanderObjectiveComponent front;
		vector enemyDir;
		if (!FrontObjective(cmd, front, enemyDir))
			return;
		vector watch = Surface(front.GetOwner().GetOrigin() + enemyDir * (front.GetRadius() + 400));
		vector site;
		if (FindSite(cmd, watch, -enemyDir, site))
			OpenOP(cmd, site, watch, "screen", now);
	}

	protected bool HasOPPurpose(string purpose)
	{
		foreach (DCO_OPPost p : m_aOP)
		{
			if (p.m_sPurpose == purpose)
				return true;
		}
		return false;
	}

	protected void OpenOP(AICommander_BaseComponent cmd, vector pos, vector watch, string purpose, float now)
	{
		DCO_GroupUtilityComponent parent = PickReserve(cmd, pos, false, false, null, 2000);
		if (!parent)
			return;

		DCO_OPPost p = new DCO_OPPost();
		p.m_vPos = pos;
		p.m_vWatch = watch;
		p.m_sPurpose = purpose;
		p.m_fStart = now;

		if (parent.GetUnitCount() <= OP_TEAM + 1)
			p.m_Group = parent;
		else
		{
			SCR_AIGroup pg = SCR_AIGroup.Cast(parent.GetOwner());
			array<AIAgent> agents = {};
			array<AIAgent> team = {};
			pg.GetAgents(agents);
			AIAgent leader = pg.GetLeaderAgent();
			foreach (AIAgent ag : agents)
			{
				if (ag && ag != leader && team.Count() < OP_TEAM)
					team.Insert(ag);
			}
			SCR_AIGroup ng = DCO_Logistics.SplitIntoNewGroup(pg, team, cmd);
			if (!ng)
				return;
			p.m_Group = DCO_GroupUtilityComponent.Cast(ng.FindComponent(DCO_GroupUtilityComponent));
			p.m_Parent = parent;
			if (!p.m_Group)
				return;
		}

		m_aOP.Insert(p);
		string parentName = "none";
		if (p.m_Parent)
			parentName = p.m_Parent.GetOwner().GetName();
		Event(string.Format("def_op_open purpose=%1 pos=%2 watch=%3 team=%4 split_from=%5", purpose, pos, watch, p.m_Group.GetOwner().GetName(), parentName));
	}

	protected void UpdateOPs(AICommander_BaseComponent cmd, float now)
	{
		for (int i = m_aOP.Count() - 1; i >= 0; i--)
		{
			DCO_OPPost p = m_aOP[i];
			if (!p.m_Group || p.m_Group.GetUnitCount() == 0)
			{
				Event(string.Format("def_op_withdraw purpose=%1 why=lost", p.m_sPurpose));
				m_aOP.Remove(i);
				continue;
			}

			if (!p.m_bOrdered)
			{
				p.m_bOrdered = true;
				p.m_iStrength = p.m_Group.GetUnitCount();
				MoveIntoPosition(cmd, p.m_Group, p.m_vPos, p.m_vWatch, now);
				continue;
			}

			if (p.m_Group.GetTask() != DCO_EGroupTask.DEFEND || p.m_Group.GetGroupObjective())
			{
				m_aOP.Remove(i);
				continue;
			}

			string why = "";
			if (p.m_Group.GetUnitCount() < p.m_iStrength || p.m_Group.HasState(DCO_EGroupState.SUPPRESSED))
				why = "discovered";
			else if (EnemyNear(p.m_Group, cmd.GetOPWithdrawDist()))
				why = "enemy_close";
			else if (now - p.m_fStart > cmd.GetOPRotateTime())
				why = "rotate";
			if (why == "")
				continue;

			p.m_Group.CompleteAllWaypoints();
			p.m_Group.SetRetreating(true);
			p.m_Group.DCO_SetHold(0);
			if (p.m_Parent)
				cmd.GetOps().RequestMerge(cmd, p.m_Group, p.m_Parent, now);
			else
			{
				vector back = cmd.GetOwner().GetOrigin() - p.m_vPos;
				back[1] = 0;
				back.Normalize();
				SCR_AIWaypoint wp = cmd.SpawnMoveWP(Surface(p.m_vPos + back * 250));
				if (wp)
					p.m_Group.MoveTo(wp, now);
			}
			Event(string.Format("def_op_withdraw purpose=%1 why=%2 team=%3 held_s=%4", p.m_sPurpose, why, p.m_Group.GetOwner().GetName(), Math.Round(now - p.m_fStart)));
			m_aOP.Remove(i);
		}
	}

	protected static bool EnemyNear(DCO_GroupUtilityComponent g, float dist)
	{
		PerceptionManager pm = GetGame().GetPerceptionManager();
		SCR_AIGroupUtilityComponent u = g.GetGroupUtilityComponent();
		if (!pm || !u || !u.GetPercGroupComp())
			return false;
		float pmNow = pm.GetTime();
		vector pos = g.GetOwner().GetOrigin();
		foreach (SCR_AITargetInfo t : u.GetPercGroupComp().m_aTargets)
		{
			if (t && pmNow - t.m_fTimestamp <= 10 && vector.DistanceXZ(t.m_vWorldPos, pos) < dist)
				return true;
		}
		return false;
	}

	void OnObjectiveLost(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, float now)
	{
		if (!cmd.IsCounterattackEnabled())
			return;
		DCO_CounterAttack c = new DCO_CounterAttack();
		c.m_Obj = obj;
		c.m_fDeadline = now + cmd.GetCounterattackWindow();
		m_aCounter.Insert(c);
		Event(string.Format("def_counterattack_window obj=%1 window_s=%2", obj.GetOwner().GetName(), Math.Round(cmd.GetCounterattackWindow())));
	}

	protected void UpdateCounterAttacks(AICommander_BaseComponent cmd, float now)
	{
		FactionKey fk = cmd.GetCommanderFactionKey();
		for (int i = m_aCounter.Count() - 1; i >= 0; i--)
		{
			DCO_CounterAttack c = m_aCounter[i];
			if (!c.m_Obj || c.m_Obj.IsCapturedBy(fk))
			{
				m_aCounter.Remove(i);
				continue;
			}
			if (now > c.m_fDeadline)
			{
				Event(string.Format("def_counterattack_skip obj=%1 why=window_expired (-> rencana operasi biasa)", c.m_Obj.GetOwner().GetName()));
				m_aCounter.Remove(i);
				continue;
			}
			if (!c.m_Obj.CheckIsItLost(fk) || c.m_Obj.GetObjectiveState(fk) != CMD_EObjectiveState.PENDING)
				continue;
			if (TryCounterAttack(cmd, c.m_Obj, now))
				m_aCounter.Remove(i);
		}
	}

	protected bool TryCounterAttack(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, float now)
	{
		vector p = obj.GetOwner().GetOrigin();
		float enemy = Math.Max(cmd.EstimateEnemyAt(obj), COUNTER_MIN_ENEMY);
		float need = cmd.GetMinAttackRatio() * enemy;

		array<DCO_GroupUtilityComponent> cands = {};
		array<float> dists = {};
		foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
		{
			if (!g || g.IsPlayerGroup() || g.IsDedicatedTransport() || g.IsMortar() || !g.IsAvailableReserve())
				continue;
			if (!g.CanCommanderOverrideRole() || !g.CanItHaveOrder() || !cmd.CanCommitGroup(g))
				continue;
			float d = vector.DistanceXZ(g.GetOwner().GetOrigin(), p);
			if (d > COUNTER_RANGE)
				continue;
			int at = 0;
			while (at < dists.Count() && dists[at] <= d)
				at++;
			cands.InsertAt(g, at);
			dists.InsertAt(d, at);
		}

		array<DCO_GroupUtilityComponent> picks = {};
		float strength;
		foreach (DCO_GroupUtilityComponent c : cands)
		{
			picks.Insert(c);
			float w = c.GetUnitCount();
			if (c.IsArmor())
				w *= 3;
			strength += w;
			if (strength >= need)
				break;
		}
		if (strength < need)
			return false;

		FactionKey fk = cmd.GetCommanderFactionKey();
		foreach (DCO_GroupUtilityComponent pg : picks)
		{
			pg.CompleteAllWaypoints();
			pg.SetTask(DCO_EGroupTask.ATTACK);
			pg.SetGroupObjective(obj);
			obj.SetObjectiveGroup(fk, 1);
			cmd.SpawnMoveRoute(pg, pg.GetOwner().GetOrigin(), p, now);
		}
		Event(string.Format("def_counterattack obj=%1 groups=%2 strength=%3 need=%4 enemy_est=%5", obj.GetOwner().GetName(), picks.Count(), strength, Math.Round(need), enemy));
		return true;
	}

	static void BroadcastIndirectFire(AICommander_BaseComponent shooterCmd, DCO_GroupUtilityComponent unit, vector impact)
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		FactionManager fm = GetGame().GetFactionManager();
		if (!mgr || !fm || !shooterCmd || !unit || !unit.GetOwner())
			return;
		Faction sf = fm.GetFactionByKey(shooterCmd.GetCommanderFactionKey());
		if (!sf)
			return;

		float now = DCO_CommanderOps.Now();
		foreach (AICommander_BaseComponent victim : mgr.m_aCommander)
		{
			if (!victim || victim == shooterCmd || !victim.GetDefense())
				continue;
			Faction vf = fm.GetFactionByKey(victim.GetCommanderFactionKey());
			if (!vf || vf == sf || vf.IsFactionFriendly(sf))
				continue;
			foreach (DCO_GroupUtilityComponent g : victim.GetOwnedGroups())
			{
				if (g && g.GetOwner() && vector.DistanceXZ(g.GetOwner().GetOrigin(), impact) <= CB_NOTIFY_RADIUS)
				{
					victim.GetDefense().OnIndirectFire(victim, unit.GetOwner(), unit.GetOwner().GetOrigin(), now);
					break;
				}
			}
		}
	}

	void OnIndirectFire(AICommander_BaseComponent cmd, IEntity source, vector truePos, float now)
	{
		if (!cmd.IsCounterBatteryEnabled())
			return;

		DCO_CBTrack t;
		foreach (DCO_CBTrack c : m_aCB)
		{
			if (c.m_Source == source)
			{
				t = c;
				break;
			}
		}
		if (!t)
		{
			t = new DCO_CBTrack();
			t.m_Source = source;
			t.m_Entry = new CMD_ThreatEntry(truePos, 3, now, null);
			t.m_Entry.m_bArtillery = true;
			m_aCB.Insert(t);
		}

		t.m_iHits++;
		t.m_fLast = now;
		t.m_vTruePos = truePos;
		float unc = cmd.GetCBInitialError() / Math.Sqrt(t.m_iHits);
		float ang = Math.RandomFloat(0, Math.PI * 2);
		float r = unc * Math.Sqrt(Math.RandomFloat01());
		vector est = Surface(truePos + Vector(Math.Cos(ang) * r, 0, Math.Sin(ang) * r));

		t.m_Entry.m_vPosition = est;
		t.m_Entry.m_vBelievedPos = est;
		t.m_Entry.m_fUncertainty = unc;
		t.m_Entry.m_fBelievedUncertainty = unc;
		t.m_Entry.m_fLastUpdateTime = now;
		Event(string.Format("def_cb_estimate hits=%1 unc=%2 error=%3", t.m_iHits, Math.Round(unc), Math.Round(vector.DistanceXZ(est, truePos))));
	}

	protected void UpdateCB(AICommander_BaseComponent cmd, float now)
	{
		for (int i = m_aCB.Count() - 1; i >= 0; i--)
		{
			DCO_CBTrack t = m_aCB[i];
			if (now - t.m_fLast > CB_TTL_S)
			{
				ReleaseHunter(t);
				m_aCB.Remove(i);
				continue;
			}

			vector est = t.m_Entry.m_vBelievedPos;
			if (t.m_Hunter && (now - t.m_fHunterStart > HUNTER_TIMEOUT_S || t.m_Hunter.GetTask() != DCO_EGroupTask.REINFORCE))
				ReleaseHunter(t);

			CMD_ArtillerySupport arty = cmd.GetArtySupport();
			string tier = "none";
			if (arty)
				tier = arty.ResolveTier(t.m_Entry.m_fBelievedUncertainty);
			if (tier != "none")
			{
				if (!arty.HasRegisteredUnits() || now - t.m_fLastFire < CB_FIRE_COOLDOWN_S)
					continue;
				CMD_FireMissionRequest req = new CMD_FireMissionRequest(est, SCR_EAIArtilleryAmmoType.HIGH_EXPLOSIVE, now, CB_SHELLS, t.m_fLast, 0.8);
				arty.ApplyTier(req, tier, t.m_Entry.m_fBelievedUncertainty, "cb");
				if (arty.HasFriendlyNearRequest(req, now))
				{
					CMD_ArtillerySupport.LogDenied(req, "friendly");
					continue;
				}
				arty.RequestShellImpact(req, now, CB_SHELLS);
				t.m_fLastFire = now;
				Event(string.Format("def_cb_fire unc=%1 error=%2 shells=%3", Math.Round(t.m_Entry.m_fBelievedUncertainty), Math.Round(vector.DistanceXZ(est, t.m_vTruePos)), CB_SHELLS));
				continue;
			}

			if (t.m_bHunted)
				continue;
			DCO_GroupUtilityComponent h = PickReserve(cmd, est, false, false, null, 2500);
			if (!h)
				continue;
			h.CompleteAllWaypoints();
			h.SetTask(DCO_EGroupTask.REINFORCE);
			h.DCO_SetHold(now + HUNTER_TIMEOUT_S);
			if (!cmd.SpawnMoveRoute(h, h.GetOwner().GetOrigin(), est, now))
			{
				h.SetTask(DCO_EGroupTask.NONE);
				h.DCO_SetHold(0);
				continue;
			}
			t.m_Hunter = h;
			t.m_bHunted = true;
			t.m_fHunterStart = now;
			Event(string.Format("def_cb_hunter group=%1 unc=%2 error=%3", h.GetOwner().GetName(), Math.Round(t.m_Entry.m_fBelievedUncertainty), Math.Round(vector.DistanceXZ(est, t.m_vTruePos))));
		}
	}

	protected static void ReleaseHunter(DCO_CBTrack t)
	{
		if (!t.m_Hunter)
			return;
		t.m_Hunter.DCO_SetHold(0);
		if (t.m_Hunter.GetTask() == DCO_EGroupTask.REINFORCE)
		{
			t.m_Hunter.CompleteAllWaypoints();
			t.m_Hunter.SetTask(DCO_EGroupTask.NONE);
		}
		t.m_Hunter = null;
	}

	protected DCO_GroupUtilityComponent PickReserve(AICommander_BaseComponent cmd, vector pos, bool needAT, bool preferMG, DCO_GroupUtilityComponent exclude, float maxDist)
	{
		DCO_GroupUtilityComponent best;
		float bestScore = -float.MAX;
		foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
		{
			if (!g || g == exclude || g.IsPlayerGroup() || g.IsDedicatedTransport() || g.IsMortar() || g.IsArmor() || !g.IsInfantry())
				continue;
			if (!g.IsAvailableReserve() || g.HasState(DCO_EGroupState.IN_CONTACT) || !g.CanCommanderOverrideRole() || !g.CanItHaveOrder() || !cmd.CanCommitGroup(g))
				continue;
			if (needAT && !g.HasAT())
				continue;
			float d = vector.DistanceXZ(g.GetOwner().GetOrigin(), pos);
			if (d > maxDist)
				continue;
			float s = -d;
			if (preferMG && g.HasMG())
				s += 300;
			if (s > bestScore)
			{
				bestScore = s;
				best = g;
			}
		}
		return best;
	}

	protected static int CountReserves(AICommander_BaseComponent cmd)
	{
		int n;
		foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
		{
			if (g && !g.IsPlayerGroup() && !g.IsMortar() && !g.IsDedicatedTransport() && g.IsAvailableReserve())
				n++;
		}
		return n;
	}

	protected static bool FrontObjective(AICommander_BaseComponent cmd, out CMD_AICommanderObjectiveComponent front, out vector enemyDir)
	{
		front = null;
		enemyDir = vector.Zero;
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
			return false;
		FactionKey fk = cmd.GetCommanderFactionKey();
		float best = float.MAX;
		foreach (CMD_AICommanderObjectiveComponent own : mgr.m_aObjective)
		{
			if (!own || !own.IsCapturedBy(fk))
				continue;
			foreach (CMD_AICommanderObjectiveComponent other : mgr.m_aObjective)
			{
				if (!other || other.IsCapturedBy(fk))
					continue;
				float d = vector.DistanceXZ(own.GetOwner().GetOrigin(), other.GetOwner().GetOrigin());
				if (d < best)
				{
					best = d;
					front = own;
					enemyDir = other.GetOwner().GetOrigin() - own.GetOwner().GetOrigin();
				}
			}
		}
		if (!front)
			return false;
		enemyDir[1] = 0;
		enemyDir.Normalize();
		return true;
	}

	protected static bool NearOwnObjective(AICommander_BaseComponent cmd, vector p)
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
			return false;
		foreach (CMD_AICommanderObjectiveComponent o : mgr.m_aObjective)
		{
			if (o && o.IsCapturedBy(cmd.GetCommanderFactionKey()) && vector.DistanceXZ(o.GetOwner().GetOrigin(), p) < o.GetRadius() + OWN_OBJ_CLEAR)
				return true;
		}
		return false;
	}

	protected static bool NearThreat(AICommander_BaseComponent cmd, vector p, float dist)
	{
		CMD_ThreatResponseComponent tr = cmd.GetThreatResponseComponent();
		if (!tr)
			return false;
		foreach (CMD_ThreatEntry t : tr.GetThreats())
		{
			if (t && vector.DistanceXZ(t.m_vBelievedPos, p) < dist + t.m_fBelievedUncertainty)
				return true;
		}
		return false;
	}

	protected static vector SnapKillZone(vector p, float maxSnap)
	{
		RoadNetworkManager roads = Roads();
		if (roads)
		{
			BaseRoad road;
			float dist;
			roads.GetClosestRoad(p, road, dist);
			if (road && dist <= maxSnap)
			{
				array<vector> pts = {};
				road.GetPoints(pts);
				vector best = p;
				float bestSq = float.MAX;
				foreach (vector rp : pts)
				{
					float dSq = vector.DistanceSqXZ(rp, p);
					if (dSq < bestSq)
					{
						bestSq = dSq;
						best = rp;
					}
				}
				p = best;
			}
		}
		return Surface(p);
	}

	protected static RoadNetworkManager Roads()
	{
		SCR_AIWorld aiWorld = SCR_AIWorld.Cast(GetGame().GetAIWorld());
		if (!aiWorld)
			return null;
		return aiWorld.GetRoadNetworkManager();
	}

	protected static vector Surface(vector p)
	{
		p[1] = GetGame().GetWorld().GetSurfaceY(p[0], p[2]);
		return p;
	}

	protected static bool IsWet(vector p)
	{
		EWaterSurfaceType waterType = EWaterSurfaceType.WST_NONE;
		float lakeArea = 0;
		float waterY = SCR_WorldTools.GetWaterSurfaceY(null, p, waterType, lakeArea);
		return waterType != EWaterSurfaceType.WST_NONE && p[1] < waterY;
	}

	protected static bool HasLOS(vector from, vector to)
	{
		TraceParam trace = new TraceParam();
		trace.Start = Vector(from[0], from[1] + 1.2, from[2]);
		trace.End = Vector(to[0], to[1] + 1.5, to[2]);
		trace.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
		DCO_Perf.Count("t:DCO_CommanderDefense");
		return GetGame().GetWorld().TraceMove(trace, null) >= 0.95;
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CommanderAmbushAttribute : DCO_CommanderLogiBoolAttribute
{
	override protected bool Get(AICommander_BaseComponent cmd) { return cmd.IsAmbushEnabled(); }
	override protected void Set(AICommander_BaseComponent cmd, bool value) { cmd.SetAmbushEnabled(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CommanderAmbushMSRAttribute : DCO_CommanderLogiBoolAttribute
{
	override protected bool Get(AICommander_BaseComponent cmd) { return cmd.IsMSRAmbushEnabled(); }
	override protected void Set(AICommander_BaseComponent cmd, bool value) { cmd.SetMSRAmbushEnabled(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CommanderOPAttribute : DCO_CommanderLogiBoolAttribute
{
	override protected bool Get(AICommander_BaseComponent cmd) { return cmd.IsOPEnabled(); }
	override protected void Set(AICommander_BaseComponent cmd, bool value) { cmd.SetOPEnabled(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CommanderCounterattackAttribute : DCO_CommanderLogiBoolAttribute
{
	override protected bool Get(AICommander_BaseComponent cmd) { return cmd.IsCounterattackEnabled(); }
	override protected void Set(AICommander_BaseComponent cmd, bool value) { cmd.SetCounterattackEnabled(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CommanderCounterBatteryAttribute : DCO_CommanderLogiBoolAttribute
{
	override protected bool Get(AICommander_BaseComponent cmd) { return cmd.IsCounterBatteryEnabled(); }
	override protected void Set(AICommander_BaseComponent cmd, bool value) { cmd.SetCounterBatteryEnabled(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CommanderFeintAttribute : DCO_CommanderLogiBoolAttribute
{
	override protected bool Get(AICommander_BaseComponent cmd) { return cmd.IsFeintEnabled(); }
	override protected void Set(AICommander_BaseComponent cmd, bool value) { cmd.SetFeintEnabled(value); }
}
