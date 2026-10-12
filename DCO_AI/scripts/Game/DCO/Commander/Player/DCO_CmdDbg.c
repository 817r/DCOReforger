enum DCO_ECDLayer
{
	OBJECTIVES = 1,
	FOCUS = 2,
	PLAN = 4,
	APCAND = 8,
	THREATS = 16,
	GROUPS = 32,
	CARD = 64,
	ASSIGN = 128
}

enum DCO_ECDShape
{
	RING = 1,
	LINE = 2,
	DOT = 3,
	CROSS = 4,
	TRI = 5,
	LABEL = 6,
	LETTER = 7
}

enum DCO_ECDCol
{
	RED,
	ORANGE,
	BLUE,
	GREY,
	GREEN,
	YELLOW,
	CYAN,
	WHITE,
	PURPLE,
	DARKRED
}

class DCO_CDShape
{
	int m_iLayer;
	int m_iType;
	float m_fX0;
	float m_fZ0;
	float m_fX1;
	float m_fZ1;
	float m_fR;
	int m_iCol;
	int m_iFlags;
	string m_sLabel;
}

class DCO_CDCmd
{
	int m_iCi;
	int m_iLayers;
	float m_fAt;
	ref array<ref DCO_CDShape> m_aShapes = {};
	ref array<string> m_aCard = {};
}

class DCO_CmdDbg
{
	protected static ref map<int, string> s_mSig;
	protected static ref map<string, float> s_mEmptyAt;
	protected static ref map<int, ref DCO_CDCmd> s_mClient;
	protected static float s_fRecvAt;
	protected static int s_iAssignOnlyLog;
	protected static int s_iIdleSkipLogged;

	static int Color(int c)
	{
		switch (c)
		{
			case DCO_ECDCol.RED: return 0xFFE0352B;
			case DCO_ECDCol.ORANGE: return 0xFFF08A24;
			case DCO_ECDCol.BLUE: return 0xFF3D7FE0;
			case DCO_ECDCol.GREY: return 0xFF9AA0A6;
			case DCO_ECDCol.GREEN: return 0xFF38C25A;
			case DCO_ECDCol.YELLOW: return 0xFFF2D024;
			case DCO_ECDCol.CYAN: return 0xFF2CC7D8;
			case DCO_ECDCol.WHITE: return 0xFFF4F4F4;
			case DCO_ECDCol.PURPLE: return 0xFFB45CF0;
			case DCO_ECDCol.DARKRED: return 0xFFA0141A;
		}
		return 0xFFFFFFFF;
	}

	static void AddShape(array<float> f, array<string> s, int mask, int layer, int type, float x0, float z0, float x1, float z1, float r, int col, int flags, string label, out int count)
	{
		if ((mask & layer) == 0 || count >= 300)
			return;
		f.Insert(layer);
		f.Insert(type);
		f.Insert(x0);
		f.Insert(z0);
		f.Insert(x1);
		f.Insert(z1);
		f.Insert(r);
		f.Insert(col);
		f.Insert(flags);
		if (label.IsEmpty())
			f.Insert(-1);
		else
		{
			s.Insert(label);
			f.Insert(s.Count() - 1);
		}
		count++;
	}

	protected static void Empty(AICommander_BaseComponent cmd, string reason)
	{
		if (!s_mEmptyAt)
			s_mEmptyAt = new map<string, float>();
		string key = cmd.GetCommanderUID() + "|" + reason;
		float at;
		float now = DCO_Util.NowS();
		if (s_mEmptyAt.Find(key, at) && now - at < 60.0)
			return;
		s_mEmptyAt.Set(key, now);
		DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", string.Format("cmddbg_empty cmd=%1 reason=%2", cmd.GetCommanderUID(), reason));
	}

	static void SendTo(int playerId)
	{
		if (!Replication.IsServer())
			return;
		array<float> f = {};
		array<string> s = {};
		int shapes = Build(f, s);
		string sig = Sig(f, s);
		if (!s_mSig)
			s_mSig = new map<int, string>();
		string last;
		if (s_mSig.Find(playerId, last) && last == sig)
			return;
		s_mSig.Set(playerId, sig);
		DCO_Perf.Count("cmddbg:send");
		DCO_Perf.CountN("cmddbg:shapes", shapes);
		PlayerController local = GetGame().GetPlayerController();
		if (local && local.GetPlayerId() == playerId)
		{
			Receive(f, s);
			return;
		}
		SCR_PlayerControllerGroupComponent comp = SCR_PlayerControllerGroupComponent.GetPlayerControllerComponent(playerId);
		if (comp)
			comp.DCO_CDSend(f, s);
	}

