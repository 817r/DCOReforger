class DCO_BodyRecord
{
	IEntity m_Corpse;
	IEntity m_Killer;
	vector m_vPos;
	FactionKey m_sFaction;
	float m_fAt;
	bool m_bFound;
}

class DCO_Awareness
{
	protected static ref array<ref DCO_BodyRecord> s_aBodies;
	protected static ref array<vector> s_aIDFPos;
	protected static ref array<float> s_aIDFAt;

	static void OnDeath(IEntity victim)
	{
		if (!victim || !Replication.IsServer() || !DCO_GlobalAIComponent.BehaviorOnFor(victim, DCO_EBehavior.AWARENESS))
			return;
		if (!s_aBodies)
			s_aBodies = {};
		if (s_aBodies.Count() >= 64)
			s_aBodies.Remove(0);
		DCO_BodyRecord r = new DCO_BodyRecord();
		r.m_Corpse = victim;
		r.m_vPos = victim.GetOrigin();
		r.m_sFaction = DCO_Util.FactionKeyOf(victim, true);
		r.m_fAt = DCO_Util.NowS();
		DamageManagerComponent dm = DamageManagerComponent.Cast(victim.FindComponent(DamageManagerComponent));
		if (dm && dm.GetInstigator())
			r.m_Killer = dm.GetInstigator().GetInstigatorEntity();
		if (r.m_Killer == victim)
			r.m_Killer = null;
		s_aBodies.Insert(r);
	}

