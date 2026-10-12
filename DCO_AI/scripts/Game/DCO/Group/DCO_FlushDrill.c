class DCO_FlushState
{
	ref map<IEntity, float> m_mHiddenSince = new map<IEntity, float>();
	ref map<IEntity, float> m_mGrenNext = new map<IEntity, float>();
	ref map<IEntity, float> m_mFlankNext = new map<IEntity, float>();
	float m_fSide = 1;
	float m_fFlankGroupNext_ms;
	IEntity m_Flanker;
	float m_fApproachUntil_ms;
	IEntity m_Approacher;
	SCR_AIUtilityComponent m_ApproachUtil;
	IEntity m_ApproachTarget;
	ref map<IEntity, int> m_mApproachN = new map<IEntity, int>();
}

modded class SCR_AIGroupUtilityComponent
{
	protected ref DCO_FlushState m_DCOFlush;
	protected float m_fDCOFlushTick_ms;

	DCO_FlushState DCO_GetFlush()
	{
		if (!m_DCOFlush)
			m_DCOFlush = new DCO_FlushState();
		return m_DCOFlush;
	}

	void DCO_TickFlush(float now_ms)
	{
		if (now_ms < m_fDCOFlushTick_ms || !Replication.IsServer())
			return;
		m_fDCOFlushTick_ms = now_ms + 2000.0;
		int t = DCO_Perf.Begin();
		DCO_FlushDrill.Tick(this, SCR_AIGroup.Cast(GetOwner()), now_ms);
		DCO_Perf.End("grp_flush", t);
	}
}

class DCO_FlushDrill
{
	static void Tick(SCR_AIGroupUtilityComponent u, SCR_AIGroup grp, float now_ms)
	{
		if (!grp || !u.m_Perception || !grp.DCO_FeatureOn(DCO_EGroupFeature.CONTACT_DRILLS) || DCO_Util.IsPlayerGroup(grp))
			return;
		PerceptionManager pm = GetGame().GetPerceptionManager();
		if (!pm)
			return;
		DCO_FlushState st = u.DCO_GetFlush();
		Prune(st.m_mHiddenSince);
		Prune(st.m_mGrenNext);
		Prune(st.m_mFlankNext);

		float pmNow = pm.GetTime();
		if (ApproachThrow(u, grp, st, pmNow, now_ms))
			return;
		foreach (SCR_AITargetInfo tgt : u.m_Perception.m_aTargets)
		{
			if (!tgt || !tgt.m_Entity || !DCO_Util.IsNotDead(tgt.m_Entity))
				continue;
			ChimeraCharacter tc = ChimeraCharacter.Cast(tgt.m_Entity);
			if (!tc || tc.IsInVehicle())
				continue;
			float age = pmNow - tgt.m_fTimestamp;
			if (age < 3.0)
			{
				st.m_mHiddenSince.Remove(tc);
				continue;
			}
			if (age > 60.0)
				continue;
			if (!st.m_mHiddenSince.Contains(tc))
			{
				st.m_mHiddenSince.Set(tc, now_ms);
				continue;
			}
			float hidden_ms = now_ms - st.m_mHiddenSince.Get(tc);
			if (hidden_ms < 20000.0)
				continue;
			if (Act(u, grp, st, tc, tgt.m_vWorldPos, hidden_ms, now_ms))
				return;
		}
	}

