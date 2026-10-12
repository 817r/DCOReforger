class DCO_SmokeDrillState
{
	ref map<int, float> m_mWatchNext = new map<int, float>();
	ref map<IEntity, float> m_mExitNext = new map<IEntity, float>();
	float m_fMGNext_ms;
}

modded class SCR_AIGroupUtilityComponent
{
	protected ref DCO_SmokeDrillState m_DCOSmoke;
	protected float m_fDCOSmokeTick_ms;

	void DCO_TickSmoke(float now_ms)
	{
		if (now_ms < m_fDCOSmokeTick_ms || !DCO_SmokeField.Any() || !Replication.IsServer())
			return;
		m_fDCOSmokeTick_ms = now_ms + 2000.0;
		if (!m_DCOSmoke)
			m_DCOSmoke = new DCO_SmokeDrillState();
		int t = DCO_Perf.Begin();
		DCO_SmokeDrill.Tick(this, SCR_AIGroup.Cast(GetOwner()), m_DCOSmoke, now_ms);
		DCO_Perf.End("grp_smoke", t);
	}
}

class DCO_SmokeDrill
{
	static void Tick(SCR_AIGroupUtilityComponent u, SCR_AIGroup grp, DCO_SmokeDrillState st, float now_ms)
	{
		if (!grp || DCO_Util.IsPlayerGroup(grp))
			return;
		array<DCO_SmokeCloud> clouds = {};
		DCO_SmokeField.GetActive(clouds, 3.0);
		if (clouds.IsEmpty())
			return;

		array<AIAgent> agents = {};
		grp.GetAgents(agents);
		array<SCR_AIUtilityComponent> mates = {};
		vector center;
		foreach (AIAgent a : agents)
		{
			SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
			if (!ca || !ca.m_UtilityComponent)
				continue;
			ChimeraCharacter c = ChimeraCharacter.Cast(ca.GetControlledEntity());
			if (!c || c.IsInVehicle() || !DCO_Util.IsAlive(c))
				continue;
			mates.Insert(ca.m_UtilityComponent);
			center = center + c.GetOrigin();
		}
		if (mates.IsEmpty())
			return;
		center = center * (1.0 / mates.Count());

		array<vector> enemies = {};
		PerceptionManager pm = GetGame().GetPerceptionManager();
		if (u.m_Perception && pm)
		{
			float pmNow = pm.GetTime();
			foreach (SCR_AITargetInfo tgt : u.m_Perception.m_aTargets)
			{
				if (tgt && tgt.m_Entity && pmNow - tgt.m_fTimestamp <= 60.0)
					enemies.Insert(tgt.m_vWorldPos);
			}
		}

		foreach (DCO_SmokeCloud cloud : clouds)
		{
			float dc = vector.DistanceXZ(center, cloud.m_vPos);
			if (dc > 250.0)
				continue;
			float r = DCO_SmokeField.Radius(cloud, now_ms);
			bool between;
			bool nearEnemy;
			foreach (vector e : enemies)
			{
				if (vector.DistanceXZ(e, cloud.m_vPos) < 60.0)
					nearEnemy = true;
				if (DCO_SmokeUtility.SegDistXZ(center, e, cloud.m_vPos) < r + 10.0)
					between = true;
			}
			if (!between && !nearEnemy && dc > 150.0)
				continue;

			Exit(grp, st, mates, cloud, r, enemies, now_ms);

			if (now_ms >= st.m_mWatchNext.Get(cloud.m_iId))
			{
				bool first = !st.m_mWatchNext.Contains(cloud.m_iId);
				st.m_mWatchNext.Set(cloud.m_iId, now_ms + 6000.0);
				int n = Watch(mates, cloud, r, center);
				DCO_Perf.Count("smoke:watch");
				if (first && DCO_Log.On(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO))
					DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(grp), string.Format("smoke_watch grp=%1 cloud=%2 dist=%3 radius=%4 between=%5 near_enemy=%6 watchers=%7",
						DCO_Log.IdGroup(grp), cloud.m_iId, Math.Round(dc), Math.Round(r), between, nearEnemy, n));
			}

			if ((between || nearEnemy) && now_ms >= st.m_fMGNext_ms)
				MGSuppress(grp, st, mates, cloud, r, dc, now_ms);
		}