	protected static string Sig(array<float> f, array<string> s)
	{
		float sum = 0;
		foreach (int i, float v : f)
		{
			int k = i % 97;
			sum += v * (k + 1);
		}
		string joined = "";
		foreach (string t : s)
			joined += t + "|";
		return f.Count().ToString() + ":" + sum.ToString(-1, 1) + ":" + joined;
	}

	protected static string ObjName(CMD_AICommanderObjectiveComponent o)
	{
		if (!o)
			return "-";
		return o.GetLogName();
	}

	protected static int Build(array<float> f, array<string> s)
	{
		f.Insert(1);
		int countAt = f.Count();
		f.Insert(0);
		int nCmd = 0;
		int total = 0;
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
			return 0;
		array<AICommander_BaseComponent> cmds = DCO_CmdViewServer.Commanders();
		int assignOnly = 0;
		int assignOnlyShapes = 0;
		foreach (int ci, AICommander_BaseComponent cmd : cmds)
		{
			if (!cmd.DCO_DebugDraw())
			{
				int start = f.Count();
				f.Insert(ci);
				f.Insert(0);
				int aShapeAt = f.Count();
				f.Insert(0);
				f.Insert(0);
				int aCount = 0;
				CMD_AICommanderObjectiveComponent aPrim = DCO_TacticTest.Primary(cmd);
				if (!aPrim && cmd.GetOps())
					aPrim = cmd.GetOps().GetMainEffort();
				AssignShapes(f, s, DCO_ECDLayer.ASSIGN, cmd, mgr, cmd.GetOwner().GetOrigin(), aPrim, aCount);
				if (aCount == 0)
				{
					f.Resize(start);
					Empty(cmd, "debug_off");
					continue;
				}
				f[aShapeAt] = aCount;
				nCmd++;
				total += aCount;
				assignOnly++;
				assignOnlyShapes += aCount;
				continue;
			}
			nCmd++;
			int mask = cmd.DCO_DebugLayers();
			f.Insert(ci);
			f.Insert(mask);
			int shapeAt = f.Count();
			f.Insert(0);
			int cardAt = f.Count();
			f.Insert(0);
			int count = 0;
			BuildCmd(f, s, cmd, mask, mgr, count);
			f[shapeAt] = count;
			f[cardAt] = 0;
			total += count;
			if (count == 0)
				Empty(cmd, "no_data");
		}
		f[countAt] = nCmd;
		if (assignOnly != s_iAssignOnlyLog)
		{
			s_iAssignOnlyLog = assignOnly;
			DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", string.Format("cmddbg_assign_only cmds=%1 shapes=%2", assignOnly, assignOnlyShapes));
		}
		return total;
	}