	protected static bool ApproachThrow(SCR_AIGroupUtilityComponent u, SCR_AIGroup grp, DCO_FlushState st, float pmNow, float now_ms)
	{
		if (!st.m_ApproachUtil || !st.m_ApproachTarget || now_ms > st.m_fApproachUntil_ms + 15000.0)
			return false;
		IEntity me = st.m_ApproachUtil.m_OwnerEntity;
		IEntity enemy = st.m_ApproachTarget;
		if (!me || !DCO_Util.IsAlive(me) || !DCO_Util.IsNotDead(enemy) || now_ms < st.m_mGrenNext.Get(enemy))
			return false;
		foreach (SCR_AITargetInfo tgt : u.m_Perception.m_aTargets)
		{
			if (!tgt || tgt.m_Entity != enemy)
				continue;
			float d = vector.Distance(me.GetOrigin(), tgt.m_vWorldPos);
			if (d > 30.0)
				return false;
			float age = pmNow - tgt.m_fTimestamp;
			if (age < 2.0 && !DCO_GrenadeUtility.HardCoverBetween(me, tgt.m_vWorldPos))
			{
				DCO_Perf.Count("flush:approach_target_open");
				return false;
			}
			DCO_Perf.Count("flush:approach_throw_try");
			bool thrown = Grenade(u, grp, st, st.m_ApproachUtil, d, enemy, tgt.m_vWorldPos, age * 1000.0, now_ms);
			if (thrown)
				st.m_ApproachUtil = null;
			return thrown;
		}
		return false;
	}

	protected static void Prune(map<IEntity, float> m)
	{
		for (int i = m.Count() - 1; i >= 0; i--)
		{
			if (!m.GetKey(i) || !DCO_Util.IsNotDead(m.GetKey(i)))
				m.RemoveElement(i);
		}
	}

	protected static bool Act(SCR_AIGroupUtilityComponent u, SCR_AIGroup grp, DCO_FlushState st, IEntity enemy, vector pos, float hidden_ms, float now_ms)
	{
		bool grenReady = now_ms >= st.m_mGrenNext.Get(enemy);
		bool flankReady = now_ms >= st.m_mFlankNext.Get(enemy) && now_ms >= st.m_fFlankGroupNext_ms && !SCR_DefendWaypoint.Cast(grp.GetCurrentWaypoint());
		if (!grenReady && !flankReady)
			return false;

		array<AIAgent> agents = {};
		grp.GetAgents(agents);
		SCR_AIUtilityComponent thrower;
		float throwerD = 70.0;
		float throwerScore = 70.0;
		array<SCR_AIUtilityComponent> mates = {};
		array<float> mateD = {};
		vector center;
		foreach (AIAgent a : agents)
		{
			SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
			if (!ca || !ca.m_UtilityComponent || !ca.m_UtilityComponent.m_CombatComponent)
				continue;
			ChimeraCharacter c = ChimeraCharacter.Cast(ca.GetControlledEntity());
			if (!c || c.IsInVehicle() || !DCO_Util.IsAlive(c))
				continue;
			float d = vector.Distance(c.GetOrigin(), pos);
			center = center + c.GetOrigin();
			mates.Insert(ca.m_UtilityComponent);
			mateD.Insert(d);
			bool flanking = c == st.m_Flanker && now_ms < st.m_fFlankGroupNext_ms;
			float score = d;
			if (c == st.m_Approacher && now_ms < st.m_fApproachUntil_ms + 15000.0)
				score = d - 1000.0;
			if (d < 70.0 && score < throwerScore && !flanking && ca.m_UtilityComponent.m_CombatComponent.HasWeaponOfType(EWeaponType.WT_FRAGGRENADE))
			{
				throwerScore = score;
				throwerD = d;
				thrower = ca.m_UtilityComponent;
			}
		}
		if (mates.IsEmpty())
			return false;
		center = center * (1.0 / mates.Count());
		if (vector.DistanceXZ(center, pos) > 120.0)
		{
			DCO_Perf.Count("flush:skip_far");
			return false;
		}

		bool did;
		if (grenReady && thrower)
			did = Grenade(u, grp, st, thrower, throwerD, enemy, pos, hidden_ms, now_ms);
		if (flankReady && mates.Count() >= 2)
			did = Flank(grp, st, mates, mateD, thrower, center, enemy, pos, hidden_ms, now_ms) || did;
		return did;
	}

