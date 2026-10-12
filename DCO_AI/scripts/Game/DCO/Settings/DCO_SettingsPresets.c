class DCO_SettingsCodec
{
	static string Num(float f)
	{
		if (Math.AbsFloat(f - Math.Round(f)) < 0.00001)
			return ((int)Math.Round(f)).ToString();
		string s = f.ToString(-1, 4);
		while (s.Length() > 1 && s.EndsWith("0"))
		{
			s = s.Substring(0, s.Length() - 1);
		}
		if (s.EndsWith("."))
			s = s.Substring(0, s.Length() - 1);
		return s;
	}

	static string Entry(string key, vector v)
	{
		string s = key + "=" + Num(v[0]);
		if (v[1] != 0 || v[2] != 0)
			s += "," + Num(v[1]) + "," + Num(v[2]);
		return s;
	}

	static void Decode(string content, notnull array<string> outKeys, notnull array<vector> outValues)
	{
		array<string> entries = {};
		content.Split(";", entries, true);
		foreach (string e : entries)
		{
			int eq = e.IndexOf("=");
			if (eq <= 0)
				continue;
			string key = e.Substring(0, eq);
			key = key.Trim();
			string raw = e.Substring(eq + 1, e.Length() - eq - 1);
			array<string> parts = {};
			raw.Split(",", parts, false);
			vector v;
			for (int i = 0; i < 3 && i < parts.Count(); i++)
			{
				v[i] = parts[i].ToFloat();
			}
			outKeys.Insert(key);
			outValues.Insert(v);
		}
	}

	static string ExportText(string scope, string content)
	{
		return "DCO1|" + scope + "|" + content;
	}

	static bool ImportText(string text, out string scope, out string content, out string error)
	{
		text = text.Trim();
		if (!text.StartsWith("DCO1|"))
		{
			if (text.StartsWith("DCO"))
				error = "version";
			else
				error = "format";
			return false;
		}
		string rest = text.Substring(5, text.Length() - 5);
		int bar = rest.IndexOf("|");
		if (bar <= 0)
		{
			error = "format";
			return false;
		}
		scope = rest.Substring(0, bar);
		content = rest.Substring(bar + 1, rest.Length() - bar - 1);
		if (content.IsEmpty() || !content.Contains("="))
		{
			error = "empty";
			return false;
		}
		return true;
	}
}

class DCO_SettingsPresets
{
	static bool ValidScope(string scope)
	{
		return scope == "Global" || scope == "Group" || scope == "Unit" || scope == "AI" || scope.StartsWith("Faction_");
	}

	static void MigrateLegacy()
	{
		array<string> files = SCR_FileIOHelper.FindFiles(Dir("AI"), ".json");
		if (!files || files.IsEmpty())
			return;
		int toGroup = 0;
		int toUnit = 0;
		foreach (string f : files)
		{
			string name = FilePath.StripExtension(FilePath.StripPath(f));
			string content;
			if (name.IsEmpty() || !Load("AI", name, content))
				continue;
			string target = "Unit";
			if (content.Contains("m:"))
				target = "Group";
			if (!Exists(target, name) && !Save(target, name, content))
				continue;
			Delete("AI", name);
			if (target == "Group")
				toGroup++;
			else
				toUnit++;
		}
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", "dco_preset_migrate from=AI group=" + toGroup.ToString() + " unit=" + toUnit.ToString());
	}

	static string Dir(string scope)
	{
		return "$profile:DCO/Presets/" + scope + "/";
	}

	static string PathOf(string scope, string name)
	{
		return Dir(scope) + name + ".json";
	}

	static void List(string scope, notnull array<string> outNames)
	{
		if (!ValidScope(scope))
			return;
		if (scope == "Group" || scope == "Unit")
			MigrateLegacy();
		array<string> files = SCR_FileIOHelper.FindFiles(Dir(scope), ".json");
		if (!files)
			return;
		foreach (string f : files)
		{
			string n = FilePath.StripExtension(FilePath.StripPath(f));
			if (!n.IsEmpty() && outNames.Find(n) < 0)
				outNames.Insert(n);
		}
		outNames.Sort();
	}

	static bool Exists(string scope, string name)
	{
		return ValidScope(scope) && !name.IsEmpty() && FileIO.FileExists(PathOf(scope, name));
	}