	protected static void BuildCmd(array<float> f, array<string> s, AICommander_BaseComponent cmd, int mask, AICommander_ManagerComponent mgr, out int count)
	{
		FactionKey fk = cmd.GetCommanderFactionKey();
		string uid = cmd.GetCommanderUID();
		vector hq = cmd.GetOwner().GetOrigin();
		DCO_DoctrineRuntime rt = cmd.GetDoctrineRuntime();
		DCO_CommanderOps ops = cmd.GetOps();
		DCO_CommanderTactics tac = cmd.GetTactics();
		CMD_AICommanderObjectiveComponent prim = DCO_TacticTest.Primary(cmd);
		if (!prim && ops)
			prim = ops.GetMainEffort();
		float now = DCO_Util.NowS();

		foreach (CMD_AICommanderObjectiveComponent o : mgr.m_aObjective)
		{
			if (!o || !o.GetOwner())
				continue;
			vector op = o.GetOwner().GetOrigin();
			int col = DCO_ECDCol.GREY;
			int flags = 0;
			if (o == prim)
			{
				col = DCO_ECDCol.RED;
				flags = 4;
			}
			else if (o.IsCapturedBy(fk))
				col = DCO_ECDCol.BLUE;
			else if (DCO_TacticTest.Forces(cmd, o))
				col = DCO_ECDCol.ORANGE;
			string owner = o.GetOwnerFaction();
			if (owner.IsEmpty())
				owner = "-";
			string label = o.GetLogName() + " · " + owner;
			if (ops)
			{
				DCO_ObjectiveIntel intel = ops.GetIntel(o);
				label += string.Format(" · def %1 seen %2 · conf %3", intel.m_fDefenders.ToString(-1, 0), intel.m_iRaw, intel.m_fConfidence.ToString(-1, 2));
			}
			if (rt)
				label += " · " + DCO_Doctrine.BandName(rt.GetBand(o));
			AddShape(f, s, mask, DCO_ECDLayer.OBJECTIVES, DCO_ECDShape.RING, op[0], op[2], 0, 0, o.GetRadius(), col, flags, label, count);
		}
		if (prim && DCO_TacticTest.ScopeActive(cmd))
		{
			vector pp = prim.GetOwner().GetOrigin();
			float sr = DCO_TacticTest.ScopeRadius();
			AddShape(f, s, mask, DCO_ECDLayer.OBJECTIVES, DCO_ECDShape.RING, pp[0], pp[2], 0, 0, sr, DCO_ECDCol.ORANGE, 1, string.Format("scope %1 m", Math.Round(sr)), count);
		}

		CMD_AICommanderObjectiveComponent focus = DCO_TacticTest.Pick(cmd);
		string why = DCO_TacticTest.PickWhy();
		if (!focus)
		{
			DCO_CmdViewObjInfo info;
			string fname = DCO_CmdViewStore.LatestObj(uid, info);
			foreach (CMD_AICommanderObjectiveComponent fo : mgr.m_aObjective)
			{
				if (fo && fo.GetLogName() == fname)
					focus = fo;
			}
			why = "ranked";
			if (info && !info.m_sWhy.IsEmpty())
				why = info.m_sWhy;
		}
		if (focus)
		{
			vector fp = focus.GetOwner().GetOrigin();
			AddShape(f, s, mask, DCO_ECDLayer.FOCUS, DCO_ECDShape.RING, fp[0], fp[2], 0, 0, focus.GetRadius() + 15.0, DCO_ECDCol.PURPLE, 2, "FOCUS " + focus.GetLogName() + " why=" + why, count);
			AddShape(f, s, mask, DCO_ECDLayer.FOCUS, DCO_ECDShape.LINE, hq[0], hq[2], fp[0], fp[2], 0, DCO_ECDCol.PURPLE, 1, "", count);
		}

		AssignShapes(f, s, mask | DCO_ECDLayer.ASSIGN, cmd, mgr, hq, prim, count);

		if (cmd.DCO_GetOpPlanner())
			cmd.DCO_GetOpPlanner().DbgShapes(cmd, f, s, mask, count);

		if (ops)
		{
			array<vector> armor = {};
			ops.DbgArmor(armor);
			foreach (vector av : armor)
				AddShape(f, s, mask, DCO_ECDLayer.PLAN, DCO_ECDShape.TRI, av[0], av[2], 0, 0, 0, DCO_ECDCol.BLUE, 0, "VEH OW", count);
		}

		CMD_ThreatResponseComponent tr = cmd.GetThreatResponseComponent();
		if (tr)
		{
			foreach (CMD_ThreatEntry t : tr.GetThreats())
			{
				if (!t)
					continue;
				float age = Math.Round((now - t.m_fLastUpdateTime) / 10.0) * 10.0;
				if (age > 300.0)
					continue;
				AddShape(f, s, mask, DCO_ECDLayer.THREATS, DCO_ECDShape.DOT, t.m_vBelievedPos[0], t.m_vBelievedPos[2], age, 0, t.m_iEstimatedEnemyCount, DCO_ECDCol.RED, 0, t.m_iEstimatedEnemyCount.ToString(), count);
			}
		}

		foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
		{
			if (!g || !g.GetOwner() || g.IsPlayerGroup() || g.GetUnitCount() <= 0)
				continue;
			vector gp = g.GetOwner().GetOrigin();
			vector dest = vector.Zero;
			AIGroup ag = AIGroup.Cast(g.GetOwner());
			if (ag && ag.GetCurrentWaypoint())
				dest = ag.GetCurrentWaypoint().GetOrigin();
			string role = "reserve";
			int gcol = DCO_ECDCol.GREY;
			if (tac)
				role = DCO_TacticsCompat.DbgRole(tac, g);
			if (role == "fix")
				gcol = DCO_ECDCol.YELLOW;
			else if (role == "assault")
				gcol = DCO_ECDCol.RED;
			else if (g.IsDedicatedTransport())
			{
				role = "transport";
				gcol = DCO_ECDCol.WHITE;
			}
			else if (g.GetTask() == DCO_EGroupTask.RECON)
			{
				role = "recon";
				gcol = DCO_ECDCol.CYAN;
			}
			string phase = typename.EnumToString(DCO_EGroupTask, g.GetTask());
			if (g.m_iDCOTRPhase >= 0)
				phase = typename.EnumToString(DCO_ETRPhase, g.m_iDCOTRPhase);
			string glabel = g.GetOwner().GetName() + " " + role + " " + phase;
			if (dest == vector.Zero)
				AddShape(f, s, mask, DCO_ECDLayer.GROUPS, DCO_ECDShape.LABEL, gp[0], gp[2], 0, 0, 0, gcol, 0, glabel, count);
			else
				AddShape(f, s, mask, DCO_ECDLayer.GROUPS, DCO_ECDShape.LINE, gp[0], gp[2], dest[0], dest[2], 0, gcol, 8, glabel, count);
		}
	}

