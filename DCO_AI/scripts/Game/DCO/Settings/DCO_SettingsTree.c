[BaseContainerProps(configRoot: true)]
class DCO_SettingsTreeConfig
{
	[Attribute()]
	ref array<ref DCO_SettingsFolderDef> m_aFolders;

	[Attribute()]
	ref array<ref DCO_SettingsDependencyDef> m_aDependencies;

	[Attribute()]
	ref array<ref DCO_SettingsHeavyDef> m_aHeavy;

	[Attribute()]
	ref array<ref DCO_SettingsRuleDef> m_aRules;

	[Attribute()]
	ref array<ref DCO_SettingsSectionDef> m_aSections;

	[Attribute(desc: "Slider attributes with a 0-1 value shown as a percentage, space separated")]
	string m_sPercent;

	[Attribute(desc: "Expert attributes, hidden in the Advanced view and shown in the Expert view, space separated")]
	string m_sExpert;
}

[BaseContainerProps(), BaseContainerCustomTitleField("m_sTitle")]
class DCO_SettingsSectionDef
{
	[Attribute(desc: "First attribute class of the section")]
	string m_sAttribute;

	[Attribute()]
	string m_sTitle;
}

[BaseContainerProps(), BaseContainerCustomTitleField("m_sName")]
class DCO_SettingsFolderDef
{
	[Attribute("0", desc: "Target: 0 Global, 1 Commander, 2 Group, 3 Character, 4 Waypoint")]
	int m_iTarget;

	[Attribute()]
	string m_sName;

	[Attribute()]
	string m_sDescription;

	[Attribute("0", desc: "World-wide folder, never overridden per faction")]
	bool m_bWorldWide;

	[Attribute()]
	ref array<ref DCO_SettingsSubDef> m_aSubs;
}

[BaseContainerProps()]
class DCO_SettingsSubDef
{
	[Attribute(params: "conf")]
	ResourceName m_Category;

	[Attribute()]
	string m_sDescription;
}

[BaseContainerProps(), BaseContainerCustomTitleField("m_sParent")]
class DCO_SettingsDependencyDef
{
	[Attribute(desc: "Parent attribute class name")]
	string m_sParent;

	[Attribute(desc: "Child attribute class names, space separated")]
	string m_sChildren;

	[Attribute("0", desc: "Parent values that grey out the children, space separated")]
	string m_sOffValues;
}

[BaseContainerProps(), BaseContainerCustomTitleField("m_sAttribute")]
class DCO_SettingsHeavyDef
{
	[Attribute(desc: "Attribute class name")]
	string m_sAttribute;

	[Attribute(desc: "Costly values, space separated. Empty = any value other than 0")]
	string m_sValues;

	[Attribute()]
	string m_sReason;
}

[BaseContainerProps(), BaseContainerCustomTitleField("m_sMessage")]
class DCO_SettingsRuleDef
{
	[Attribute("0", desc: "0: A > B x factor. 1: A == value A. 2: A >= value A and B == value B. 3: A > value A")]
	int m_iKind;

	[Attribute(desc: "Attribute class name A")]
	string m_sA;

	[Attribute(desc: "Attribute class name B")]
	string m_sB;

	[Attribute("0")]
	float m_fA;

	[Attribute("0")]
	float m_fB;

	[Attribute("1")]
	float m_fFactor;

	[Attribute()]
	string m_sMessage;
}

class DCO_SettingsTree
{
	protected static ref DCO_SettingsTreeConfig s_Config;
	protected static ref map<string, int> s_mCategory;
	protected static ref set<string> s_sExpert;

