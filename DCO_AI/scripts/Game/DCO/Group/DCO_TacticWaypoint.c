enum DCO_ETacticWP
{
	ASSAULT,
	SUPPORT_BY_FIRE,
	AMBUSH,
	WITHDRAW,
	CONVOY
}

enum DCO_ETacticWPVal
{
	CLEAR_S,
	HOLD_S,
	SPACING_M,
	AMBUSH_M,
	DONE_S,
	SPEED_KMH,
	GAP_M,
	CONTACT,
	TARGET_M
}

class DCO_TacticWaypointClass : SCR_AIWaypointClass
{
}

class DCO_TacticWaypoint : SCR_AIWaypoint
{
	[Attribute("0", UIWidgets.ComboBox, "Tactic ordered by this waypoint. Assault: attack the area and finish when no enemy is left. Support by Fire: line up facing the waypoint direction and engage at long range. Ambush: line up prone, hold fire until the enemy is close. Withdraw: break contact toward the waypoint. Convoy: vehicle groups with the same destination drive in a column.", "", ParamEnumArray.FromEnum(DCO_ETacticWP))]
	protected DCO_ETacticWP m_eMode;

	DCO_ETacticWP DCO_Mode()
	{
		return m_eMode;
	}

	vector DCO_Facing()
	{
		vector dir = GetTransformAxis(2);
		dir[1] = 0;
		if (dir.LengthSq() < 0.01)
			return Vector(0, 0, 1);
		return dir.Normalized();
	}

	protected ref array<float> m_aDCOVal = {};

	bool DCO_UsesVal(int i)
	{
		switch (m_eMode)
		{
			case DCO_ETacticWP.ASSAULT:
				return i == DCO_ETacticWPVal.CLEAR_S;
			case DCO_ETacticWP.SUPPORT_BY_FIRE:
				return i == DCO_ETacticWPVal.HOLD_S || i == DCO_ETacticWPVal.SPACING_M || i == DCO_ETacticWPVal.TARGET_M;
			case DCO_ETacticWP.AMBUSH:
				return i == DCO_ETacticWPVal.SPACING_M || i == DCO_ETacticWPVal.AMBUSH_M || i == DCO_ETacticWPVal.DONE_S;
			case DCO_ETacticWP.CONVOY:
				return i == DCO_ETacticWPVal.SPEED_KMH || i == DCO_ETacticWPVal.GAP_M || i == DCO_ETacticWPVal.CONTACT;
		}
		return false;
	}

	static float DCO_ValDefault(int i)
	{
		switch (i)
		{
			case DCO_ETacticWPVal.CLEAR_S:
				return DCO_Ctx.TuneG("wp.assault_clear_s", 60.0);
			case DCO_ETacticWPVal.HOLD_S:
				return DCO_Ctx.TuneG("wp.sbf_hold_s", 0.0);
			case DCO_ETacticWPVal.SPACING_M:
				return DCO_Ctx.TuneG("wp.line_spacing_m", 4.0);
			case DCO_ETacticWPVal.AMBUSH_M:
				return DCO_Ctx.TuneG("wp.ambush_range_m", 60.0);
			case DCO_ETacticWPVal.DONE_S:
				return DCO_Ctx.TuneG("wp.ambush_done_s", 60.0);
			case DCO_ETacticWPVal.SPEED_KMH:
				return DCO_Ctx.TuneG("convoy.speed_kmh", 40.0);
			case DCO_ETacticWPVal.GAP_M:
				return DCO_Ctx.TuneG("convoy.gap_m", 25.0);
			case DCO_ETacticWPVal.CONTACT:
				return DCO_Ctx.TuneG("convoy.contact", 0.0);
			case DCO_ETacticWPVal.TARGET_M:
				return DCO_Ctx.TuneG("wp.sbf_target_m", 0.0);
		}
		return 0;
	}

	float DCO_Val(int i)
	{
		if (i < m_aDCOVal.Count() && m_aDCOVal[i] >= 0)
			return m_aDCOVal[i];
		return DCO_ValDefault(i);
	}

