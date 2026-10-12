class DCO_OppMotion
{
	vector m_vPos;
	float m_fAt_ms = -100000.0;
	float m_fSpeed;
	float m_fExposedAt_ms = -100000.0;
	bool m_bExposed;
	float m_fTracked_ms = -1;
}

class DCO_OppState
{
	float m_fNextTick_ms;
	float m_fNextCluster_ms;
	float m_fSideNext_ms;
	float m_fPushNext_ms;
	float m_fReloadNext_ms;
	IEntity m_PatientTgt;
	float m_fPatientFrom_ms = -1;
	float m_fPatientFreeUntil_ms = -1;
	float m_fPatientCheck_ms;
	bool m_bPatientPartial;
	int m_iFlags;
	float m_fFlagsAt_ms = -100000.0;
}

class DCO_Opp
{
	protected static ref map<IEntity, ref DCO_OppMotion> s_mMotion;
	protected static ref map<SCR_AIUtilityComponent, ref DCO_OppState> s_mState;
	protected static ref array<BaseTarget> s_aT;

	static bool On(SCR_AIUtilityComponent u, string item)
	{
		return u && DCO_Stress.T(u, "opp." + item, DefOf(item)) > 0;
	}

	static float DefOf(string item)
	{
		if (item == "exposed" || item == "suppressed_push" || item == "distracted" || item == "patient" || item == "reload" || item == "loot" || item == "cluster_scan")
			return 1.0;
		return 0.0;
	}

	static int Flags(SCR_AIUtilityComponent u)
	{
		if (!u)
			return 0;
		DCO_OppState st = StateOf(u);
		float now = GetGame().GetWorld().GetWorldTime();
		if (now - st.m_fFlagsAt_ms < 2000.0)
			return st.m_iFlags;
		st.m_fFlagsAt_ms = now;
		int f = 0;
		if (On(u, "exposed"))
			f |= 1;
		if (On(u, "distracted"))
			f |= 2;
		if (On(u, "suppressed_push"))
			f |= 4;
		if (On(u, "cluster_scan"))
			f |= 8;
		if (On(u, "patient"))
			f |= 16;
		if (On(u, "loot"))
			f |= 32;
		if (On(u, "reload"))
			f |= 64;
		st.m_iFlags = f;
		return f;
	}

	protected static DCO_OppState StateOf(SCR_AIUtilityComponent u)
	{
		if (!s_mState)
			s_mState = new map<SCR_AIUtilityComponent, ref DCO_OppState>();
		DCO_OppState st = s_mState.Get(u);
		if (st)
			return st;
		if (s_mState.Count() > 2000)
		{
			map<SCR_AIUtilityComponent, ref DCO_OppState> fresh = new map<SCR_AIUtilityComponent, ref DCO_OppState>();
			foreach (SCR_AIUtilityComponent k, DCO_OppState v : s_mState)
			{
				if (k && k.m_OwnerEntity)
					fresh.Set(k, v);
			}
			s_mState = fresh;
		}
		st = new DCO_OppState();
		s_mState.Set(u, st);
		return st;
	}

	protected static DCO_OppMotion MotionOf(IEntity e, float now)
	{
		if (!s_mMotion)
			s_mMotion = new map<IEntity, ref DCO_OppMotion>();
		DCO_OppMotion m = s_mMotion.Get(e);
		if (!m)
		{
			if (s_mMotion.Count() > 2000)
			{
				map<IEntity, ref DCO_OppMotion> keep = new map<IEntity, ref DCO_OppMotion>();
				foreach (IEntity mk, DCO_OppMotion mv : s_mMotion)
				{
					if (mk)
						keep.Set(mk, mv);
				}
				s_mMotion = keep;
			}
			m = new DCO_OppMotion();
			m.m_vPos = e.GetOrigin();
			m.m_fAt_ms = now;
			m.m_fTracked_ms = now;
			s_mMotion.Set(e, m);
			DCO_ShotCounter.Track(e);
			return m;
		}
		float dt = now - m.m_fAt_ms;
		if (dt >= 500.0)
		{
			vector p = e.GetOrigin();
			m.m_fSpeed = 1000.0 * vector.DistanceXZ(p, m.m_vPos) / dt;
			m.m_vPos = p;
			m.m_fAt_ms = now;
		}
		return m;
	}