	static int AssignShapes(array<float> f, array<string> s, int mask, AICommander_BaseComponent cmd, AICommander_ManagerComponent mgr, vector hq, CMD_AICommanderObjectiveComponent planned, out int count)
	{
		string uid = cmd.GetCommanderUID();
		CMD_AICommanderObjectiveComponent focus = cmd.DCO_GetFocus();
		int lines;
		bool idleOk = mgr.m_aObjective.Count() <= 12;
		int objCount = mgr.m_aObjective.Count();
		if (idleOk)
			s_iIdleSkipLogged = 0;
		else if (s_iIdleSkipLogged != objCount)
		{
			s_iIdleSkipLogged = objCount;
			DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", string.Format("obj_assign_overlay idle_lines=0 objectives=%1 -- more than 12 objectives, allowed-but-idle lines not sent", mgr.m_aObjective.Count()));
		}
		foreach (int key, CMD_AICommanderObjectiveComponent o : mgr.m_aObjective)
		{
			if (!o || !o.GetOwner())
				continue;
			vector op = o.GetOwner().GetOrigin();
			vector dir = op - hq;
			dir[1] = 0;
			float len = dir.Length();
			vector edge = op;
			if (len > o.GetRadius() + 1.0)
				edge = op - dir * (o.GetRadius() / len);
			bool blocked = !o.DCO_CanProcess(uid) || o.IsCommanderBlackListed(uid);
			if (o == focus)
			{
				string fl = "Focus";
				if (cmd.DCO_FocusForced())
					fl = "Focus · forced";
				AddShape(f, s, mask, DCO_ECDLayer.ASSIGN, DCO_ECDShape.LINE, hq[0], hq[2], edge[0], edge[2], key, DCO_ECDCol.PURPLE, 4, fl, count);
				AddShape(f, s, mask, DCO_ECDLayer.ASSIGN, DCO_ECDShape.LETTER, 0.5 * (hq[0] + edge[0]), 0.5 * (hq[2] + edge[2]), 0, 0, key, DCO_ECDCol.PURPLE, 0, fl, count);
				lines++;
			}
			else if (blocked)
			{
				AddShape(f, s, mask, DCO_ECDLayer.ASSIGN, DCO_ECDShape.LINE, hq[0], hq[2], edge[0], edge[2], key, DCO_ECDCol.RED, 1, "Blocked", count);
				AddShape(f, s, mask, DCO_ECDLayer.ASSIGN, DCO_ECDShape.CROSS, edge[0], edge[2], 0, 0, key, DCO_ECDCol.RED, 0, "", count);
				AddShape(f, s, mask, DCO_ECDLayer.ASSIGN, DCO_ECDShape.LETTER, 0.5 * (hq[0] + edge[0]), 0.5 * (hq[2] + edge[2]), 0, 0, key, DCO_ECDCol.RED, 0, "Blocked", count);
				lines++;
			}
			else if (o == planned)
			{
				AddShape(f, s, mask, DCO_ECDLayer.ASSIGN, DCO_ECDShape.LINE, hq[0], hq[2], edge[0], edge[2], key, DCO_ECDCol.WHITE, 0, "Planned", count);
				lines++;
			}
			else if (idleOk)
				AddShape(f, s, mask, DCO_ECDLayer.ASSIGN, DCO_ECDShape.LINE, hq[0], hq[2], edge[0], edge[2], key, DCO_ECDCol.GREY, 1, "", count);
		}
		return lines;
	}

