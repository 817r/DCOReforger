class DCO_GroupPreset
{
	string m_sName;
	string m_sFaction;
	string m_sMembers;
	ResourceName m_sVehicle;
	int m_iRole;
	int m_iExperience = 1;
	int m_iMorale = 1;
	int m_iShape;
	int m_iArrival;
	string m_sMaker;
	int m_iCreated;
	int m_iSaved;
	string m_sWhere;
	string m_sMissing;
	string m_sPcSaved;

	int MemberCount()
	{
		array<string> m = {};
		Members(m);
		return m.Count();
	}

	void Members(notnull array<string> outTypes)
	{
		if (!m_sMembers.IsEmpty())
			m_sMembers.Split(",", outTypes, true);
	}

	string Content()
	{
		return string.Format("%1|%2|%3|%4|%5|%6|%7|%8", m_sFaction, m_sMembers, m_sVehicle, m_iRole, m_iExperience, m_iMorale, m_iShape, m_iArrival);
	}

	string Pack()
	{
		return string.Format("%1|%2|%3|%4|%5|%6|%7|%8|%9", m_sName, m_sFaction, m_sMembers, m_sVehicle, m_iRole, m_iExperience, m_iMorale, m_iShape, m_iArrival)
			+ string.Format("|%1|%2|%3", m_sMaker, m_iSaved, m_iCreated);
	}

	static DCO_GroupPreset Unpack(string s)
	{
		array<string> p = {};
		s.Split("|", p, false);
		if (p.Count() < 7)
			return null;
		DCO_GroupPreset g = new DCO_GroupPreset();
		g.m_sName = DCO_GroupPresets.CleanName(p[0]);
		g.m_sFaction = p[1];
		g.m_sMembers = p[2];
		g.m_sVehicle = p[3];
		g.m_iRole = p[4].ToInt();
		g.m_iExperience = p[5].ToInt();
		g.m_iMorale = p[6].ToInt();
		if (p.Count() > 8)
		{
			g.m_iShape = p[7].ToInt();
			g.m_iArrival = p[8].ToInt();
		}
		if (p.Count() > 11)
		{
			g.m_sMaker = p[9];
			g.m_iSaved = p[10].ToInt();
			g.m_iCreated = p[11].ToInt();
		}
		return g;
	}

	DCO_GroupPreset Copy()
	{
		DCO_GroupPreset c = Unpack(Pack());
		c.m_sWhere = m_sWhere;
		c.m_sMissing = m_sMissing;
		return c;
	}
}

[BaseContainerProps(configRoot: true)]
class DCO_GroupPresetConf
{
	[Attribute(desc: "Built-in group presets. Read-only in game; GMs can duplicate them.")]
	ref array<ref DCO_GroupPresetDef> m_aPresets;
}

[BaseContainerProps(), BaseContainerCustomTitleField("m_sName")]
class DCO_GroupPresetDef
{
	[Attribute(desc: "Preset name")]
	string m_sName;

	[Attribute("US", desc: "Faction key")]
	string m_sFaction;

	[Attribute(desc: "Members in order, comma separated: SL TL RIF GL AR MG AT MED RTO MRK. The first leader type leads.")]
	string m_sMembers;

	[Attribute(desc: "Optional group vehicle", params: "et")]
	ResourceName m_sVehicle;

	[Attribute("0", UIWidgets.ComboBox, enums: ParamEnumArray.FromEnum(DCO_ESpawnerRole))]
	int m_iRole;

	[Attribute("1", UIWidgets.ComboBox, enums: ParamEnumArray.FromEnum(DCO_ESpawnerExperience))]
	int m_iExperience;

	[Attribute("1", UIWidgets.ComboBox, enums: ParamEnumArray.FromEnum(DCO_ESpawnerMorale))]
	int m_iMorale;

	[Attribute("0", UIWidgets.ComboBox, enums: ParamEnumArray.FromEnum(DCO_ESpawnerShape))]
	int m_iShape;

	[Attribute("0", desc: "Arrival in minutes: 0 / 10 / 25 / 40")]
	int m_iArrival;
}

class DCO_GroupPresets
{
	protected static ref array<ref DCO_GroupPreset> s_aBuiltin;
	protected static string s_sMsg;
	protected static int s_iMsgSeq;
	protected static string s_sConflict;
	protected static bool s_bPoolOnlyNote;

	static void NotePoolAdd()
	{
		s_bPoolOnlyNote = true;
	}

	static void ClearPoolNote()
	{
		s_bPoolOnlyNote = false;
	}

	static string Dir()
	{
		return "$profile:DCO/GroupPresets/";
	}

	static string CleanName(string name)
	{
		string s = name.Trim();
		s.Replace("|", " ");
		s.Replace(";", " ");
		s.Replace("\n", " ");
		s.Replace("~", " ");
		return s.Trim();
	}

	protected static string FileKeyV1(string name)
	{
		string s = CleanName(name);
		s.ToLower();
		string allowed = "abcdefghijklmnopqrstuvwxyz0123456789";
		string outKey;
		for (int i = 0; i < s.Length() && outKey.Length() < 32; i++)
		{
			string ch = s.Substring(i, 1);
			if (allowed.Contains(ch))
				outKey += ch;
			else
				outKey += "_";
		}
		return outKey;
	}

	static string FileKey(string name)
	{
		string key = FileKeyV1(name);
		string s = CleanName(name);
		int h = 7;
		bool wide;
		for (int i = 0; i < s.Length(); i++)
		{
			int b = s.ToAscii(i);
			if (b < 0)
				b += 256;
			if (b > 127)
				wide = true;
			h = (h * 31 + b) % 1000003;
		}
		if (!wide)
			return key;
		string digits = "abcdefghijklmnopqrstuvwxyz0123456789";
		string tag;
		for (int j = 0; j < 5; j++)
		{
			tag += digits.Substring(h % 36, 1);
			h = h / 36;
		}
		if (key.Length() > 26)
			key = key.Substring(0, 26);
		return key + "_" + tag;
	}

	static bool SameKey(string a, string b)
	{
		return !FileKey(a).IsEmpty() && FileKey(a) == FileKey(b);
	}

	protected static string OldKeyPath(string name)
	{
		return Dir() + FileKeyV1(name) + ".json";
	}

