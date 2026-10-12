class DCO_PinnedGroup
{
	static void Tick(SCR_AIGroupUtilityComponent u, SCR_AIGroup grp, DCO_GroupUtilityComponent g, float now_ms)
	{
		if (!DCO_GlobalAIComponent.BehaviorOnFor(grp, DCO_EBehavior.PINNED))
		{
			u.m_bDCOPinGroup = false;
			return;
		}
		array<AIAgent> agents = {};
		grp.GetAgents(agents);
		array<SCR_AIUtilityComponent> free = {};
		int alive;
		int pinned;
		array<vector> froms = {};
		vector center;
		foreach (AIAgent a : agents)
		{
			SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
			if (!ca || !ca.m_UtilityComponent || !ca.m_UtilityComponent.m_CombatComponent || !DCO_Util.IsAlive(ca.GetControlledEntity()))
				continue;
			alive++;
			center = center + ca.GetControlledEntity().GetOrigin();
			SCR_AICombatComponent cc = ca.m_UtilityComponent.m_CombatComponent;
			if (cc.DCO_IsPinned())
			{
				pinned++;
				froms.Insert(cc.m_vDCOPinFrom);
			}
			else
				free.Insert(ca.m_UtilityComponent);
		}
		if (alive == 0)
			return;
		center = center * (1.0 / alive);
		float ratio = pinned;
		ratio = ratio / alive;
		float bestSq = float.MAX;
		foreach (vector fp : froms)
		{
			float dSq = vector.DistanceSqXZ(fp, center);
			if (fp != vector.Zero && dSq < bestSq)
			{
				bestSq = dSq;
				u.m_vDCOPinShooter = fp;
			}
		}

		if (!u.m_bDCOPinGroup)
		{
			if (alive < 2 || ratio < 0.5)
				return;
			if (u.m_fDCOPinGroupAt_ms >= 0 && now_ms - u.m_fDCOPinGroupAt_ms < 60000.0)
				return;
			u.m_bDCOPinGroup = true;
			u.m_fDCOPinGroupAt_ms = now_ms;
			u.m_fDCOPinLow_ms = -1;
			u.m_fDCOPinSuppress_ms = -1;
			DCO_Perf.Count("pin_group");
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("pin_group_start grp=%1 pinned=%2 alive=%3 shooter_dist=%4", DCO_Log.IdGroup(grp), pinned, alive, Math.Round(vector.Distance(center, u.m_vDCOPinShooter))));
			Start(u, grp, g, center, free, alive);
			return;
		}

		if (ratio < 0.25)
		{
			if (u.m_fDCOPinLow_ms < 0)
				u.m_fDCOPinLow_ms = now_ms;
			else if (now_ms - u.m_fDCOPinLow_ms >= 10000.0)
			{
				u.m_bDCOPinGroup = false;
				DCO_Perf.Count("pin_group_end");
				DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("pin_group_end grp=%1 s=%2", DCO_Log.IdGroup(grp), Math.Round((now_ms - u.m_fDCOPinGroupAt_ms) / 1000.0)));
				return;
			}
		}
		else
			u.m_fDCOPinLow_ms = -1;

		if (u.m_fDCOPinSuppress_ms < 0 || now_ms - u.m_fDCOPinSuppress_ms >= 6000.0)
		{
			u.m_fDCOPinSuppress_ms = now_ms;
			foreach (SCR_AIUtilityComponent fu : free)
				Suppress(fu, u.m_vDCOPinShooter);
		}
	}

	protected static void Start(SCR_AIGroupUtilityComponent u, SCR_AIGroup grp, DCO_GroupUtilityComponent g, vector center, array<SCR_AIUtilityComponent> free, int alive)
	{
		vector shooter = u.m_vDCOPinShooter;
		if (g)
		{
			AICommander_BaseComponent cmd = g.GetMyCommander();
			if (cmd && cmd.GetThreatResponseComponent())
			{
				CMD_ContactReport report = new CMD_ContactReport(shooter, 1, DCO_Util.NowS(), "PINNED:" + grp.GetName());
				report.m_bFiringAtUs = true;
				cmd.GetThreatResponseComponent().ReceiveContactReport(report, g);
				DCO_Perf.Count("pin_group_support");
			}
			CMD_ThreatResponseComponent.ThrowSmoke(g, shooter);
			DCO_Perf.Count("pin_group_smoke");
		}

		foreach (SCR_AIUtilityComponent fu : free)
			Suppress(fu, shooter);

		if (alive < 4 || free.Count() < 2)
			return;
		vector flank;
		if (!FlankPos(grp, center, shooter, flank))
		{
			DCO_Perf.Count("pin_group_flank_blocked");
			return;
		}
		int sent;
		foreach (int i, SCR_AIUtilityComponent fu2 : free)
		{
			if (i % 2 == 1 && free.Count() > 2)
				continue;
			vector p = DCO_Util.Surface(flank + Vector(Math.RandomFloat(-4, 4), 0, Math.RandomFloat(-4, 4)));
			DCO_RelocateBehavior.Start(fu2, p, shooter, true, ECharacterStance.CROUCH, vector.Distance(fu2.GetOrigin(), p) / 3.5 + 2.0, "pin_flank");
			sent++;
		}
		DCO_Perf.Count("pin_group_flank");
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("pin_group_flank grp=%1 sent=%2 dist=%3", DCO_Log.IdGroup(grp), sent, Math.Round(vector.Distance(center, flank))));
	}

	protected static bool FlankPos(SCR_AIGroup grp, vector center, vector shooter, out vector outPos)
	{
		vector to = DCO_Specialist.Flat(shooter - center);
		vector side = Vector(to[2], 0, -to[0]);
		if (Math.RandomIntInclusive(0, 1) == 1)
			side = side * -1.0;
		float along = Math.Min(vector.DistanceXZ(center, shooter) * 0.4, 60.0);
		vector p = center + side * Math.RandomFloat(60.0, 150.0) + to * along;
		AIWaypoint wp = grp.GetCurrentWaypoint();
		if (wp)
		{
			vector wc = wp.GetOrigin();
			float r = Math.Max(wp.GetCompletionRadius(), 60.0) + 60.0;
			vector off = p - wc;
			off[1] = 0;
			if (off.Length() > r)
				p = wc + off.Normalized() * r;
		}
		p = DCO_Util.Surface(p);
		NavmeshWorldComponent nav = GetGame().GetAIWorld().GetNavmeshWorldComponent("Soldiers");
		if (!nav)
			return false;
		if (!nav.IsTileLoaded(p))
		{
			if (!nav.IsTileRequested(p))
				nav.LoadTileIn(p);
			DCO_Perf.Count("pin_group_flank:nav_tile_request");
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.DEBUG, "-", string.Format("pin_group_flank_tile grp=%1 pos=%2 action=load_retry", grp, DCO_Util.XZ(p)));
			return false;
		}
		return nav.GetReachablePoint(p, 10.0, outPos);
	}

	protected static void Suppress(SCR_AIUtilityComponent fu, vector shooter)
	{
		if (!fu || !fu.m_CombatComponent || shooter == vector.Zero)
			return;
		vector bbMin, bbMax;
		SCR_AISuppressionVolumeBase.CreateSuppressionBox(shooter, 3.0, 3, bbMin, bbMax);
		SCR_AISuppressionObjectVolumeBox vol = new SCR_AISuppressionObjectVolumeBox(bbMin, bbMax);
		fu.AddAction(new SCR_AISuppressBehavior(fu, null, vol, 6.0, 1.5));
	}
}