	static int AssignLineCount(AICommander_BaseComponent cmd)
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!cmd || !mgr)
			return 0;
		array<float> f = {};
		array<string> s = {};
		int count;
		CMD_AICommanderObjectiveComponent prim = DCO_TacticTest.Primary(cmd);
		if (!prim && cmd.GetOps())
			prim = cmd.GetOps().GetMainEffort();
		return AssignShapes(f, s, DCO_ECDLayer.ASSIGN, cmd, mgr, cmd.GetOwner().GetOrigin(), prim, count);
	}

	static void PanelRows(AICommander_BaseComponent cmd, out string thinking, out string focus, out string waiting)
	{
		array<string> card = {};
		Card(cmd, card);
		thinking = Strip(card, 1, "THINKING NOW: ");
		focus = Strip(card, 2, "FOCUS: ");
		waiting = Strip(card, 4, "WAITING ON: ");
	}

	protected static string Strip(array<string> card, int i, string prefix)
	{
		if (i >= card.Count())
			return "-";
		string t = card[i];
		if (t.StartsWith(prefix))
			t = t.Substring(prefix.Length(), t.Length() - prefix.Length());
		return t;
	}

	static void Card(AICommander_BaseComponent cmd, array<string> card)
	{
		string mode = typename.EnumToString(CMD_ECommanderMode, cmd.GetCommanderMode());
		string doc = "-";
		DCO_Doctrine d = cmd.GetDoctrine();
		if (d && !d.m_sPreset.IsEmpty())
			doc = d.m_sPreset;
		string pers = cmd.GetPersonalityPreset();
		if (pers.IsEmpty())
			pers = "-";
		card.Insert(string.Format("%1 · %2 · %3 · doctrine %4 · %5 · think %6 s", cmd.GetCommanderUID(), cmd.GetCommanderFactionKey(), pers, doc, mode, Math.Round(cmd.DCO_ThinkInterval())));
		string uid = cmd.GetCommanderUID();
		DCO_CmdViewObjInfo info;
		string latest = DCO_CmdViewStore.LatestObj(uid, info);
		string thinking = "";
		string waiting = "-";
		string plan = "-";
		if (thinking.IsEmpty() && info)
		{
			if (!info.m_sSync.IsEmpty() && info.m_fSyncAt > 0 && DCO_Util.NowS() - info.m_fSyncAt < 60.0)
				thinking = "sync_wait " + latest + " " + info.m_sSync;
			else
				thinking = latest + " " + info.m_sDecision + " " + info.m_sWhy;
		}
		if (thinking.IsEmpty())
			thinking = "thinking, no decision yet";
		card.Insert("THINKING NOW: " + thinking);
		string focusLine = "-";
		CMD_AICommanderObjectiveComponent pick = DCO_TacticTest.Pick(cmd);
		if (pick)
		{
			DCO_CmdViewObjInfo pi = DCO_CmdViewStore.PeekObj(uid, pick.GetLogName());
			focusLine = pick.GetLogName() + " why=" + DCO_TacticTest.PickWhy();
			if (pi)
				focusLine += " band " + pi.m_sBand + " ratio " + pi.m_sRatio;
		}
		else if (info)
			focusLine = latest + " why=" + info.m_sWhy + " band " + info.m_sBand + " ratio " + info.m_sRatio;
		card.Insert("FOCUS: " + focusLine);
		card.Insert("PLAN: " + plan);
		card.Insert("WAITING ON: " + waiting);
		card.Insert("LAST DECISIONS:");
		array<DCO_CmdViewDecision> decs = {};
		DCO_CmdViewStore.Decisions(uid, "", 6, decs);
		foreach (DCO_CmdViewDecision dd : decs)
		{
			string on = dd.m_sObj;
			if (on.IsEmpty())
				on = "-";
			card.Insert("  T+" + DCO_CmdViewStore.Clock(dd.m_fAt) + " " + on + " " + dd.m_sText);
		}
		while (card.Count() > 14)
			card.Remove(card.Count() - 1);
	}

	static void Receive(array<float> f, array<string> s)
	{
		if (!s_mClient)
			s_mClient = new map<int, ref DCO_CDCmd>();
		s_mClient.Clear();
		s_fRecvAt = DCO_CmdViewUI.Now();
		if (f.Count() < 2)
			return;
		int at = 1;
		int nCmd = Math.Round(f[at]);
		at++;
		for (int c = 0; c < nCmd && at + 3 < f.Count(); c++)
		{
			DCO_CDCmd cd = new DCO_CDCmd();
			cd.m_iCi = Math.Round(f[at]);
			cd.m_iLayers = Math.Round(f[at + 1]);
			int nShape = Math.Round(f[at + 2]);
			int nCard = Math.Round(f[at + 3]);
			at += 4;
			cd.m_fAt = s_fRecvAt;
			for (int i = 0; i < nShape && at + 9 < f.Count(); i++)
			{
				DCO_CDShape sh = new DCO_CDShape();
				sh.m_iLayer = Math.Round(f[at]);
				sh.m_iType = Math.Round(f[at + 1]);
				sh.m_fX0 = f[at + 2];
				sh.m_fZ0 = f[at + 3];
				sh.m_fX1 = f[at + 4];
				sh.m_fZ1 = f[at + 5];
				sh.m_fR = f[at + 6];
				sh.m_iCol = Math.Round(f[at + 7]);
				sh.m_iFlags = Math.Round(f[at + 8]);
				int li = Math.Round(f[at + 9]);
				if (li >= 0 && li < s.Count())
					sh.m_sLabel = s[li];
				cd.m_aShapes.Insert(sh);
				at += 10;
			}
			for (int k = 0; k < nCard && at < f.Count(); k++)
			{
				int si = Math.Round(f[at]);
				if (si >= 0 && si < s.Count())
					cd.m_aCard.Insert(s[si]);
				at++;
			}
			s_mClient.Set(cd.m_iCi, cd);
		}
	}

	static DCO_CDCmd ClientCmd(int ci)
	{
		if (!s_mClient)
			return null;
		return s_mClient.Get(ci);
	}

	static int ClientCount()
	{
		if (!s_mClient)
			return 0;
		return s_mClient.Count();
	}

	static float RecvAt()
	{
		return s_fRecvAt;
	}

}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CommanderDebugAttribute : SCR_BaseEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!DCO_TestCaseComponent.RunOn())
			return null;
		AICommander_BaseComponent cmd = DCO_CommanderBaseAttribute.GetCommander(item);
		if (!cmd)
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(cmd.DCO_DebugDraw());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		AICommander_BaseComponent cmd = DCO_CommanderBaseAttribute.GetCommander(item);
		if (!cmd || !var)
			return;
		cmd.DCO_SetDebugDraw(var.GetBool());
		DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", string.Format("cmddbg_toggle player=%1 cmd=%2 layer=all on=%3", playerID, cmd.GetCommanderUID(), DCO_Util.B01(var.GetBool())));
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CommanderDebugLayersAttribute : SCR_BaseMultiSelectPresetsEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!DCO_TestCaseComponent.RunOn())
			return null;
		AICommander_BaseComponent cmd = DCO_CommanderBaseAttribute.GetCommander(item);
		if (!cmd)
			return null;
		int layers = cmd.DCO_DebugLayers();
		int gui = layers & 63;
		if ((layers & DCO_ECDLayer.ASSIGN) != 0)
			gui = gui | 64;
		return SCR_BaseEditorAttributeVar.CreateInt(gui);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		AICommander_BaseComponent cmd = DCO_CommanderBaseAttribute.GetCommander(item);
		if (!cmd || !var)
			return;
		int before = cmd.DCO_DebugLayers();
		int gui = var.GetInt();
		int after = (gui & 63) | (before & DCO_ECDLayer.CARD);
		if ((gui & 64) != 0)
			after = after | DCO_ECDLayer.ASSIGN;
		cmd.DCO_SetDebugLayers(after);
		if ((after & DCO_ECDLayer.ASSIGN) != 0 && (before & DCO_ECDLayer.ASSIGN) == 0)
			DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", string.Format("obj_assign_overlay on=1 player=%1 cmd=%2 lines=%3", playerID, cmd.GetCommanderUID(), DCO_CmdDbg.AssignLineCount(cmd)));
		else if ((after & DCO_ECDLayer.ASSIGN) == 0 && (before & DCO_ECDLayer.ASSIGN) != 0)
			DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", string.Format("obj_assign_overlay on=0 player=%1 cmd=%2", playerID, cmd.GetCommanderUID()));
		for (int b = 0; b < 8; b++)
		{
			int bit = 1 << b;
			if ((before & bit) == (after & bit))
				continue;
			DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", string.Format("cmddbg_toggle player=%1 cmd=%2 layer=%3 on=%4", playerID, cmd.GetCommanderUID(), typename.EnumToString(DCO_ECDLayer, bit), DCO_Util.B01((after & bit) != 0)));
		}
	}
}