	void DCO_SetVal(int i, float v)
	{
		if (!DCO_UsesVal(i))
			return;
		while (m_aDCOVal.Count() <= i)
			m_aDCOVal.Insert(-1);
		v = Math.Max(v, 0);
		if (m_aDCOVal[i] == v)
			return;
		m_aDCOVal[i] = v;
		DCO_Perf.Count("wp_tactic:gm_set");
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", string.Format("wp_tactic_set wp=%1 mode=%2 setting=%3 value=%4", DCO_Log.Pos(GetOrigin()), typename.EnumToString(DCO_ETacticWP, m_eMode), typename.EnumToString(DCO_ETacticWPVal, i), v));
	}

	override bool DCO_WantsSetupDialog()
	{
		return m_eMode != DCO_ETacticWP.WITHDRAW;
	}

	override string DCO_SetupInfo()
	{
		return "type=tactic mode=" + typename.EnumToString(DCO_ETacticWP, m_eMode);
	}

	override SCR_AIWaypointState CreateWaypointState(SCR_AIGroupUtilityComponent groupUtilityComp)
	{
		return new DCO_TacticWaypointState(groupUtilityComp, this);
	}
}

class DCO_TacticWaypointState : SCR_AIWaypointState
{
	protected DCO_ETacticWP m_eMode;
	protected bool m_bApplied;
	protected bool m_bEnded;
	protected bool m_bSprung;
	protected float m_fSelectS = -1;
	protected float m_fStartS = -1;
	protected float m_fQuietS = -1;
	protected float m_fNextS;
	protected float m_fLogS;
	protected float m_fSpacing;
	protected SCR_AISuppressActivity m_Suppress;
	protected vector m_vSuppressAt;
	protected bool m_bSuppress;
	protected float m_fSuppressNextS;
	protected ref array<vector> m_aSpots = {};
	protected ref map<IEntity, float> m_mRelocAt = new map<IEntity, float>();

	protected DCO_GroupUtilityComponent G()
	{
		return m_Utility.DCO_GetUtil();
	}

	protected SCR_AIGroup Grp()
	{
		DCO_GroupUtilityComponent g = G();
		if (!g)
			return null;
		return SCR_AIGroup.Cast(g.GetOwner());
	}

	protected string Mode()
	{
		return typename.EnumToString(DCO_ETacticWP, m_eMode);
	}

