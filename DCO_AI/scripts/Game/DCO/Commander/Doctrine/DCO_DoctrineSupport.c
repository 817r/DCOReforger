class DCO_SupportState
{
	int m_iStart;
	float m_fContact;
	float m_fSafeSince;
	bool m_bFatigued;
	ref array<float> m_aNext;

	void DCO_SupportState()
	{
		m_fSafeSince = -1;
		m_aNext = {0, 0, 0, 0, 0};
	}
}

class DCO_DoctrineSupport
{
	protected ref map<DCO_GroupUtilityComponent, ref DCO_SupportState> m_mState = new map<DCO_GroupUtilityComponent, ref DCO_SupportState>();
	protected float m_fLastTick = -1;

	static string CondNames(int mask)
	{
		array<string> names = {"reduced", "fatigued", "low_ammo", "low_morale", "in_contact", "threatened", "overran"};
		string text;
		foreach (int i, string name : names)
		{
			if ((mask & (1 << i)) == 0)
				continue;
			if (!text.IsEmpty())
				text += ",";
			text += name;
		}
		if (text.IsEmpty())
			return "none";
		return text;
	}

	static string JobName(int job)
	{
		switch (job)
		{
			case DCO_ESupportJob.SUPPLIES: return "RESUPPLY";
			case DCO_ESupportJob.REINFORCEMENT: return "REINFORCE";
			case DCO_ESupportJob.SUPPORT: return "SUPPORT";
			case DCO_ESupportJob.FIRESUPPORT: return "FIRESUPPORT";
		}
		return "MEDEVAC";
	}

	protected DCO_SupportState State(DCO_GroupUtilityComponent g)
	{
		DCO_SupportState st = m_mState.Get(g);
		if (!st)
		{
			st = new DCO_SupportState();
			m_mState.Set(g, st);
		}
		return st;
	}

	protected static float TraitFor(AICommander_BaseComponent cmd, int job)
	{
		if (job == DCO_ESupportJob.SUPPORT || job == DCO_ESupportJob.FIRESUPPORT)
			return cmd.GetTraitSupport();
		return cmd.GetTraitLogistics();
	}

	static float AvgFatigue(DCO_GroupUtilityComponent g)
	{
		SCR_AIGroup grp;
		if (g)
			grp = SCR_AIGroup.Cast(g.GetOwner());
		if (!grp)
			return 0;
		array<AIAgent> agents = {};
		grp.GetAgents(agents);
		float sum = 0;
		int n = 0;
		foreach (AIAgent a : agents)
		{
			SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
			if (!ca || !ca.m_UtilityComponent || !DCO_Util.IsAlive(ca.GetControlledEntity()))
				continue;
			DCO_AIMoraleSystem ms = ca.m_UtilityComponent.GetMoraleSystem();
			if (!ms)
				continue;
			sum += ms.GetFatigueSeconds();
			n++;
		}
		if (n == 0)
			return 0;
		return sum / n;
	}

	bool IsFatigued(DCO_GroupUtilityComponent g)
	{
		DCO_SupportState st = m_mState.Get(g);
		return st && st.m_bFatigued;
	}

	static bool EnemyWithin(AICommander_BaseComponent cmd, vector pos, float radius, float now)
	{
		CMD_ThreatResponseComponent tr = cmd.GetThreatResponseComponent();
		if (!tr)
			return false;
		foreach (CMD_ThreatEntry t : tr.GetThreats())
		{
			if (t && now - t.m_fLastUpdateTime < 60.0 && vector.DistanceXZ(t.m_vBelievedPos, pos) < radius)
				return true;
		}
		return false;
	}

