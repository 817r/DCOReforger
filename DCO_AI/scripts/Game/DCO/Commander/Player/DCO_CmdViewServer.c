enum DCO_ECVLayer
{
	PLAN = 1,
	GROUPS = 2,
	ENEMY = 4,
	DECISIONS = 8,
	TRUTH = 16
}

enum DCO_ECVRec
{
	CMD = 1,
	OBJ = 2,
	GRP = 3,
	GEO = 4,
	CLU = 5,
	TRU = 6,
	DEC = 7
}

enum DCO_ECVGeo
{
	AXIS = 1,
	STAGING = 2,
	ATTACK = 3,
	SBF = 4,
	RALLY = 5,
	LZ = 6,
	TRIGGER = 7
}

class DCO_CmdViewServer
{
	protected static ref map<int, int> s_mSubs;
	protected static float s_fNextLog;
	protected static bool s_bTicking;

	static bool Allowed(int playerId)
	{
#ifdef WORKBENCH
		return true;
#else
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetInstance();
		if (!g || !g.GetIntentOverlay())
			return false;
		SCR_EditorManagerCore core = SCR_EditorManagerCore.Cast(SCR_EditorManagerCore.GetInstance(SCR_EditorManagerCore));
		if (!core)
			return false;
		SCR_EditorManagerEntity em = core.GetEditorManager(playerId);
		return em && !em.IsLimited();
#endif
	}

	static void Subscribe(int playerId, bool on, int layers)
	{
		if (!Replication.IsServer())
			return;
		if (!s_mSubs)
			s_mSubs = new map<int, int>();
		DCO_CmdViewStore.Refresh();
		if (!on)
		{
			s_mSubs.Remove(playerId);
			DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", string.Format("cmdview_sub player=%1 on=0 subs=%2", playerId, s_mSubs.Count()));
			return;
		}
		bool ok = Allowed(playerId);
		DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", string.Format("cmdview_sub player=%1 on=1 layers=%2 allowed=%3 subs=%4", playerId, layers, DCO_Util.B01(ok), s_mSubs.Count()));
		if (!ok)
		{
			s_mSubs.Remove(playerId);
			array<float> f = {};
			array<string> s = {};
			f.Insert(1);
			f.Insert(1);
			f.Insert(DCO_Util.NowS());
			Deliver(playerId, f, s);
			return;
		}
		s_mSubs.Set(playerId, layers);
		Send(playerId, layers);
		DCO_CmdDbg.SendTo(playerId);
		if (!s_bTicking)
		{
			s_bTicking = true;
			GetGame().GetCallqueue().CallLater(Tick, 2000, true);
		}
	}

	protected static void Tick()
	{
		if (!s_mSubs || s_mSubs.IsEmpty())
		{
			s_bTicking = false;
			GetGame().GetCallqueue().Remove(Tick);
			return;
		}
		array<int> drop = {};
		foreach (int pid, int layers : s_mSubs)
		{
			if (!Allowed(pid) || !GetGame().GetPlayerManager().GetPlayerController(pid))
			{
				drop.Insert(pid);
				continue;
			}
			Send(pid, layers);
			DCO_CmdDbg.SendTo(pid);
		}
		foreach (int d : drop)
			s_mSubs.Remove(d);
	}

	protected static void Deliver(int playerId, array<float> f, array<string> s)
	{
		PlayerController local = GetGame().GetPlayerController();
		if (local && local.GetPlayerId() == playerId)
		{
			DCO_CmdViewUI.Receive(f, s);
			return;
		}
		SCR_PlayerControllerGroupComponent comp = SCR_PlayerControllerGroupComponent.GetPlayerControllerComponent(playerId);
		if (comp)
			comp.DCO_CVSendSnap(f, s);
	}

	protected static void DeliverDetail(int playerId, int type, string id, array<string> rows)
	{
		PlayerController local = GetGame().GetPlayerController();
		if (local && local.GetPlayerId() == playerId)
		{
			DCO_CmdViewUI.ReceiveDetail(type, id, rows);
			return;
		}
		SCR_PlayerControllerGroupComponent comp = SCR_PlayerControllerGroupComponent.GetPlayerControllerComponent(playerId);
		if (comp)
			comp.DCO_CVSendDetail(type, id, rows);
	}

	protected static int Str(array<string> s, string v)
	{
		s.Insert(v);
		return s.Count() - 1;
	}

	static int ColorOf(string faction)
	{
		string f = faction;
		f.ToUpper();
		if (f.StartsWith("US") && !f.StartsWith("USSR"))
			return 0x2F6FD0;
		if (f.StartsWith("USSR") || f.StartsWith("RU"))
			return 0xC8352D;
		if (f.StartsWith("FIA"))
			return 0x3E9A3E;
		return 0xE3A13B;
	}

	static array<AICommander_BaseComponent> Commanders()
	{
		array<AICommander_BaseComponent> list = {};
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
			return list;
		foreach (AICommander_BaseComponent c : mgr.m_aCommander)
		{
			if (c)
				list.Insert(c);
		}
		return list;
	}

