enum DCO_EMeter
{
	DIFFICULTY,
	PERFORMANCE_COST,
	REALISM
}

enum DCO_EMeterMode
{
	LINEAR,
	PER_OPTION
}

enum DCO_ESettingTarget
{
	GLOBAL,
	COMMANDER,
	GROUP,
	CHARACTER
}

[BaseContainerProps(configRoot: true)]
class DCO_SettingsHelpConfig
{
	[Attribute(desc: "Help entry per GM setting row, matched by attribute class name")]
	ref array<ref DCO_SettingHelp> m_aHelp;

	[Attribute(desc: "Help page per settings category (tab/folder)")]
	ref array<ref DCO_SettingSubHelp> m_aSubs;
}

[BaseContainerProps(), BaseContainerCustomTitleField("m_sAttribute")]
class DCO_SettingHelp
{
	[Attribute("", UIWidgets.EditBox, "Attribute class name of the GM setting this help belongs to, e.g. SCR_AIGlobalLODModeAttribute")]
	string m_sAttribute;

	[Attribute("", UIWidgets.EditBoxMultiline, "What the setting does, one or two plain sentences")]
	string m_sWhat;

	[Attribute("", UIWidgets.EditBoxMultiline, "What happens at low values (reference text for writers and tools)")]
	string m_sLow;

	[Attribute("", UIWidgets.EditBoxMultiline, "What happens at high values (reference text for writers and tools)")]
	string m_sHigh;

	[Attribute("", UIWidgets.EditBox, "Recommended value or range, shown after the 'Recommended' label")]
	string m_sRecommended;

	[Attribute("0", UIWidgets.EditBox, "Performance note level for writers and tools: 0 none, higher = more CPU cost")]
	int m_iPerf;

	[Attribute("", UIWidgets.EditBox, "Related attribute class names, space separated (reference for writers and tools)")]
	string m_sRelated;

	[Attribute("", UIWidgets.EditBoxMultiline, "Effect line for sliders; %1 is replaced by the current value text")]
	string m_sEffect;

	[Attribute(desc: "Effect line per option for combo/toggle settings, in option order (index 0 = first option)")]
	ref array<string> m_aEffectPerOption;

	[Attribute("", UIWidgets.EditBox, "Glossary term ids used by this setting, space separated (see DCO_SettingsGlossary.conf)")]
	string m_sTerms;

	[Attribute("0", UIWidgets.EditBox, "1 = mark this setting as Beta (experimental) in DCO Settings, 0 = normal")]
	int m_iBeta;
}

[BaseContainerProps(), BaseContainerCustomTitleField("m_sCategory")]
class DCO_SettingSubHelp
{
	[Attribute("", UIWidgets.EditBox, "Attribute category (.conf resource) this page belongs to")]
	string m_sCategory;

	[Attribute("", UIWidgets.EditBoxMultiline, "What this category is for")]
	string m_sPurpose;

	[Attribute("", UIWidgets.EditBoxMultiline, "How the settings in this category interact with each other")]
	string m_sInteract;

	[Attribute(desc: "Example setups, each a name plus the values to use")]
	ref array<ref DCO_SettingSetup> m_aSetups;

	[Attribute("", UIWidgets.EditBoxMultiline, "Common mistakes when using this category")]
	string m_sPitfalls;
}

[BaseContainerProps(), BaseContainerCustomTitleField("m_sName")]
class DCO_SettingSetup
{
	[Attribute("", UIWidgets.EditBox, "Setup name, e.g. 'Hardcore milsim'")]
	string m_sName;

	[Attribute("", UIWidgets.EditBoxMultiline, "Values for this setup in plain text")]
	string m_sValues;
}

[BaseContainerProps(configRoot: true)]
class DCO_SettingsGlossaryConfig
{
	[Attribute(desc: "Glossary terms shown in DCO Settings help")]
	ref array<ref DCO_SettingTerm> m_aTerms;
}

[BaseContainerProps(), BaseContainerCustomTitleField("m_sTerm")]
class DCO_SettingTerm
{
	[Attribute("", UIWidgets.EditBox, "Unique id, referenced from DCO_SettingHelp.m_sTerms")]
	string m_sId;

	[Attribute("", UIWidgets.EditBox, "Display name of the term")]
	string m_sTerm;

	[Attribute("", UIWidgets.EditBoxMultiline, "Plain-language definition")]
	string m_sDefinition;

