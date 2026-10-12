enum DCO_EGroupFeature
{
	CQB = 1,
	OVERMATCH = 2,
	LEAN = 4,
	WINDOW_FIRE = 8,
	SPECIALISTS = 16,
	HEARING = 32,
	NIGHT = 64,
	AWARENESS = 128,
	REARM = 256,
	CONTACT_DRILLS = 512,
	TARGET_PRIORITY = 1024,
	EXPLOSION_REACTIONS = 2048
}

enum DCO_EGroupDetail
{
	FULL,
	BALANCED,
	LIGHT,
	CUSTOM
}

class DCO_GroupFeatures
{
	protected static int s_iGroupsLimited;

	static int AllMask()
	{
		return 4095;
	}

	static int PresetOff(int detail)
	{
		if (detail == DCO_EGroupDetail.BALANCED)
			return DCO_EGroupFeature.LEAN | DCO_EGroupFeature.WINDOW_FIRE | DCO_EGroupFeature.AWARENESS | DCO_EGroupFeature.REARM;
		if (detail == DCO_EGroupDetail.LIGHT)
			return AllMask() & ~(DCO_EGroupFeature.SPECIALISTS | DCO_EGroupFeature.CONTACT_DRILLS | DCO_EGroupFeature.EXPLOSION_REACTIONS);
		return 0;
	}

	static int DetailOf(int off)
	{
		for (int d = DCO_EGroupDetail.FULL; d <= DCO_EGroupDetail.LIGHT; d++)
		{
			if (PresetOff(d) == off)
				return d;
		}
		return DCO_EGroupDetail.CUSTOM;
	}

	static int BehaviorOffMask(int featOff)
	{
		int b;
		if (featOff & DCO_EGroupFeature.LEAN)
			b = b | DCO_EBehavior.LEAN_PEEK;
		if (featOff & DCO_EGroupFeature.WINDOW_FIRE)
			b = b | DCO_EBehavior.WINDOW_FIRE;
		if (featOff & DCO_EGroupFeature.SPECIALISTS)
			b = b | DCO_EBehavior.MG_GUNNER | DCO_EBehavior.AT_SPECIALIST | DCO_EBehavior.MARKSMAN | DCO_EBehavior.LEADER_RTO_GRENADIER;
		if (featOff & DCO_EGroupFeature.HEARING)
			b = b | DCO_EBehavior.HEARING | DCO_EBehavior.PLAYER_VOICE | DCO_EBehavior.BUDDY_CALLOUT;
		if (featOff & DCO_EGroupFeature.AWARENESS)
			b = b | DCO_EBehavior.AWARENESS | DCO_EBehavior.SECURITY_360;
		if (featOff & DCO_EGroupFeature.REARM)
			b = b | DCO_EBehavior.REARM | DCO_EBehavior.REARM_ENEMY_SOURCES | DCO_EBehavior.ENEMY_WEAPONS | DCO_EBehavior.SPECIALIST_RECOVERY;
		if (featOff & DCO_EGroupFeature.CONTACT_DRILLS)
			b = b | DCO_EBehavior.CONTACT_SOP | DCO_EBehavior.BREAK_CONTACT | DCO_EBehavior.AMBUSH_DRILL;
		if (featOff & DCO_EGroupFeature.TARGET_PRIORITY)
			b = b | DCO_EBehavior.TARGET_PRIORITY | DCO_EBehavior.RELOAD_DISCIPLINE;
		if (featOff & DCO_EGroupFeature.EXPLOSION_REACTIONS)
			b = b | DCO_EBehavior.EXPLOSION_REACTIONS;
		return b;
	}

	static void NoteLimited(int oldOff, int newOff)
	{
		if (oldOff == 0 && newOff != 0)
			s_iGroupsLimited++;
		else if (oldOff != 0 && newOff == 0)
			s_iGroupsLimited = Math.Max(0, s_iGroupsLimited - 1);
	}

	static bool AnyLimited()
	{
		return s_iGroupsLimited > 0;
	}

	static SCR_AIGroup GroupOf(IEntity e)
	{
		if (!e)
			return null;
		SCR_AIGroup g = SCR_AIGroup.Cast(e);
		if (g)
			return g;
		AIAgent a = AIAgent.Cast(e);
		if (!a)
		{
			AIControlComponent ctrl = AIControlComponent.Cast(e.FindComponent(AIControlComponent));
			if (ctrl)
				a = ctrl.GetControlAIAgent();
		}
		if (!a)
			return null;
		return SCR_AIGroup.Cast(a.GetParentGroup());
	}

	static bool BehaviorAllowed(IEntity e, int bit)
	{
		if (s_iGroupsLimited <= 0)
			return true;
		SCR_AIGroup g = GroupOf(e);
		return !g || (g.DCO_GetBehaviorOff() & bit) == 0;
	}

	static bool BehaviorAllowedAgent(AIAgent a, int bit)
	{
		if (s_iGroupsLimited <= 0 || !a)
			return true;
		SCR_AIGroup g = SCR_AIGroup.Cast(a.GetParentGroup());
		return !g || (g.DCO_GetBehaviorOff() & bit) == 0;
	}

	static string Names(int mask)
	{
		if (mask == 0)
			return "none";
		string s;
		for (int bit = 1; bit <= 2048; bit = bit << 1)
		{
			if ((mask & bit) == 0)
				continue;
			if (!s.IsEmpty())
				s += "|";
			s += typename.EnumToString(DCO_EGroupFeature, bit);
		}
		return s;
	}
}

modded class SCR_AIGroup
{
	protected int m_iDCOFeatOff;
	protected int m_iDCOBehOff;

	int DCO_GetFeatureOff()
	{
		return m_iDCOFeatOff;
	}

	int DCO_GetBehaviorOff()
	{
		return m_iDCOBehOff;
	}

	bool DCO_FeatureOn(int feature)
	{
		return (m_iDCOFeatOff & feature) == 0;
	}

	void DCO_SetFeatureOff(int off, string source)
	{
		off = off & DCO_GroupFeatures.AllMask();
		if (off == m_iDCOFeatOff)
			return;
		DCO_GroupFeatures.NoteLimited(m_iDCOFeatOff, off);
		int old = m_iDCOFeatOff;
		m_iDCOFeatOff = off;
		m_iDCOBehOff = DCO_GroupFeatures.BehaviorOffMask(off);
		DCO_Perf.Count("grp_feat:set_" + source);
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(this), string.Format("group_features grp=%1 detail=%2 off=%3 was=%4 source=%5", DCO_Log.IdGroup(this), typename.EnumToString(DCO_EGroupDetail, DCO_GroupFeatures.DetailOf(off)), DCO_GroupFeatures.Names(off), DCO_GroupFeatures.Names(old), source));
	}
}
