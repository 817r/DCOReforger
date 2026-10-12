class DCO_CQBPrecheckEntry
{
	int m_iResult;
	string m_sWhy;
	float m_fAt_ms;
}

class DCO_CQBPrecheck
{
	protected static ref map<IEntity, ref DCO_CQBPrecheckEntry> s_mCache;
	protected static float s_fBuildAt_ms;

	static bool On(IEntity owner)
	{
		DCO_GlobalAIComponent c = DCO_GlobalAIComponent.GetFor(owner);
		return c && c.DCO_TuneF("cqb.precheck", 1.0) > 0;
	}

	static string Label(int result)
	{
		if (result == 1)
			return "ok";
		if (result == 0)
			return "bad";
		return "unknown";
	}

	static int Check(IEntity building, IEntity owner, out string why)
	{
		why = "-";
		if (!building)
			return -1;
		if (!s_mCache)
			s_mCache = new map<IEntity, ref DCO_CQBPrecheckEntry>();
		float now = GetGame().GetWorld().GetWorldTime();
		float ttl = 600000.0;
		DCO_GlobalAIComponent c = DCO_GlobalAIComponent.GetFor(owner);
		if (c)
			ttl = c.DCO_TuneF("cqb.precheck_ttl_s", 600.0) * 1000.0;
		DCO_CQBPrecheckEntry e = s_mCache.Get(building);
		if (e && now - e.m_fAt_ms < ttl)
		{
			why = e.m_sWhy;
			DCO_Perf.Count("cqb_precheck:cached");
			return e.m_iResult;
		}
		bool built = DCO_IndoorNav.Get(building, false) != null;
		if (!built && s_fBuildAt_ms == now)
		{
			why = "budget";
			DCO_Perf.Count("cqb_precheck:budget");
			return -1;
		}
		if (!built)
			s_fBuildAt_ms = now;
		string w;
		DCO_INavCQBPlan plan = DCO_INavCQBPlan.Make(building, building.GetOrigin(), null, "precheck", w);
		int r = 1;
		if (!plan)
		{
			r = 0;
			if (w == "wait")
			{
				r = -1;
				w = "no_graph";
			}
		}
		why = w;
		e = new DCO_CQBPrecheckEntry();
		e.m_iResult = r;
		e.m_sWhy = w;
		e.m_fAt_ms = now;
		s_mCache.Set(building, e);
		DCO_Perf.Count("cqb_precheck:" + Label(r));
		DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_precheck building=%1 pos=%2 result=%3 why=%4 built_now=%5 ttl_s=%6", DCO_Util.PrefabShort(building), DCO_Util.XZ(building.GetOrigin()), Label(r), w, DCO_Util.B01(!built), Math.Round(ttl * 0.001)));
		return r;
	}

	static bool Plannable(IEntity building, IEntity owner)
	{
		string why;
		return Check(building, owner, why) != 0;
	}

	protected static ref map<IEntity, float> s_mSkipLog;

	static bool LogSkip(IEntity building)
	{
		if (!building)
			return false;
		if (!s_mSkipLog)
			s_mSkipLog = new map<IEntity, float>();
		float now = GetGame().GetWorld().GetWorldTime();
		float at;
		if (s_mSkipLog.Find(building, at) && now - at < 60000.0)
			return false;
		s_mSkipLog.Set(building, now);
		return true;
	}
}
