class DCO_BtWatch
{
	protected static ref set<string> s_sSeen;
	protected static bool s_bCombat;
	protected static bool s_bArmed;

	static void Mark(string tree)
	{
		if (!s_sSeen)
			s_sSeen = new set<string>();
		if (!s_sSeen.Contains(tree))
			s_sSeen.Insert(tree);
	}

	static string Txt(string s)
	{
		if (s.IsEmpty())
			return "-";
		return s;
	}

	static void Combat()
	{
		s_bCombat = true;
	}

	static void Arm()
	{
		if (s_bArmed || !Replication.IsServer())
			return;
		s_bArmed = true;
		GetGame().GetCallqueue().CallLater(Check, 300000, false, 1);
	}

	static void Check(int round)
	{
		if (!s_sSeen)
			s_sSeen = new set<string>();
		array<string> always = {"Idle", "ActivityIdle"};
		array<string> branch = {"Idle_TakeCover", "HandleAmmo", "CombatMoveRequest_Move"};
		array<string> combat = {"Attack_Default", "SuppressBehavior", "MoveAndInvestigate", "HandleCombatMove", "Defend"};
		string missing;
		string unseen;
		foreach (string t : always)
		{
			if (!s_sSeen.Contains(t))
				missing += t + " ";
		}
		foreach (string bt : branch)
		{
			if (!s_sSeen.Contains(bt))
				unseen += bt + " ";
		}
		if (s_bCombat)
		{
			foreach (string t : combat)
			{
				if (!s_sSeen.Contains(t))
					unseen += t + " ";
			}
		}
		missing = missing.Trim();
		string seen;
		foreach (string t : s_sSeen)
			seen += t + " ";
		DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", string.Format("compat_bt round=%1 combat=%2 seen=%3 missing=%4 unseen=%5 not_checked=Soldier,ThrowGrenadeTo,MoveFromDanger_UnknownFire", round, DCO_Util.B01(s_bCombat), DCO_BtWatch.Txt(seen.Trim()), DCO_BtWatch.Txt(missing), DCO_BtWatch.Txt(unseen.Trim())));
		if (!missing.IsEmpty())
		{
			DCO_Perf.Count("compat_bt:missing");
			DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.WARN, "-", string.Format("compat_bt_missing trees=%1 hint=another addon replaces these behaviour trees, DCO nodes never ran", missing));
		}
		if (round < 3)
			GetGame().GetCallqueue().CallLater(Check, 600000, false, round + 1);
	}
}
