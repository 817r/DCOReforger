enum DCO_EPlayerRequest
{
	SUPPRESS,
	SUPPORT,
	FLANK_LEFT,
	FLANK_RIGHT,
	ARMOR
}

class DCO_SupportLink
{
	DCO_GroupUtilityComponent m_Squad;
	int m_iPlayerGroup;
	DCO_EPlayerRequest m_eType;
	vector m_vTarget;
	float m_fExpire_s;
}

class DCO_SquadWatch
{
	int m_iLastUnits = -1;
	bool m_bArrived;
	bool m_bIneffective;
	float m_fLastReport_s = -1000;
	float m_fLastContact_s = -1000;
}

class DCO_PlayerRequests
{
	protected static const float REQUEST_COOLDOWN_S = 60;
	protected static const float LINK_DURATION_S = 300;
	protected static const float SQUAD_REPORT_COOLDOWN_S = 30;
	protected static const float CONTACT_REPORT_COOLDOWN_S = 90;
	protected static const float TICK_S = 5;
	protected static const float SUPPRESS_STANDOFF_M = 200;
	protected static const float SUPPORT_STANDOFF_M = 30;
	protected static const float FLANK_OFFSET_M = 150;
	protected static const float ARRIVED_M = 50;
	protected static const float FIRE_COOLDOWN_S = 180;
	protected static const float NOISE_PER_100M = 12;
	protected static const float ARMOR_STANDOFF_M = 175;
	protected static const float ARMOR_AT_STANDOFF_M = 350;
	protected static const float ARMOR_AT_WARN_M = 300;

	protected static ref DCO_PlayerRequests s_Instance;

	protected ref array<ref DCO_SupportLink> m_aLinks = {};
	protected ref map<int, float> m_mLastRequest = new map<int, float>();
	protected ref map<int, float> m_mLastFire = new map<int, float>();
	protected ref map<DCO_GroupUtilityComponent, ref DCO_SquadWatch> m_mWatch = new map<DCO_GroupUtilityComponent, ref DCO_SquadWatch>();
	protected float m_fTimer;

	static DCO_PlayerRequests Get()
	{
		if (!s_Instance)
			s_Instance = new DCO_PlayerRequests();
		return s_Instance;
	}

	static void Handle(int pid, DCO_EPlayerRequest type, vector pos)
	{
		if (Replication.IsServer())
			Get().DoHandle(pid, type, pos);
	}

	static void HandleFire(int pid, SCR_EAIArtilleryAmmoType ammo, vector pos)
	{
		if (Replication.IsServer())
			Get().DoHandleFire(pid, ammo, pos);
	}

	static void HandleCancel(int pid, bool support, bool fire, bool transport)
	{
		if (Replication.IsServer())
			Get().DoHandleCancel(pid, support, fire, transport);
	}

