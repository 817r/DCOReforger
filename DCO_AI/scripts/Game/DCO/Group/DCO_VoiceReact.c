class DCO_VoiceState
{
	ref array<IEntity> m_aSpk = {};
	ref array<float> m_aAt = {};
	IEntity m_LastSrc;
	float m_fLastDist = -1;
	float m_fLastAt = -100000;
	float m_fReactAt = -100000;
	float m_fReportAt = -100000;

	int Note(IEntity spk, float now_ms)
	{
		for (int i = m_aSpk.Count() - 1; i >= 0; i--)
		{
			if (!m_aSpk[i] || now_ms - m_aAt[i] > 30000.0 || m_aSpk[i] == spk)
			{
				m_aSpk.Remove(i);
				m_aAt.Remove(i);
			}
		}
		m_aSpk.Insert(spk);
		m_aAt.Insert(now_ms);
		return m_aSpk.Count();
	}
}

class DCO_VoiceReact
{
	static void OnHeard(SCR_AIGroupUtilityComponent u, SCR_AIGroup grp, DCO_VoiceState st, IEntity src, vector pos, Faction f, bool bearingOnly, float noise, IEntity listener, bool contact, float now_ms)
	{
		int talkers = st.Note(src, now_ms);
		float dist = vector.DistanceXZ(listener.GetOrigin(), src.GetOrigin());
		bool approaching = src == st.m_LastSrc && st.m_fLastDist > 0 && now_ms - st.m_fLastAt < 20000.0 && dist < st.m_fLastDist - 3.0;
		st.m_LastSrc = src;
		st.m_fLastDist = dist;
		st.m_fLastAt = now_ms;

		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("heard_voice grp=%1 speaker=%2 dist=%3 bearing_only=%4 noise=%5 talkers=%6 contact=%7",
			grp, src, Math.Round(dist), bearingOnly, Math.Round(noise), talkers, contact));

		DCO_GroupUtilityComponent g = DCO_GroupUtilityComponent.Cast(grp.FindComponent(DCO_GroupUtilityComponent));
		Report(g, st, pos, talkers, bearingOnly, noise, now_ms);

		if (contact || now_ms - st.m_fReactAt < 30000.0)
			return;
		st.m_fReactAt = now_ms;

		if (g && (g.GetTask() == DCO_EGroupTask.GARRISON || g.GetTask() == DCO_EGroupTask.DEFEND))
		{
			GarrisonAlert(g, grp, pos);
			return;
		}

		AICommander_BaseComponent cmd;
		if (g && !g.IsInTransport())
			cmd = g.GetMyCommander();

