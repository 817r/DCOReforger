modded class SCR_AITargetClusterState
{
	int m_iDCOInvestigateRoll = -1;
	bool m_bDCOEngaged;
	bool m_bDCOBlind;
	vector m_vDCOTrackPos;
	float m_fDCOTrackTs = -1;
	vector m_vDCOVel;
	vector m_vDCOPursuit;
	ref DCO_StrengthInfo m_DCOStrength = new DCO_StrengthInfo();
	float m_fDCOStrengthAt_ms = -1;
	float m_fDCOErrAng = -1;
	int m_iDCOInfoEvents;
	float m_fDCOPrevInfoAge = -1;
	int m_iDCOLogTo = -1;
	float m_fDCOLogAt_ms;
}

modded class SCR_AIGroupTargetClusterProcessor
{
	protected ref map<SCR_AITargetClusterState, float> m_mDCOAttackSince = new map<SCR_AITargetClusterState, float>();

	protected string m_sDCOStateWhy;

	override EAITargetClusterState EvaluateNewDesiredState(SCR_AITargetClusterState s)
	{
		m_sDCOStateWhy = "vanilla";
		EAITargetClusterState want = DCO_DesiredState(s);
		if (want != s.m_eState)
			DCO_StateLog(s, want);
		return want;
	}

	protected void DCO_StateLog(SCR_AITargetClusterState s, EAITargetClusterState want)
	{
		float now = GetGame().GetWorld().GetWorldTime();
		if (s.m_iDCOLogTo == want && now < s.m_fDCOLogAt_ms)
			return;
		s.m_iDCOLogTo = want;
		s.m_fDCOLogAt_ms = now + 30000.0;
		string to = typename.EnumToString(EAITargetClusterState, want);
		DCO_Perf.Count("cluster:want_" + to + "_" + m_sDCOStateWhy);
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(m_Utility.m_Owner), string.Format("cluster_state grp=%1 from=%2 to=%3 why=%4 dist=%5 identified=%6 endangering=%7 alive=%8 info_age_s=%9", DCO_Log.IdGroup(m_Utility.m_Owner), typename.EnumToString(EAITargetClusterState, s.m_eState), to, m_sDCOStateWhy, Math.Round(s.m_fDistMin), s.m_iCountIdentified, s.m_iCountEndangering, s.m_iCountAlive, Math.Round(s.GetTimeSinceLastNewInformation())));
	}

	protected EAITargetClusterState DCO_DesiredState(SCR_AITargetClusterState s)
	{
		EAITargetClusterState desired = super.EvaluateNewDesiredState(s);
		DCO_Track(s);
		desired = DCO_BlindCheck(s, desired);
		float dwellNow = GetGame().GetWorld().GetWorldTime();
		if (m_mDCOAttackSince.Count() > 64)
		{
			for (int pi = m_mDCOAttackSince.Count() - 1; pi >= 0; pi--)
			{
				if (!m_mDCOAttackSince.GetKey(pi))
					m_mDCOAttackSince.RemoveElement(pi);
			}
		}
		if (desired == EAITargetClusterState.ATTACKING && s.m_eState != EAITargetClusterState.ATTACKING)
			m_mDCOAttackSince.Set(s, dwellNow);
		if (desired == EAITargetClusterState.INVESTIGATING && s.m_eState == EAITargetClusterState.ATTACKING)
		{
			float attackSince;
			if (m_mDCOAttackSince.Find(s, attackSince) && dwellNow - attackSince < 10000.0)
			{
				DCO_Perf.Count("cluster:attack_dwell");
				m_sDCOStateWhy = "attack_dwell";
				desired = EAITargetClusterState.ATTACKING;
			}
		}
		if (s.m_eState == EAITargetClusterState.NONE && s.m_fTimer_ms < 5000.0 && DCO_Relentless.Group(m_Utility.m_Owner))
		{
			DCO_Perf.Count("relentless:decide_now");
			s.m_fTimer_ms = 5001.0;
		}
		if (DCO_StaleForAssault(s, desired) || DCO_DriveDrop(s, desired))
			return EAITargetClusterState.NONE;
		if (desired == EAITargetClusterState.ATTACKING || desired == EAITargetClusterState.DEFENDING)
		{
			s.m_bDCOEngaged = true;
			s.m_vDCOPursuit = vector.Zero;
			if (s.m_iDCOInvestigateRoll == 2)
				s.m_iDCOInvestigateRoll = -1;
		}
		if (desired != EAITargetClusterState.INVESTIGATING && desired != EAITargetClusterState.ATTACKING)
			return desired;

		if (DCO_DefenderLeash(s, desired))
			return EAITargetClusterState.NONE;

		bool superior = m_Utility.DCO_IsSuperiorTo(s);
		int garrisonMode = m_Utility.DCO_GetGarrisonMode();
		if (garrisonMode >= 0)
		{
			if (superior && desired == EAITargetClusterState.ATTACKING && garrisonMode != DCO_EGarrisonMode.HOLD)
				return desired;
			m_sDCOStateWhy = "garrison";
			return EAITargetClusterState.NONE;
		}

		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetFor(m_Utility.m_Owner);
		if (!cfg)
			return desired;

		bool idle = m_Utility.DCO_IsIdle();
		bool relentless = DCO_Relentless.Group(m_Utility.m_Owner);
		if (relentless && idle)
		{
			DCO_Perf.Count("relentless:idle_bypass");
			idle = false;
		}

		if (desired == EAITargetClusterState.INVESTIGATING)
		{
			if (DCO_ArmorOnlyNoAT(s))
				return EAITargetClusterState.NONE;
			bool aware = DCO_GlobalAIComponent.BehaviorOnFor(m_Utility.m_Owner, DCO_EBehavior.AWARENESS);
			float range = 150.0;
			if (aware)
				range = DCO_Awareness.PursuitRange(m_Utility.m_Owner);
			if (relentless)
				range = 400.0;
			if (s.m_bDCOEngaged && s.m_fDistMin <= range && DCO_CanPursue(relentless))
			{
				if (s.m_iDCOInvestigateRoll != 2)
				{
					s.m_iDCOInvestigateRoll = 2;
					if (aware)
						DCO_PlanPursuit(s, range);
					DCO_Perf.Count("pursue_lost");
					m_sDCOStateWhy = "pursue";
					DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("pursue_lost grp=%1 dist=%2 range=%3 speed=%4", m_Utility.m_Owner.GetName(), Math.Round(s.m_fDistMin), range, s.m_vDCOVel.Length().ToString(1, 1)));
				}
				return desired;
			}
			if (aware && m_Utility.m_fDCOBodySweep_ms > GetGame().GetWorld().GetWorldTime() && s.m_fDistMin <= 150.0)
				return desired;
			if (idle && !cfg.GetIdleLeaveToInvestigate())
			{
				float joinM = DCO_Ctx.TuneForGU(m_Utility, "idle.join_fight_m", 250.0);
				if (joinM <= 0 || s.m_fDistMin > joinM)
				{
					m_sDCOStateWhy = "idle_stay";
					return EAITargetClusterState.NONE;
				}
				DCO_Perf.Count("idle:join_fight");
				m_sDCOStateWhy = "idle_join_fight";
			}

			float maxDist = cfg.GetInvestigateMaxDist();
			float chance = cfg.GetInvestigateChance();
			DCO_GroupConfigComponent groupCfg = DCO_GroupConfigComponent.Cast(m_Utility.m_Owner.FindComponent(DCO_GroupConfigComponent));
			if (groupCfg)
			{
				maxDist = groupCfg.GetInvestigateMaxDist();
				chance = groupCfg.GetInvestigateChance();
			}

			if (s.m_fDistMin > maxDist)
			{
				m_sDCOStateWhy = "too_far";
				return EAITargetClusterState.NONE;
			}

			if (s.m_iDCOInvestigateRoll < 0)
				s.m_iDCOInvestigateRoll = Math.RandomFloat01() < chance;

			if (s.m_iDCOInvestigateRoll == 0)
			{
				m_sDCOStateWhy = "chance_roll";
				return EAITargetClusterState.NONE;
			}

			return desired;
		}

		if (!idle || s.m_fDistMin <= cfg.GetIdleOverrunDist() || superior)
			return desired;

		if (s.m_iCountEndangering > 0 || !cfg.GetIdleLeaveToAssist())
		{
			m_sDCOStateWhy = "idle_no_assist";
			return DCO_AirDefend(s, "idle");
		}

		return desired;
	}

	protected EAITargetClusterState DCO_AirDefend(SCR_AITargetClusterState s, string why)
	{
		if (!DCO_Air.On() || DCO_Air.ClusterAirDist(s, 10.0) >= DCO_Air.OpenRange())
			return EAITargetClusterState.NONE;
		DCO_Air.Note("cluster_defend_" + why, DCO_Log.IdGroup(m_Utility.m_Owner), s.m_fDistMin);
		return EAITargetClusterState.DEFENDING;
	}

	protected float m_fDCOLeashLogAt;

	protected bool DCO_DefenderLeash(SCR_AITargetClusterState s, EAITargetClusterState desired)
	{
		if (DCO_Ctx.TuneForGU(m_Utility, "def.engage_leash_on", 1.0) <= 0)
			return false;
		DCO_GroupUtilityComponent g = m_Utility.DCO_GetUtil();
		if (!g || !g.GetGroupObjective() || g.IsPlayerGroup())
			return false;
		DCO_EGroupTask t = g.GetTask();
		if (t != DCO_EGroupTask.DEFEND && t != DCO_EGroupTask.GARRISON)
			return false;
		int role = g.m_iDCODefRole;
		if (role == DCO_EDefRole.COUNTER || role == DCO_EDefRole.SPOIL_ABF || role == DCO_EDefRole.SPOIL_RAID)
			return false;
		CMD_AICommanderObjectiveComponent obj = g.GetGroupObjective();
		float limit = obj.GetRadius() + DCO_Ctx.TuneForGU(m_Utility, "def.engage_leash", 30.0);
		float d = vector.DistanceXZ(s.GetCenterPosition(), obj.GetOwner().GetOrigin());
		if (d <= limit)
			return false;
		DCO_Perf.Count("def:cluster_hold");
		m_sDCOStateWhy = "defender_leash";
		float now = GetGame().GetWorld().GetWorldTime();
		if (now >= m_fDCOLeashLogAt)
		{
			m_fDCOLeashLogAt = now + 30000.0;
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(m_Utility.m_Owner), string.Format("def_cluster_hold grp=%1 state=%2 cluster_obj_m=%3 limit_m=%4 grp_dist=%5", DCO_Log.IdGroup(m_Utility.m_Owner), typename.EnumToString(EAITargetClusterState, desired), Math.Round(d), Math.Round(limit), Math.Round(s.m_fDistMin)));
		}
		return true;
	}

	protected float m_fDCOArmorSkipLogAt;

	protected bool DCO_ArmorOnlyNoAT(SCR_AITargetClusterState s)
	{
		if (!s.m_Cluster)
			return false;
		DCO_GroupUtilityComponent g = m_Utility.DCO_GetUtil();
		if (!g || g.IsPlayerGroup() || g.HasAT() || g.IsArmor())
			return false;
		int armored;
		int other;
		foreach (SCR_AITargetInfo t : s.m_Cluster.m_aTargets)
		{
			if (!t)
				continue;
			EAITargetInfoCategory cat = t.m_eCategory;
			if (cat != EAITargetInfoCategory.DETECTED && cat != EAITargetInfoCategory.IDENTIFIED && cat != EAITargetInfoCategory.LOST)
				continue;
			IEntity ent = t.m_Entity;
			IEntity veh;
			if (ent && Vehicle.Cast(ent))
				veh = ent;
			else if (ent)
				veh = DCO_VehicleCombat.GetVehicle(ent);
			if (veh && DCO_VehicleCombat.IsArmored(veh))
				armored++;
			else
				other++;
		}
		if (armored == 0 || other > 0)
			return false;
		DCO_Perf.Count("investigate_skip:armor_no_at");
		m_sDCOStateWhy = "armor_no_at";
		float now = GetGame().GetWorld().GetWorldTime();
		if (now >= m_fDCOArmorSkipLogAt)
		{
			m_fDCOArmorSkipLogAt = now + 30000.0;
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(m_Utility.m_Owner), string.Format("investigate_skip grp=%1 why=armor_no_at armored=%2 dist=%3", DCO_Log.IdGroup(m_Utility.m_Owner), armored, Math.Round(s.m_fDistMin)));
		}
		return true;
	}

	protected bool DCO_DriveDrop(SCR_AITargetClusterState s, EAITargetClusterState desired)
	{
		if (desired != EAITargetClusterState.INVESTIGATING && desired != EAITargetClusterState.ATTACKING)
			return false;
		DCO_GroupUtilityComponent g = m_Utility.DCO_GetUtil();
		if (!DCO_AssaultDrive.Active(g))
			return false;
		if (s.m_fDistMin < DCO_Ctx.TuneForGU(m_Utility, "tactic.drive_close_m", 60.0))
		{
			DCO_Perf.Count("assault:drive_close_fight");
			return false;
		}
		DCO_Perf.Count("assault:drive_drop");
		m_sDCOStateWhy = "assault_drive";
		if (DCO_Log.On(DCO_ELogChannel.SQUAD, DCO_ELogLevel.DEBUG))
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.DEBUG, DCO_Log.IdGroup(m_Utility.m_Owner), string.Format("assault_drive_drop grp=%1 state=%2 dist=%3 info_age_s=%4", DCO_Log.IdGroup(m_Utility.m_Owner), typename.EnumToString(EAITargetClusterState, desired), Math.Round(s.m_fDistMin), Math.Round(s.GetTimeSinceLastNewInformation())));
		return true;
	}

	protected bool DCO_StaleForAssault(SCR_AITargetClusterState s, EAITargetClusterState desired)
	{
		if (desired != EAITargetClusterState.INVESTIGATING && desired != EAITargetClusterState.ATTACKING)
			return false;
		if (s.m_iCountEndangering > 0 || s.GetTimeSinceLastNewInformation() < 45.0)
			return false;
		DCO_GroupUtilityComponent g = m_Utility.DCO_GetUtil();
		if (!g || !g.GetGroupObjective() || g.IsPlayerGroup())
			return false;
		DCO_EGroupTask t = g.GetTask();
		if (t != DCO_EGroupTask.ATTACK && t != DCO_EGroupTask.FLANK)
			return false;
		DCO_Perf.Count("assault:stale_cluster_drop");
		m_sDCOStateWhy = "stale_assault";
		if (DCO_Log.On(DCO_ELogChannel.SQUAD, DCO_ELogLevel.DEBUG))
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.DEBUG, DCO_Log.IdGroup(m_Utility.m_Owner), string.Format("assault_stale_cluster grp=%1 state=%2 info_age_s=%3 dist=%4", DCO_Log.IdGroup(m_Utility.m_Owner), typename.EnumToString(EAITargetClusterState, desired), Math.Round(s.GetTimeSinceLastNewInformation()), Math.Round(s.m_fDistMin)));
		return true;
	}

	protected EAITargetClusterState DCO_BlindCheck(SCR_AITargetClusterState s, EAITargetClusterState desired)
	{
		if (desired != EAITargetClusterState.ATTACKING || !s.m_bDCOEngaged || s.m_iCountEndangering > 0 || m_Utility.m_Owner.IsSlave())
		{
			if (s.m_bDCOBlind && desired == EAITargetClusterState.ATTACKING)
			{
				s.m_bDCOBlind = false;
				DCO_Perf.Count("lostgap:blind_back");
			}
			return desired;
		}
		float blind = DCO_LostGap.Blind(m_Utility.m_Owner);
		if (blind < 8.0)
		{
			if (s.m_bDCOBlind)
			{
				s.m_bDCOBlind = false;
				DCO_Perf.Count("lostgap:blind_back");
				if (DCO_Log.On(DCO_ELogChannel.SQUAD, DCO_ELogLevel.DEBUG))
					DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.DEBUG, DCO_Log.IdGroup(m_Utility.m_Owner), string.Format("blind_back grp=%1 seen_s=%2", DCO_Log.IdGroup(m_Utility.m_Owner), blind.ToString(1, 1)));
			}
			return desired;
		}
		if (!s.m_bDCOBlind)
		{
			s.m_bDCOBlind = true;
			DCO_Perf.Count("lostgap:blind_switch");
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(m_Utility.m_Owner), string.Format("blind_investigate grp=%1 blind_s=%2 identified=%3 detected=%4 lost=%5 alive=%6 dist=%7 info_age_s=%8",
				DCO_Log.IdGroup(m_Utility.m_Owner), Math.Round(blind), s.m_iCountIdentified, s.m_iCountDetected, s.m_iCountLost, s.m_iCountAlive, Math.Round(s.m_fDistMin), Math.Round(s.GetTimeSinceLastNewInformation())));
		}
		m_sDCOStateWhy = "blind";
		return EAITargetClusterState.INVESTIGATING;
	}

	protected void DCO_Track(SCR_AITargetClusterState s)
	{
		if (s.m_iCountDetected + s.m_iCountIdentified <= 0)
			return;
		float ts = s.m_fMaxTimestamp;
		vector c = s.GetCenterPosition();
		if (s.m_fDCOTrackTs < 0)
		{
			s.m_vDCOTrackPos = c;
			s.m_fDCOTrackTs = ts;
			return;
		}
		float dt = ts - s.m_fDCOTrackTs;
		if (dt < 2.0)
			return;
		vector v = (c - s.m_vDCOTrackPos) * (1.0 / dt);
		v[1] = 0;
		if (dt > 20.0)
			v = vector.Zero;
		s.m_vDCOVel = s.m_vDCOVel * 0.5 + v * 0.5;
		s.m_vDCOTrackPos = c;
		s.m_fDCOTrackTs = ts;
	}

	protected void DCO_PlanPursuit(SCR_AITargetClusterState s, float range)
	{
		IEntity leader = m_Utility.m_Owner.GetLeaderEntity();
		if (!leader)
			return;
		vector c = s.m_vDCOTrackPos;
		if (c == vector.Zero)
			c = s.GetCenterPosition();
		vector v = s.m_vDCOVel;
		float spd = v.Length();
		vector goal = c;
		vector dir;
		if (spd > 0.5)
		{
			dir = v * (1.0 / spd);
			goal = c + dir * Math.Min(spd * s.GetTimeSinceLastNewInformation(), range * 0.6);
		}
		vector lp = leader.GetOrigin();
		vector toGoal = goal - lp;
		toGoal[1] = 0;
		if (toGoal.Length() > range)
			goal = lp + toGoal.Normalized() * range;
		vector hide = DCO_Awareness.HidingSpot(goal, 40.0);
		if (hide != vector.Zero)
			goal = hide;
		s.m_vDCOPursuit = DCO_Util.Surface(goal);

		DCO_GroupUtilityComponent g = m_Utility.DCO_GetUtil();
		if (spd > 1.0 && g)
		{
			AICommander_BaseComponent cmd = g.GetMyCommander();
			if (cmd && cmd.GetDefense())
				cmd.GetDefense().Intercept(cmd, s.m_vDCOPursuit, dir, g);
		}
	}

	protected bool DCO_CanPursue(bool relentless)
	{
		DCO_GroupUtilityComponent g = m_Utility.DCO_GetUtil();
		if (!g)
			return true;
		if (g.IsPlayerGroup() || g.IsInTransport() || g.IsMortar() || g.IsArmor() || g.IsDedicatedTransport())
			return false;
		DCO_EGroupTask t = g.GetTask();
		if (t == DCO_EGroupTask.ATTACK || t == DCO_EGroupTask.FLANK || t == DCO_EGroupTask.TRANSPORT)
			return false;
		SCR_AIGroup grp = SCR_AIGroup.Cast(g.GetOwner());
		int peak = Math.Max(g.DCO_GetPeakStrength(), 1);
		bool heavyLoss = g.GetUnitCount() < 1.0 * peak * 0.7;
		bool lowAmmo = grp && DCO_Logistics.IsLowAmmo(grp, 3);
		if (heavyLoss && relentless)
		{
			DCO_Perf.Count("relentless:pursue_despite_losses");
			heavyLoss = false;
		}
		if (heavyLoss || lowAmmo)
		{
			DCO_Perf.Count("pursue_skip:lace");
			return false;
		}
		return true;
	}

	override SCR_AIActivityBase TryCreateActivityForState(SCR_AITargetClusterState s, EAITargetClusterState estate, notnull TFireteamLockRefArray inFtsMain, notnull TFireteamLockRefArray inFtsAux)
	{
		string ctrlSrc;
		if (DCO_CtrlLock.GroupOwned(m_Utility.m_Owner, ctrlSrc))
		{
			DCO_Perf.Count("ctrl:group_attack_block");
			if (DCO_Log.On(DCO_ELogChannel.SQUAD, DCO_ELogLevel.DEBUG))
				DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.DEBUG, DCO_Log.IdGroup(m_Utility.m_Owner), string.Format("ctrl_group_block grp=%1 activity=cluster_%2 members=%3 src=%4", DCO_Log.IdGroup(m_Utility.m_Owner), typename.EnumToString(EAITargetClusterState, estate), m_Utility.m_Owner.GetAgentsCount(), ctrlSrc));
			return null;
		}
		TFireteamLockRefArray ftsMain;
		TFireteamLockRefArray ftsAux;

		array<SCR_AIGroupFireteam> newFireteams = {};
		switch (estate)
		{
			case EAITargetClusterState.INVESTIGATING:
			{
				if (m_Utility.DCO_GetPosture() == DCO_GroupTactics.EVASIVE)
					return null;

				if (!inFtsMain.IsEmpty())
					ftsMain = SCR_AIGroupFireteamLock.CopyLockArray(inFtsMain);
				else
					ftsMain = {};

				if (!inFtsAux.IsEmpty())
					ftsAux = SCR_AIGroupFireteamLock.CopyLockArray(inFtsAux);
				else
					ftsAux = {};

				AllocateFTForInvestigate(s, ftsMain, ftsAux);

				if (ftsMain.IsEmpty())
					return null;

				DCO_SplitInvestigateFireteams(ftsMain, ftsAux);
				break;
			}
			case EAITargetClusterState.ATTACKING:
			{
				if (!inFtsMain.IsEmpty())
					ftsMain = SCR_AIGroupFireteamLock.CopyLockArray(inFtsMain);
				else
					ftsMain = {};

				if (!inFtsAux.IsEmpty())
					ftsAux = SCR_AIGroupFireteamLock.CopyLockArray(inFtsAux);
				else
					ftsAux = {};

				AllocateMoreFireteams(s, ftsMain, ftsAux);

				if (ftsMain.IsEmpty())
					return null;

				if (ftsAux.IsEmpty())
				{
					if (ftsMain.Count() > 1)
					{
						SCR_AIGroupFireteamLock ftLock = ftsMain[ftsMain.Count()-1];
						ftsMain.Remove(ftsMain.Count()-1);
						ftsAux.Insert(ftLock);
					}
					else if (m_Utility.m_FireteamMgr.FindFreeFireteams(newFireteams, 1, SCR_AIGroupFireteam))
					{
						SCR_AIGroupFireteamLock.TryLockFireteams(newFireteams, ftsAux, true);
					}
				}
				else
				{
					int totalFireteams = ftsMain.Count() + ftsAux.Count();
					int maxAuxFireteams = Math.Max(1, totalFireteams / 3);

					while (ftsAux.Count() > maxAuxFireteams)
					{
						SCR_AIGroupFireteamLock ftLock = ftsAux[ftsAux.Count()-1];
						ftsMain.Insert(ftLock);
						ftsAux.Remove(ftsAux.Count()-1);
					}
				}

				break;
			}

			case EAITargetClusterState.DEFENDING:
			{
				if (!inFtsMain.IsEmpty() || !inFtsAux.IsEmpty())
				{
					ftsMain = {};
					foreach (auto ft : inFtsMain)
						ftsMain.Insert(ft);
					foreach (auto ft : inFtsAux)
						ftsAux.Insert(ft);

					ftsAux = {};
				}
				else if (m_Utility.m_FireteamMgr.FindFreeFireteams(newFireteams, 1, SCR_AIGroupFireteam))
				{
					ftsMain = {};
					SCR_AIGroupFireteamLock.TryLockFireteams(newFireteams, ftsMain, true);
					ftsAux = {};
				}
				else
				{
					return null;
				}

				break;
			}

			default:
			{
				return null;
			}
		}

		SCR_AIActivityBase activity = null;
		switch (estate)
		{
			case EAITargetClusterState.INVESTIGATING:
			{
				activity = new SCR_AIInvestigateClusterActivity(m_Utility, null, s, ftsMain, ftsAux);
				break;
			}
			case EAITargetClusterState.ATTACKING:
			{
				activity = new SCR_AIAttackClusterActivity(m_Utility, null, s, ftsMain, ftsAux);
				break;
			}
			case EAITargetClusterState.DEFENDING:
			{
				activity = new SCR_AIDefendFromClusterActivity(m_Utility, null, s, ftsMain);
				break;
			}
		}

		return activity;
	}

	void AllocateFTForInvestigate(SCR_AITargetClusterState s, notnull TFireteamLockRefArray inOutFtLocksMain, notnull TFireteamLockRefArray ftLocksAux)
	{
		array<SCR_AIGroupFireteam> freeFireteams = {};
		m_Utility.m_FireteamMgr.GetFreeFireteams(freeFireteams, SCR_AIGroupFireteam);
		while (!freeFireteams.IsEmpty())
		{
			SCR_AIGroupFireteam newFireteam = freeFireteams[0];
			SCR_AIGroupFireteamLock newFtLock = newFireteam.TryLock();
			inOutFtLocksMain.Insert(newFtLock);
			freeFireteams.Remove(0);
		}
	}

	protected void DCO_SplitInvestigateFireteams(notnull TFireteamLockRefArray ftsMain, notnull TFireteamLockRefArray ftsAux)
	{
		TFireteamLockRefArray sorted = {};
		array<int> sortedIndoor = {};

		for (int i = 0, count = ftsMain.Count() + ftsAux.Count(); i < count; i++)
		{
			SCR_AIGroupFireteamLock ftLock;
			if (i < ftsMain.Count())
				ftLock = ftsMain[i];
			else
				ftLock = ftsAux[i - ftsMain.Count()];

			int indoor = DCO_CountIndoor(ftLock.GetFireteam());

			int at = 0;
			while (at < sortedIndoor.Count() && sortedIndoor[at] <= indoor)
				at++;

			sorted.InsertAt(ftLock, at);
			sortedIndoor.InsertAt(indoor, at);
		}

		int mainCount = Math.Max(1, (sorted.Count() + 1) / 2);

		ftsMain.Clear();
		ftsAux.Clear();
		foreach (int idx, SCR_AIGroupFireteamLock sortedLock : sorted)
		{
			if (idx < mainCount)
				ftsMain.Insert(sortedLock);
			else
				ftsAux.Insert(sortedLock);
		}
	}

	protected int DCO_CountIndoor(SCR_AIGroupFireteam ft)
	{
		if (!ft)
			return 0;

		array<AIAgent> agents = {};
		ft.GetMembers(agents);

		int indoor = 0;
		foreach (AIAgent agent : agents)
		{
			if (agent && DCO_CoverManagerComponent.DCO_GetBuildingAt(agent.GetControlledEntity()))
				indoor++;
		}

		return indoor;
	}
}