	protected void DoHandleFire(int pid, SCR_EAIArtilleryAmmoType ammo, vector pos)
	{
		float now = Now();
		SCR_GroupsManagerComponent groups = SCR_GroupsManagerComponent.GetInstance();
		SCR_AIGroup pgrp;
		if (groups)
			pgrp = groups.GetPlayerGroup(pid);
		AICommander_BaseComponent cmd = DCO_PlayerContactReports.FindCommanderForPlayer(pid);
		if (!pgrp || !cmd)
		{
			DCO_Radio.Player(pid, "FIRE SUPPORT", "no_net");
			return;
		}

		int gid = pgrp.GetGroupID();
		string typeName = typename.EnumToString(SCR_EAIArtilleryAmmoType, ammo);
		float last;
		if (m_mLastFire.Find(gid, last) && now - last < FIRE_COOLDOWN_S)
		{
			FireDenied(gid, pid, typeName, "-", 0, "cooldown", FIRE_COOLDOWN_S - (now - last));
			return;
		}

		CMD_ArtillerySupport arty = cmd.GetArtySupport();
		if (!arty || !arty.HasRegisteredUnits())
		{
			FireDenied(gid, pid, typeName, "-", 0, "no_unit");
			return;
		}

		IEntity player = GetGame().GetPlayerManager().GetPlayerControlledEntity(pid);
		vector ppos = pos;
		if (player)
			ppos = player.GetOrigin();

		float noise = vector.DistanceXZ(ppos, pos) / 100 * NOISE_PER_100M;
		float unc = noise * 1.5 + 10;
		string tier = arty.ResolveTier(unc);
		bool he = ammo == SCR_EAIArtilleryAmmoType.HIGH_EXPLOSIVE;
		if (tier == "none")
		{
			if (he)
			{
				FireDenied(gid, pid, typeName, tier, unc, "observe");
				return;
			}
			tier = "area";
			unc = arty.GetAreaUncertainty();
		}

		int shells = 4;
		if (ammo == SCR_EAIArtilleryAmmoType.SMOKE)
			shells = 3;
		else if (ammo == SCR_EAIArtilleryAmmoType.ILLUMINATION)
			shells = 1;

		CMD_FireMissionRequest req = new CMD_FireMissionRequest(pos, ammo, now, shells);
		arty.ApplyTier(req, tier, unc, "player");
		req.m_bDangerClose = true;
		req.m_Requester = DCO_GroupUtilityComponent.Cast(pgrp.FindComponent(DCO_GroupUtilityComponent));
		req.m_iPlayerGroup = gid;

		if (he && arty.HasFriendlyNearRequest(req, now))
		{
			FireDenied(gid, pid, typeName, tier, unc, "friendly");
			return;
		}

		if (!arty.RequestShellImpact(req, now, shells))
		{
			FireDenied(gid, pid, typeName, tier, unc, req.m_sDeny);
			return;
		}

		m_mLastFire.Set(gid, now);
		DCO_Radio.Group(gid, "FIRE SUPPORT", "fire_received", DCO_ERadioKind.INFO, DCO_Radio.P("count", req.m_iShellCount.ToString(), "type", CMD_ArtillerySupport.ShellKey(ammo), "grid", DCO_PlayerComms.Grid(pos)));
		DCO_BenchmarkLoggerComponent.Event(string.Format("player_fire_request player=%1 grp=%2 type=%3 tier=%4 unc=%5 result=accepted shells=%6", pid, gid, typeName, tier, Math.Round(unc), req.m_iShellCount));
	}

	protected void FireDenied(int gid, int pid, string typeName, string tier, float unc, string reason, float sec = 0)
	{
		if (reason == "cooldown")
			DCO_Radio.Group(gid, "FIRE SUPPORT", "fire_deny_cooldown", DCO_ERadioKind.WARNING, DCO_Radio.P("sec", DCO_Radio.N(sec)));
		else
			DCO_Radio.Group(gid, "FIRE SUPPORT", "fire_deny", DCO_ERadioKind.WARNING, DCO_Radio.P("reason", CMD_ArtillerySupport.DenyKey(reason)));
		DCO_BenchmarkLoggerComponent.Event(string.Format("player_fire_request player=%1 grp=%2 type=%3 tier=%4 unc=%5 result=denied reason=\"%6\"", pid, gid, typeName, tier, Math.Round(unc), reason));
	}

	protected void DoHandleCancel(int pid, bool support, bool fire, bool transport)
	{
		SCR_GroupsManagerComponent groups = SCR_GroupsManagerComponent.GetInstance();
		SCR_AIGroup pgrp;
		if (groups)
			pgrp = groups.GetPlayerGroup(pid);
		AICommander_BaseComponent cmd = DCO_PlayerContactReports.FindCommanderForPlayer(pid);
		if (!pgrp || !cmd)
		{
			DCO_Radio.Player(pid, "COMMANDER", "no_net");
			return;
		}

		int gid = pgrp.GetGroupID();
		int links, missions;
		bool job;

		if (support)
		{
			for (int i = m_aLinks.Count() - 1; i >= 0; i--)
			{
				DCO_SupportLink l = m_aLinks[i];
				if (l.m_iPlayerGroup != gid)
					continue;
				if (l.m_Squad && l.m_Squad.GetSupportPlayerGroup() == gid)
				{
					l.m_Squad.SetSupportPlayerGroup(-1);
					l.m_Squad.CompleteAllWaypoints();
					l.m_Squad.SetTask(DCO_EGroupTask.NONE);
					DCO_Radio.Group(gid, SquadName(l.m_Squad), "support_cancelled", DCO_ERadioKind.REPORT);
				}
				m_aLinks.Remove(i);
				links++;
			}
		}

		if (fire && cmd.GetArtySupport())
			missions = cmd.GetArtySupport().CancelPlayerMissions(gid);

		if (transport && cmd.GetLogistics())
			job = cmd.GetLogistics().CancelPlayerJob(pgrp);

		bool all = support && fire && transport;
		bool sent = links > 0;
		if (fire && missions > 0)
		{
			DCO_Radio.Group(gid, "FIRE SUPPORT", "cancel_fire_done", DCO_ERadioKind.INFO, DCO_Radio.P("count", missions.ToString()));
			sent = true;
		}
		else if (fire && !all)
		{
			DCO_Radio.Group(gid, "FIRE SUPPORT", "cancel_fire_none");
			sent = true;
		}
		if (transport && job)
		{
			DCO_Radio.Group(gid, "LOGISTICS", "cancel_transport_done");
			sent = true;
		}
		else if (transport && !all)
		{
			DCO_Radio.Group(gid, "LOGISTICS", "cancel_transport_none");
			sent = true;
		}
		if (support && !all && links == 0)
		{
			DCO_Radio.Group(gid, "COMMANDER", "cancel_support_none");
			sent = true;
		}
		if (!sent)
			DCO_Radio.Group(gid, "COMMANDER", "cancel_nothing");
		DCO_BenchmarkLoggerComponent.Event(string.Format("player_cancel player=%1 grp=%2 support=%3 fire=%4 transport=%5 links=%6 missions=%7 job=%8", pid, gid, support, fire, transport, links, missions, job));
	}

