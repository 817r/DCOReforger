class DCO_MoveArbiter
{
	static int Prio(string why)
	{
		if (why.StartsWith("idf_"))
			return 100;
		if (why == "stress_panic" || why == "stress_break")
			return 90;
		if (why == "pin_crawl")
			return 80;
		if (why == "break_contact" || why == "smoke_exit" || why == "ambush_flank" || why == "pin_flank" || why.StartsWith("flush_"))
			return 60;
		if (why.StartsWith("mg_") || why.StartsWith("at_") || why.StartsWith("marksman_"))
			return 50;
		return 40;
	}

	static string Layer(int prio)
	{
		if (prio >= 100)
			return "survive";
		if (prio >= 90)
			return "panic";
		if (prio >= 80)
			return "pin";
		if (prio >= 60)
			return "drill";
		if (prio >= 50)
			return "specialist";
		return "task";
	}

	static bool Allow(SCR_AIUtilityComponent u, string why, string oldWhy, float oldAge_ms = -1)
	{
		int pn = Prio(why);
		int po = Prio(oldWhy);
		bool ok = pn >= po;
		string decision = "supersede";
		if (!ok)
			decision = "block";
		else if (pn == po && why != oldWhy && oldAge_ms >= 0 && oldAge_ms < DCO_Ctx.TuneForU(u, "osc.commit_s", 3.0) * 1000.0 && DCO_Ctx.TuneForU(u, "osc.fix_commit", 1.0) > 0)
		{
			ok = false;
			decision = "commit";
		}
		DCO_Perf.Count("move_arbiter:" + decision + "_" + Layer(pn) + "_by_" + Layer(po));
		if (u && u.m_OwnerEntity && DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG))
			DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(u.m_OwnerEntity), string.Format("move_arbiter unit=%1 new=%2 old=%3 decision=%4 new_layer=%5 old_layer=%6 old_age_s=%7",
				DCO_Log.IdUnit(u.m_OwnerEntity), why, oldWhy, decision, Layer(pn), Layer(po), (oldAge_ms / 1000.0).ToString(-1, 1)));
		return ok;
	}
}
