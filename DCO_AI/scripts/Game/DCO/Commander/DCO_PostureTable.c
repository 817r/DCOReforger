class DCO_PostureTable
{
	static DCO_GroupTactics SB(bool quiet)
	{
		if (quiet)
			return DCO_GroupTactics.SNEAK;
		return DCO_GroupTactics.BALANCE;
	}

	static DCO_GroupTactics FixRow(bool moving, bool contact)
	{
		if (!moving)
			return DCO_GroupTactics.DEFENSIVE;
		if (contact)
			return DCO_GroupTactics.BALANCE;
		return DCO_GroupTactics.SNEAK;
	}

	static DCO_GroupTactics HoldRow(bool moving)
	{
		if (moving)
			return DCO_GroupTactics.BALANCE;
		return DCO_GroupTactics.DEFENSIVE;
	}

	static DCO_GroupTactics For(int task, int tactic, int role, int stage, int planPhase, bool moving, bool contact, bool sprung, int threatAct, bool attackObj, bool front, bool retreat, bool pursuitCons, bool quiet, out string row, out bool pair)
	{
		pair = false;
		row = "O10";
		if (tactic >= 0)
		{
			bool keepAfter = tactic == DCO_ETactic.RECON_IN_FORCE || tactic == DCO_ETactic.RAID || tactic == DCO_ETactic.BYPASS || tactic == DCO_ETactic.DELAY || tactic == DCO_ETactic.PURSUIT;
			if (stage == 2 && !keepAfter)
			{
				row = "O9";
				if (role == DCO_CommanderTactics.ROLE_FIX)
					return FixRow(moving, true);
				return DCO_GroupTactics.DEFENSIVE;
			}
			switch (tactic)
			{
				case DCO_ETactic.MOVEMENT_TO_CONTACT:
				{
					if (role == 5)
					{
						row = "T-MTC1";
						if (contact)
							return HoldRow(moving);
						return DCO_GroupTactics.BALANCE;
					}
					row = "T-MTC2";
					if (stage == 1)
						return DCO_GroupTactics.AGGRESIVE;
					return DCO_GroupTactics.BALANCE;
				}
				case DCO_ETactic.RECON_IN_FORCE:
				{
					row = "T-RIF";
					if (planPhase == 1)
						return DCO_GroupTactics.DELAY;
					return DCO_GroupTactics.BALANCE;
				}
				case DCO_ETactic.FIRE_AND_MANEUVER:
				case DCO_ETactic.FLANKING:
				{
					string tp = "T-FM";
					if (tactic == DCO_ETactic.FLANKING)
						tp = "T-FL";
					if (role == DCO_CommanderTactics.ROLE_FIX)
					{
						row = tp + "1";
						return FixRow(moving, contact);
					}
					row = tp + "2";
					if (stage == 1)
						return DCO_GroupTactics.AGGRESIVE;
					pair = true;
					return SB(quiet);
				}
				case DCO_ETactic.HAMMER_AND_ANVIL:
				{
					if (role == DCO_CommanderTactics.ROLE_BLOCK)
					{
						row = "T-HA2";
						if (sprung)
							return DCO_GroupTactics.DEFENSIVE;
						if (moving)
							return DCO_GroupTactics.SNEAK;
						return DCO_GroupTactics.AMBUSH;
					}
					row = "T-HA1";
					return DCO_GroupTactics.AGGRESIVE;
				}
				case DCO_ETactic.PINCER:
				{
					row = "T-PI";
					if (stage == 1)
						return DCO_GroupTactics.AGGRESIVE;
					pair = true;
					return SB(quiet);
				}
				case DCO_ETactic.INFILTRATION:
				{
					row = "T-IN";
					if (stage == 1 || planPhase == 1)
						return DCO_GroupTactics.AGGRESIVE;
					return DCO_GroupTactics.SNEAK;
				}
				case DCO_ETactic.SIEGE:
				{
					if (role == DCO_CommanderTactics.ROLE_BLOCK)
					{
						row = "T-SI1";
						return HoldRow(moving);
					}
					if (role == DCO_CommanderTactics.ROLE_FIX)
					{
						row = "T-SI2";
						return FixRow(moving, contact);
					}
					row = "T-SI3";
					if (task == DCO_EGroupTask.ATTACK)
						return DCO_GroupTactics.AGGRESIVE;
					return DCO_GroupTactics.DEFENSIVE;
				}
				case DCO_ETactic.BYPASS:
				{
					if (role == DCO_CommanderTactics.ROLE_FIX)
					{
						row = "T-BY2";
						return DCO_GroupTactics.DELAY;
					}
					row = "T-BY1";
					if (moving)
						return DCO_GroupTactics.SNEAK;
					return DCO_GroupTactics.BALANCE;
				}
				case DCO_ETactic.RAID:
				{
					row = "T-RA";
					if (planPhase == 1)
						return DCO_GroupTactics.EVASIVE;
					if (stage == 1)
						return DCO_GroupTactics.AGGRESIVE;
					return DCO_GroupTactics.SNEAK;
				}
				case DCO_ETactic.ATTACK_BY_FIRE:
				{
					row = "T-ABF";
					if (role == DCO_CommanderTactics.ROLE_FIX)
						return HoldRow(moving);
					return DCO_GroupTactics.DEFENSIVE;
				}
				case DCO_ETactic.PURSUIT:
				{
					row = "T-PU";
					return DCO_GroupTactics.AGGRESIVE;
				}
				case DCO_ETactic.DELAY:
				{
					row = "T-DE";
					return DCO_GroupTactics.DELAY;
				}
			}
		}

		switch (threatAct)
		{
			case CMD_EThreatAction.HOLD_DESTROY:
			case CMD_EThreatAction.PREPARE_DEFENSE:
			{
				row = "D6";
				return DCO_GroupTactics.DEFENSIVE;
			}
			case CMD_EThreatAction.COUNTERATTACK:
			{
				row = "D7";
				return DCO_GroupTactics.AGGRESIVE;
			}
			case CMD_EThreatAction.DELAY_WITHDRAW:
			{
				row = "D8";
				return DCO_GroupTactics.DELAY;
			}
		}
		if (retreat)
		{
			row = "O7";
			return DCO_GroupTactics.EVASIVE;
		}
		if (pursuitCons)
		{
			row = "T-PU";
			return HoldRow(moving);
		}

		switch (task)
		{
			case DCO_EGroupTask.ATTACK:
			{
				if (stage == 2)
				{
					row = "O9";
					return DCO_GroupTactics.DEFENSIVE;
				}
				row = "U6";
				return DCO_GroupTactics.AGGRESIVE;
			}
			case DCO_EGroupTask.FLANK:
			{
				if (stage == 2)
				{
					row = "O9";
					return DCO_GroupTactics.DEFENSIVE;
				}
				row = "T-FL2";
				if (stage == 1)
					return DCO_GroupTactics.AGGRESIVE;
				pair = true;
				return SB(quiet);
			}
			case DCO_EGroupTask.SUPPORT_BY_FIRE:
			{
				row = "T-FM1";
				return FixRow(moving, contact);
			}
			case DCO_EGroupTask.DEFEND:
			case DCO_EGroupTask.GARRISON:
			{
				row = "D1";
				return DCO_GroupTactics.DEFENSIVE;
			}
			case DCO_EGroupTask.RECON:
			{
				row = "O1";
				return DCO_GroupTactics.SNEAK;
			}
			case DCO_EGroupTask.PATROL:
			{
				row = "O2";
				if (front)
					return DCO_GroupTactics.SNEAK;
				return DCO_GroupTactics.BALANCE;
			}
			case DCO_EGroupTask.REINFORCE:
			{
				if (!attackObj)
				{
					row = "O4";
					return DCO_GroupTactics.DEFENSIVE;
				}
				row = "O3";
				if (stage == 1)
					return DCO_GroupTactics.AGGRESIVE;
				return DCO_GroupTactics.BALANCE;
			}
			case DCO_EGroupTask.TRANSPORT:
			{
				row = "O5";
				return DCO_GroupTactics.EVASIVE;
			}
			case DCO_EGroupTask.FIRE_MISSION:
			{
				row = "O6";
				return DCO_GroupTactics.DEFENSIVE;
			}
		}
		return DCO_GroupTactics.BALANCE;
	}

	static float AltChance(bool night, bool closed)
	{
		float sneak = 0;
		float bal = 0;
		if (closed)
			sneak += 1;
		else
			bal += 1;
		if (night)
			sneak += 1;
		float diff = Math.AbsFloat(sneak - bal);
		float p = 1.0 / (1.0 + Math.Pow(2.718281828, diff * 2.0));
		return Math.Min(p, 0.15);
	}

	static bool Closed(vector pos)
	{
		array<DCO_TerrainPoint> pts = {};
		if (DCO_TerrainCache.Query(DCO_ETerrainFlag.FOREST_EDGE, pos, 150, pts) >= 3)
			return true;
		DCO_GarrisonRegistry reg = DCO_GarrisonRegistry.GetInstance();
		if (!reg)
			return false;
		array<IEntity> buildings = {};
		reg.FindBuildings(pos, 120, buildings);
		return buildings.Count() >= 4;
	}

	static bool BeyondFront(AICommander_BaseComponent cmd, vector pos, FactionKey fk)
	{
		if (!cmd)
			return false;
		float best = float.MAX;
		bool own = true;
		foreach (CMD_AICommanderObjectiveComponent o : cmd.DCO_AllObjectives())
		{
			if (!o || !o.GetOwner())
				continue;
			float d = vector.DistanceSqXZ(o.GetOwner().GetOrigin(), pos);
			if (d < best)
			{
				best = d;
				own = o.IsCapturedBy(fk);
			}
		}
		return !own;
	}

	static string RoleText(int role, int task)
	{
		if (role == DCO_CommanderTactics.ROLE_FIX || task == DCO_EGroupTask.SUPPORT_BY_FIRE)
			return "Support";
		if (role == 6 || task == DCO_EGroupTask.FLANK)
			return "Flank";
		return "Assault";
	}

	static DCO_GroupTactics Resolve(SCR_AIGroupUtilityComponent u, DCO_GroupUtilityComponent g, bool trActive)
	{
		SCR_AIGroup grp = SCR_AIGroup.Cast(g.GetOwner());
		vector lp = g.GetOwner().GetOrigin();
		if (grp && grp.GetLeaderEntity())
			lp = grp.GetLeaderEntity().GetOrigin();
		int task = g.GetTask();
		FactionKey fk = g.GetFactionKey();
		CMD_AICommanderObjectiveComponent obj = g.GetGroupObjective();
		AICommander_BaseComponent cmd = g.GetMyCommander();

		int tactic = -1;
		int role = -1;
		int planPhase = 0;
		vector goal = vector.Zero;
		bool hasGoal = false;
		if (cmd && obj)
		{
			DCO_TacticPlan p = cmd.GetTactics().GetPlan(obj);
			if (p)
			{
				tactic = p.m_eTactic;
				planPhase = p.m_iPhase;
				int gi = p.m_aGroups.Find(g);
				if (gi >= 0)
				{
					role = p.m_aRoles[gi];
					goal = p.m_aGoals[gi];
					hasGoal = true;
				}
				else if (task == DCO_EGroupTask.FLANK)
					role = 6;
				else if (task == DCO_EGroupTask.ATTACK)
					role = DCO_CommanderTactics.ROLE_ASSAULT;
				else
					tactic = -1;
			}
		}

		int stage = 0;
		bool moving = g.GetPhase() == DCO_ETaskPhase.MOVING;
		if (hasGoal && vector.DistanceXZ(lp, goal) <= 60.0)
			moving = false;
		bool attackObj = false;
		if (obj && obj.GetOwner())
		{
			attackObj = !obj.IsCapturedBy(fk);
			if (!attackObj && tactic >= 0)
				stage = 2;
			else if (g.GetPhase() == DCO_ETaskPhase.EXECUTING || vector.DistanceXZ(lp, obj.GetOwner().GetOrigin()) <= obj.GetRadius() + 150.0)
				stage = 1;
			else if (tactic >= 0 && cmd && DCO_Ctx.TuneForGU(u, "posture.release_exec", 1) > 0 && cmd.IsAssaultReleased(obj))
			{
				stage = 1;
				DCO_Perf.Count("posture:release_exec");
			}
		}
		if (trActive && g.m_iDCOTRPhase >= 0)
		{
			moving = g.m_iDCOTRPhase == DCO_ETRPhase.MOVING;
			stage = 0;
			if (g.m_iDCOTRPhase == DCO_ETRPhase.EXECUTING)
				stage = 1;
			else if (g.m_iDCOTRPhase == DCO_ETRPhase.HOLDING || g.m_iDCOTRPhase == DCO_ETRPhase.WITHDRAW)
				stage = 2;
		}
		bool contact = g.HasState(DCO_EGroupState.IN_CONTACT);

		string objName = "-";
		if (obj)
			objName = obj.GetLogName();
		string key = string.Format("%1|%2|%3|%4", task, tactic, role, objName);
		if (key != g.m_sDCOPosKey)
		{
			g.m_sDCOPosKey = key;
			g.m_bDCOPosNight = DCO_Util.IsNight();
			g.m_bDCOPosClosed = Closed(lp);
			g.m_bDCOPosFront = task == DCO_EGroupTask.PATROL && BeyondFront(cmd, lp, fk);
			g.m_fDCOPosRoll = Math.RandomFloat01();
			g.m_bDCOPosVarLogged = false;
			g.m_bDCOPosSprung = false;
		}
		if (tactic == DCO_ETactic.HAMMER_AND_ANVIL && role == DCO_CommanderTactics.ROLE_BLOCK && !moving && (contact || u.m_fDCOAmbSprung_ms >= 0))
			g.m_bDCOPosSprung = true;

		bool quiet = g.m_bDCOPosNight || g.m_bDCOPosClosed;
		string row;
		bool pair;
		DCO_GroupTactics post;
		if (g.m_iDCODefRole != DCO_EDefRole.NONE)
			post = DCO_DefenseTactics.RolePosture(g, stage, row);
		else
			post = For(task, tactic, role, stage, planPhase, moving, contact, g.m_bDCOPosSprung, g.m_iDCOThreatAct, attackObj, g.m_bDCOPosFront, g.IsRetreating(), g.m_bDCOPursuitCons, quiet, row, pair);

		string variant = "base";
		if (pair)
		{
			if (g.m_bDCOPosNight)
				variant = "night";
			else if (g.m_bDCOPosClosed)
				variant = "closed";
			float pAlt = AltChance(g.m_bDCOPosNight, g.m_bDCOPosClosed);
			DCO_GroupTactics basePost = post;
			if (g.m_fDCOPosRoll < pAlt)
			{
				post = SB(!quiet);
				variant = "random";
			}
			if (!g.m_bDCOPosVarLogged)
			{
				g.m_bDCOPosVarLogged = true;
				int radio = 0;
				if (variant != "base" && cmd && obj)
				{
					string rk = "posture_var_steady";
					if (post == DCO_GroupTactics.SNEAK)
						rk = "posture_var_quiet";
					DCO_PlayerAwareness.BroadcastOperation(cmd, obj, rk, DCO_Radio.P("role", RoleText(role, task), "obj", obj.GetDisplayName()));
					radio = 1;
				}
				DCO_Perf.Count("posture_variant:" + variant);
				DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(grp), string.Format("posture_variant grp=%1 from=%2 to=%3 variant=%4 weight=%5 radio=%6",
					DCO_Log.IdGroup(grp), typename.EnumToString(DCO_GroupTactics, basePost), typename.EnumToString(DCO_GroupTactics, post), variant, pAlt.ToString(-1, 2), radio));
			}
		}

		g.m_iDCOPosStage = stage;
		if (post != g.m_eDCOTablePosture || row != g.m_sDCOPosRow)
		{
			g.m_eDCOTablePosture = post;
			g.m_sDCOPosRow = row;
			string tacticName = "-";
			if (tactic >= 0)
				tacticName = DCO_CommanderTactics.Name(tactic);
			string postName = typename.EnumToString(DCO_GroupTactics, post);
			string stageName = "approach";
			if (stage == 1)
				stageName = "execute";
			else if (stage == 2)
				stageName = "after";
			DCO_Perf.Count("posture_cmd:" + postName);
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(grp), string.Format("posture_cmd grp=%1 task=%2 tactic=%3 role=%4 phase=%5 posture=%6 variant=%7 reason=%8",
				DCO_Log.IdGroup(grp), typename.EnumToString(DCO_EGroupTask, task), tacticName, DCO_CommanderTactics.RoleName(role), stageName, postName, variant, row) + string.Format(" moving=%1 contact=%2", moving, contact));
		}
		return post;
	}
}

