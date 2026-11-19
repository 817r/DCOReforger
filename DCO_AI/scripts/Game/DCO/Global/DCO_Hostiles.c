// One spawned AI group (or an ungrouped AI soldier) as seen by the last refresh.
class DCO_HostileEntry
{
	AIGroup m_Group;
	vector m_vPos;
	Faction m_Faction;
	bool m_bSim;
	int m_iAgents;
}

// Where the spawned AI is, refreshed at most every 3 s from the AI world (all AI, not only DCO's).
// A dormant group has no perception of its own, so this is what lets it wake for enemy AI.
// "Sim" means at least one soldier is below max LOD, i.e. actually thinking: a spawned group far
// from every player is frozen at max LOD and must not wake sleepers or block sleep around it.
class DCO_Hostiles
{
	protected static ref array<ref DCO_HostileEntry> s_aAll;
	protected static ref map<AIGroup, int> s_mIdx;
	protected static float s_fAt_ms = -100000;
	protected static int s_iAgents;
	protected static int s_iSim;

	static void Refresh()
	{
		if (!s_aAll)
		{
			s_aAll = {};
			s_mIdx = new map<AIGroup, int>();
		}
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return;
		float now = world.GetWorldTime();
		if (now - s_fAt_ms < 3000.0 && now >= s_fAt_ms)
			return;
		s_fAt_ms = now;
		s_aAll.Clear();
		s_mIdx.Clear();
		s_iAgents = 0;
		s_iSim = 0;
		AIWorld aiWorld = GetGame().GetAIWorld();
		if (!aiWorld)
			return;
		int maxLod = AIAgent.GetMaxLOD();
		array<AIAgent> agents = {};
		aiWorld.GetAIAgents(agents);
		foreach (AIAgent a : agents)
		{
			if (!SCR_ChimeraAIAgent.Cast(a))
				continue;
			IEntity ch = a.GetControlledEntity();
			if (!ch)
				continue;
			s_iAgents++;
			bool sim = a.GetLOD() < maxLod;
			if (sim)
				s_iSim++;
			AIGroup grp = a.GetParentGroup();
			int idx;
			if (grp && s_mIdx.Find(grp, idx))
			{
				s_aAll[idx].m_iAgents++;
				if (sim)
					s_aAll[idx].m_bSim = true;
				continue;
			}
			DCO_HostileEntry e = new DCO_HostileEntry();
			e.m_Group = grp;
			e.m_vPos = ch.GetOrigin();
			e.m_bSim = sim;
			e.m_iAgents = 1;
			e.m_Faction = FactionOf(grp, ch);
			if (grp)
				s_mIdx.Set(grp, s_aAll.Count());
			s_aAll.Insert(e);
		}
		DCO_Perf.Count("hostiles:refresh");
	}

	protected static Faction FactionOf(AIGroup grp, IEntity ch)
	{
		SCR_AIGroup sg = SCR_AIGroup.Cast(grp);
		if (sg && sg.GetFaction())
			return sg.GetFaction();
		FactionAffiliationComponent fa = FactionAffiliationComponent.Cast(ch.FindComponent(FactionAffiliationComponent));
		if (fa)
			return fa.GetAffiliatedFaction();
		return null;
	}

	// Nearest spawned AI hostile to `f` within `m`; simOnly skips groups frozen at max LOD.
	static DCO_HostileEntry Nearest(vector pos, Faction f, float m, bool simOnly)
	{
		if (!f || m <= 0)
			return null;
		Refresh();
		DCO_HostileEntry best;
		float bestD2 = m * m;
		foreach (DCO_HostileEntry e : s_aAll)
		{
			if (simOnly && !e.m_bSim)
				continue;
			if (!e.m_Faction || !f.IsFactionEnemy(e.m_Faction))
				continue;
			float d2 = vector.DistanceSqXZ(e.m_vPos, pos);
			if (d2 > bestD2)
				continue;
			bestD2 = d2;
			best = e;
		}
		return best;
	}

	static bool SimNear(vector pos, Faction f, float m)
	{
		return Nearest(pos, f, m, true) != null;
	}

	static int Agents()
	{
		Refresh();
		return s_iAgents;
	}

	static int Sim()
	{
		Refresh();
		return s_iSim;
	}
}