		for (int i = st.m_mWatchNext.Count() - 1; i >= 0; i--)
		{
			if (now_ms - st.m_mWatchNext.GetElement(i) > 120000.0)
				st.m_mWatchNext.RemoveElement(i);
		}
	}

	protected static int Watch(array<SCR_AIUtilityComponent> mates, DCO_SmokeCloud cloud, float r, vector center)
	{
		vector to = DCO_Specialist.Flat(cloud.m_vPos - center);
		vector side = Vector(to[2], 0, -to[0]);
		int n;
		foreach (SCR_AIUtilityComponent m : mates)
		{
			if (n >= 2)
				break;
			if (!m.m_LookAction || DCO_SmokeField.Inside(m.m_OwnerEntity.GetOrigin(), 1.0))
				continue;
			if (m.m_CombatComponent && m.m_CombatComponent.GetCurrentTarget())
			{
				DCO_Perf.Count("smoke:watch_skip_engaged");
				continue;
			}
			vector look = cloud.m_vPos;
			int role = n % 3;
			if (role == 0)
				look = cloud.m_vPos + side * (r + 3.0);
			else if (role == 1)
				look = cloud.m_vPos - side * (r + 3.0);
			m.m_LookAction.LookAt(look + vector.Up * 1.2, 35.0, 6.0);
			n++;
		}
		return n;
	}

	protected static void MGSuppress(SCR_AIGroup grp, DCO_SmokeDrillState st, array<SCR_AIUtilityComponent> mates, DCO_SmokeCloud cloud, float r, float dc, float now_ms)
	{
		foreach (SCR_AIUtilityComponent m : mates)
		{
			if (!m.m_CombatComponent || m.m_CombatComponent.GetSelectedWeaponType() != EWeaponType.WT_MACHINEGUN)
				continue;
			if (m.m_CombatComponent.GetCurrentTarget())
			{
				DCO_Perf.Count("smoke:mg_skip_engaged");
				continue;
			}
			if (DCO_SmokeField.Inside(m.m_OwnerEntity.GetOrigin(), 1.2))
				continue;
			vector bbMin, bbMax;
			SCR_AISuppressionVolumeBase.CreateSuppressionBox(cloud.m_vPos + vector.Up, Math.Max(r, 3.0), 3, bbMin, bbMax);
			SCR_AISuppressionObjectVolumeBox vol = new SCR_AISuppressionObjectVolumeBox(bbMin, bbMax);
			m.AddAction(new SCR_AISuppressBehavior(m, null, vol, 8.0, 2.0));
			st.m_fMGNext_ms = now_ms + 15000.0;
			DCO_Perf.Count("smoke:mg_suppress");
			if (DCO_Log.On(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO))
				DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(grp), string.Format("smoke_mg_suppress grp=%1 unit=%2 cloud=%3 dist=%4 radius=%5",
					DCO_Log.IdGroup(grp), DCO_Log.IdUnit(m.m_OwnerEntity), cloud.m_iId, Math.Round(dc), Math.Round(r)));
			return;
		}
	}

	protected static void Exit(SCR_AIGroup grp, DCO_SmokeDrillState st, array<SCR_AIUtilityComponent> mates, DCO_SmokeCloud cloud, float r, array<vector> enemies, float now_ms)
	{
		if (enemies.IsEmpty())
			return;
		foreach (SCR_AIUtilityComponent m : mates)
		{
			IEntity e = m.m_OwnerEntity;
			vector p = e.GetOrigin();
			if (vector.DistanceXZ(p, cloud.m_vPos) > r * 0.8 || now_ms < st.m_mExitNext.Get(e))
				continue;
			if (!m.m_ThreatSystem || m.m_ThreatSystem.GetState() < EAIThreatState.ALERTED)
				continue;
			if (e.GetPhysics() && e.GetPhysics().GetVelocity().Length() > 0.5)
				continue;
			st.m_mExitNext.Set(e, now_ms + 10000.0);
			vector away = DCO_Specialist.Flat(p - cloud.m_vPos);
			if (away.LengthSq() < 0.01)
				away = Vector(1, 0, 0);
			vector goal = DCO_Util.Surface(cloud.m_vPos + away * (r + 4.0));
			DCO_RelocateBehavior.Start(m, goal, enemies[0], true, ECharacterStance.CROUCH, 8.0, "smoke_exit");
			DCO_Perf.Count("smoke:exit");
			if (DCO_Log.On(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO))
				DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(grp), string.Format("smoke_exit grp=%1 unit=%2 cloud=%3 radius=%4", DCO_Log.IdGroup(grp), DCO_Log.IdUnit(e), cloud.m_iId, Math.Round(r)));
		}
	}
}