class DCO_AssaultCommit
{
	static bool Ready(float lead, bool supportLifted, int nearMiss, float prepAge_ms, string prepKind, out string trigger)
	{
		trigger = "quiet";
		if (lead > 40.0 || !supportLifted)
			return false;
		if (nearMiss == 0)
			return true;
		if (prepAge_ms >= 5000.0 && prepAge_ms <= 10000.0)
		{
			trigger = prepKind;
			return true;
		}
		return false;
	}

	static void Log(AIGroup grp, string tactic, float lead, string trigger)
	{
		DCO_Perf.Count("assault_commit:" + trigger);
		DCO_Log.Write(DCO_ELogChannel.TACTIC, DCO_ELogLevel.INFO, DCO_Log.IdGroup(grp), string.Format("assault_commit grp=%1 tactic=%2 lead=%3 trigger=%4", DCO_Log.IdGroup(grp), tactic, Math.Round(lead), trigger));
	}

	static int NearMisses(array<AIAgent> agents, float now, float window_ms)
	{
		int n = 0;
		foreach (AIAgent a : agents)
		{
			SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
			if (ca && ca.m_UtilityComponent && ca.m_UtilityComponent.m_CombatComponent)
				n += ca.m_UtilityComponent.m_CombatComponent.DCO_NearMissCount(now, window_ms);
		}
		return n;
	}

