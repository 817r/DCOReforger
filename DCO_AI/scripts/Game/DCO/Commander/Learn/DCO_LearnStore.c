class DCO_LearnStore
{
	protected ref map<string, vector> m_mStat = new map<string, vector>();
	protected string m_sFile;
	protected int m_iSessions;
	protected bool m_bDirty;
	protected bool m_bReadOnly;

	void SetReadOnly(bool b)
	{
		m_bReadOnly = b;
	}

	bool IsReadOnly()
	{
		return m_bReadOnly;
	}

	void DCO_LearnStore(string file)
	{
		m_sFile = file;
		Load();
	}

	int Clear()
	{
		int n = m_mStat.Count();
		m_mStat.Clear();
		m_bDirty = true;
		return n;
	}

	static string WorldName()
	{
		string world = GetGame().GetWorldFile();
		array<string> bad = {"/", "{", "}", ".", ":", " "};
		foreach (string ch : bad)
			world.Replace(ch, "_");
		return world;
	}

	static string PosKey(vector p, float cell)
	{
		return DCO_Util.XZ(Vector(Math.Round(p[0] / cell) * cell, 0, Math.Round(p[2] / cell) * cell));
	}

	float Post(string k, float prior, out float n)
	{
		vector s;
		if (!m_mStat.Find(k, s))
		{
			n = 0;
			return prior;
		}
		n = s[0];
		return (s[1] + prior * 4.0) / (s[0] + 4.0);
	}

	static float MultOf(float post, float prior)
	{
		return 1.0 + 0.25 * Math.Clamp((post - prior) * 2.0, -1.0, 1.0);
	}

	float Mult(string k, float prior, out float n)
	{
		return MultOf(Post(k, prior, n), prior);
	}

	void Add(string k, float value, float weight = 1.0)
	{
		if (m_bReadOnly)
			return;
		vector s;
		if (!m_mStat.Find(k, s))
			s = vector.Zero;
		s[0] = s[0] + weight;
		s[1] = s[1] + value * weight;
		m_mStat.Set(k, s);
		m_bDirty = true;
	}

	vector Raw(string k)
	{
		vector s;
		if (!m_mStat.Find(k, s))
			s = vector.Zero;
		return s;
	}

	void AddRaw(string k, float dn, float dv)
	{
		if (m_bReadOnly)
			return;
		vector s = Raw(k);
		s[0] = s[0] + dn;
		s[1] = s[1] + dv;
		m_mStat.Set(k, s);
		m_bDirty = true;
	}

	void Keys(string prefix, notnull array<string> outKeys)
	{
		foreach (string k, vector s : m_mStat)
		{
			if (k.StartsWith(prefix))
				outKeys.Insert(k);
		}
	}

	void Scale(string prefix, float f)
	{
		array<string> keys = {};
		Keys(prefix, keys);
		foreach (string k : keys)
		{
			vector s = m_mStat.Get(k);
			if (s[0] * f < 0.2)
				m_mStat.Remove(k);
			else
				m_mStat.Set(k, s * f);
		}
		if (!keys.IsEmpty())
			m_bDirty = true;
	}

	int PruneSmall(string prefix, float minN)
	{
		array<string> keys = {};
		Keys(prefix, keys);
		int n;
		foreach (string k : keys)
		{
			vector s = m_mStat.Get(k);
			if (s[0] < minN && s[1] <= 0)
			{
				m_mStat.Remove(k);
				n++;
			}
		}
		if (n > 0)
			m_bDirty = true;
		return n;
	}

	vector Totals(string prefix)
	{
		vector t;
		foreach (string k, vector s : m_mStat)
		{
			if (!k.StartsWith(prefix))
				continue;
			t[0] = t[0] + s[0];
			t[1] = t[1] + s[1];
			t[2] = t[2] + 1;
		}
		return t;
	}

	int Sessions()
	{
		return m_iSessions;
	}

	bool IsDirty()
	{
		return m_bDirty;
	}

	string Summary(string prefix, out int keys, out float samples)
	{
		keys = 0;
		samples = 0;
		array<string> names = {};
		array<float> devs = {};
		foreach (string k, vector s : m_mStat)
		{
			if (!k.StartsWith(prefix))
				continue;
			keys++;
			samples += s[0];
			float m = MultOf((s[1] + 2.0) / (s[0] + 4.0), 0.5);
			float dev = Math.AbsFloat(m - 1.0);
			int at = 0;
			while (at < devs.Count() && devs[at] >= dev)
				at++;
			if (at >= 5)
				continue;
			names.InsertAt(k.Substring(prefix.Length(), k.Length() - prefix.Length()) + "=" + m.ToString(-1, 2), at);
			devs.InsertAt(dev, at);
			if (names.Count() > 5)
			{
				names.Remove(5);
				devs.Remove(5);
			}
		}
		string top;
		foreach (string nm : names)
		{
			if (!top.IsEmpty())
				top += "|";
			top += nm;
		}
		if (top.IsEmpty())
			top = "-";
		return top;
	}

	static void MakeDirs(string file)
	{
		int at = file.IndexOfFrom(9, "/");
		while (at > 0)
		{
			FileIO.MakeDirectory(file.Substring(0, at));
			at = file.IndexOfFrom(at + 1, "/");
		}
	}

	protected void Load()
	{
		if (!FileIO.FileExists(m_sFile))
		{
			m_iSessions = 1;
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("learn_load file=%1 entries=0 version=1 sessions=1 fresh=1", m_sFile));
			return;
		}
		SCR_JsonLoadContext ctx = new SCR_JsonLoadContext();
		array<string> keys = {};
		array<float> ns = {};
		array<float> vs = {};
		int version;
		int sessions;
		if (!ctx.LoadFromFile(m_sFile) || !ctx.ReadValue("version", version) || version != 1 || !ctx.ReadValue("keys", keys) || !ctx.ReadValue("n", ns) || !ctx.ReadValue("v", vs) || keys.Count() != ns.Count() || keys.Count() != vs.Count())
		{
			m_iSessions = 1;
			DCO_Perf.Count("learn:load_fail");
			m_sFile = m_sFile + ".recovered.json";
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.WARN, "-", string.Format("learn_load_fail file=%1 reason=parse_or_version kept_original=1", m_sFile));
			return;
		}
		if (!ctx.ReadValue("sessions", sessions))
			sessions = 0;
		m_iSessions = sessions + 1;
		bool decay = m_iSessions % 10 == 0;
		foreach (int i, string k : keys)
		{
			float n = ns[i];
			float v = vs[i];
			if (n < 0.05 && v <= 0)
				continue;
			if (decay)
			{
				n = n * 0.5;
				v = v * 0.5;
			}
			m_mStat.Set(k, Vector(n, v, 0));
		}
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("learn_load file=%1 entries=%2 version=1 sessions=%3 decay=%4", m_sFile, keys.Count(), m_iSessions, DCO_Util.B01(decay)));
	}

	void Save(string why)
	{
		if (m_sFile.IsEmpty() || m_bReadOnly)
			return;
		int t0 = System.GetTickCount();
		MakeDirs(m_sFile);
		array<string> keys = {};
		array<float> ns = {};
		array<float> vs = {};
		foreach (string k, vector s : m_mStat)
		{
			keys.Insert(k);
			ns.Insert(s[0]);
			vs.Insert(s[1]);
		}
		SCR_JsonSaveContext ctx = new SCR_JsonSaveContext();
		ctx.WriteValue("version", 1);
		ctx.WriteValue("sessions", m_iSessions);
		ctx.WriteValue("keys", keys);
		ctx.WriteValue("n", ns);
		ctx.WriteValue("v", vs);
		bool ok = ctx.SaveToFile(m_sFile);
		m_bDirty = !ok;
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("learn_save file=%1 entries=%2 ms=%3 ok=%4 why=%5", m_sFile, keys.Count(), System.GetTickCount() - t0, DCO_Util.B01(ok), why));
	}
}
