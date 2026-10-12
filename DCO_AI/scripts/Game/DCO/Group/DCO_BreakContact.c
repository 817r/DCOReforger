class DCO_BreakContact
{
	protected static ref array<ref DCO_BreakContact> s_aActive;

	DCO_GroupUtilityComponent m_Group;
	vector m_vThreat;
	vector m_vRally;
	string m_sWhy;
	float m_fStart;
	float m_fStepStart;
	float m_fLastSeen;
	int m_iMover;
	float m_fSide = 1;
	float m_fMaxDist;
	vector m_vFrom;
	bool m_bRegroup;
	bool m_bIneffHoldLogged;
	float m_fRegroupStart;
	vector m_vGoal;
	int m_iLeg = 1;
	vector m_vDogleg;
	float m_fLegStart;
	ref array<ref array<AIAgent>> m_aElements = {};

	static int ActiveCount()
	{
		if (!s_aActive)
			return 0;
		return s_aActive.Count();
	}

	static bool Start(DCO_GroupUtilityComponent g, vector threat, vector rally, string why, float maxDist = 0)
	{
		if (!g || g.IsPlayerGroup() || g.IsInTransport() || !Replication.IsServer() || !DCO_GlobalAIComponent.BehaviorOnFor(g.GetOwner(), DCO_EBehavior.BREAK_CONTACT))
			return false;
		SCR_AIGroup grp = SCR_AIGroup.Cast(g.GetOwner());
		if (!grp || !DCO_Util.IsAlive(grp.GetLeaderEntity()))
			return false;
		DCO_EGroupTask bcTask = g.GetTask();
		bool cmdOrder = why.StartsWith("def_") || why.StartsWith("op_") || why == "threat_withdraw" || why.EndsWith("_depleted") || why.EndsWith("_shaken");
		if (!cmdOrder && (g.m_iDCOTRPhase >= 0 || bcTask == DCO_EGroupTask.ATTACK || bcTask == DCO_EGroupTask.SUPPORT_BY_FIRE))
		{
			DCO_Perf.Count("break_contact_skip:attack_task");
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("break_contact_skip grp=%1 why=%2 reason=attack_task task=%3 tr_phase=%4", DCO_Log.IdGroup(grp), why, typename.EnumToString(DCO_EGroupTask, bcTask), g.m_iDCOTRPhase));
			return false;
		}
		if (!cmdOrder && bcTask == DCO_EGroupTask.DEFEND && g.GetMyCommander())
		{
			DCO_Perf.Count("break_contact_skip:defend_task");
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("break_contact_skip grp=%1 why=%2 reason=defend_task_commander_decides", DCO_Log.IdGroup(grp), why));
			return false;
		}
		vector lp = grp.GetLeaderEntity().GetOrigin();
		if (vector.DistanceXZ(lp, threat) > 400.0)
			return false;

		float now = DCO_Util.NowS();
		if (rally == vector.Zero)
			rally = RallyFrom(lp, threat);

		DCO_BreakContact bc = g.m_DCOBreak;
		if (bc)
		{
			bc.m_vThreat = threat;
			bc.m_vRally = rally;
			return true;
		}

		bc = new DCO_BreakContact();
		bc.m_Group = g;
		bc.m_vThreat = threat;
		bc.m_vRally = rally;
		bc.m_sWhy = why;
		bc.m_fMaxDist = maxDist;
		bc.m_vFrom = lp;
		bc.m_fStart = now;
		bc.m_fLastSeen = now;
		g.m_DCOBreak = bc;
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("roe_set grp=%1 why=breaking_contact mode=FIRE_AT_WILL", DCO_Log.IdGroup(grp)));
		if (g.GetMyCommander())
			g.CompleteAllWaypoints();
		g.SetRetreating(true);
		bc.Split(grp);
		CMD_ThreatResponseComponent.ThrowSmoke(g, threat);
		bc.NextStep(now);

		if (!s_aActive)
		{
			s_aActive = {};
			GetGame().GetCallqueue().CallLater(Tick, 1000, true);
		}
		s_aActive.Insert(bc);
		DCO_Perf.Count("bc:start");
		DCO_MoraleEvents.OnEnemyRetreat(grp.GetLeaderEntity(), grp.GetFaction());
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("break_contact_start grp=%1 why=%2 dist=%3 elements=%4", DCO_Log.IdGroup(grp), why, Math.Round(vector.DistanceXZ(lp, threat)), bc.m_aElements.Count()));
		return true;
	}

	static vector RallyFrom(vector from, vector threat)
	{
		vector away = DCO_Specialist.Flat(from - threat);
		vector p = from + away * 400.0;
		return DCO_Util.Surface(p);
	}

	protected static void Tick()
	{
		int pt = DCO_Perf.Begin();
		float now = DCO_Util.NowS();
		for (int i = s_aActive.Count() - 1; i >= 0; i--)
		{
			DCO_BreakContact bc = s_aActive[i];
			if (!bc.Update(now))
			{
				if (bc.m_Group && bc.m_Group.m_DCOBreak == bc)
					bc.m_Group.m_DCOBreak = null;
				s_aActive.Remove(i);
			}
		}
		DCO_Perf.End("grp_break_contact", pt);
	}

	protected void Split(SCR_AIGroup grp)
	{
		array<AIAgent> agents = {};
		grp.GetAgents(agents);
		array<AIAgent> able = {};
		foreach (AIAgent a : agents)
		{
			if (a && DCO_Util.IsAlive(a.GetControlledEntity()))
				able.Insert(a);
		}

		m_aElements.Clear();
		if (able.Count() >= 6)
		{
			m_aElements.Insert(new array<AIAgent>());
			m_aElements.Insert(new array<AIAgent>());
			foreach (int i, AIAgent a : able)
				m_aElements[i % 2].Insert(a);
			return;
		}
		for (int j = 0; j < able.Count(); j += 2)
		{
			array<AIAgent> pair = {};
			pair.Insert(able[j]);
			if (j + 1 < able.Count())
				pair.Insert(able[j + 1]);
			m_aElements.Insert(pair);
		}
	}

	protected bool Update(float now)
	{
		DCO_GroupUtilityComponent g = m_Group;
		if (!g || g.m_DCOBreak != this)
			return false;
		SCR_AIGroup grp = SCR_AIGroup.Cast(g.GetOwner());
		if (!grp || !DCO_Util.IsAlive(grp.GetLeaderEntity()))
			return false;
		if (!g.HasState(DCO_EGroupState.RETREATING))
		{
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("break_contact_cancel grp=%1 why=retasked", DCO_Log.IdGroup(grp)));
			return false;
		}

		vector lp = grp.GetLeaderEntity().GetOrigin();
		if (m_fMaxDist > 0 && vector.DistanceXZ(lp, m_vFrom) >= m_fMaxDist)
		{
			g.m_DCOBreak = null;
			g.SetRetreating(false);
			DCO_Perf.Count("bc:bound_done");
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("break_contact_bound grp=%1 why=%2 dist=%3 s=%4", DCO_Log.IdGroup(grp), m_sWhy, Math.Round(m_fMaxDist), Math.Round(now - m_fStart)));
			return false;
		}
		if (m_bRegroup)
		{
			if (m_iLeg == 0)
			{
				bool reached = vector.DistanceXZ(lp, m_vDogleg) <= 20.0;
				if (!reached && now - m_fLegStart < 90.0)
					return true;
				m_iLeg = 1;
				m_fRegroupStart = now;
				if (!g.GetMyCommander())
				{
					foreach (array<AIAgent> legEl : m_aElements)
						MoveElement(legEl, m_vRally, m_vThreat);
				}
				string legWhy = "reached";
				if (!reached)
					legWhy = "timeout";
				DCO_Perf.Count("bc:turn_leg_" + legWhy);
				DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("break_contact_turn_done grp=%1 why=%2 leg_s=%3 to_rally=%4", DCO_Log.IdGroup(grp), legWhy, Math.Round(now - m_fLegStart), Math.Round(vector.DistanceXZ(lp, m_vRally))));
				return true;
			}
			if (vector.DistanceXZ(lp, m_vRally) > 60.0 && now - m_fRegroupStart < 180.0)
				return true;
			if (!g.GetMyCommander() && now < g.m_fDCOIneffUntil_s)
			{
				if (!m_bIneffHoldLogged)
				{
					m_bIneffHoldLogged = true;
					DCO_Perf.Count("bc:ineffective_hold");
					DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("break_contact_hold grp=%1 why=combat_ineffective left_s=%2", DCO_Log.IdGroup(grp), Math.Round(g.m_fDCOIneffUntil_s - now)));
				}
				return true;
			}
			g.m_DCOBreak = null;
			g.SetRetreating(false);
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("break_contact_rally grp=%1 units=%2 total_s=%3", DCO_Log.IdGroup(grp), g.GetUnitCount(), Math.Round(now - m_fStart)));
			if (DCO_Ctx.TuneFor(grp, "bc.face", 0.0) > 0)
				FaceContact(grp, lp);
			return false;
		}

		RefreshThreat(g, now);
		float dist = vector.DistanceXZ(lp, m_vThreat);
		if (dist > 500.0 || now - m_fLastSeen >= 30.0 || now - m_fStart > 240.0)
		{
			m_bRegroup = true;
			m_fRegroupStart = now;
			m_iLeg = 1;
			if (DCO_Ctx.TuneFor(grp, "bc.dogleg", 0.0) > 0 && PlanDogleg(grp, lp))
				m_iLeg = 0;
			m_fLegStart = now;
			AICommander_BaseComponent cmd = g.GetMyCommander();
			if (cmd)
			{
				if (m_iLeg == 0)
				{
					cmd.SpawnMoveRoute(g, lp, m_vDogleg, now);
					cmd.SpawnMoveRoute(g, m_vDogleg, m_vRally, now);
				}
				else
					cmd.SpawnMoveRoute(g, lp, m_vRally, now);
			}
			else
			{
				vector firstGoal = m_vRally;
				if (m_iLeg == 0)
					firstGoal = m_vDogleg;
				foreach (array<AIAgent> el : m_aElements)
					MoveElement(el, firstGoal, m_vThreat);
			}
			string why = "distance";
			if (dist <= 500.0)
				why = "los_broken";
			if (now - m_fStart > 240.0)
				why = "timeout";
			DCO_Perf.Count("bc:done");
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("break_contact_done grp=%1 why=%2 dist=%3 units=%4 s=%5", DCO_Log.IdGroup(grp), why, Math.Round(dist), g.GetUnitCount(), Math.Round(now - m_fStart)));
			return true;
		}

		if (m_aElements.IsEmpty())
			return true;
		array<AIAgent> mover = m_aElements[m_iMover];
		bool arrived = true;
		foreach (AIAgent a : mover)
		{
			if (a && a.GetControlledEntity() && DCO_Util.IsAlive(a.GetControlledEntity()) && vector.DistanceXZ(a.GetControlledEntity().GetOrigin(), m_vGoal) > 12.0)
				arrived = false;
		}
		if (arrived || now - m_fStepStart > 25.0)
			NextStep(now);
		return true;
	}

	protected void RefreshThreat(DCO_GroupUtilityComponent g, float now)
	{
		SCR_AIGroupUtilityComponent u = g.GetGroupUtilityComponent();
		PerceptionManager pm = GetGame().GetPerceptionManager();
		if (!u || !u.m_Perception || !pm)
			return;
		float pmNow = pm.GetTime();
		vector sum;
		int n;
		foreach (SCR_AITargetInfo t : u.m_Perception.m_aTargets)
		{
			if (!t || pmNow - t.m_fTimestamp > 10.0)
				continue;
			sum = sum + t.m_vWorldPos;
			n++;
			if (pmNow - t.m_fTimestamp < 2.0 || t.m_bEndangering)
				m_fLastSeen = now;
		}
		if (n > 0)
			m_vThreat = sum * (1.0 / n);
	}

	protected void NextStep(float now)
	{
		if (m_aElements.IsEmpty())
			return;
		m_iMover = (m_iMover + 1) % m_aElements.Count();
		m_fSide = -m_fSide;
		m_fStepStart = now;

		array<AIAgent> mover = m_aElements[m_iMover];
		IEntity first;
		foreach (AIAgent a : mover)
		{
			if (a && DCO_Util.IsAlive(a.GetControlledEntity()))
			{
				first = a.GetControlledEntity();
				break;
			}
		}
		if (!first)
			return;

		vector from = first.GetOrigin();
		vector dir = DCO_Specialist.Flat(DCO_Specialist.Flat(from - m_vThreat) + DCO_Specialist.Flat(m_vRally - from));
		vector side = Vector(dir[2], 0, -dir[0]) * m_fSide;
		vector goal = from + dir * Math.RandomFloat(50.0, 100.0) + side * Math.RandomFloat(15.0, 30.0);
		vector cover = DCO_TerrainCache.QueryBest(DCO_ETerrainFlag.FOREST_EDGE, goal, 30.0, string.Empty, goal, m_vThreat);
		if (cover != vector.Zero)
			goal = cover;
		m_vGoal = DCO_Util.Surface(goal);
		MoveElement(mover, m_vGoal, m_vThreat);

		if (DCO_ConcealmentUtility.IsPositionExposed(from, first))
			CMD_ThreatResponseComponent.ThrowSmoke(m_Group, m_vThreat);
	}

	protected bool PlanDogleg(SCR_AIGroup grp, vector lp)
	{
		vector axis = DCO_Specialist.Flat(m_vRally - lp);
		vector away = DCO_Specialist.Flat(lp - m_vThreat);
		float deg = Math.RandomFloat(DCO_Ctx.TuneFor(grp, "bc.dogleg_min_deg", 45.0), DCO_Ctx.TuneFor(grp, "bc.dogleg_max_deg", 90.0));
		float side = m_fSide;
		float rad = deg * side * Math.DEG2RAD;
		vector dir = Vector(axis[0] * Math.Cos(rad) - axis[2] * Math.Sin(rad), 0, axis[0] * Math.Sin(rad) + axis[2] * Math.Cos(rad));
		if (vector.Dot(dir, away) < -0.2)
		{
			side = -side;
			rad = -rad;
			dir = Vector(axis[0] * Math.Cos(rad) - axis[2] * Math.Sin(rad), 0, axis[0] * Math.Sin(rad) + axis[2] * Math.Cos(rad));
		}
		float len = Math.Clamp(DCO_Ctx.TuneFor(grp, "bc.dogleg_m", 60.0), 20.0, 200.0);
		vector p = DCO_Util.Surface(lp + dir * len);
		string skip;
		AIWorld aiWorld = GetGame().GetAIWorld();
		NavmeshWorldComponent nav;
		if (aiWorld)
			nav = aiWorld.GetNavmeshWorldComponent("Soldiers");
		if (nav && nav.IsTileLoaded(p))
		{
			vector snapped;
			if (nav.GetReachablePoint(p, 12.0, snapped))
				p = DCO_Util.Surface(snapped);
			else
				skip = "off_navmesh";
		}
		if (skip.IsEmpty() && vector.DistanceXZ(p, m_vThreat) < vector.DistanceXZ(lp, m_vThreat))
			skip = "toward_threat";
		if (!skip.IsEmpty())
		{
			DCO_Perf.Count("bc:turn_skip_" + skip);
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("break_contact_turn_skip grp=%1 reason=%2 deg=%3", DCO_Log.IdGroup(grp), skip, Math.Round(deg * side)));
			return false;
		}
		m_vDogleg = p;
		DCO_Perf.Count("bc:turn");
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("break_contact_turn grp=%1 deg=%2 dist=%3 to=%4 rally=%5", DCO_Log.IdGroup(grp), Math.Round(deg * side), Math.Round(vector.DistanceXZ(lp, p)), DCO_Log.Pos(p), DCO_Log.Pos(m_vRally)));
		return true;
	}

	protected void FaceContact(SCR_AIGroup grp, vector lp)
	{
		vector axis = DCO_Specialist.Flat(m_vThreat - lp);
		array<AIAgent> agents = {};
		grp.GetAgents(agents);
		array<SCR_AIUtilityComponent> us = {};
		foreach (AIAgent a : agents)
		{
			SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
			if (ca && ca.m_UtilityComponent && ca.m_UtilityComponent.m_LookAction && DCO_Util.IsAlive(ca.GetControlledEntity()))
				us.Insert(ca.m_UtilityComponent);
		}
		int n = us.Count();
		if (n == 0)
			return;
		float fan = DCO_Ctx.TuneFor(grp, "bc.face_fan_deg", 60.0);
		float dur = DCO_Ctx.TuneFor(grp, "bc.face_s", 10.0);
		foreach (int i, SCR_AIUtilityComponent u : us)
		{
			float off = 0;
			if (n > 1)
				off = (i / (float)(n - 1) - 0.5) * fan;
			float rad = off * Math.DEG2RAD;
			vector dir = Vector(axis[0] * Math.Cos(rad) - axis[2] * Math.Sin(rad), 0, axis[0] * Math.Sin(rad) + axis[2] * Math.Cos(rad));
			vector me = u.m_OwnerEntity.GetOrigin();
			u.m_LookAction.LookAt(me + dir * 50.0 + Vector(0, 1.5, 0), 50.0, dur);
		}
		DCO_Perf.Count("bc:face");
		float bearing = Math.Atan2(axis[0], axis[2]) * Math.RAD2DEG;
		if (bearing < 0)
			bearing += 360.0;
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("break_contact_face grp=%1 units=%2 bearing=%3 fan=%4 dur_s=%5 threat_d=%6", DCO_Log.IdGroup(grp), n, Math.Round(bearing), Math.Round(fan), Math.Round(dur), Math.Round(vector.DistanceXZ(lp, m_vThreat))));
	}

	protected static void MoveElement(array<AIAgent> el, vector goal, vector threat)
	{
		foreach (AIAgent a : el)
		{
			SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
			if (!ca || !ca.m_UtilityComponent || !DCO_Util.IsAlive(ca.GetControlledEntity()))
				continue;
			vector p = DCO_Util.Surface(goal + Vector(Math.RandomFloat(-4, 4), 0, Math.RandomFloat(-4, 4)));
			float d = vector.DistanceXZ(ca.GetControlledEntity().GetOrigin(), p);
			if (d > 60.0)
				DCO_Specialist.MoveIndividually(ca.m_UtilityComponent, p);
			else
				DCO_RelocateBehavior.Start(ca.m_UtilityComponent, p, threat, true, ECharacterStance.CROUCH, d / 3.5 + 3.0, "break_contact");
		}
	}
}