	static float LastThrow(array<AIAgent> agents)
	{
		float last = -1;
		foreach (AIAgent a : agents)
		{
			if (a && a.GetControlledEntity())
				last = Math.Max(last, DCO_GrenadeUtility.LastThrow(a.GetControlledEntity()));
		}
		return last;
	}

	static int Volley(array<AIAgent> agents, vector enemy)
	{
		int n = 0;
		foreach (AIAgent a : agents)
		{
			if (n >= 3)
				break;
			SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
			if (!ca || !ca.GetControlledEntity() || !DCO_Util.IsAlive(ca.GetControlledEntity()))
				continue;
			SCR_AIUtilityComponent u = ca.m_UtilityComponent;
			if (!u || !u.m_CombatComponent || !u.m_CombatComponent.HasWeaponOfType(EWeaponType.WT_FRAGGRENADE))
				continue;
			float d = vector.DistanceXZ(ca.GetControlledEntity().GetOrigin(), enemy);
			if (d < 8.0 || d > 45.0)
				continue;
			vector tp = vector.Zero;
			if (!DCO_GrenadeUtility.ResolveThrowPos(u, enemy, tp))
				continue;
			u.AddAction(new SCR_AIThrowGrenadeToBehavior(u, null, tp, EWeaponType.WT_FRAGGRENADE, Math.RandomFloat(0.0, 0.5), SCR_AIThrowGrenadeToBehavior.PRIORITY_BEHAVIOR_THROW_GRENADE + SCR_AIThrowGrenadeToBehavior.PRIORITY_LEVEL_PLAYER));
			DCO_GrenadeUtility.NotifyGrenadeThrown(u);
			n++;
		}
		return n;
	}

	static bool Smoke(array<AIAgent> agents, vector enemy)
	{
		foreach (AIAgent a : agents)
		{
			SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
			if (!ca || !ca.GetControlledEntity() || !DCO_Util.IsAlive(ca.GetControlledEntity()))
				continue;
			SCR_AIUtilityComponent u = ca.m_UtilityComponent;
			if (!u || !u.m_CombatComponent || !u.m_CombatComponent.HasWeaponOfType(EWeaponType.WT_SMOKEGRENADE))
				continue;
			vector me = ca.GetControlledEntity().GetOrigin();
			vector sp = DCO_Util.Surface(me + DCO_Specialist.Flat(enemy - me) * Math.Min(15.0, vector.DistanceXZ(me, enemy) * 0.4));
			u.AddAction(new SCR_AIThrowGrenadeToBehavior(u, null, sp, EWeaponType.WT_SMOKEGRENADE, 0, SCR_AIThrowGrenadeToBehavior.PRIORITY_BEHAVIOR_THROW_GRENADE + SCR_AIThrowGrenadeToBehavior.PRIORITY_LEVEL_PLAYER));
			return true;
		}
		return false;
	}
}