	static string FileName(string name)
	{
		return FileKey(name) + ".json";
	}

	static string PathOf(string name)
	{
		return Dir() + FileName(name);
	}

	protected static string LegacyPath(string name)
	{
		string s = name.Trim();
		s.Replace("/", "_");
		s.Replace("\\", "_");
		s.Replace(":", "_");
		s.Replace("|", "_");
		s.Replace(";", "_");
		return Dir() + s + ".json";
	}

	static bool Exists(string name)
	{
		if (CleanName(name).IsEmpty())
			return false;
		return FileIO.FileExists(PathOf(name)) || FileIO.FileExists(LegacyPath(name)) || (OldKeyPath(name) != PathOf(name) && FileIO.FileExists(OldKeyPath(name)));
	}

	static bool Save(DCO_GroupPreset p)
	{
		if (!p)
			return false;
		p.m_sName = CleanName(p.m_sName);
		if (FileKey(p.m_sName).IsEmpty())
			return false;
		p.m_iSaved = System.GetUnixTime();
		if (p.m_iCreated <= 0)
			p.m_iCreated = p.m_iSaved;
		FileIO.MakeDirectory("$profile:DCO");
		FileIO.MakeDirectory(Dir());
		JsonSaveContext save = new JsonSaveContext();
		save.WriteValue("version", 2);
		save.WriteValue("name", p.m_sName);
		save.WriteValue("faction", p.m_sFaction);
		save.WriteValue("members", p.m_sMembers);
		save.WriteValue("vehicle", p.m_sVehicle);
		save.WriteValue("role", p.m_iRole);
		save.WriteValue("experience", p.m_iExperience);
		save.WriteValue("morale", p.m_iMorale);
		save.WriteValue("shape", p.m_iShape);
		save.WriteValue("arrival", p.m_iArrival);
		save.WriteValue("maker", p.m_sMaker);
		save.WriteValue("created", p.m_iCreated);
		save.WriteValue("saved", p.m_iSaved);
		bool ok = save.SaveToFile(PathOf(p.m_sName));
		string legacy = LegacyPath(p.m_sName);
		if (ok && legacy != PathOf(p.m_sName) && FileIO.FileExists(legacy))
			FileIO.DeleteFile(legacy);
		string oldKey = OldKeyPath(p.m_sName);
		if (ok && oldKey != PathOf(p.m_sName) && FileIO.FileExists(oldKey))
		{
			DCO_GroupPreset other = LoadFile(oldKey, "");
			if (other && other.m_sName == p.m_sName)
			{
				FileIO.DeleteFile(oldKey);
				DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("group_preset_key_migrated name=%1 from=%2 to=%3", p.m_sName, oldKey, PathOf(p.m_sName)));
			}
		}
		return ok;
	}

	protected static DCO_GroupPreset LoadFile(string path, string fallbackName)
	{
		if (!FileIO.FileExists(path))
			return null;
		JsonLoadContext load = new JsonLoadContext();
		if (!load.LoadFromFile(path))
			return null;
		DCO_GroupPreset p = new DCO_GroupPreset();
		load.ReadValue("name", p.m_sName);
		load.ReadValue("faction", p.m_sFaction);
		load.ReadValue("members", p.m_sMembers);
		string veh;
		load.ReadValue("vehicle", veh);
		p.m_sVehicle = veh;
		load.ReadValue("role", p.m_iRole);
		load.ReadValue("experience", p.m_iExperience);
		load.ReadValue("morale", p.m_iMorale);
		load.ReadValue("shape", p.m_iShape);
		load.ReadValue("arrival", p.m_iArrival);
		load.ReadValue("maker", p.m_sMaker);
		load.ReadValue("created", p.m_iCreated);
		load.ReadValue("saved", p.m_iSaved);
		if (p.m_iSaved <= 0)
			p.m_iSaved = p.m_iCreated;
		p.m_sName = CleanName(p.m_sName);
		if (p.m_sName.IsEmpty())
			p.m_sName = fallbackName;
		return p;
	}

	static DCO_GroupPreset Load(string name)
	{
		DCO_GroupPreset p = LoadFile(PathOf(name), CleanName(name));
		if (!p)
			p = LoadFile(LegacyPath(name), CleanName(name));
		if (!p && OldKeyPath(name) != PathOf(name))
		{
			p = LoadFile(OldKeyPath(name), "");
			if (p && p.m_sName != CleanName(name))
				p = null;
		}
		return p;
	}

	static bool Delete(string name)
	{
		bool any;
		if (FileIO.FileExists(PathOf(name)))
			any = FileIO.DeleteFile(PathOf(name));
		if (FileIO.FileExists(LegacyPath(name)))
			any = FileIO.DeleteFile(LegacyPath(name)) || any;
		if (OldKeyPath(name) != PathOf(name) && FileIO.FileExists(OldKeyPath(name)))
		{
			DCO_GroupPreset other = LoadFile(OldKeyPath(name), "");
			if (other && other.m_sName == CleanName(name))
				any = FileIO.DeleteFile(OldKeyPath(name)) || any;
		}
		return any;
	}

	static void LoadAll(notnull array<ref DCO_GroupPreset> outList)
	{
		array<string> files = SCR_FileIOHelper.FindFiles(Dir(), ".json");
		if (!files)
			return;
		array<string> seen = {};
		foreach (string f : files)
		{
			string base = FilePath.StripExtension(FilePath.StripPath(f));
			DCO_GroupPreset p = LoadFile(Dir() + base + ".json", base);
			if (!p || seen.Contains(p.m_sName))
				continue;
			seen.Insert(p.m_sName);
			outList.Insert(p);
		}
	}

	static void List(notnull array<string> outNames)
	{
		array<ref DCO_GroupPreset> all = {};
		LoadAll(all);
		foreach (DCO_GroupPreset p : all)
			outNames.Insert(p.m_sName);
		outNames.Sort();
	}

	static int CountFiles()
	{
		array<string> names = {};
		List(names);
		return names.Count();
	}

	static void Builtin(notnull array<ref DCO_GroupPreset> outList)
	{
		if (!s_aBuiltin)
		{
			s_aBuiltin = {};
			array<string> paths = {"{8C3E5A71D2B94F06}Configs/DCO/GroupPresets/DCO_BuiltinGroupPresets.conf"};
			array<string> found = SCR_FileIOHelper.FindFiles("Configs/DCO/GroupPresets/", ".conf");
			if (found)
			{
				foreach (string fp : found)
				{
					if (!fp.Contains("DCO_BuiltinGroupPresets"))
						paths.Insert(fp);
				}
			}
			foreach (string path : paths)
			{
				Resource res = BaseContainerTools.LoadContainer(path);
				if (!res || !res.IsValid())
					continue;
				DCO_GroupPresetConf conf = DCO_GroupPresetConf.Cast(BaseContainerTools.CreateInstanceFromContainer(res.GetResource().ToBaseContainer()));
				if (!conf || !conf.m_aPresets)
					continue;
				foreach (DCO_GroupPresetDef d : conf.m_aPresets)
				{
					if (!d || CleanName(d.m_sName).IsEmpty() || d.m_sMembers.IsEmpty())
						continue;
					DCO_GroupPreset p = new DCO_GroupPreset();
					p.m_sName = CleanName(d.m_sName);
					p.m_sFaction = d.m_sFaction;
					p.m_sMembers = d.m_sMembers;
					p.m_sMembers.Replace(" ", "");
					p.m_sVehicle = d.m_sVehicle;
					p.m_iRole = d.m_iRole;
					p.m_iExperience = d.m_iExperience;
					p.m_iMorale = d.m_iMorale;
					p.m_iShape = d.m_iShape;
					p.m_iArrival = d.m_iArrival;
					p.m_sMaker = "DCO_AI mod";
					p.m_sWhere = "builtin";
					s_aBuiltin.Insert(p);
				}
			}
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("group_preset_builtin loaded=%1 files=%2", s_aBuiltin.Count(), paths.Count()));
		}
		foreach (DCO_GroupPreset b : s_aBuiltin)
			outList.Insert(b);
	}

	static DCO_GroupPreset FindBuiltin(string name, string faction = "")
	{
		array<ref DCO_GroupPreset> list = {};
		Builtin(list);
		foreach (DCO_GroupPreset p : list)
		{
			if (p.m_sName == name && (faction.IsEmpty() || p.m_sFaction == faction))
				return p;
		}
		foreach (DCO_GroupPreset a : list)
		{
			if (a.m_sName == a.m_sFaction + " " + name && (faction.IsEmpty() || a.m_sFaction == faction))
			{
				DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("group_preset_builtin_alias old=%1 new=%2 faction=%3", name, a.m_sName, faction));
				return a;
			}
		}
		return null;
	}

	static bool IsBuiltinName(string name)
	{
		string n = CleanName(name);
		n.ToLower();
		array<ref DCO_GroupPreset> list = {};
		Builtin(list);
		foreach (DCO_GroupPreset p : list)
		{
			string b = p.m_sName;
			b.ToLower();
			if (b == n || SameKey(p.m_sName, name))
				return true;
		}
		return false;
	}

	static string NameProblem(string name, out string code)
	{
		code = "-";
		string n = CleanName(name);
		if (n.Length() > 40)
		{
			code = "name_too_long";
			return string.Format("Preset names can be at most 40 characters (this one has %1).", n.Length());
		}
		if (IsBuiltinName(n))
		{
			code = "builtin_name";
			return string.Format("\"%1\" is a built-in name. Pick another.", n);
		}
		return string.Empty;
	}

	static DCO_GroupPreset Find(string name, string faction = "")
	{
		DCO_GroupPreset p = Load(name);
		if (p)
			return p;
		return FindBuiltin(name, faction);
	}

	static string AgeText(int stamp)
	{
		if (stamp <= 0)
			return "unknown age";
		int age = System.GetUnixTime() - stamp;
		if (age < 3600)
			return "just now";
		if (age < 86400)
			return string.Format("%1 h ago", age / 3600);
		return string.Format("%1 days ago", age / 86400);
	}

	static ResourceName MemberPrefab(FactionKey fk, string type)
	{
		FactionManager fm = GetGame().GetFactionManager();
		if (!fm)
			return string.Empty;
		SCR_Faction fac = SCR_Faction.Cast(fm.GetFactionByKey(fk));
		if (!fac)
			return string.Empty;
		SCR_EntityCatalog cat = fac.GetFactionEntityCatalogOfType(EEntityCatalogType.CHARACTER, false);
		if (!cat)
			return string.Empty;
		array<SCR_EntityCatalogEntry> list = {};
		cat.GetEntityList(list);

		EEditableEntityLabel label = EEditableEntityLabel.ROLE_RIFLEMAN;
		array<string> prefer = {};
		array<string> avoid = {};
		if (type == "SL")
		{
			label = EEditableEntityLabel.ROLE_LEADER;
			prefer = {"_sl", "squadleader", "squad_leader"};
		}
		else if (type == "TL")
		{
			label = EEditableEntityLabel.ROLE_LEADER;
			prefer = {"_tl", "teamleader", "team_leader"};
		}
		else if (type == "AR")
		{
			label = EEditableEntityLabel.ROLE_MACHINEGUNNER;
			prefer = {"_ar", "autorifle", "automatic"};
		}
		else if (type == "MG")
		{
			label = EEditableEntityLabel.ROLE_MACHINEGUNNER;
			prefer = {"_mg", "machinegun", "machine_gun"};
			avoid = {"_ar", "autorifle", "automatic", "_amg"};
		}
		else if (type == "GL")
			label = EEditableEntityLabel.ROLE_GRENADIER;
		else if (type == "AT")
		{
			label = EEditableEntityLabel.ROLE_ANTITANK;
			prefer = {"_lat", "_at"};
		}
		else if (type == "MED")
			label = EEditableEntityLabel.ROLE_MEDIC;
		else if (type == "RTO")
			label = EEditableEntityLabel.ROLE_RADIOOPERATOR;
		else if (type == "MRK")
			label = EEditableEntityLabel.ROLE_SHARPSHOOTER;
		else
			prefer = {"rifleman"};

		ResourceName first;
		ResourceName firstClean;
		foreach (SCR_EntityCatalogEntry e : list)
		{
			if (!e || !e.IsEnabled() || !e.HasEditableEntityLabel(label))
				continue;
			string n = e.GetPrefab();
			n.ToLower();
			if (first.IsEmpty())
				first = e.GetPrefab();
			bool bad;
			foreach (string av : avoid)
			{
				if (n.Contains(av))
					bad = true;
			}
			if (!bad && firstClean.IsEmpty())
				firstClean = e.GetPrefab();
			foreach (string pf : prefer)
			{
				if (n.Contains(pf))
					return e.GetPrefab();
			}
		}
		if (!firstClean.IsEmpty())
			return firstClean;
		return first;
	}

	static ResourceName CrewPrefab(FactionKey fk)
	{
		FactionManager fm = GetGame().GetFactionManager();
		if (!fm)
			return string.Empty;
		SCR_Faction fac = SCR_Faction.Cast(fm.GetFactionByKey(fk));
		if (!fac)
			return string.Empty;
		SCR_EntityCatalog cat = fac.GetFactionEntityCatalogOfType(EEntityCatalogType.CHARACTER, false);
		if (cat)
		{
			array<SCR_EntityCatalogEntry> list = {};
			cat.GetEntityList(list);
			foreach (SCR_EntityCatalogEntry e : list)
			{
				if (!e || !e.IsEnabled())
					continue;
				string n = e.GetPrefab();
				n.ToLower();
				if (n.Contains("crew"))
					return e.GetPrefab();
			}
		}
		return MemberPrefab(fk, "RIF");
	}

	static string GearText(FactionKey fk, string type)
	{
		ResourceName prefab = MemberPrefab(fk, type);
		if (prefab.IsEmpty())
			return "not in catalog";
		Resource r = Resource.Load(prefab);
		if (!r || !r.IsValid())
			return FilePath.StripExtension(FilePath.StripPath(prefab));
		IEntityComponentSource ecs = SCR_EditableEntityComponentClass.GetEditableEntitySource(r);
		SCR_EditableEntityUIInfo info;
		if (ecs)
			info = SCR_EditableEntityComponentClass.GetInfo(ecs);
		if (info && !info.GetName().IsEmpty())
			return WidgetManager.Translate(info.GetName());
		return FilePath.StripExtension(FilePath.StripPath(prefab));
	}

	static void SeatsOf(ResourceName prefab, out int pilot, out int turret, out int cargo)
	{
		pilot = 0;
		turret = 0;
		cargo = 0;
		if (prefab.IsEmpty())
			return;
		Resource r = Resource.Load(prefab);
		if (!r || !r.IsValid())
			return;
		IEntitySource src = r.GetResource().ToEntitySource();
		if (!src)
			return;
		for (int i = 0; i < src.GetComponentCount(); i++)
		{
			IEntityComponentSource cs = src.GetComponent(i);
			if (!cs)
				continue;
			string cn = cs.GetClassName();
			if (!cn.Contains("CompartmentManager"))
				continue;
			BaseContainerList list = cs.GetObjectArray("CompartmentSlots");
			if (!list)
				continue;
			for (int k = 0; k < list.Count(); k++)
			{
				BaseContainer slot = list.Get(k);
				if (!slot)
					continue;
				string sn = slot.GetClassName();
				if (sn.Contains("Pilot"))
					pilot++;
				else if (sn.Contains("Turret"))
					turret++;
				else
					cargo++;
			}
		}
	}

	static string TypeOf(IEntity ch)
	{
		if (!ch)
			return "RIF";
		SCR_EditableEntityComponent ed = SCR_EditableEntityComponent.Cast(ch.FindComponent(SCR_EditableEntityComponent));
		string name;
		if (ch.GetPrefabData())
			name = ch.GetPrefabData().GetPrefabName();
		name.ToLower();
		SCR_EditableEntityUIInfo info;
		if (ed)
			info = SCR_EditableEntityUIInfo.Cast(ed.GetInfo());
		if (info)
		{
			if (info.HasEntityLabel(EEditableEntityLabel.ROLE_LEADER))
			{
				if (name.Contains("_tl") || name.Contains("teamleader"))
					return "TL";
				return "SL";
			}
			if (info.HasEntityLabel(EEditableEntityLabel.ROLE_MACHINEGUNNER))
			{
				if (name.Contains("_mg") || name.Contains("machinegun"))
					return "MG";
				return "AR";
			}
			if (info.HasEntityLabel(EEditableEntityLabel.ROLE_ANTITANK))
				return "AT";
			if (info.HasEntityLabel(EEditableEntityLabel.ROLE_GRENADIER))
				return "GL";
			if (info.HasEntityLabel(EEditableEntityLabel.ROLE_MEDIC))
				return "MED";
			if (info.HasEntityLabel(EEditableEntityLabel.ROLE_RADIOOPERATOR))
				return "RTO";
			if (info.HasEntityLabel(EEditableEntityLabel.ROLE_SHARPSHOOTER))
				return "MRK";
		}
		return "RIF";
	}

	static void Types(notnull array<string> outTypes)
	{
		outTypes.Copy({"SL", "TL", "RIF", "GL", "AR", "MG", "AT", "MED", "RTO", "MRK"});
	}

	static string Badge(string t)
	{
		if (t == "RIF")
			return "RF";
		if (t == "MED")
			return "MD";
		if (t == "RTO")
			return "RO";
		if (t == "MRK")
			return "DM";
		return t;
	}

	static string TypeName(string t)
	{
		if (t == "SL")
			return "Squad leader";
		if (t == "TL")
			return "Team leader";
		if (t == "AR")
			return "Automatic rifleman";
		if (t == "MG")
			return "Machine gunner";
		if (t == "GL")
			return "Grenadier";
		if (t == "AT")
			return "AT rifleman";
		if (t == "MED")
			return "Medic";
		if (t == "RTO")
			return "Radio operator";
		if (t == "MRK")
			return "Marksman";
		return "Rifleman";
	}

	static bool IsLeaderType(string t)
	{
		return t == "SL" || t == "TL";
	}

	static DCO_ESpawnerRole AutoRole(string members, bool vehicle)
	{
		array<string> m = {};
		if (!members.IsEmpty())
			members.Split(",", m, true);
		if (vehicle)
			return DCO_ESpawnerRole.MOTORIZED;
		int at;
		bool mrk;
		foreach (string t : m)
		{
			if (t == "AT")
				at++;
			if (t == "MRK")
				mrk = true;
		}
		if (at >= 2 && m.Count() <= 5)
			return DCO_ESpawnerRole.AT;
		if (mrk && m.Count() <= 4)
			return DCO_ESpawnerRole.RECON;
		if (m.Count() <= 4)
			return DCO_ESpawnerRole.FIRE_TEAM;
		return DCO_ESpawnerRole.ASSAULT;
	}

	static string AutoRoleReason(string members, bool vehicle)
	{
		array<string> m = {};
		if (!members.IsEmpty())
			members.Split(",", m, true);
		if (vehicle)
			return "it has a vehicle";
		int at;
		bool mrk;
		foreach (string t : m)
		{
			if (t == "AT")
				at++;
			if (t == "MRK")
				mrk = true;
		}
		if (at >= 2 && m.Count() <= 5)
			return string.Format("it has %1 AT riflemen in %2 men", at, m.Count());
		if (mrk && m.Count() <= 4)
			return string.Format("it has a marksman in %1 men", m.Count());
		if (m.Count() <= 4)
			return string.Format("it has %1 men", m.Count());
		return string.Format("it has %1 men and no special makeup", m.Count());
	}

	static string LeaderFirst(string members)
	{
		array<string> m = {};
		if (!members.IsEmpty())
			members.Split(",", m, true);
		int lead = m.Find("SL");
		if (lead < 0)
			lead = m.Find("TL");
		if (lead > 0)
		{
			string l = m[lead];
			m.RemoveOrdered(lead);
			m.InsertAt(l, 0);
		}
		string s;
		foreach (int i, string t : m)
		{
			if (i > 0)
				s += ",";
			s += t;
		}
		return s;
	}

	static string MissingItem(DCO_GroupPreset p, out bool vehicleOnly)
	{
		vehicleOnly = false;
		if (!p)
			return "preset";
		FactionManager fm = GetGame().GetFactionManager();
		if (!fm || !fm.GetFactionByKey(p.m_sFaction))
			return "faction " + p.m_sFaction;
		array<string> m = {};
		p.Members(m);
		foreach (string t : m)
		{
			if (MemberPrefab(p.m_sFaction, t).IsEmpty())
				return TypeName(t) + " for " + p.m_sFaction;
		}
		if (!p.m_sVehicle.IsEmpty())
		{
			Resource r = Resource.Load(p.m_sVehicle);
			if (!r || !r.IsValid())
			{
				vehicleOnly = true;
				return "vehicle " + FilePath.StripExtension(FilePath.StripPath(p.m_sVehicle));
			}
		}
		return string.Empty;
	}

	static string Validate(DCO_GroupPreset p, bool allowNoVehicle, out string code)
	{
		code = "-";
		if (!p || CleanName(p.m_sName).IsEmpty() || FileKey(p.m_sName).IsEmpty())
		{
			code = "name";
			return "The preset needs a name.";
		}
		int men = p.MemberCount();
		if (men == 0 || men > 12)
		{
			code = "men";
			return "A preset needs 1 to 12 men.";
		}
		bool vehicleOnly;
		string missing = MissingItem(p, vehicleOnly);
		if (!missing.IsEmpty())
		{
			if (vehicleOnly && allowNoVehicle)
			{
				p.m_sVehicle = string.Empty;
				return string.Empty;
			}
			code = "missing_prefab";
			if (missing.Contains("faction"))
				code = "missing_faction";
			return string.Format("\"%1\" cannot be copied as it is: %2 is from a mod that is not loaded on this server.", p.m_sName, missing);
		}
		return string.Empty;
	}

	static string UploadMode()
	{
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetInstance();
		if (!g)
			return "gm";
		return g.DCO_PresetUploads();
	}

	static int UploadLimit()
	{
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetInstance();
		if (!g)
			return 200;
		return g.DCO_PresetLimit();
	}

	static bool CanUpload(int playerID)
	{
		string mode = UploadMode();
		if (mode == "off")
			return false;
		if (mode == "admin")
		{
			PlayerManager pm = GetGame().GetPlayerManager();
			return pm && pm.HasPlayerRole(playerID, EPlayerRole.ADMINISTRATOR);
		}
		return true;
	}

	static bool IsSinglePc()
	{
		return Replication.IsServer();
	}

	static void LogOp(string op, DCO_GroupPreset p, string from, string dest, int gm)
	{
		string veh = "none";
		int men = 0;
		string name = "-";
		string fac = "-";
		if (p)
		{
			if (!p.m_sVehicle.IsEmpty())
				veh = p.m_sVehicle;
			men = p.MemberCount();
			name = p.m_sName;
			fac = p.m_sFaction;
		}
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("group_preset op=%1 name=%2 faction=%3 from=%4 men=%5 vehicle=%6 gm=%7 dest=%8", op, name, fac, from, men, veh, gm, dest));
		DCO_Perf.Count("group_preset:" + op);
	}

	static void LogCopy(string dir, string name, int gm, string result, string reason)
	{
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("group_preset_copy dir=%1 name=%2 gm=%3 result=%4 reason=%5", dir, name, gm, result, reason));
		DCO_Perf.Count("group_preset_copy:" + result);
	}

	static string UniqueName(string base)
	{
		string n = CleanName(base);
		if (!Exists(n) && !FindBuiltin(n))
			return n;
		for (int i = 2; i < 100; i++)
		{
			string c = n + " " + i.ToString();
			if (!Exists(c) && !FindBuiltin(c))
				return c;
		}
		return n + " " + System.GetUnixTime().ToString();
	}

	static void ServerOp(SCR_AttributesManagerEditorComponent mgr, int op, string pack, string arg, int playerID)
	{
		if (!Replication.IsServer())
			return;
		string gm = GetGame().GetPlayerManager().GetPlayerName(playerID);
		if (op >= 1 && op <= 4 && !CanUpload(playerID))
		{
			LogOp("blocked_no_rights_op" + op.ToString(), null, "builder_global", "server", playerID);
			Reply(mgr, 1, "Changing presets on this server is not allowed for you (server rule: " + UploadMode() + ").", "blocked");
			return;
		}
		if (op == 1 || op == 5)
		{
			DCO_GroupPreset p = DCO_GroupPreset.Unpack(pack);
			string from = arg;
			if (op == 5)
				from = "pc";
			bool replace = arg == "replace";
			bool keep = arg == "keep";
			bool noVeh = arg == "novehicle" || arg.Contains("novehicle");
			if (op == 1)
			{
				array<string> parts = {};
				arg.Split(",", parts, true);
				from = "builder_global";
				foreach (string a : parts)
				{
					if (a == "replace")
						replace = true;
					else if (a.StartsWith("from="))
						from = a.Substring(5, a.Length() - 5);
				}
			}
			if (op == 5 && !CanUpload(playerID))
			{
				string nm = "-";
				if (p)
					nm = p.m_sName;
				LogCopy("to_server", nm, playerID, "blocked", "no_rights");
				Reply(mgr, 1, "Copying presets to this server is not allowed for you (server rule: " + UploadMode() + ").", "blocked");
				return;
			}
			string code;
			string err = Validate(p, noVeh, code);
			if (!err.IsEmpty())
			{
				string bn = "-";
				if (p)
					bn = p.m_sName;
				if (op == 5)
					LogCopy("to_server", bn, playerID, "blocked", code);
				Reply(mgr, 1, err, "blocked");
				return;
			}
			string nameCode;
			string nameErr = NameProblem(p.m_sName, nameCode);
			if (!nameErr.IsEmpty())
			{
				DCO_Perf.Count("group_preset_refused:" + nameCode);
				DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("group_preset_refused op=%1 name=%2 reason=%3 gm=%4", op, p.m_sName, nameCode, gm));
				if (op == 5)
					LogCopy("to_server", p.m_sName, playerID, "blocked", nameCode);
				Reply(mgr, 1, nameErr, "blocked");
				return;
			}
			p.m_sMembers = LeaderFirst(p.m_sMembers);
			bool exists = Exists(p.m_sName);
			if (exists)
			{
				DCO_GroupPreset holder = Load(p.m_sName);
				if (!holder || holder.m_sName != p.m_sName)
					DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("group_preset_key_collision name=%1 key=%2 gm=%3", p.m_sName, FileKey(p.m_sName), gm));
			}
			if (exists && !replace && !keep)
			{
				DCO_GroupPreset old = Load(p.m_sName);
				if (old && old.Content() == p.Content())
				{
					if (op == 5)
						LogCopy("to_server", p.m_sName, playerID, "ok", "same");
					Reply(mgr, 1, string.Format("\"%1\" is already the same on this server.", p.m_sName), "ok");
					return;
				}
				Reply(mgr, 3, p.m_sName, pack);
				return;
			}
			if (!exists && CountFiles() >= UploadLimit())
			{
				if (op == 5)
					LogCopy("to_server", p.m_sName, playerID, "blocked", "limit");
				Reply(mgr, 1, string.Format("This server already has %1 presets, the most it allows. Delete one first.", UploadLimit()), "blocked");
				return;
			}
			string result = "ok";
			if (keep && exists)
			{
				p.m_sName = UniqueName(p.m_sName);
				result = "kept_both";
			}
			else if (exists)
				result = "replaced";
			if (p.m_sMaker.IsEmpty())
				p.m_sMaker = gm;
			bool ok = Save(p);
			string opName = "save";
			if (result == "replaced")
				opName = "replace";
			if (op == 5)
				LogCopy("to_server", p.m_sName, playerID, result, "-");
			else
				LogOp(opName, p, from, "server", playerID);
			if (!ok)
			{
				Reply(mgr, 1, "The server could not write the preset file.", "blocked");
				return;
			}
			Publish();
			Reply(mgr, 1, string.Format("\"%1\" saved on this server.", p.m_sName), result);
			return;
		}
		if (op == 2)
		{
			DCO_GroupPreset d = Load(pack);
			bool del = Delete(pack);
			LogOp("delete", d, "builder_global", "server", playerID);
			Publish();
			if (del)
				Reply(mgr, 1, string.Format("\"%1\" deleted from this server. Spawners that use it keep their copy.", pack), "ok");
			return;
		}
		if (op == 3)
		{
			string newName = CleanName(arg);
			DCO_GroupPreset rp = Load(pack);
			if (!rp || newName.IsEmpty() || FileKey(newName).IsEmpty())
			{
				Reply(mgr, 1, "Rename failed: give a new name.", "blocked");
				return;
			}
			string renCode;
			string renErr = NameProblem(newName, renCode);
			if (!renErr.IsEmpty())
			{
				DCO_Perf.Count("group_preset_refused:" + renCode);
				DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("group_preset_refused op=rename name=%1 reason=%2 gm=%3", newName, renCode, gm));
				Reply(mgr, 1, "Rename failed: " + renErr, "blocked");
				return;
			}
			if (Exists(newName) && FileKey(newName) != FileKey(pack))
			{
				DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("group_preset_key_collision op=rename name=%1 key=%2 gm=%3", newName, FileKey(newName), gm));
				Reply(mgr, 1, string.Format("Rename failed: \"%1\" already exists.", newName), "blocked");
				return;
			}
			rp.m_sName = newName;
			if (Save(rp) && FileKey(newName) != FileKey(pack))
				Delete(pack);
			LogOp("rename", rp, "builder_global", "server", playerID);
			Publish();
			Reply(mgr, 1, string.Format("Renamed to \"%1\".", newName), "ok");
			return;
		}
		if (op == 4)
		{
			DCO_GroupPreset src = Find(pack);
			if (!src)
				return;
			DCO_GroupPreset dup = src.Copy();
			dup.m_sName = UniqueName(src.m_sName + " (copy)");
			dup.m_sMaker = gm;
			dup.m_iCreated = 0;
			if (CountFiles() >= UploadLimit())
			{
				Reply(mgr, 1, string.Format("This server already has %1 presets, the most it allows. Delete one first.", UploadLimit()), "blocked");
				return;
			}
			Save(dup);
			LogOp("duplicate", dup, "builder_global", "server", playerID);
			Publish();
			Reply(mgr, 1, string.Format("Duplicated as \"%1\".", dup.m_sName), "ok");
			return;
		}
		if (op == 6)
		{
			DCO_GroupPreset fp = Find(pack);
			if (!fp)
			{
				Reply(mgr, 1, "That preset is no longer on the server.", "blocked");
				return;
			}
			Reply(mgr, 2, arg, fp.Pack());
		}
	}

	protected static void Reply(SCR_AttributesManagerEditorComponent mgr, int kind, string a, string b)
	{
		if (mgr)
			mgr.DCO_PresetReply(kind, a, b);
		else
			ClientReply(kind, a, b);
	}

	static void ClientReply(int kind, string a, string b)
	{
		if (kind == 1)
		{
			if (s_bPoolOnlyNote && b == "blocked")
			{
				SetMsg(a + " Not saved as a preset; the group was added to this spawner's pool only.");
				DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", "preset_save_refused_pool_only reason=" + a);
			}
			else
				SetMsg(a);
			s_bPoolOnlyNote = false;
			return;
		}
		if (kind == 3)
		{
			s_bPoolOnlyNote = false;
			s_sConflict = b;
			SetMsg(string.Format("\"%1\" already exists on this server and is different.", a));
			return;
		}
		if (kind == 2)
		{
			DCO_GroupPreset p = DCO_GroupPreset.Unpack(b);
			if (!p)
				return;
			string mode = a;
			string result = "ok";
			if (Exists(p.m_sName))
			{
				if (mode == "keep")
				{
					p.m_sName = UniqueName(p.m_sName);
					result = "kept_both";
				}
				else
					result = "replaced";
			}
			bool ok = Save(p);
			if (!ok)
				result = "blocked";
			LogCopy("to_pc", p.m_sName, LocalPlayerId(), result, "-");
			if (ok)
				SetMsg(string.Format("\"%1\" copied to your PC.", p.m_sName));
			else
				SetMsg("Your PC could not write the preset file.");
		}
	}

	static int VerifiedGM(SCR_EditorManagerEntity em, int claimed, string what)
	{
		int pid = -1;
		if (em)
			pid = em.GetPlayerID();
		if (!em || em.IsLimited())
		{
			DCO_Perf.Count("gm_rpc:refused");
			DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.WARN, "-", string.Format("gm_rpc_refused what=%1 sender=%2 claimed=%3 why=not_gm", what, pid, claimed));
			return -1;
		}
		if (claimed != pid)
		{
			DCO_Perf.Count("gm_rpc:id_mismatch");
			DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.WARN, "-", string.Format("gm_rpc_id_mismatch what=%1 sender=%2 claimed=%3 used=sender", what, pid, claimed));
		}
		return pid;
	}

	static int LocalPlayerId()
	{
		if (GetGame().GetPlayerController())
			return GetGame().GetPlayerController().GetPlayerId();
		return -1;
	}

	static void SetMsg(string m)
	{
		s_sMsg = m;
		s_iMsgSeq++;
	}

	static string Msg()
	{
		return s_sMsg;
	}

	static int MsgSeq()
	{
		return s_iMsgSeq;
	}

	static string TakeConflict()
	{
		string c = s_sConflict;
		s_sConflict = string.Empty;
		return c;
	}

	static string PeekConflict()
	{
		return s_sConflict;
	}

	static bool SaveFromGroup(SCR_AIGroup grp, int playerID)
	{
		if (!grp || !Replication.IsServer())
			return false;
		DCO_GroupPreset p = FromGroup(grp);
		if (!p)
			return false;
		if (p.m_sName.Length() > 40)
		{
			string longName = p.m_sName;
			p.m_sName = p.m_sName.Substring(0, 40).Trim();
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("group_preset_renamed reason=name_too_long from=%1 to=%2", longName, p.m_sName));
		}
		string why;
		string code;
		if (!CanUpload(playerID))
			why = "Saving presets on this server is not allowed for you (server rule: " + UploadMode() + ").";
		else if (CountFiles() >= UploadLimit())
			why = string.Format("This server already has %1 presets, the most it allows. Delete one first.", UploadLimit());
		else
			why = Validate(p, true, code);
		if (!why.IsEmpty())
		{
			LogOp("blocked_world_save", p, "world", "server", playerID);
			DCO_PlayerComms.SendToGameMasters("DCO", why);
			return false;
		}
		p.m_sName = UniqueName(p.m_sName);
		p.m_sMaker = GetGame().GetPlayerManager().GetPlayerName(playerID);
		bool ok = Save(p);
		LogOp("save", p, "world", "server", playerID);
		if (ok)
		{
			string where = "on this server";
			if (RplSession.Mode() == RplMode.None)
				where = "on your PC";
			DCO_PlayerComms.SendToGameMasters("DCO", string.Format("Saved \"%1\" %2 (%3 men). Rename or edit it in Global › Group presets. Gear changed by hand is not saved.", p.m_sName, where, p.MemberCount()));
		}
		Publish();
		return ok;
	}

	static DCO_GroupPreset FromGroup(SCR_AIGroup grp)
	{
		if (!grp)
			return null;
		array<AIAgent> agents = {};
		grp.GetAgents(agents);
		if (agents.IsEmpty())
			return null;
		string members;
		ResourceName vehicle;
		foreach (AIAgent a : agents)
		{
			IEntity ch = a.GetControlledEntity();
			if (!ch)
				continue;
			if (!members.IsEmpty())
				members += ",";
			members += TypeOf(ch);
			if (vehicle.IsEmpty())
			{
				IEntity veh = DCO_VehicleCombat.GetVehicle(ch);
				if (veh && veh.GetPrefabData())
					vehicle = veh.GetPrefabData().GetPrefabName();
			}
		}
		DCO_GroupPreset p = new DCO_GroupPreset();
		string base = grp.GetName();
		if (base.IsEmpty())
			base = "Group";
		if (grp.GetFaction())
			p.m_sFaction = grp.GetFaction().GetFactionKey();
		p.m_sName = CleanName(p.m_sFaction + " " + base + " (saved)");
		p.m_sMembers = LeaderFirst(members);
		p.m_sVehicle = vehicle;
		p.m_iRole = DCO_ESpawnerRole.AUTO;
		p.m_iExperience = 1;
		p.m_iMorale = 1;
		return p;
	}

	static void Publish()
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (mgr)
			mgr.DCO_RefreshGroupPresetList();
	}

	static string EntryText(DCO_GroupPreset p, string src)
	{
		return src + "|" + p.m_sMissing + "|" + p.Pack();
	}

	static void ParseList(string joined, notnull array<ref DCO_GroupPreset> outList, out string rule, out int limit, out int serverCount)
	{
		rule = "gm";
		limit = 200;
		serverCount = 0;
		if (joined.IsEmpty())
			return;
		array<string> lines = {};
		joined.Split("\n", lines, true);
		foreach (string ln : lines)
		{
			array<string> f = {};
			ln.Split("|", f, false);
			if (f.Count() < 3)
				continue;
			if (f[0] == "rule")
			{
				rule = f[1];
				limit = f[2].ToInt();
				if (f.Count() > 3)
					serverCount = f[3].ToInt();
				continue;
			}
			string rest;
			for (int i = 2; i < f.Count(); i++)
			{
				if (i > 2)
					rest += "|";
				rest += f[i];
			}
			DCO_GroupPreset p = DCO_GroupPreset.Unpack(rest);
			if (!p)
				continue;
			p.m_sWhere = f[0];
			p.m_sMissing = f[1];
			outList.Insert(p);
		}
	}
}

