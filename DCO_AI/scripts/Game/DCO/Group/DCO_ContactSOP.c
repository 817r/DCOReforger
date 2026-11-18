class DCO_ContactSOP
{
	static bool IsCombatTask(DCO_EGroupTask t)
	{
		return t == DCO_EGroupTask.ATTACK || t == DCO_EGroupTask.FLANK || t == DCO_EGroupTask.REINFORCE || t == DCO_EGroupTask.SUPPORT_BY_FIRE;
	}

	static bool OnObjectiveAxis(DCO_GroupUtilityComponent g, vector lp, vector enemy)
	{
		vector dest;
		float r = 0;
		CMD_AICommanderObjectiveComponent obj = g.GetGroupObjective();
		if (obj)
		{
			dest = obj.GetOwner().GetOrigin();
			r = obj.GetRadius();
		}
		else
		{
			SCR_AIGroup grp = SCR_AIGroup.Cast(g.GetOwner());
			if (!grp || !grp.GetCurrentWaypoint())
				return false;
			dest = grp.GetCurrentWaypoint().GetOrigin();
		}
		if (vector.DistanceXZ(enemy, dest) < r + 150.0)
			return true;
		vector ax = dest - lp;
		ax[1] = 0;
		float lenSq = ax.LengthSq();
		if (lenSq < 1.0)
			return false;
		vector to = enemy - lp;
		float t = (to[0] * ax[0] + to[2] * ax[2]) / lenSq;
		if (t < 0 || t > 1)
			return false;
		return vector.DistanceXZ(enemy, lp + ax * t) < 100.0;
	}

	static DCO_ESOPAction Pick(DCO_GroupUtilityComponent g, float dist, int enemies, bool onAxis, float engageM)
	{
		bool moving = g.GetPhase() == DCO_ETaskPhase.MOVING;
		switch (g.GetTask())
		{
			case DCO_EGroupTask.RECON:
			{
				if (enemies <= 4 && g.GetUnitCount() >= 1.0 * enemies * 1.5 && DCO_Ctx.TuneFor(g.GetOwner(), "sop.recon_ambush", 1.0) > 0)
				{
					if (dist < engageM)
						return DCO_ESOPAction.ENGAGE;
					return DCO_ESOPAction.AMBUSH;
				}
				return DCO_ESOPAction.OBSERVE;
			}
			case DCO_EGroupTask.PATROL:
			{
				if (dist < engageM)
					return DCO_ESOPAction.ENGAGE;
				if (enemies <= 4 && g.GetUnitCount() >= 1.0 * enemies * 1.5)
					return DCO_ESOPAction.AMBUSH;
				return DCO_ESOPAction.OBSERVE;
			}
			case DCO_EGroupTask.ATTACK:
			case DCO_EGroupTask.FLANK:
			{
				if (g.GetPhase() == DCO_ETaskPhase.EXECUTING || dist < engageM || !g.GetMyCommander())
					return DCO_ESOPAction.ENGAGE;
				if (moving)
				{
					if (onAxis || g.GetTask() == DCO_EGroupTask.FLANK)
						return DCO_ESOPAction.ENGAGE;
					return DCO_ESOPAction.BYPASS;
				}
				if (enemies <= 4 && g.GetUnitCount() >= enemies)
					return DCO_ESOPAction.ENGAGE;
				if (g.GetGroupObjective() && g.GetMyCommander().IsAssaultReleased(g.GetGroupObjective()) && DCO_Ctx.TuneFor(g.GetOwner(), "sop.attack_released_engage", 1.0) > 0)
				{
					DCO_Perf.Count("sop:attack_released_engage");
					return DCO_ESOPAction.ENGAGE;
				}
				return DCO_ESOPAction.OBSERVE;
			}
			case DCO_EGroupTask.SUPPORT_BY_FIRE:
			case DCO_EGroupTask.REINFORCE:
			{
				if (moving && dist >= 75.0 && !onAxis)
					return DCO_ESOPAction.BYPASS;
				return DCO_ESOPAction.ENGAGE;
			}
			case DCO_EGroupTask.DEFEND:
			case DCO_EGroupTask.GARRISON:
			{
				if (DCO_Radio.PersonalityOf(g.GetMyCommander()) == 2)
					return DCO_ESOPAction.AMBUSH;
				return DCO_ESOPAction.DYNAMIC;
			}
			case DCO_EGroupTask.TRANSPORT:
				return DCO_ESOPAction.BYPASS;
			case DCO_EGroupTask.FIRE_MISSION:
			case DCO_EGroupTask.NONE:
			{
				if (dist < engageM)
					return DCO_ESOPAction.ENGAGE;
				return DCO_ESOPAction.OBSERVE;
			}
		}
		return DCO_ESOPAction.DYNAMIC;
	}

	static void Update(SCR_AIGroupUtilityComponent u, DCO_GroupUtilityComponent g, bool threatened)
	{
		bool noOrder = !g.GetMyCommander() && g.GetSOPOverride() == DCO_ESOPAction.DYNAMIC && g.GetTask() == DCO_EGroupTask.NONE;
		if (noOrder || g.IsPlayerGroup() || g.IsBreakingContact() || g.m_DCOAmbush || !DCO_GlobalAIComponent.BehaviorOnFor(g.GetOwner(), DCO_EBehavior.CONTACT_SOP))
		{
			g.m_eSOPAction = DCO_ESOPAction.DYNAMIC;
			return;
		}

		SCR_AIGroup grp = SCR_AIGroup.Cast(g.GetOwner());
		PerceptionManager pm = GetGame().GetPerceptionManager();
		if (!grp || !grp.GetLeaderEntity() || !pm || !u.m_Perception)
			return;
		float pmNow = pm.GetTime();
		vector lp = grp.GetLeaderEntity().GetOrigin();
		vector sum;
		int n;
		float nearest = float.MAX;
		foreach (SCR_AITargetInfo t : u.m_Perception.m_aTargets)
		{
			if (!t || pmNow - t.m_fTimestamp > 10.0)
				continue;
			if (t.m_eCategory != EAITargetInfoCategory.DETECTED && t.m_eCategory != EAITargetInfoCategory.IDENTIFIED)
				continue;
			sum = sum + t.m_vWorldPos;
			n++;
			nearest = Math.Min(nearest, vector.DistanceXZ(lp, t.m_vWorldPos));
		}

		DCO_ESOPAction prev = g.m_eSOPAction;
		if (n == 0)
		{
			g.m_eSOPAction = DCO_ESOPAction.DYNAMIC;
			g.m_bSOPSprung = false;
			g.m_bSOPReturnFire = false;
			return;
		}
		vector enemy = sum * (1.0 / n);
		bool combatTask = IsCombatTask(g.GetTask());
		bool onAxis = combatTask && OnObjectiveAxis(g, lp, enemy);
		float engageM = DCO_Ctx.TuneFor(grp, "sop.engage_m", 150.0) * DCO_Stress.ReachFor(grp);
		if (combatTask && !g.m_bSOPReturnFire && (threatened || MemberThreatened(grp)))
		{
			g.m_bSOPReturnFire = true;
			DCO_Perf.Count("sop:staging_return_fire");
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("contact_sop_return_fire grp=%1 task=%2 phase=%3", DCO_Log.IdGroup(grp), typename.EnumToString(DCO_EGroupTask, g.GetTask()), typename.EnumToString(DCO_ETaskPhase, g.GetPhase())));
		}

		if (threatened)
		{
			if (prev == DCO_ESOPAction.AMBUSH && !g.m_bSOPSprung)
				Spring(g, "detected");
			else if (prev == DCO_ESOPAction.OBSERVE && g.GetTask() == DCO_EGroupTask.RECON)
				DCO_BreakContact.Start(g, enemy, vector.Zero, "sop_recon_detected");
			if (!combatTask)
				g.m_fSOPShotAt = DCO_Util.NowS();
			if (g.m_bSOPReturnFire)
				g.m_eSOPAction = DCO_ESOPAction.ENGAGE;
			else if (prev != DCO_ESOPAction.AMBUSH)
				g.m_eSOPAction = DCO_ESOPAction.DYNAMIC;
			return;
		}

		DCO_ESOPAction action = g.GetSOPOverride();
		bool continueFar = false;
		bool picked = action == DCO_ESOPAction.DYNAMIC;
		if (picked)
		{
			action = Pick(g, nearest, n, onAxis, engageM);
			if (action == DCO_ESOPAction.OBSERVE && StagingFar(g, nearest))
			{
				action = DCO_ESOPAction.DYNAMIC;
				continueFar = true;
				DCO_Perf.Count("sop:continue_staging_far");
			}
		}
		if (combatTask && (g.m_bSOPReturnFire || (onAxis && action == DCO_ESOPAction.BYPASS)))
			action = DCO_ESOPAction.ENGAGE;
		if (action == DCO_ESOPAction.BYPASS || action == DCO_ESOPAction.OBSERVE)
		{
			string planWhy = PlanRole(g);
			if (!planWhy.IsEmpty())
			{
				if (prev != DCO_ESOPAction.ENGAGE)
				{
					DCO_Perf.Count("sop:override_" + planWhy);
					DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("contact_sop_override grp=%1 from=%2 to=ENGAGE_MOVE reason=%3 dist=%4 enemies=%5", DCO_Log.IdGroup(grp), typename.EnumToString(DCO_ESOPAction, action), planWhy, Math.Round(nearest), n));
				}
				action = DCO_ESOPAction.ENGAGE;
			}
		}
		float sopNow = DCO_Util.NowS();
		if (picked && action == DCO_ESOPAction.OBSERVE)
		{
			string freeWhy = ObserveOff(g, n, sopNow);
			if (!freeWhy.IsEmpty())
			{
				action = DCO_ESOPAction.DYNAMIC;
				DCO_Perf.Count("sop:" + freeWhy);
				if (sopNow - g.m_fSOPFreeLogAt > 60.0)
				{
					g.m_fSOPFreeLogAt = sopNow;
					DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("contact_sop_free grp=%1 task=%2 why=%3 dist=%4 enemies=%5 units=%6", DCO_Log.IdGroup(grp), typename.EnumToString(DCO_EGroupTask, g.GetTask()), freeWhy, Math.Round(nearest), n, g.GetUnitCount()));
				}
			}
		}
		if (action == DCO_ESOPAction.ENGAGE && prev != DCO_ESOPAction.ENGAGE)
			g.m_fSOPEngageAt = sopNow;
		else if (combatTask && prev == DCO_ESOPAction.ENGAGE && action == DCO_ESOPAction.OBSERVE && sopNow - g.m_fSOPEngageAt < DCO_Ctx.TuneFor(grp, "sop.engage_hold_s", 30.0))
		{
			action = DCO_ESOPAction.ENGAGE;
			DCO_Perf.Count("sop:engage_hold");
		}
		if (prev == DCO_ESOPAction.AMBUSH && action != DCO_ESOPAction.ENGAGE)
			action = DCO_ESOPAction.AMBUSH;
		g.m_eSOPAction = action;

		if (action == DCO_ESOPAction.AMBUSH && !g.m_bSOPSprung && nearest < 60.0)
			Spring(g, "killzone");

		if (g.GetTask() == DCO_EGroupTask.RECON && (action == DCO_ESOPAction.OBSERVE || (action == DCO_ESOPAction.AMBUSH && !g.m_bSOPSprung)))
			ReconObserve(g, grp, enemy, nearest, n);

		if (action == prev)
			return;

		string actName = typename.EnumToString(DCO_ESOPAction, action);
		string actWhy;
		if (continueFar)
		{
			actName = "CONTINUE";
			actWhy = " reason=staging_far";
		}
		DCO_Perf.Count("sop:" + actName);
		if (onAxis && action == DCO_ESOPAction.ENGAGE && g.GetPhase() == DCO_ETaskPhase.MOVING)
			DCO_Perf.Count("sop:no_bypass_obj");
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("contact_sop grp=%1 action=%2 task=%3 phase=%4 dist=%5 enemies=%6", DCO_Log.IdGroup(grp), actName,
			typename.EnumToString(DCO_EGroupTask, g.GetTask()), typename.EnumToString(DCO_ETaskPhase, g.GetPhase()), Math.Round(nearest), n) + actWhy + string.Format(" engage_m=%1 commander=%2 gm_order=%3", Math.Round(engageM), DCO_Util.B01(g.GetMyCommander() != null), typename.EnumToString(DCO_ESOPAction, g.GetSOPOverride())));

		switch (action)
		{
			case DCO_ESOPAction.AMBUSH:
			{
				g.m_bSOPSprung = false;
				array<AIAgent> agents = {};
				grp.GetAgents(agents);
				foreach (AIAgent a : agents)
				{
					SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
					if (ca && ca.m_UtilityComponent && DCO_Util.IsAlive(ca.GetControlledEntity()))
						DCO_Specialist.CombatMove(ca.m_UtilityComponent, ca.GetControlledEntity().GetOrigin(), enemy, true, ECharacterStance.PRONE, 20.0);
				}
				break;
			}
			case DCO_ESOPAction.BYPASS:
			{
				Detour(g, grp, lp, enemy);
				break;
			}
			case DCO_ESOPAction.WITHDRAW:
			{
				DCO_BreakContact.Start(g, enemy, vector.Zero, "sop_withdraw");
				break;
			}
		}
	}

	protected static void ReconObserve(DCO_GroupUtilityComponent g, SCR_AIGroup grp, vector enemy, float nearest, int n)
	{
		float now = DCO_Util.NowS();
		if (g.m_fSOPObserveSince < 0 || now - g.m_fSOPObserveSeen > 120.0)
			g.m_fSOPObserveSince = now;
		g.m_fSOPObserveSeen = now;
		float limit = DCO_Ctx.TuneFor(grp, "sop.recon_observe_s", 180.0);
		AICommander_BaseComponent cmd = g.GetMyCommander();
		float held = now - g.m_fSOPObserveSince;
		if (limit <= 0 || !cmd || held < limit || g.IsInTransport())
			return;
		g.m_fSOPObserveSince = now;
		bool ok = cmd.DCO_ReconRelocate(g, enemy);
		DCO_Perf.Count("sop:recon_relocate");
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("contact_sop_relocate grp=%1 held_s=%2 dist=%3 enemies=%4 ok=%5 limit_s=%6", DCO_Log.IdGroup(grp), Math.Round(held), Math.Round(nearest), n, DCO_Util.B01(ok), Math.Round(limit)));
	}

	protected static string PlanRole(DCO_GroupUtilityComponent g)
	{
		AICommander_BaseComponent cmd = g.GetMyCommander();
		if (!cmd)
			return "";
		if (cmd.DCO_IsScopeFix(g))
			return "scope_fix";
		if (DCO_TacticsCompat.OwnsGroup(cmd.GetTactics(), g))
			return "plan_role";
		return "";
	}

	protected static string ObserveOff(DCO_GroupUtilityComponent g, int enemies, float now)
	{
		DCO_EGroupTask t = g.GetTask();
		if (t != DCO_EGroupTask.NONE && t != DCO_EGroupTask.FIRE_MISSION)
			return "";
		if (now - g.m_fSOPShotAt < DCO_Ctx.TuneFor(g.GetOwner(), "sop.observe_latch_s", 120.0))
			return "observe_latch";
		if (t == DCO_EGroupTask.NONE && enemies <= g.GetUnitCount() && DCO_Ctx.TuneFor(g.GetOwner(), "sop.none_fight", 1.0) > 0)
			return "none_fight";
		return "";
	}

	protected static bool StagingFar(DCO_GroupUtilityComponent g, float nearest)
	{
		if (nearest <= 300.0)
			return false;
		DCO_EGroupTask t = g.GetTask();
		if (t != DCO_EGroupTask.ATTACK && t != DCO_EGroupTask.FLANK)
			return false;
		CMD_AICommanderObjectiveComponent obj = g.GetGroupObjective();
		AICommander_BaseComponent cmd = g.GetMyCommander();
		return !obj || !cmd || !cmd.IsAssaultReleased(obj);
	}

	protected static bool MemberThreatened(SCR_AIGroup grp)
	{
		array<AIAgent> agents = {};
		grp.GetAgents(agents);
		foreach (AIAgent a : agents)
		{
			SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
			if (!ca || !ca.m_UtilityComponent || !ca.m_UtilityComponent.m_ThreatSystem)
				continue;
			if (ca.m_UtilityComponent.m_ThreatSystem.GetState() == EAIThreatState.THREATENED)
				return true;
			if (ca.m_UtilityComponent.m_ThreatSystem.GetSuppressionMeasure() >= DCO_Stress.T(ca.m_UtilityComponent, "tm.supp_s1", 0.6))
			{
				DCO_Perf.Count("sop:return_fire_suppressed");
				return true;
			}
		}
		return false;
	}

	protected static void Spring(DCO_GroupUtilityComponent g, string why)
	{
		g.m_bSOPSprung = true;
		DCO_Perf.Count("sop:sprung");
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("contact_sop_sprung grp=%1 why=%2", DCO_Log.IdGroup(AIGroup.Cast(g.GetOwner())), why));
	}

	protected static void Detour(DCO_GroupUtilityComponent g, SCR_AIGroup grp, vector lp, vector enemy)
	{
		float now = DCO_Util.NowS();
		if (now - g.m_fSOPDetourAt < 120.0 || g.IsInTransport() || g.GetTask() == DCO_EGroupTask.TRANSPORT)
			return;
		AIWaypoint wp = grp.GetCurrentWaypoint();
		if (!wp)
			return;
		AICommander_BaseComponent cmd = g.GetMyCommander();
		if (!cmd)
		{
			g.m_fSOPDetourAt = now;
			DCO_Perf.Count("sop:bypass_no_detour");
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("contact_sop_bypass grp=%1 detour=0 reason=no_commander", DCO_Log.IdGroup(grp)));
			return;
		}

		vector route = DCO_Specialist.Flat(wp.GetOrigin() - lp);
		vector side = Vector(route[2], 0, -route[0]);
		vector toEnemy = enemy - lp;
		if (side[0] * toEnemy[0] + side[2] * toEnemy[2] > 0)
			side = side * -1.0;
		vector pos = DCO_Util.Surface(lp + route * 100.0 + side * 150.0);
		SCR_AIWaypoint detour = cmd.SpawnMoveWP(pos);
		if (!detour)
			return;
		grp.AddWaypointAt(detour, 0);
		g.m_fSOPDetourAt = now;
		DCO_Perf.Count("sop:detour");
	}
}
