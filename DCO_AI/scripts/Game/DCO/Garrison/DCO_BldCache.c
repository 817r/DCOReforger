class DCO_BldCache
{
	static string Id(ResourceName prefab)
	{
		string s = prefab;
		int a = s.IndexOf("{");
		int b = s.IndexOf("}");
		if (a >= 0 && b > a + 1)
			return s.Substring(a + 1, b - a - 1);
		s = FilePath.StripPath(s);
		s.Replace(".", "_");
		return s;
	}

	static string Name(ResourceName prefab)
	{
		return FilePath.StripExtension(FilePath.StripPath(prefab));
	}

	static string File(ResourceName prefab)
	{
		return "$profile:DCO/BuildingCache/" + Id(prefab) + ".json";
	}

	protected static ref map<string, ref SCR_JsonLoadContext> s_mPre;

	static void Preload()
	{
		int t0 = System.GetTickCount();
		s_mPre = new map<string, ref SCR_JsonLoadContext>();
		array<string> files = SCR_FileIOHelper.FindFiles("$profile:DCO/BuildingCache/", ".json");
		int failed;
		if (files)
		{
			foreach (string pf : files)
			{
				string id = FilePath.StripExtension(FilePath.StripPath(pf));
				if (id.IsEmpty())
					continue;
				SCR_JsonLoadContext pctx = new SCR_JsonLoadContext();
				if (pctx.LoadFromFile("$profile:DCO/BuildingCache/" + id + ".json"))
					s_mPre.Set(id, pctx);
				else
				{
					s_mPre.Set(id, null);
					failed++;
				}
			}
		}
		DCO_Perf.Count("bldcache:preload");
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("bldcache_preload files=%1 failed=%2 ms=%3", s_mPre.Count(), failed, System.GetTickCount() - t0));
	}

	static DCO_GarrisonPrefabSlots Load(ResourceName prefab, IEntity building)
	{
		string file = File(prefab);
		SCR_JsonLoadContext ctx;
		bool okFile;
		if (s_mPre)
		{
			if (!s_mPre.Contains(Id(prefab)))
			{
				DCO_Perf.Count("bldcache:pre_miss");
				return null;
			}
			ctx = s_mPre.Get(Id(prefab));
			okFile = ctx != null;
			DCO_Perf.Count("bldcache:pre_hit");
		}
		else
		{
			if (!FileIO.FileExists(file))
				return null;
			ctx = new SCR_JsonLoadContext();
			okFile = ctx.LoadFromFile(file);
		}
		if (!ctx)
			ctx = new SCR_JsonLoadContext();

		vector mins, maxs;
		building.GetBounds(mins, maxs);

		int version;
		array<float> bb = {};
		array<float> f = {};
		if (!okFile || !ctx.ReadValue("version", version) || version < 1 || version > 2 || !ctx.ReadValue("bbox", bb) || bb.Count() != 6 || !ctx.ReadValue("slots", f) || f.Count() % 13 != 0)
		{
			DCO_Perf.Count("bldcache:load_fail");
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.WARN, "-", string.Format("bldcache_load_fail prefab=%1 id=%2 reason=parse_or_version", Name(prefab), Id(prefab)));
			return null;
		}

		for (int a = 0; a < 3; a++)
		{
			if (Math.AbsFloat(bb[a] - mins[a]) > 0.05 || Math.AbsFloat(bb[a + 3] - maxs[a]) > 0.05)
			{
				DCO_Perf.Count("bldcache:fingerprint");
				DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.WARN, "-", string.Format("bldcache_load_fail prefab=%1 id=%2 reason=fingerprint", Name(prefab), Id(prefab)));
				return null;
			}
		}

		DCO_GarrisonPrefabSlots p = new DCO_GarrisonPrefabSlots();
		p.m_vMins = mins;
		p.m_vMaxs = maxs;
		bool grid = false;
		bool complete = false;
		int misses = 0;
		if (!ctx.ReadValue("grid", grid))
			grid = false;
		if (!ctx.ReadValue("complete", complete))
			complete = false;
		if (!ctx.ReadValue("misses", misses))
			misses = 0;
		p.m_bGrid = grid;
		p.m_bComplete = complete;
		p.m_iMisses = misses;

		array<float> gone = {};
		if (ctx.ReadValue("gone", gone) && gone.Count() % 4 == 0)
		{
			int ses = DCO_BldLearn.Session();
			int expired = 0;
			for (int g = 0; g + 3 < gone.Count(); g += 4)
			{
				int at = Math.Round(gone[g + 3]);
				if (ses > 0 && ses - at >= 10)
				{
					expired++;
					continue;
				}
				p.m_aGone.Insert(Vector(gone[g], gone[g + 1], gone[g + 2]));
				p.m_aGoneSes.Insert(at);
			}
			if (expired > 0)
			{
				p.m_bDirty = true;
				DCO_Perf.Count("bldcache:gone_expire");
				DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("bldcache_gone_expire prefab=%1 id=%2 expired=%3 kept=%4 session=%5", Name(prefab), Id(prefab), expired, p.m_aGone.Count(), ses));
			}
		}

		int failReset;
		for (int i = 0; i + 12 < f.Count(); i += 13)
		{
			DCO_GarrisonSlot s = new DCO_GarrisonSlot();
			s.m_vLocalPos = Vector(f[i], f[i + 1], f[i + 2]);
			s.m_fLocalYaw = f[i + 3];
			s.m_fArc = f[i + 4];
			int stance = Math.Round(f[i + 5]);
			int type = Math.Round(f[i + 6]);
			int floor = Math.Round(f[i + 7]);
			int fails = Math.Round(f[i + 12]);
			s.m_eStance = stance;
			s.m_eType = type;
			s.m_iFloor = floor;
			s.m_fScore = f[i + 8];
			s.m_fWinDist = f[i + 9];
			s.m_fCover = f[i + 10];
			s.m_fFire = f[i + 11];
			if (version < 2 && fails > 0)
			{
				fails = 0;
				failReset++;
			}
			s.m_iFailCount = fails;
			p.m_aSlots.Insert(s);
		}
		if (failReset > 0)
		{
			p.m_bDirty = true;
			DCO_Perf.Count("bldcache:fail_reset_v1");
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("bldcache_fail_reset prefab=%1 id=%2 slots_reset=%3 reason=v1_counted_transient_timeouts", Name(prefab), Id(prefab), failReset));
		}

		DCO_Perf.Count("bldcache:load");
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("bldcache_load prefab=%1 id=%2 slots=%3 grid=%4 complete=%5 misses=%6 gone=%7", Name(prefab), Id(prefab), p.m_aSlots.Count(), DCO_Util.B01(p.m_bGrid), DCO_Util.B01(p.m_bComplete), p.m_iMisses, p.m_aGone.Count()));
		return p;
	}

	static bool Save(ResourceName prefab, DCO_GarrisonPrefabSlots p)
	{
		string file = File(prefab);
		DCO_LearnStore.MakeDirs(file);

		array<float> bb = {};
		for (int a = 0; a < 3; a++)
			bb.Insert(p.m_vMins[a]);
		for (int a2 = 0; a2 < 3; a2++)
			bb.Insert(p.m_vMaxs[a2]);

		array<float> f = {};
		foreach (DCO_GarrisonSlot s : p.m_aSlots)
		{
			f.Insert(s.m_vLocalPos[0]);
			f.Insert(s.m_vLocalPos[1]);
			f.Insert(s.m_vLocalPos[2]);
			f.Insert(s.m_fLocalYaw);
			f.Insert(s.m_fArc);
			int stance = s.m_eStance;
			int type = s.m_eType;
			f.Insert(stance);
			f.Insert(type);
			f.Insert(s.m_iFloor);
			f.Insert(s.m_fScore);
			f.Insert(s.m_fWinDist);
			f.Insert(s.m_fCover);
			f.Insert(s.m_fFire);
			f.Insert(s.m_iFailCount);
		}

		array<float> gone = {};
		foreach (int gi, vector g : p.m_aGone)
		{
			gone.Insert(g[0]);
			gone.Insert(g[1]);
			gone.Insert(g[2]);
			int at = 0;
			if (p.m_aGoneSes.IsIndexValid(gi))
				at = p.m_aGoneSes[gi];
			gone.Insert(at);
		}

		SCR_JsonSaveContext ctx = new SCR_JsonSaveContext();
		ctx.WriteValue("version", 2);
		ctx.WriteValue("prefab", Name(prefab));
		ctx.WriteValue("bbox", bb);
		ctx.WriteValue("grid", p.m_bGrid);
		ctx.WriteValue("complete", p.m_bComplete);
		ctx.WriteValue("misses", p.m_iMisses);
		ctx.WriteValue("slots", f);
		ctx.WriteValue("gone", gone);
		bool ok = ctx.SaveToFile(file);
		if (ok)
			p.m_bDirty = false;
		return ok;
	}
}
