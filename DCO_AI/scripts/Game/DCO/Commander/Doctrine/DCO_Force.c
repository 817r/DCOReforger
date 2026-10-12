class DCO_Force
{
	protected static ref map<string, float> s_mLogAt;
	protected static map<string, float> GetLogAt()
	{
		if (!s_mLogAt)
			s_mLogAt = new map<string, float>();
		return s_mLogAt;
	}
	protected static ref map<string, int> s_mLogN;
	protected static map<string, int> GetLogN()
	{
		if (!s_mLogN)
			s_mLogN = new map<string, int>();
		return s_mLogN;
	}

	static void Keys(notnull array<string> outKeys)
	{
		outKeys.Insert("reserve");
		outKeys.Insert("sbf");
		outKeys.Insert("attack");
		outKeys.Insert("objectives");
		outKeys.Insert("recon");
		outKeys.Insert("recon_front");
		outKeys.Insert("mtc_lead");
		outKeys.Insert("op");
		outKeys.Insert("patrol");
		outKeys.Insert("ambush");
		outKeys.Insert("raid");
		outKeys.Insert("defend_front");
		outKeys.Insert("garrison");
		outKeys.Insert("counter");
		outKeys.Insert("delay_move");
		outKeys.Insert("qrf");
		outKeys.Insert("feint");
	}

	static string DefaultText()
	{
		return "reserve 1 - 2; sbf 1 0.33 2; attack 2 -1 6; objectives - - 2; recon 1 0 1; recon_front 0 0 2; mtc_lead 1 0 1; op 1 0 2; patrol 1 0 2; ambush 1 0 2; raid 2 0 3; defend_front 2 0.67 -; garrison 1 0 1; counter 1 -1 3; delay_move - 0.5 -; qrf 1 0 2; feint 0 0 1";
	}

	static bool Row(string text, string key, out int min, out float share, out int max)
	{
		min = 0;
		share = 0;
		max = 99;
		array<string> rows = {};
		text.Split(";", rows, true);
		foreach (string r : rows)
		{
			array<string> p = {};
			r.Split(" ", p, true);
			if (p.Count() < 4 || p[0] != key)
				continue;
			if (p[1] != "-")
				min = p[1].ToInt();
			if (p[2] != "-")
				share = p[2].ToFloat();
			if (p[3] != "-")
				max = p[3].ToInt();
			return true;
		}
		return false;
	}

	static void RowOrDefault(string text, string key, out int min, out float share, out int max)
	{
		if (!Row(text, key, min, share, max))
			Row(DefaultText(), key, min, share, max);
	}

	static string Canon(string text)
	{
		array<string> keys = {};
		Keys(keys);
		string result;
		foreach (string k : keys)
		{
			int mi;
			float sh;
			int ma;
			RowOrDefault(text, k, mi, sh, ma);
			if (!result.IsEmpty())
				result += "; ";
			string maxText = ma.ToString();
			if (ma >= 99)
				maxText = "-";
			result += k + " " + mi.ToString() + " " + sh.ToString(-1, 2) + " " + maxText;
		}
		return result;
	}

	static string SetField(string text, string key, int field, float value)
	{
		array<string> keys = {};
		Keys(keys);
		string result;
		foreach (string k : keys)
		{
			int mi;
			float sh;
			int ma;
			RowOrDefault(text, k, mi, sh, ma);
			if (k == key)
			{
				if (field == 0)
					mi = Math.Clamp(Math.Round(value), 0, 20);
				else if (field == 1)
					sh = Math.Clamp(value, -1, 1);
				else
					ma = Math.Clamp(Math.Round(value), 0, 99);
			}
			if (!result.IsEmpty())
				result += "; ";
			string maxText = ma.ToString();
			if (ma >= 99)
				maxText = "-";
			result += k + " " + mi.ToString() + " " + sh.ToString(-1, 2) + " " + maxText;
		}
		return result;
	}

	protected static DCO_Doctrine Doc(AICommander_BaseComponent cmd)
	{
		if (!cmd)
			return null;
		return cmd.GetDoctrine();
	}

	static void Get(AICommander_BaseComponent cmd, string key, out int min, out float share, out int max)
	{
		string text;
		DCO_Doctrine d = Doc(cmd);
		if (d)
			text = d.m_sForce;
		RowOrDefault(text, key, min, share, max);
	}

	static int Count(AICommander_BaseComponent cmd, string key, int avail, int ratioN = -1, int cap = 99)
	{
		int min;
		float share;
		int max;
		Get(cmd, key, min, share, max);
		if (avail < min)
		{
			LogSkip(cmd, key, avail, min);
			return -1;
		}
		int want;
		if (share < -0.5)
			want = ratioN;
		else if (share <= 0.001)
			want = min;
		else
			want = Math.Round(share * avail);
		want = Math.ClampInt(want, min, max);
		int n = want;
		if (cap < n)
		{
			LogCap(cmd, key, want, cap);
			n = cap;
		}
		n = Math.ClampInt(n, 0, avail);
		LogSize(cmd, key, avail, min, share, max, cap, n);
		return n;
	}

	static int Max(AICommander_BaseComponent cmd, string key, int cap = 99)
	{
		int min;
		float share;
		int max;
		Get(cmd, key, min, share, max);
		int n = max;
		if (cap < n)
		{
			LogCap(cmd, key, max, cap);
			n = cap;
		}
		LogSize(cmd, key, -1, min, share, max, cap, n);
		return n;
	}

	static int Min(AICommander_BaseComponent cmd, string key)
	{
		int min;
		float share;
		int max;
		Get(cmd, key, min, share, max);
		return min;
	}

	static float Share(AICommander_BaseComponent cmd, string key)
	{
		int min;
		float share;
		int max;
		Get(cmd, key, min, share, max);
		return share;
	}

	static int AttackMax(AICommander_BaseComponent cmd)
	{
		int cap = 99;
		if (cmd)
			cap = cmd.GetMaxAttackGroups();
		return Math.Max(1, Max(cmd, "attack", cap));
	}

	static int Reserve(AICommander_BaseComponent cmd, int avail)
	{
		int min;
		float share;
		int max;
		Get(cmd, "reserve", min, share, max);
		float rs = 0.25;
		DCO_Doctrine d = Doc(cmd);
		if (d)
			rs = d.m_fReserve;
		int n = Math.Round(rs * avail);
		n = Math.Min(n, max);
		if (avail >= min + 2)
			n = Math.Max(n, min);
		n = Math.ClampInt(n, 0, avail);
		LogSize(cmd, "reserve", avail, min, rs, max, 99, n);
		return n;
	}

	protected static string Uid(AICommander_BaseComponent cmd)
	{
		if (!cmd)
			return "-";
		return cmd.GetCommanderUID();
	}

	protected static void LogSize(AICommander_BaseComponent cmd, string key, int avail, int min, float share, int max, int cap, int n)
	{
		string k = Uid(cmd) + ":" + key;
		float now = DCO_Util.NowS();
		float last;
		int lastN;
		if (GetLogN().Find(k, lastN) && lastN == n && GetLogAt().Find(k, last) && now - last < 30.0)
			return;
		GetLogAt().Set(k, now);
		GetLogN().Set(k, n);
		string preset = "-";
		bool modified;
		DCO_Doctrine d = Doc(cmd);
		if (d)
		{
			preset = d.m_sPreset;
			modified = d.m_bModified;
		}
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("force_size cmd=%1 mission=%2 avail=%3 min=%4 share=%5 max=%6 cap=%7 result=%8 preset=%9", Uid(cmd), key, avail, min, share.ToString(-1, 2), max, cap, n, DCO_SettingsBaseline.LogText(preset)) + " modified=" + DCO_Util.B01(modified).ToString());
	}

	static void LogSkip(AICommander_BaseComponent cmd, string key, int avail, int min)
	{
		DCO_Perf.Count("force:skip_" + key);
		string k = Uid(cmd) + ":skip:" + key;
		float now = DCO_Util.NowS();
		float last;
		if (GetLogAt().Find(k, last) && now - last < 30.0)
			return;
		GetLogAt().Set(k, now);
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("force_skip cmd=%1 mission=%2 avail=%3 min=%4", Uid(cmd), key, avail, min));
	}

	protected static void LogCap(AICommander_BaseComponent cmd, string key, int wanted, int cap)
	{
		DCO_Perf.Count("force:cap_" + key);
		string k = Uid(cmd) + ":cap:" + key;
		float now = DCO_Util.NowS();
		float last;
		if (GetLogAt().Find(k, last) && now - last < 30.0)
			return;
		GetLogAt().Set(k, now);
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("force_cap cmd=%1 mission=%2 wanted=%3 cap=%4", Uid(cmd), key, wanted, cap));
	}
}
