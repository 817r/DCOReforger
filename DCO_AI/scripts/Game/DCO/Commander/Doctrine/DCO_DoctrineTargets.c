class DCO_HVT
{
	static string Kind(AICommander_BaseComponent cmd, CMD_ThreatEntry threat)
	{
		if (!cmd || !threat)
			return string.Empty;
		if (threat.m_iArmor > 0 || threat.m_bArmorSeen)
			return "armor";
		if (threat.m_bArtillery)
			return "mortar";
		vector pos = threat.m_vBelievedPos;
		FactionKey own = cmd.GetCommanderFactionKey();
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (mgr)
		{
			foreach (AICommander_BaseComponent other : mgr.m_aCommander)
			{
				if (other && other != cmd && other.GetOwner() && other.GetCommanderFactionKey() != own && vector.DistanceXZ(other.GetOwner().GetOrigin(), pos) < 75.0)
					return "hq";
			}
		}
		array<DCO_LogisticsHubComponent> hubs = DCO_LogisticsHubComponent.GetHubs();
		if (hubs)
		{
			foreach (DCO_LogisticsHubComponent hub : hubs)
			{
				if (hub && hub.GetOwner() && !hub.GetFactionKey().IsEmpty() && hub.GetFactionKey() != own && vector.DistanceXZ(hub.GetOwner().GetOrigin(), pos) < 75.0)
					return "hub";
			}
		}
		return string.Empty;
	}

	static void Log(AICommander_BaseComponent cmd, CMD_ThreatEntry threat, string kind, string action)
	{
		DCO_Perf.Count("doctrine:hvt:" + kind + ":" + action);
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(cmd), "hvt_target cmd=" + cmd.GetCommanderUID() + " kind=" + kind + " action=" + action + " pos=" + threat.m_vBelievedPos.ToString());
	}

	static bool NearObjective(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, out string kind)
	{
		CMD_ThreatResponseComponent tr = cmd.GetThreatResponseComponent();
		if (!tr || !obj || !obj.GetOwner())
			return false;
		vector objPos = obj.GetOwner().GetOrigin();
		float radius = obj.GetRadius() + 200.0;
		float now = DCO_Util.NowS();
		foreach (CMD_ThreatEntry t : tr.GetThreats())
		{
			if (!t || now - t.m_fLastUpdateTime > 300.0 || vector.DistanceXZ(t.m_vBelievedPos, objPos) > radius)
				continue;
			kind = Kind(cmd, t);
			if (!kind.IsEmpty())
				return true;
		}
		return false;
	}
}

class DCO_TerrainObj
{
	CMD_AICommanderObjectiveComponent m_For;
	vector m_vPos;
	DCO_GroupUtilityComponent m_Group;
	bool m_bOccupied;
	float m_fCreated;
	float m_fIdleSince;
}

class DCO_TerrainObjectives
{
	protected ref array<ref DCO_TerrainObj> m_aObjs = {};

	int CountFor(CMD_AICommanderObjectiveComponent obj)
	{
		int n;
		foreach (DCO_TerrainObj t : m_aObjs)
		{
			if (t.m_For == obj)
				n++;
		}
		return n;
	}

	bool OccupiedFor(CMD_AICommanderObjectiveComponent obj, out vector pos)
	{
		foreach (DCO_TerrainObj t : m_aObjs)
		{
			if (t.m_For == obj && t.m_bOccupied)
			{
				pos = t.m_vPos;
				return true;
			}
		}
		return false;
	}

