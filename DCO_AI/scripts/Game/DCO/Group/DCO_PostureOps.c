class DCO_PostureOps
{
	static bool FreshContact(SCR_AIGroupUtilityComponent u, vector from, out vector center, out float nearest, out int count)
	{
		nearest = float.MAX;
		count = 0;
		PerceptionManager pm = GetGame().GetPerceptionManager();
		if (!u.m_Perception || !pm)
			return false;
		float pmNow = pm.GetTime();
		vector sum;
		foreach (SCR_AITargetInfo t : u.m_Perception.m_aTargets)
		{
			if (!t || pmNow - t.m_fTimestamp > 10.0)
				continue;
			if (t.m_eCategory != EAITargetInfoCategory.DETECTED && t.m_eCategory != EAITargetInfoCategory.IDENTIFIED)
				continue;
			sum = sum + t.m_vWorldPos;
			count++;
			nearest = Math.Min(nearest, vector.DistanceXZ(from, t.m_vWorldPos));
		}
		if (count == 0)
			return false;
		center = sum * (1.0 / count);
		return true;
	}

	static vector Away(vector from, vector threat, float dist)
	{
		vector away = DCO_Specialist.Flat(from - threat);
		return DCO_Util.Surface(from + away * dist);
	}

	static void Report(DCO_GroupUtilityComponent g, SCR_AIGroup grp, vector pos, int n, string tag)
	{
		AICommander_BaseComponent cmd = g.GetMyCommander();
		if (!cmd || !cmd.GetThreatResponseComponent())
			return;
		CMD_ContactReport report = new CMD_ContactReport(pos, Math.Max(n, 1), DCO_Util.NowS(), tag + ":" + grp.GetName());
		cmd.GetThreatResponseComponent().ReceiveContactReport(report, g);
	}

	static bool ROE(SCR_AIGroupUtilityComponent u, DCO_GroupUtilityComponent g, out EAIGroupCombatMode mode)
	{
		DCO_GroupTactics p = u.DCO_GetPosture();
		if (p == DCO_GroupTactics.EVASIVE)
		{
			if (u.DCO_GetCombatModeExternal() == EAIGroupCombatMode.HOLD_FIRE)
				return false;
			float evNow = GetGame().GetWorld().GetWorldTime();
			vector evLp = u.m_Owner.GetOrigin();
			SCR_AIGroup evGrp = u.m_Owner;
			if (evGrp && evGrp.GetLeaderEntity())
				evLp = evGrp.GetLeaderEntity().GetOrigin();
			vector evC;
			float evNear;
			int evN;
			bool forced = u.DCO_IsDirectlyThreatened() || (FreshContact(u, evLp, evC, evNear, evN) && evNear < 30.0);
			if (forced && (u.m_fDCOEvasiveFire_ms < 0 || evNow - u.m_fDCOEvasiveFire_ms >= 20000.0))
			{
				DCO_Perf.Count("pos:evasive_fire_forced");
				DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("evasive_fire_forced grp=%1 nearest=%2 threatened=%3", DCO_Log.IdGroup(u.m_Owner), Math.Round(evNear), u.DCO_IsDirectlyThreatened()));
			}
			if (forced)
				u.m_fDCOEvasiveFire_ms = evNow;
			if (u.m_fDCOEvasiveFire_ms >= 0 && evNow - u.m_fDCOEvasiveFire_ms < 20000.0)
			{
				mode = EAIGroupCombatMode.FIRE_AT_WILL;
				return true;
			}
			mode = EAIGroupCombatMode.HOLD_FIRE;
			return true;
		}
		if (p != DCO_GroupTactics.SNEAK && p != DCO_GroupTactics.AMBUSH)
			return false;
		if (u.DCO_GetCombatModeExternal() == EAIGroupCombatMode.HOLD_FIRE)
			return false;
		if (g && (g.IsBreakingContact() || g.m_bDCOCmdAmbush))
			return false;
		float now_ms = GetGame().GetWorld().GetWorldTime();
		if (p == DCO_GroupTactics.AMBUSH && u.m_fDCOAmbSprung_ms >= 0)
		{
			mode = EAIGroupCombatMode.FIRE_AT_WILL;
			return true;
		}
		if (p == DCO_GroupTactics.SNEAK && u.m_fDCOSneakSpotted_ms >= 0 && now_ms - u.m_fDCOSneakSpotted_ms < 60000.0)
		{
			mode = EAIGroupCombatMode.FIRE_AT_WILL;
			return true;
		}
		mode = EAIGroupCombatMode.HOLD_FIRE;
		return true;
	}

	static void Tick(SCR_AIGroupUtilityComponent u, SCR_AIGroup grp, DCO_GroupUtilityComponent g, float now_ms)
	{
		IEntity leader = grp.GetLeaderEntity();
		if (!leader || !DCO_Util.IsAlive(leader))
			return;
		vector lp = leader.GetOrigin();
		vector threat;
		float nearest;
		int n;
		bool contact = FreshContact(u, lp, threat, nearest, n);
		if (contact)
		{
			u.m_fDCOContactSeen_ms = now_ms;
			if (u.m_bDCOForcedUntilContact)
				u.DCO_ClearForced("contact");
		}

		DCO_GroupTactics p = u.DCO_GetPosture();
		if (p != DCO_GroupTactics.AMBUSH && (u.m_bDCOAmbSet || u.m_fDCOAmbSprung_ms >= 0))
		{
			u.m_bDCOAmbSet = false;
			u.m_fDCOAmbSprung_ms = -1;
		}
		if (p != DCO_GroupTactics.DELAY)
			u.m_fDCODelayHold_ms = 0;
		if (p != DCO_GroupTactics.EVASIVE && (u.m_iDCOLurkPhase != 0 || u.m_fDCOEvasiveFire_ms >= 0))
		{
			u.m_iDCOLurkPhase = 0;
			u.m_fDCOEvasiveFire_ms = -1;
		}
		if (!g || g.IsPlayerGroup())
			return;
		if (p == DCO_GroupTactics.DEFENSIVE)
		{
			bool farOff = vector.DistanceXZ(lp, u.m_vDCODefAnchor) > 80.0 && !g.IsBreakingContact();
			if (u.m_vDCODefAnchor == vector.Zero || farOff)
			{
				u.m_vDCODefAnchor = lp;
				DCO_Perf.Count("pos:def_anchor");
				DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("def_anchor_set grp=%1 pos=%2 reason=%3", DCO_Log.IdGroup(grp), DCO_Util.XZ(lp), DCO_Util.B01(farOff)));
			}
		}
		else if (u.m_vDCODefAnchor != vector.Zero && !g.IsBreakingContact())
		{
			u.m_vDCODefAnchor = vector.Zero;
			u.m_iDCODefFallbacks = 0;
		}

		switch (p)
		{
			case DCO_GroupTactics.EVASIVE:
			{
				TickEvasive(u, grp, g, lp, contact, threat, nearest, n, now_ms);
				break;
			}
			case DCO_GroupTactics.DELAY:
			{
				TickDelay(u, grp, g, lp, contact, threat, nearest, now_ms);
				break;
			}
			case DCO_GroupTactics.SNEAK:
			{
				if (!contact || g.IsBreakingContact())
					break;
				if (!u.DCO_IsDirectlyThreatened() && !g.HasState(DCO_EGroupState.SUPPRESSED))
					break;
				if (u.m_fDCOSneakSpotted_ms >= 0 && now_ms - u.m_fDCOSneakSpotted_ms < 60000.0)
					break;
				u.m_fDCOSneakSpotted_ms = now_ms;
				DCO_Perf.Count("pos:sneak_spotted");
				DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("posture_sneak_spotted grp=%1 dist=%2 enemies=%3", grp, Math.Round(nearest), n));
				Report(g, grp, threat, n, "SNEAK");
				DCO_BreakContact.Start(g, threat, vector.Zero, "posture_sneak_spotted");
				break;
			}
			case DCO_GroupTactics.AMBUSH:
			{
				TickAmbush(u, grp, g, contact, threat, nearest, n, now_ms);
				break;
			}
		}
	}

	protected static void TickEvasive(SCR_AIGroupUtilityComponent u, SCR_AIGroup grp, DCO_GroupUtilityComponent g, vector lp, bool contact, vector threat, float nearest, int n, float now_ms)
	{
		if (g.IsBreakingContact())
			return;
		string trigger;
		if (contact)
			trigger = "posture_evasive";
		else if (u.DCO_IsDirectlyThreatened() || g.HasState(DCO_EGroupState.SUPPRESSED))
		{
			float estR;
			int estN;
			if (DCO_FireEstimate.Get(grp, now_ms, 15000.0, threat, estR, estN))
			{
				trigger = "posture_evasive_fire";
				nearest = vector.DistanceXZ(lp, threat);
			}
		}
		if (!trigger.IsEmpty())
		{
			bool lurking = u.m_iDCOLurkPhase == 2;
			if (!lurking && u.m_fDCOEvasiveAt_ms >= 0 && now_ms - u.m_fDCOEvasiveAt_ms < 60000.0)
				return;
			u.m_fDCOEvasiveAt_ms = now_ms;
			u.m_vDCOLurkEnemy = threat;
			u.m_iDCOLurkPhase = 0;
			if (DCO_BreakContact.Start(g, threat, Away(lp, threat, Math.RandomFloat(150.0, 300.0)), trigger))
			{
				DCO_Perf.Count("pos:evasive_withdraw");
				if (trigger == "posture_evasive_fire")
					DCO_Perf.Count("pos:evasive_fire_trigger");
				Report(g, grp, threat, n, "EVASIVE");
				DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("evasive_escape grp=%1 trigger=%2 dist=%3 enemies=%4 lurking=%5", DCO_Log.IdGroup(grp), trigger, Math.Round(nearest), n, lurking));
			}
			return;
		}
		if (u.m_fDCOEvasiveAt_ms < 0)
			return;
		if (u.m_iDCOLurkPhase == 0)
		{
			Hide(u, grp, lp, now_ms, Math.RandomFloat(30000.0, 90000.0));
			return;
		}
		if (now_ms < u.m_fDCOLurkUntil_ms)
			return;
		if (u.m_iDCOLurkPhase == 1)
		{
			if (u.DCO_PostureGm() != DCO_GroupTactics.EVASIVE)
			{
				u.m_iDCOLurkPhase = 3;
				DCO_Perf.Count("pos:evasive_stay_hidden");
				DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("evasive_stay_hidden grp=%1 reason=not_gm_evasive", DCO_Log.IdGroup(grp)));
				return;
			}
			Lurk(u, grp, lp, now_ms);
			return;
		}
		if (u.m_iDCOLurkPhase == 2)
			Hide(u, grp, lp, now_ms, Math.RandomFloat(20000.0, 45000.0));
	}

	protected static void Hide(SCR_AIGroupUtilityComponent u, SCR_AIGroup grp, vector lp, float now_ms, float dur_ms)
	{
		u.m_iDCOLurkPhase = 1;
		u.m_fDCOLurkUntil_ms = now_ms + dur_ms;
		vector spot = DCO_TerrainCache.QueryBest(DCO_ETerrainFlag.FOREST_EDGE, lp, 60.0, string.Empty, lp, u.m_vDCOLurkEnemy);
		if (spot == vector.Zero)
			spot = lp;
		array<AIAgent> agents = {};
		grp.GetAgents(agents);
		foreach (AIAgent a : agents)
		{
			SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
			if (!ca || !ca.m_UtilityComponent || !DCO_Util.IsAlive(ca.GetControlledEntity()))
				continue;
			vector p = DCO_Util.Surface(spot + Vector(Math.RandomFloat(-6, 6), 0, Math.RandomFloat(-6, 6)));
			DCO_Specialist.CombatMove(ca.m_UtilityComponent, p, u.m_vDCOLurkEnemy, true, ECharacterStance.PRONE, 20.0);
		}
		DCO_Perf.Count("pos:evasive_hide");
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("evasive_hide grp=%1 s=%2 spot_dist=%3 to_enemy=%4", DCO_Log.IdGroup(grp), Math.Round(dur_ms / 1000.0), Math.Round(vector.DistanceXZ(lp, spot)), Math.Round(vector.DistanceXZ(spot, u.m_vDCOLurkEnemy))));
	}

	protected static void Lurk(SCR_AIGroupUtilityComponent u, SCR_AIGroup grp, vector lp, float now_ms)
	{
		u.m_iDCOLurkPhase = 2;
		u.m_fDCOLurkUntil_ms = now_ms + 120000.0;
		vector enemy = u.m_vDCOLurkEnemy;
		vector dir = DCO_Specialist.Flat(lp - enemy);
		vector goal = enemy + dir * Math.RandomFloat(150.0, 250.0);
		vector spot = DCO_TerrainCache.QueryBest(DCO_ETerrainFlag.FOREST_EDGE, goal, 60.0, string.Empty, goal, enemy);
		if (spot != vector.Zero)
			goal = spot;
		goal = DCO_Util.Surface(goal);
		array<AIAgent> agents = {};
		grp.GetAgents(agents);
		foreach (AIAgent a : agents)
		{
			SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
			if (!ca || !ca.m_UtilityComponent || !DCO_Util.IsAlive(ca.GetControlledEntity()))
				continue;
			vector p = DCO_Util.Surface(goal + Vector(Math.RandomFloat(-5, 5), 0, Math.RandomFloat(-5, 5)));
			DCO_RelocateBehavior b = DCO_RelocateBehavior.Start(ca.m_UtilityComponent, p, enemy, true, ECharacterStance.CROUCH, 150.0, "evasive_lurk");
			if (b)
				b.DCO_SetMove(ECharacterStance.CROUCH, EMovementType.WALK, 8.0, false);
		}
		DCO_Perf.Count("pos:evasive_lurk");
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("evasive_lurk grp=%1 move=%2 to_enemy=%3", DCO_Log.IdGroup(grp), Math.Round(vector.DistanceXZ(lp, goal)), Math.Round(vector.DistanceXZ(goal, enemy))));
	}

	protected static void TickDelay(SCR_AIGroupUtilityComponent u, SCR_AIGroup grp, DCO_GroupUtilityComponent g, vector lp, bool contact, vector threat, float nearest, float now_ms)
	{
		if (!contact)
		{
			u.m_fDCODelayHold_ms = 0;
			return;
		}
		if (g.IsBreakingContact())
			return;
		if (u.m_fDCODelayHold_ms <= 0)
		{
			u.m_fDCODelayHold_ms = now_ms + Math.RandomFloat(30000.0, 60000.0);
			DCO_Perf.Count("pos:delay_hold");
			return;
		}
		if (now_ms < u.m_fDCODelayHold_ms && nearest >= 75.0 && !g.HasState(DCO_EGroupState.SUPPRESSED))
			return;
		u.m_fDCODelayHold_ms = 0;
		float d = Math.RandomFloat(100.0, 150.0);
		if (DCO_BreakContact.Start(g, threat, Away(lp, threat, d + 50.0), "posture_delay", d))
		{
			DCO_Perf.Count("pos:delay_bound");
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("posture_delay_bound grp=%1 dist=%2 enemy=%3", DCO_Log.IdGroup(grp), Math.Round(d), Math.Round(nearest)));
		}
	}

	protected static void TickAmbush(SCR_AIGroupUtilityComponent u, SCR_AIGroup grp, DCO_GroupUtilityComponent g, bool contact, vector threat, float nearest, int n, float now_ms)
	{
		if (g.m_bDCOCmdAmbush)
			return;
		if (u.m_fDCOAmbSprung_ms >= 0)
		{
			if (!contact && now_ms - u.m_fDCOContactSeen_ms > 60000.0)
			{
				u.m_bDCOAmbSet = false;
				u.m_fDCOAmbSprung_ms = -1;
				return;
			}
			if (u.m_fDCOAmbDecide_ms <= 0 || now_ms < u.m_fDCOAmbDecide_ms)
				return;
			u.m_fDCOAmbDecide_ms = 0;
			if (g.m_sDCOPosRow == "T-HA2")
			{
				DCO_Perf.Count("pos:anvil_hold");
				DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(grp), string.Format("anvil_hold grp=%1 ratio=%2 reason=no_ambush_assault_withdraw", DCO_Log.IdGroup(grp), u.m_fDCOLastRatio.ToString(-1, 2)));
				return;
			}
			if (u.m_fDCOLastRatio >= 1.0)
			{
				DCO_Perf.Count("pos:ambush_assault");
				u.DCO_ForcePosture(DCO_GroupTactics.AGGRESIVE, now_ms, 120000.0, false, "ambush_assault");
			}
			else
			{
				DCO_Perf.Count("pos:ambush_withdraw");
				u.DCO_ForcePosture(DCO_GroupTactics.EVASIVE, now_ms, 120000.0, false, "ambush_withdraw");
			}
			return;
		}
		if (!contact)
			return;

		if (!u.m_bDCOAmbSet)
		{
			u.m_bDCOAmbSet = true;
			array<AIAgent> agents = {};
			grp.GetAgents(agents);
			foreach (AIAgent a : agents)
			{
				SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
				if (ca && ca.m_UtilityComponent && DCO_Util.IsAlive(ca.GetControlledEntity()))
					DCO_Specialist.CombatMove(ca.m_UtilityComponent, ca.GetControlledEntity().GetOrigin(), threat, true, ECharacterStance.PRONE, 20.0);
			}
			DCO_Perf.Count("pos:ambush_set");
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("posture_ambush_set grp=%1 dist=%2 enemies=%3", DCO_Log.IdGroup(grp), Math.Round(nearest), n));
		}

		string why;
		if (nearest < u.m_fDCOAmbRangeM)
			why = "killzone";
		else if (u.DCO_IsDirectlyThreatened() || g.HasState(DCO_EGroupState.SUPPRESSED))
			why = "detected";
		if (why.IsEmpty())
			return;
		u.m_fDCOAmbSprung_ms = now_ms;
		u.m_fDCOAmbDecide_ms = now_ms + Math.RandomFloat(20000.0, 40000.0);
		DCO_Perf.Count("pos:ambush_sprung");
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("posture_ambush_sprung grp=%1 why=%2 dist=%3 enemies=%4 range_m=%5", grp, why, Math.Round(nearest), n, Math.Round(u.m_fDCOAmbRangeM)));
	}
}
