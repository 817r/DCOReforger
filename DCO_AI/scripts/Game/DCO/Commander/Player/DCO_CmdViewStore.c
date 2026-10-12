class DCO_CmdViewDecision
{
	float m_fAt;
	string m_sCmd;
	string m_sObj;
	string m_sText;
}

class DCO_CmdViewObjInfo
{
	string m_sDecision = "-";
	string m_sWhy = "";
	string m_sBand = "-";
	string m_sRatio = "-";
	string m_sEff = "-";
	string m_sConf = "-";
	string m_sEnemy = "-";
	string m_sPosture = "-";
	string m_sPhase = "-";
	string m_sPhaseWhy = "";
	string m_sTactic = "-";
	string m_sTacticWhy = "";
	string m_sTrPhase = "-";
	string m_sSync = "";
	float m_fSyncAt = -1;
	vector m_vStaging;
	float m_fAt = -1;
}

class DCO_CmdViewGrpInfo
{
	string m_sWhy = "";
	string m_sOrder = "";
	string m_sRole = "";
	float m_fAt = -1;
	vector m_vWp;
}

class DCO_CmdViewStore
{
	static bool s_bOn;
	protected static ref map<string, ref DCO_CmdViewObjInfo> s_mObj;
	protected static ref map<string, ref DCO_CmdViewGrpInfo> s_mGrp;
	protected static ref array<ref DCO_CmdViewDecision> s_aDec;
	protected static BaseWorld s_World;

	protected static void Init()
	{
		if (s_mObj && s_World == GetGame().GetWorld())
			return;
		s_World = GetGame().GetWorld();
		s_mObj = new map<string, ref DCO_CmdViewObjInfo>();
		s_mGrp = new map<string, ref DCO_CmdViewGrpInfo>();
		s_aDec = {};
	}

	static void Refresh()
	{
		s_bOn = false;
		if (!Replication.IsServer())
			return;
#ifdef WORKBENCH
		s_bOn = true;
#else
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetInstance();
		s_bOn = g && g.GetIntentOverlay();
#endif
		if (s_bOn)
			DCO_CmdViewServer.StartInfo();
	}

	static string Val(string text, string key)
	{
		string k = " " + key + "=";
		int st = text.IndexOf(k);
		if (st < 0)
			return "";
		st += k.Length();
		int en = text.IndexOfFrom(st, " ");
		if (en < 0)
			en = text.Length();
		return text.Substring(st, en - st);
	}

	static string Rest(string text, string key)
	{
		string k = " " + key + "=";
		int st = text.IndexOf(k);
		if (st < 0)
			return "";
		st += k.Length();
		return text.Substring(st, text.Length() - st);
	}

	static DCO_CmdViewObjInfo Obj(string cmd, string obj)
	{
		Init();
		string key = cmd + "|" + obj;
		DCO_CmdViewObjInfo o = s_mObj.Get(key);
		if (!o)
		{
			o = new DCO_CmdViewObjInfo();
			s_mObj.Set(key, o);
		}
		return o;
	}

	static DCO_CmdViewObjInfo PeekObj(string cmd, string obj)
	{
		if (!s_mObj)
			return null;
		DCO_CmdViewObjInfo o = s_mObj.Get(cmd + "|" + obj);
		if (!o)
			o = s_mObj.Get("|" + obj);
		return o;
	}

	static string LatestObj(string cmd, out DCO_CmdViewObjInfo best)
	{
		best = null;
		string bestName = "";
		if (!s_mObj)
			return bestName;
		string prefix = cmd + "|";
		foreach (string k, DCO_CmdViewObjInfo o : s_mObj)
		{
			if (!o || !k.StartsWith(prefix) || o.m_fAt < 0)
				continue;
			if (!best || o.m_fAt > best.m_fAt)
			{
				best = o;
				bestName = k.Substring(prefix.Length(), k.Length() - prefix.Length());
			}
		}
		return bestName;
	}

	static DCO_CmdViewGrpInfo PeekGrp(string id)
	{
		if (!s_mGrp)
			return null;
		return s_mGrp.Get(id);
	}