modded class AICommander_ManagerComponent
{
	[RplProp()]
	protected string m_sDCOGroupPresetList;

	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if (Replication.IsServer() && GetGame().InPlayMode())
			GetGame().GetCallqueue().CallLater(DCO_RefreshGroupPresetList, 2000, false);
	}

	void DCO_RefreshGroupPresetList()
	{
		if (!Replication.IsServer())
			return;
		array<ref DCO_GroupPreset> all = {};
		DCO_GroupPresets.LoadAll(all);
		int cap = DCO_GroupPresets.UploadLimit();
		if (cap <= 0)
			cap = 200;
		if (all.Count() > cap)
		{
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.WARN, "-", string.Format("group_preset_list_capped files=%1 sent=%2", all.Count(), cap));
			all.Resize(cap);
		}
		string joined = string.Format("rule|%1|%2|%3", DCO_GroupPresets.UploadMode(), DCO_GroupPresets.UploadLimit(), all.Count());
		foreach (DCO_GroupPreset p : all)
		{
			bool vehOnly;
			p.m_sMissing = DCO_GroupPresets.MissingItem(p, vehOnly);
			joined += "\n" + DCO_GroupPresets.EntryText(p, "server");
		}
		array<ref DCO_GroupPreset> built = {};
		DCO_GroupPresets.Builtin(built);
		foreach (DCO_GroupPreset b : built)
		{
			bool bVehOnly;
			b.m_sMissing = DCO_GroupPresets.MissingItem(b, bVehOnly);
			joined += "\n" + DCO_GroupPresets.EntryText(b, "builtin");
		}
		if (joined == m_sDCOGroupPresetList)
			return;
		m_sDCOGroupPresetList = joined;
		Replication.BumpMe();
	}

	string DCO_GetGroupPresetList()
	{
		return m_sDCOGroupPresetList;
	}
}