	override void OnSelected()
	{
		super.OnSelected();
		DCO_TacticWaypoint wp = DCO_TacticWaypoint.Cast(m_Waypoint);
		DCO_GroupUtilityComponent g = G();
		SCR_AIGroup grp = Grp();
		if (!wp || !g || !grp)
			return;
		m_eMode = wp.DCO_Mode();
		m_bApplied = true;
		m_fSelectS = DCO_Util.NowS();
		float now_ms = GetGame().GetWorld().GetWorldTime();
		string extra = "-";
		switch (m_eMode)
		{
			case DCO_ETacticWP.ASSAULT:
			{
				g.SetTask(DCO_EGroupTask.ATTACK, "wp_assault");
				m_Utility.DCO_ForcePosture(DCO_GroupTactics.AGGRESIVE, now_ms, 3600000.0, false, "wp_assault");
				break;
			}
			case DCO_ETacticWP.WITHDRAW:
			{
				extra = "contact=0";
				vector threat;
				float nearest;
				int count;
				IEntity leader = grp.GetLeaderEntity();
				if (leader && DCO_PostureOps.FreshContact(m_Utility, leader.GetOrigin(), threat, nearest, count))
				{
					bool started = DCO_BreakContact.Start(g, threat, wp.GetOrigin(), "wp_withdraw");
					extra = string.Format("contact=1 break_contact=%1 enemy_m=%2", DCO_Util.B01(started), Math.Round(nearest));
				}
				break;
			}
			case DCO_ETacticWP.CONVOY:
			{
				DCO_Convoy.Join(grp, g, m_Utility, wp);
				break;
			}
		}
		float dist = -1;
		if (grp.GetLeaderEntity())
			dist = vector.DistanceXZ(grp.GetLeaderEntity().GetOrigin(), wp.GetOrigin());
		DCO_Perf.Count("wp_tactic:select_" + Mode());
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, DCO_Log.IdGroup(grp), string.Format("wp_tactic_select grp=%1 mode=%2 wp=%3 dist=%4 %5", DCO_Log.IdGroup(grp), Mode(), DCO_Log.Pos(wp.GetOrigin()), Math.Round(dist), extra));
	}

	override void OnDeselected()
	{
		Release("deselect");
		super.OnDeselected();
	}

	protected void Release(string why)
	{
		if (!m_bApplied)
			return;
		m_bApplied = false;
		DCO_GroupUtilityComponent g = G();
		SCR_AIGroup grp = Grp();
		if (!g || !grp)
			return;
		switch (m_eMode)
		{
			case DCO_ETacticWP.ASSAULT:
			{
				if (g.GetTask() == DCO_EGroupTask.ATTACK)
					g.SetTask(DCO_EGroupTask.NONE, "wp_assault_end");
				m_Utility.DCO_ClearForced("wp_assault_end");
				break;
			}
			case DCO_ETacticWP.SUPPORT_BY_FIRE:
			{
				if (g.GetTask() == DCO_EGroupTask.SUPPORT_BY_FIRE)
					g.SetTask(DCO_EGroupTask.NONE, "wp_sbf_end");
				StopSuppress(grp, why);
				break;
			}
			case DCO_ETacticWP.AMBUSH:
			{
				if (!m_bSprung)
					m_Utility.DCO_ClearForced("wp_ambush_end");
				m_Utility.m_fDCOAmbRangeM = 60;
				break;
			}
			case DCO_ETacticWP.CONVOY:
			{
				DCO_Convoy.Leave(grp);
				break;
			}
		}
		float age = 0;
		if (m_fSelectS >= 0)
			age = DCO_Util.NowS() - m_fSelectS;
		DCO_Perf.Count("wp_tactic:release_" + Mode());
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, DCO_Log.IdGroup(grp), string.Format("wp_tactic_release grp=%1 mode=%2 why=%3 secs=%4 reached=%5", DCO_Log.IdGroup(grp), Mode(), why, Math.Round(age), DCO_Util.B01(m_fStartS >= 0)));
	}

	protected void BuildSpots(SCR_AIGroup group, DCO_TacticWaypoint wp, int count)
	{
		m_aSpots.Clear();
		vector fwd = wp.DCO_Facing();
		vector side = Vector(fwd[2], 0, -fwd[0]);
		float spacing = wp.DCO_Val(DCO_ETacticWPVal.SPACING_M);
		m_fSpacing = spacing;
		NavmeshWorldComponent nav = GetGame().GetAIWorld().GetNavmeshWorldComponent("Soldiers");
		int offNav;
		for (int i = 0; i < count; i++)
		{
			float off = (i - (count - 1) * 0.5) * spacing;
			vector p = DCO_Util.Surface(wp.GetOrigin() + side * off);
			vector snapped;
			if (nav && nav.IsTileLoaded(p) && nav.GetReachablePoint(p, 3.0, snapped))
				p = snapped;
			else
				offNav++;
			m_aSpots.Insert(p);
		}
		DCO_Perf.Count("wp_tactic:spots");
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, DCO_Log.IdGroup(group), string.Format("wp_tactic_spots grp=%1 mode=%2 count=%3 off_navmesh=%4 spacing=%5 facing=%6", DCO_Log.IdGroup(group), Mode(), count, offNav, spacing, DCO_Util.XZ(fwd * 100.0)));
	}

	protected void Hold(SCR_AIGroup group, DCO_TacticWaypoint wp, float now)
	{
		array<AIAgent> agents = {};
		group.GetAgents(agents);
		array<AIAgent> crew = {};
		foreach (AIAgent a : agents)
		{
			if (a && DCO_Util.IsAlive(a.GetControlledEntity()))
				crew.Insert(a);
		}
		if (crew.Count() != m_aSpots.Count() || m_fSpacing != wp.DCO_Val(DCO_ETacticWPVal.SPACING_M))
		{
			BuildSpots(group, wp, crew.Count());
			m_mRelocAt.Clear();
		}
		vector look = wp.GetOrigin() + wp.DCO_Facing() * 150.0;
		ECharacterStance stance = ECharacterStance.CROUCH;
		if (m_eMode == DCO_ETacticWP.AMBUSH)
			stance = ECharacterStance.PRONE;
		foreach (int i, AIAgent member : crew)
		{
			IEntity e = member.GetControlledEntity();
			if (vector.DistanceXZ(e.GetOrigin(), m_aSpots[i]) <= 10.0)
				continue;
			float at;
			if (m_mRelocAt.Find(e, at) && now < at)
				continue;
			m_mRelocAt.Set(e, now + 20.0);
			SCR_AIUtilityComponent u = SCR_AIUtilityComponent.Cast(member.FindComponent(SCR_AIUtilityComponent));
			if (u && DCO_RelocateBehavior.Start(u, m_aSpots[i], look, true, stance, 60.0, "wp_tactic"))
				DCO_Perf.Count("wp_tactic:hold_spot");
		}
	}

	protected void StopSuppress(SCR_AIGroup group, string why)
	{
		if (!m_bSuppress)
			return;
		m_bSuppress = false;
		if (m_Suppress && m_Suppress.GetActionState() != EAIActionState.FAILED && m_Suppress.GetActionState() != EAIActionState.COMPLETED)
		{
			m_Suppress.SetFailReason(EAIActionFailReason.CANCELLED);
			m_Suppress.Fail(true);
		}
		m_Suppress = null;
		DCO_Perf.Count("wp_tactic:suppress_stop");
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, DCO_Log.IdGroup(group), string.Format("wp_tactic_suppress grp=%1 on=0 why=%2", DCO_Log.IdGroup(group), why));
	}

	protected void UpdateSuppress(SCR_AIGroup group, DCO_TacticWaypoint wp)
	{
		float dist = wp.DCO_Val(DCO_ETacticWPVal.TARGET_M);
		if (dist <= 0)
		{
			StopSuppress(group, "setting_off");
			return;
		}
		vector at = DCO_Util.Surface(wp.GetOrigin() + wp.DCO_Facing() * dist);
		if (m_bSuppress && m_Suppress && vector.DistanceXZ(at, m_vSuppressAt) < 5.0)
			return;
		float now = DCO_Util.NowS();
		if (now < m_fSuppressNextS)
			return;
		m_fSuppressNextS = now + 30.0;
		StopSuppress(group, "restart");
		DCO_LiveSuppressVolume volume = new DCO_LiveSuppressVolume(vector.Zero, vector.Zero);
		volume.InitAt(m_Utility, at, 15.0, 2.0);
		SCR_AISuppressActivity activity = new SCR_AISuppressActivity(m_Utility, m_Waypoint, volume, priorityLevel: wp.GetPriorityLevel());
		m_Utility.AddAction(activity);
		m_Suppress = activity;
		m_vSuppressAt = at;
		m_bSuppress = true;
		DCO_Perf.Count("wp_tactic:suppress_start");
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, DCO_Log.IdGroup(group), string.Format("wp_tactic_suppress grp=%1 on=1 target=%2 dist_m=%3", DCO_Log.IdGroup(group), DCO_Log.Pos(at), Math.Round(dist)));
	}

	protected ENodeResult End(SCR_AIGroup group, string why)
	{
		m_bEnded = true;
		DCO_Perf.Count("wp_tactic:end_" + why);
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, DCO_Log.IdGroup(group), string.Format("wp_tactic_end grp=%1 mode=%2 why=%3 on_wp_s=%4 alive=%5", DCO_Log.IdGroup(group), Mode(), why, Math.Round(DCO_Util.NowS() - m_fStartS), group.GetAgentsCount()));
		Release(why);
		group.CompleteWaypoint(m_Waypoint);
		return ENodeResult.SUCCESS;
	}

	ENodeResult DCO_Tick(SCR_AIGroup group)
	{
		if (m_bEnded)
			return ENodeResult.SUCCESS;
		float now = DCO_Util.NowS();
		if (now < m_fNextS)
			return ENodeResult.RUNNING;
		m_fNextS = now + 1.0;
		DCO_TacticWaypoint wp = DCO_TacticWaypoint.Cast(m_Waypoint);
		DCO_GroupUtilityComponent g = G();
		IEntity leader = group.GetLeaderEntity();
		if (!wp || !g || !leader || !m_bApplied)
			return ENodeResult.RUNNING;
		if (m_fStartS < 0)
		{
			m_fStartS = now;
			if (m_eMode == DCO_ETacticWP.SUPPORT_BY_FIRE)
				g.SetTask(DCO_EGroupTask.SUPPORT_BY_FIRE, "wp_sbf");
			if (m_eMode == DCO_ETacticWP.AMBUSH)
				m_Utility.DCO_ForcePosture(DCO_GroupTactics.AMBUSH, GetGame().GetWorld().GetWorldTime(), 21600000.0, false, "wp_ambush");
			DCO_Perf.Count("wp_tactic:arrive_" + Mode());
			DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, DCO_Log.IdGroup(group), string.Format("wp_tactic_arrive grp=%1 mode=%2 wp=%3 travel_s=%4", DCO_Log.IdGroup(group), Mode(), DCO_Log.Pos(wp.GetOrigin()), Math.Round(now - m_fSelectS)));
		}
		if (m_eMode == DCO_ETacticWP.AMBUSH)
			m_Utility.m_fDCOAmbRangeM = wp.DCO_Val(DCO_ETacticWPVal.AMBUSH_M);
		vector threat;
		float nearest;
		int count;
		bool contact = DCO_PostureOps.FreshContact(m_Utility, leader.GetOrigin(), threat, nearest, count);
		if (contact)
			m_fQuietS = -1;
		else if (m_fQuietS < 0)
			m_fQuietS = now;
		float quiet = 0;
		if (m_fQuietS >= 0)
			quiet = now - m_fQuietS;
		switch (m_eMode)
		{
			case DCO_ETacticWP.ASSAULT:
			{
				if (quiet >= wp.DCO_Val(DCO_ETacticWPVal.CLEAR_S))
					return End(group, "area_clear");
				break;
			}
			case DCO_ETacticWP.SUPPORT_BY_FIRE:
			{
				float hold = wp.DCO_Val(DCO_ETacticWPVal.HOLD_S);
				if (hold > 0 && now - m_fStartS >= hold)
					return End(group, "hold_time");
				UpdateSuppress(group, wp);
				if (!contact)
					Hold(group, wp, now);
				break;
			}
			case DCO_ETacticWP.AMBUSH:
			{
				if (!m_bSprung && m_Utility.m_fDCOAmbSprung_ms >= 0)
				{
					m_bSprung = true;
					DCO_Perf.Count("wp_tactic:ambush_sprung");
					DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, DCO_Log.IdGroup(group), string.Format("wp_tactic_ambush_sprung grp=%1 waited_s=%2 enemies=%3", DCO_Log.IdGroup(group), Math.Round(now - m_fStartS), count));
				}
				if (m_bSprung && quiet >= wp.DCO_Val(DCO_ETacticWPVal.DONE_S))
					return End(group, "ambush_done");
				if (!m_bSprung && !contact)
					Hold(group, wp, now);
				break;
			}
			default:
			{
				return End(group, "arrived");
			}
		}
		if (now - m_fLogS >= 30.0)
		{
			m_fLogS = now;
			DCO_Perf.Count("wp_tactic:tick");
			DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, DCO_Log.IdGroup(group), string.Format("wp_tactic_tick grp=%1 mode=%2 on_wp_s=%3 contact=%4 enemies=%5 quiet_s=%6 sprung=%7", DCO_Log.IdGroup(group), Mode(), Math.Round(now - m_fStartS), DCO_Util.B01(contact), count, Math.Round(quiet), DCO_Util.B01(m_bSprung)));
		}
		return ENodeResult.RUNNING;
	}
}

