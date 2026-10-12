class DCO_AmbushDrill
{
	protected static ref array<ref DCO_AmbushDrill> s_aActive;

	DCO_GroupUtilityComponent m_Group;
	vector m_vThreat;
	bool m_bNear;
	bool m_bL;
	bool m_bDecide;
	float m_fStart;
	float m_fDecideAt;

	static void Start(DCO_GroupUtilityComponent g, vector threat, bool lShape, int known)
	{
		SCR_AIGroup grp = SCR_AIGroup.Cast(g.GetOwner());
		if (!grp || !grp.GetLeaderEntity())
			return;
		float now = DCO_Util.NowS();
		vector lp = grp.GetLeaderEntity().GetOrigin();
		float dist = vector.DistanceXZ(lp, threat);
		g.m_fDCOAmbushAt = now;

		AICommander_BaseComponent cmd = g.GetMyCommander();
		if (cmd && cmd.GetDefense())
			cmd.GetDefense().AddDanger(threat, now);
		bool mounted = g.IsInTransport() || g.HasState(DCO_EGroupState.MOUNTED);
		DCO_Perf.Count("amb:detect");
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("ambush_detected grp=%1 dist=%2 known=%3 L=%4 mounted=%5 mode=%6", grp, Math.Round(dist), known, lShape, mounted,
			typename.EnumToString(DCO_EAmbushReaction, g.GetAmbushReaction())));
		if (mounted)
			return;

		DCO_AmbushDrill d = new DCO_AmbushDrill();
		d.m_Group = g;
		d.m_vThreat = threat;
		d.m_bL = lShape;
		d.m_fStart = now;
		g.m_DCOAmbush = d;

		int pers = DCO_Radio.PersonalityOf(cmd);
		DCO_EAmbushReaction mode = g.GetAmbushReaction();
		d.m_bNear = dist < 50.0 || (pers == 1 && dist < 150.0 && !lShape);

		array<AIAgent> agents = {};
		grp.GetAgents(agents);
		vector axis = DCO_Specialist.Flat(lp - threat);
		vector side = Vector(axis[2], 0, -axis[0]);
		if (Math.RandomFloat01() < 0.5)
			side = side * -1.0;

		if (mode == DCO_EAmbushReaction.GO_PRONE || (mode == DCO_EAmbushReaction.DYNAMIC && pers == 2 && !d.m_bNear))
		{
			foreach (AIAgent a : agents)
				Move(a, vector.Zero, threat, false, ECharacterStance.PRONE, 5.0);
			d.m_bDecide = mode == DCO_EAmbushReaction.DYNAMIC;
		}
		else if (mode == DCO_EAmbushReaction.SUPPRESS)
		{
			foreach (AIAgent a : agents)
				Move(a, vector.Zero, threat, true, ECharacterStance.CROUCH, 8.0);
		}
		else if (mode == DCO_EAmbushReaction.FAN_OUT)
		{
			foreach (int i, AIAgent a : agents)
			{
				float s = Math.RandomFloat(10.0, 20.0);
				if (i % 2 == 1)
					s = -s;
				Move(a, side * s, threat, true, ECharacterStance.PRONE, 12.0);
			}
		}
		else if (d.m_bNear)
		{
			foreach (AIAgent a : agents)
			{
				IEntity e = a.GetControlledEntity();
				if (e)
					Move(a, threat - axis * 10.0 - e.GetOrigin(), threat, false, ECharacterStance.CROUCH, 20.0);
			}
			DCO_Perf.Count("amb:assault");
		}
		else
		{
			CMD_ThreatResponseComponent.ThrowSmoke(g, threat);
			foreach (AIAgent a : agents)
				Move(a, side * Math.RandomFloat(15.0, 25.0), threat, true, ECharacterStance.PRONE, 15.0);
			d.m_bDecide = true;
		}

		float wait = Math.RandomFloat(20.0, 40.0);
		if (pers == 2)
			wait = Math.RandomFloat(30.0, 40.0);
		else if (pers == 1)
			wait = Math.RandomFloat(20.0, 25.0);
		d.m_fDecideAt = now + wait;

		if (!s_aActive)
		{
			s_aActive = {};
			GetGame().GetCallqueue().CallLater(Tick, 1000, true);
		}
		s_aActive.Insert(d);
	}

	protected static void Move(AIAgent a, vector offset, vector threat, bool cover, ECharacterStance stance, float duration)
	{
		SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
		if (!ca || !ca.m_UtilityComponent || !DCO_Util.IsAlive(ca.GetControlledEntity()))
			return;
		vector p = DCO_Util.Surface(ca.GetControlledEntity().GetOrigin() + offset);
		DCO_Specialist.CombatMove(ca.m_UtilityComponent, p, threat, cover, stance, duration);
	}

	protected static void Tick()
	{
		float now = DCO_Util.NowS();
		for (int i = s_aActive.Count() - 1; i >= 0; i--)
		{
			DCO_AmbushDrill d = s_aActive[i];
			if (!d.m_Group || d.m_Group.m_DCOAmbush != d)
			{
				s_aActive.Remove(i);
				continue;
			}
			if (now < d.m_fDecideAt)
				continue;
			d.Decide(now);
			d.m_Group.m_DCOAmbush = null;
			s_aActive.Remove(i);
		}
	}

	protected void Decide(float now)
	{
		SCR_AIGroup grp = SCR_AIGroup.Cast(m_Group.GetOwner());
		if (!m_bDecide || !grp || !DCO_Util.IsAlive(grp.GetLeaderEntity()))
			return;

		int units = m_Group.GetUnitCount();
		int enemies = 0;
		SCR_AIGroupUtilityComponent u = m_Group.GetGroupUtilityComponent();
		PerceptionManager pm = GetGame().GetPerceptionManager();
		if (u && u.m_Perception && pm)
		{
			float pmNow = pm.GetTime();
			foreach (SCR_AITargetInfo t : u.m_Perception.m_aTargets)
			{
				if (t && pmNow - t.m_fTimestamp < 30.0 && vector.DistanceXZ(t.m_vWorldPos, m_vThreat) < 150.0)
					enemies++;
			}
		}

		string verdict = "break_contact";
		if (!m_bL && enemies > 0 && units >= 4 && units >= 1.0 * enemies * 1.5)
			verdict = "flank";

		if (verdict == "flank")
		{
			vector lp = grp.GetLeaderEntity().GetOrigin();
			vector toward = DCO_Specialist.Flat(m_vThreat - lp);
			vector side = Vector(toward[2], 0, -toward[0]);
			if (Math.RandomFloat01() < 0.5)
				side = side * -1.0;
			vector flank = DCO_Util.Surface(lp + side * 60.0 + toward * (vector.DistanceXZ(lp, m_vThreat) * 0.6));
			array<AIAgent> agents = {};
			grp.GetAgents(agents);
			foreach (int i, AIAgent a : agents)
			{
				SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
				if (i % 2 == 0 || !ca || !ca.m_UtilityComponent || !DCO_Util.IsAlive(ca.GetControlledEntity()))
					continue;
				DCO_RelocateBehavior.Start(ca.m_UtilityComponent, DCO_Util.Surface(flank + Vector(Math.RandomFloat(-5, 5), 0, Math.RandomFloat(-5, 5))), m_vThreat, true, ECharacterStance.CROUCH, vector.DistanceXZ(ca.GetControlledEntity().GetOrigin(), flank) / 3.0 + 3.0, "ambush_flank");
			}
		}
		else if (!DCO_BreakContact.Start(m_Group, m_vThreat, vector.Zero, "ambush_far"))
			verdict = "fight";

		DCO_Perf.Count("amb:" + verdict);
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("ambush_decide grp=%1 verdict=%2 units=%3 enemies=%4 L=%5", grp, verdict, units, enemies, m_bL));
	}
}
