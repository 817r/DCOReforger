class DCO_Explosion
{
	protected static ref map<IEntity, float> s_mProneUntil;
	protected static ref map<IEntity, float> s_mProneFirst;
	protected static ref map<IEntity, bool> s_mCrouchOnly;
	protected static ref map<IEntity, float> s_mDeafUntil;
	protected static ref map<IEntity, float> s_mIDFReported;

	protected static ref array<IEntity> s_aBadFrag;

	static void CountBadFrag(IEntity grenade)
	{
		if (!s_aBadFrag)
			s_aBadFrag = {};
		if (s_aBadFrag.Contains(grenade))
			return;
		if (s_aBadFrag.Count() >= 16)
			s_aBadFrag.Remove(0);
		s_aBadFrag.Insert(grenade);
		DCO_Perf.Count("gren:badfrag");
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, "-", string.Format("grenade_badfrag pos=%1", grenade.GetOrigin()));
	}

	static float DeafFactor(IEntity ent)
	{
		if (!s_mDeafUntil || !ent)
			return 1.0;
		float until;
		if (!s_mDeafUntil.Find(ent, until))
			return 1.0;
		float now = GetGame().GetWorld().GetWorldTime();
		if (now >= until)
		{
			s_mDeafUntil.Remove(ent);
			return 1.0;
		}
		return 0.3 + 0.7 * (1.0 - (until - now) / 10000.0);
	}

	static bool React(SCR_AIUtilityComponent u, SCR_AIThreatSystem ts, AIDangerEvent ev)
	{
		IEntity me = u.m_OwnerEntity;
		if (!me)
			return false;
		vector pos = ev.GetPosition();
		float dist = vector.Distance(me.GetOrigin(), pos);
		if (dist > SCR_AIThreatSystem.EXPLOSION_MAX_DISTANCE)
			return false;
		DCO_Perf.Count("ev:explosion");

		IEntity src = ev.GetObject();
		if (src)
			src = src.GetRootParent();
		DCO_PostFrag.OnBurst(src, pos);
		bool large = !src || !ChimeraCharacter.Cast(src);
		SCR_ChimeraAIAgent agent = SCR_ChimeraAIAgent.Cast(u.GetOwner());
		bool friendly = src && (src == me || (u.IsMilitary() && agent && !agent.IsPerceivedEnemy(src)));

		if (friendly)
		{
			if (dist < 15.0)
				ts.DCO_ThreatExplosion(dist, 0.5);
		}
		else
		{
			ts.ThreatExplosion(dist);
			u.m_SectorThreatFilter.OnExplosion(DCO_FrontHold.ExplosionThreatPos(src, pos));
			if (dist < 60.0 && agent)
			{
				SCR_AIGroup eg = SCR_AIGroup.Cast(agent.GetParentGroup());
				if (eg)
				{
					SCR_AIGroupUtilityComponent egu = SCR_AIGroupUtilityComponent.Cast(eg.FindComponent(SCR_AIGroupUtilityComponent));
					if (egu)
						egu.DCO_NoteContact("explosion");
				}
			}
		}

		ChimeraCharacter ch = ChimeraCharacter.Cast(me);
		float proneR = 20.0;
		float shockR = 8.0;
		if (large)
		{
			proneR = 45.0;
			shockR = 12.0;
		}
		if (ch && !ch.IsInVehicle() && dist < proneR)
			HitDirt(u, pos);
		if (dist < shockR && (!friendly || dist < 8.0))
			Shock(u, pos, dist, shockR);

		if (friendly)
			return true;

		if (large)
			GroupImpact(u, pos, src);
		else if (src && dist < 60.0 && agent)
			GuessShooter(u, src, dist);
		return true;
	}

	protected static void HitDirt(SCR_AIUtilityComponent u, vector pos)
	{
		IEntity me = u.m_OwnerEntity;
		float now = GetGame().GetWorld().GetWorldTime();
		float dur = 3.0;
		if (u.m_DCOConfig)
		{
			switch (u.m_DCOConfig.GetPersonality())
			{
				case DCO_EAIPersonality.CAUTIOUS:	dur = 4.0; break;
				case DCO_EAIPersonality.AGGRESSIVE:	dur = 2.0; break;
				case DCO_EAIPersonality.RECKLESS:	dur = 1.0; break;
			}
		}
		DCO_AIMoraleSystem morale = u.GetMoraleSystem();
		if (morale && (morale.GetState() == moraleState.SHAKEN || morale.GetState() == moraleState.PANIC || morale.GetState() == moraleState.BREAK))
			dur *= 1.5;

		if (!s_mProneUntil)
		{
			s_mProneUntil = new map<IEntity, float>();
			s_mProneFirst = new map<IEntity, float>();
			s_mCrouchOnly = new map<IEntity, bool>();
		}
		bool wasEmpty = s_mProneUntil.IsEmpty();
		float first;
		if (!s_mProneFirst.Find(me, first) || !s_mProneUntil.Contains(me))
		{
			first = now;
			s_mProneFirst.Set(me, now);
			DCO_Perf.Count("exp_prone");
		}
		s_mProneUntil.Set(me, Math.Min(now + dur * 1000.0, first + 8000.0));
		s_mCrouchOnly.Set(me, InBuildingPost(u));
		if (u.m_LookAction)
			u.m_LookAction.LookAt(pos, 60.0, dur);
		Keep(me, now);
		if (wasEmpty)
			GetGame().GetCallqueue().CallLater(KeepAll, 500, true);
	}

	protected static bool InBuildingPost(SCR_AIUtilityComponent u)
	{
		AIAgent agent = u.GetAIAgent();
		if (!agent || !agent.GetParentGroup())
			return false;
		DCO_GroupUtilityComponent g = DCO_GroupUtilityComponent.Cast(agent.GetParentGroup().FindComponent(DCO_GroupUtilityComponent));
		if (!g || (g.GetTask() != DCO_EGroupTask.GARRISON && g.GetTask() != DCO_EGroupTask.DEFEND))
			return false;
		return SCR_CoverManagerComponent.IsEntityInsideBuilding(u.m_OwnerEntity);
	}

	protected static void KeepAll()
	{
		float now = GetGame().GetWorld().GetWorldTime();
		array<IEntity> done = {};
		foreach (IEntity e, float until : s_mProneUntil)
		{
			if (!e || now >= until || !DCO_Util.IsAlive(e))
				done.Insert(e);
			else
				Keep(e, now);
		}
		foreach (IEntity d : done)
		{
			s_mProneUntil.Remove(d);
			s_mProneFirst.Remove(d);
			s_mCrouchOnly.Remove(d);
		}
		if (s_mProneUntil.IsEmpty())
			GetGame().GetCallqueue().Remove(KeepAll);
	}

	protected static void Keep(IEntity e, float now)
	{
		ChimeraCharacter ch = ChimeraCharacter.Cast(e);
		if (!ch || ch.IsInVehicle())
			return;
		SCR_CharacterControllerComponent ctrl = SCR_CharacterControllerComponent.Cast(ch.GetCharacterController());
		if (!ctrl || DCO_Nav.Busy(ctrl, "blast_stance"))
			return;
		if (s_mCrouchOnly.Get(e) || !DCO_CQC.ProneRoll(e))
		{
			if (ctrl.GetStance() == ECharacterStance.STAND)
				DCO_Stance.Request(ctrl, 2, "blast");
			return;
		}
		if (ctrl.GetStance() != ECharacterStance.PRONE)
			DCO_Stance.Request(ctrl, 3, "blast");
	}

	protected static void Shock(SCR_AIUtilityComponent u, vector pos, float dist, float radius)
	{
		float closeness = Math.Clamp(1.0 - dist / radius, 0.1, 1.0);
		if (u.m_CombatComponent)
			u.m_CombatComponent.DangerSuppressedDecreaseAIM(Math.Round(1 + closeness * 3));
		if (u.m_LookAction)
			u.m_LookAction.LookAt(pos, 70.0, 2.0);
		if (!s_mDeafUntil)
			s_mDeafUntil = new map<IEntity, float>();
		s_mDeafUntil.Set(u.m_OwnerEntity, GetGame().GetWorld().GetWorldTime() + 10000.0);
		DCO_Perf.Count("exp_shock");
	}

	protected static void GuessShooter(SCR_AIUtilityComponent u, IEntity shooter, float dist)
	{
		AIAgent agent = u.GetAIAgent();
		if (!agent || !agent.GetParentGroup())
			return;
		SCR_AIGroupUtilityComponent gu = SCR_AIGroupUtilityComponent.Cast(agent.GetParentGroup().FindComponent(SCR_AIGroupUtilityComponent));
		PerceptionManager pm = GetGame().GetPerceptionManager();
		if (!gu || !gu.m_Perception || !pm)
			return;
		float pmNow = pm.GetTime();
		int idx = gu.m_Perception.m_aTargetEntities.Find(shooter);
		if (idx >= 0 && pmNow - gu.m_Perception.m_aTargets[idx].m_fTimestamp < 2.0)
			return;

		float sd = vector.Distance(u.m_OwnerEntity.GetOrigin(), shooter.GetOrigin());
		float err = sd * Math.RandomFloat(0.15, 0.3);
		float ang = Math.RandomFloat(0, Math.PI2);
		vector est = DCO_Util.Surface(shooter.GetOrigin() + Vector(Math.Cos(ang) * err, 0, Math.Sin(ang) * err));
		Faction f;
		FactionAffiliationComponent fac = FactionAffiliationComponent.Cast(shooter.FindComponent(FactionAffiliationComponent));
		if (fac)
			f = fac.GetAffiliatedFaction();
		gu.m_Perception.AddOrUpdateGunshot(shooter, est, f, pmNow, true);
		DCO_Perf.Count("exp_gl_source");
	}

	protected static void GroupImpact(SCR_AIUtilityComponent u, vector pos, IEntity src)
	{
		AIAgent agent = u.GetAIAgent();
		if (!agent || !agent.GetParentGroup())
			return;
		SCR_AIGroup grp = SCR_AIGroup.Cast(agent.GetParentGroup());
		DCO_GroupUtilityComponent g = DCO_GroupUtilityComponent.Cast(agent.GetParentGroup().FindComponent(DCO_GroupUtilityComponent));
		if (!grp || !g || !grp.GetLeaderEntity())
			return;

		float now = DCO_Util.NowS();
		if (!g.m_aDCOImpactPos)
		{
			g.m_aDCOImpactPos = {};
			g.m_aDCOImpactAt = {};
		}
		foreach (int i, vector p : g.m_aDCOImpactPos)
		{
			if (now - g.m_aDCOImpactAt[i] < 1.0 && vector.DistanceXZ(p, pos) < 3.0)
				return;
		}
		if (g.m_aDCOImpactPos.Count() >= 6)
		{
			g.m_aDCOImpactPos.Remove(0);
			g.m_aDCOImpactAt.Remove(0);
		}
		g.m_aDCOImpactPos.Insert(pos);
		g.m_aDCOImpactAt.Insert(now);
		DCO_Awareness.MarkIDF(pos);

		vector lp = grp.GetLeaderEntity().GetOrigin();
		if (vector.DistanceXZ(lp, pos) <= 150.0)
			Report(g, grp, src, now);

		if (g.IsPlayerGroup())
			return;
		vector sum;
		int n;
		foreach (int j, vector q : g.m_aDCOImpactPos)
		{
			if (now - g.m_aDCOImpactAt[j] <= 30.0 && vector.DistanceXZ(q, lp) <= 60.0)
			{
				sum = sum + q;
				n++;
			}
		}
		if (n >= 2 && now - g.m_fDCODisplacedAt > 60.0)
			Displace(g, grp, lp, sum * (1.0 / n), now);
	}

	protected static void Report(DCO_GroupUtilityComponent g, SCR_AIGroup grp, IEntity src, float now)
	{
		AICommander_BaseComponent cmd = g.GetMyCommander();
		if (src && cmd && cmd.GetDefense())
		{
			if (!s_mIDFReported)
				s_mIDFReported = new map<IEntity, float>();
			float last;
			if (!s_mIDFReported.Find(src, last) || now - last > 3.0)
			{
				s_mIDFReported.Set(src, now);
				cmd.GetDefense().OnIndirectFire(cmd, src, src.GetOrigin(), now);
				DCO_Perf.Count("exp_idf_report");
			}
		}

		if (now - g.m_fDCOIDFRadioAt < 60.0 || !grp.GetFaction())
			return;
		g.m_fDCOIDFRadioAt = now;
		string callsign = DCO_PlayerComms.GetCallsign(grp);
		vector lp = grp.GetLeaderEntity().GetOrigin();
		array<int> players = {};
		GetGame().GetPlayerManager().GetPlayers(players);
		foreach (int pid : players)
		{
			IEntity pe = GetGame().GetPlayerManager().GetPlayerControlledEntity(pid);
			Faction pf = SCR_FactionManager.SGetPlayerFaction(pid);
			if (!pe || pf != grp.GetFaction() || vector.DistanceXZ(pe.GetOrigin(), lp) > 500.0)
				continue;
			DCO_Radio.Player(pid, callsign, "idf_under_fire", DCO_ERadioKind.WARNING, DCO_Radio.P("callsign", callsign, "grid", DCO_PlayerComms.Grid(lp)));
		}
	}

	protected static bool UnderCover(IEntity e)
	{
		if (!e)
			return false;
		if (SCR_CoverManagerComponent.IsEntityInsideBuilding(e))
			return true;
		vector p = e.GetOrigin();
		return !DCO_Util.TraceClear(p + Vector(0, 1.5, 0), p + Vector(0, 12.0, 0), e, 0.99, TraceFlags.WORLD | TraceFlags.ENTS, -1, null, "t:idf_roof");
	}

	protected static void Displace(DCO_GroupUtilityComponent g, SCR_AIGroup grp, vector lp, vector impact, float now)
	{
		AICommander_BaseComponent cmd = g.GetMyCommander();
		int pers = DCO_Radio.PersonalityOf(cmd);
		string mode;

		if (pers == 1 && g.GetTask() == DCO_EGroupTask.ATTACK && g.GetPhase() == DCO_ETaskPhase.EXECUTING && g.GetGroupObjective()
			&& vector.DistanceXZ(lp, g.GetGroupObjective().GetOwner().GetOrigin()) < 150.0)
		{
			g.m_fDCODisplacedAt = now;
			DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, "-", string.Format("exp_displace grp=%1 mode=push", grp));
			return;
		}

		array<AIAgent> agents = {};
		grp.GetAgents(agents);
		int inside = 0;
		foreach (AIAgent ia : agents)
		{
			if (ia && DCO_Util.IsAlive(ia.GetControlledEntity()) && UnderCover(ia.GetControlledEntity()))
				inside++;
		}
		bool prepared = DCO_DefenseTactics.InPreparedPosition(g, lp);
		if (inside > 0 || prepared)
		{
			int stayed = 0;
			int movedOpen = 0;
			foreach (AIAgent sa : agents)
			{
				SCR_ChimeraAIAgent sca = SCR_ChimeraAIAgent.Cast(sa);
				if (!sca || !sca.m_UtilityComponent || !DCO_Util.IsAlive(sca.GetControlledEntity()))
					continue;
				IEntity se = sca.GetControlledEntity();
				vector sp = se.GetOrigin();
				if (UnderCover(se) || vector.DistanceXZ(sp, impact) > 25.0)
				{
					stayed++;
					continue;
				}
				vector rally = lp;
				if (vector.DistanceXZ(lp, impact) < 30.0)
					rally = sp + DCO_Specialist.Flat(sp - impact) * 30.0;
				DCO_RelocateBehavior.Start(sca.m_UtilityComponent, DCO_Util.Surface(rally), impact, true, ECharacterStance.CROUCH, vector.DistanceXZ(sp, rally) / 3.5 + 3.0, "idf_rally");
				movedOpen++;
			}
			g.m_fDCODisplacedAt = now;
			DCO_Perf.Count("idf_stay");
			DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, "-", string.Format("idf_stay grp=%1 stayed=%2 moved_open=%3 inside=%4 prepared=%5", DCO_Log.IdGroup(grp), stayed, movedOpen, inside, prepared));
			if (prepared)
			{
				DCO_Perf.Count("def:idf_stay");
				DCO_Log.Write(DCO_ELogChannel.TACTIC, DCO_ELogLevel.INFO, "-", string.Format("def_idf_stay obj=%1 grp=%2 stayed=%3 moved_open=%4", g.GetGroupObjective().GetLogName(), DCO_Log.IdGroup(grp), stayed, movedOpen));
			}
			return;
		}
		if (g.GetTask() == DCO_EGroupTask.DEFEND || g.GetTask() == DCO_EGroupTask.GARRISON)
		{
			DCO_GarrisonRegistry reg = DCO_GarrisonRegistry.GetInstance();
			if (!reg)
				return;
			array<IEntity> buildings = {};
			reg.FindBuildings(lp, 100.0, buildings);
			int spreadN = 1;
			if (DCO_GlobalAIComponent.BehaviorOnFor(grp, DCO_EBehavior.AWARENESS))
				spreadN = Math.Min(3, Math.Max(1, agents.Count() / 3));
			array<IEntity> picks = {};
			while (picks.Count() < spreadN)
			{
				IEntity best;
				float bestD = float.MAX;
				foreach (IEntity b : buildings)
				{
					if (picks.Contains(b))
						continue;
					float d = vector.DistanceSqXZ(b.GetOrigin(), lp) - vector.DistanceSqXZ(b.GetOrigin(), impact) * 0.25;
					if (d < bestD)
					{
						bestD = d;
						best = b;
					}
				}
				if (!best)
					break;
				picks.Insert(best);
			}
			if (picks.IsEmpty())
				return;
			foreach (int ai, AIAgent a : agents)
			{
				SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
				if (ca && ca.m_UtilityComponent && DCO_Util.IsAlive(ca.GetControlledEntity()))
					DCO_RelocateBehavior.Start(ca.m_UtilityComponent, picks[ai % picks.Count()].GetOrigin() + Vector(Math.RandomFloat(-3, 3), 0, Math.RandomFloat(-3, 3)), impact, false, ECharacterStance.CROUCH, vector.DistanceXZ(ca.GetControlledEntity().GetOrigin(), picks[ai % picks.Count()].GetOrigin()) / 3.5 + 3.0, "idf_hard_cover");
			}
			mode = "hard_cover";
		}
		else
		{
			vector dir = DCO_Specialist.Flat(lp - impact);
			SCR_AIGroupUtilityComponent gu = g.GetGroupUtilityComponent();
			PerceptionManager pm = GetGame().GetPerceptionManager();
			if (gu && gu.m_Perception && pm)
			{
				vector esum;
				int en;
				foreach (SCR_AITargetInfo t : gu.m_Perception.m_aTargets)
				{
					if (t && pm.GetTime() - t.m_fTimestamp < 60.0)
					{
						esum = esum + t.m_vWorldPos;
						en++;
					}
				}
				if (en > 0)
				{
					vector toEnemy = DCO_Specialist.Flat(esum * (1.0 / en) - lp);
					if (vector.Dot(dir, toEnemy) > 0.5)
					{
						vector side = Vector(toEnemy[2], 0, -toEnemy[0]);
						if (vector.Dot(side, dir) < 0)
							side = side * -1.0;
						dir = side;
					}
				}
			}
			vector goal = lp + dir * Math.RandomFloat(150.0, 250.0);
			vector cover = DCO_TerrainCache.QueryBest(DCO_ETerrainFlag.FOREST_EDGE, goal, 60.0, string.Empty, goal);
			if (cover != vector.Zero)
				goal = cover;
			goal = DCO_Util.Surface(goal);

			if (cmd && grp.GetCurrentWaypoint() && !g.IsInTransport())
			{
				SCR_AIWaypoint wp = cmd.SpawnMoveWP(goal);
				if (wp)
					grp.AddWaypointAt(wp, 0);
				mode = "detour";
			}
			else if (DCO_BreakContact.Start(g, impact, goal, "beaten_zone"))
				mode = "break_contact";
			else
			{
				foreach (AIAgent a2 : agents)
				{
					SCR_ChimeraAIAgent ca2 = SCR_ChimeraAIAgent.Cast(a2);
					if (ca2 && ca2.m_UtilityComponent && DCO_Util.IsAlive(ca2.GetControlledEntity()))
						DCO_RelocateBehavior.Start(ca2.m_UtilityComponent, DCO_Util.Surface(goal + Vector(Math.RandomFloat(-6, 6), 0, Math.RandomFloat(-6, 6))), impact, true, ECharacterStance.CROUCH, vector.DistanceXZ(ca2.GetControlledEntity().GetOrigin(), goal) / 3.5 + 3.0, "idf_move");
				}
				mode = "move";
			}
		}

		g.m_fDCODisplacedAt = now;
		DCO_Perf.Count("exp_displace");
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, "-", string.Format("exp_displace grp=%1 mode=%2 task=%3", grp, mode, typename.EnumToString(DCO_EGroupTask, g.GetTask())));
	}

	static void GrenadeCover(SCR_AIUtilityComponent u, IEntity grenade, float delay_ms)
	{
		IEntity me = u.m_OwnerEntity;
		vector gp = grenade.GetOrigin();
		vector mp = me.GetOrigin();
		float fuse = 1.5;
		TimerTriggerComponent timer = TimerTriggerComponent.Cast(grenade.FindComponent(TimerTriggerComponent));
		if (timer && timer.GetTimer() > 0)
			fuse = timer.GetTimer();
		float reach = Math.Max(fuse - delay_ms / 1000.0, 0) * 5.0;

		vector best = vector.Zero;
		float bestScore = -1;
		for (int i = 0; i < 8; i++)
		{
			float a = 1.0 * i * Math.PI * 0.25;
			float r = Math.Clamp(reach, 2.0, 8.0);
			vector c = DCO_Util.Surface(mp + Vector(Math.Cos(a) * r, 0, Math.Sin(a) * r));
			if (DCO_Util.TraceFrac(gp + Vector(0, 0.3, 0), c + Vector(0, 1.0, 0), grenade, -1, -1, "t:DCO_GrenadeCover") >= 0.99)
				continue;
			float score = vector.DistanceXZ(c, gp);
			if (score > bestScore)
			{
				bestScore = score;
				best = c;
			}
		}

		if (best != vector.Zero)
		{
			u.AddAction(new SCR_AIMoveIndividuallyBehavior(u, null, best, SCR_AIActionBase.PRIORITY_BEHAVIOR_MOVE_FROM_DANGER + 1, SCR_AIActionBase.PRIORITY_LEVEL_NORMAL));
			DCO_Perf.Count("exp_grenade_cover");
			return;
		}
		if (reach < 7.0 - vector.DistanceXZ(mp, gp))
		{
			if (!s_mProneUntil)
			{
				s_mProneUntil = new map<IEntity, float>();
				s_mProneFirst = new map<IEntity, float>();
				s_mCrouchOnly = new map<IEntity, bool>();
			}
			bool wasEmpty = s_mProneUntil.IsEmpty();
			float now = GetGame().GetWorld().GetWorldTime();
			s_mProneUntil.Set(me, now + 4000.0);
			s_mProneFirst.Set(me, now);
			s_mCrouchOnly.Set(me, false);
			Keep(me, now);
			if (wasEmpty)
				GetGame().GetCallqueue().CallLater(KeepAll, 500, true);
			DCO_Perf.Count("exp_grenade_prone");
		}
	}
}

