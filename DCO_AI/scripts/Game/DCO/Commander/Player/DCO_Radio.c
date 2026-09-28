[BaseContainerProps()]
class DCO_RadioVariant
{
	[Attribute("", UIWidgets.EditBox, "Teks. Placeholder: {callsign} {grid} {dir} {dist} {eta} {obj} {count} {type} {reason} dll. Boleh ID stringtable (#...).")]
	string m_sText;

	[Attribute("", UIWidgets.EditBox, "Faction key (US / USSR / FIA). Kosong = semua.")]
	string m_sFaction;

	[Attribute("-1", UIWidgets.EditBox, "-1 semua, 0 Measured, 1 Aggressive, 2 Cautious")]
	int m_iPersonality;

	[Attribute("-1", UIWidgets.EditBox, "-1 semua, 0 tenang, 1 kritis")]
	int m_iUrgency;
}

[BaseContainerProps()]
class DCO_RadioPhrase
{
	[Attribute("", UIWidgets.EditBox)]
	string m_sKey;

	[Attribute("", UIWidgets.Object)]
	ref array<ref DCO_RadioVariant> m_aVariants;
}

[BaseContainerProps(configRoot: true)]
class DCO_RadioBankConfig
{
	[Attribute("", UIWidgets.Object)]
	ref array<ref DCO_RadioPhrase> m_aPhrases;
}

class DCO_Radio
{
	static const ResourceName BANK = "{6921DC7753900C22}Configs/DCO/DCO_RadioBank.conf";
	static const string OVERRIDE_PATH = "$profile:DCO/DCO_Radio.json";

	static const int PERS_MEASURED = 0;
	static const int PERS_AGGRESSIVE = 1;
	static const int PERS_CAUTIOUS = 2;

	protected static ref DCO_RadioBankConfig s_Bank;
	protected static ref map<string, DCO_RadioPhrase> s_mBank;
	protected static ref map<string, ref array<ref DCO_RadioVariant>> s_mOverride;
	protected static ref map<string, int> s_mLast = new map<string, int>();

	static void Group(int groupID, string title, string key, DCO_ERadioKind kind = DCO_ERadioKind.INFO, array<string> params = null, bool critical = false)
	{
		if (!Replication.IsServer())
			return;

		SCR_GroupsManagerComponent groups = SCR_GroupsManagerComponent.GetInstance();
		if (!groups)
			return;

		SCR_AIGroup grp = groups.FindGroup(groupID);
		if (!grp)
			return;

		array<int> players = grp.GetPlayerIDs();
		if (!players || players.IsEmpty())
			return;

		bool isOverride;
		int idx = Pick(key, grp, critical, "g" + groupID, isOverride);
		foreach (int pid : players)
			Deliver(pid, title, key, idx, isOverride, params, kind);
	}

	static void Player(int playerID, string title, string key, DCO_ERadioKind kind = DCO_ERadioKind.INFO, array<string> params = null, bool critical = false)
	{
		if (!Replication.IsServer())
			return;

		SCR_AIGroup grp;
		SCR_GroupsManagerComponent groups = SCR_GroupsManagerComponent.GetInstance();
		if (groups)
			grp = groups.GetPlayerGroup(playerID);

		bool isOverride;
		int idx = Pick(key, grp, critical, "p" + playerID, isOverride);
		Deliver(playerID, title, key, idx, isOverride, params, kind);
	}

	static array<string> P(string k1, string v1, string k2 = "", string v2 = "", string k3 = "", string v3 = "", string k4 = "", string v4 = "", string k5 = "", string v5 = "")
	{
		array<string> p = {};
		p.Insert(k1);
		p.Insert(v1);
		if (!k2.IsEmpty())
		{
			p.Insert(k2);
			p.Insert(v2);
		}
		if (!k3.IsEmpty())
		{
			p.Insert(k3);
			p.Insert(v3);
		}
		if (!k4.IsEmpty())
		{
			p.Insert(k4);
			p.Insert(v4);
		}
		if (!k5.IsEmpty())
		{
			p.Insert(k5);
			p.Insert(v5);
		}
		return p;
	}

	static string Join(string list, string item)
	{
		if (list.IsEmpty())
			return item;
		return list + "+" + item;
	}

	static string N(float v)
	{
		int n = Math.Round(v);
		return n.ToString();
	}

	static string Dir(vector from, vector to)
	{
		return "@dir_" + DCO_PlayerComms.Bearing(from, to);
	}