modded class SCR_AttributesManagerEditorComponent
{
	void DCO_PresetRequest(int op, string pack, string arg)
	{
		Rpc(DCO_RpcPresetOp, op, pack, arg, DCO_GroupPresets.LocalPlayerId());
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void DCO_RpcPresetOp(int op, string pack, string arg, int playerID)
	{
		int pid = DCO_GroupPresets.VerifiedGM(GetManager(), playerID, "preset_op_" + op.ToString());
		if (pid < 0)
			return;
		DCO_GroupPresets.ServerOp(this, op, pack, arg, pid);
	}

	void DCO_PresetReply(int kind, string a, string b)
	{
		Rpc(DCO_RpcPresetReply, kind, a, b);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void DCO_RpcPresetReply(int kind, string a, string b)
	{
		DCO_GroupPresets.ClientReply(kind, a, b);
	}
}

[BaseContainerProps(), SCR_BaseContainerCustomTitleUIInfo("m_Info")]
class DCO_SaveGroupPresetContextAction : SCR_SelectedEntitiesContextAction
{
	override int GetParam()
	{
		return GetGame().GetPlayerController().GetPlayerId();
	}

	override bool CanBeShown(SCR_EditableEntityComponent selectedEntity, vector cursorWorldPosition, int flags)
	{
		if (!selectedEntity || !selectedEntity.GetOwner())
			return false;
		SCR_AIGroup grp = SCR_AIGroup.Cast(selectedEntity.GetOwner());
		return grp && !grp.IsPlayable() && grp.GetAgentsCount() > 0;
	}

	override bool CanBePerformed(SCR_EditableEntityComponent selectedEntity, vector cursorWorldPosition, int flags)
	{
		return CanBeShown(selectedEntity, cursorWorldPosition, flags);
	}

	override void Perform(SCR_EditableEntityComponent hoveredEntity, notnull set<SCR_EditableEntityComponent> selectedEntities, vector cursorWorldPosition, int flags, int param = -1)
	{
		foreach (SCR_EditableEntityComponent entity : selectedEntities)
		{
			if (!entity)
				continue;
			SCR_AIGroup grp = SCR_AIGroup.Cast(entity.GetOwner());
			if (grp)
				DCO_GroupPresets.SaveFromGroup(grp, param);
		}
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GroupPresetRuleAttribute : SCR_BaseFloatValueHolderEditorAttribute
{
	protected static int RuleIndex(string rule)
	{
		if (rule == "admin")
			return 1;
		if (rule == "off")
			return 2;
		return 0;
	}

	override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
	{
		m_aValues.Clear();
		array<string> labels = {"GMs", "Admins", "Nobody"};
		foreach (int i, string l : labels)
		{
			SCR_EditorAttributeFloatStringValueHolder value = new SCR_EditorAttributeFloatStringValueHolder();
			value.SetName(l);
			value.SetFloatValue(i);
			m_aValues.Insert(value);
		}
		return super.GetEntries(outEntries);
	}

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!DCO_GlobalFloatAttribute.GetGlobal(item))
			return null;
		string rule = DCO_GroupPresets.UploadMode();
		if (!Replication.IsServer())
		{
			AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
			if (mgr)
			{
				array<ref DCO_GroupPreset> tmp = {};
				int limit;
				int count;
				DCO_GroupPresets.ParseList(mgr.DCO_GetGroupPresetList(), tmp, rule, limit, count);
			}
		}
		return SCR_BaseEditorAttributeVar.CreateFloat(RuleIndex(rule));
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("group_preset_rule ignored=set_by_server rule=%1 gm=%2", DCO_GroupPresets.UploadMode(), playerID));
	}
}