	[Attribute("", UIWidgets.EditBox, "Other words for the same term, space separated")]
	string m_sAliases;
}

[BaseContainerProps(configRoot: true)]
class DCO_SettingsSimpleConfig
{
	[Attribute(desc: "Simple-mode settings; each one sets several advanced attributes at once")]
	ref array<ref DCO_SimpleSetting> m_aSimple;
}

[BaseContainerProps(), BaseContainerCustomTitleField("m_sName")]
class DCO_SimpleSetting
{
	[Attribute("0", UIWidgets.ComboBox, "Which entity the setting applies to", "", ParamEnumArray.FromEnum(DCO_ESettingTarget))]
	int m_iTarget;

	[Attribute("", UIWidgets.EditBox, "Display name in simple mode")]
	string m_sName;

	[Attribute("", UIWidgets.EditBoxMultiline, "Description shown in simple mode")]
	string m_sDescription;

	[Attribute("", UIWidgets.EditBox, "Optional attribute class whose current value this setting mirrors")]
	string m_sMirror;

	[Attribute(desc: "Levels the player can pick, in order; each level lists attribute values")]
	ref array<ref DCO_SimpleLevel> m_aLevels;
}

[BaseContainerProps(), BaseContainerCustomTitleField("m_sName")]
class DCO_SimpleLevel
{
	[Attribute("", UIWidgets.EditBox, "Level name, e.g. Low / Medium / High")]
	string m_sName;

	[Attribute(desc: "Attribute values applied when this level is picked")]
	ref array<ref DCO_SimpleValue> m_aValues;
}

[BaseContainerProps(), BaseContainerCustomTitleField("m_sAttribute")]
class DCO_SimpleValue
{
	[Attribute("", UIWidgets.EditBox, "Attribute class name to set")]
	string m_sAttribute;

	[Attribute("0", UIWidgets.EditBox, "Value to set (combo index, 0/1 for toggles, or slider value)")]
	float m_fValue;
}

[BaseContainerProps(configRoot: true)]
class DCO_SettingsMetersConfig
{
	[Attribute(desc: "How much each setting adds to the Difficulty / Performance cost / Realism meters in DCO Settings")]
	ref array<ref DCO_MeterWeight> m_aWeights;
}

[BaseContainerProps(), BaseContainerCustomTitleField("m_sAttribute")]
class DCO_MeterWeight
{
	[Attribute("", UIWidgets.EditBox, "Attribute class name of the GM setting, e.g. SCR_AIGlobalLODModeAttribute")]
	string m_sAttribute;

	[Attribute("0", UIWidgets.ComboBox, "Which meter this setting adds points to", "", ParamEnumArray.FromEnum(DCO_EMeter))]
	int m_iMeter;

	[Attribute("0", UIWidgets.ComboBox, "LINEAR: points scale between At Min and At Max across the slider range. PER_OPTION: points come from Per Option; At Min/At Max are ignored", "", ParamEnumArray.FromEnum(DCO_EMeterMode))]
	int m_iMode;

	[Attribute("0", UIWidgets.EditBox, "LINEAR only: points at the lowest value")]
	float m_fAtMin;

	[Attribute("0", UIWidgets.EditBox, "LINEAR only: points at the highest value")]
	float m_fAtMax;

	[Attribute("", UIWidgets.EditBox, "PER_OPTION only: 'value:points' pairs separated by spaces, e.g. '0:0 1:30 2:60' = option value 0 gives 0 points, 1 gives 30, 2 gives 60. Values not listed give 0")]
	string m_sPerOption;
}

class DCO_SettingsHelp
{
	protected static ref DCO_SettingsHelpConfig s_Config;
	protected static ref map<string, DCO_SettingHelp> s_mHelp;
	protected static ref map<string, DCO_SettingSubHelp> s_mSub;
	protected static ref DCO_SettingsGlossaryConfig s_Glossary;
	protected static ref DCO_SettingsSimpleConfig s_Simple;
	protected static ref DCO_SettingsMetersConfig s_Meters;
	protected static bool s_bExtraLoaded;

	protected static Managed LoadConfig(ResourceName path)
	{
		Resource res = BaseContainerTools.LoadContainer(path);
		if (!res || !res.IsValid())
			return null;
		return BaseContainerTools.CreateInstanceFromContainer(res.GetResource().ToBaseContainer());
	}