	static int PersonalityOf(AICommander_BaseComponent cmd)
	{
		if (!cmd)
			return PERS_MEASURED;
		if (cmd.GetAggression() >= 0.65)
			return PERS_AGGRESSIVE;
		if (cmd.GetRiskTaking() <= 0.35 || cmd.GetPatience() >= 0.7)
			return PERS_CAUTIOUS;
		return PERS_MEASURED;
	}

	static string Resolve(string key, int idx, array<string> params)
	{
		string text = key;
		DCO_RadioPhrase phrase = FindPhrase(key);
		if (phrase && phrase.m_aVariants && idx >= 0 && idx < phrase.m_aVariants.Count())
			text = WidgetManager.Translate(phrase.m_aVariants[idx].m_sText);
		return Fill(text, params);
	}

	static string Fill(string text, array<string> params)
	{
		if (!params)
			return text;

		for (int i = 0; i + 1 < params.Count(); i += 2)
		{
			string value = params[i + 1];
			if (value.StartsWith("@"))
				value = SubList(value);
			text.Replace("{" + params[i] + "}", value);
		}
		return text;
	}

	protected static string SubList(string value)
	{
		array<string> parts = {};
		value.Split("+", parts, true);
		string outText;
		foreach (int i, string part : parts)
		{
			string word = part;
			if (part.StartsWith("@"))
				word = SubPhrase(part.Substring(1, part.Length() - 1));
			if (i > 0)
				outText += ", ";
			outText += word;
		}
		return outText;
	}

	protected static string SubPhrase(string key)
	{
		DCO_RadioPhrase phrase = FindPhrase(key);
		if (!phrase || !phrase.m_aVariants || phrase.m_aVariants.IsEmpty())
			return key;
		return WidgetManager.Translate(phrase.m_aVariants[0].m_sText);
	}

	protected static void Deliver(int pid, string title, string key, int idx, bool isOverride, array<string> params, DCO_ERadioKind kind)
	{
		SCR_PlayerControllerGroupComponent comp = SCR_PlayerControllerGroupComponent.GetPlayerControllerComponent(pid);
		if (!comp)
			return;

		if (isOverride)
		{
			array<ref DCO_RadioVariant> list = s_mOverride.Get(key);
			comp.DCO_SendRadio(title, Fill(list[idx].m_sText, params), kind);
			return;
		}

		array<string> send = params;
		if (!send)
			send = {};
		comp.DCO_SendRadioKey(title, key, idx, send, kind);
	}

	protected static int Pick(string key, SCR_AIGroup grp, bool critical, string lastKey, out bool isOverride)
	{
		LoadOverride();
		array<ref DCO_RadioVariant> list;
		isOverride = s_mOverride && s_mOverride.Find(key, list) && list && !list.IsEmpty();
		if (!isOverride)
		{
			DCO_RadioPhrase phrase = FindPhrase(key);
			if (!phrase || !phrase.m_aVariants || phrase.m_aVariants.IsEmpty())
				return -1;
			list = phrase.m_aVariants;
		}

		string faction;
		AICommander_BaseComponent cmd;
		if (grp)
		{
			if (grp.GetFaction())
				faction = grp.GetFaction().GetFactionKey();
			cmd = grp.DCO_GetOrdersCommander();
		}
		int pers = PersonalityOf(cmd);
		int urg = 0;
		if (critical)
			urg = 1;

		array<int> pool = {};
		for (int level = 0; level < 4; level++)
		{
			array<int> cand = {};
			foreach (int i, DCO_RadioVariant v : list)
			{
				if (!v || !Compatible(v, faction, pers, urg))
					continue;
				bool ok = true;
				if (level <= 2 && v.m_sFaction != faction)
					ok = false;
				if (level <= 1 && v.m_iUrgency != urg)
					ok = false;
				if (level == 0 && v.m_iPersonality != pers)
					ok = false;
				if (ok)
					cand.Insert(i);
			}
			if (cand.Count() >= 2)
			{
				pool = cand;
				break;
			}
			if (pool.IsEmpty() && !cand.IsEmpty())
				pool = cand;
		}

		if (pool.IsEmpty())
			return 0;

		string memo = lastKey + "|" + key;
		int last = -1;
		s_mLast.Find(memo, last);
		if (pool.Count() > 1)
			pool.RemoveItem(last);

		int pick = pool.GetRandomElement();
		s_mLast.Set(memo, pick);
		return pick;
	}

	protected static bool Compatible(DCO_RadioVariant v, string faction, int pers, int urg)
	{
		if (!v.m_sFaction.IsEmpty() && v.m_sFaction != faction)
			return false;
		if (v.m_iPersonality >= 0 && v.m_iPersonality != pers)
			return false;
		if (v.m_iUrgency >= 0 && v.m_iUrgency != urg)
			return false;
		return true;
	}

