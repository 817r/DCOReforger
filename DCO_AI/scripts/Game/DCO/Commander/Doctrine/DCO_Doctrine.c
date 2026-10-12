enum DCO_EForceBand
{
	STRONG,
	FAVORABLE,
	EQUAL,
	WEAKER,
	UNKNOWN
}

enum DCO_EBattleCol
{
	RECON = 1,
	STAGED = 2,
	BARRAGE = 4,
	DIRECT = 8,
	FLANK = 16,
	HITRUN = 32,
	ABF = 64,
	WAIT = 128,
	WITHDRAW = 256,
	FALLBACK = 512,
	RESERVE = 1024,
	RESERVE_ADVISED = 2048
}

enum DCO_EDefenseCol
{
	OBJ_DEFENCE = 1,
	COUNTERATTACK = 2,
	ARTY_LOST = 4,
	DELAY = 8,
	FALLBACK = 16,
	WITHDRAW = 32,
	OPLP = 64
}

enum DCO_ESupportJob
{
	SUPPLIES,
	REINFORCEMENT,
	SUPPORT,
	FIRESUPPORT,
	MEDEVAC
}

enum DCO_ESupportCond
{
	REDUCED = 1,
	FATIGUED = 2,
	LOW_AMMO = 4,
	LOW_MORALE = 8,
	IN_CONTACT = 16,
	THREATENED = 32,
	OVERRAN = 64
}

enum DCO_ECourage
{
	HESITANT,
	CAREFUL,
	CALCULATED,
	BOLD,
	RECKLESS
}

enum DCO_EDecision
{
	FAST,
	MODERATE,
	SLOW
}

enum DCO_EPreBattleIntel
{
	ACCURATE,
	NORMAL,
	WEAK,
	NONE
}

[BaseContainerProps()]
class DCO_DoctrinePreset
{
	[Attribute(desc: "Preset name shown in the GM doctrine dropdown.")]
	string m_sName;

	[Attribute(desc: "Battle plan masks for Strong Favorable Equal Weaker Unknown, space separated (DCO_EBattleCol bits).")]
	string m_sBattle;

	[Attribute(desc: "Defensive plan masks for Strong Favorable Equal Weaker Unknown, space separated (DCO_EDefenseCol bits).")]
	string m_sDefense;

	[Attribute(desc: "Support and supply condition masks for Supplies Reinforcement Support Firesupport Medevac, space separated (DCO_ESupportCond bits).")]
	string m_sSupport;

	[Attribute("300", desc: "Recon timeout in seconds on Unknown before Movement to Contact.")]
	float m_fReconTimeout;

	[Attribute("1", desc: "Scale for the Weaker defence thresholds 0.33 and 0.17. Below 1 holds longer.")]
	float m_fWeakerScale;

	[Attribute("240", desc: "Wait timeout in seconds at Moderate decision (Fast x0.5, Slow x1.5).")]
	float m_fWaitTimeout;

	[Attribute("0.25", desc: "Reserve share of the force kept back when Keep reserve applies (0-0.5).")]
	float m_fReserve;

	[Attribute("0", desc: "Minimum attack ratio for this doctrine (0 = the commander's own attack ratio). Aggression moves it by up to 15 %.")]
	float m_fAttackRatio;

	[Attribute("300", desc: "Counterattack window on a lost objective, shortest (seconds).")]
	float m_fCounterMin;

	[Attribute("600", desc: "Counterattack window on a lost objective, longest (seconds).")]
	float m_fCounterMax;

	[Attribute("0", desc: "Abort the attack when this share of the committed force is lost (0 = off).")]
	float m_fRetreatLoss;

	[Attribute("480", desc: "Consolidation hold on a captured objective in seconds before the groups are released (security, LACE, redistribution, ready for counterattack).")]
	float m_fConsolidate;

	[Attribute("", desc: "Force sizes: rows 'key min share max' separated by ';'. '-' = unused, share -1 = from the force ratio, share 0 = use min (max = cap at once). Empty = Default (Measured) rows.")]
	string m_sForce;
}

[BaseContainerProps(configRoot: true)]
class DCO_DoctrinePresetsConfig
{
	[Attribute()]
	ref array<ref DCO_DoctrinePreset> m_aPresets;
}