	protected static void LoadExtra()
	{
		if (s_bExtraLoaded)
			return;
		s_bExtraLoaded = true;
		s_Glossary = DCO_SettingsGlossaryConfig.Cast(LoadConfig("{11FA225FF78DD566}Configs/DCO/DCO_SettingsGlossary.conf"));
		s_Simple = DCO_SettingsSimpleConfig.Cast(LoadConfig("{BB4E81ED370246E8}Configs/DCO/DCO_SettingsSimple.conf"));
		s_Meters = DCO_SettingsMetersConfig.Cast(LoadConfig("{F9E9589E85B85FB2}Configs/DCO/DCO_SettingsMeters.conf"));
		int terms, simple, weights;
		if (s_Glossary && s_Glossary.m_aTerms)
			terms = s_Glossary.m_aTerms.Count();
		if (s_Simple && s_Simple.m_aSimple)
			simple = s_Simple.m_aSimple.Count();
		if (s_Meters && s_Meters.m_aWeights)
			weights = s_Meters.m_aWeights.Count();
		DCO_ELogLevel level = DCO_ELogLevel.INFO;
		if (terms == 0 || simple == 0 || weights == 0)
			level = DCO_ELogLevel.ERROR;
		DCO_Log.Write(DCO_ELogChannel.GM, level, "-", "settings_help_extra terms=" + terms.ToString() + " simple=" + simple.ToString() + " weights=" + weights.ToString());
	}

	static array<ref DCO_SettingTerm> Terms()
	{
		LoadExtra();
		if (!s_Glossary)
			return null;
		return s_Glossary.m_aTerms;
	}

	static DCO_SettingTerm Term(string id)
	{
		array<ref DCO_SettingTerm> terms = Terms();
		if (!terms)
			return null;
		foreach (DCO_SettingTerm term : terms)
		{
			if (term && term.m_sId == id)
				return term;
		}
		return null;
	}

	static array<ref DCO_SimpleSetting> Simple()
	{
		LoadExtra();
		if (!s_Simple)
			return null;
		return s_Simple.m_aSimple;
	}

	static array<ref DCO_MeterWeight> Weights()
	{
		LoadExtra();
		if (!s_Meters)
			return null;
		return s_Meters.m_aWeights;
	}

	protected static void Load()
	{
		if (s_mHelp)
			return;
		s_mHelp = new map<string, DCO_SettingHelp>();
		s_mSub = new map<string, DCO_SettingSubHelp>();
		Resource res = BaseContainerTools.LoadContainer("{0CE47544D2BFA6C7}Configs/DCO/DCO_SettingsHelp.conf");
		if (res && res.IsValid())
			s_Config = DCO_SettingsHelpConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(res.GetResource().ToBaseContainer()));
		if (!s_Config)
		{
			DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.ERROR, "-", "settings_help_missing");
			return;
		}
		if (s_Config.m_aHelp)
		{
			foreach (DCO_SettingHelp help : s_Config.m_aHelp)
			{
				if (help && !help.m_sAttribute.IsEmpty())
					s_mHelp.Set(help.m_sAttribute, help);
			}
		}
		if (s_Config.m_aSubs)
		{
			foreach (DCO_SettingSubHelp sub : s_Config.m_aSubs)
			{
				if (sub && !sub.m_sCategory.IsEmpty())
					s_mSub.Set(sub.m_sCategory, sub);
			}
		}
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", "settings_help_loaded settings=" + s_mHelp.Count().ToString() + " subs=" + s_mSub.Count().ToString());
	}

	static DCO_SettingHelp Get(string attributeClass)
	{
		Load();
		DCO_SettingHelp help;
		s_mHelp.Find(attributeClass, help);
		return help;
	}

	static bool IsBeta(string attributeClass)
	{
		DCO_SettingHelp help = Get(attributeClass);
		return help && help.m_iBeta > 0;
	}

	static DCO_SettingSubHelp GetSub(ResourceName category)
	{
		Load();
		string key = FilePath.StripExtension(FilePath.StripPath(category));
		DCO_SettingSubHelp sub;
		s_mSub.Find(key, sub);
		return sub;
	}

	static string OptionEffect(DCO_SettingHelp help, int index)
	{
		if (!help || !help.m_aEffectPerOption || !help.m_aEffectPerOption.IsIndexValid(index))
			return string.Empty;
		return help.m_aEffectPerOption[index];
	}
}