	protected static DCO_RadioPhrase FindPhrase(string key)
	{
		if (!s_mBank)
			LoadBank();
		return s_mBank.Get(key);
	}

	protected static void LoadBank()
	{
		s_mBank = new map<string, DCO_RadioPhrase>();
		Resource res = BaseContainerTools.LoadContainer(BANK);
		if (!res || !res.IsValid())
		{
			Print("[DCO_Radio] Bank radio gak ketemu: " + BANK, LogLevel.WARNING);
			return;
		}

		s_Bank = DCO_RadioBankConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(res.GetResource().ToBaseContainer()));
		if (!s_Bank || !s_Bank.m_aPhrases)
			return;

		foreach (DCO_RadioPhrase p : s_Bank.m_aPhrases)
		{
			if (p && !p.m_sKey.IsEmpty())
				s_mBank.Set(p.m_sKey, p);
		}
	}

	protected static void LoadOverride()
	{
		if (s_mOverride)
			return;

		s_mOverride = new map<string, ref array<ref DCO_RadioVariant>>();
		if (!s_mBank)
			LoadBank();

		if (!FileIO.FileExists(OVERRIDE_PATH))
		{
			WriteTemplate();
			return;
		}

		SCR_JsonLoadContext ctx = new SCR_JsonLoadContext();
		if (!ctx.LoadFromFile(OVERRIDE_PATH))
		{
			Print("[DCO_Radio] GAGAL parse " + OVERRIDE_PATH + " -- pakai bank default.", LogLevel.ERROR);
			return;
		}

		int keys;
		foreach (string key, DCO_RadioPhrase p : s_mBank)
		{
			array<string> texts = {};
			if (!ctx.ReadValue(key, texts) || texts.IsEmpty())
				continue;

			array<ref DCO_RadioVariant> list = {};
			foreach (string t : texts)
			{
				DCO_RadioVariant v = ParseVariant(t);
				if (v)
					list.Insert(v);
			}
			if (list.IsEmpty())
				continue;
			s_mOverride.Set(key, list);
			keys++;
		}
		PrintFormat("[DCO_Radio] %1 key radio di-override dari %2", keys, OVERRIDE_PATH);
	}

	protected static DCO_RadioVariant ParseVariant(string raw)
	{
		string t = raw.Trim();
		if (t.IsEmpty())
			return null;

		DCO_RadioVariant v = new DCO_RadioVariant();
		v.m_iPersonality = -1;
		v.m_iUrgency = -1;
		if (t.StartsWith("["))
		{
			int close = t.IndexOf("]");
			if (close > 0)
			{
				array<string> tags = {};
				t.Substring(1, close - 1).Split(",", tags, true);
				foreach (string tag : tags)
				{
					string tg = tag.Trim();
					tg.ToUpper();
					switch (tg)
					{
						case "AGGRESSIVE":	v.m_iPersonality = PERS_AGGRESSIVE; break;
						case "CAUTIOUS":	v.m_iPersonality = PERS_CAUTIOUS; break;
						case "MEASURED":	v.m_iPersonality = PERS_MEASURED; break;
						case "CRITICAL":	v.m_iUrgency = 1; break;
						case "CALM":		v.m_iUrgency = 0; break;
						default:			v.m_sFaction = tg; break;
					}
				}
				t = t.Substring(close + 1, t.Length() - close - 1).Trim();
			}
		}
		v.m_sText = t;
		return v;
	}

	protected static void WriteTemplate()
	{
		FileIO.MakeDirectory("$profile:DCO");
		SCR_JsonSaveContext save = new SCR_JsonSaveContext();
		save.WriteValue("_comment", "DCO radio override. Isi array key dengan teks sendiri (menggantikan bank default untuk key itu). Tag opsional di depan: [US,AGGRESSIVE,CRITICAL] teks. Faction = key faction, personality = AGGRESSIVE/CAUTIOUS/MEASURED, urgency = CRITICAL/CALM. Placeholder sama dengan bank default, contoh {callsign} {grid}. Array kosong = pakai bank default.");

		array<string> keys = {};
		foreach (string key, DCO_RadioPhrase p : s_mBank)
			keys.Insert(key);
		keys.Sort();

		foreach (string k : keys)
		{
			array<string> empty = {};
			save.WriteValue(k, empty);
		}

		if (save.SaveToFile(OVERRIDE_PATH))
			Print("[DCO_Radio] Template override ditulis ke " + OVERRIDE_PATH);
	}
}