	protected static bool Grenade(SCR_AIGroupUtilityComponent u, SCR_AIGroup grp, DCO_FlushState st, SCR_AIUtilityComponent tu, float d, IEntity enemy, vector pos, float hidden_ms, float now_ms)
	{
		IEntity me = tu.m_OwnerEntity;
		bool inBuilding = SCR_CoverManagerComponent.DCO_GetBuildingAt(enemy) != null;
		if (d <= 30.0)
		{
			st.m_mGrenNext.Set(enemy, now_ms + 6000.0);
			vector tp;
			if (!DCO_GrenadeUtility.CanThrowGrenadeNow(tu))
			{
				DCO_Perf.Count("flush:gren_not_now");
				return false;
			}
			vector toMe = DCO_Specialist.Flat(me.GetOrigin() - pos);
			vector side = Vector(toMe[2], 0, -toMe[0]);
			array<vector> aims = {pos, pos + toMe * 2.0, pos + side * 2.0, pos - side * 2.0};
			int aimIdx = -1;
			for (int ai = 0; ai < aims.Count(); ai++)
			{
				if (DCO_GrenadeUtility.ResolveThrowPos(tu, DCO_Util.Surface(aims[ai]), tp))
				{
					aimIdx = ai;
					break;
				}
			}
			if (aimIdx > 0)
				DCO_Perf.Count("flush:gren_alt_aim");
			if (aimIdx < 0)
			{
				DCO_Perf.Count("flush:gren_rejected");
				if (DCO_Log.On(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO))
					DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(grp), string.Format("flush_grenade_reject grp=%1 unit=%2 enemy=%3 dist=%4 reason=%5",
						DCO_Log.IdGroup(grp), DCO_Log.IdUnit(me), DCO_Log.IdUnit(enemy), Math.Round(d), DCO_GrenadeUtility.s_sDCORej));
				st.m_mGrenNext.Set(enemy, now_ms + 20000.0);
				return false;
			}
			SCR_AIThrowGrenadeToBehavior gren = new SCR_AIThrowGrenadeToBehavior(tu, null, tp, EWeaponType.WT_FRAGGRENADE, 1,
				SCR_AIThrowGrenadeToBehavior.PRIORITY_BEHAVIOR_THROW_GRENADE + SCR_AIThrowGrenadeToBehavior.PRIORITY_LEVEL_PLAYER);
			tu.AddAction(gren);
			DCO_GrenadeUtility.NotifyGrenadeThrown(tu);
			st.m_mGrenNext.Set(enemy, now_ms + 25000.0);
			DCO_Perf.Count("flush:grenade");
			if (DCO_Log.On(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO))
				DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(grp), string.Format("flush_grenade grp=%1 unit=%2 enemy=%3 dist=%4 hidden_s=%5 in_building=%6 player=%7",
					DCO_Log.IdGroup(grp), DCO_Log.IdUnit(me), DCO_Log.IdUnit(enemy), Math.Round(d), Math.Round(hidden_ms / 1000.0), inBuilding, SCR_CharacterHelper.IsPlayer(enemy)));
			return true;
		}

		if (now_ms < st.m_fApproachUntil_ms || st.m_mApproachN.Get(enemy) >= 2)
		{
			DCO_Perf.Count("flush:approach_wait");
			return false;
		}
		vector ap;
		if (!Reachable(pos + DCO_Specialist.Flat(me.GetOrigin() - pos) * 20.0, ap))
		{
			DCO_Perf.Count("flush:approach_no_nav");
			return false;
		}
		if (!DCO_Engage.ApproachAllowed(tu, ap, "flush_approach"))
		{
			st.m_fApproachUntil_ms = now_ms + 10000.0;
			return false;
		}
		DCO_RelocateBehavior.Start(tu, ap, pos, false, ECharacterStance.CROUCH, 20.0, "flush_approach");
		st.m_mApproachN.Set(enemy, st.m_mApproachN.Get(enemy) + 1);
		st.m_fApproachUntil_ms = now_ms + 15000.0;
		st.m_Approacher = me;
		st.m_ApproachUtil = tu;
		st.m_ApproachTarget = enemy;
		st.m_mGrenNext.Set(enemy, now_ms + 4000.0);
		DCO_Perf.Count("flush:approach");
		if (DCO_Log.On(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO))
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(grp), string.Format("flush_approach grp=%1 unit=%2 enemy=%3 dist=%4 hidden_s=%5 in_building=%6",
				DCO_Log.IdGroup(grp), DCO_Log.IdUnit(me), DCO_Log.IdUnit(enemy), Math.Round(d), Math.Round(hidden_ms / 1000.0), inBuilding));
		return true;
	}

	protected static bool Flank(SCR_AIGroup grp, DCO_FlushState st, array<SCR_AIUtilityComponent> mates, array<float> mateD, SCR_AIUtilityComponent thrower, vector center, IEntity enemy, vector pos, float hidden_ms, float now_ms)
	{
		SCR_AIUtilityComponent flanker;
		SCR_AIUtilityComponent supp;
		float fD = float.MAX;
		float sD = float.MAX;
		for (int i = 0; i < mates.Count(); i++)
		{
			if (mates[i] == thrower || !FlankCapable(mates[i]))
				continue;
			if (mateD[i] < fD)
			{
				fD = mateD[i];
				flanker = mates[i];
			}
		}
		if (!flanker)
			return false;
		for (int j = 0; j < mates.Count(); j++)
		{
			if (mates[j] != flanker && mateD[j] < sD)
			{
				sD = mateD[j];
				supp = mates[j];
			}
		}
		if (!supp)
			return false;

		vector back = DCO_Specialist.Flat(center - pos);
		float dist = Math.Clamp(vector.DistanceXZ(center, pos), 20.0, 60.0);
		st.m_fSide = -st.m_fSide;
		float ang = Math.RandomFloat(35.0, 55.0) * Math.DEG2RAD * st.m_fSide;
		float cs = Math.Cos(ang);
		float sn = Math.Sin(ang);
		vector dir = Vector(back[0] * cs - back[2] * sn, 0, back[0] * sn + back[2] * cs);
		vector fp;
		st.m_mFlankNext.Set(enemy, now_ms + 10000.0);
		if (!Reachable(pos + dir * dist, fp))
		{
			DCO_Perf.Count("flush:flank_no_nav");
			return false;
		}
		DCO_RelocateBehavior.Start(flanker, fp, pos, false, ECharacterStance.CROUCH, 30.0, "flush_flank");

		vector bbMin, bbMax;
		SCR_AISuppressionVolumeBase.CreateSuppressionBox(pos, 3.0, 3, bbMin, bbMax);
		SCR_AISuppressionObjectVolumeBox vol = new SCR_AISuppressionObjectVolumeBox(bbMin, bbMax);
		supp.AddAction(new SCR_AISuppressBehavior(supp, null, vol, 8.0, 1.5));

		st.m_mFlankNext.Set(enemy, now_ms + 45000.0);
		st.m_fFlankGroupNext_ms = now_ms + 45000.0;
		st.m_Flanker = flanker.m_OwnerEntity;
		DCO_Perf.Count("flush:flank");
		if (DCO_Log.On(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO))
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(grp), string.Format("flush_flank grp=%1 flanker=%2 suppressor=%3 enemy=%4 angle=%5 flank_dist=%6 hidden_s=%7",
				DCO_Log.IdGroup(grp), DCO_Log.IdUnit(flanker.m_OwnerEntity), DCO_Log.IdUnit(supp.m_OwnerEntity), DCO_Log.IdUnit(enemy), Math.Round(ang * Math.RAD2DEG), Math.Round(dist), Math.Round(hidden_ms / 1000.0)));
		return true;
	}

	protected static bool FlankCapable(SCR_AIUtilityComponent m)
	{
		if (!m.m_CombatComponent)
			return false;
		DCO_SpecState spec = m.m_CombatComponent.DCO_GetSpec();
		if (spec && spec.m_iSortie != 0)
			return false;
		int roles = DCO_Roles.Get(m);
		return (roles & 5) == 0;
	}

	static bool Reachable(vector p, out vector outPos)
	{
		p = DCO_Util.Surface(p);
		NavmeshWorldComponent nav = GetGame().GetAIWorld().GetNavmeshWorldComponent("Soldiers");
		if (!nav)
			return false;
		if (!nav.IsTileLoaded(p))
		{
			if (!nav.IsTileRequested(p))
				nav.LoadTileIn(p);
			return false;
		}
		return nav.GetReachablePoint(p, 8.0, outPos);
	}
}
