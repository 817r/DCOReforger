class DCO_SpecState
{
	vector m_vSpot;
	float m_fSpotAt_ms;
	int m_iSpotAmmo = -1;
	bool m_bBaseOfFire;
	float m_fNearImpact_ms;
	float m_fNextTick_ms;
	SCR_ChimeraAIAgent m_Assistant;
	float m_fNextFlank_ms;
	float m_fNextShift_ms;
	float m_fNextWorthy_ms;
	bool m_bLauncherAllowed = true;
	vector m_vOverwatch;
	float m_fNextOverwatch_ms;
	float m_fBofAt_ms = -1;
	int m_iSortie;
	string m_sSortieWhy = "-";
	float m_fSortieStart_ms;
	float m_fSortieSeen_ms;
	float m_fSortieCooldown_ms;
	float m_fSortieNextGoal_ms;
	vector m_vSortieFrom;
	IEntity m_SortieTarget;
	vector m_vSortieGoal;
	float m_fNextHunt_ms;
	float m_fSortieSide;
	float m_fSortieStandoff;
	SCR_AIWaitBehavior m_OverwatchWait;
	bool m_bBofUnseen;
	vector m_vEstUsed;
	float m_fNextSupp_ms;
	float m_fNextLaneCheck_ms;
	float m_fNextLaneShift_ms;
	float m_fContactAt_ms = -1;
	float m_fLastFight_ms = -100000;
	float m_fRapidUntil_ms;
	bool m_bRapid;
	int m_iTravStep;
	float m_fTravNext_ms;
	float m_fNextTeam_ms;
	bool m_bSortieLeader;
	ref DCO_MGEval m_Eval;
	float m_fNextPre_ms;
	vector m_vLeaderLast;
	float m_fLeaderStill_ms;
	float m_fLeaderCheck_ms;
	vector m_vPreSpot;
	int m_iScootNeed = 3;
	float m_fNextGL_ms;
	float m_fNextDesig_ms;
}

class DCO_Roles
{
	static int Get(SCR_AIUtilityComponent u)
	{
		if (!u || !u.m_OwnerEntity)
			return 0;
		float now = GetGame().GetWorld().GetWorldTime();
		if (now < u.m_fDCORolesAt_ms)
			return u.m_iDCORoles;
		u.m_fDCORolesAt_ms = now + 10000.0;
		u.m_iDCORoles = Compute(u.m_OwnerEntity, u.GetAIAgent());
		return u.m_iDCORoles;
	}

	static int Compute(IEntity ent, AIAgent agent)
	{
		int mask = 0;
		BaseWeaponManagerComponent wm = BaseWeaponManagerComponent.Cast(ent.FindComponent(BaseWeaponManagerComponent));
		if (wm)
		{
			array<IEntity> weapons = {};
			wm.GetWeaponsList(weapons);
			foreach (IEntity w : weapons)
			{
				BaseWeaponComponent wc = BaseWeaponComponent.Cast(w.FindComponent(BaseWeaponComponent));
				if (!wc)
					continue;
				EWeaponType wt = wc.GetWeaponType();
				if (wt == EWeaponType.WT_MACHINEGUN)
				{
					mask = mask | 1;
					if (IsLightMG(w, wc))
						mask = mask | 64;
				}
				else if (wt == EWeaponType.WT_ROCKETLAUNCHER)
					mask = mask | 2;
				else if (wt == EWeaponType.WT_SNIPERRIFLE)
					mask = mask | 4;
				if (DCO_UGLUtility.HasUGL(wc))
					mask = mask | 8;
			}
		}
		if (DCO_Util.HasEditorLabel(ent, EEditableEntityLabel.ROLE_SHARPSHOOTER))
			mask = mask | 4;
		if (DCO_Util.HasEditorLabel(ent, EEditableEntityLabel.ROLE_GRENADIER))
			mask = mask | 8;
		if (DCO_Util.HasManpackRadio(ent))
			mask = mask | 16;
		if (agent && agent.GetParentGroup() && agent.GetParentGroup().GetLeaderEntity() == ent)
			mask = mask | 32;
		return mask;
	}

	static bool IsLightMG(IEntity w, BaseWeaponComponent wc)
	{
		string n;
		if (w.GetPrefabData())
			n = w.GetPrefabData().GetPrefabName();
		n.ToUpper();
		if (n.Contains("M249") || n.Contains("RPK") || n.Contains("MINIMI") || n.Contains("M27") || n.Contains("MK46") || n.Contains("LMG"))
			return true;
		if (n.Contains("M60") || n.Contains("PKM") || n.Contains("PKP") || n.Contains("MG3") || n.Contains("M240") || n.Contains("UK59") || n.Contains("GPMG"))
			return false;
		BaseMagazineComponent mag = wc.GetCurrentMagazine();
		if (!mag || !mag.GetOwner() || !mag.GetOwner().GetPrefabData())
			return false;
		string m = mag.GetOwner().GetPrefabData().GetPrefabName();
		m.ToUpper();
		return m.Contains("556") || m.Contains("545") || m.Contains("5_56") || m.Contains("5_45");
	}

	static string MGRole(int roles)
	{
		if (roles & 64)
			return "ar";
		return "mg";
	}
}

class DCO_Specialist
{
	static void CombatMove(SCR_AIUtilityComponent u, vector pos, vector targetPos, bool findCover, ECharacterStance endStance, float duration)
	{
		if (!u || !u.m_CombatMoveState || (u.m_AIInfo && u.m_AIInfo.HasUnitState(EUnitState.IN_VEHICLE)))
			return;
		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();
		rq.m_eType = SCR_EAICombatMoveRequestType.MOVE;
		rq.m_eReason = SCR_EAICombatMoveReason.STANDARD;
		rq.m_vTargetPos = targetPos;
		rq.m_vMovePos = pos;
		rq.m_bTryFindCover = findCover;
		rq.m_bFailIfNoCover = false;
		rq.m_bUseCoverSearchDirectivity = false;
		rq.m_bCheckCoverVisibility = findCover;
		rq.m_fCoverSearchDistMin = 0;
		rq.m_fCoverSearchDistMax = 8;
		rq.m_eStanceMoving = ECharacterStance.CROUCH;
		rq.m_eStanceEnd = endStance;
		rq.m_eMovementType = EMovementType.RUN;
		rq.m_bAimAtTarget = false;
		rq.m_bAimAtTargetEnd = true;
		rq.m_fMoveDuration_s = Math.Clamp(duration, 2, 30);
		u.m_CombatMoveState.ApplyNewRequest(rq);
	}

	static void MoveIndividually(SCR_AIUtilityComponent u, vector pos)
	{
		if (!u || (u.m_AIInfo && u.m_AIInfo.HasUnitState(EUnitState.IN_VEHICLE)))
			return;
		SCR_AIMoveIndividuallyBehavior existing = SCR_AIMoveIndividuallyBehavior.Cast(u.FindActionOfType(SCR_AIMoveIndividuallyBehavior));
		if (existing)
		{
			existing.m_vPosition.m_Value = pos;
			return;
		}
		u.AddAction(new SCR_AIMoveIndividuallyBehavior(u, null, pos));
	}

	static void SpecMove(SCR_AIUtilityComponent u, vector pos, vector threat, bool findCover, ECharacterStance stance, float duration, string why, bool crit = false)
	{
		DCO_GunnerBehavior gb = DCO_GunnerBehavior.Get(u);
		if (gb)
		{
			gb.MoveTo(pos, threat, findCover, stance, duration, why, crit);
			return;
		}
		DCO_RelocateBehavior.Start(u, pos, threat, findCover, stance, duration, why);
	}

	static bool Busy(SCR_AIUtilityComponent u)
	{
		DCO_GunnerBehavior gb = DCO_GunnerBehavior.Get(u);
		if (gb)
			return gb.IsMoving();
		return u.m_CombatMoveState && u.m_CombatMoveState.IsExecutingRequest();
	}

	static bool InCombat(SCR_AIUtilityComponent u)
	{
		BaseTarget t = u.m_CombatComponent.GetCurrentTarget();
		return t && t.GetTargetEntity() && t.GetTimeSinceSeen() < 10.0;
	}

	static vector Flat(vector v)
	{
		v[1] = 0;
		if (v.LengthSq() < 0.0001)
			return "0 0 1";
		return v.Normalized();
	}

	static void Tick(SCR_AIUtilityComponent u, DCO_SpecState st, float now)
	{
		if (now < st.m_fNextTick_ms)
			return;
		st.m_fNextTick_ms = now + 500.0;
		if (!u.m_OwnerEntity || !DCO_Util.IsAlive(u.m_OwnerEntity) || (u.m_AIInfo && u.m_AIInfo.HasUnitState(EUnitState.IN_VEHICLE)))
			return;

		if (st.m_iSortie != 0)
			TickSortie(u, st, now);
		int roles = DCO_Roles.Get(u);
		if ((roles & 36) && now >= st.m_fNextDesig_ms && DCO_Designate.On(u))
		{
			st.m_fNextDesig_ms = now + 3000.0;
			int p0 = DCO_Perf.Begin();
			DCO_Designate.Scan(u, now);
			DCO_Perf.End("spec_designate", p0);
		}
		if ((roles & 1) && DCO_GlobalAIComponent.BehaviorOnUtility(u, DCO_EBehavior.MG_GUNNER))
		{
			int p1 = DCO_Perf.Begin();
			TickMG(u, st, now);
			DCO_Perf.End("spec_mg", p1);
		}
		if ((roles & 2) && DCO_GlobalAIComponent.BehaviorOnUtility(u, DCO_EBehavior.AT_SPECIALIST))
		{
			int p2 = DCO_Perf.Begin();
			TickAT(u, st, now);
			DCO_Perf.End("spec_at", p2);
		}
		if ((roles & 4) && DCO_GlobalAIComponent.BehaviorOnUtility(u, DCO_EBehavior.MARKSMAN))
		{
			int p3 = DCO_Perf.Begin();
			TickMarksman(u, st, now);
			DCO_Perf.End("spec_marksman", p3);
		}
		if ((roles & 8) && !(roles & 3) && DCO_GlobalAIComponent.BehaviorOnUtility(u, DCO_EBehavior.LEADER_RTO_GRENADIER) && DCO_GunnerBehavior.On(u))
		{
			int p4 = DCO_Perf.Begin();
			TickGL(u, st, now);
			DCO_Perf.End("spec_gl", p4);
		}
	}

	static string SortieName(int kind)
	{
		if (kind == 1)
			return "mg_position";
		if (kind == 2)
			return "at_hunt";
		if (kind == 4)
			return "marksman_overwatch";
		return "none";
	}