	protected static void Log(AICommander_BaseComponent cmd, DCO_TerrainObj t, string action, string reason)
	{
		DCO_Perf.Count("doctrine:terrain_obj:" + action);
		string forName = "none";
		if (t.m_For)
			forName = t.m_For.GetLogName();
		string grp = "none";
		if (t.m_Group && t.m_Group.GetOwner())
			grp = DCO_Log.IdGroup(AIGroup.Cast(t.m_Group.GetOwner()));
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(cmd), "terrain_obj cmd=" + cmd.GetCommanderUID() + " for=" + forName + " action=" + action + " pos=" + t.m_vPos.ToString() + " grp=" + grp + " reason=" + reason);
	}

	void OnAttackPlanned(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, float now)
	{
		if (!obj || !obj.GetOwner() || m_aObjs.Count() >= 5 || CountFor(obj) >= 2)
			return;
		float chance = Math.Clamp(cmd.GetTraitStrategy() * 0.3, 0, 0.8);
		if (Math.RandomFloat01() >= chance)
			return;
		vector objPos = obj.GetOwner().GetOrigin();
		array<DCO_TerrainPoint> pts = {};
		DCO_TerrainCache.Query(DCO_ETerrainFlag.HILLTOP | DCO_ETerrainFlag.OVERWATCH, objPos, 600.0, pts, obj.GetOwner().GetName());
		vector best = vector.Zero;
		float bestScore = -float.MAX;
		foreach (DCO_TerrainPoint p : pts)
		{
			float dist = vector.DistanceXZ(p.m_vPos, objPos);
			if (dist < 300.0 || p.m_fRelElev < 3.0)
				continue;
			bool crowded = false;
			foreach (DCO_TerrainObj other : m_aObjs)
			{
				if (vector.DistanceXZ(other.m_vPos, p.m_vPos) < 150.0)
					crowded = true;
			}
			if (crowded || DCO_DoctrineSupport.EnemyWithin(cmd, p.m_vPos, 200.0, now))
				continue;
			float score = p.m_fRelElev - Math.AbsFloat(dist - 450.0) / 50.0;
			if (score > bestScore)
			{
				bestScore = score;
				best = p.m_vPos;
			}
		}
		DCO_TerrainObj t = new DCO_TerrainObj();
		t.m_For = obj;
		t.m_fCreated = now;
		t.m_fIdleSince = -1;
		if (best == vector.Zero)
		{
			Log(cmd, t, "skip", "no_candidate_" + pts.Count().ToString());
			return;
		}
		t.m_vPos = best;
		DCO_CommanderDefense def = cmd.GetDefense();
		if (def)
			t.m_Group = def.PickReserve(cmd, best, false, true, null, 1500.0);
		if (!t.m_Group)
		{
			Log(cmd, t, "skip", "no_group");
			return;
		}
		t.m_Group.CompleteAllWaypoints();
		t.m_Group.SetTask(DCO_EGroupTask.SUPPORT_BY_FIRE);
		cmd.SpawnMoveRoute(t.m_Group, t.m_Group.GetOwner().GetOrigin(), best, now);
		m_aObjs.Insert(t);
		Log(cmd, t, "create", "strategy_" + cmd.GetTraitStrategy().ToString(-1, 1));
	}

	void Update(AICommander_BaseComponent cmd, DCO_BattlePlanner planner, float now)
	{
		FactionKey fk = cmd.GetCommanderFactionKey();
		for (int i = m_aObjs.Count() - 1; i >= 0; i--)
		{
			DCO_TerrainObj t = m_aObjs[i];
			string reason = string.Empty;
			if (!t.m_For || !t.m_For.GetOwner())
				reason = "objective_gone";
			else if (!t.m_Group || !t.m_Group.GetOwner() || t.m_Group.GetUnitCount() == 0)
				reason = "unguarded";
			if (reason.IsEmpty())
			{
				vector groupPos = t.m_Group.GetOwner().GetOrigin();
				if (!t.m_bOccupied && vector.DistanceXZ(groupPos, t.m_vPos) < 60.0 && !DCO_DoctrineSupport.EnemyWithin(cmd, t.m_vPos, 100.0, now))
				{
					t.m_bOccupied = true;
					vector face = t.m_For.GetOwner().GetOrigin() - t.m_vPos;
					DCO_GarrisonWaypoint gw = cmd.SpawnGarrisonWP(DCO_Util.Surface(t.m_vPos), Math.Atan2(face[2], face[0]), 30, true);
					if (gw)
						t.m_Group.MoveTo(gw, now);
					Log(cmd, t, "occupy", "arrived");
				}
				bool active = t.m_For.GetObjectiveState(fk) == CMD_EObjectiveState.ASSIGNED;
				DCO_BattleState st = planner.Peek(t.m_For);
				if (st && st.m_bPlanActive)
					active = true;
				if (active)
					t.m_fIdleSince = -1;
				else if (t.m_fIdleSince < 0)
					t.m_fIdleSince = now;
				else if (now - t.m_fIdleSince > 300.0)
					reason = "operation_done";
			}
			if (reason.IsEmpty())
				continue;
			if (t.m_Group && t.m_Group.GetOwner())
			{
				t.m_Group.CompleteAllWaypoints();
				t.m_Group.SetTask(DCO_EGroupTask.NONE);
			}
			Log(cmd, t, "remove", reason);
			m_aObjs.Remove(i);
		}
	}
}