	static bool Save(string scope, string name, string content)
	{
		if (!ValidScope(scope) || name.IsEmpty())
			return false;
		FileIO.MakeDirectory("$profile:DCO");
		FileIO.MakeDirectory("$profile:DCO/Presets");
		FileIO.MakeDirectory(Dir(scope));
		SCR_JsonSaveContext save = new SCR_JsonSaveContext();
		save.WriteValue("scope", scope);
		save.WriteValue("version", 2);
		save.WriteValue("values", content);
		return save.SaveToFile(PathOf(scope, name));
	}

	static bool Load(string scope, string name, out string content)
	{
		if (!Exists(scope, name))
			return false;
		SCR_JsonLoadContext load = new SCR_JsonLoadContext();
		if (!load.LoadFromFile(PathOf(scope, name)))
			return false;
		string fileScope;
		load.ReadValue("scope", fileScope);
		if (!fileScope.IsEmpty() && fileScope != scope)
			return false;
		if (!load.ReadValue("values", content) || content.IsEmpty())
			return false;
		int version = 1;
		load.ReadValue("version", version);
		if (version < 2)
			content = MigrateAim(content, scope + "/" + name);
		return !content.IsEmpty();
	}

	static string MigrateAim(string content, string file)
	{
		array<string> entries = {};
		content.Split(";", entries, true);
		string kept;
		int dropped = 0;
		foreach (string e : entries)
		{
			string key = e;
			if (key.StartsWith("m:"))
				key = key.Substring(2, key.Length() - 2);
			if (key.StartsWith("SCR_AIGlobalAIMAttribute=") || key.StartsWith("SCR_AIIndividualAIMAttribute=") || key.StartsWith("SCR_AIGlobalAIMTimeAttribute=") || key.StartsWith("SCR_AIIndividualAIMTimeAttribute="))
			{
				dropped++;
				continue;
			}
			if (!kept.IsEmpty())
				kept += ";";
			kept += e;
		}
		if (dropped > 0)
			DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.WARN, "-", string.Format("aim_cfg_migrated file=%1 dropped=%2 model=angle", file, dropped));
		return kept;
	}

	static bool Delete(string scope, string name)
	{
		if (!Exists(scope, name))
			return false;
		return FileIO.DeleteFile(PathOf(scope, name));
	}
}

class DCO_SettingsChangeLog
{
	protected static ref array<string> s_aEntries;

	static string Stamp()
	{
		int h, m, s;
		System.GetHourMinuteSecond(h, m, s);
		return h.ToString(2) + ":" + m.ToString(2) + ":" + s.ToString(2);
	}

	static string PlayerName(int playerId)
	{
		PlayerManager pm = GetGame().GetPlayerManager();
		string name;
		if (pm)
			name = pm.GetPlayerName(playerId);
		if (name.IsEmpty())
			name = "Player " + playerId.ToString();
		return name;
	}

	static void Add(int playerId, string target, string setting, string oldValue, string newValue)
	{
		string line = Stamp() + "|" + PlayerName(playerId) + "|" + target + "|" + setting + "|" + oldValue + "|" + newValue;
		if (!s_aEntries)
			s_aEntries = {};
		s_aEntries.Insert(line);
		while (s_aEntries.Count() > 500)
		{
			s_aEntries.RemoveOrdered(0);
		}
		FileIO.MakeDirectory("$profile:DCO");
		FileIO.MakeDirectory("$profile:DCO/Logs");
		string path = "$profile:DCO/Logs/settings_changes.log";
		FileHandle f = FileIO.OpenFile(path, FileMode.APPEND);
		if (!f)
			f = FileIO.OpenFile(path, FileMode.WRITE);
		if (f)
		{
			int y, mo, d;
			System.GetYearMonthDay(y, mo, d);
			f.WriteLine(y.ToString() + "-" + mo.ToString(2) + "-" + d.ToString(2) + " " + line);
			f.Close();
		}
	}

	static string Recent(string target, int maxLines)
	{
		if (!s_aEntries)
			return string.Empty;
		string outText;
		int n = 0;
		for (int i = s_aEntries.Count() - 1; i >= 0 && n < maxLines; i--)
		{
			string e = s_aEntries[i];
			if (!target.IsEmpty())
			{
				array<string> f = {};
				e.Split("|", f, false);
				if (f.Count() < 3 || f[2] != target)
					continue;
			}
			if (!outText.IsEmpty())
				outText += "\n";
			outText += e;
			n++;
		}
		return outText;
	}
}