		if (cmd && approaching)
		{
			g.m_fDCOVoiceAmbushUntil = DCO_Util.NowS() + 60.0;
			GoProne(grp, pos, 30.0);
			DCO_Perf.Count("voice:ambush");
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("voice_ambush grp=%1 dist=%2", grp, Math.Round(dist)));
			return;
		}

		DCO_EAIPersonality pers = DCO_EAIPersonality.STANDARD;
		SCR_ChimeraAIAgent la = SCR_ChimeraAIAgent.Cast(grp.GetLeaderAgent());
		if (la)
			pers = DCO_AIConfigComponent.PersonalityOf(la.m_UtilityComponent);

		switch (pers)
		{
			case DCO_EAIPersonality.CAUTIOUS:
			{
				GoProne(grp, pos, 20.0);
				DCO_Perf.Count("voice:cautious_prone");
				break;
			}
			case DCO_EAIPersonality.AGGRESSIVE:
			{
				if (cmd && Flank(cmd, g, grp, pos))
					DCO_Perf.Count("voice:flank");
				break;
			}
			case DCO_EAIPersonality.RECKLESS:
			{
				if (cmd && dist < 40.0 && ReconByFire(cmd, g, grp, pos))
					DCO_Perf.Count("voice:recon_fire");
				break;
			}
			default:
			{
				DCO_Perf.Count("voice:investigate");
				break;
			}
		}
	}

	protected static void GoProne(SCR_AIGroup grp, vector facing, float duration)
	{
		array<AIAgent> agents = {};
		grp.GetAgents(agents);
		foreach (AIAgent a : agents)
		{
			SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
			if (ca && ca.m_UtilityComponent && DCO_Util.IsAlive(ca.GetControlledEntity()))
				DCO_Specialist.CombatMove(ca.m_UtilityComponent, ca.GetControlledEntity().GetOrigin(), facing, true, ECharacterStance.PRONE, duration);
		}
	}

	protected static bool Flank(AICommander_BaseComponent cmd, DCO_GroupUtilityComponent g, SCR_AIGroup grp, vector pos)
	{
		DCO_EGroupTask task = g.GetTask();
		if (task != DCO_EGroupTask.NONE && task != DCO_EGroupTask.PATROL)
			return false;
		IEntity leader = grp.GetLeaderEntity();
		if (!leader)
			return false;
		vector to = pos - leader.GetOrigin();
		to[1] = 0;
		if (to.Length() < 5)
			return false;
		to.Normalize();
		vector side = Vector(to[2], 0, -to[0]);
		if (Math.RandomFloat01() < 0.5)
			side = side * -1.0;
		SCR_AIWaypoint wp = cmd.SpawnMoveWP(DCO_Util.Surface(pos + side * 60.0));
		if (!wp)
			return false;
		grp.AddWaypointAt(wp, 0);
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("voice_flank grp=%1", grp));
		return true;
	}

	protected static bool ReconByFire(AICommander_BaseComponent cmd, DCO_GroupUtilityComponent g, SCR_AIGroup grp, vector pos)
	{
		SCR_AIWaypoint wp = cmd.SpawnSuppressWP(pos);
		if (!wp)
			return false;
		g.m_fDCOVoiceReconFireUntil = DCO_Util.NowS() + 15.0;
		grp.AddWaypointAt(wp, 0);
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("voice_recon_fire grp=%1", grp));
		return true;
	}

	protected static void GarrisonAlert(DCO_GroupUtilityComponent g, SCR_AIGroup grp, vector pos)
	{
		IEntity leader = grp.GetLeaderEntity();
		DCO_AIGarrisonActivity act = g.GetGarrisonActivity();
		if (leader && act && act.IsActive())
		{
			vector dir = pos - leader.GetOrigin();
			dir[1] = 0;
			if (dir.Length() > 1)
			{
				dir.Normalize();
				act.DCO_Reface(dir, false);
			}
		}
		if (DCO_Night.IsActive())
			DCO_Night.ApplyLights(grp, true, null);
		DCO_Perf.Count("voice:garrison_alert");
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("voice_garrison_alert grp=%1", grp));
	}

	protected static void Report(DCO_GroupUtilityComponent g, DCO_VoiceState st, vector pos, int talkers, bool bearingOnly, float noise, float now_ms)
	{
		if (!g || now_ms - st.m_fReportAt < 60000.0)
			return;
		AICommander_BaseComponent cmd = g.GetMyCommander();
		if (!cmd || !cmd.GetThreatResponseComponent())
			return;
		st.m_fReportAt = now_ms;

		CMD_ContactReport report = new CMD_ContactReport(pos, Math.Max(talkers, 1), DCO_Util.NowS(), "VOICE:" + g.GetOwner().GetName());
		report.m_fUncertainty = noise;
		report.m_iInf = Math.Max(talkers, 1);
		report.m_bLowPriority = bearingOnly && talkers <= 2;
		cmd.GetThreatResponseComponent().ReceiveContactReport(report, g);
		DCO_Perf.Count("voice:report");
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("voice_report grp=%1 talkers=%2 unc=%3 low=%4", g.GetOwner(), talkers, Math.Round(noise), report.m_bLowPriority));
	}
}