[BaseContainerProps()]
modded class SCR_AIDangerReaction_Explosion
{
	override bool PerformReaction(notnull SCR_AIUtilityComponent utility, notnull SCR_AIThreatSystem threatSystem, AIDangerEvent dangerEvent, int dangerEventCount)
	{
		if (!DCO_GlobalAIComponent.BehaviorOnUtility(utility, DCO_EBehavior.EXPLOSION_REACTIONS))
			return super.PerformReaction(utility, threatSystem, dangerEvent, dangerEventCount);
		int pt = DCO_Perf.Begin();
		bool r = DCO_Explosion.React(utility, threatSystem, dangerEvent);
		DCO_Perf.End("danger_explosion", pt);
		return r;
	}
}

[BaseContainerProps()]
modded class SCR_AIDangerReaction_GrenadeLanding
{
	protected bool DCO_BreachShielded(SCR_AIUtilityComponent utility, SCR_ChimeraCharacter shooter, IEntity grenade)
	{
		if (!DCO_CQBMemberBehavior.Cast(utility.GetCurrentBehavior()))
			return false;
		AIControlComponent sc = AIControlComponent.Cast(shooter.FindComponent(AIControlComponent));
		if (!sc || !sc.GetControlAIAgent() || sc.GetControlAIAgent().GetParentGroup() != utility.GetOwner().GetParentGroup())
			return false;
		IEntity me = utility.m_OwnerEntity;
		vector gp = grenade.GetOrigin() + Vector(0, 0.3, 0);
		if (DCO_Util.TraceFrac(gp, me.GetOrigin() + Vector(0, 1.0, 0), me, TraceFlags.WORLD | TraceFlags.ENTS, EPhysicsLayerDefs.Projectile, "t:cqb_breach_shield") >= 0.95)
			return false;
		IEntity hit = DCO_Util.LastTraceEnt();
		if (hit && (ChimeraCharacter.Cast(hit.GetRootParent()) || hit.GetRootParent() == grenade))
			return false;
		CharacterControllerComponent ctrl = CharacterControllerComponent.Cast(me.FindComponent(CharacterControllerComponent));
		if (ctrl && ctrl.GetStance() == ECharacterStance.STAND)
			DCO_Stance.Set(ctrl, ECharacterStance.CROUCH, "blast");
		DCO_Perf.Count("cqb:breach_hold_cover");
		if (DCO_Log.On(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO))
			DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("cqb_breach_hold_cover unit=%1 dist=%2 thrower=%3",
				DCO_Log.IdUnit(me), Math.Round(vector.Distance(me.GetOrigin(), grenade.GetOrigin()) * 10) / 10, DCO_Log.IdUnit(shooter)));
		return true;
	}

	override bool PerformReaction(notnull SCR_AIUtilityComponent utility, notnull SCR_AIThreatSystem threatSystem, AIDangerEvent dangerEvent, int dangerEventCount)
	{
		IEntity grenadeObject = dangerEvent.GetObject();
		if (!grenadeObject)
			return false;
		SCR_ChimeraCharacter shooter = GetShooterCharacter(grenadeObject);
		if (!shooter)
		{
			vector gp = grenadeObject.GetOrigin();
			if (vector.Distance(utility.GetOrigin(), gp) >= GRENADE_AVOID_RADIUS)
				return false;
			utility.AddAction(new SCR_AIMoveFromGrenadeBehavior(utility, null, vector.Zero, grenadeObject, 300, vector.Zero));
			return true;
		}

		Faction ownerFaction = GetOwnerFaction(utility);
		bool friendly = shooter == utility.m_OwnerEntity || (ownerFaction && shooter.GetFaction() && ownerFaction.IsFactionFriendly(shooter.GetFaction()));
		if (friendly && DCO_GlobalAIComponent.BehaviorOnUtility(utility, DCO_EBehavior.EXPLOSION_REACTIONS))
		{
			if (vector.Distance(utility.GetOrigin(), grenadeObject.GetOrigin()) >= 15.0)
				return false;
			if (DCO_BreachShielded(utility, shooter, grenadeObject))
				return true;
			if (SCR_AIMoveFromDangerBehavior.ExistsBehaviorForEntity(utility, grenadeObject))
				return true;
			float delay = Math.RandomFloat(100.0, 400.0);
			utility.AddAction(new SCR_AIMoveFromGrenadeBehavior(utility, null, grenadeObject.GetOrigin(), grenadeObject, delay, vector.Zero));
			DCO_Explosion.GrenadeCover(utility, grenadeObject, delay);
			DCO_Explosion.CountBadFrag(grenadeObject);
			return true;
		}

		bool reacted = super.PerformReaction(utility, threatSystem, dangerEvent, dangerEventCount);
		if (!reacted || !DCO_GlobalAIComponent.BehaviorOnUtility(utility, DCO_EBehavior.EXPLOSION_REACTIONS))
			return reacted;

		if (GetGrenadeVisibilityFactor(utility.m_OwnerEntity, grenadeObject.GetOrigin()) >= 0.6)
		{
			int pt = DCO_Perf.Begin();
			DCO_Explosion.GrenadeCover(utility, grenadeObject, 200);
			DCO_Perf.End("danger_grenade", pt);
		}
		return true;
	}
}