	static float Speed(IEntity e)
	{
		if (!e)
			return 0;
		return MotionOf(e, GetGame().GetWorld().GetWorldTime()).m_fSpeed;
	}

	static bool InOpen(IEntity e)
	{
		if (!e)
			return false;
		float now = GetGame().GetWorld().GetWorldTime();
		DCO_OppMotion m = MotionOf(e, now);
		if (now - m.m_fExposedAt_ms >= 2000.0)
		{
			m.m_fExposedAt_ms = now;
			m.m_bExposed = DCO_ConcealmentUtility.IsPositionExposed(e.GetOrigin(), e);
		}
		return m.m_bExposed;
	}

	static float QuietS(IEntity e)
	{
		float now = GetGame().GetWorld().GetWorldTime();
		DCO_OppMotion m = MotionOf(e, now);
		float last = Math.Max(DCO_ShotCounter.LastShotAt(e), m.m_fTracked_ms);
		return 0.001 * (now - last);
	}

	static float FacingAwayDeg(IEntity enemy, IEntity me)
	{
		vector mat[4];
		enemy.GetWorldTransform(mat);
		vector fwd = mat[2];
		fwd[1] = 0;
		vector toMe = me.GetOrigin() - enemy.GetOrigin();
		toMe[1] = 0;
		if (fwd.Length() < 0.01 || toMe.Length() < 0.01)
			return 0;
		float dot = vector.Dot(fwd.Normalized(), toMe.Normalized());
		return Math.Acos(Math.Clamp(dot, -1.0, 1.0)) * Math.RAD2DEG;
	}

	static bool Standing(ChimeraCharacter tc)
	{
		CharacterControllerComponent cc = tc.GetCharacterController();
		return cc && cc.GetStance() == ECharacterStance.STAND;
	}

	static bool Distracted(BaseTarget t, ChimeraCharacter tc, IEntity me)
	{
		if (t.IsEndangering() || t.GetTimeSinceEndangered() < 5.0)
			return false;
		return FacingAwayDeg(tc, me) > 60.0;
	}

	static float TargetBonus(SCR_AIUtilityComponent u, BaseTarget t, ChimeraCharacter tc, float d, out string why)
	{
		why = "";
		if (!u || !tc || tc.IsInVehicle())
			return 0;
		int fl = Flags(u);
		if (fl == 0)
			return 0;
		float b = 0;
		if (fl & 1)
		{
			float eb = 0;
			if (InOpen(tc))
			{
				eb += 3.0;
				why += "open";
			}
			if (Speed(tc) > 1.5)
			{
				eb += 1.0;
				why += "+moving";
			}
			if (DCO_Util.IsMGShooter(tc))
			{
				eb += 4.0;
				why += "+mg";
			}
			if (Standing(tc))
			{
				eb += 1.0;
				why += "+stand";
			}
			b += Math.Min(eb, 6.0);
		}
		if ((fl & 2) && d >= 40.0 && d <= 150.0 && Distracted(t, tc, u.m_OwnerEntity))
		{
			b += 2.0;
			why += "+distracted";
		}
		return b;
	}