	static void MarkIDF(vector pos)
	{
		if (!DCO_GlobalAIComponent.BehaviorOn(DCO_EBehavior.AWARENESS))
			return;
		if (!s_aIDFPos)
		{
			s_aIDFPos = {};
			s_aIDFAt = {};
		}
		float now = DCO_Util.NowS();
		foreach (int i, vector p : s_aIDFPos)
		{
			if (vector.DistanceXZ(p, pos) < 150.0)
			{
				s_aIDFAt[i] = now;
				return;
			}
		}
		if (s_aIDFPos.Count() >= 16)
		{
			s_aIDFPos.Remove(0);
			s_aIDFAt.Remove(0);
		}
		s_aIDFPos.Insert(pos);
		s_aIDFAt.Insert(now);
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("idf_sector pos=%1,%2", Math.Round(pos[0]), Math.Round(pos[2])));
	}

	static bool IDFActive(vector pos)
	{
		if (!s_aIDFPos)
			return false;
		float now = DCO_Util.NowS();
		for (int i = s_aIDFPos.Count() - 1; i >= 0; i--)
		{
			if (now - s_aIDFAt[i] > 300.0)
			{
				s_aIDFPos.Remove(i);
				s_aIDFAt.Remove(i);
				continue;
			}
			if (vector.DistanceXZ(s_aIDFPos[i], pos) < 300.0)
				return true;
		}
		return false;
	}

	static float PursuitRange(AIGroup grp)
	{
		if (!grp || !grp.GetLeaderEntity())
			return 200.0;
		SCR_AICombatComponent combat = SCR_AICombatComponent.Cast(grp.GetLeaderEntity().FindComponent(SCR_AICombatComponent));
		if (!combat || !combat.GetUtilityComponent() || !combat.GetUtilityComponent().m_DCOConfig)
			return 200.0;
		switch (combat.GetUtilityComponent().m_DCOConfig.GetPersonality())
		{
			case DCO_EAIPersonality.CAUTIOUS:	return 150.0;
			case DCO_EAIPersonality.AGGRESSIVE:	return 250.0;
			case DCO_EAIPersonality.RECKLESS:	return 300.0;
		}
		return 200.0;
	}

	static vector HidingSpot(vector goal, float radius)
	{
		DCO_GarrisonRegistry reg = DCO_GarrisonRegistry.GetInstance();
		if (reg)
		{
			array<IEntity> buildings = {};
			reg.FindBuildings(goal, radius, buildings);
			IEntity best;
			float bestD = float.MAX;
			foreach (IEntity b : buildings)
			{
				float d = vector.DistanceSqXZ(b.GetOrigin(), goal);
				if (d < bestD)
				{
					bestD = d;
					best = b;
				}
			}
			if (best)
				return best.GetOrigin();
		}
		return DCO_TerrainCache.QueryBest(DCO_ETerrainFlag.FOREST_EDGE, goal, radius, string.Empty, goal);
	}

	static void GroupTick(SCR_AIGroupUtilityComponent gu, SCR_AIGroup grp, DCO_GroupUtilityComponent g, float now_ms)
	{
		if (!grp || !gu.m_Perception || !DCO_GlobalAIComponent.BehaviorOnFor(grp, DCO_EBehavior.AWARENESS) || DCO_Util.IsPlayerGroup(grp))
			return;
		IEntity leader = grp.GetLeaderEntity();
		if (!leader || (g && (g.IsInTransport() || g.IsMortar() || g.IsArmor() || g.IsDedicatedTransport())))
			return;
		ChimeraCharacter lc = ChimeraCharacter.Cast(leader);
		if (!lc || lc.IsInVehicle())
			return;
		vector lp = leader.GetOrigin();
		CheckBodies(gu, grp, g, lc, lp, now_ms);
		if (g && IDFActive(lp))
			Disperse(gu, grp, g, lc, lp, now_ms);
		else
			gu.m_fDCOIdfStill_ms = -1;
	}

	protected static void CheckBodies(SCR_AIGroupUtilityComponent gu, SCR_AIGroup grp, DCO_GroupUtilityComponent g, ChimeraCharacter leader, vector lp, float now_ms)
	{
		if (!s_aBodies || s_aBodies.IsEmpty())
			return;
		if (g)
		{
			DCO_EGroupTask t = g.GetTask();
			if (t == DCO_EGroupTask.TRANSPORT || g.HasState(DCO_EGroupState.IN_CONTACT))
				return;
		}
		FactionKey fk = DCO_Util.FactionKeyOf(leader, false);
		Faction myF = grp.GetFaction();
		float now = DCO_Util.NowS();
		for (int i = s_aBodies.Count() - 1; i >= 0; i--)
		{
			DCO_BodyRecord r = s_aBodies[i];
			if (!r.m_Corpse || now - r.m_fAt > 900.0)
			{
				s_aBodies.Remove(i);
				continue;
			}
			if (r.m_bFound || r.m_sFaction != fk || vector.DistanceSqXZ(lp, r.m_vPos) > 3600.0)
				continue;
			if (!DCO_Util.TraceClear(leader.EyePosition(), r.m_vPos + Vector(0, 0.3, 0), leader, 0.97, TraceFlags.WORLD | TraceFlags.ENTS, -1, r.m_Corpse, "t:DCO_Body"))
				continue;
			r.m_bFound = true;

			IEntity killer = r.m_Killer;
			Faction kf;
			if (killer)
			{
				FactionAffiliationComponent fac = FactionAffiliationComponent.Cast(killer.GetRootParent().FindComponent(FactionAffiliationComponent));
				if (!fac)
					fac = FactionAffiliationComponent.Cast(killer.FindComponent(FactionAffiliationComponent));
				if (fac)
					kf = fac.GetAffiliatedFaction();
			}
			if (!killer || !kf || (myF && myF.IsFactionFriendly(kf)))
			{
				DCO_Perf.Count("body_found_nokiller");
				DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("body_found grp=%1 killer=none", grp.GetName()));
				return;
			}

			if (g && (g.GetTask() == DCO_EGroupTask.DEFEND || g.GetTask() == DCO_EGroupTask.GARRISON))
			{
				AICommander_BaseComponent hcmd = g.GetMyCommander();
				if (hcmd && hcmd.GetDefense())
					hcmd.GetDefense().AddDanger(r.m_vPos, now);
				DCO_Perf.Count("body_found:hold");
				DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("body_found grp=%1 dist=%2 action=hold_report task=%3", grp.GetName(), Math.Round(vector.DistanceXZ(lp, r.m_vPos)), typename.EnumToString(DCO_EGroupTask, g.GetTask())));
				return;
			}
			vector dir = DCO_Specialist.Flat(killer.GetOrigin() - r.m_vPos);
			float sweep = Math.Clamp(vector.DistanceXZ(killer.GetOrigin(), r.m_vPos), 50.0, 100.0);
			vector est = DCO_Util.Surface(r.m_vPos + dir * sweep);

			PerceptionManager pm = GetGame().GetPerceptionManager();
			if (!pm)
				return;
			int idx = gu.m_Perception.m_aTargetEntities.Find(killer);
			if (idx < 0 || pm.GetTime() - gu.m_Perception.m_aTargets[idx].m_fTimestamp > 30.0)
				gu.m_Perception.AddOrUpdateGunshot(killer, est, kf, pm.GetTime(), false);
			gu.m_fDCOBodySweep_ms = now_ms + 120000.0;
			DCO_Perf.Count("body_found");
			DCO_Perf.Count("body_found:sweep");
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("body_found grp=%1 dist=%2 sweep=%3", grp.GetName(), Math.Round(vector.DistanceXZ(lp, r.m_vPos)), Math.Round(sweep)));
			return;
		}
	}

	protected static void Disperse(SCR_AIGroupUtilityComponent gu, SCR_AIGroup grp, DCO_GroupUtilityComponent g, ChimeraCharacter leader, vector lp, float now_ms)
	{
		DCO_EGroupTask t = g.GetTask();
		if (g.HasState(DCO_EGroupState.IN_CONTACT) || t == DCO_EGroupTask.DEFEND || t == DCO_EGroupTask.GARRISON || gu.DCO_GetGarrisonMode() >= 0)
			return;
		if (leader.GetPhysics() && leader.GetPhysics().GetVelocity().Length() > 0.5)
		{
			gu.m_fDCOIdfStill_ms = -1;
			return;
		}
		if (gu.m_fDCOIdfStill_ms < 0)
		{
			gu.m_fDCOIdfStill_ms = now_ms;
			return;
		}
		if (now_ms - gu.m_fDCOIdfStill_ms < 8000.0)
			return;

		array<AIAgent> agents = {};
		grp.GetAgents(agents);
		float now = DCO_Util.NowS();
		if (now - g.m_fDCODisplacedAt > 60.0 && !SCR_CoverManagerComponent.IsEntityInsideBuilding(leader) && DCO_ConcealmentUtility.IsPositionExposed(lp, leader))
		{
			vector cover = HidingSpot(lp, 80.0);
			if (cover != vector.Zero && vector.DistanceXZ(cover, lp) > 15.0)
			{
				foreach (int k, AIAgent a : agents)
				{
					SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
					if (ca && ca.m_UtilityComponent && DCO_Util.IsAlive(ca.GetControlledEntity()))
					{
						float ang = 1.0 * k * 2.4;
						DCO_RelocateBehavior.Start(ca.m_UtilityComponent, DCO_Util.Surface(cover + Vector(Math.Cos(ang) * 8.0, 0, Math.Sin(ang) * 8.0)), cover + (cover - lp), true, ECharacterStance.CROUCH, vector.DistanceXZ(ca.GetControlledEntity().GetOrigin(), cover) / 3.5 + 3.0, "idf_hide");
					}
				}
				g.m_fDCODisplacedAt = now;
				DCO_Perf.Count("idf_no_halt");
				DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("idf_no_halt grp=%1 dist=%2", grp.GetName(), Math.Round(vector.DistanceXZ(cover, lp))));
				return;
			}
		}

		int moved;
		foreach (int i, AIAgent ai : agents)
		{
			IEntity ei = ai.GetControlledEntity();
			SCR_ChimeraAIAgent cai = SCR_ChimeraAIAgent.Cast(ai);
			if (!ei || !cai || !cai.m_UtilityComponent || !DCO_Util.IsAlive(ei) || SCR_CoverManagerComponent.IsEntityInsideBuilding(ei))
				continue;
			for (int j = 0; j < i; j++)
			{
				IEntity ej = agents[j].GetControlledEntity();
				if (!ej || vector.DistanceSqXZ(ei.GetOrigin(), ej.GetOrigin()) > 25.0)
					continue;
				vector away = DCO_Specialist.Flat(ei.GetOrigin() - ej.GetOrigin());
				DCO_RelocateBehavior.Start(cai.m_UtilityComponent, DCO_Util.Surface(ei.GetOrigin() + away * 6.0), ei.GetOrigin() + away * 30.0, false, ECharacterStance.CROUCH, 5.0, "idf_disperse");
				moved++;
				break;
			}
			if (moved >= 3)
				break;
		}
		if (moved > 0)
			DCO_Perf.Count("idf_spread");
	}
}