class DCO_AITacticWaypoint : AITaskScripted
{
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		SCR_AIGroup group = SCR_AIGroup.Cast(owner);
		if (!group)
			return ENodeResult.FAIL;
		SCR_AIGroupUtilityComponent util = SCR_AIGroupUtilityComponent.Cast(group.FindComponent(SCR_AIGroupUtilityComponent));
		if (!util)
			return ENodeResult.FAIL;
		DCO_TacticWaypointState st = DCO_TacticWaypointState.Cast(util.DCO_GetWaypointState());
		if (!st)
			return ENodeResult.FAIL;
		return st.DCO_Tick(group);
	}

	static override bool VisibleInPalette() { return true; }

	static override string GetOnHoverDescription() { return "DCO: tactical waypoint (Assault, Support by Fire, Ambush). Runs the tactic of the group's current DCO tactic waypoint and completes it when the tactic is over."; }
}

class DCO_ConvoyMember
{
	SCR_AIGroup m_Group;
	DCO_GroupUtilityComponent m_G;
	SCR_AIGroupUtilityComponent m_U;
	DCO_TacticWaypoint m_Wp;
	vector m_vDest;
	bool m_bContact;
	float m_fContactS = -1000;
	ref map<IEntity, float> m_mExtra = new map<IEntity, float>();