	static int ObjKey(CMD_AICommanderObjectiveComponent obj)
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr || !obj)
			return -1;
		return mgr.m_aObjective.Find(obj);
	}

	static CMD_AICommanderObjectiveComponent ObjByKey(int key)
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr || key < 0 || key >= mgr.m_aObjective.Count())
			return null;
		return mgr.m_aObjective[key];
	}

	static int RoleOf(AICommander_BaseComponent cmd, DCO_GroupUtilityComponent g)
	{
		CMD_AICommanderObjectiveComponent obj = g.GetGroupObjective();
		if (obj && cmd.GetTactics())
		{
			DCO_TacticPlan p = cmd.GetTactics().GetPlan(obj);
			if (p)
			{
				int i = p.m_aGroups.Find(g);
				if (i >= 0 && i < p.m_aRoles.Count())
				{
					int r = p.m_aRoles[i];
					if (r == DCO_CommanderTactics.ROLE_FIX)
					{
						if (g.GetTask() == DCO_EGroupTask.SUPPORT_BY_FIRE)
							return 2;
						return 0;
					}
					if (r == DCO_CommanderTactics.ROLE_ASSAULT)
						return 1;
					if (r == DCO_CommanderTactics.ROLE_BLOCK)
						return 7;
				}
			}
		}
		switch (g.GetTask())
		{
			case DCO_EGroupTask.ATTACK:
			case DCO_EGroupTask.FLANK:
			case DCO_EGroupTask.REINFORCE:
				return 1;
			case DCO_EGroupTask.SUPPORT_BY_FIRE:
			case DCO_EGroupTask.FIRE_MISSION:
				return 2;
			case DCO_EGroupTask.RECON:
				return 3;
			case DCO_EGroupTask.PATROL:
				return 5;
			case DCO_EGroupTask.TRANSPORT:
				return 6;
			case DCO_EGroupTask.DEFEND:
			case DCO_EGroupTask.GARRISON:
				return 7;
		}
		return 4;
	}

	static string RoleName(int role)
	{
		switch (role)
		{
			case 0: return "Fix";
			case 1: return "Assault";
			case 2: return "Support by fire";
			case 3: return "Recon";
			case 5: return "Patrol";
			case 6: return "Transport";
			case 7: return "Defend";
		}
		return "Reserve";
	}

	static string RoleLetter(int role)
	{
		switch (role)
		{
			case 0: return "F";
			case 1: return "A";
			case 2: return "S";
			case 3: return "R";
			case 5: return "P";
			case 6: return "T";
			case 7: return "D";
		}
		return "Res";
	}

	static string Compass(vector from, vector to)
	{
		vector d = to - from;
		d[1] = 0;
		if (d.LengthSq() < 900.0)
			return "";
		float yaw = Math.Repeat(d.ToYaw(), 360.0);
		int i = Math.Round(yaw / 45.0);
		switch (i % 8)
		{
			case 0: return "north";
			case 1: return "north-east";
			case 2: return "east";
			case 3: return "south-east";
			case 4: return "south";
			case 5: return "south-west";
			case 6: return "west";
		}
		return "north-west";
	}

	static vector DestOf(DCO_GroupUtilityComponent g)
	{
		AIGroup ag = AIGroup.Cast(g.GetOwner());
		if (ag && ag.GetCurrentWaypoint())
			return ag.GetCurrentWaypoint().GetOrigin();
		return vector.Zero;
	}

	static string TaskSentence(DCO_GroupUtilityComponent g, int role)
	{
		CMD_AICommanderObjectiveComponent obj = g.GetGroupObjective();
		string on = "";
		string from = "";
		if (obj)
		{
			on = obj.GetDisplayName();
			vector dest = DestOf(g);
			if (dest != vector.Zero)
				from = Compass(obj.GetOwner().GetOrigin(), dest);
		}
		string fromTxt = "";
		if (!from.IsEmpty())
			fromTxt = " from the " + from;
		switch (role)
		{
			case 0: return "Fix " + on + fromTxt;
			case 1: return "Assault " + on + fromTxt;
			case 2: return "Support by fire on " + on + fromTxt;
			case 3: return "Recon " + on;
			case 5: return "Patrol near " + on;
			case 6: return "Transport";
			case 7: return "Defend " + on;
		}
		if (!on.IsEmpty())
			return "Reserve for " + on;
		return "Reserve";
	}

	static string TacticShort(int t)
	{
		string n = DCO_CommanderTactics.Name(t);
		if (n == "FIRE_AND_MANEUVER")
			return "F&M";
		if (n == "HAMMER_AND_ANVIL")
			return "H&A";
		n.Replace("_", " ");
		return n;
	}

	static string ObjShort(CMD_AICommanderObjectiveComponent obj)
	{
		if (!obj)
			return "";
		string n = obj.GetDisplayName();
		n.Replace("_Objective", "");
		n.Replace(" Objective", "");
		if (n.Length() > 18)
			n = n.Substring(0, 18);
		return n;
	}

	static string PlanText(AICommander_BaseComponent cmd, DCO_GroupUtilityComponent g, int role)
	{
		CMD_AICommanderObjectiveComponent obj = g.GetGroupObjective();
		string tac = "";
		if (obj && cmd.GetTactics())
		{
			DCO_TacticPlan p = cmd.GetTactics().GetPlan(obj);
			if (p)
				tac = TacticShort(p.m_eTactic);
		}
		if (tac.IsEmpty() && obj && cmd.GetDefenseTactics() && cmd.GetDefenseTactics().GetPlan(obj))
			tac = "Defense";
		string t = tac;
		string os = ObjShort(obj);
		if (!os.IsEmpty())
		{
			if (!t.IsEmpty())
				t += " · ";
			t += os;
		}
		if (!t.IsEmpty())
			t += " · ";
		return t + RoleName(role);
	}

	static string ShortLine(AICommander_BaseComponent cmd, DCO_GroupUtilityComponent g, int role)
	{
		string line = RoleName(role);
		CMD_AICommanderObjectiveComponent obj = g.GetGroupObjective();
		string tac = "";
		if (obj && cmd.GetTactics())
		{
			DCO_TacticPlan p = cmd.GetTactics().GetPlan(obj);
			if (p)
				tac = TacticShort(p.m_eTactic);
		}
		string os = ObjShort(obj);
		if (!tac.IsEmpty() || !os.IsEmpty())
		{
			line += " ·";
			if (!tac.IsEmpty())
				line += " " + tac;
			if (!os.IsEmpty())
				line += " " + os;
		}
		if (g.m_iDCOTRPhase >= 0)
			line += " · " + typename.EnumToString(DCO_ETRPhase, g.m_iDCOTRPhase);
		return line;
	}

	protected static bool s_bInfoTicking;

	static void StartInfo()
	{
		if (s_bInfoTicking || !Replication.IsServer())
			return;
		s_bInfoTicking = true;
		GetGame().GetCallqueue().CallLater(TickInfo, 2000, true);
	}

	protected static void TickInfo()
	{
		if (!DCO_CmdViewStore.s_bOn)
		{
			s_bInfoTicking = false;
			GetGame().GetCallqueue().Remove(TickInfo);
			return;
		}
		foreach (AICommander_BaseComponent cmd : Commanders())
		{
			foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
			{
				if (!g)
					continue;
				SCR_AIGroup ag = SCR_AIGroup.Cast(g.GetOwner());
				if (!ag)
					continue;
				int role = RoleOf(cmd, g);
				string phase = "-";
				if (g.m_iDCOTRPhase >= 0)
					phase = typename.EnumToString(DCO_ETRPhase, g.m_iDCOTRPhase);
				string why = "-";
				DCO_CmdViewGrpInfo gi = DCO_CmdViewStore.PeekGrp(DCO_Log.IdGroup(ag));
				if (gi && !gi.m_sWhy.IsEmpty())
					why = gi.m_sWhy + " · " + gi.m_sOrder;
				ag.DCO_SetPlanInfo(PlanText(cmd, g, role), phase, why);
			}
		}
	}

	static string CmdTitle(AICommander_BaseComponent cmd)
	{
		string mode = typename.EnumToString(CMD_ECommanderMode, cmd.GetCommanderMode());
		string doc = "-";
		DCO_Doctrine d = cmd.GetDoctrine();
		if (d && !d.m_sPreset.IsEmpty())
			doc = d.m_sPreset;
		return cmd.GetCommanderUID() + " · " + cmd.GetCommanderFactionKey() + " · " + mode + " · doctrine " + doc;
	}

	static string NowLine(AICommander_BaseComponent cmd)
	{
		DCO_CmdViewObjInfo info = null;
		string obj = DCO_CmdViewStore.LatestObj(cmd.GetCommanderUID(), info);
		if (!info)
			return "Now: thinking, no decision yet";
		if (info.m_sDecision == "SKIP" || info.m_sDecision == "WAIT")
			return "Now: " + obj + ": " + info.m_sDecision + " " + info.m_sWhy;
		string line = "Now: " + obj + ": " + info.m_sDecision + " (band " + info.m_sBand + " eff " + info.m_sEff + ")";
		if (info.m_sTactic != "-")
			line += " -> " + info.m_sTactic;
		if (!info.m_sSync.IsEmpty() && info.m_fSyncAt > 0 && DCO_Util.NowS() - info.m_fSyncAt < 120.0)
			line += " · sync " + info.m_sSync;
		return line;
	}

	static string PlanLines(AICommander_BaseComponent cmd)
	{
		string lines = "";
		if (!cmd.GetTactics())
			return lines;
		foreach (CMD_AICommanderObjectiveComponent obj : cmd.GetObjectiveList())
		{
			if (!obj)
				continue;
			DCO_TacticPlan p = cmd.GetTactics().GetPlan(obj);
			if (!p)
				continue;
			string phase = "-";
			DCO_CmdViewObjInfo info = DCO_CmdViewStore.PeekObj(cmd.GetCommanderUID(), obj.GetLogName());
			if (info && info.m_sTrPhase != "-")
				phase = info.m_sTrPhase;
			string fix = "-";
			int atAP = 0;
			int nAssault = 0;
			foreach (int i, DCO_GroupUtilityComponent g : p.m_aGroups)
			{
				if (!g || i >= p.m_aRoles.Count())
					continue;
				int r = p.m_aRoles[i];
				if (r == DCO_CommanderTactics.ROLE_FIX)
				{
					if (g.m_bDCOTRReady)
						fix = "ready";
					else if (g.IsInContact())
						fix = "contact";
					else if (g.IsMoving())
						fix = "moving";
					else if (fix == "-")
						fix = "waiting";
				}
				else if (r == DCO_CommanderTactics.ROLE_ASSAULT)
				{
					nAssault++;
					if (g.m_iDCOTRPhase >= DCO_ETRPhase.STAGING)
						atAP++;
				}
			}
			if (!lines.IsEmpty())
				lines += "\n";
			lines += ObjShort(obj) + " " + TacticShort(p.m_eTactic) + " " + phase + " fix:" + fix + " assault:" + atAP.ToString() + "/" + nAssault.ToString();
		}
		return lines;
	}

	static string DecLines(string uid, int max)
	{
		array<DCO_CmdViewDecision> decs = {};
		DCO_CmdViewStore.Decisions(uid, "", max, decs);
		string lines = "";
		foreach (DCO_CmdViewDecision d : decs)
		{
			if (!lines.IsEmpty())
				lines += "\n";
			lines += "T+" + DCO_CmdViewStore.Clock(d.m_fAt) + " " + d.m_sObj + " " + d.m_sText;
		}
		return lines;
	}

	static string ForceLine(AICommander_BaseComponent cmd)
	{
		int groups = 0;
		int reserve = 0;
		foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
		{
			if (!g)
				continue;
			groups++;
			if (g.IsAvailableReserve())
				reserve++;
		}
		string arty = "-";
		if (cmd.GetArtySupport())
			arty = cmd.GetArtySupport().GetQueueCount().ToString();
		return "Forces: groups=" + groups.ToString() + " manpower=" + cmd.GetTotalManpower().ToString() + " reserve=" + reserve.ToString() + " arty_queue=" + arty;
	}

	protected static void DetailCmd(AICommander_BaseComponent cmd, int ci, array<string> rows)
	{
		string uid = cmd.GetCommanderUID();
		Row(rows, "K", "Commander · " + cmd.GetCommanderFactionKey());
		Row(rows, "T", uid);
		Row(rows, "S", typename.EnumToString(CMD_ECommanderMode, cmd.GetCommanderMode()) + " · " + CmdTitle(cmd));
		Row(rows, "W", NowLine(cmd));
		CommanderRows(cmd, rows);
		if (cmd.DCO_GetOpPlanner())
			cmd.DCO_GetOpPlanner().PanelRows(rows);
		Row(rows, "H", "Objectives");
		int no = 0;
		foreach (CMD_AICommanderObjectiveComponent obj : cmd.GetObjectiveList())
		{
			if (!obj)
				continue;
			DCO_CmdViewObjInfo info = DCO_CmdViewStore.PeekObj(uid, obj.GetLogName());
			string dec = DecisionOf(cmd, obj, info);
			if (dec == "-" && !info)
				continue;
			string tactic = "";
			if (cmd.GetTactics() && cmd.GetTactics().GetPlan(obj))
				tactic = DCO_CommanderTactics.Name(cmd.GetTactics().GetPlan(obj).m_eTactic);
			Row(rows, "O", ci.ToString() + ":" + ObjKey(obj).ToString(), obj.GetDisplayName() + ": " + dec + " - " + WhySentence(info, dec, tactic));
			no++;
		}
		if (no == 0)
			Row(rows, "N", "No objective assessed yet.");
		Row(rows, "H", "Plans");
		string plans = PlanLines(cmd);
		if (plans.IsEmpty())
			Row(rows, "N", "No active tactic plan.");
		else
		{
			array<string> pl = {};
			plans.Split("\n", pl, true);
			foreach (string ln : pl)
				Row(rows, "N", ln);
		}
		Row(rows, "H", "Forces");
		Row(rows, "N", ForceLine(cmd));
		Row(rows, "H", "Decisions");
		array<DCO_CmdViewDecision> decs = {};
		DCO_CmdViewStore.Decisions(uid, "", 12, decs);
		foreach (DCO_CmdViewDecision d : decs)
			Row(rows, "D", "T+" + DCO_CmdViewStore.Clock(d.m_fAt), d.m_sObj + " " + d.m_sText);
		if (decs.IsEmpty())
			Row(rows, "N", "No decisions recorded yet.");
	}

	static int DecisionCode(string d)
	{
		switch (d)
		{
			case "ATTACK": return 0;
			case "HOLD": return 1;
			case "DEFEND": return 2;
			case "SKIP": return 3;
			case "WAIT": return 4;
		}
		return 5;
	}

	static string DecisionOf(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, DCO_CmdViewObjInfo info)
	{
		if (cmd.GetDefenseTactics() && cmd.GetDefenseTactics().GetPlan(obj))
			return "DEFEND";
		if (cmd.GetTactics() && cmd.GetTactics().GetPlan(obj))
			return "ATTACK";
		if (info && info.m_sDecision != "-")
			return info.m_sDecision;
		if (obj.IsOwnedBy(cmd.GetCommanderFactionKey()))
			return "HOLD";
		return "-";
	}

	static string WhySentence(DCO_CmdViewObjInfo info, string decision, string tactic)
	{
		if (!info)
			return "No decision recorded yet.";
		if (decision == "SKIP")
			return "Skipped: " + info.m_sWhy;
		if (decision == "WAIT")
			return "Waiting: " + info.m_sWhy;
		string s = "enemy ~" + info.m_sEnemy;
		if (info.m_sPosture != "-")
			s += " (posture " + info.m_sPosture + ")";
		s += " -> ratio " + info.m_sEff + " " + info.m_sBand;
		if (info.m_sPhase != "-")
			s += " -> doctrine " + info.m_sPhase;
		if (!tactic.IsEmpty() && tactic != "-")
			s += " -> " + tactic;
		else if (!info.m_sWhy.IsEmpty())
			s += " -> " + info.m_sWhy;
		return s;
	}

	protected static void Geo(array<float> f, int ci, int ok, int kind, vector a, vector b, bool past, float w)
	{
		f.Insert(DCO_ECVRec.GEO);
		f.Insert(ci);
		f.Insert(ok);
		f.Insert(kind);
		f.Insert(a[0]);
		f.Insert(a[2]);
		f.Insert(b[0]);
		f.Insert(b[2]);
		f.Insert(DCO_Util.B01(past));
		f.Insert(w);
	}

	static void Send(int playerId, int layers)
	{
		int pt = DCO_Perf.Begin();
		int t0 = System.GetTickCount();
		array<float> f = {};
		array<string> s = {};
		f.Insert(1);
		f.Insert(0);
		f.Insert(DCO_Util.NowS());
		float now = DCO_Util.NowS();
		int nObj = 0;
		int nGrp = 0;
		int nClu = 0;
		string cmdList = "";
		array<AICommander_BaseComponent> cmds = Commanders();
		foreach (int ci, AICommander_BaseComponent cmd : cmds)
		{
			string uid = cmd.GetCommanderUID();
			string fac = cmd.GetCommanderFactionKey();
			if (!cmdList.IsEmpty())
				cmdList += "|";
			cmdList += uid;
			f.Insert(DCO_ECVRec.CMD);
			f.Insert(ci);
			f.Insert(Str(s, uid));
			f.Insert(Str(s, fac));
			f.Insert(ColorOf(fac));
			vector hq = cmd.GetOwner().GetOrigin();
			f.Insert(hq[0]);
			f.Insert(hq[1]);
			f.Insert(hq[2]);
			f.Insert(Str(s, CmdTitle(cmd)));
			f.Insert(Str(s, NowLine(cmd)));
			f.Insert(Str(s, PlanLines(cmd)));
			f.Insert(Str(s, DecLines(uid, 3)));
			f.Insert(Str(s, ForceLine(cmd)));

			map<CMD_AICommanderObjectiveComponent, int> grpCount = new map<CMD_AICommanderObjectiveComponent, int>();
			foreach (DCO_GroupUtilityComponent cg : cmd.GetOwnedGroups())
			{
				if (cg && cg.GetGroupObjective())
					grpCount.Set(cg.GetGroupObjective(), grpCount.Get(cg.GetGroupObjective()) + 1);
			}

			foreach (CMD_AICommanderObjectiveComponent obj : cmd.GetObjectiveList())
			{
				if (!obj)
					continue;
				int ok = ObjKey(obj);
				DCO_CmdViewObjInfo info = DCO_CmdViewStore.PeekObj(uid, obj.GetLogName());
				string dec = DecisionOf(cmd, obj, info);
				DCO_TacticPlan p = null;
				if (cmd.GetTactics())
					p = cmd.GetTactics().GetPlan(obj);
				string tactic = "";
				string phase = "";
				if (p)
				{
					tactic = DCO_CommanderTactics.Name(p.m_eTactic);
					if (info && info.m_sTrPhase != "-")
						phase = info.m_sTrPhase;
				}
				else if (info && info.m_sTactic != "-" && dec == "DEFEND")
					tactic = info.m_sTactic;
				int gc = grpCount.Get(obj);
				if (dec == "-" && gc == 0 && !info)
					continue;
				string card = uid + ": " + dec;
				if (!tactic.IsEmpty())
					card += " · " + tactic;
				if (!phase.IsEmpty())
					card += " " + phase;
				if (info && !info.m_sSync.IsEmpty() && info.m_fSyncAt > 0 && now - info.m_fSyncAt < 120.0)
				{
					int sl = info.m_sSync.IndexOf(" ");
					if (sl > 0)
						card += " · sync " + info.m_sSync.Substring(0, sl);
				}
				if (info && info.m_fAt > 0)
					card += " · T+" + DCO_CmdViewStore.Clock(info.m_fAt);
				string tip = obj.GetDisplayName() + ": " + dec;
				if (info)
					tip += " · " + info.m_sBand;
				vector op = obj.GetOwner().GetOrigin();
				f.Insert(DCO_ECVRec.OBJ);
				f.Insert(ci);
				f.Insert(ok);
				f.Insert(op[0]);
				f.Insert(op[2]);
				f.Insert(obj.GetRadius());
				f.Insert(DecisionCode(dec));
				f.Insert(Str(s, obj.GetDisplayName()));
				f.Insert(Str(s, card));
				f.Insert(Str(s, tip + "\n" + WhySentence(info, dec, tactic)));
				f.Insert(gc);
				string syncTxt = "";
				if (info && !info.m_sSync.IsEmpty() && info.m_fSyncAt > 0 && now - info.m_fSyncAt < 120.0)
					syncTxt = "sync " + info.m_sSync;
				f.Insert(Str(s, syncTxt));
				nObj++;

				if ((layers & DCO_ECVLayer.PLAN) == 0)
					continue;
				vector staging = vector.Zero;
				if (info)
					staging = info.m_vStaging;
				if (p && staging != vector.Zero)
				{
					Geo(f, ci, ok, DCO_ECVGeo.STAGING, staging, staging, false, 0);
					Geo(f, ci, ok, DCO_ECVGeo.AXIS, staging, op, false, 1);
				}
				else if (staging != vector.Zero && dec == "ATTACK")
				{
					Geo(f, ci, ok, DCO_ECVGeo.STAGING, staging, staging, false, 0);
					Geo(f, ci, ok, DCO_ECVGeo.AXIS, staging, op, false, 1);
				}
				if (cmd.GetDefenseTactics())
				{
					DCO_DefensePlan dp = cmd.GetDefenseTactics().GetPlan(obj);
					if (dp && dp.m_fTrigger > 0)
					{
						vector axis = dp.m_vAxis;
						if (axis == vector.Zero)
							axis = dp.m_vThreat - op;
						axis = DCO_Specialist.Flat(axis);
						if (axis != vector.Zero)
						{
							vector side = Vector(-axis[2], 0, axis[0]);
							vector tc = op + axis * dp.m_fTrigger;
							Geo(f, ci, ok, DCO_ECVGeo.TRIGGER, tc - side * 150.0, tc + side * 150.0, dp.m_bFired, 0);
						}
					}
				}
			}

			if ((layers & DCO_ECVLayer.PLAN) != 0 && cmd.GetLogistics())
			{
				foreach (DCO_LogiJob job : cmd.GetLogistics().GetJobs())
				{
					if (job && job.m_vLZ != vector.Zero)
						Geo(f, ci, -1, DCO_ECVGeo.LZ, job.m_vLZ, job.m_vLZ, false, 0);
				}
			}

			if ((layers & DCO_ECVLayer.GROUPS) != 0)
			{
				foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
				{
					if (!g || !g.GetOwner() || g.GetUnitCount() <= 0)
						continue;
					SCR_AIGroup ag = SCR_AIGroup.Cast(g.GetOwner());
					string gid = DCO_Log.IdGroup(ag);
					int role = RoleOf(cmd, g);
					vector gp = g.GetOwner().GetOrigin();
					IEntity lead = null;
					if (ag)
						lead = ag.GetLeaderEntity();
					if (lead)
						gp = lead.GetOrigin();
					vector dest = DestOf(g);
					DCO_CmdViewGrpInfo gi = DCO_CmdViewStore.PeekGrp(gid);
					float age = -1;
					if (gi && gi.m_fAt > 0)
						age = now - gi.m_fAt;
					int flags = 0;
					if (g.IsInContact())
						flags |= 1;
					if (g.IsMoving())
						flags |= 2;
					if (dest != vector.Zero && vector.DistanceXZ(dest, gp) > 15.0)
						flags |= 4;
					if (g.GetGroupObjective() && cmd.GetTactics() && cmd.GetTactics().GetPlan(g.GetGroupObjective()))
						flags |= 8;
					f.Insert(DCO_ECVRec.GRP);
					f.Insert(ci);
					f.Insert(ObjKey(g.GetGroupObjective()));
					f.Insert(gp[0]);
					f.Insert(gp[2]);
					f.Insert(role);
					f.Insert(flags);
					f.Insert(dest[0]);
					f.Insert(dest[2]);
					f.Insert(age);
					f.Insert(Str(s, g.GetOwner().GetName()));
					f.Insert(Str(s, gid));
					f.Insert(Str(s, TaskSentence(g, role)));
					f.Insert(gp[1]);
					f.Insert(Str(s, ShortLine(cmd, g, role)));
					nGrp++;
				}
			}

			if ((layers & DCO_ECVLayer.ENEMY) != 0 && cmd.GetThreatResponseComponent())
				nClu += Clusters(f, ci, cmd, now);
		}

		if ((layers & DCO_ECVLayer.TRUTH) != 0)
			Truth(f, cmds);

		if ((layers & DCO_ECVLayer.DECISIONS) != 0)
		{
			array<DCO_CmdViewDecision> decs = {};
			DCO_CmdViewStore.Decisions("", "", 24, decs);
			foreach (DCO_CmdViewDecision d : decs)
			{
				int dci = -1;
				foreach (int k, AICommander_BaseComponent kc : cmds)
				{
					if (kc.GetCommanderUID() == d.m_sCmd)
						dci = k;
				}
				f.Insert(DCO_ECVRec.DEC);
				f.Insert(dci);
				f.Insert(Str(s, d.m_sObj));
				f.Insert(d.m_fAt);
				f.Insert(Str(s, d.m_sText));
			}
		}

		Deliver(playerId, f, s);
		DCO_Perf.Count("rpc:cmdview");
		DCO_Perf.End("DCO_CmdView", pt);
		if (DCO_Log.On(DCO_ELogChannel.SYS, DCO_ELogLevel.DEBUG))
		{
			int bytes = f.Count() * 4;
			foreach (string str : s)
				bytes += str.Length() + 2;
			DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.DEBUG, "-", string.Format("cmdview_snapshot player=%1 cmd=%2 objs=%3 groups=%4 clusters=%5 bytes=%6 ms=%7 layers=%8", playerId, cmdList, nObj, nGrp, nClu, bytes, System.GetTickCount() - t0, layers));
		}
	}

	protected static int Clusters(array<float> f, int ci, AICommander_BaseComponent cmd, float now)
	{
		array<ref CMD_ThreatEntry> threats = cmd.GetThreatResponseComponent().GetThreats();
		array<vector> cPos = {};
		array<float> cEst = {};
		array<float> cLast = {};
		array<float> cRad = {};
		array<float> cConf = {};
		array<int> cN = {};
		array<int> cRep = {};
		foreach (CMD_ThreatEntry t : threats)
		{
			if (!t)
				continue;
			vector p = t.m_vBelievedPos;
			if (p == vector.Zero)
				p = t.m_vPosition;
			int hit = -1;
			foreach (int i, vector cp : cPos)
			{
				if (vector.DistanceXZ(cp, p) < 60.0)
				{
					hit = i;
					break;
				}
			}
			float unc = Math.Max(t.m_fBelievedUncertainty, t.m_fUncertainty);
			if (hit < 0)
			{
				cPos.Insert(p);
				cEst.Insert(t.m_iEstimatedEnemyCount);
				cLast.Insert(t.m_fLastUpdateTime);
				cRad.Insert(Math.Max(unc, 20.0));
				cConf.Insert(t.m_fReportQuality);
				cN.Insert(1);
				int rep = 0;
				if (t.m_iReporterPid > 0)
					rep = 1;
				cRep.Insert(rep);
				continue;
			}
			cEst[hit] = cEst[hit] + t.m_iEstimatedEnemyCount;
			cLast[hit] = Math.Max(cLast[hit], t.m_fLastUpdateTime);
			cRad[hit] = Math.Max(cRad[hit], Math.Max(unc, vector.DistanceXZ(cPos[hit], p) + 20.0));
			cConf[hit] = cConf[hit] + t.m_fReportQuality;
			cN[hit] = cN[hit] + 1;
			if (t.m_iReporterPid > 0)
				cRep[hit] = 1;
		}
		foreach (int j, vector q : cPos)
		{
			f.Insert(DCO_ECVRec.CLU);
			f.Insert(ci);
			f.Insert(q[0]);
			f.Insert(q[2]);
			f.Insert(cRad[j]);
			f.Insert(cEst[j]);
			f.Insert(Math.Max(0, now - cLast[j]));
			f.Insert(cRep[j]);
			f.Insert(cConf[j] / Math.Max(1, cN[j]));
			f.Insert(j);
		}
		return cPos.Count();
	}

	protected static void Truth(array<float> f, array<AICommander_BaseComponent> cmds)
	{
		array<AIAgent> agents = {};
		GetGame().GetAIWorld().GetAIAgents(agents);
		int n = 0;
		foreach (AIAgent a : agents)
		{
			if (n >= 600)
				break;
			IEntity e = a.GetControlledEntity();
			if (!e || !ChimeraCharacter.Cast(e) || !DCO_Util.IsNotDead(e))
				continue;
			string fk = DCO_Util.FactionKeyOf(e, true);
			int fi = -1;
			foreach (int k, AICommander_BaseComponent c : cmds)
			{
				if (c.GetCommanderFactionKey() == fk)
					fi = k;
			}
			vector p = e.GetOrigin();
			f.Insert(DCO_ECVRec.TRU);
			f.Insert(p[0]);
			f.Insert(p[2]);
			f.Insert(fi);
			n++;
		}
		array<int> players = {};
		GetGame().GetPlayerManager().GetPlayers(players);
		foreach (int pid : players)
		{
			IEntity pe = GetGame().GetPlayerManager().GetPlayerControlledEntity(pid);
			if (!pe || !DCO_Util.IsNotDead(pe))
				continue;
			string pfk = DCO_Util.FactionKeyOf(pe, true);
			int pfi = -1;
			foreach (int k2, AICommander_BaseComponent c2 : cmds)
			{
				if (c2.GetCommanderFactionKey() == pfk)
					pfi = k2;
			}
			vector pp = pe.GetOrigin();
			f.Insert(DCO_ECVRec.TRU);
			f.Insert(pp[0]);
			f.Insert(pp[2]);
			f.Insert(pfi);
		}
	}

	static void Detail(int playerId, int type, string id)
	{
		if (!Replication.IsServer() || !Allowed(playerId))
			return;
		array<string> rows = {};
		int c = id.IndexOf(":");
		if (c <= 0)
			return;
		int ci = id.Substring(0, c).ToInt();
		string rest = id.Substring(c + 1, id.Length() - c - 1);
		array<AICommander_BaseComponent> cmds = Commanders();
		if (ci < 0 || ci >= cmds.Count())
			return;
		AICommander_BaseComponent cmd = cmds[ci];
		if (type == 1)
			DetailObj(cmd, ObjByKey(rest.ToInt()), rows);
		else if (type == 2)
			DetailGrp(cmd, rest, rows);
		else if (type == 3)
			DetailClu(cmd, ci, rest.ToInt(), rows);
		else if (type == 4)
			DetailCmd(cmd, ci, rows);
		DeliverDetail(playerId, type, id, rows);
		if (DCO_Log.On(DCO_ELogChannel.SYS, DCO_ELogLevel.DEBUG))
		{
			int bytes = 0;
			foreach (string r : rows)
				bytes += r.Length() + 2;
			DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.DEBUG, "-", string.Format("cmdview_detail player=%1 type=%2 id=%3 rows=%4 bytes=%5", playerId, type, id, rows.Count(), bytes));
		}
	}

	protected static void Row(array<string> rows, string style, string a, string b = "")
	{
		rows.Insert(style + "\t" + a + "\t" + b);
	}

	protected static void CommanderRows(AICommander_BaseComponent cmd, array<string> rows)
	{
		if (!cmd.DCO_DebugDraw())
			return;
		string thinking;
		string focus;
		string waiting;
		DCO_CmdDbg.PanelRows(cmd, thinking, focus, waiting);
		Row(rows, "H", "Commander");
		Row(rows, "V", "Thinking now", thinking);
		Row(rows, "V", "Focus", focus);
		Row(rows, "V", "Waiting on", waiting);
		array<DCO_CmdViewDecision> decs = {};
		DCO_CmdViewStore.Decisions(cmd.GetCommanderUID(), "", 4, decs);
		foreach (DCO_CmdViewDecision dd : decs)
			Row(rows, "D", "T+" + DCO_CmdViewStore.Clock(dd.m_fAt), dd.m_sObj + " " + dd.m_sText);
	}

	protected static void DetailObj(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, array<string> rows)
	{
		if (!obj)
			return;
		string uid = cmd.GetCommanderUID();
		DCO_CmdViewObjInfo info = DCO_CmdViewStore.PeekObj(uid, obj.GetLogName());
		string dec = DecisionOf(cmd, obj, info);
		DCO_TacticPlan p = null;
		if (cmd.GetTactics())
			p = cmd.GetTactics().GetPlan(obj);
		string tactic = "-";
		if (p)
			tactic = DCO_CommanderTactics.Name(p.m_eTactic);
		else if (info && dec == "DEFEND")
			tactic = info.m_sTactic;
		string band = "-";
		if (info)
			band = info.m_sBand;
		Row(rows, "K", "Objective · " + cmd.GetCommanderFactionKey() + " · " + uid);
		Row(rows, "T", obj.GetDisplayName());
		Row(rows, "C", dec, band);
		Row(rows, "W", WhySentence(info, dec, tactic));
		CommanderRows(cmd, rows);
		if (cmd.DCO_GetOpPlanner() && cmd.DCO_GetOpPlanner().Contains(obj))
			cmd.DCO_GetOpPlanner().PanelRows(rows);
		Row(rows, "H", "Force picture");
		if (info)
		{
			Row(rows, "V", "Ratio", info.m_sRatio + " -> " + info.m_sEff + " " + info.m_sBand);
			Row(rows, "V", "Confidence", info.m_sConf);
			Row(rows, "V", "Enemy estimate", "~" + info.m_sEnemy);
		}
		else
			Row(rows, "V", "Ratio", "no assessment yet");
		Row(rows, "H", "Plan");
		if (info)
		{
			string ph = info.m_sPhase;
			if (!info.m_sPhaseWhy.IsEmpty())
				ph += " (" + info.m_sPhaseWhy + ")";
			Row(rows, "V", "Doctrine", ph);
		}
		Row(rows, "V", "Tactic", tactic);
		if (info && info.m_sTrPhase != "-")
			Row(rows, "V", "Phase", info.m_sTrPhase);
		if (info && !info.m_sSync.IsEmpty())
			Row(rows, "V", "Sync", info.m_sSync);
		Row(rows, "H", "Groups");
		int ng = 0;
		foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
		{
			if (!g || g.GetGroupObjective() != obj || g.GetUnitCount() <= 0)
				continue;
			int role = RoleOf(cmd, g);
			string trp = "";
			if (g.m_iDCOTRPhase >= 0)
				trp = " [" + typename.EnumToString(DCO_ETRPhase, g.m_iDCOTRPhase) + "]";
			Row(rows, "G", DCO_Log.IdGroup(AIGroup.Cast(g.GetOwner())), RoleLetter(role) + "  " + RoleName(role) + " · " + g.GetOwner().GetName() + trp + "\n" + TaskSentence(g, role));
			ng++;
		}
		if (ng == 0)
			Row(rows, "N", "No groups assigned.");
		Row(rows, "H", "Decisions");
		array<DCO_CmdViewDecision> decs = {};
		DCO_CmdViewStore.Decisions(uid, obj.GetLogName(), 8, decs);
		foreach (DCO_CmdViewDecision d : decs)
			Row(rows, "D", "T+" + DCO_CmdViewStore.Clock(d.m_fAt), d.m_sText);
		if (decs.IsEmpty())
			Row(rows, "N", "No decisions recorded yet.");
	}

	protected static void DetailGrp(AICommander_BaseComponent cmd, string gid, array<string> rows)
	{
		DCO_GroupUtilityComponent grp = null;
		foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
		{
			if (g && DCO_Log.IdGroup(AIGroup.Cast(g.GetOwner())) == gid)
				grp = g;
		}
		if (!grp)
			return;
		int role = RoleOf(cmd, grp);
		DCO_CmdViewGrpInfo gi = DCO_CmdViewStore.PeekGrp(gid);
		float now = DCO_Util.NowS();
		Row(rows, "K", "Group · " + cmd.GetCommanderUID());
		Row(rows, "T", grp.GetOwner().GetName());
		Row(rows, "S", RoleLetter(role) + "  " + RoleName(role));
		string why = "No order recorded yet.";
		if (gi && !gi.m_sWhy.IsEmpty())
			why = "why: " + gi.m_sWhy + " · order " + gi.m_sOrder;
		Row(rows, "W", why);
		Row(rows, "H", "Order");
		Row(rows, "V", "Task", TaskSentence(grp, role));
		Row(rows, "V", "Label", grp.GetTaskLabel());
		if (gi && gi.m_fAt > 0)
			Row(rows, "V", "Order age", DCO_CmdViewStore.Clock(now - gi.m_fAt));
		vector dest = DestOf(grp);
		vector gp = grp.GetOwner().GetOrigin();
		SCR_AIGroup ag = SCR_AIGroup.Cast(grp.GetOwner());
		if (ag && ag.GetLeaderEntity())
			gp = ag.GetLeaderEntity().GetOrigin();
		if (dest != vector.Zero && vector.DistanceXZ(dest, gp) > 15.0)
		{
			int dm = Math.Round(vector.DistanceXZ(dest, gp));
			Row(rows, "V", "Destination", dm.ToString() + " m away");
		}
		else
			Row(rows, "V", "Destination", "holding");
		Row(rows, "H", "State");
		string st = "in position";
		if (grp.IsInContact())
			st = "in contact";
		else if (grp.IsMoving())
			st = "moving";
		Row(rows, "V", "Now", st);
		Row(rows, "V", "Men", grp.GetUnitCount().ToString());
		Row(rows, "V", "Morale", grp.GetGroupMorale().ToString(-1, 2));
		if (grp.m_iDCOTRPhase >= 0)
			Row(rows, "V", "TR phase", typename.EnumToString(DCO_ETRPhase, grp.m_iDCOTRPhase));
		SCR_AIGroupUtilityComponent gu = grp.GetGroupUtilityComponent();
		if (gu && gu.GetCurrentAction())
			Row(rows, "V", "Activity", gu.GetCurrentAction().ClassName());
	}

	protected static void DetailClu(AICommander_BaseComponent cmd, int ci, int idx, array<string> rows)
	{
		array<float> f = {};
		if (!cmd.GetThreatResponseComponent())
			return;
		Clusters(f, ci, cmd, DCO_Util.NowS());
		for (int i = 0; i + 9 < f.Count(); i += 10)
		{
			int id = Math.Round(f[i + 9]);
			if (id != idx)
				continue;
			int est = Math.Round(f[i + 5]);
			int age = Math.Round(f[i + 6]);
			int rad = Math.Round(f[i + 4]);
			string src = "seen by own units";
			if (f[i + 7] > 0.5)
				src = "player report";
			Row(rows, "K", "Enemy picture · " + cmd.GetCommanderUID());
			Row(rows, "T", "~" + est.ToString() + " enemy");
			Row(rows, "W", "What " + cmd.GetCommanderUID() + " believes, not the truth. Turn on Truth (dev) to compare.");
			Row(rows, "H", "Knowledge");
			Row(rows, "V", "Info age", DCO_CmdViewStore.Clock(age));
			Row(rows, "V", "Source", src);
			Row(rows, "V", "Confidence", f[i + 8].ToString(-1, 2));
			Row(rows, "V", "Uncertainty", "±" + rad.ToString() + " m");
			return;
		}
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GlobalIntentOverlayAttribute : DCO_GlobalBoolAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!DCO_TestCaseComponent.RunOn())
			return null;
		return super.ReadVariable(item, manager);
	}

	override protected bool Get(DCO_GlobalAIComponent cfg) { return cfg.GetIntentOverlay(); }
	override protected void Set(DCO_GlobalAIComponent cfg, bool value) { cfg.SetIntentOverlay(value); }
}
