#define DCO_GAME_19
class DCO_Dormant
{
	protected static ref map<AIGroup, bool> s_mSeen;

	static bool Is(AIGroup grp)
	{
		if (!grp)
			return false;
#ifdef DCO_GAME_19
		SCR_AIGroup sg = SCR_AIGroup.Cast(grp);
		if (!sg)
			return false;
		bool dormant = sg.IsDormant();
		Note(sg, dormant);
		return dormant;
#endif
		return false;
	}

	static int Count(AIGroup grp)
	{
		if (!grp)
			return 0;
#ifdef DCO_GAME_19
		SCR_AIGroup sg = SCR_AIGroup.Cast(grp);
		if (sg && sg.IsDormant())
		{
			Note(sg, true);
			int n = sg.GetDormantAliveCount();
			if (n >= 0)
			{
				DCO_Perf.Count("dormant:count");
				return n;
			}
		}
		else if (sg)
			Note(sg, false);
#endif
		return grp.GetAgentsCount();
	}

	static bool WorldEnabled()
	{
#ifdef DCO_GAME_19
		ChimeraAIWorld aiWorld = ChimeraAIWorld.Cast(GetGame().GetAIWorld());
		return aiWorld && aiWorld.IsGroupDormancyEnabled();
#endif
		return false;
	}

	static int AliveCount(AIGroup grp)
	{
#ifdef DCO_GAME_19
		SCR_AIGroup sg = SCR_AIGroup.Cast(grp);
		if (sg)
			return sg.GetDormantAliveCount();
#endif
		return 0;
	}

	static bool SetProximity(AIGroup grp, float spawnM, float despawnM)
	{
#ifdef DCO_GAME_19
		SCR_AIGroup sg = SCR_AIGroup.Cast(grp);
		if (!sg)
			return false;
		sg.SetLifecyclePolicy(SCR_EAIGroupLifecyclePolicy.ProximityDriven, spawnM, despawnM);
		return true;
#endif
		return false;
	}

	static bool Release(AIGroup grp)
	{
#ifdef DCO_GAME_19
		SCR_AIGroup sg = SCR_AIGroup.Cast(grp);
		if (!sg)
			return false;
		sg.SetLifecyclePolicy(SCR_EAIGroupLifecyclePolicy.Manual);
		if (sg.GetAgentsCount() == 0 && sg.GetDormantAliveCount() > 0)
		{
			sg.SpawnMembers();
			return true;
		}
#endif
		return false;
	}

#ifdef DCO_GAME_19
	protected static void Note(SCR_AIGroup sg, bool dormant)
	{
		if (!s_mSeen)
			s_mSeen = new map<AIGroup, bool>();
		bool was;
		bool known = s_mSeen.Find(sg, was);
		if (known && was == dormant)
			return;
		if (!known && !dormant)
			return;
		if (s_mSeen.Count() > 256)
			s_mSeen.Clear();
		s_mSeen.Set(sg, dormant);
		string state = "active";
		if (dormant)
			state = "dormant";
		DCO_Perf.Count("dormant:" + state);
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(sg), string.Format("grp_dormant grp=%1 state=%2 alive=%3 dead=%4 agents=%5", DCO_Log.IdGroup(sg), state, sg.GetDormantAliveCount(), sg.GetDormantDeadCount(), sg.GetAgentsCount()));
	}
#endif
}