	protected bool HasATNear(AICommander_BaseComponent cmd, vector pos)
	{
		CMD_ThreatResponseComponent threat = cmd.GetThreatResponseComponent();
		if (!threat)
			return false;
		float rSq = ARMOR_AT_WARN_M * ARMOR_AT_WARN_M;
		foreach (CMD_ThreatEntry t : threat.GetThreats())
		{
			if (t && t.m_bATSeen && vector.DistanceSqXZ(t.m_vPosition, pos) <= rSq)
				return true;
		}
		return false;
	}

	static void Tick(float timeSlice)
	{
		DCO_PlayerRequests r = Get();
		r.m_fTimer += timeSlice;
		if (r.m_fTimer < TICK_S)
			return;
		r.m_fTimer = 0;
		r.UpdateLinks();
		r.UpdateReports();
	}

	protected void DoHandle(int pid, DCO_EPlayerRequest type, vector pos)
	{
		float now = Now();
		SCR_GroupsManagerComponent groups = SCR_GroupsManagerComponent.GetInstance();
		if (!groups)
			return;

		SCR_AIGroup pgrp = groups.GetPlayerGroup(pid);
		AICommander_BaseComponent cmd = DCO_PlayerContactReports.FindCommanderForPlayer(pid);
		if (!pgrp || !cmd)
		{
			DCO_Radio.Player(pid, "COMMANDER", "no_net");
			return;
		}

		int gid = pgrp.GetGroupID();
		float last;
		if (m_mLastRequest.Find(gid, last) && now - last < REQUEST_COOLDOWN_S)
		{
			DCO_Radio.Player(pid, "COMMANDER", "request_wait", DCO_ERadioKind.WARNING, DCO_Radio.P("sec", DCO_Radio.N(REQUEST_COOLDOWN_S - (now - last))));
			return;
		}

		IEntity player = GetGame().GetPlayerManager().GetPlayerControlledEntity(pid);
		vector ppos = pos;
		if (player)
			ppos = player.GetOrigin();

		DCO_EGroupTask task;
		vector movePos, suppressPos;
		string okKey = "support_flank_ok";
		string flankSide;
		switch (type)
		{
			case DCO_EPlayerRequest.SUPPRESS:
			{
				task = DCO_EGroupTask.SUPPORT_BY_FIRE;
				movePos = Standoff(pos, ppos, SUPPRESS_STANDOFF_M);
				suppressPos = pos;
				okKey = "support_suppress_ok";
				break;
			}
			case DCO_EPlayerRequest.ARMOR:
			{
				task = DCO_EGroupTask.SUPPORT_BY_FIRE;
				float standoff = ARMOR_STANDOFF_M;
				if (HasATNear(cmd, pos))
				{
					standoff = ARMOR_AT_STANDOFF_M;
					DCO_Radio.Group(gid, "COMMANDER", "armor_at_warning", DCO_ERadioKind.WARNING, null, true);
				}
				movePos = Standoff(pos, ppos, standoff);
				vector cover = DCO_TerrainCache.QueryBest(DCO_ETerrainFlag.HILLTOP | DCO_ETerrainFlag.OVERWATCH | DCO_ETerrainFlag.FOREST_EDGE, movePos, 75, string.Empty, movePos);
				if (cover != vector.Zero)
					movePos = cover;
				suppressPos = pos;
				okKey = "support_armor_ok";
				break;
			}
			case DCO_EPlayerRequest.SUPPORT:
			{
				task = DCO_EGroupTask.REINFORCE;
				movePos = ppos;
				okKey = "support_support_ok";
				break;
			}
			default:
			{
				task = DCO_EGroupTask.FLANK;
				vector dir = pos - ppos;
				dir[1] = 0;
				dir.Normalize();
				vector side = Vector(-dir[2], 0, dir[0]);
				if (type == DCO_EPlayerRequest.FLANK_RIGHT)
					side = -side;
				movePos = pos + side * FLANK_OFFSET_M;
				suppressPos = pos;
				flankSide = DCO_Radio.Dir(pos, movePos);
				break;
			}
		}
		movePos[1] = GetGame().GetWorld().GetSurfaceY(movePos[0], movePos[2]);

		bool armor = type == DCO_EPlayerRequest.ARMOR;
		DCO_GroupUtilityComponent squad = cmd.FindBestIdleGroupForTask_Public(task, movePos, -1, armor, 2);
		if (!squad || !cmd.CanCommitGroup(squad))
		{
			string none = "support_none";
			if (armor)
				none = "armor_none";
			DCO_Radio.Group(gid, "COMMANDER", none, DCO_ERadioKind.WARNING);
			Log(pid, type, pos, null, false);
			return;
		}

		if (!cmd.SendSupportSquad(squad, task, movePos, suppressPos))
		{
			DCO_Radio.Group(gid, "COMMANDER", "support_no_route", DCO_ERadioKind.WARNING);
			Log(pid, type, pos, squad, false);
			return;
		}

		m_mLastRequest.Set(gid, now);
		squad.SetSupportPlayerGroup(gid);

		DCO_SupportLink link = new DCO_SupportLink();
		link.m_Squad = squad;
		link.m_iPlayerGroup = gid;
		link.m_eType = type;
		link.m_vTarget = movePos;
		link.m_fExpire_s = now + LINK_DURATION_S;
		m_aLinks.Insert(link);

		vector squadPos = squad.GetOwner().GetOrigin();
		float eta = vector.Distance(squadPos, movePos) / 2.5;
		array<string> params = DCO_Radio.P("callsign", DCO_PlayerComms.GetCallsign(SCR_AIGroup.Cast(squad.GetOwner())), "grid", DCO_PlayerComms.Grid(pos), "eta", DCO_Radio.N(Math.Max(1, eta / 60)),
			"dir", DCO_Radio.Dir(ppos, squadPos), "dist", DCO_Radio.N(vector.DistanceXZ(ppos, squadPos)));
		if (!flankSide.IsEmpty())
		{
			params.Insert("side");
			params.Insert(flankSide);
		}
		DCO_Radio.Group(gid, "COMMANDER", okKey, DCO_ERadioKind.INFO, params);
		Log(pid, type, pos, squad, true);
	}