	int Conditions(AICommander_BaseComponent cmd, DCO_GroupUtilityComponent g, int job, float now)
	{
		DCO_SupportState st = State(g);
		int units = g.GetUnitCount();
		if (units > st.m_iStart)
			st.m_iStart = units;
		float unitsF = units;
		float alive = unitsF / Math.Max(st.m_iStart, 1.0);
		float trait = TraitFor(cmd, job);
		float reducedTh = Math.Clamp(0.6 + 0.15 * (trait - 1), 0.45, 0.8);
		int mask;
		if (st.m_iStart >= 3 && alive <= reducedTh)
			mask |= DCO_ESupportCond.REDUCED;
		float avgFat = AvgFatigue(g);
		bool fatigued = st.m_fContact > 600 || avgFat >= 300;
		if (fatigued)
		{
			mask |= DCO_ESupportCond.FATIGUED;
			if (!st.m_bFatigued)
			{
				string cause = "strain";
				if (st.m_fContact > 600)
					cause = "contact";
				DCO_Perf.Count("cmd:group_fatigued");
				DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(cmd), string.Format("cmd_group_fatigued grp=%1 avg_fat_s=%2 contact_s=%3 cause=%4", DCO_Log.IdGroup(AIGroup.Cast(g.GetOwner())), Math.Round(avgFat), Math.Round(st.m_fContact), cause));
			}
		}
		st.m_bFatigued = fatigued;
		int mags = cmd.GetLogiLowAmmoMags();
		if (trait >= 1.5)
			mags++;
		SCR_AIGroup grp = SCR_AIGroup.Cast(g.GetOwner());
		if (grp && DCO_Logistics.IsLowAmmo(grp, mags))
			mask |= DCO_ESupportCond.LOW_AMMO;
		if (g.GetGroupMorale() > cmd.GetMoraleAttackThreshold())
			mask |= DCO_ESupportCond.LOW_MORALE;
		if (g.HasState(DCO_EGroupState.IN_CONTACT))
			mask |= DCO_ESupportCond.IN_CONTACT;
		if (g.HasState(DCO_EGroupState.SUPPRESSED))
			mask |= DCO_ESupportCond.THREATENED;
		if (st.m_iStart >= 3 && alive <= 0.5 && EnemyWithin(cmd, g.GetOwner().GetOrigin(), 50.0, now))
			mask |= DCO_ESupportCond.OVERRAN;
		return mask;
	}

	protected void Log(AICommander_BaseComponent cmd, DCO_GroupUtilityComponent g, int cond, int job, int crossLevel, float cooldown, string result)
	{
		DCO_Perf.Count("doctrine:support:" + JobName(job) + ":" + result);
		string line = "support_trigger cmd=" + cmd.GetCommanderUID() + " grp=" + DCO_Log.IdGroup(AIGroup.Cast(g.GetOwner()));
		line += " cond=" + CondNames(cond) + " job=" + JobName(job) + " crosslevel=" + crossLevel.ToString();
		line += " cooldown_s=" + Math.Round(cooldown).ToString() + " result=" + result;
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(cmd), line);
	}

	void Tick(AICommander_BaseComponent cmd, float now)
	{
		DCO_Doctrine d = cmd.GetDoctrine();
		if (!d)
			return;
		float dt = 15.0;
		if (m_fLastTick >= 0)
			dt = now - m_fLastTick;
		m_fLastTick = now;
		array<DCO_GroupUtilityComponent> gone = {};
		foreach (DCO_GroupUtilityComponent known, DCO_SupportState ks : m_mState)
		{
			if (!known || !known.GetOwner() || known.GetUnitCount() == 0)
				gone.Insert(known);
		}
		foreach (DCO_GroupUtilityComponent dead : gone)
		{
			m_mState.Remove(dead);
		}
		foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
		{
			if (!g || !g.GetOwner() || g.IsPlayerGroup() || g.IsMortar() || g.IsDedicatedTransport() || g.IsInTransport() || g.GetUnitCount() == 0)
				continue;
			DCO_SupportState st = State(g);
			if (g.HasState(DCO_EGroupState.IN_CONTACT) || g.HasState(DCO_EGroupState.SUPPRESSED))
			{
				st.m_fContact += dt;
				st.m_fSafeSince = -1;
			}
			else
			{
				if (st.m_fSafeSince < 0)
					st.m_fSafeSince = now;
				if (now - st.m_fSafeSince >= 120)
					st.m_fContact = 0;
			}
			int logiMask = Conditions(cmd, g, DCO_ESupportJob.REINFORCEMENT, now);
			int supportMask = logiMask;
			if (Math.AbsFloat(cmd.GetTraitSupport() - cmd.GetTraitLogistics()) > 0.01)
				supportMask = Conditions(cmd, g, DCO_ESupportJob.SUPPORT, now);
			TryJob(cmd, g, d, DCO_ESupportJob.REINFORCEMENT, logiMask, now);
			TryJob(cmd, g, d, DCO_ESupportJob.SUPPORT, supportMask, now);
			TryJob(cmd, g, d, DCO_ESupportJob.FIRESUPPORT, supportMask, now);
		}
	}

	protected void TryJob(AICommander_BaseComponent cmd, DCO_GroupUtilityComponent g, DCO_Doctrine d, int job, int mask, float now)
	{
		int hit = mask & d.m_aSupport[job];
		if (hit == 0)
			return;
		DCO_SupportState st = State(g);
		if (now < st.m_aNext[job])
			return;
		st.m_aNext[job] = now + 180;
		vector pos = g.GetOwner().GetOrigin();
		CMD_ThreatResponseComponent tr = cmd.GetThreatResponseComponent();
		CMD_ThreatEntry threat;
		if (tr)
			threat = tr.FindThreatNear(pos);
		string result = "no_asset";
		switch (job)
		{
			case DCO_ESupportJob.REINFORCEMENT:
			{
				if ((hit & ~DCO_ESupportCond.OVERRAN) == 0)
				{
					result = "skip_overran";
					break;
				}
				if (tr)
				{
					tr.ReceiveReinforcementRequest(g, now);
					result = "requested";
				}
				break;
			}
			case DCO_ESupportJob.SUPPORT:
			{
				DCO_CommanderDefense def = cmd.GetDefense();
				if (!def)
					break;
				bool needAT = threat != null && (threat.m_iArmor > 0 || threat.m_iVehicle > 0);
				DCO_GroupUtilityComponent helper = def.PickReserve(cmd, pos, needAT, true, g, 1500);
				if (!helper)
					break;
				vector away = vector.Forward;
				if (threat)
					away = DCO_Specialist.Flat(pos - threat.m_vBelievedPos).Normalized();
				vector supportPos = DCO_Util.Surface(pos + away * 80.0);
				helper.CompleteAllWaypoints();
				helper.SetTask(DCO_EGroupTask.SUPPORT_BY_FIRE);
				if (cmd.SpawnMoveRoute(helper, helper.GetOwner().GetOrigin(), supportPos, now))
					result = "sent_" + DCO_Log.IdGroup(AIGroup.Cast(helper.GetOwner()));
				break;
			}
			case DCO_ESupportJob.FIRESUPPORT:
			{
				if (!threat || vector.DistanceXZ(threat.m_vBelievedPos, pos) > 600.0)
				{
					result = "no_target";
					break;
				}
				if (DCO_DefenseTactics.Mortar(cmd, threat.m_vBelievedPos, 4, "doctrine_support"))
					result = "fired";
				break;
			}
		}
		Log(cmd, g, hit, job, 0, 180, result);
	}

	bool WantsSupplies(AICommander_BaseComponent cmd, DCO_GroupUtilityComponent g, string hubSource, float now)
	{
		DCO_Doctrine d = cmd.GetDoctrine();
		int hit = Conditions(cmd, g, DCO_ESupportJob.SUPPLIES, now) & d.m_aSupport[DCO_ESupportJob.SUPPLIES];
		if (hit == 0)
			return false;
		DCO_SupportState st = State(g);
		if (now < st.m_aNext[DCO_ESupportJob.SUPPLIES])
			return false;
		st.m_aNext[DCO_ESupportJob.SUPPLIES] = now + 180;
		vector pos = g.GetOwner().GetOrigin();
		string src;
		vector hub = cmd.GetHubFor(pos, src);
		if (hub != vector.Zero && vector.DistanceXZ(hub, pos) < 200.0)
		{
			float sd = DCO_Rearm.StationDist(pos, 200.0);
			if (sd >= 0 || DCO_Ctx.TuneForCmd(cmd, "logi.cross_src", 1.0) <= 0)
			{
				Log(cmd, g, hit, DCO_ESupportJob.SUPPLIES, 1, 180, "crosslevel_hub");
				return false;
			}
			DCO_Perf.Count("logi:crosslevel_skip_no_source");
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(cmd), string.Format("crosslevel_skip grp=%1 reason=no_ammo_source hub_d=%2 src=%3", DCO_Log.IdGroup(AIGroup.Cast(g.GetOwner())), Math.Round(vector.DistanceXZ(hub, pos)), src));
		}
		Log(cmd, g, hit, DCO_ESupportJob.SUPPLIES, 0, 180, "queued");
		return true;
	}

	bool MedevacAllowed(AICommander_BaseComponent cmd, DCO_GroupUtilityComponent g, float now)
	{
		DCO_Doctrine d = cmd.GetDoctrine();
		int row = d.m_aSupport[DCO_ESupportJob.MEDEVAC];
		if (row == 0)
			return false;
		if ((row & DCO_ESupportCond.REDUCED) != 0)
			return true;
		return (Conditions(cmd, g, DCO_ESupportJob.MEDEVAC, now) & row) != 0;
	}
}