class DCO_Doctrine
{
	string m_sPreset;
	bool m_bModified;
	string m_sSource;
	ref array<int> m_aBattle;
	ref array<int> m_aDefense;
	ref array<int> m_aSupport;
	float m_fReconTimeout;
	float m_fWeakerScale;
	float m_fWaitTimeout;
	float m_fReserve;
	float m_fAttackRatio;
	float m_fCounterMin;
	float m_fCounterMax;
	float m_fRetreatLoss;
	float m_fConsolidate;
	string m_sForce;

	void DCO_Doctrine()
	{
		m_sForce = DCO_Force.Canon(DCO_Force.DefaultText());
		m_aBattle = {0, 0, 0, 0, 0};
		m_aDefense = {0, 0, 0, 0, 0};
		m_aSupport = {0, 0, 0, 0, 0};
		m_fReconTimeout = 300;
		m_fWeakerScale = 1;
		m_fWaitTimeout = 240;
		m_fReserve = 0.25;
		m_fAttackRatio = 0;
		m_fCounterMin = 300;
		m_fCounterMax = 600;
		m_fRetreatLoss = 0;
		m_fConsolidate = 480;
	}

	float Number(int which)
	{
		switch (which)
		{
			case 0: return m_fReconTimeout;
			case 1: return m_fWeakerScale;
			case 2: return m_fWaitTimeout;
			case 3: return m_fReserve;
			case 4: return m_fAttackRatio;
			case 5: return m_fCounterMin;
			case 6: return m_fCounterMax;
			case 7: return m_fRetreatLoss;
			case 8: return m_fConsolidate;
		}
		return 0;
	}

	void SetNumber(int which, float value)
	{
		switch (which)
		{
			case 0: m_fReconTimeout = Math.Clamp(value, 60, 1800); break;
			case 1: m_fWeakerScale = Math.Clamp(value, 0.2, 2); break;
			case 2: m_fWaitTimeout = Math.Clamp(value, 30, 1800); break;
			case 3: m_fReserve = Math.Clamp(value, 0, 0.5); break;
			case 4: m_fAttackRatio = Math.Clamp(value, 0, 6); break;
			case 5: m_fCounterMin = Math.Clamp(value, 0, 1800); break;
			case 6: m_fCounterMax = Math.Clamp(value, 0, 1800); break;
			case 7: m_fRetreatLoss = Math.Clamp(value, 0, 0.9); break;
			case 8: m_fConsolidate = Math.Clamp(value, 60, 1200); break;
		}
		if (m_fCounterMax < m_fCounterMin)
			m_fCounterMax = m_fCounterMin;
	}

	string NumText()
	{
		string text;
		for (int i = 0; i < 9; i++)
		{
			if (i > 0)
				text += " ";
			text += Number(i).ToString();
		}
		return text;
	}

	void ParseNumbers(string text)
	{
		array<string> parts = {};
		text.Split(" ", parts, true);
		for (int i = 0; i < 9 && i < parts.Count(); i++)
		{
			SetNumber(i, parts[i].ToFloat());
		}
	}

	int Mask(int index)
	{
		if (index >= 0 && index < 5)
			return m_aBattle[index];
		if (index >= 5 && index < 10)
			return m_aDefense[index - 5];
		if (index >= 10 && index < 15)
			return m_aSupport[index - 10];
		return 0;
	}

	void SetMask(int index, int mask)
	{
		if (index >= 0 && index < 5)
			m_aBattle[index] = mask;
		else if (index >= 5 && index < 10)
			m_aDefense[index - 5] = mask;
		else if (index >= 10 && index < 15)
			m_aSupport[index - 10] = mask;
	}

	DCO_Doctrine Copy()
	{
		DCO_Doctrine d = new DCO_Doctrine();
		d.m_sPreset = m_sPreset;
		d.m_bModified = m_bModified;
		d.m_sSource = m_sSource;
		for (int i = 0; i < 15; i++)
		{
			d.SetMask(i, Mask(i));
		}
		for (int n = 0; n < 9; n++)
		{
			d.SetNumber(n, Number(n));
		}
		d.m_sForce = m_sForce;
		return d;
	}

	string MaskText()
	{
		return JoinMasks(m_aBattle) + "|" + JoinMasks(m_aDefense) + "|" + JoinMasks(m_aSupport) + "|v3";
	}