	static bool StartSortie(SCR_AIUtilityComponent u, DCO_SpecState st, int kind, string why, IEntity target, float now)
	{
		IEntity me = u.m_OwnerEntity;
		if (st.m_iSortie != 0)
			return st.m_iSortie == kind;
		if (now < st.m_fSortieCooldown_ms)
		{
			DCO_Perf.Count("spec:sortie_cooldown_" + SortieName(kind));
			if (DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG))
				DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(me), string.Format("spec_sortie_skip unit=%1 kind=%2 why=cooldown left_s=%3", DCO_Log.IdUnit(me), SortieName(kind), Math.Round((st.m_fSortieCooldown_ms - now) / 1000.0)));
			return false;
		}
		int owner = DCO_CtrlLock.OwnerOf(u.GetAIAgent());
		if (!SortieOwnerOk(owner))
		{
			DCO_Perf.Count("spec:sortie_skip_owner");
			if (DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG))
				DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(me), string.Format("spec_sortie_skip unit=%1 kind=%2 why=owner_%3", DCO_Log.IdUnit(me), SortieName(kind), DCO_CtrlLock.OwnerName(owner)));
			return false;
		}
		st.m_iSortie = kind;
		st.m_sSortieWhy = why;
		st.m_fSortieStart_ms = now;
		st.m_fSortieSeen_ms = now;
		st.m_fSortieNextGoal_ms = 0;
		st.m_fSortieSide = 0;
		st.m_vSortieFrom = me.GetOrigin();
		st.m_SortieTarget = target;
		st.m_bSortieLeader = u.GetAIAgent().GetParentGroup() && u.GetAIAgent().GetParentGroup().GetLeaderEntity() == me;
		DCO_Perf.Count("spec:sortie_start_" + SortieName(kind));
		float leaderDist = -1;
		IEntity leader = SortieLeader(u);
		if (leader)
			leaderDist = vector.DistanceXZ(leader.GetOrigin(), me.GetOrigin());
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("spec_sortie_start unit=%1 kind=%2 why=%3 target=%4 leader_dist=%5 timeout_s=300", DCO_Log.IdUnit(me), SortieName(kind), why, DCO_Log.IdUnit(target), Math.Round(leaderDist)));
		return true;
	}

	protected static IEntity SortieLeader(SCR_AIUtilityComponent u)
	{
		AIGroup grp = u.GetAIAgent().GetParentGroup();
		if (!grp)
			return null;
		IEntity leader = grp.GetLeaderEntity();
		if (leader == u.m_OwnerEntity)
			return null;
		return leader;
	}

	protected static void TickSortie(SCR_AIUtilityComponent u, DCO_SpecState st, float now)
	{
		IEntity me = u.m_OwnerEntity;
		IEntity leader = SortieLeader(u);
		float leash = 150.0;
		if (st.m_iSortie == 2)
			leash = 500.0;
		else if (st.m_iSortie == 4)
			leash = 450.0;
		float leaderDist = 0;
		if (leader)
			leaderDist = vector.DistanceXZ(leader.GetOrigin(), me.GetOrigin());

		string end;
		if (now - st.m_fSortieStart_ms > 300000.0)
			end = "timeout";
		else if (leaderDist > leash)
			end = "leash";
		else if (!st.m_bSortieLeader && u.GetAIAgent().GetParentGroup() && u.GetAIAgent().GetParentGroup().GetLeaderEntity() == me)
			end = "became_leader";
		else if (!SortieOwnerOk(DCO_CtrlLock.OwnerOf(u.GetAIAgent())))
			end = "role_" + DCO_CtrlLock.OwnerName(DCO_CtrlLock.OwnerOf(u.GetAIAgent()));
		else if (st.m_iSortie == 2)
		{
			if (!st.m_SortieTarget || VehicleDead(st.m_SortieTarget))
				end = "target_gone";
			else if (DCO_RpgUnseen.Rockets(me) <= 0)
				end = "no_rockets";
			else if (now - st.m_fSortieSeen_ms > 90000.0)
				end = "lost";
		}
		else if (st.m_iSortie == 4 && st.m_vOverwatch != vector.Zero)
		{
			float od = vector.DistanceXZ(me.GetOrigin(), st.m_vOverwatch);
			bool waiting = st.m_OverwatchWait && st.m_OverwatchWait.GetActionState() != EAIActionState.COMPLETED && st.m_OverwatchWait.GetActionState() != EAIActionState.FAILED;
			if (od <= 8.0 && !waiting)
			{
				SCR_AIWaitBehavior holdWait = new SCR_AIWaitBehavior(u, null);
				holdWait.SetPriority(45.0);
				u.AddAction(holdWait);
				st.m_OverwatchWait = holdWait;
				CharacterControllerComponent hctrl = u.GetCharacterController();
				if (hctrl && DCO_CQC.IsProneAllowed(me, 300.0))
					DCO_Stance.Set(hctrl, ECharacterStance.PRONE, "specialist");
				DCO_Perf.Count("spec:marksman_hold");
				DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("spec_marksman_hold unit=%1 spot=%2 leader_dist=%3", DCO_Log.IdUnit(me), DCO_Log.Pos(st.m_vOverwatch), Math.Round(leaderDist)));
			}
			if (od > 8.0 && !(InCombat(u) && od < 25.0) && now >= st.m_fSortieNextGoal_ms)
			{
				st.m_fSortieNextGoal_ms = now + 4000.0;
				SortieMove(u, st.m_vOverwatch);
				DCO_Perf.Count("spec:marksman_hold_return");
				if (DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG))
					DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(me), string.Format("spec_marksman_hold_return unit=%1 dist=%2 spot=%3", DCO_Log.IdUnit(me), Math.Round(od), DCO_Log.Pos(st.m_vOverwatch)));
			}
		}
		else if (st.m_iSortie == 1)
		{
			if (InCombat(u))
				st.m_fSortieSeen_ms = now;
			else if (now - st.m_fSortieSeen_ms > 60000.0)
				end = "no_contact";
		}
		if (end.IsEmpty())
			return;
		EndSortie(u, st, end, now, leader, leaderDist);
	}

	protected static bool SortieOwnerOk(int owner)
	{
		return owner == DCO_ECtrlOwner.NONE || owner == DCO_ECtrlOwner.IDLE_DEFEND;
	}

	protected static bool VehicleDead(IEntity veh)
	{
		SCR_VehicleDamageManagerComponent dm = SCR_VehicleDamageManagerComponent.Cast(veh.FindComponent(SCR_VehicleDamageManagerComponent));
		return dm && dm.IsDestroyed();
	}

	static void EndSortie(SCR_AIUtilityComponent u, DCO_SpecState st, string end, float now, IEntity leader, float leaderDist)
	{
		IEntity me = u.m_OwnerEntity;
		float cd_s = 90.0;
		if (st.m_iSortie == 2)
			cd_s = 120.0;
		else if (st.m_iSortie == 1)
			cd_s = 60.0;
		if (end == "target_gone" || end.StartsWith("role_"))
			cd_s = 30.0;
		st.m_fSortieCooldown_ms = now + cd_s * 1000.0;
		bool back = leader && leaderDist > 25.0;
		if (back)
		{
			vector pos = DCO_Util.Surface(leader.GetOrigin());
			SCR_AIMoveIndividuallyBehavior existing = SCR_AIMoveIndividuallyBehavior.Cast(u.FindActionOfType(SCR_AIMoveIndividuallyBehavior));
			if (existing)
				existing.Fail();
			u.AddAction(new SCR_AIMoveIndividuallyBehavior(u, null, pos, 60.0, ent: leader, radius: 15.0));
		}
		DCO_Perf.Count("spec:sortie_end_" + end);
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("spec_sortie_end unit=%1 kind=%2 why=%3 result=%4 dur_s=%5 moved=%6 leader_dist=%7 return=%8 cooldown_s=%9", DCO_Log.IdUnit(me), SortieName(st.m_iSortie), st.m_sSortieWhy, end, Math.Round((now - st.m_fSortieStart_ms) / 1000.0), Math.Round(vector.DistanceXZ(st.m_vSortieFrom, me.GetOrigin())), Math.Round(leaderDist), DCO_Util.B01(back), cd_s));
		st.m_iSortie = 0;
		st.m_SortieTarget = null;
		st.m_sSortieWhy = "-";
		st.m_vSortieGoal = vector.Zero;
		st.m_fSortieStandoff = 0;
		if (st.m_OverwatchWait)
		{
			st.m_OverwatchWait.Fail();
			st.m_OverwatchWait = null;
		}
		st.m_vOverwatch = vector.Zero;
	}

	protected static void TickMG(SCR_AIUtilityComponent u, DCO_SpecState st, float now)
	{
		vector phEst;
		float phRad;
		int phCnt;
		bool fighting = InCombat(u) || DCO_FireEstimate.Get(u.GetAIAgent().GetParentGroup(), now, 25000.0, phEst, phRad, phCnt);
		if (fighting)
			UpdateFirePhase(u, st, now);
		vector gbThreat;
		bool gbHas = ThreatPos(u, now, gbThreat);
		if (fighting || gbHas)
		{
			if (!fighting && !DCO_GunnerBehavior.Get(u))
				DCO_Perf.Count("spec_bt:start_group_target");
			DCO_GunnerBehavior.Ensure(u, DCO_Roles.MGRole(DCO_Roles.Get(u)), gbThreat, now);
		}
		if (fighting && (DCO_Roles.Get(u) & 64))
		{
			vector teamThreat;
			if (ThreatPos(u, now, teamThreat) && TickARTeam(u, st, now, teamThreat))
				return;
		}
		if (!(DCO_Roles.Get(u) & 64))
		{
			if (st.m_Eval && TickEval(u, st, now))
				return;
			if (!fighting)
				TickMGPre(u, st, now);
		}
		if (!InCombat(u))
		{
			if (TickMGUnseen(u, st, now))
				return;
			if (now - st.m_fBofAt_ms > 45000.0)
			{
				st.m_bBaseOfFire = false;
				st.m_bBofUnseen = false;
			}
			return;
		}

		IEntity me = u.m_OwnerEntity;
		vector myPos = me.GetOrigin();
		vector enemyPos = u.m_CombatComponent.GetCurrentTarget().GetLastSeenPosition();
		AIGroup grp = u.GetAIAgent().GetParentGroup();
		IEntity leader;
		if (grp)
			leader = grp.GetLeaderEntity();
		DCO_GroupUtilityComponent gu;
		if (grp)
			gu = DCO_GroupUtilityComponent.Cast(grp.FindComponent(DCO_GroupUtilityComponent));

		float enemyDist = vector.DistanceXZ(myPos, enemyPos);
		int roleOwner = DCO_CtrlLock.OwnerOf(u.GetAIAgent());
		if (roleOwner == DCO_ECtrlOwner.FM_FIX || roleOwner == DCO_ECtrlOwner.FM_ASSAULT)
		{
			TickMGInRole(u, st, now, roleOwner, me, myPos, enemyPos, grp);
			return;
		}
		int mgRoles = DCO_Roles.Get(u);
		bool ar = (mgRoles & 64) != 0;
		if (st.m_bBaseOfFire && st.m_bBofUnseen && vector.DistanceXZ(st.m_vEstUsed, enemyPos) > 60.0)
		{
			st.m_bBaseOfFire = false;
			DCO_Perf.Count("spec:mg_bof_reseat_seen");
		}
		st.m_bBofUnseen = false;
		if (!st.m_bBaseOfFire && leader && enemyDist >= 60.0 && enemyDist <= 600.0)
		{
			st.m_bBaseOfFire = true;
			st.m_fBofAt_ms = now;
			if (SCR_CoverManagerComponent.IsEntityInsideBuilding(me))
			{
				DCO_Perf.Count("spec:mg_bof_skip_indoor");
				DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("spec_mg_base_of_fire unit=%1 result=skip_indoor enemy_dist=%2", DCO_Log.IdUnit(me), Math.Round(enemyDist)));
				return;
			}
			vector lp = leader.GetOrigin();
			vector axis = Flat(enemyPos - lp);
			vector side = Vector(axis[2], 0, -axis[0]);
			if (vector.Dot(side, myPos - lp) < 0)
				side = -side;
			ECharacterStance bofStance;
			int tried;
			if (!ar)
				StartEval(u, st, now, enemyPos, lp, "contact_seen");
			float bofMove = 30.0;
			if (ar)
				bofMove = 15.0;
			vector pos = PickFireSpot(me, enemyPos, lp, side, axis, bofStance, tried, bofMove);
			string task = "-";
			if (gu)
				task = typename.EnumToString(DCO_EGroupTask, gu.GetTask());
			task += " role=" + DCO_Roles.MGRole(mgRoles);
			if (pos == vector.Zero && ar)
				DCO_Perf.Count("spec:ar_no_sortie");
			if (pos == vector.Zero && !ar && now >= st.m_fSortieCooldown_ms)
			{
				pos = PickWideFireSpot(me, enemyPos, lp, side, axis, bofStance, tried);
				if (pos != vector.Zero && !StartSortie(u, st, 1, "no_los_near", null, now))
					pos = vector.Zero;
				if (pos != vector.Zero)
				{
					SpecMove(u, pos, enemyPos, true, bofStance, vector.Distance(myPos, pos) / 3.0 + 5.0, "mg_sortie_position");
					st.m_vSpot = pos;
					st.m_fSpotAt_ms = now + 10000.0;
					DCO_Perf.Count("spec:mg_sortie_position");
					DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("spec_mg_base_of_fire unit=%1 result=sortie pos=%2 move_dist=%3 leader_dist=%4 enemy_dist=%5 stance=%6 tried=%7 task=%8", DCO_Log.IdUnit(me), DCO_Log.Pos(pos), Math.Round(vector.Distance(myPos, pos)), Math.Round(vector.DistanceXZ(pos, lp)), Math.Round(vector.DistanceXZ(pos, enemyPos)), typename.EnumToString(ECharacterStance, bofStance), tried, task));
					return;
				}
			}
			if (pos == vector.Zero)
			{
				st.m_vSpot = myPos;
				st.m_fSpotAt_ms = now;
				DCO_Perf.Count("spec:mg_bof_no_los");
				DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("spec_mg_base_of_fire unit=%1 result=no_los_spot tried=%2 enemy_dist=%3 task=%4", DCO_Log.IdUnit(me), tried, Math.Round(enemyDist), task));
				return;
			}
			SpecMove(u, pos, enemyPos, true, bofStance, vector.Distance(myPos, pos) / 3.0 + 3.0, "mg_base_of_fire");
			st.m_vSpot = pos;
			st.m_fSpotAt_ms = now + 5000.0;
			DCO_Perf.Count("spec:mg_base_of_fire");
			DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("spec_mg_base_of_fire unit=%1 result=move pos=%2 move_dist=%3 enemy_dist=%4 stance=%5 tried=%6 task=%7", DCO_Log.IdUnit(me), DCO_Log.Pos(pos), Math.Round(vector.Distance(myPos, pos)), Math.Round(vector.DistanceXZ(pos, enemyPos)), typename.EnumToString(ECharacterStance, bofStance), tried, task));
			return;
		}

		if (vector.DistanceXZ(myPos, st.m_vSpot) > 6.0 && now > st.m_fSpotAt_ms)
		{
			st.m_vSpot = myPos;
			st.m_fSpotAt_ms = now;
		}

		SCR_CharacterDamageManagerComponent dm = SCR_CharacterDamageManagerComponent.Cast(me.FindComponent(SCR_CharacterDamageManagerComponent));
		bool hit = dm && dm.DCO_HitsWithin(4.0) > 0;
		bool accurate = st.m_fNearImpact_ms > 0 && now - st.m_fNearImpact_ms < 3000.0;
		bool longHere = now - st.m_fSpotAt_ms > 20000.0 && u.m_CombatComponent.GetCurrentTarget().GetTimeSinceSeen() < 3.0;
		if ((accurate || longHere || hit) && now > st.m_fSpotAt_ms && !SCR_CoverManagerComponent.IsEntityInsideBuilding(me))
		{
			vector toEnemy = Flat(enemyPos - myPos);
			vector lateral = Vector(toEnemy[2], 0, -toEnemy[0]);
			if (Math.RandomFloat01() < 0.5)
				lateral = -lateral;
			ECharacterStance dispStance;
			int dTried;
			vector newPos = PickFireSpot(me, enemyPos, myPos, lateral, toEnemy, dispStance, dTried);
			if (newPos == vector.Zero)
			{
				newPos = DCO_Util.Surface(myPos + lateral * Math.RandomFloat(10, 20) - toEnemy * 5.0);
				dispStance = ECharacterStance.PRONE;
			}
			SpecMove(u, newPos, enemyPos, true, dispStance, 8.0, "mg_displace", hit || accurate);
			string why = "long_here";
			if (hit)
				why = "hit";
			else if (accurate)
				why = "accurate_fire";
			DCO_Perf.Count("spec:mg_displace");
			DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("spec_mg_displace unit=%1 why=%2 from=%3 to=%4 move_dist=%5 stance=%6 held_s=%7", DCO_Log.IdUnit(me), why, DCO_Log.Pos(myPos), DCO_Log.Pos(newPos), Math.Round(vector.Distance(myPos, newPos)), typename.EnumToString(ECharacterStance, dispStance), Math.Round((now - st.m_fSpotAt_ms) / 1000.0)));
			st.m_vSpot = newPos;
			st.m_fSpotAt_ms = now + 5000.0;
			st.m_fNearImpact_ms = 0;
		}

		if (ClearLane(u, st, now, enemyPos, DCO_Roles.MGRole(mgRoles)))
			return;
		BaseTarget lkT = u.m_CombatComponent.GetCurrentTarget();
		if (lkT && lkT.GetTimeSinceSeen() > 2.0 && !Busy(u) && MGReady(u) && DCO_GunnerBehavior.Get(u))
		{
			DCO_Designation cdz = DCO_Designate.Best(grp, 2, myPos, now, 8000.0, 40.0, 600.0);
			if (cdz && vector.DistanceXZ(cdz.m_vPos, enemyPos) > 20.0)
			{
				IEntity cdzBlocker;
				if (DCO_Util.FireLaneCheck(me, cdz.m_vPos, 1.5, cdzBlocker) == 0 && TraverseFire(u, st, now, cdz.m_vPos, DCO_InfoQ.SeenRadius((now - cdz.m_fAt_ms) / 1000.0), ar, DCO_Roles.MGRole(mgRoles), "designated", 1))
				{
					DCO_Designate.Note(cdz, me, DCO_Roles.MGRole(mgRoles), now);
					UpdateAssistant(u, st, grp, leader, enemyPos);
					return;
				}
			}
		}
		if (lkT && lkT.GetTimeSinceSeen() > 4.0 && !Busy(u) && MGReady(u))
		{
			IEntity lkBlocker;
			if (DCO_Util.FireLaneCheck(me, enemyPos, 1.5, lkBlocker) == 0)
				TraverseFire(u, st, now, enemyPos, 6.0, ar, DCO_Roles.MGRole(mgRoles), "last_known", 0);
		}
		UpdateAssistant(u, st, grp, leader, enemyPos);
	}

	protected static bool GroupKnownEnemy(SCR_AIUtilityComponent u, AIGroup grp, out vector pos)
	{
		float rad;
		return GroupKnownEnemyQ(u, grp, pos, rad);
	}

	protected static bool GroupKnownEnemyQ(SCR_AIUtilityComponent u, AIGroup grp, out vector pos, out float rad)
	{
		rad = 6.0;
		SCR_AIGroupUtilityComponent gu = SCR_AIGroupUtilityComponent.Cast(grp.FindComponent(SCR_AIGroupUtilityComponent));
		PerceptionManager pm = GetGame().GetPerceptionManager();
		if (!gu || !gu.m_Perception || !pm)
			return false;
		float pmNow = pm.GetTime();
		vector myPos = u.m_OwnerEntity.GetOrigin();
		float bestD = 600.0;
		bool found;
		foreach (SCR_AITargetInfo ti : gu.m_Perception.m_aTargets)
		{
			if (!ti || !ti.m_Entity || pmNow - ti.m_fTimestamp > 15.0 || !DCO_Util.IsAlive(ti.m_Entity))
				continue;
			float d = vector.DistanceXZ(myPos, ti.m_vWorldPos);
			if (d < 40.0 || d >= bestD)
				continue;
			bestD = d;
			pos = ti.m_vWorldPos;
			found = true;
			float injected = -1;
			bool reported = gu.m_mDCOInjected.Find(ti.m_Entity, injected) && ti.m_fTimestamp <= injected + 0.01;
			rad = DCO_InfoQ.TargetRadius(pmNow - ti.m_fTimestamp, reported);
		}
		if (found)
			DCO_Perf.Count("spec:mg_group_target");
		return found;
	}

	protected static bool ThreatPos(SCR_AIUtilityComponent u, float now, out vector pos)
	{
		if (InCombat(u))
		{
			pos = u.m_CombatComponent.GetCurrentTarget().GetLastSeenPosition();
			return true;
		}
		AIGroup grp = u.GetAIAgent().GetParentGroup();
		float rad;
		int cnt;
		if (DCO_FireEstimate.Get(grp, now, 25000.0, pos, rad, cnt))
			return true;
		return grp && GroupKnownEnemy(u, grp, pos);
	}

	protected static IEntity TeamAnchor(SCR_AIUtilityComponent u)
	{
		IEntity me = u.m_OwnerEntity;
		AIGroup grp = u.GetAIAgent().GetParentGroup();
		if (!grp)
			return null;
		IEntity best = grp.GetLeaderEntity();
		if (best == me || (best && !DCO_Util.IsAlive(best)))
			best = null;
		float bestD = 1000000.0;
		if (best)
			bestD = vector.DistanceXZ(best.GetOrigin(), me.GetOrigin());
		array<AIAgent> agents = {};
		grp.GetAgents(agents);
		foreach (AIAgent a : agents)
		{
			IEntity e = a.GetControlledEntity();
			if (!e || e == me || !DCO_Util.IsAlive(e) || !e.GetPrefabData())
				continue;
			string n = e.GetPrefabData().GetPrefabName();
			n.ToUpper();
			if (!n.Contains("_TL") && !n.Contains("TEAMLEADER") && !n.Contains("_FTL"))
				continue;
			float d = vector.DistanceXZ(e.GetOrigin(), me.GetOrigin());
			if (d < bestD)
			{
				bestD = d;
				best = e;
			}
		}
		return best;
	}

	protected static bool TeammateBounding(SCR_AIUtilityComponent u)
	{
		IEntity me = u.m_OwnerEntity;
		AIGroup grp = u.GetAIAgent().GetParentGroup();
		if (!grp)
			return false;
		array<AIAgent> agents = {};
		grp.GetAgents(agents);
		foreach (AIAgent a : agents)
		{
			SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
			if (!ca || !ca.m_UtilityComponent || ca.m_UtilityComponent == u || !ca.m_UtilityComponent.m_CombatMoveState)
				continue;
			IEntity e = ca.GetControlledEntity();
			if (!e || vector.DistanceXZ(e.GetOrigin(), me.GetOrigin()) > 30.0 || !e.GetPhysics())
				continue;
			if (ca.m_UtilityComponent.m_CombatMoveState.IsMoving() && e.GetPhysics().GetVelocity().Length() > 1.5)
				return true;
		}
		return false;
	}

	protected static bool TickARTeam(SCR_AIUtilityComponent u, DCO_SpecState st, float now, vector threatPos)
	{
		if (now < st.m_fNextTeam_ms)
			return false;
		st.m_fNextTeam_ms = now + 3000.0;
		if (TeammateBounding(u))
		{
			if (now >= st.m_fRapidUntil_ms)
				DCO_Perf.Count("spec:ar_cover_move");
			Rapid(u, st, now, 8000.0, "cover_move");
			DCO_GunnerBehavior gbc = DCO_GunnerBehavior.Get(u);
			if (gbc && !gbc.IsMoving() && !InCombat(u) && MGReady(u))
			{
				IEntity cBlocker;
				if (DCO_Util.FireLaneCheck(u.m_OwnerEntity, threatPos, 1.5, cBlocker) == 0 && TraverseFire(u, st, now, threatPos, 6.0, true, "ar", "cover_move", 0))
					DCO_Perf.Count("spec:ar_cover_fire");
			}
		}
		if (Busy(u))
			return false;
		IEntity anchor = TeamAnchor(u);
		if (!anchor)
			return false;
		IEntity me = u.m_OwnerEntity;
		vector myPos = me.GetOrigin();
		vector ap = anchor.GetOrigin();
		float d = vector.DistanceXZ(myPos, ap);
		if (d <= 25.0)
			return false;
		BaseTarget seenT = u.m_CombatComponent.GetCurrentTarget();
		if (d <= 40.0 && seenT && seenT.GetTimeSinceSeen() < 2.0)
		{
			DCO_Perf.Count("spec:ar_team_hold_engaged");
			return false;
		}
		vector axis = Flat(threatPos - ap);
		vector side = Vector(axis[2], 0, -axis[0]);
		if (vector.Dot(side, myPos - ap) < 0)
			side = -side;
		vector aim = threatPos + Vector(0, 1.0, 0);
		array<float> offs = {8.0, -8.0, 12.0, -12.0, 5.0, -5.0};
		int tried;
		foreach (float o : offs)
		{
			vector p = DCO_Util.Surface(ap + side * o - axis * 2.0);
			tried++;
			if (!LaneFree(me, p, threatPos))
				continue;
			ECharacterStance stance = ECharacterStance.PRONE;
			if (!FireLine(p + Vector(0, 0.6, 0), aim, me, 6.0))
			{
				if (!FireLine(p + Vector(0, 1.2, 0), aim, me, 6.0))
					continue;
				stance = ECharacterStance.CROUCH;
			}
			SpecMove(u, p, threatPos, true, stance, d / 3.0 + 3.0, "ar_team");
			st.m_fNextTeam_ms = now + 15000.0;
			DCO_Perf.Count("spec:ar_team_move");
			DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("spec_ar_team unit=%1 result=move team=%2 team_dist=%3 to=%4 flank=%5 stance=%6 tried=%7", DCO_Log.IdUnit(me), DCO_Log.IdUnit(anchor), Math.Round(d), DCO_Log.Pos(p), Math.Round(o), typename.EnumToString(ECharacterStance, stance), tried));
			return true;
		}
		if (d <= 35.0)
			return false;
		st.m_fNextTeam_ms = now + 15000.0;
		vector close = DCO_Util.Surface(ap + side * 6.0 - axis * 2.0);
		SpecMove(u, close, threatPos, true, ECharacterStance.CROUCH, d / 3.0 + 3.0, "ar_team_close");
		DCO_Perf.Count("spec:ar_team_close");
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("spec_ar_team unit=%1 result=close_up team=%2 team_dist=%3 to=%4 tried=%5", DCO_Log.IdUnit(me), DCO_Log.IdUnit(anchor), Math.Round(d), DCO_Log.Pos(close), tried));
		return true;
	}

	protected static void StartEval(SCR_AIUtilityComponent u, DCO_SpecState st, float now, vector center, vector anchor, string mode)
	{
		bool contact = mode.StartsWith("contact");
		if (st.m_Eval && (!contact || st.m_Eval.m_sMode.StartsWith("contact")))
			return;
		float maxMove = 60.0;
		if (contact)
			maxMove = 100.0;
		st.m_Eval = DCO_MGPosition.Begin(u.m_OwnerEntity, center, anchor, maxMove, mode, now);
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(u.m_OwnerEntity), string.Format("spec_mg_position_eval unit=%1 mode=%2 threat=%3 threat_dist=%4 max_move=%5 candidates=%6", DCO_Log.IdUnit(u.m_OwnerEntity), mode, DCO_Log.Pos(center), Math.Round(vector.DistanceXZ(u.m_OwnerEntity.GetOrigin(), center)), Math.Round(maxMove), st.m_Eval.m_aCand.Count()));
	}

	protected static bool TickEval(SCR_AIUtilityComponent u, DCO_SpecState st, float now)
	{
		DCO_MGEval e = st.m_Eval;
		IEntity me = u.m_OwnerEntity;
		if (!DCO_MGPosition.Step(e, me, 8))
			return false;
		st.m_Eval = null;
		bool contact = e.m_sMode.StartsWith("contact");
		float need = 0.1;
		if (contact)
			need = 0.15;
		float gain = e.m_fBest - e.m_fHere;
		int ms = Math.Round(now - e.m_fStart_ms);
		if (e.m_fBest <= -999.0 || gain < need)
		{
			DCO_Perf.Count("spec:mg_position_stay");
			DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("spec_mg_position unit=%1 mode=%2 result=stay here=%3 best=%4 gain=%5 rejected=%6 ms=%7 here_why=%8", DCO_Log.IdUnit(me), e.m_sMode, e.m_fHere.ToString(-1, 2), e.m_fBest.ToString(-1, 2), gain.ToString(-1, 2), e.m_iRejected, ms, e.m_sHereWhy));
			if (!contact)
				st.m_vPreSpot = me.GetOrigin();
			return false;
		}
		float mv = vector.DistanceXZ(me.GetOrigin(), e.m_vBest);
		if (contact && mv > 30.0 && !StartSortie(u, st, 1, "position_eval", null, now))
		{
			DCO_Perf.Count("spec:mg_position_no_sortie");
			return false;
		}
		SpecMove(u, e.m_vBest, e.m_vCenter, false, e.m_eBestStance, mv / 3.0 + 5.0, "mg_pos_" + e.m_sMode);
		st.m_vSpot = e.m_vBest;
		st.m_fSpotAt_ms = now + 10000.0;
		if (!contact)
			st.m_vPreSpot = e.m_vBest;
		DCO_Perf.Count("spec:mg_position_move");
		DCO_Perf.Count("spec:mg_position_move_" + e.m_sMode);
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("spec_mg_position unit=%1 mode=%2 result=move here=%3 best=%4 gain=%5 move=%6 to=%7 stance=%8 ms=%9", DCO_Log.IdUnit(me), e.m_sMode, e.m_fHere.ToString(-1, 2), e.m_fBest.ToString(-1, 2), gain.ToString(-1, 2), Math.Round(mv), DCO_Log.Pos(e.m_vBest), typename.EnumToString(ECharacterStance, e.m_eBestStance), ms) + " why=" + e.m_sBestWhy + " here_why=" + e.m_sHereWhy);
		return true;
	}

	protected static void TickMGPre(SCR_AIUtilityComponent u, DCO_SpecState st, float now)
	{
		if (now < st.m_fLeaderCheck_ms)
			return;
		st.m_fLeaderCheck_ms = now + 4000.0;
		AIGroup grp = u.GetAIAgent().GetParentGroup();
		IEntity me = u.m_OwnerEntity;
		if (!grp)
			return;
		IEntity leader = grp.GetLeaderEntity();
		if (!leader)
			leader = me;
		vector lp = leader.GetOrigin();
		if (vector.DistanceXZ(lp, st.m_vLeaderLast) < 2.0)
			st.m_fLeaderStill_ms = st.m_fLeaderStill_ms + 4000.0;
		else
			st.m_fLeaderStill_ms = 0;
		st.m_vLeaderLast = lp;
		if (st.m_fLeaderStill_ms < 8000.0 || now < st.m_fNextPre_ms || st.m_Eval)
			return;
		if (Busy(u))
			return;
		if (DCO_Stress.T(u, "spec.mg_pre_yield_rearm", 1.0) > 0.5 && (u.FindActionOfType(DCO_AIRearmBehavior) || u.FindActionOfType(DCO_AIPickupWeaponBehavior)))
		{
			DCO_Perf.Count("spec:mg_precontact_yield_rearm");
			return;
		}
		if (st.m_vPreSpot != vector.Zero && vector.DistanceXZ(me.GetOrigin(), st.m_vPreSpot) < 5.0 && vector.DistanceXZ(lp, st.m_vPreSpot) < 70.0)
			return;
		vector center;
		string why = "leader_facing";
		AIWaypoint wp = grp.GetCurrentWaypoint();
		vector hp;
		float hr;
		float hc;
		int ht;
		if (DCO_HeardEst.Get(grp, now, 20000.0, hp, hr, hc, ht) && hc >= 0.4 && vector.DistanceXZ(lp, hp) >= 30.0 && vector.DistanceXZ(lp, hp) <= 400.0)
		{
			center = hp;
			why = "heard_" + DCO_Hearing.TypeName(ht);
		}
		else if (wp && vector.DistanceXZ(lp, wp.GetOrigin()) > 60.0)
		{
			center = wp.GetOrigin();
			why = "waypoint";
		}
		else
			center = DCO_Util.Surface(lp + Flat(leader.GetWorldTransformAxis(2)) * 200.0);
		st.m_fNextPre_ms = now + 45000.0;
		DCO_Perf.Count("spec:mg_precontact_" + why);
		StartEval(u, st, now, center, lp, "precontact");
	}

	protected static void UpdateFirePhase(SCR_AIUtilityComponent u, DCO_SpecState st, float now)
	{
		string role = DCO_Roles.MGRole(DCO_Roles.Get(u));
		if (now - st.m_fLastFight_ms > 60000.0)
		{
			st.m_fContactAt_ms = now;
			st.m_bRapid = true;
			st.m_iTravStep = 0;
			DCO_Perf.Count("spec:mg_phase_rapid");
			DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(u.m_OwnerEntity), string.Format("spec_mg_fire_phase unit=%1 role=%2 phase=rapid why=new_contact", DCO_Log.IdUnit(u.m_OwnerEntity), role));
		}
		st.m_fLastFight_ms = now;
		bool rapid = now - st.m_fContactAt_ms < 90000.0 || now < st.m_fRapidUntil_ms;
		if (rapid == st.m_bRapid)
			return;
		st.m_bRapid = rapid;
		string phase = "sustained";
		if (rapid)
			phase = "rapid";
		DCO_Perf.Count("spec:mg_phase_" + phase);
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(u.m_OwnerEntity), string.Format("spec_mg_fire_phase unit=%1 role=%2 phase=%3 contact_s=%4", DCO_Log.IdUnit(u.m_OwnerEntity), role, phase, Math.Round((now - st.m_fContactAt_ms) / 1000.0)));
	}

	static void Rapid(SCR_AIUtilityComponent u, DCO_SpecState st, float now, float ms, string why)
	{
		if (now >= st.m_fRapidUntil_ms)
			DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(u.m_OwnerEntity), string.Format("spec_mg_fire_phase unit=%1 role=%2 phase=rapid why=%3 for_s=%4", DCO_Log.IdUnit(u.m_OwnerEntity), DCO_Roles.MGRole(DCO_Roles.Get(u)), why, Math.Round(ms / 1000.0)));
		st.m_fRapidUntil_ms = now + ms;
		st.m_bRapid = true;
	}

	static float RateMul(DCO_SpecState st)
	{
		if (!st || st.m_fContactAt_ms < 0)
			return 1.0;
		if (st.m_bRapid)
			return 1.4;
		return 0.8;
	}

	protected static bool MGReady(SCR_AIUtilityComponent u)
	{
		BaseWeaponComponent wc;
		int muzzle;
		u.m_CombatComponent.GetSelectedWeapon(wc, muzzle);
		if (!wc)
		{
			CharacterControllerComponent ctrl = u.GetCharacterController();
			if (ctrl && ctrl.GetWeaponManagerComponent())
				wc = ctrl.GetWeaponManagerComponent().GetCurrentWeapon();
			if (wc)
				DCO_Perf.Count("spec:mg_ready_current_weapon");
		}
		if (!wc)
		{
			DCO_Perf.Count("spec:mg_ready_no_weapon");
			return false;
		}
		if (wc.GetWeaponType() != EWeaponType.WT_MACHINEGUN)
		{
			DCO_Perf.Count("spec:mg_ready_not_mg");
			return false;
		}
		if (DCO_AmmoUtility.ShouldAvoidSuppressiveFire(u, wc))
		{
			DCO_Perf.Count("spec:mg_traverse_ammo");
			return false;
		}
		return true;
	}

	protected static bool TraverseFire(SCR_AIUtilityComponent u, DCO_SpecState st, float now, vector center, float radius, bool ar, string role, string src, int reports)
	{
		DCO_GunnerBehavior gb = DCO_GunnerBehavior.Get(u);
		if (now < st.m_fTravNext_ms || (!gb && u.FindActionOfType(SCR_AISuppressBehavior)) || (gb && gb.IsMoving()))
			return false;
		IEntity me = u.m_OwnerEntity;
		vector toT = Flat(center - me.GetOrigin());
		vector lat = Vector(toT[2], 0, -toT[0]);
		float w = Math.Clamp(radius * 2.0, 8.0, 30.0);
		int step = st.m_iTravStep % 4;
		float off = 0;
		if (step == 0)
			off = -0.5 * w;
		else if (step == 2)
			off = 0.5 * w;
		vector c = DCO_Util.Surface(center + lat * off);
		vector bbMin, bbMax;
		SCR_AISuppressionVolumeBase.CreateSuppressionBox(c, 2.5, 3.0, bbMin, bbMax);
		SCR_AISuppressionObjectVolumeBox vol = new SCR_AISuppressionObjectVolumeBox(bbMin, bbMax);
		float rate;
		float dur;
		float pause;
		if (st.m_bRapid)
		{
			rate = 2.5;
			if (ar)
				rate = 2.0;
			dur = Math.RandomFloat(2.0, 3.0);
			pause = Math.RandomFloat(2.0, 4.0);
		}
		else
		{
			rate = 1.2;
			if (ar)
				rate = 0.9;
			dur = Math.RandomFloat(1.5, 2.5);
			pause = Math.RandomFloat(5.0, 8.0);
		}
		if (gb)
			gb.Suppress(vol, rate, dur, src);
		else
		{
			SCR_AISuppressBehavior supp = new SCR_AISuppressBehavior(u, null, vol, dur, rate);
			supp.SetPriority(68.0);
			u.AddAction(supp);
		}
		st.m_iTravStep = st.m_iTravStep + 1;
		float next = dur * 1000.0;
		if (st.m_iTravStep % 4 == 0)
			next += pause * 1000.0;
		st.m_fTravNext_ms = now + next;
		string phase = "sustained";
		if (st.m_bRapid)
			phase = "rapid";
		DCO_Perf.Count("spec:" + role + "_traverse_" + src);
		if (step == 0)
			DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("spec_mg_traverse unit=%1 role=%2 src=%3 center=%4 dist=%5 width=%6 phase=%7 rate=%8 reports=%9", DCO_Log.IdUnit(me), role, src, DCO_Log.Pos(center), Math.Round(vector.DistanceXZ(me.GetOrigin(), center)), Math.Round(w), phase, rate, reports));
		else if (DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG))
			DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(me), string.Format("spec_mg_traverse_step unit=%1 step=%2 off=%3 dur_s=%4", DCO_Log.IdUnit(me), step, Math.Round(off), dur.ToString(-1, 1)));
		return true;
	}

	static bool LaneFree(IEntity me, vector p, vector tgt)
	{
		AIControlComponent ac = AIControlComponent.Cast(me.FindComponent(AIControlComponent));
		if (!ac || !ac.GetControlAIAgent() || !ac.GetControlAIAgent().GetParentGroup())
			return true;
		vector dir = tgt - p;
		dir[1] = 0;
		float len = dir.Length();
		if (len < 1.0)
			return true;
		dir = dir * (1.0 / len);
		array<AIAgent> agents = {};
		ac.GetControlAIAgent().GetParentGroup().GetAgents(agents);
		foreach (AIAgent a : agents)
		{
			IEntity e = a.GetControlledEntity();
			if (!e || e == me || !DCO_Util.IsAlive(e))
				continue;
			vector rel = e.GetOrigin() - p;
			rel[1] = 0;
			float along = vector.Dot(rel, dir);
			if (along < 0.5 || along > len)
				continue;
			if ((rel - dir * along).Length() < 2.5 + along * 0.04)
				return false;
		}
		return true;
	}

	protected static bool ClearLane(SCR_AIUtilityComponent u, DCO_SpecState st, float now, vector tgt, string role)
	{
		if (now < st.m_fNextLaneCheck_ms || now < st.m_fNextLaneShift_ms)
			return false;
		st.m_fNextLaneCheck_ms = now + 2000.0;
		if (Busy(u))
			return false;
		IEntity me = u.m_OwnerEntity;
		IEntity blocker;
		int lr = DCO_Util.FireLaneCheck(me, tgt, 1.5, blocker);
		if (lr == 0)
			return false;
		st.m_fNextLaneShift_ms = now + 8000.0;
		vector myPos = me.GetOrigin();
		vector toT = Flat(tgt - myPos);
		vector lat = Vector(toT[2], 0, -toT[0]);
		if (blocker && vector.Dot(blocker.GetOrigin() - myPos, lat) > 0)
			lat = -lat;
		bool ar = role == "ar";
		float maxMove = 20.0;
		if (ar)
			maxMove = 12.0;
		vector aim = tgt + Vector(0, 1.0, 0);
		array<float> offs = {6.0, 10.0, -6.0, 15.0, -10.0, 20.0};
		array<float> backs = {0.0, 3.0};
		int tried;
		foreach (float o : offs)
		{
			foreach (float b : backs)
			{
				vector p = DCO_Util.Surface(myPos + lat * o - toT * b);
				tried++;
				if (vector.DistanceXZ(p, myPos) > maxMove || !LaneFree(me, p, tgt))
					continue;
				ECharacterStance stance = ECharacterStance.PRONE;
				if (!FireLine(p + Vector(0, 0.6, 0), aim, me, 6.0))
				{
					if (!FireLine(p + Vector(0, 1.2, 0), aim, me, 6.0))
						continue;
					stance = ECharacterStance.CROUCH;
				}
				SpecMove(u, p, tgt, false, stance, vector.Distance(myPos, p) / 3.0 + 3.0, role + "_clear_lane");
				DCO_Perf.Count("spec:mg_clear_lane");
				DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("spec_mg_clear_lane unit=%1 role=%2 result=move blocker=%3 reason=%4 shift=%5 to=%6 stance=%7 tried=%8", DCO_Log.IdUnit(me), role, DCO_Log.IdUnit(blocker), DCO_Util.LaneReason(lr), Math.Round(vector.DistanceXZ(myPos, p)), DCO_Log.Pos(p), typename.EnumToString(ECharacterStance, stance), tried));
				return true;
			}
		}
		DCO_Perf.Count("spec:mg_clear_lane_none");
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("spec_mg_clear_lane unit=%1 role=%2 result=none blocker=%3 reason=%4 tried=%5", DCO_Log.IdUnit(me), role, DCO_Log.IdUnit(blocker), DCO_Util.LaneReason(lr), tried));
		return false;
	}

	protected static bool TickMGUnseen(SCR_AIUtilityComponent u, DCO_SpecState st, float now)
	{
		AIGroup grp = u.GetAIAgent().GetParentGroup();
		vector est;
		float rad;
		int cnt;
		if (!grp)
			return false;
		string estSrc = "unseen";
		float conf = 1.0;
		IEntity me = u.m_OwnerEntity;
		vector myPos = me.GetOrigin();
		DCO_Designation dz = DCO_Designate.Best(grp, 6, myPos, now, 8000.0, 40.0, 600.0);
		if (dz)
		{
			est = dz.m_vPos;
			rad = DCO_InfoQ.SeenRadius((now - dz.m_fAt_ms) / 1000.0);
			cnt = 1;
			estSrc = "designated";
			DCO_Designate.Note(dz, me, DCO_Roles.MGRole(DCO_Roles.Get(u)), now);
		}
		else if (DCO_FireEstimate.GetQ(grp, now, 25000.0, est, rad, cnt, conf, estSrc))
		{
		}
		else
		{
			if (!GroupKnownEnemyQ(u, grp, est, rad))
				return false;
			cnt = 0;
			estSrc = "group_target";
		}
		float d = vector.DistanceXZ(myPos, est);
		if (d < 40.0 || d > 600.0)
			return false;
		if (u.m_CombatComponent.GetCombatMode() == EAIGroupCombatMode.HOLD_FIRE || !SortieOwnerOk(DCO_CtrlLock.OwnerOf(u.GetAIAgent())))
			return false;
		int roles = DCO_Roles.Get(u);
		bool ar = (roles & 64) != 0;
		string role = DCO_Roles.MGRole(roles);
		IEntity leader = grp.GetLeaderEntity();
		if (!st.m_bBaseOfFire || (st.m_bBofUnseen && vector.DistanceXZ(st.m_vEstUsed, est) > 50.0))
		{
			if (st.m_bBofUnseen && st.m_bBaseOfFire)
				Rapid(u, st, now, 30000.0, "new_shooter");
			st.m_bBaseOfFire = true;
			st.m_bBofUnseen = true;
			st.m_fBofAt_ms = now;
			st.m_vEstUsed = est;
			st.m_fNextSupp_ms = now + 2500.0;
			if (SCR_CoverManagerComponent.IsEntityInsideBuilding(me))
			{
				DCO_Perf.Count("spec:mg_bof_unseen_indoor");
				DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("spec_mg_base_of_fire unit=%1 role=%2 src=" + estSrc + " result=stay_indoor est_dist=%3 radius=%4 reports=%5", DCO_Log.IdUnit(me), role, Math.Round(d), Math.Round(rad), cnt));
				return true;
			}
			vector lp = myPos;
			if (leader && !ar)
				lp = leader.GetOrigin();
			vector axis = Flat(est - lp);
			vector side = Vector(axis[2], 0, -axis[0]);
			if (vector.Dot(side, myPos - lp) < 0)
				side = -side;
			ECharacterStance stance;
			int tried;
			if (!ar)
				StartEval(u, st, now, est, lp, "contact_" + estSrc);
			float maxMove = 30.0;
			if (ar)
				maxMove = 15.0;
			vector pos = PickFireSpot(me, est, lp, side, axis, stance, tried, maxMove, Math.Max(4.0, rad));
			if (pos == vector.Zero)
			{
				if (DCO_GunnerBehavior.Get(u))
					SpecMove(u, myPos, est, true, ECharacterStance.PRONE, 8.0, role + "_bof_cover", true);
				else
					CombatMove(u, myPos, est, true, ECharacterStance.PRONE, 8.0);
				DCO_Perf.Count("spec:mg_bof_unseen");
				DCO_Perf.Count("spec:mg_bof_unseen_cover");
				DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("spec_mg_base_of_fire unit=%1 role=%2 src=" + estSrc + " result=cover_fallback tried=%3 est_dist=%4 radius=%5 reports=%6", DCO_Log.IdUnit(me), role, tried, Math.Round(d), Math.Round(rad), cnt));
				return true;
			}
			SpecMove(u, pos, est, true, stance, vector.Distance(myPos, pos) / 3.0 + 3.0, role + "_bof_unseen", true);
			st.m_vSpot = pos;
			st.m_fSpotAt_ms = now + 5000.0;
			DCO_Perf.Count("spec:mg_bof_unseen");
			DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("spec_mg_base_of_fire unit=%1 role=%2 src=" + estSrc + " result=move pos=%3 move_dist=%4 est_dist=%5 radius=%6 reports=%7 stance=%8 tried=%9", DCO_Log.IdUnit(me), role, DCO_Log.Pos(pos), Math.Round(vector.Distance(myPos, pos)), Math.Round(d), Math.Round(rad), cnt, typename.EnumToString(ECharacterStance, stance), tried));
			return true;
		}
		DCO_GunnerBehavior wgb = DCO_GunnerBehavior.Get(u);
		if (now < st.m_fNextSupp_ms)
		{
			if (wgb)
				wgb.Watch(est, 4.0, "wait_supp");
			return true;
		}
		if (Busy(u))
			return true;
		if (!MGReady(u))
			return true;
		if (ClearLane(u, st, now, est, role))
			return true;
		IEntity blocker;
		int lr = DCO_Util.FireLaneCheck(me, est, 1.5, blocker);
		if (lr != 0)
		{
			DCO_Perf.Count("spec:mg_unseen_lane_block");
			if (DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG))
				DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(me), string.Format("spec_mg_suppress_unseen unit=%1 role=%2 result=lane_%3 blocker=%4 est_dist=%5", DCO_Log.IdUnit(me), role, DCO_Util.LaneReason(lr), DCO_Log.IdUnit(blocker), Math.Round(d)));
			return true;
		}
		if (TraverseFire(u, st, now, est, rad, ar, role, estSrc, cnt))
			DCO_Perf.Count("spec:" + role + "_suppress_unseen");
		else if (wgb)
			wgb.Watch(est, 4.0, "traverse_pause");
		UpdateAssistant(u, st, grp, leader, est);
		return true;
	}

	protected static vector PickFireSpot(IEntity me, vector enemyPos, vector anchor, vector side, vector axis, out ECharacterStance stance, out int tried, float maxMove = 30.0, float tol = 4.0)
	{
		tried = 0;
		stance = ECharacterStance.PRONE;
		vector tgt = enemyPos + Vector(0, 1.0, 0);
		array<float> sides = {18.0, 12.0, 25.0, 8.0, -12.0};
		array<float> backs = {8.0, 3.0, 12.0};
		foreach (float s : sides)
		{
			foreach (float b : backs)
			{
				vector p = DCO_Util.Surface(anchor + side * s - axis * b);
				tried++;
				if (vector.DistanceXZ(p, me.GetOrigin()) > maxMove || FriendlyAhead(p, enemyPos, me) || !LaneFree(me, p, enemyPos))
					continue;
				if (FireLine(p + Vector(0, 0.6, 0), tgt, me, tol))
				{
					stance = ECharacterStance.PRONE;
					return p;
				}
				if (FireLine(p + Vector(0, 1.2, 0), tgt, me, tol))
				{
					stance = ECharacterStance.CROUCH;
					return p;
				}
				if (tried >= 9)
					return vector.Zero;
			}
		}
		return vector.Zero;
	}

	protected static vector PickWideFireSpot(IEntity me, vector enemyPos, vector anchor, vector side, vector axis, out ECharacterStance stance, out int tried)
	{
		stance = ECharacterStance.PRONE;
		vector tgt = enemyPos + Vector(0, 1.0, 0);
		array<float> sides = {40.0, -40.0, 60.0, -60.0, 80.0, -80.0};
		array<float> fwds = {0.0, 15.0};
		foreach (float s : sides)
		{
			foreach (float f : fwds)
			{
				vector p = DCO_Util.Surface(anchor + side * s + axis * f);
				tried++;
				if (vector.DistanceXZ(p, me.GetOrigin()) > 100.0 || vector.DistanceXZ(p, enemyPos) < 80.0 || FriendlyAhead(p, enemyPos, me))
					continue;
				if (FireLine(p + Vector(0, 0.6, 0), tgt, me))
				{
					stance = ECharacterStance.PRONE;
					return p;
				}
				if (FireLine(p + Vector(0, 1.2, 0), tgt, me))
				{
					stance = ECharacterStance.CROUCH;
					return p;
				}
			}
		}
		return vector.Zero;
	}

	protected static void TickMGInRole(SCR_AIUtilityComponent u, DCO_SpecState st, float now, int roleOwner, IEntity me, vector myPos, vector enemyPos, AIGroup grp)
	{
		if (!st.m_bBaseOfFire)
		{
			st.m_bBaseOfFire = true;
			st.m_fBofAt_ms = now;
			DCO_Perf.Count("spec:mg_role_hold_" + DCO_CtrlLock.OwnerName(roleOwner));
			DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("spec_mg_role unit=%1 role=%2 action=hold_role_position enemy_dist=%3", DCO_Log.IdUnit(me), DCO_CtrlLock.OwnerName(roleOwner), Math.Round(vector.DistanceXZ(myPos, enemyPos))));
		}
		if (roleOwner != DCO_ECtrlOwner.FM_FIX || now < st.m_fSpotAt_ms)
			return;
		SCR_CharacterDamageManagerComponent dm = SCR_CharacterDamageManagerComponent.Cast(me.FindComponent(SCR_CharacterDamageManagerComponent));
		bool hit = dm && dm.DCO_HitsWithin(4.0) > 0;
		bool accurate = st.m_fNearImpact_ms > 0 && now - st.m_fNearImpact_ms < 3000.0;
		if (!hit && !accurate)
			return;
		vector axis = Flat(enemyPos - myPos);
		vector side = Vector(axis[2], 0, -axis[0]);
		if (Math.RandomFloat01() < 0.5)
			side = -side;
		vector tgt = enemyPos + Vector(0, 1.0, 0);
		array<float> shifts = {8.0, 12.0, -8.0, -12.0};
		vector spot = vector.Zero;
		ECharacterStance stance = ECharacterStance.PRONE;
		foreach (float s : shifts)
		{
			vector p = DCO_Util.Surface(myPos + side * s - axis * 1.0);
			if (FriendlyAhead(p, enemyPos, me) || DCO_Util.HasFriendlyNear(me, p, 2.5, "q:DCO_SpecFireSpot"))
				continue;
			if (FireLine(p + Vector(0, 0.6, 0), tgt, me))
			{
				spot = p;
				break;
			}
			if (FireLine(p + Vector(0, 1.2, 0), tgt, me))
			{
				spot = p;
				stance = ECharacterStance.CROUCH;
				break;
			}
		}
		st.m_fSpotAt_ms = now + 8000.0;
		st.m_fNearImpact_ms = 0;
		string why = "accurate_fire";
		if (hit)
			why = "hit";
		if (spot == vector.Zero)
		{
			DCO_Perf.Count("spec:mg_role_shift_none");
			DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("spec_mg_role unit=%1 role=FM_FIX action=shift_none why=%2", DCO_Log.IdUnit(me), why));
			return;
		}
		DCO_RelocateBehavior.Start(u, spot, enemyPos, false, stance, 6.0, "mg_role_shift");
		DCO_Perf.Count("spec:mg_role_shift");
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("spec_mg_role unit=%1 role=FM_FIX action=shift_on_line why=%2 to=%3 shift=%4 stance=%5", DCO_Log.IdUnit(me), why, DCO_Log.Pos(spot), Math.Round(vector.DistanceXZ(myPos, spot)), typename.EnumToString(ECharacterStance, stance)));
	}

	protected static bool FriendlyAhead(vector p, vector enemyPos, IEntity me)
	{
		vector dir = Flat(enemyPos - p);
		if (DCO_Util.HasFriendlyNear(me, p + dir * 10.0, 4.0, "q:DCO_SpecFireSpot") || DCO_Util.HasFriendlyNear(me, p + dir * 22.0, 5.0, "q:DCO_SpecFireSpot"))
		{
			DCO_Perf.Count("spec:mg_spot_friendly_ahead");
			return true;
		}
		return false;
	}

	protected static bool FireLine(vector from, vector tgt, IEntity me, float tol = 4.0)
	{
		float len = vector.Distance(from, tgt);
		float frac = DCO_Util.TraceFrac(from, tgt, me, TraceFlags.WORLD | TraceFlags.ENTS, EPhysicsLayerDefs.Projectile, "t:DCO_SpecFireSpot");
		return frac * len >= len - tol;
	}

	protected static void UpdateAssistant(SCR_AIUtilityComponent u, DCO_SpecState st, AIGroup grp, IEntity leader, vector enemyPos)
	{
		if (!grp)
			return;
		IEntity me = u.m_OwnerEntity;
		if (!st.m_Assistant || !DCO_Util.IsAlive(st.m_Assistant.GetControlledEntity()) || st.m_Assistant.GetParentGroup() != grp)
		{
			st.m_Assistant = null;
			float bestSq = 2500;
			array<AIAgent> agents = {};
			grp.GetAgents(agents);
			foreach (AIAgent a : agents)
			{
				SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
				if (!ca || !ca.m_UtilityComponent)
					continue;
				IEntity e = ca.GetControlledEntity();
				if (!e || e == me || e == leader || !DCO_Util.IsAlive(e) || DCO_Util.IsRealMedic(e))
					continue;
				if ((DCO_Roles.Get(ca.m_UtilityComponent) & 23) != 0)
					continue;
				float dSq = vector.DistanceSq(e.GetOrigin(), me.GetOrigin());
				if (dSq < bestSq)
				{
					bestSq = dSq;
					st.m_Assistant = ca;
				}
			}
		}

		if (!st.m_Assistant)
			return;
		IEntity ae = st.m_Assistant.GetControlledEntity();
		SCR_AIUtilityComponent au = st.m_Assistant.m_UtilityComponent;
		if (!ae || !au || vector.DistanceXZ(ae.GetOrigin(), me.GetOrigin()) < 8.0)
			return;

		vector back = Flat(me.GetOrigin() - enemyPos);
		vector pos = DCO_Util.Surface(me.GetOrigin() + back * 3.0);
		if (InCombat(au))
		{
			DCO_RelocateBehavior cur = DCO_RelocateBehavior.Cast(au.FindActionOfType(DCO_RelocateBehavior));
			if (cur && !cur.DCO_IsDone() && cur.DCO_Why() == "mg_assistant" && cur.DCO_AgeMs() < 1000.0 * DCO_Stress.T(au, "spec.assist_commit_s", 6.0))
			{
				DCO_Perf.Count("spec:assist_commit_hold");
				return;
			}
			if (!au.m_CombatMoveState.IsExecutingRequest())
				DCO_RelocateBehavior.Start(au, pos, enemyPos, false, ECharacterStance.CROUCH, 6.0, "mg_assistant");
		}
		else
		{
			MoveIndividually(au, pos);
		}
	}

	protected static void TickAT(SCR_AIUtilityComponent u, DCO_SpecState st, float now)
	{
		if (now >= st.m_fNextWorthy_ms)
		{
			st.m_fNextWorthy_ms = now + 2000.0;
			st.m_bLauncherAllowed = HasLauncherTarget(u);
		}

		IEntity me = u.m_OwnerEntity;
		vector myPos = me.GetOrigin();

		if (DCO_GunnerBehavior.On(u))
		{
			IEntity jobArmor = ArmorFor(u, 400.0, now);
			BaseTarget jt = u.m_CombatComponent.GetCurrentTarget();
			bool launcherJob = st.m_bLauncherAllowed && jt && jt.GetTargetEntity() && jt.GetTimeSinceSeen() < 10.0 && DCO_RpgUnseen.Rockets(me) > 0;
			if (jobArmor || launcherJob || st.m_iSortie == 2)
			{
				vector jobPos = st.m_vSortieGoal;
				string jobWhy = "hunt";
				if (jobArmor)
				{
					jobPos = jobArmor.GetOrigin();
					jobWhy = "armor";
				}
				else if (launcherJob)
				{
					jobPos = jt.GetLastSeenPosition();
					jobWhy = "launcher_target";
				}
				DCO_GunnerBehavior atb = DCO_GunnerBehavior.Ensure(u, "at", jobPos, now);
				if (atb)
					atb.SetHold(jobArmor != null || launcherJob, jobWhy);
			}
			else
			{
				DCO_GunnerBehavior atb2 = DCO_GunnerBehavior.Get(u);
				if (atb2)
					atb2.SetHold(false, "no_launcher_job");
			}
		}

		if (now >= st.m_fNextHunt_ms)
		{
			st.m_fNextHunt_ms = now + 5000.0;
			TickATHunt(u, st, now);
		}

		if (now >= st.m_fNextFlank_ms)
		{
			st.m_fNextFlank_ms = now + 5000.0;
			IEntity armor = ArmorFor(u, 400.0, now);
			if (armor)
			{
				vector vp = armor.GetOrigin();
				vector fwd = Flat(armor.GetWorldTransformAxis(2));
				vector rel = myPos - vp;
				float dist = vector.DistanceXZ(myPos, vp);
				bool front = vector.Dot(fwd, Flat(rel)) > 0.5;
				if (front || dist < 100.0)
				{
					vector side = Vector(fwd[2], 0, -fwd[0]);
					float sgn = 1;
					if (vector.Dot(side, rel) < 0)
						sgn = -1;
					bool paired;
					sgn = DCO_AntiArmor.ClaimSide(armor, me, sgn, now, paired);
					side = side * sgn;
					float back = 50.0;
					if (paired)
						back = 120.0;
					if (st.m_iSortie == 0)
						StartSortie(u, st, 2, "armor_flank", armor, now);
					if (st.m_iSortie == 2 && armor == st.m_SortieTarget)
						st.m_fSortieSeen_ms = now;
					vector point = DCO_Util.Surface(vp + side * Math.Clamp(dist, 150, 250) - fwd * back);
					SpecMove(u, point, vp, true, ECharacterStance.CROUCH, vector.Distance(myPos, point) / 3.5, "at_flank");
					st.m_fNextFlank_ms = now + 30000.0;
					CoverWithSmoke(u, vp);
					DCO_Perf.Count("spec:at_flank");
					DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("spec_at_flank unit=%1 armor=%2 armor_dist=%3 to=%4 front=%5 paired=%6", DCO_Log.IdUnit(me), DCO_Log.IdUnit(armor), Math.Round(dist), DCO_Log.Pos(point), DCO_Util.B01(front), DCO_Util.B01(paired)));
				}
			}
		}

		BaseWeaponComponent wc;
		int muzzle;
		u.m_CombatComponent.GetSelectedWeapon(wc, muzzle);
		BaseTarget t = u.m_CombatComponent.GetCurrentTarget();
		if (!wc || wc.GetWeaponType() != EWeaponType.WT_ROCKETLAUNCHER || !t || t.GetTimeSinceSeen() > 3.0 || now < st.m_fNextShift_ms)
			return;

		vector behind = Flat(myPos - t.GetLastSeenPosition());
		bool blocked = false;
		AIGroup grp = u.GetAIAgent().GetParentGroup();
		if (grp)
		{
			array<AIAgent> agents = {};
			grp.GetAgents(agents);
			foreach (AIAgent a : agents)
			{
				IEntity e = a.GetControlledEntity();
				if (!e || e == me)
					continue;
				vector d = e.GetOrigin() - myPos;
				if (d.LengthSq() < 25.0 && vector.Dot(Flat(d), behind) > 0.7)
				{
					blocked = true;
					break;
				}
			}
		}
		vector eye = myPos + Vector(0, 1.2, 0);
		if (!blocked && DCO_Util.TraceFrac(eye, eye + behind * 2.0, me, -1, -1, "t:DCO_Backblast") < 0.99)
			blocked = true;
		if (!blocked)
			return;

		vector lateral = Vector(behind[2], 0, -behind[0]);
		if (Math.RandomFloat01() < 0.5)
			lateral = -lateral;
		ChimeraCharacter ch = ChimeraCharacter.Cast(me);
		ECharacterStance stance = ECharacterStance.CROUCH;
		if (ch && ch.GetCharacterController())
			stance = ch.GetCharacterController().GetStance();
		SpecMove(u, DCO_Util.Surface(myPos + lateral * 3.0), t.GetLastSeenPosition(), false, stance, 3.0, "at_backblast", true);
		st.m_fNextShift_ms = now + 4000.0;
		DCO_Perf.Count("spec:at_backblast_shift");
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("spec_at_backblast_shift unit=%1 tgt_dist=%2", DCO_Log.IdUnit(me), Math.Round(vector.Distance(myPos, t.GetLastSeenPosition()))));
	}

	protected static void TickATHunt(SCR_AIUtilityComponent u, DCO_SpecState st, float now)
	{
		IEntity me = u.m_OwnerEntity;
		if (st.m_iSortie != 0 && st.m_iSortie != 2)
			return;
		if (st.m_iSortie == 0 && (!SortieLeader(u) || DCO_RpgUnseen.Rockets(me) <= 0))
			return;
		vector myPos = me.GetOrigin();
		vector vpos;
		float seenAgo;
		IEntity veh = KnownVehicle(u, 800.0, st.m_SortieTarget, vpos, seenAgo);
		if (!veh)
			return;
		float dist = vector.DistanceXZ(myPos, vpos);
		if (st.m_iSortie == 0)
		{
			if (seenAgo > 30.0 || dist < 200.0)
				return;
			if (!StartSortie(u, st, 2, "vehicle_known", veh, now))
				return;
		}
		if (veh != st.m_SortieTarget)
			return;
		if (seenAgo < 5.0)
			st.m_fSortieSeen_ms = now;
		float standoff = st.m_fSortieStandoff;
		if (standoff <= 0)
			standoff = 170.0;
		bool closer;
		if (st.m_vSortieGoal != vector.Zero && seenAgo > 20.0 && standoff > 90.0 && vector.DistanceXZ(myPos, st.m_vSortieGoal) < 15.0)
		{
			standoff = Math.Max(standoff - 40.0, 90.0);
			st.m_fSortieStandoff = standoff;
			closer = true;
			DCO_Perf.Count("spec:at_hunt_close");
			DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("spec_at_hunt_close unit=%1 target=%2 standoff=%3 seen_ago_s=%4 veh_dist=%5", DCO_Log.IdUnit(me), DCO_Log.IdUnit(veh), standoff, Math.Round(seenAgo), Math.Round(dist)));
		}
		if (dist < standoff + 20.0 && !closer)
			return;

		vector toMe = Flat(myPos - vpos);
		vector side = Vector(toMe[2], 0, -toMe[0]);
		if (st.m_fSortieSide == 0)
		{
			float pref = 1;
			IEntity hl = SortieLeader(u);
			if (hl && vector.Dot(side, me.GetOrigin() - hl.GetOrigin()) < 0)
				pref = -1;
			bool paired;
			st.m_fSortieSide = DCO_AntiArmor.ClaimSide(veh, me, pref, now, paired);
			if (paired)
				DCO_Perf.Count("spec:at_hunt_paired");
		}
		side = side * st.m_fSortieSide;
		vector goal = DCO_Util.Surface(vpos + toMe * standoff + side * (40.0 * standoff / 170.0));
		vector snapped;
		if (DCO_FlushDrill.Reachable(goal, snapped))
			goal = snapped;
		else
			DCO_Perf.Count("spec:at_hunt_goal_unsnapped");
		if (!closer && st.m_vSortieGoal != vector.Zero && vector.DistanceXZ(goal, st.m_vSortieGoal) < 30.0)
			return;
		st.m_vSortieGoal = goal;
		SortieMove(u, goal);
		DCO_Perf.Count("spec:at_hunt_move");
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("spec_at_hunt unit=%1 target=%2 armored=%3 veh_dist=%4 seen_ago_s=%5 goal=%6 move_dist=%7", DCO_Log.IdUnit(me), DCO_Log.IdUnit(veh), DCO_Util.B01(DCO_VehicleCombat.IsArmored(veh)), Math.Round(dist), Math.Round(seenAgo), DCO_Log.Pos(goal), Math.Round(vector.DistanceXZ(myPos, goal))) + " side=" + st.m_fSortieSide);
	}

	protected static void SortieMove(SCR_AIUtilityComponent u, vector pos)
	{
		if (u.m_AIInfo && u.m_AIInfo.HasUnitState(EUnitState.IN_VEHICLE))
			return;
		DCO_GunnerBehavior gb = DCO_GunnerBehavior.Get(u);
		if (gb)
		{
			float d = vector.DistanceXZ(u.m_OwnerEntity.GetOrigin(), pos);
			gb.MoveTo(pos, pos, false, ECharacterStance.CROUCH, d / 3.5 + 5.0, "sortie_move", false);
			return;
		}
		SCR_AIMoveIndividuallyBehavior existing = SCR_AIMoveIndividuallyBehavior.Cast(u.FindActionOfType(SCR_AIMoveIndividuallyBehavior));
		if (existing)
			existing.Fail();
		u.AddAction(new SCR_AIMoveIndividuallyBehavior(u, null, pos, 75.0, radius: 5.0));
		AICharacterMovementComponent move = AICharacterMovementComponent.Cast(u.GetAIAgent().GetMovementComponent());
		if (move)
			move.SetMovementTypeWanted(EMovementType.RUN);
	}

	protected static IEntity KnownVehicle(SCR_AIUtilityComponent u, float range, IEntity prefer, out vector pos, out float seenAgo)
	{
		IEntity best;
		float bestScore = float.MAX;
		vector myPos = u.m_OwnerEntity.GetOrigin();
		if (u.m_PerceptionComponent)
		{
			array<BaseTarget> targets = {};
			u.m_PerceptionComponent.GetTargetsList(targets, ETargetCategory.ENEMY);
			foreach (BaseTarget t : targets)
			{
				if (!t || !t.GetTargetEntity() || t.GetTimeSinceSeen() > 60.0)
					continue;
				IEntity veh = HuntVehicle(t.GetTargetEntity());
				if (!veh)
					continue;
				vector p = t.GetLastSeenPosition();
				float d = vector.DistanceXZ(myPos, p);
				if (d > range)
					continue;
				float score = HuntScore(veh, d, prefer);
				if (score < bestScore)
				{
					bestScore = score;
					best = veh;
					pos = p;
					seenAgo = t.GetTimeSinceSeen();
				}
			}
		}
		AIGroup grp = u.GetAIAgent().GetParentGroup();
		SCR_AIGroupUtilityComponent gu;
		if (grp)
			gu = SCR_AIGroupUtilityComponent.Cast(grp.FindComponent(SCR_AIGroupUtilityComponent));
		PerceptionManager pm = GetGame().GetPerceptionManager();
		if (gu && gu.m_Perception && pm)
		{
			float pmNow = pm.GetTime();
			foreach (SCR_AITargetInfo ti : gu.m_Perception.m_aTargets)
			{
				if (!ti || !ti.m_Entity || pmNow - ti.m_fTimestamp > 60.0)
					continue;
				IEntity gveh = HuntVehicle(ti.m_Entity);
				if (!gveh)
					continue;
				float gd = vector.DistanceXZ(myPos, ti.m_vWorldPos);
				if (gd > range)
					continue;
				float gscore = HuntScore(gveh, gd, prefer);
				if (gscore < bestScore)
				{
					bestScore = gscore;
					best = gveh;
					pos = ti.m_vWorldPos;
					seenAgo = pmNow - ti.m_fTimestamp;
				}
			}
		}
		return best;
	}

	protected static IEntity HuntVehicle(IEntity e)
	{
		IEntity veh = DCO_VehicleCombat.TargetVehicle(e);
		if (!Vehicle.Cast(veh) || VehicleDead(veh))
			return null;
		return veh;
	}

	protected static float HuntScore(IEntity veh, float d, IEntity prefer)
	{
		float score = d;
		if (DCO_VehicleCombat.IsArmored(veh))
			score -= 200.0;
		if (veh == prefer)
			score -= 400.0;
		return score;
	}

	protected static bool HasLauncherTarget(SCR_AIUtilityComponent u)
	{
		if (!u.m_PerceptionComponent)
			return true;
		array<BaseTarget> targets = {};
		u.m_PerceptionComponent.GetTargetsList(targets, ETargetCategory.ENEMY);
		int buildingChecks = 0;
		foreach (BaseTarget t : targets)
		{
			if (!t || !t.GetTargetEntity() || t.GetTimeSinceSeen() > 10.0)
				continue;
			IEntity e = t.GetTargetEntity();
			ChimeraCharacter tc = ChimeraCharacter.Cast(e);
			if (!tc)
				return true;
			if (tc.IsInVehicle() && t.GetTimeSinceSeen() < 5.0)
			{
				DCO_Perf.Count("spec:at_target_crew");
				if (DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG))
					DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(u.m_OwnerEntity), string.Format("spec_at_launcher_ok unit=%1 tgt=%2 why=crew_or_static dist=%3", DCO_Log.IdUnit(u.m_OwnerEntity), DCO_Log.IdUnit(tc), Math.Round(vector.Distance(u.m_OwnerEntity.GetOrigin(), tc.GetOrigin()))));
				return true;
			}
			if (t.GetTimeSinceSeen() < 5.0 && buildingChecks < 4)
			{
				buildingChecks++;
				if (SCR_CoverManagerComponent.IsEntityInsideBuilding(e))
					return true;
			}
		}
		return false;
	}

	protected static IEntity ArmorFor(SCR_AIUtilityComponent u, float range, float now)
	{
		IEntity a = NearestArmor(u, range);
		if (a)
			return a;
		DCO_Designation dz = DCO_Designate.Best(u.GetAIAgent().GetParentGroup(), 1, u.m_OwnerEntity.GetOrigin(), now, 15000.0, 0.0, range);
		if (!dz || !dz.m_Target || VehicleDead(dz.m_Target))
			return null;
		DCO_Designate.Note(dz, u.m_OwnerEntity, "at", now);
		return dz.m_Target;
	}

	protected static IEntity NearestArmor(SCR_AIUtilityComponent u, float range)
	{
		if (!u.m_PerceptionComponent)
			return null;
		array<BaseTarget> targets = {};
		u.m_PerceptionComponent.GetTargetsList(targets, ETargetCategory.ENEMY);
		IEntity best;
		float bestSq = range * range;
		vector myPos = u.m_OwnerEntity.GetOrigin();
		foreach (BaseTarget t : targets)
		{
			if (!t || t.GetTimeSinceSeen() > 20.0)
				continue;
			IEntity e = t.GetTargetEntity();
			if (!e || ChimeraCharacter.Cast(e) || !DCO_VehicleCombat.IsArmored(e))
				continue;
			float dSq = vector.DistanceSqXZ(myPos, e.GetOrigin());
			if (dSq < bestSq)
			{
				bestSq = dSq;
				best = e;
			}
		}
		return best;
	}

	protected static void CoverWithSmoke(SCR_AIUtilityComponent u, vector threatPos)
	{
		AIGroup grp = u.GetAIAgent().GetParentGroup();
		if (!grp)
			return;
		array<AIAgent> agents = {};
		grp.GetAgents(agents);
		SCR_AIUtilityComponent best;
		float bestSq = 2500;
		foreach (AIAgent a : agents)
		{
			SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
			if (!ca || !ca.m_UtilityComponent || ca.m_UtilityComponent == u || !ca.GetControlledEntity())
				continue;
			float dSq = vector.DistanceSq(ca.GetControlledEntity().GetOrigin(), u.m_OwnerEntity.GetOrigin());
			if (dSq < bestSq)
			{
				bestSq = dSq;
				best = ca.m_UtilityComponent;
			}
		}
		if (best)
			DCO_SmokeUtility.TryDeploySmokeForRetreat(best, threatPos, 1.0);
	}

	protected static void LeaderFacing(SCR_AIUtilityComponent u, out vector watch)
	{
		AIGroup grp = u.GetAIAgent().GetParentGroup();
		if (!grp || !grp.GetLeaderEntity() || watch != vector.Zero)
			return;
		IEntity leader = grp.GetLeaderEntity();
		watch = leader.GetOrigin() + Flat(leader.GetWorldTransformAxis(2)) * 300.0;
	}

	protected static void TickGL(SCR_AIUtilityComponent u, DCO_SpecState st, float now)
	{
		if (now < st.m_fNextGL_ms)
			return;
		st.m_fNextGL_ms = now + 2000.0;
		if (u.m_CombatComponent.GetCombatMode() == EAIGroupCombatMode.HOLD_FIRE || !SortieOwnerOk(DCO_CtrlLock.OwnerOf(u.GetAIAgent())))
			return;
		bool glCombat = InCombat(u);
		if (glCombat)
		{
			BaseTarget glT = u.m_CombatComponent.GetCurrentTarget();
			if (glT && glT.GetTimeSinceSeen() < 2.0)
				return;
			if (!DCO_Designate.Best(u.GetAIAgent().GetParentGroup(), 2, u.m_OwnerEntity.GetOrigin(), now, 8000.0, 35.0, 300.0))
				return;
		}
		DCO_GunnerBehavior cur = DCO_GunnerBehavior.Get(u);
		if (cur && cur.IsMoving())
			return;
		AIGroup grp = u.GetAIAgent().GetParentGroup();
		if (!grp)
			return;
		vector est;
		float rad;
		int cnt;
		float conf = 1.0;
		string src = "unseen";
		IEntity me = u.m_OwnerEntity;
		int glKinds = 6;
		if (glCombat)
			glKinds = 2;
		DCO_Designation dz = DCO_Designate.Best(grp, glKinds, me.GetOrigin(), now, 8000.0, 35.0, 300.0);
		if (dz)
		{
			est = dz.m_vPos;
			rad = DCO_InfoQ.SeenRadius((now - dz.m_fAt_ms) / 1000.0);
			src = "designated";
			if (glCombat)
				src = "designated_contact";
		}
		else if (DCO_FireEstimate.GetQ(grp, now, 25000.0, est, rad, cnt, conf, src))
		{
			if (DCO_InfoQ.On() && conf < 0.4)
			{
				DCO_Perf.Count("spec:gl_lob_low_conf");
				return;
			}
		}
		else
		{
			if (!GroupKnownEnemyQ(u, grp, est, rad))
				return;
			src = "group_target";
		}
		float d = vector.DistanceXZ(me.GetOrigin(), est);
		if (d < 35.0 || d > 300.0)
			return;
		BaseWeaponComponent wc;
		int muzzle;
		u.m_CombatComponent.GetSelectedWeapon(wc, muzzle);
		if (!wc || !DCO_UGLUtility.HasUGL(wc))
			return;
		if (!DCO_UGLUtility.ShouldUseGL(u, wc, est))
			return;
		DCO_GunnerBehavior gb = DCO_GunnerBehavior.Ensure(u, "gl", est, now);
		if (!gb)
			return;
		gb.Lob(est, 8.0, src);
		if (dz)
			DCO_Designate.Note(dz, me, "gl", now);
		DCO_Perf.Count("spec:gl_lob_" + src);
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("spec_gl_lob unit=%1 src=%2 target=%3 dist=%4 radius=%5 reports=%6 conf=%7", DCO_Log.IdUnit(me), src, DCO_Log.Pos(est), Math.Round(d), Math.Round(rad), cnt, conf.ToString(-1, 2)));
	}

	protected static void TickMarksman(SCR_AIUtilityComponent u, DCO_SpecState st, float now)
	{
		IEntity me = u.m_OwnerEntity;
		vector myPos = me.GetOrigin();
		BaseTarget t = u.m_CombatComponent.GetCurrentTarget();
		bool combat = InCombat(u);

		DCO_GunnerBehavior mmb;
		if (DCO_GunnerBehavior.On(u) && (combat || st.m_iSortie == 4))
		{
			vector mmWatch = st.m_vOverwatch;
			if (t && t.GetTargetEntity())
				mmWatch = t.GetLastSeenPosition();
			else
				LeaderFacing(u, mmWatch);
			mmb = DCO_GunnerBehavior.Ensure(u, "mm", mmWatch, now);
			if (mmb)
				mmb.SetHold(st.m_iSortie == 4 && st.m_vOverwatch != vector.Zero && vector.DistanceXZ(myPos, st.m_vOverwatch) <= 8.0, "overwatch");
		}

		BaseWeaponComponent wc;
		int muzzle;
		u.m_CombatComponent.GetSelectedWeapon(wc, muzzle);
		if (combat && wc && wc.GetCurrentMagazine())
		{
			int ammo = wc.GetCurrentMagazine().GetAmmoCount();
			if (st.m_iSpotAmmo < 0 || ammo > st.m_iSpotAmmo)
			{
				st.m_iSpotAmmo = ammo;
				st.m_iScootNeed = 3;
				if (mmb)
					st.m_iScootNeed = Math.RandomInt(2, 4);
			}
			if (st.m_iSpotAmmo - ammo >= st.m_iScootNeed)
			{
				vector toEnemy = Flat(t.GetLastSeenPosition() - myPos);
				vector lateral = Vector(toEnemy[2], 0, -toEnemy[0]);
				if (Math.RandomFloat01() < 0.5)
					lateral = -lateral;
				vector newSpot = DCO_Util.Surface(myPos + lateral * Math.RandomFloat(15, 30));
				SpecMove(u, newSpot, t.GetLastSeenPosition(), false, ECharacterStance.PRONE, 10.0, "marksman_relocate");
				if (st.m_iSortie == 4)
					st.m_vOverwatch = newSpot;
				st.m_iSpotAmmo = -1;
				DCO_Perf.Count("spec:marksman_relocate");
				DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("spec_marksman_relocate unit=%1 shots=%2 tgt_dist=%3", DCO_Log.IdUnit(me), st.m_iScootNeed, Math.Round(vector.Distance(myPos, t.GetLastSeenPosition()))));
				return;
			}
		}

		if (now < st.m_fNextOverwatch_ms)
			return;
		int mOwner = DCO_CtrlLock.OwnerOf(u.GetAIAgent());
		if (mOwner == DCO_ECtrlOwner.FM_FIX || mOwner == DCO_ECtrlOwner.FM_ASSAULT)
		{
			st.m_fNextOverwatch_ms = now + 10000.0;
			DCO_Perf.Count("spec:marksman_role_hold");
			return;
		}
		st.m_fNextOverwatch_ms = now + 10000.0;
		AIGroup grp = u.GetAIAgent().GetParentGroup();
		if (!grp || !grp.GetLeaderEntity() || grp.GetLeaderEntity() == me)
			return;
		if (combat && vector.Distance(myPos, t.GetLastSeenPosition()) < 100.0)
			return;

		IEntity leader = grp.GetLeaderEntity();
		vector lp = leader.GetOrigin();
		if (st.m_iSortie != 0 || (st.m_vOverwatch != vector.Zero && vector.DistanceXZ(lp, st.m_vOverwatch) <= 250.0))
			return;
		if (now < st.m_fSortieCooldown_ms)
			return;

		vector watch = lp + Flat(leader.GetWorldTransformAxis(2)) * 300.0;
		if (t && t.GetTargetEntity())
			watch = t.GetLastSeenPosition();
		vector spot = CMD_ReconSpotFinder.FindBestReconSpot(lp, watch, 150.0, 400.0, 10);
		if (spot == vector.Zero || !StartSortie(u, st, 4, "overwatch", null, now))
			return;
		st.m_vOverwatch = spot;
		if (combat || DCO_GunnerBehavior.Get(u))
			SpecMove(u, spot, watch, false, ECharacterStance.PRONE, vector.Distance(myPos, spot) / 3.0, "marksman_overwatch");
		else
			MoveIndividually(u, spot);
		DCO_Perf.Count("spec:marksman_overwatch");
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("spec_marksman_overwatch unit=%1 spot=%2 leader_dist=%3 combat=%4", DCO_Log.IdUnit(me), DCO_Log.Pos(spot), Math.Round(vector.DistanceXZ(spot, lp)), DCO_Util.B01(combat)));
	}
}