	static void Decisions(string cmd, string obj, int max, notnull array<DCO_CmdViewDecision> outList)
	{
		outList.Clear();
		if (!s_aDec)
			return;
		for (int i = s_aDec.Count() - 1; i >= 0 && outList.Count() < max; i--)
		{
			DCO_CmdViewDecision d = s_aDec[i];
			if (!obj.IsEmpty() && d.m_sObj != obj)
				continue;
			if (!cmd.IsEmpty() && !d.m_sCmd.IsEmpty() && d.m_sCmd != cmd)
				continue;
			outList.Insert(d);
		}
	}

	protected static void Note(string cmd, string obj, string text)
	{
		Init();
		DCO_CmdViewDecision d = new DCO_CmdViewDecision();
		d.m_fAt = DCO_Util.NowS();
		d.m_sCmd = cmd;
		d.m_sObj = obj;
		d.m_sText = text;
		s_aDec.Insert(d);
		if (s_aDec.Count() > 160)
			s_aDec.RemoveOrdered(0);
	}

	static void Capture(string text)
	{
		int sp = text.IndexOf(" ");
		if (sp <= 0)
			return;
		string kind = text.Substring(0, sp);
		switch (kind)
		{
			case "cmd_think":
			case "obj_skip":
			case "battle_phase":
			case "force_condition":
			case "tactic_chosen":
			case "tactic_phase":
			case "sync_wait":
			case "sync_release":
			case "staging_pos":
			case "cmd_order":
			case "tactic_none":
			case "tactic_plan_lost":
			case "tactic_result":
			case "tactic_abort":
			case "def_phase":
			case "tactic_forced":
			case "tactic_persist":
			case "tactic_culminate":
			case "op_scope":
			case "op_scope_keep":
				break;
			default:
				return;
		}
		Init();
		string cmd = Val(text, "cmd");
		string obj = Val(text, "obj");
		float now = DCO_Util.NowS();

		if (kind == "cmd_order")
		{
			string gid = Val(text, "grp");
			if (gid.IsEmpty())
				return;
			DCO_CmdViewGrpInfo g = s_mGrp.Get(gid);
			if (!g)
			{
				g = new DCO_CmdViewGrpInfo();
				s_mGrp.Set(gid, g);
				if (s_mGrp.Count() > 2000)
					s_mGrp.Clear();
			}
			g.m_sWhy = Val(text, "why");
			g.m_sOrder = Val(text, "order");
			g.m_sRole = Val(text, "role");
			g.m_fAt = now;
			g.m_vWp = ParseXZ(Val(text, "pos"));
			return;
		}

		DCO_CmdViewObjInfo o = Obj(cmd, obj);
		switch (kind)
		{
			case "cmd_think":
			{
				string dec = Val(text, "decision");
				dec.ToUpper();
				o.m_sDecision = dec;
				o.m_sWhy = Val(text, "why");
				o.m_sBand = Val(text, "band");
				o.m_sRatio = Val(text, "ratio");
				o.m_sEnemy = Val(text, "est_enemy");
				o.m_fAt = now;
				Note(cmd, obj, "think " + o.m_sDecision + " (" + o.m_sWhy + ") ratio " + o.m_sRatio + " " + o.m_sBand);
				return;
			}
			case "obj_skip":
			{
				o.m_sDecision = "SKIP";
				o.m_sWhy = Val(text, "reason");
				o.m_fAt = now;
				Note(cmd, obj, "skip - " + o.m_sWhy);
				return;
			}
			case "battle_phase":
			{
				o.m_sPhase = Val(text, "phase");
				o.m_sPhaseWhy = Val(text, "reason");
				if (o.m_sPhase == "wait")
				{
					o.m_sDecision = "WAIT";
					o.m_sWhy = o.m_sPhaseWhy;
				}
				Note(cmd, obj, "phase " + o.m_sPhase + " - " + o.m_sPhaseWhy);
				return;
			}
			case "force_condition":
			{
				o.m_sRatio = Val(text, "raw");
				o.m_sEff = Val(text, "eff");
				o.m_sBand = Val(text, "band");
				o.m_sConf = Val(text, "conf");
				o.m_sEnemy = Val(text, "enemy");
				o.m_sPosture = Val(text, "posture");
				return;
			}
			case "tactic_chosen":
			{
				o.m_sTactic = Val(text, "tactic");
				o.m_sTacticWhy = Val(text, "reason");
				string tw = "";
				if (!o.m_sTacticWhy.IsEmpty())
					tw = " - " + o.m_sTacticWhy;
				Note(cmd, obj, "tactic " + o.m_sTactic + tw);
				return;
			}
			case "tactic_phase":
			{
				o.m_sTrPhase = Val(text, "phase");
				string pw = Val(text, "reason");
				if (!pw.IsEmpty())
					pw = " - " + pw;
				Note(cmd, obj, "phase " + o.m_sTrPhase + pw);
				return;
			}
			case "sync_wait":
			{
				string here = Val(text, "here");
				string contact = Val(text, "contact_here");
				string dl = Val(text, "deadline_in");
				o.m_sSync = here + " groups at staging";
				if (!contact.IsEmpty() && contact != "0")
					o.m_sSync += " · contact_here " + contact;
				if (!dl.IsEmpty())
					o.m_sSync += " · deadline " + Clock(dl.ToFloat());
				string strength = Val(text, "strength");
				if (!strength.IsEmpty())
					o.m_sSync += " · strength " + strength + "/" + Val(text, "need") + " (" + Val(text, "src") + ")";
				o.m_fSyncAt = now;
				return;
			}
			case "sync_release":
			{
				o.m_sSync = "released";
				o.m_fSyncAt = now;
				string rr = Val(text, "reason");
				if (rr.IsEmpty())
					rr = "timeout " + Val(text, "timeout");
				string rh = Val(text, "here");
				if (!rh.IsEmpty())
					rr += " · here " + rh;
				Note(cmd, obj, "sync release - " + rr);
				return;
			}
			case "staging_pos":
			{
				o.m_vStaging = ParseXZ(Val(text, "pos"));
				return;
			}
			case "tactic_none":
			{
				Note(cmd, obj, "no tactic - " + Val(text, "reason"));
				return;
			}
			case "tactic_plan_lost":
			{
				Note(cmd, obj, "plan lost - " + Val(text, "tactic"));
				return;
			}
			case "tactic_result":
			{
				Note(cmd, obj, "result " + Val(text, "result") + " - " + Val(text, "reason"));
				return;
			}
			case "tactic_abort":
			{
				Note(cmd, obj, "abort - " + Val(text, "reason"));
				return;
			}
			case "tactic_forced":
			{
				Note(cmd, obj, "forced " + Val(text, "tactic") + " groups " + Val(text, "groups"));
				return;
			}
			case "tactic_persist":
			{
				Note(cmd, obj, "persist " + Val(text, "reason") + " -> " + Val(text, "action"));
				return;
			}
			case "tactic_culminate":
			{
				Note(cmd, obj, "culminate " + Val(text, "reason") + " losses " + Val(text, "losses_pct") + "%");
				return;
			}
			case "op_scope":
			{
				Note(cmd, Val(text, "pick"), "scope pick why=" + Val(text, "why"));
				return;
			}
			case "op_scope_keep":
			{
				Note(cmd, Val(text, "pick"), "scope keep (would " + Val(text, "would_pick") + ")");
				return;
			}
			case "def_phase":
			{
				o.m_sTactic = Val(text, "tactic");
				o.m_sTrPhase = Val(text, "phase");
				Note(cmd, obj, "defense " + o.m_sTactic + " " + o.m_sTrPhase);
				return;
			}
		}
	}

	static vector ParseXZ(string xz)
	{
		int c = xz.IndexOf(",");
		if (c <= 0)
			return vector.Zero;
		float x = xz.Substring(0, c).ToFloat();
		float z = xz.Substring(c + 1, xz.Length() - c - 1).ToFloat();
		return Vector(x, 0, z);
	}

	static string Clock(float s)
	{
		int t = Math.Max(0, Math.Round(s));
		int m = t / 60;
		int r = t % 60;
		string rs = r.ToString();
		if (r < 10)
			rs = "0" + rs;
		return m.ToString() + ":" + rs;
	}
}