	protected void UpdateLinks()
	{
		float now = Now();
		for (int i = m_aLinks.Count() - 1; i >= 0; i--)
		{
			DCO_SupportLink l = m_aLinks[i];
			if (l.m_Squad && now < l.m_fExpire_s && l.m_Squad.GetSupportPlayerGroup() == l.m_iPlayerGroup)
				continue;

			if (l.m_Squad && l.m_Squad.GetSupportPlayerGroup() == l.m_iPlayerGroup)
			{
				l.m_Squad.SetSupportPlayerGroup(-1);
				l.m_Squad.SetTask(DCO_EGroupTask.NONE);
				DCO_Radio.Group(l.m_iPlayerGroup, SquadName(l.m_Squad), "support_complete", DCO_ERadioKind.REPORT);
			}
			m_aLinks.Remove(i);
		}
	}

	protected void UpdateReports()
	{
		map<DCO_GroupUtilityComponent, ref array<int>> listeners = new map<DCO_GroupUtilityComponent, ref array<int>>();
		map<DCO_GroupUtilityComponent, vector> targets = new map<DCO_GroupUtilityComponent, vector>();

		foreach (DCO_SupportLink l : m_aLinks)
		{
			if (!l.m_Squad)
				continue;
			AddListener(listeners, l.m_Squad, l.m_iPlayerGroup);
			targets.Set(l.m_Squad, l.m_vTarget);
		}

		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (mgr)
		{
			foreach (AICommander_BaseComponent cmd : mgr.m_aCommander)
			{
				if (!cmd)
					continue;

				foreach (DCO_GroupUtilityComponent grp : cmd.GetOwnedGroups())
				{
					if (!grp || !grp.GetGroupObjective())
						continue;

					array<int> ids = {};
					cmd.GetPlayerTasking().GetGroupsOnObjective(grp.GetGroupObjective(), ids);
					foreach (int id : ids)
						AddListener(listeners, grp, id);
				}
			}
		}

		float now = Now();
		foreach (DCO_GroupUtilityComponent squad, array<int> ids : listeners)
		{
			if (!squad)
				continue;

			DCO_SquadWatch w = m_mWatch.Get(squad);
			if (!w)
			{
				w = new DCO_SquadWatch();
				w.m_iLastUnits = squad.GetUnitCount();
				m_mWatch.Insert(squad, w);
			}

			string msg;
			array<string> msgParams;
			bool critical;
			DCO_ERadioKind kind = DCO_ERadioKind.REPORT;
			int units = squad.GetUnitCount();
			int st = squad.GetState();

			if (!w.m_bIneffective && (st & (DCO_EGroupState.COMBAT_INEFFECTIVE | DCO_EGroupState.RETREATING)))
			{
				w.m_bIneffective = true;
				msg = "squad_ineffective";
				kind = DCO_ERadioKind.WARNING;
				critical = true;
			}
			else if (units < w.m_iLastUnits && units > 0)
			{
				msg = "squad_casualties";
				msgParams = DCO_Radio.P("count", units.ToString());
				critical = (st & DCO_EGroupState.IN_CONTACT) != 0;
			}
			else if ((st & DCO_EGroupState.IN_CONTACT) && now - w.m_fLastContact_s > CONTACT_REPORT_COOLDOWN_S)
			{
				w.m_fLastContact_s = now;
				msg = "squad_contact";
			}
			else if (!w.m_bArrived && !squad.IsMoving())
			{
				vector tgt;
				bool hasTgt = targets.Find(squad, tgt);
				if (!hasTgt && squad.GetGroupObjective())
					tgt = squad.GetGroupObjective().GetOwner().GetOrigin();
				if (vector.DistanceXZ(squad.GetOwner().GetOrigin(), tgt) <= ARRIVED_M || (!hasTgt && squad.GetPhase() == DCO_ETaskPhase.HOLDING))
				{
					w.m_bArrived = true;
					msg = "squad_in_position";
				}
			}
			w.m_iLastUnits = units;

			if (msg.IsEmpty() || now - w.m_fLastReport_s < SQUAD_REPORT_COOLDOWN_S)
				continue;

			w.m_fLastReport_s = now;
			foreach (int id : ids)
				DCO_Radio.Group(id, SquadName(squad), msg, kind, msgParams, critical);
		}

		for (int i = m_mWatch.Count() - 1; i >= 0; i--)
		{
			if (!listeners.Contains(m_mWatch.GetKey(i)))
				m_mWatch.RemoveElement(i);
		}
	}