	static DCO_SettingsTreeConfig Get()
	{
		if (s_Config)
			return s_Config;

		Resource res = BaseContainerTools.LoadContainer("{6EA18F64CA4BA79B}Configs/DCO/DCO_SettingsTree.conf");
		if (res && res.IsValid())
			s_Config = DCO_SettingsTreeConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(res.GetResource().ToBaseContainer()));
		if (!s_Config)
		{
			DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.ERROR, "-", "settings_tree_missing");
			s_Config = new DCO_SettingsTreeConfig();
		}
		if (!s_Config.m_aFolders)
			s_Config.m_aFolders = {};
		if (!s_Config.m_aDependencies)
			s_Config.m_aDependencies = {};
		if (!s_Config.m_aHeavy)
			s_Config.m_aHeavy = {};
		if (!s_Config.m_aRules)
			s_Config.m_aRules = {};
		if (!s_Config.m_aSections)
			s_Config.m_aSections = {};

		s_mCategory = new map<string, int>();
		foreach (int f, DCO_SettingsFolderDef folder : s_Config.m_aFolders)
		{
			if (!folder.m_aSubs)
				folder.m_aSubs = {};
			foreach (int s, DCO_SettingsSubDef sub : folder.m_aSubs)
			{
				s_mCategory.Set(Key(sub.m_Category), f * 1000 + s);
			}
		}
		return s_Config;
	}

	static string Key(ResourceName res)
	{
		string s = res;
		int close = s.IndexOf("}");
		if (s.StartsWith("{") && close > 1)
			return s.Substring(1, close - 1);
		return s;
	}

	static int FindSub(ResourceName category)
	{
		Get();
		int v;
		if (s_mCategory.Find(Key(category), v))
			return v;
		return -1;
	}

	static bool IsDCO(SCR_BaseEditorAttribute attribute)
	{
		return attribute && FindSub(attribute.GetCategoryConfig()) >= 0;
	}

	static bool IsWorldWide(SCR_BaseEditorAttribute attribute)
	{
		if (!attribute)
			return false;
		int v = FindSub(attribute.GetCategoryConfig());
		if (v < 0)
			return false;
		return s_Config.m_aFolders[v / 1000].m_bWorldWide;
	}

	static int TargetOf(SCR_BaseEditorAttribute attribute)
	{
		if (!attribute)
			return -1;
		int v = FindSub(attribute.GetCategoryConfig());
		if (v < 0)
			return -1;
		return s_Config.m_aFolders[v / 1000].m_iTarget;
	}

	static bool HasValue(string list, float value)
	{
		array<string> parts = {};
		list.Split(" ", parts, true);
		foreach (string p : parts)
		{
			if (Math.AbsFloat(p.ToFloat() - value) < 0.0001)
				return true;
		}
		return false;
	}

	static string HeavyReason(string attributeClass, float value)
	{
		foreach (DCO_SettingsHeavyDef h : Get().m_aHeavy)
		{
			if (h.m_sAttribute != attributeClass)
				continue;
			if (h.m_sValues.IsEmpty())
			{
				if (value != 0)
					return h.m_sReason;
			}
			else if (HasValue(h.m_sValues, value))
			{
				return h.m_sReason;
			}
		}
		return string.Empty;
	}

	static string SectionTitle(string attributeClass)
	{
		foreach (DCO_SettingsSectionDef sec : Get().m_aSections)
		{
			if (sec.m_sAttribute == attributeClass)
				return sec.m_sTitle;
		}
		return string.Empty;
	}

	static bool IsPercent(string attributeClass)
	{
		string list = " " + Get().m_sPercent + " ";
		return list.Contains(" " + attributeClass + " ");
	}

	static bool IsHeavyAttribute(string attributeClass)
	{
		foreach (DCO_SettingsHeavyDef h : Get().m_aHeavy)
		{
			if (h.m_sAttribute == attributeClass)
				return true;
		}
		return false;
	}

	static bool IsExpert(string attributeClass)
	{
		if (!s_sExpert)
		{
			s_sExpert = new set<string>();
			array<string> parts = {};
			Get().m_sExpert.Split(" ", parts, true);
			foreach (string p : parts)
			{
				s_sExpert.Insert(p);
			}
			DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", "settings_expert_list n=" + s_sExpert.Count());
		}
		return s_sExpert.Contains(attributeClass);
	}
}