	static void StagedLegacy(notnull array<int> battle, string source)
	{
		string before = JoinMasks(battle);
		for (int i = 0; i < battle.Count(); i++)
			battle[i] = battle[i] | DCO_EBattleCol.STAGED;
		DCO_Perf.Count("doc:staged_legacy");
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("doctrine_staged_legacy source=%1 before=%2 after=%3 (old doctrine data: Staged prep ticked to keep the old synchronized attacks)", source, before, JoinMasks(battle)));
	}

	static void ParseMasks(string text, notnull array<int> outMasks)
	{
		array<string> parts = {};
		text.Split(" ", parts, true);
		for (int i = 0; i < outMasks.Count(); i++)
		{
			if (parts.IsIndexValid(i))
				outMasks[i] = parts[i].ToInt();
			else
				outMasks[i] = 0;
		}
	}

	static string JoinMasks(notnull array<int> masks)
	{
		string text;
		foreach (int i, int mask : masks)
		{
			if (i > 0)
				text += " ";
			text += mask.ToString();
		}
		return text;
	}

	void FromPreset(DCO_DoctrinePreset preset, string source)
	{
		if (!preset)
			return;
		m_sPreset = preset.m_sName;
		m_sSource = source;
		m_bModified = false;
		ParseMasks(preset.m_sBattle, m_aBattle);
		ParseMasks(preset.m_sDefense, m_aDefense);
		ParseMasks(preset.m_sSupport, m_aSupport);
		SetNumber(0, preset.m_fReconTimeout);
		SetNumber(1, preset.m_fWeakerScale);
		SetNumber(2, preset.m_fWaitTimeout);
		SetNumber(3, preset.m_fReserve);
		SetNumber(4, preset.m_fAttackRatio);
		SetNumber(5, preset.m_fCounterMin);
		SetNumber(6, preset.m_fCounterMax);
		SetNumber(7, preset.m_fRetreatLoss);
		SetNumber(8, preset.m_fConsolidate);
		m_sForce = DCO_Force.Canon(preset.m_sForce);
	}

	float ForceValue(string key, int field)
	{
		int mi;
		float sh;
		int ma;
		DCO_Force.RowOrDefault(m_sForce, key, mi, sh, ma);
		if (field == 0)
			return mi;
		if (field == 1)
			return sh;
		return ma;
	}

	void SetForce(string key, int field, float value)
	{
		m_sForce = DCO_Force.SetField(m_sForce, key, field, value);
	}

	DCO_DoctrinePreset ToPreset(string name)
	{
		DCO_DoctrinePreset preset = new DCO_DoctrinePreset();
		preset.m_sName = name;
		preset.m_sBattle = JoinMasks(m_aBattle);
		preset.m_sDefense = JoinMasks(m_aDefense);
		preset.m_sSupport = JoinMasks(m_aSupport);
		preset.m_fReconTimeout = m_fReconTimeout;
		preset.m_fWeakerScale = m_fWeakerScale;
		preset.m_fWaitTimeout = m_fWaitTimeout;
		preset.m_fReserve = m_fReserve;
		preset.m_fAttackRatio = m_fAttackRatio;
		preset.m_fCounterMin = m_fCounterMin;
		preset.m_fCounterMax = m_fCounterMax;
		preset.m_fRetreatLoss = m_fRetreatLoss;
		preset.m_fConsolidate = m_fConsolidate;
		preset.m_sForce = m_sForce;
		return preset;
	}

	bool Battle(int band, int col)
	{
		return m_aBattle.IsIndexValid(band) && (m_aBattle[band] & col) != 0;
	}

	bool Defense(int band, int col)
	{
		return m_aDefense.IsIndexValid(band) && (m_aDefense[band] & col) != 0;
	}

	bool SupportFires(int job, int conditions)
	{
		return m_aSupport.IsIndexValid(job) && (m_aSupport[job] & conditions) != 0;
	}

	string Encode()
	{
		return "D2|" + m_sPreset + "|" + JoinMasks(m_aBattle) + "|" + JoinMasks(m_aDefense) + "|" + JoinMasks(m_aSupport) + "|" + NumText();
	}

	bool Decode(string text)
	{
		array<string> parts = {};
		text.Split("|", parts, false);
		if (parts.Count() < 6 || parts[0] != "D2")
			return false;
		m_sPreset = parts[1];
		ParseMasks(parts[2], m_aBattle);
		ParseMasks(parts[3], m_aDefense);
		ParseMasks(parts[4], m_aSupport);
		ParseNumbers(parts[5]);
		return true;
	}

	bool SameTables(DCO_Doctrine other)
	{
		if (!other)
			return false;
		for (int i = 0; i < 5; i++)
		{
			if (m_aBattle[i] != other.m_aBattle[i] || m_aDefense[i] != other.m_aDefense[i] || m_aSupport[i] != other.m_aSupport[i])
				return false;
		}
		for (int n = 0; n < 9; n++)
		{
			if (Math.AbsFloat(Number(n) - other.Number(n)) > 0.005)
				return false;
		}
		if (DCO_Force.Canon(m_sForce) != DCO_Force.Canon(other.m_sForce))
			return false;
		return true;
	}

	static string BandName(int band)
	{
		switch (band)
		{
			case DCO_EForceBand.STRONG:
				return "Strong";
			case DCO_EForceBand.FAVORABLE:
				return "Favorable";
			case DCO_EForceBand.EQUAL:
				return "Equal";
			case DCO_EForceBand.WEAKER:
				return "Weaker";
		}
		return "Unknown";
	}
}