	float Val(int i)
	{
		if (m_Wp)
			return m_Wp.DCO_Val(i);
		return DCO_TacticWaypoint.DCO_ValDefault(i);
	}

	string Reaction()
	{
		if (Val(DCO_ETacticWPVal.CONTACT) >= 0.5)
			return "halt";
		return "push_through";
	}
}

class DCO_ConvoyUnit
{
	DCO_ConvoyMember m_M;
	IEntity m_Veh;
	float m_fLeft;
}

class DCO_Convoy
{
	protected static ref array<ref DCO_ConvoyMember> s_aMembers;
	protected static ref map<IEntity, float> s_mWaitS;
	protected static ref array<IEntity> s_aBraked;
	protected static BaseWorld s_World;
	protected static float s_fLogS;

	static void Join(SCR_AIGroup grp, DCO_GroupUtilityComponent g, SCR_AIGroupUtilityComponent gu, DCO_TacticWaypoint wp)
	{
		if (!Replication.IsServer() || !grp || !g || !wp)
			return;
		if (!s_aMembers || s_World != GetGame().GetWorld())
		{
			s_aMembers = {};
			s_mWaitS = new map<IEntity, float>();
			s_aBraked = {};
			s_World = GetGame().GetWorld();
			GetGame().GetCallqueue().Remove(Tick);
			GetGame().GetCallqueue().CallLater(Tick, 1000, true);
		}
		Leave(grp);
		DCO_ConvoyMember m = new DCO_ConvoyMember();
		m.m_Group = grp;
		m.m_G = g;
		m.m_U = gu;
		m.m_Wp = wp;
		m.m_vDest = wp.GetOrigin();
		s_aMembers.Insert(m);
		DCO_Perf.Count("convoy:join");
		DCO_Log.Write(DCO_ELogChannel.VEH, DCO_ELogLevel.INFO, DCO_Log.IdGroup(grp), string.Format("convoy_join grp=%1 dest=%2 members=%3 has_vehicle=%4 speed_kmh=%5 gap_m=%6 on_contact=%7", DCO_Log.IdGroup(grp), DCO_Log.Pos(m.m_vDest), s_aMembers.Count(), DCO_Util.B01(g.GetGroupVehicle() != null), m.Val(DCO_ETacticWPVal.SPEED_KMH), m.Val(DCO_ETacticWPVal.GAP_M), m.Reaction()));
	}

