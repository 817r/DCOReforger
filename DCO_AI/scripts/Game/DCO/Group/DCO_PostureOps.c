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
		if (!g || g.IsPlayerGroup())
			return;

		switch (p)
		{
			case DCO_GroupTactics.EVASIVE:
			{
				if (!contact || g.IsBreakingContact())
					break;
				if (u.m_fDCOEvasiveAt_ms >= 0 && now_ms - u.m_fDCOEvasiveAt_ms < 60000.0)
					break;
				u.m_fDCOEvasiveAt_ms = now_ms;
				if (DCO_BreakContact.Start(g, threat, Away(lp, threat, Math.RandomFloat(150.0, 300.0)), "posture_evasive"))
				{
					DCO_Perf.Count("pos:evasive_withdraw");
					Report(g, grp, threat, n, "EVASIVE");
				}
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
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("posture_delay_bound grp=%1 dist=%2 enemy=%3", grp, Math.Round(d), Math.Round(nearest)));
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
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("posture_ambush_set grp=%1 dist=%2 enemies=%3", grp, Math.Round(nearest), n));
		}

		string why;
		if (nearest < 60.0)
			why = "killzone";
		else if (u.DCO_IsDirectlyThreatened() || g.HasState(DCO_EGroupState.SUPPRESSED))
			why = "detected";
		if (why.IsEmpty())
			return;
		u.m_fDCOAmbSprung_ms = now_ms;
		u.m_fDCOAmbDecide_ms = now_ms + Math.RandomFloat(20000.0, 40000.0);
		DCO_Perf.Count("pos:ambush_sprung");
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("posture_ambush_sprung grp=%1 why=%2 dist=%3 enemies=%4", grp, why, Math.Round(nearest), n));
	}
}