class DCO_DoctrineStore
{
	protected static ref DCO_DoctrinePresetsConfig s_BuiltIn;
	protected static bool s_bLoaded;

	static string Dir()
	{
		return "$profile:DCO/Doctrine/";
	}

	static string DefaultName()
	{
		return "Default (Measured)";
	}

	static string FactionDefault(string factionKey)
	{
		string name;
		switch (factionKey)
		{
			case "US": name = "US 1989"; break;
			case "USSR": name = "USSR 1989"; break;
			case "FIA": name = "FIA"; break;
		}
		if (name.IsEmpty() || !BuiltIn(name))
			return string.Empty;
		return name;
	}

	protected static void LoadBuiltIn()
	{
		if (s_bLoaded)
			return;
		s_bLoaded = true;
		Resource res = BaseContainerTools.LoadContainer("{6C1E2B7A9D3F4E58}Configs/DCO/DCO_DoctrinePresets.conf");
		if (res && res.IsValid())
			s_BuiltIn = DCO_DoctrinePresetsConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(res.GetResource().ToBaseContainer()));
		int count;
		if (s_BuiltIn && s_BuiltIn.m_aPresets)
			count = s_BuiltIn.m_aPresets.Count();
		DCO_ELogLevel level = DCO_ELogLevel.INFO;
		if (count == 0)
			level = DCO_ELogLevel.ERROR;
		DCO_Log.Write(DCO_ELogChannel.CMD, level, "-", "doctrine_builtin_loaded count=" + count.ToString());
	}

	static DCO_DoctrinePreset BuiltIn(string name)
	{
		LoadBuiltIn();
		if (!s_BuiltIn || !s_BuiltIn.m_aPresets)
			return null;
		foreach (DCO_DoctrinePreset preset : s_BuiltIn.m_aPresets)
		{
			if (preset && preset.m_sName == name)
				return preset;
		}
		return null;
	}

	static void List(notnull array<string> outNames, notnull array<bool> outBuiltIn)
	{
		LoadBuiltIn();
		if (s_BuiltIn && s_BuiltIn.m_aPresets)
		{
			foreach (DCO_DoctrinePreset preset : s_BuiltIn.m_aPresets)
			{
				if (!preset)
					continue;
				outNames.Insert(preset.m_sName);
				outBuiltIn.Insert(true);
			}
		}
		array<string> files = SCR_FileIOHelper.FindFiles(Dir(), ".json");
		if (!files)
			return;
		array<string> custom = {};
		foreach (string f : files)
		{
			string n = FilePath.StripExtension(FilePath.StripPath(f));
			if (!n.IsEmpty() && outNames.Find(n) < 0 && custom.Find(n) < 0)
				custom.Insert(n);
		}
		custom.Sort();
		foreach (string c : custom)
		{
			outNames.Insert(c);
			outBuiltIn.Insert(false);
		}
	}

	static DCO_DoctrinePreset LoadJson(string name)
	{
		string path = Dir() + name + ".json";
		if (name.IsEmpty() || !FileIO.FileExists(path))
			return null;
		SCR_JsonLoadContext load = new SCR_JsonLoadContext();
		if (!load.LoadFromFile(path))
			return null;
		DCO_DoctrinePreset preset = new DCO_DoctrinePreset();
		preset.m_sName = name;
		load.ReadValue("battle", preset.m_sBattle);
		int fileVer;
		load.ReadValue("version", fileVer);
		if (fileVer < 3 && !preset.m_sBattle.IsEmpty())
		{
			array<int> legacy = {0, 0, 0, 0, 0};
			DCO_Doctrine.ParseMasks(preset.m_sBattle, legacy);
			DCO_Doctrine.StagedLegacy(legacy, "preset_file:" + name);
			preset.m_sBattle = DCO_Doctrine.JoinMasks(legacy);
		}
		load.ReadValue("defense", preset.m_sDefense);
		load.ReadValue("support", preset.m_sSupport);
		load.ReadValue("recon_timeout", preset.m_fReconTimeout);
		load.ReadValue("weaker_scale", preset.m_fWeakerScale);
		preset.m_fWaitTimeout = 240;
		preset.m_fReserve = 0.25;
		preset.m_fCounterMin = 300;
		preset.m_fCounterMax = 600;
		preset.m_fConsolidate = 480;
		load.ReadValue("wait_timeout", preset.m_fWaitTimeout);
		load.ReadValue("reserve", preset.m_fReserve);
		load.ReadValue("attack_ratio", preset.m_fAttackRatio);
		load.ReadValue("counter_min", preset.m_fCounterMin);
		load.ReadValue("counter_max", preset.m_fCounterMax);
		load.ReadValue("retreat_loss", preset.m_fRetreatLoss);
		load.ReadValue("force", preset.m_sForce);
		load.ReadValue("consolidate", preset.m_fConsolidate);
		if (preset.m_sBattle.IsEmpty())
			return null;
		if (preset.m_fReconTimeout <= 0)
			preset.m_fReconTimeout = 300;
		if (preset.m_fWeakerScale <= 0)
			preset.m_fWeakerScale = 1;
		return preset;
	}

	static bool SaveJson(string name, DCO_DoctrinePreset preset)
	{
		if (name.IsEmpty() || !preset || BuiltIn(name))
			return false;
		FileIO.MakeDirectory("$profile:DCO");
		FileIO.MakeDirectory(Dir());
		SCR_JsonSaveContext save = new SCR_JsonSaveContext();
		save.WriteValue("version", 3);
		save.WriteValue("battle", preset.m_sBattle);
		save.WriteValue("defense", preset.m_sDefense);
		save.WriteValue("support", preset.m_sSupport);
		save.WriteValue("recon_timeout", preset.m_fReconTimeout);
		save.WriteValue("weaker_scale", preset.m_fWeakerScale);
		save.WriteValue("wait_timeout", preset.m_fWaitTimeout);
		save.WriteValue("reserve", preset.m_fReserve);
		save.WriteValue("attack_ratio", preset.m_fAttackRatio);
		save.WriteValue("counter_min", preset.m_fCounterMin);
		save.WriteValue("counter_max", preset.m_fCounterMax);
		save.WriteValue("retreat_loss", preset.m_fRetreatLoss);
		save.WriteValue("force", preset.m_sForce);
		save.WriteValue("consolidate", preset.m_fConsolidate);
		bool ok = save.SaveToFile(Dir() + name + ".json");
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", "doctrine_save preset=" + DCO_SettingsBaseline.LogText(name) + " result=" + ok.ToString());
		return ok;
	}

	static DCO_DoctrinePreset Find(string name)
	{
		DCO_DoctrinePreset preset = BuiltIn(name);
		if (!preset)
			preset = LoadJson(name);
		return preset;
	}

	static DCO_Doctrine Resolve(string name, out bool found)
	{
		DCO_Doctrine doctrine = new DCO_Doctrine();
		DCO_DoctrinePreset preset = BuiltIn(name);
		string source = "builtin";
		if (!preset)
		{
			preset = LoadJson(name);
			source = "json";
		}
		found = preset != null;
		if (!preset)
		{
			preset = BuiltIn(DefaultName());
			source = "builtin";
		}
		doctrine.FromPreset(preset, source);
		if (!preset)
			doctrine.m_sPreset = DefaultName();
		return doctrine;
	}
}