	static void Leave(SCR_AIGroup grp)
	{
		if (!s_aMembers)
			return;
		for (int i = s_aMembers.Count() - 1; i >= 0; i--)
		{
			DCO_ConvoyMember m = s_aMembers[i];
			if (m.m_Group != grp)
				continue;
			if (m.m_G)
			{
				m.m_G.DCO_SetConvoyCruise(0);
				foreach (IEntity extra, float kmh : m.m_mExtra)
				{
					m.m_G.DCO_ConvoyCruiseExtra(extra, 0);
				}
			}
			s_aMembers.RemoveOrdered(i);
			DCO_Perf.Count("convoy:leave");
			DCO_Log.Write(DCO_ELogChannel.VEH, DCO_ELogLevel.INFO, DCO_Log.IdGroup(grp), string.Format("convoy_leave grp=%1 members=%2", DCO_Log.IdGroup(grp), s_aMembers.Count()));
		}
	}

	protected static void AddUnit(array<ref DCO_ConvoyUnit> units, DCO_ConvoyMember m, IEntity veh)
	{
		if (!veh)
			return;
		foreach (DCO_ConvoyUnit have : units)
		{
			if (have.m_Veh == veh)
				return;
		}
		DCO_ConvoyUnit unit = new DCO_ConvoyUnit();
		unit.m_M = m;
		unit.m_Veh = veh;
		unit.m_fLeft = vector.DistanceXZ(veh.GetOrigin(), m.m_vDest);
		units.Insert(unit);
	}