	protected static void AddListener(map<DCO_GroupUtilityComponent, ref array<int>> listeners, DCO_GroupUtilityComponent squad, int id)
	{
		array<int> ids = listeners.Get(squad);
		if (!ids)
		{
			ids = {};
			listeners.Insert(squad, ids);
		}
		if (!ids.Contains(id))
			ids.Insert(id);
	}

	protected static string SquadName(DCO_GroupUtilityComponent squad)
	{
		return DCO_PlayerComms.GetCallsign(SCR_AIGroup.Cast(squad.GetOwner()));
	}

	protected static vector Standoff(vector target, vector from, float dist)
	{
		vector d = from - target;
		d[1] = 0;
		if (d.Length() < 1)
			d = Vector(0, 0, 1);
		d.Normalize();
		return target + d * dist;
	}

	protected static void Log(int pid, DCO_EPlayerRequest type, vector pos, DCO_GroupUtilityComponent squad, bool accepted)
	{
		string name = "-";
		if (squad)
			name = squad.GetOwner().GetName();
		string line = string.Format("player_request player=%1 type=%2 pos=%3 squad=%4 accepted=%5", pid, typename.EnumToString(DCO_EPlayerRequest, type), pos, name, accepted);
		Print("[DCO_PlayerRequest] " + line);
		DCO_BenchmarkLoggerComponent.Event(line);
	}

	protected static float Now()
	{
		return GetGame().GetWorld().GetWorldTime() / 1000.0;
	}
}