	static void NotePick(SCR_AIUtilityComponent u, BaseTarget t, float d, float bonus, string why)
	{
		if (bonus <= 0)
			return;
		DCO_Perf.Count("opp:exposed_pick");
		if (why.Contains("distracted"))
			DCO_Perf.Count("opp:distracted_shot");
		if (DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG))
			DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(u.m_OwnerEntity), string.Format("opp_exposed unit=%1 tgt=%2 dist=%3 bonus=%4 why=%5",
				DCO_Log.IdUnit(u.m_OwnerEntity), DCO_Log.IdUnit(t.GetTargetEntity()), Math.Round(d), bonus, why));
	}

	static float AggrF(SCR_AIUtilityComponent u)
	{
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetForUtility(u);
		if (!g)
			return 1.0;
		return Math.Clamp(g.DCO_AggressionSlider(), 0.5, 3.0);
	}

	static float SettleMul(SCR_AIUtilityComponent u, BaseTarget t)
	{
		if (!t || !(Flags(u) & 1))
			return 1.0;
		ChimeraCharacter tc = ChimeraCharacter.Cast(t.GetTargetEntity());
		if (!tc || tc.IsInVehicle() || Speed(tc) > 0.5 || !InOpen(tc))
			return 1.0;
		DCO_Perf.Count("opp:exposed_fast_settle");
		return 0.8;
	}

	static string Gate(SCR_AIUtilityComponent u, bool move)
	{
		if (!u || !u.m_OwnerEntity || !u.m_CombatComponent)
			return "no_unit";
		if (u.m_AIInfo && u.m_AIInfo.HasUnitState(EUnitState.IN_VEHICLE))
			return "vehicle";
		if (u.m_CombatComponent.GetCombatMode() == EAIGroupCombatMode.HOLD_FIRE)
			return "roe";
		if (u.m_CombatComponent.DCO_IsPinned() || u.m_CombatComponent.DCO_IsPinRecovering())
			return "pinned";
		DCO_AIMoraleSystem ms = u.GetMoraleSystem();
		if (ms && (ms.GetState() == moraleState.PANIC || ms.GetState() == moraleState.BREAK))
			return "panic";
		AIAgent ag = u.GetAIAgent();
		SCR_AIGroup grp;
		if (ag)
			grp = SCR_AIGroup.Cast(ag.GetParentGroup());
		if (!grp)
			return "no_group";
		if (DCO_Util.IsPlayerGroup(grp))
			return "player_group";
		if (!move)
			return "";
		if (SCR_DefendWaypoint.Cast(grp.GetCurrentWaypoint()))
			return "defend";
		if (SCR_CoverManagerComponent.DCO_GetBuildingAt(u.m_OwnerEntity))
			return "indoor";
		SCR_AIGroupUtilityComponent gu = grp.GetGroupUtilityComponent();
		if (gu && gu.DCO_IsClearingBuilding())
			return "cqb";
		DCO_GroupUtilityComponent dg = DCO_GroupUtilityComponent.Cast(grp.FindComponent(DCO_GroupUtilityComponent));
		if (dg && (dg.IsRetreating() || dg.IsBreakingContact()))
			return "retreat";
		if (dg && (dg.GetTask() == DCO_EGroupTask.DEFEND || dg.GetTask() == DCO_EGroupTask.GARRISON || dg.GetTask() == DCO_EGroupTask.SUPPORT_BY_FIRE))
			return "task";
		return "";
	}

	protected static int OppMovers(SCR_AIGroup grp, out int alive)
	{
		alive = 0;
		int n = 0;
		array<AIAgent> agents = {};
		grp.GetAgents(agents);
		foreach (AIAgent a : agents)
		{
			SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
			if (!ca || !ca.m_UtilityComponent || !DCO_Util.IsAlive(ca.GetControlledEntity()))
				continue;
			alive++;
			DCO_RelocateBehavior rb = DCO_RelocateBehavior.Cast(ca.m_UtilityComponent.FindActionOfType(DCO_RelocateBehavior));
			if (rb && !rb.DCO_IsDone() && rb.DCO_Why().StartsWith("opp_"))
				n++;
		}
		return n;
	}

	protected static ref map<AIGroup, float> s_mSquadVoiceAt;
	protected static ref map<AIGroup, IEntity> s_mSquadVoiceKey;

	static bool SquadVoiceOk(AIGroup grp, IEntity key, float window_ms)
	{
		if (!grp)
			return true;
		if (!s_mSquadVoiceAt)
		{
			s_mSquadVoiceAt = new map<AIGroup, float>();
			s_mSquadVoiceKey = new map<AIGroup, IEntity>();
		}
		if (s_mSquadVoiceAt.Count() > 500)
		{
			s_mSquadVoiceAt.Clear();
			s_mSquadVoiceKey.Clear();
		}
		float now = GetGame().GetWorld().GetWorldTime();
		float at;
		IEntity lastKey;
		if (s_mSquadVoiceAt.Find(grp, at) && now - at < window_ms && s_mSquadVoiceKey.Find(grp, lastKey) && lastKey == key)
			return false;
		s_mSquadVoiceAt.Set(grp, now);
		s_mSquadVoiceKey.Set(grp, key);
		return true;
	}

	protected static ref map<AIGroup, float> s_mBoundVoiceAt;
	protected static ref map<SCR_AIUtilityComponent, float> s_mUnderFireAt;

	static void UnderFireCallout(SCR_AIUtilityComponent u, float now)
	{
		if (!u || !u.m_CommsHandler || u.m_CommsHandler.CanBypass() || DCO_Stress.T(u, "opp.bound_callout", 1.0) <= 0)
			return;
		if (!s_mUnderFireAt)
			s_mUnderFireAt = new map<SCR_AIUtilityComponent, float>();
		if (s_mUnderFireAt.Count() > 1000)
			s_mUnderFireAt.Clear();
		float at;
		if (s_mUnderFireAt.Find(u, at) && now - at < 30000.0)
			return;
		s_mUnderFireAt.Set(u, now);
		u.m_CommsHandler.AddRequest(new SCR_AITalkRequest(ECommunicationType.REPORT_UNDER_FIRE, null, vector.Zero, 0, false, false, SCR_EAITalkRequestPreset.MEDIUM));
		DCO_Perf.Count("opp:callout_under_fire");
	}

	static void BoundCallout(SCR_AIUtilityComponent u)
	{
		if (!u || !u.m_CommsHandler || u.m_CommsHandler.CanBypass() || DCO_Stress.T(u, "opp.bound_callout", 1.0) <= 0)
			return;
		AIAgent ag = u.GetAIAgent();
		if (!ag)
			return;
		SCR_AIGroup grp = SCR_AIGroup.Cast(ag.GetParentGroup());
		if (!grp || DCO_Util.IsPlayerGroup(grp))
			return;
		if (!s_mBoundVoiceAt)
			s_mBoundVoiceAt = new map<AIGroup, float>();
		if (s_mBoundVoiceAt.Count() > 500)
			s_mBoundVoiceAt.Clear();
		float now = GetGame().GetWorld().GetWorldTime();
		float at;
		if (s_mBoundVoiceAt.Find(grp, at) && now - at < 20000.0)
			return;
		s_mBoundVoiceAt.Set(grp, now);
		DCO_Perf.Count("opp:callout_bound");
		Callout(u, grp, null);
	}

	protected static void Callout(SCR_AIUtilityComponent u, SCR_AIGroup grp, IEntity enemy)
	{
		if (!u || !u.m_CommsHandler || DCO_Stress.T(u, "opp.callout", 1.0) <= 0)
			return;
		u.m_CommsHandler.AddRequest(new SCR_AITalkRequest(ECommunicationType.REPORT_MOVING, null, vector.Zero, 0, false, false, SCR_EAITalkRequestPreset.MEDIUM));
		DCO_Perf.Count("opp:callout_moving");
		if (!grp)
			return;
		IEntity me = u.m_OwnerEntity;
		float now = GetGame().GetWorld().GetWorldTime();
		array<AIAgent> mates = {};
		grp.GetAgents(mates);
		foreach (AIAgent a : mates)
		{
			SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
			if (!ca || !ca.m_UtilityComponent || !ca.m_UtilityComponent.m_CommsHandler)
				continue;
			IEntity mate = ca.GetControlledEntity();
			if (!mate || mate == me || !DCO_Util.IsAlive(mate) || vector.DistanceSq(mate.GetOrigin(), me.GetOrigin()) > 2500.0)
				continue;
			if (now - DCO_ShotCounter.LastShotAt(mate) > 3000.0)
				continue;
			ca.m_UtilityComponent.m_CommsHandler.AddRequest(new SCR_AITalkRequest(ECommunicationType.REPORT_COVERING, null, vector.Zero, 0, false, false, SCR_EAITalkRequestPreset.MEDIUM));
			DCO_Perf.Count("opp:callout_covering");
			return;
		}
	}

	protected static bool OtherEnemyNear(SCR_AIUtilityComponent u, IEntity tgt, vector p, float r)
	{
		if (!u.m_PerceptionComponent)
			return false;
		if (!s_aT)
			s_aT = {};
		s_aT.Clear();
		u.m_PerceptionComponent.GetTargetsList(s_aT, ETargetCategory.ENEMY);
		float r2 = r * r;
		foreach (BaseTarget o : s_aT)
		{
			if (!o || o.GetTargetEntity() == tgt || o.GetTimeSinceSeen() > 5.0 || !DCO_Util.IsAlive(o.GetTargetEntity()))
				continue;
			if (vector.DistanceSqXZ(o.GetLastSeenPosition(), p) <= r2)
				return true;
		}
		return false;
	}

	protected static bool BuddyCover(SCR_AIGroup grp, IEntity me, float now)
	{
		array<AIAgent> bud = {};
		grp.GetAgents(bud);
		foreach (AIAgent a : bud)
		{
			IEntity e = a.GetControlledEntity();
			if (!e || e == me || !DCO_Util.IsAlive(e) || vector.DistanceSq(e.GetOrigin(), me.GetOrigin()) > 900.0)
				continue;
			if (now - DCO_ShotCounter.LastShotAt(e) <= 5000.0)
				return true;
		}
		return false;
	}

	protected static int TeamFiring(SCR_AIGroup grp, IEntity me, IEntity enemy, float now, out bool mg)
	{
		mg = false;
		int n = 0;
		array<AIAgent> mates = {};
		grp.GetAgents(mates);
		foreach (AIAgent a : mates)
		{
			SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
			if (!ca || !ca.m_UtilityComponent || !ca.m_UtilityComponent.m_CombatComponent)
				continue;
			IEntity mate = ca.GetControlledEntity();
			if (!mate || mate == me || !DCO_Util.IsAlive(mate))
				continue;
			BaseTarget mt = ca.m_UtilityComponent.m_CombatComponent.GetCurrentTarget();
			if (!mt || mt.GetTargetEntity() != enemy)
				continue;
			if (now - DCO_ShotCounter.LastShotAt(mate) <= 6000.0)
			{
				n++;
				if (DCO_Util.IsMGShooter(mate))
					mg = true;
			}
		}
		return n;
	}

	static void Tick(SCR_AIUtilityComponent u, float now)
	{
		if (!u || !u.m_OwnerEntity)
			return;
		DCO_OppState st = StateOf(u);
		if (now < st.m_fNextTick_ms)
			return;
		st.m_fNextTick_ms = now + 1000.0;
		int fl = Flags(u);
		bool anyMove = (fl & 70) != 0;
		bool cluster = (fl & 8) != 0;
		if (!anyMove && !cluster)
			return;
		if (Gate(u, false) != "")
			return;
		if (cluster && now >= st.m_fNextCluster_ms)
		{
			st.m_fNextCluster_ms = now + 2000.0;
			DCO_GrenadeUtility.TryOppCluster(u);
		}
		if (!anyMove)
			return;
		if (DCO_Roles.Get(u) & 1)
		{
			DCO_Perf.Count("opp:move_gate_mg_base");
			DCO_Perf.Count("opp:move_gate_mg_base_" + SideTag(u));
			return;
		}
		string mg = Gate(u, true);
		if (mg != "")
		{
			DCO_Perf.Count("opp:move_gate_" + mg);
			DCO_Perf.Count("opp:move_gate_" + mg + "_" + SideTag(u));
			return;
		}
		SCR_AIGroup grp = SCR_AIGroup.Cast(u.GetAIAgent().GetParentGroup());
		BaseTarget t = u.m_CombatComponent.GetCurrentTarget();
		if (!t)
		{
			DCO_Perf.Count("opp:move_no_target_" + SideTag(u));
			return;
		}
		ChimeraCharacter tc = ChimeraCharacter.Cast(t.GetTargetEntity());
		if (!tc || tc.IsInVehicle() || !DCO_Util.IsAlive(tc))
			return;
		IEntity me = u.m_OwnerEntity;
		vector ep = t.GetLastSeenPosition();
		float d = vector.Distance(me.GetOrigin(), ep);
		if ((fl & 64) && now >= st.m_fReloadNext_ms && TryReload(u, st, grp, t, tc, d, now))
			return;
		if ((fl & 4) && now >= st.m_fPushNext_ms && TryPush(u, st, grp, t, tc, d, now))
			return;
		if ((fl & 2) && now >= st.m_fSideNext_ms)
			TrySide(u, st, grp, t, tc, d, now);
	}

	protected static string SideTag(SCR_AIUtilityComponent u)
	{
		string side = DCO_GlobalAIComponent.DCO_FactionOf(u.m_OwnerEntity);
		if (side.IsEmpty())
			return "none";
		return side;
	}

	protected static bool TryPush(SCR_AIUtilityComponent u, DCO_OppState st, SCR_AIGroup grp, BaseTarget t, ChimeraCharacter tc, float d, float now)
	{
		float minD = DCO_Stress.T(u, "opp.push_min_m", 35.0);
		if (t.GetTimeSinceSeen() > 10.0)
		{
			DCO_Perf.Count("opp:push_skip_unseen");
			return false;
		}
		if (d < minD + 10.0 || d > 250.0)
		{
			DCO_Perf.Count("opp:push_skip_range");
			return false;
		}
		float quiet = QuietS(tc);
		float af = AggrF(u);
		float needQuiet = DCO_Stress.T(u, "opp.push_quiet_s", 5.0) / af;
		if (quiet < needQuiet || t.GetTimeSinceEndangered() < needQuiet)
		{
			DCO_Perf.Count("opp:push_skip_not_quiet");
			return false;
		}
		IEntity me = u.m_OwnerEntity;
		bool mgFiring;
		int firing = TeamFiring(grp, me, tc, now, mgFiring);
		if (!mgFiring && firing < 2)
		{
			DCO_Perf.Count("opp:push_skip_no_cover_fire");
			return false;
		}
		if (u.m_ThreatSystem && u.m_ThreatSystem.GetSuppressionMeasure() >= DCO_Stress.T(u, "tm.supp_s1", 0.6))
		{
			DCO_Perf.Count("opp:push_skip_mover_suppressed");
			return false;
		}
		int alive;
		int movers = OppMovers(grp, alive);
		int cap = Math.Max(1, Math.Floor(alive * Math.Min(0.5 * af, 0.75)));
		if (movers >= cap)
		{
			DCO_Perf.Count("opp:push_skip_movers");
			return false;
		}
		st.m_fPushNext_ms = now + 1000.0 * DCO_Stress.T(u, "opp.push_cd_s", 15.0) / af;
		vector dir = t.GetLastSeenPosition() - me.GetOrigin();
		dir[1] = 0;
		dir.Normalize();
		float step = Math.Clamp(d - minD, 10.0, Math.Max(10.0, DCO_Stress.T(u, "opp.push_step_m", 25.0)));
		vector pos = DCO_Util.Surface(me.GetOrigin() + dir * step);
		DCO_RelocateBehavior rb = DCO_RelocateBehavior.Start(u, pos, t.GetLastSeenPosition(), true, ECharacterStance.CROUCH, 12.0, "opp_push");
		if (!rb)
		{
			DCO_Perf.Count("opp:push_skip_blocked");
			return false;
		}
		rb.DCO_SetRequireCover(true);
		DCO_Perf.Count("opp:push");
		Callout(u, grp, tc);
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("opp_push unit=%1 tgt=%2 dist_before=%3 dist_after=%4 quiet_s=%5 team_firing=%6 mg=%7 movers=%8 aggr=%9",
			DCO_Log.IdUnit(me), DCO_Log.IdUnit(tc), Math.Round(d), Math.Round(d - step), Math.Round(quiet), firing, DCO_Util.B01(mgFiring), (movers + 1).ToString() + "/" + alive.ToString(), af.ToString(-1, 2)));
		return true;
	}

	protected static bool TryReload(SCR_AIUtilityComponent u, DCO_OppState st, SCR_AIGroup grp, BaseTarget t, ChimeraCharacter tc, float d, float now)
	{
		if (d < 10.0 || d > 40.0 || t.GetTimeSinceSeen() > 1.5)
			return false;
		CharacterControllerComponent cc = tc.GetCharacterController();
		if (!cc || !cc.IsReloading())
			return false;
		if (OtherEnemyNear(u, tc, t.GetLastSeenPosition(), 30.0))
		{
			DCO_Perf.Count("opp:reload_skip_not_alone");
			return false;
		}
		int alive;
		int movers = OppMovers(grp, alive);
		if (movers >= Math.Max(1, alive / 2))
		{
			DCO_Perf.Count("opp:reload_skip_movers");
			return false;
		}
		st.m_fReloadNext_ms = now + 10000.0 / AggrF(u);
		IEntity me = u.m_OwnerEntity;
		vector dir = t.GetLastSeenPosition() - me.GetOrigin();
		dir[1] = 0;
		dir.Normalize();
		float step = Math.Clamp(d - 8.0, 3.0, 8.0);
		vector pos = DCO_Util.Surface(me.GetOrigin() + dir * step);
		DCO_RelocateBehavior rb = DCO_RelocateBehavior.Start(u, pos, t.GetLastSeenPosition(), true, ECharacterStance.CROUCH, 6.0, "opp_reload");
		if (!rb)
		{
			DCO_Perf.Count("opp:reload_skip_blocked");
			return false;
		}
		rb.DCO_SetMove(ECharacterStance.CROUCH, EMovementType.SPRINT, 5.0, true);
		DCO_Perf.Count("opp:reload_rush");
		Callout(u, grp, tc);
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("opp_reload unit=%1 tgt=%2 dist=%3 step=%4 movers=%5/%6",
			DCO_Log.IdUnit(me), DCO_Log.IdUnit(tc), Math.Round(d), Math.Round(step), movers + 1, alive));
		return true;
	}

	protected static bool TrySide(SCR_AIUtilityComponent u, DCO_OppState st, SCR_AIGroup grp, BaseTarget t, ChimeraCharacter tc, float d, float now)
	{
		if (d < 40.0 || d > 150.0 || t.GetTimeSinceSeen() <= 1.0 || t.GetTimeSinceSeen() > 3.0)
			return false;
		IEntity me = u.m_OwnerEntity;
		if (!Distracted(t, tc, me))
			return false;
		if (!BuddyCover(grp, me, now))
		{
			DCO_Perf.Count("opp:distracted_skip_no_buddy");
			return false;
		}
		if (OtherEnemyNear(u, tc, t.GetLastSeenPosition(), 50.0))
		{
			DCO_Perf.Count("opp:distracted_skip_not_alone");
			return false;
		}
		int alive;
		int movers = OppMovers(grp, alive);
		if (movers >= Math.Max(1, alive / 4))
		{
			DCO_Perf.Count("opp:distracted_skip_movers");
			return false;
		}
		vector ep = t.GetLastSeenPosition();
		vector axis = ep - me.GetOrigin();
		axis[1] = 0;
		axis.Normalize();
		vector mat[4];
		tc.GetWorldTransform(mat);
		vector efwd = mat[2];
		efwd[1] = 0;
		float ang = Math.RandomFloat(30.0, 60.0);
		float run = Math.RandomFloat(15.0, 30.0);
		vector a = DCO_Util.Surface(me.GetOrigin() + DCO_CommanderOps.RotateXZ(axis, ang) * run);
		vector b = DCO_Util.Surface(me.GetOrigin() + DCO_CommanderOps.RotateXZ(axis, -ang) * run);
		vector pos = a;
		if (vector.Dot(efwd, (b - ep).Normalized()) < vector.Dot(efwd, (a - ep).Normalized()))
			pos = b;
		st.m_fSideNext_ms = now + 1000.0 * DCO_Stress.T(u, "opp.side_cd_s", 20.0) / AggrF(u);
		DCO_RelocateBehavior rb = DCO_RelocateBehavior.Start(u, pos, ep, true, ECharacterStance.CROUCH, 14.0, "opp_side");
		if (!rb)
		{
			DCO_Perf.Count("opp:distracted_skip_blocked");
			return false;
		}
		rb.DCO_SetRequireCover(true);
		DCO_Perf.Count("opp:distracted_sidestep");
		Callout(u, grp, tc);
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("opp_distracted unit=%1 tgt=%2 dist=%3 facing_deg=%4 action=sidestep move=%5 angle=%6",
			DCO_Log.IdUnit(me), DCO_Log.IdUnit(tc), Math.Round(d), Math.Round(FacingAwayDeg(tc, me)), Math.Round(run), Math.Round(ang)));
		return true;
	}

	static bool PatientHold(SCR_AIUtilityComponent u, BaseTarget t, bool visible, EWeaponType wt, float d)
	{
		if (!t || !visible || !(Flags(u) & 16) || d <= 50.0 || wt == EWeaponType.WT_MACHINEGUN)
			return false;
		if (DCO_Roles.Get(u) & 1)
			return false;
		if (DCO_TeamBound.OverwatchFor(u))
		{
			DCO_Perf.Count("opp:patient_skip_overwatch");
			return false;
		}
		AIAgent pag = u.GetAIAgent();
		if (pag && pag.GetParentGroup())
		{
			DCO_GroupUtilityComponent pdg = DCO_GroupUtilityComponent.Cast(pag.GetParentGroup().FindComponent(DCO_GroupUtilityComponent));
			if (pdg && pdg.GetTask() == DCO_EGroupTask.SUPPORT_BY_FIRE)
				return false;
		}
		ChimeraCharacter tc = ChimeraCharacter.Cast(t.GetTargetEntity());
		if (!tc || tc.IsInVehicle() || t.IsEndangering() || t.GetTimeSinceEndangered() < 3.0)
			return false;
		float now = GetGame().GetWorld().GetWorldTime();
		DCO_OppState st = StateOf(u);
		if (st.m_PatientTgt != tc)
		{
			st.m_PatientTgt = tc;
			st.m_fPatientFrom_ms = -1;
			st.m_fPatientFreeUntil_ms = -1;
			st.m_fPatientCheck_ms = 0;
		}
		if (now < st.m_fPatientFreeUntil_ms)
			return false;
		if (now >= st.m_fPatientCheck_ms)
		{
			st.m_fPatientCheck_ms = now + 500.0;
			st.m_bPatientPartial = Partial(u.m_OwnerEntity, tc);
		}
		string fired;
		if (!st.m_bPatientPartial)
		{
			if (st.m_fPatientFrom_ms < 0)
				return false;
			fired = "exposed";
		}
		else if (Speed(tc) > 1.0)
			fired = "move";
		else if (Standing(tc))
			fired = "stand";
		else if (st.m_fPatientFrom_ms >= 0 && now - st.m_fPatientFrom_ms >= 1000.0 * DCO_Stress.T(u, "opp.patient_max_s", 6.0) * PatientScale(d))
			fired = "timeout";
		if (fired == "")
		{
			if (st.m_fPatientFrom_ms < 0)
			{
				st.m_fPatientFrom_ms = now;
				DCO_Perf.Count("opp:patient_hold");
			}
			return true;
		}
		float held = 0;
		if (st.m_fPatientFrom_ms >= 0)
			held = 0.001 * (now - st.m_fPatientFrom_ms);
		st.m_fPatientFrom_ms = -1;
		st.m_fPatientFreeUntil_ms = now + 1500.0;
		if (fired == "move" || fired == "stand")
			st.m_fPatientFreeUntil_ms = now + 3000.0;
		DCO_Perf.Count("opp:patient_" + fired);
		if (DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG))
			DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(u.m_OwnerEntity), string.Format("opp_patient unit=%1 tgt=%2 dist=%3 held_s=%4 fired_on=%5",
				DCO_Log.IdUnit(u.m_OwnerEntity), DCO_Log.IdUnit(tc), Math.Round(d), held.ToString(-1, 1), fired));
		return false;
	}

	protected static float PatientScale(float d)
	{
		if (d > 150.0)
			return 1.33;
		return 0.67;
	}

	protected static bool Partial(IEntity me, ChimeraCharacter tc)
	{
		ChimeraCharacter mc = ChimeraCharacter.Cast(me);
		if (!mc)
			return false;
		vector eye = mc.EyePosition();
		vector head = tc.EyePosition();
		vector chest = tc.GetOrigin() + Vector(0, 0.9, 0);
		if (head[1] - tc.GetOrigin()[1] < 1.1)
			chest = tc.GetOrigin() + Vector(0, 0.35, 0);
		bool headClear = DCO_Util.TraceClear(eye, head, me, 0.97, TraceFlags.WORLD | TraceFlags.ENTS, EPhysicsLayerDefs.Projectile, tc, "t:DCO_OppPartial");
		bool bodyClear = DCO_Util.TraceClear(eye, chest, me, 0.97, TraceFlags.WORLD | TraceFlags.ENTS, EPhysicsLayerDefs.Projectile, tc, "t:DCO_OppPartial");
		return headClear && !bodyClear;
	}

	static bool LootSafe(SCR_AIUtilityComponent u, IEntity corpse)
	{
		if (!u.m_PerceptionComponent || !corpse)
			return false;
		if (!s_aT)
			s_aT = {};
		s_aT.Clear();
		u.m_PerceptionComponent.GetTargetsList(s_aT, ETargetCategory.ENEMY);
		vector cp = corpse.GetOrigin() + Vector(0, 1.0, 0);
		foreach (BaseTarget t : s_aT)
		{
			if (!t || t.GetTimeSinceSeen() > 30.0)
				continue;
			vector ep = t.GetLastSeenPosition() + Vector(0, 1.6, 0);
			if (vector.Distance(ep, cp) > 400.0)
				continue;
			if (DCO_Util.TraceClear(ep, cp, corpse, 0.97, TraceFlags.WORLD, EPhysicsLayerDefs.Projectile, null, "t:DCO_OppLoot"))
				return false;
		}
		return true;
	}
}