	protected static void Brake(IEntity veh, bool on)
	{
		int at = s_aBraked.Find(veh);
		if (on == (at >= 0))
			return;
		CarControllerComponent car;
		if (veh)
			car = CarControllerComponent.Cast(veh.FindComponent(CarControllerComponent));
		if (car)
			car.SetPersistentHandBrake(on);
		if (on)
			s_aBraked.Insert(veh);
		else
			s_aBraked.Remove(at);
		DCO_Perf.Count("convoy:brake_" + DCO_Util.B01(on).ToString());
	}

	protected static bool Moving(IEntity veh)
	{
		Physics phys = veh.GetPhysics();
		return phys && phys.GetVelocity().Length() > 0.5;
	}

	protected static void NoteContact(DCO_ConvoyMember m, float now)
	{
		IEntity leader = m.m_Group.GetLeaderEntity();
		vector center;
		float nearest;
		int count;
		if (leader && m.m_U && DCO_PostureOps.FreshContact(m.m_U, leader.GetOrigin(), center, nearest, count))
			m.m_fContactS = now;
		else
			nearest = -1;
		bool on = now - m.m_fContactS < 30.0;
		if (on == m.m_bContact)
			return;
		m.m_bContact = on;
		DCO_Perf.Count("convoy:contact_" + DCO_Util.B01(on).ToString());
		DCO_Log.Write(DCO_ELogChannel.VEH, DCO_ELogLevel.INFO, DCO_Log.IdGroup(m.m_Group), string.Format("convoy_contact grp=%1 on=%2 enemies=%3 nearest_m=%4 reaction=%5", DCO_Log.IdGroup(m.m_Group), DCO_Util.B01(on), count, Math.Round(nearest), m.Reaction()));
	}

	protected static int Wait(DCO_ConvoyUnit unit, DCO_ConvoyUnit behind, float gap, float waitS, float now)
	{
		float db = vector.DistanceXZ(unit.m_Veh.GetOrigin(), behind.m_Veh.GetOrigin());
		float since;
		bool had = s_mWaitS.Find(unit.m_Veh, since);
		if (waitS <= 0 || db <= gap * 3.0)
		{
			if (had)
				s_mWaitS.Remove(unit.m_Veh);
			return 0;
		}
		string grp = DCO_Log.IdGroup(unit.m_M.m_Group);
		if (!had)
		{
			since = now;
			s_mWaitS.Set(unit.m_Veh, now);
			DCO_Perf.Count("convoy:wait_start");
			DCO_Log.Write(DCO_ELogChannel.VEH, DCO_ELogLevel.INFO, grp, string.Format("convoy_wait grp=%1 for=%2 behind_m=%3 max_s=%4", grp, DCO_Log.IdGroup(behind.m_M.m_Group), Math.Round(db), waitS));
		}
		if (since < 0)
			return 0;
		if (now - since > waitS)
		{
			s_mWaitS.Set(unit.m_Veh, -1);
			DCO_Perf.Count("convoy:wait_timeout");
			DCO_Log.Write(DCO_ELogChannel.VEH, DCO_ELogLevel.INFO, grp, string.Format("convoy_wait_end grp=%1 why=timeout for=%2 behind_m=%3 waited_s=%4", grp, DCO_Log.IdGroup(behind.m_M.m_Group), Math.Round(db), Math.Round(now - since)));
			return 0;
		}
		if (db > gap * 5.0)
			return 2;
		return 1;
	}

	protected static void Tick()
	{
		if (!s_aMembers)
			return;
		for (int i = s_aMembers.Count() - 1; i >= 0; i--)
		{
			if (!s_aMembers[i].m_Group || !s_aMembers[i].m_G)
				s_aMembers.RemoveOrdered(i);
		}
		if (s_aMembers.IsEmpty())
		{
			s_mWaitS.Clear();
			while (!s_aBraked.IsEmpty())
			{
				Brake(s_aBraked[0], false);
			}
			return;
		}
		float now = DCO_Util.NowS();
		array<ref DCO_ConvoyUnit> units = {};
		string text;
		int inContact;
		foreach (DCO_ConvoyMember m : s_aMembers)
		{
			NoteContact(m, now);
			if (m.m_bContact)
				inContact++;
			int before = units.Count();
			AddUnit(units, m, m.m_G.GetGroupVehicle());
			array<AIAgent> agents = {};
			m.m_Group.GetAgents(agents);
			foreach (AIAgent a : agents)
			{
				if (a)
					AddUnit(units, m, DCO_VehicleCombat.GetVehicle(a.GetControlledEntity()));
			}
			if (units.Count() > before)
				continue;
			m.m_G.DCO_SetConvoyCruise(0);
			text += " " + DCO_Log.IdGroup(m.m_Group) + ":no_vehicle";
		}
		float waitS = DCO_Ctx.TuneG("convoy.wait_s", 60.0);
		foreach (DCO_ConvoyUnit u : units)
		{
			DCO_ConvoyMember um = u.m_M;
			float gap = um.Val(DCO_ETacticWPVal.GAP_M);
			float lead = um.Val(DCO_ETacticWPVal.SPEED_KMH);
			bool contact = um.m_bContact;
			DCO_ConvoyUnit ahead;
			DCO_ConvoyUnit behind;
			foreach (DCO_ConvoyUnit o : units)
			{
				if (o == u || vector.DistanceXZ(o.m_M.m_vDest, um.m_vDest) > 50.0)
					continue;
				if (o.m_M.m_bContact)
					contact = true;
				if (o.m_fLeft < u.m_fLeft - 2.0)
				{
					if (!ahead || o.m_fLeft > ahead.m_fLeft)
						ahead = o;
				}
				else if (o.m_fLeft > u.m_fLeft + 2.0)
				{
					if (!behind || o.m_fLeft < behind.m_fLeft)
						behind = o;
				}
			}
			float want = lead;
			float d = -1;
			string st = "lead";
			if (ahead && Moving(ahead.m_Veh))
			{
				st = "follow";
				d = vector.DistanceXZ(u.m_Veh.GetOrigin(), ahead.m_Veh.GetOrigin());
				if (d < gap * 0.6)
					want = 5.0;
				else if (d < gap)
					want = lead * 0.7;
				else if (d > gap * 2.0)
					want = lead * 1.4;
			}
			int wait;
			if (!contact && behind)
				wait = Wait(u, behind, gap, waitS, now);
			if (contact && um.Val(DCO_ETacticWPVal.CONTACT) >= 0.5)
			{
				want = 5.0;
				st = "halt";
			}
			else if (contact)
			{
				if (d < 0 || d >= gap * 0.6)
					want = lead * 1.3;
				st = "push";
			}
			else if (wait == 2)
			{
				want = 5.0;
				st = "wait";
			}
			else if (wait == 1)
			{
				want = Math.Min(want, lead * 0.5);
				st = "slow";
			}
			Brake(u.m_Veh, st == "halt");
			want = Math.Round(want);
			if (u.m_Veh == um.m_G.GetGroupVehicle())
			{
				um.m_G.DCO_SetConvoyCruise(want);
			}
			else
			{
				float last;
				if (!um.m_mExtra.Find(u.m_Veh, last) || last != want)
				{
					um.m_mExtra.Set(u.m_Veh, want);
					um.m_G.DCO_ConvoyCruiseExtra(u.m_Veh, want);
				}
			}
			text += string.Format(" %1:%2:%3:gap%4:left%5", DCO_Log.IdGroup(um.m_Group), want, st, Math.Round(d), Math.Round(u.m_fLeft));
		}
		for (int b = s_aBraked.Count() - 1; b >= 0; b--)
		{
			IEntity braked = s_aBraked[b];
			bool have = false;
			foreach (DCO_ConvoyUnit hu : units)
			{
				if (hu.m_Veh == braked)
					have = true;
			}
			if (!have)
				Brake(braked, false);
		}
		if (now - s_fLogS < 15.0)
			return;
		s_fLogS = now;
		DCO_Perf.Count("convoy:tick");
		DCO_Log.Write(DCO_ELogChannel.VEH, DCO_ELogLevel.INFO, "-", string.Format("convoy_tick members=%1 vehicles=%2 in_contact=%3 wait_s=%4 units=%5", s_aMembers.Count(), units.Count(), inContact, waitS, text));
	}
}
