modded class DCO_SettingsDialogUI
{
	protected bool m_bPB;
	protected int m_iPBFrom;
	protected ref array<string> m_aPBM = {};
	protected string m_sPBName;
	protected bool m_bPBNamed;
	protected string m_sPBTemplate = "rifle squad";
	protected string m_sPBFaction;
	protected bool m_bPBFacFixed;
	protected string m_sPBVeh;
	protected int m_iPBRole;
	protected int m_iPBExp = 1;
	protected int m_iPBMor = 1;
	protected int m_iPBShape;
	protected int m_iPBArr;
	protected int m_iPBDest;
	protected int m_iPBSel = -1;
	protected string m_sPBUndo;
	protected string m_sPBUndoText;
	protected bool m_bPBDirty;
	protected bool m_bPBAskDiscard;
	protected bool m_bPBAskReplace;
	protected int m_iPBPending;
	protected string m_sPBEditing;
	protected string m_sPBEditWhere;
	protected ref array<string> m_aPBVehicles = {};
	protected string m_sPBMsg;
	protected ref array<ref DCO_GroupPreset> m_aPBPick = {};

	protected int m_iGPFac;
	protected int m_iGPWhere;
	protected int m_iGPSel = -1;
	protected ref array<ref DCO_GroupPreset> m_aGP = {};
	protected bool m_bGPRename;
	protected string m_sGPRename;
	protected bool m_bGPAskDelete;
	protected int m_iGPConflict;
	protected string m_sGPConflictName;
	protected string m_sGPMsg;
	protected int m_iGPMsgSeq;
	protected string m_sGPRule = "gm";
	protected int m_iGPLimit = 200;

	protected static int PBDestPref()
	{
		string path = "$profile:DCO/GroupPresetDest.txt";
		if (!FileIO.FileExists(path))
			return 0;
		FileHandle fh = FileIO.OpenFile(path, FileMode.READ);
		if (!fh)
			return 0;
		string line;
		fh.ReadLine(line);
		fh.Close();
		return Math.ClampInt(line.ToInt(), 0, 2);
	}

	protected static void PBSetDestPref(int d)
	{
		FileIO.MakeDirectory("$profile:DCO");
		FileHandle fh = FileIO.OpenFile("$profile:DCO/GroupPresetDest.txt", FileMode.WRITE);
		if (!fh)
			return;
		fh.WriteLine(d.ToString());
		fh.Close();
	}

	override protected void BuildLines()
	{
		if (m_bPB)
		{
			m_aLines.Clear();
			PBBuildLines();
			return;
		}
		super.BuildLines();
	}

	override protected void BuildSpawnerLines()
	{
		if (SpSub("GlobalGroupPresets"))
		{
			PBuildPage();
			return;
		}
		super.BuildSpawnerLines();
	}

	override protected void RenderSpawnerLine(DCO_SetLine line, float y)
	{
		if (line.m_iType >= 20 && line.m_iType <= 24)
		{
			PBRender(line, y);
			return;
		}
		if (line.m_iType >= 25 && line.m_iType <= 28)
		{
			PRenderPage(line, y);
			return;
		}
		super.RenderSpawnerLine(line, y);
	}

	override protected void SpAct(int code, int b)
	{
		if (code >= 100)
		{
			PAct(code, b);
			return;
		}
		super.SpAct(code, b);
	}

	override void OnEditChange(Widget w, bool finished)
	{
		EditBoxWidget box = EditBoxWidget.Cast(w);
		int id = -1;
		if (w)
			id = w.GetUserID();
		if (box && id > 0 && id < m_aActKind.Count() && m_aActKind[id] == DCO_ESetAct.SPAWNER)
		{
			int a = m_aActA[id];
			if (a == 140)
			{
				string pbName = box.GetText();
				if (pbName != PBName())
				{
					m_sPBName = pbName;
					m_bPBNamed = !pbName.Trim().IsEmpty();
					m_bPBDirty = true;
					m_bPBAskReplace = false;
				}
				return;
			}
			if (a == 141)
			{
				m_sGPRename = box.GetText();
				return;
			}
		}
		super.OnEditChange(w, finished);
	}

	override protected void OnBack()
	{
		if (m_bPB && m_iDragRow < 0 && !IsTyping())
		{
			PBBack();
			return;
		}
		super.OnBack();
	}

	protected static string PBExpName(int e)
	{
		array<string> n = {"Recruit", "Regular", "Veteran", "Elite"};
		return n[Math.ClampInt(e, 0, 3)];
	}

	protected static string PBRoleName(int r)
	{
		array<string> n = {"Auto", "Assault", "Fire team", "AT", "Recon", "Motorized"};
		return n[Math.ClampInt(r, 0, 5)];
	}

	protected void PMerge(notnull array<ref DCO_GroupPreset> outList)
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		array<ref DCO_GroupPreset> remote = {};
		int serverCount;
		if (mgr)
			DCO_GroupPresets.ParseList(mgr.DCO_GetGroupPresetList(), remote, m_sGPRule, m_iGPLimit, serverCount);
		bool single = DCO_GroupPresets.IsSinglePc();
		array<ref DCO_GroupPreset> local = {};
		DCO_GroupPresets.LoadAll(local);
		map<string, int> byName = new map<string, int>();
		foreach (DCO_GroupPreset lp : local)
		{
			lp.m_sWhere = "pc";
			bool vehOnly;
			lp.m_sMissing = DCO_GroupPresets.MissingItem(lp, vehOnly);
			lp.m_sPcSaved = lp.m_iSaved.ToString();
			byName.Set(lp.m_sName, outList.Count());
			outList.Insert(lp);
		}
		bool anyBuiltin;
		foreach (DCO_GroupPreset rp : remote)
		{
			if (rp.m_sWhere == "builtin")
			{
				anyBuiltin = true;
				outList.Insert(rp);
				continue;
			}
			if (single)
				continue;
			int idx;
			if (byName.Find(rp.m_sName, idx))
			{
				DCO_GroupPreset both = outList[idx];
				if (both.Content() == rp.Content())
					both.m_sWhere = "both";
				else
				{
					both.m_sWhere = "differ";
					both.m_sMaker = rp.m_sMaker + "|" + rp.m_iSaved.ToString();
				}
				continue;
			}
			outList.Insert(rp);
		}
		if (!anyBuiltin)
		{
			array<ref DCO_GroupPreset> built = {};
			DCO_GroupPresets.Builtin(built);
			foreach (DCO_GroupPreset b : built)
				outList.Insert(b.Copy());
		}
	}

	protected int PUsedIn(string name)
	{
		int n;
		foreach (DCO_CommanderSpawnerComponent cs : DCO_CommanderSpawnerComponent.GetAll())
		{
			if (!cs)
				continue;
			array<string> rows = {};
			string pool = cs.GetRplPool();
			if (!pool.IsEmpty())
				pool.Split(";", rows, true);
			foreach (string r : rows)
			{
				array<string> f = {};
				r.Split("|", f, false);
				if (!f.IsEmpty() && (f[0] == name || f[0].StartsWith(name + " · ")))
				{
					n++;
					break;
				}
			}
		}
		return n;
	}

	protected string PWhereTag(string where)
	{
		if (where == "server")
			return "Server";
		if (where == "pc")
		{
			if (DCO_GroupPresets.IsSinglePc())
				return "My PC";
			return "My PC";
		}
		if (where == "both")
			return "Both";
		if (where == "differ")
			return "Both · differ";
		return "Built-in";
	}

	protected string PSourceTag(string where)
	{
		if (where == "server")
			return "· server";
		if (where == "pc")
			return "· my PC";
		if (where == "both" || where == "differ")
			return "· server + my PC";
		return "· built-in";
	}

	protected bool PFacOk(DCO_GroupPreset p)
	{
		if (m_iGPFac == 0)
			return true;
		array<string> keys = {"", "US", "USSR", "FIA"};
		return p.m_sFaction == keys[m_iGPFac];
	}

	protected bool PWhereOk(DCO_GroupPreset p)
	{
		if (m_iGPWhere == 0)
			return true;
		if (m_iGPWhere == 1)
			return p.m_sWhere == "server" || p.m_sWhere == "both" || p.m_sWhere == "differ";
		if (m_iGPWhere == 2)
			return p.m_sWhere == "pc" || p.m_sWhere == "both" || p.m_sWhere == "differ";
		return p.m_sWhere == "builtin";
	}

	protected void PBuildPage()
	{
		if (DCO_GroupPresets.MsgSeq() != m_iGPMsgSeq)
		{
			m_iGPMsgSeq = DCO_GroupPresets.MsgSeq();
			m_sGPMsg = DCO_GroupPresets.Msg();
			string conflict = DCO_GroupPresets.PeekConflict();
			if (!conflict.IsEmpty() && m_iGPConflict == 0)
			{
				m_iGPConflict = 1;
				DCO_GroupPreset cp = DCO_GroupPreset.Unpack(conflict);
				if (cp)
					m_sGPConflictName = cp.m_sName;
			}
		}
		array<ref DCO_GroupPreset> all = {};
		PMerge(all);
		m_aGP.Clear();
		foreach (DCO_GroupPreset p : all)
		{
			if (PFacOk(p) && PWhereOk(p))
				m_aGP.Insert(p);
		}
		if (m_iGPSel >= m_aGP.Count())
			m_iGPSel = -1;
		SpLine(25, -1, 92, string.Empty);
		SpLine(26, -1, 32, string.Empty);
		foreach (int i, DCO_GroupPreset gp : m_aGP)
			SpLine(27, i, 36, gp.m_sName);
		if (m_aGP.IsEmpty())
			SpLine(26, -2, 32, "No presets match the filters.");
		SpLine(28, -1, 150, string.Empty);
	}

	protected void PRenderPage(DCO_SetLine line, float y)
	{
		float x0 = m_fSheetX + 16;
		float w = m_fSheetW - 32;
		if (line.m_iType == 25)
		{
			int nServer;
			int nPc;
			int nBuilt;
			array<ref DCO_GroupPreset> all = {};
			PMerge(all);
			foreach (DCO_GroupPreset p : all)
			{
				if (p.m_sWhere == "server")
					nServer++;
				else if (p.m_sWhere == "pc")
					nPc++;
				else if (p.m_sWhere == "builtin")
					nBuilt++;
				else
				{
					nServer++;
					nPc++;
				}
			}
			string count = string.Format("%1 on this server · %2 on my PC · %3 built-in", nServer, nPc, nBuilt);
			if (DCO_GroupPresets.IsSinglePc())
				count = string.Format("%1 on my PC · %2 built-in", nPc, nBuilt);
			Cell(m_wDyn, x0, y, 300, 30, "Group presets", m_cClear, m_cOrange, 1, 20, 0);
			Cell(m_wDyn, x0 + 300, y + 2, w - 300, 28, count, m_cClear, m_cDim, 0, 18, 0);
			float fy = y + 34;
			float bx = x0;
			array<string> facs = {"All", "US", "USSR", "FIA"};
			Cell(m_wDyn, bx, fy, 74, 34, "Faction", m_cClear, m_cDim, 0, 18, 0);
			bx += 76;
			foreach (int fi, string fn : facs)
			{
				Color fbg = m_cCell;
				if (fi == m_iGPFac)
					fbg = m_cTileOn;
				bx = SpButton(bx, fy, 70, fn, 150, fi, fbg, m_cText);
			}
			bx += 12;
			Cell(m_wDyn, bx, fy, 66, 34, "Where", m_cClear, m_cDim, 0, 18, 0);
			bx += 68;
			array<string> wh = {"All", "This server", "My PC", "Built-in"};
			if (DCO_GroupPresets.IsSinglePc())
				wh = {"All", "-", "My PC", "Built-in"};
			foreach (int wi, string wn : wh)
			{
				if (wn == "-")
					continue;
				Color wbg = m_cCell;
				if (wi == m_iGPWhere)
					wbg = m_cTileOn;
				bx = SpButton(bx, fy, 112, wn, 151, wi, wbg, m_cText);
			}
			return;
		}
		if (line.m_iType == 26)
		{
			if (line.m_iRow == -2)
			{
				Cell(m_wDyn, x0 + 8, y, w - 16, 30, line.m_sText, m_cClear, m_cDim, 0, 18, 0);
				return;
			}
			PCols(x0, y, "Preset", "Where", "Faction", "Men", "Role", "Vehicle", "Made", "Used in", m_cDim, 0);
			return;
		}
		if (line.m_iType == 27)
		{
			if (!m_aGP.IsIndexValid(line.m_iRow))
				return;
			DCO_GroupPreset p = m_aGP[line.m_iRow];
			Color bg = m_cRowBg;
			if (line.m_iRow == m_iGPSel)
				bg = m_cTileOn;
			int id = Act(DCO_ESetAct.SPAWNER, 152, line.m_iRow, PRowDesc(p));
			Cell(m_wDyn, x0, y, w, line.m_fH - 4, string.Empty, bg, m_cText, 0, 10, id);
			string name = p.m_sName;
			if (!p.m_sMissing.IsEmpty())
				name += "  <color rgba=\"220,70,60,255\">Missing here: " + p.m_sMissing + "</color>";
			string veh = "none";
			if (!p.m_sVehicle.IsEmpty())
				veh = FilePath.StripExtension(FilePath.StripPath(p.m_sVehicle));
			string made = p.m_sMaker;
			if (p.m_sWhere == "differ")
			{
				array<string> mk = {};
				p.m_sMaker.Split("|", mk, false);
				if (!mk.IsEmpty())
					made = mk[0];
			}
			if (p.m_sWhere != "builtin")
				made += " · " + DCO_GroupPresets.AgeText(p.m_iSaved);
			int used = PUsedIn(p.m_sName);
			string usedText = "none";
			if (used == 1)
				usedText = "1 spawner";
			else if (used > 1)
				usedText = string.Format("%1 spawners", used);
			string role = PBRoleName(p.m_iRole);
			if (p.m_iRole == 0)
				role = PBRoleName(DCO_GroupPresets.AutoRole(p.m_sMembers, !p.m_sVehicle.IsEmpty())) + " · auto";
			PCols(x0, y, name, PWhereTag(p.m_sWhere), p.m_sFaction, p.MemberCount().ToString(), role, veh, made, usedText, m_cText, id);
			return;
		}
		if (line.m_iType == 28)
		{
			float bx = x0;
			bool has = m_aGP.IsIndexValid(m_iGPSel);
			DCO_GroupPreset sel;
			if (has)
				sel = m_aGP[m_iGPSel];
			bool builtin = has && sel.m_sWhere == "builtin";
			bx = SpButton(bx, y, 140, "New preset", 153, 0, m_cOrange, m_cDark);
			Color eb = m_cCell;
			if (!has || builtin)
				eb = m_cRowBg;
			bx = SpButton(bx, y, 90, "Edit", 154, 0, eb, m_cText);
			bx = SpButton(bx, y, 120, "Duplicate", 155, 0, m_cCell, m_cText);
			bx = SpButton(bx, y, 110, "Rename", 156, 0, eb, m_cText);
			bx = SpButton(bx, y, 100, "Delete", 157, 0, eb, m_cText);
			if (!DCO_GroupPresets.IsSinglePc())
			{
				bx = SpButton(bx, y, 150, "Copy to server", 158, 0, m_cCell, m_cText);
				bx = SpButton(bx, y, 150, "Copy to my PC", 159, 0, m_cCell, m_cText);
			}
			float ly = y + 40;
			string text = m_sGPMsg;
			if (has && text.IsEmpty())
				text = PRowDesc(sel);
			if (m_bGPRename && has)
			{
				Widget edit = GetGame().GetWorkspace().CreateWidgets(m_sEditLayout, m_wDyn);
				if (edit)
				{
					FrameSlot.SetPos(edit, x0, ly);
					FrameSlot.SetSize(edit, 360, 34);
					EditBoxWidget box = EditBoxWidget.Cast(edit.FindAnyWidget("Box"));
					if (box)
					{
						box.SetText(m_sGPRename);
						box.SetPlaceholderText("New name");
					}
					Tag(edit, Act(DCO_ESetAct.SPAWNER, 141, 0, string.Empty));
				}
				float rx = SpButton(x0 + 370, ly, 120, "Rename", 156, 1, m_cOrange, m_cDark);
				SpButton(rx, ly, 100, "Cancel", 156, 2, m_cCell, m_cText);
				text = "Type the new name. Empty or an existing name is refused.";
				ly += 40;
			}
			else if (m_bGPAskDelete && has)
			{
				int used = PUsedIn(sel.m_sName);
				text = string.Format("Delete \"%1\"?", sel.m_sName);
				if (used == 1)
					text += " The 1 spawner that uses it keeps its copy.";
				else if (used > 1)
					text += string.Format(" The %1 spawners that use it keep their copy.", used);
				float dx = SpButton(x0, ly, 120, "Delete", 157, 1, m_cDanger, m_cWhite);
				SpButton(dx, ly, 110, "Cancel", 157, 2, m_cCell, m_cText);
				ly += 40;
			}
			else if (m_iGPConflict > 0)
			{
				string where = "this server";
				if (m_iGPConflict == 2)
					where = "your PC";
				text = string.Format("\"%1\" already exists on %2 and is different.", m_sGPConflictName, where);
				int code = 158;
				if (m_iGPConflict == 2)
					code = 159;
				float cx = SpButton(x0, ly, 120, "Replace", code, 1, m_cOrange, m_cDark);
				cx = SpButton(cx, ly, 130, "Keep both", code, 2, m_cCell, m_cText);
				SpButton(cx, ly, 110, "Cancel", 160, 0, m_cCell, m_cText);
				ly += 40;
			}
			else if (has && (sel.m_sWhere == "pc" || sel.m_sWhere == "differ" || sel.m_sWhere == "both") && !sel.m_sMissing.IsEmpty() && !DCO_GroupPresets.IsSinglePc())
			{
				bool vehOnly;
				DCO_GroupPresets.MissingItem(sel, vehOnly);
				if (vehOnly)
				{
					SpButton(x0, ly, 230, "Copy without the vehicle", 158, 3, m_cCell, m_cText);
					ly += 40;
				}
			}
			string rule = "GMs";
			if (m_sGPRule == "admin")
				rule = "Admins";
			else if (m_sGPRule == "off")
				rule = "nobody";
			string tail = string.Format("Who can copy presets to this server: %1 (set by server) · at most %2 presets on this server. Used in counts the spawners in this mission.", rule, m_iGPLimit);
			if (DCO_GroupPresets.IsSinglePc())
				tail = "Used in counts the spawners in this mission.";
			Widget tc = Cell(m_wDyn, x0, ly, w, 44, text + "\n" + tail, m_cClear, m_cDim, 0, 18, 0);
			WrapCell(tc);
		}
	}

	protected string PRowDesc(DCO_GroupPreset p)
	{
		if (!p)
			return string.Empty;
		if (DCO_GroupPresets.IsSinglePc() && p.m_sWhere == "pc")
			return "On your PC: $profile:DCO/GroupPresets/" + DCO_GroupPresets.FileName(p.m_sName) + " in your Arma Reforger profile folder.";
		if (p.m_sWhere == "server")
			return "Only on this server. Copy to my PC to take it to other servers.";
		if (p.m_sWhere == "pc")
			return "Only on your PC. Copy to server to let the other GMs here use it.";
		if (p.m_sWhere == "both")
			return "The same on this server and on your PC.";
		if (p.m_sWhere == "differ")
		{
			array<string> mk = {};
			p.m_sMaker.Split("|", mk, false);
			int serverSaved;
			if (mk.Count() > 1)
				serverSaved = mk[1].ToInt();
			string newer = "your PC";
			if (serverSaved > p.m_iSaved)
				newer = "this server";
			return string.Format("On both, but they differ: the copy on %1 is newer (%2 on your PC vs %3 on the server). Copy to server or Copy to my PC to make them the same.", newer, DCO_GroupPresets.AgeText(p.m_iSaved), DCO_GroupPresets.AgeText(serverSaved));
		}
		return "Built-in: ships with the DCO_AI mod (Configs/DCO/GroupPresets), so it is on every server that loads the mod. Read-only; Duplicate it to change it.";
	}

	protected void PCols(float x0, float y, string a, string b, string c, string d, string e, string f, string g, string h, Color fg, int id)
	{
		Cell(m_wDyn, x0 + 8, y + 3, 290, 30, a, m_cClear, fg, 0, 18, id);
		Cell(m_wDyn, x0 + 300, y + 3, 130, 30, b, m_cClear, fg, 0, 16, id);
		Cell(m_wDyn, x0 + 432, y + 3, 70, 30, c, m_cClear, fg, 0, 18, id);
		Cell(m_wDyn, x0 + 504, y + 3, 50, 30, d, m_cClear, fg, 2, 18, id);
		Cell(m_wDyn, x0 + 560, y + 3, 140, 30, e, m_cClear, fg, 0, 16, id);
		Cell(m_wDyn, x0 + 702, y + 3, 100, 30, f, m_cClear, fg, 0, 16, id);
		Cell(m_wDyn, x0 + 804, y + 3, 120, 30, g, m_cClear, fg, 0, 16, id);
		Cell(m_wDyn, x0 + 920, y + 3, 84, 30, h, m_cClear, fg, 0, 16, id);
	}

	override protected void PBuildSpawnerPicker(DCO_CommanderSpawnerComponent s)
	{
		SpLine(13, -1, 40, string.Empty);
		m_aPBPick.Clear();
		array<ref DCO_GroupPreset> all = {};
		PMerge(all);
		foreach (DCO_GroupPreset p : all)
		{
			if (p.m_sFaction != s.GetRplFaction())
				continue;
			m_aPBPick.Insert(p);
			SpLine(12, m_aPBPick.Count() - 1, 38, p.m_sName);
		}
		if (m_aPBPick.IsEmpty())
			SpLine(9, -1, 32, "No presets for this faction yet.");
	}

	override protected void PRenderPickRow(DCO_SetLine line, float y)
	{
		if (!m_aPBPick.IsIndexValid(line.m_iRow))
			return;
		DCO_GroupPreset p = m_aPBPick[line.m_iRow];
		float x0 = m_fSheetX + 32;
		Cell(m_wDyn, x0, y, m_fSheetW - 64, 34, string.Empty, m_cRowBg, m_cText, 0, 10, 0);
		string veh;
		if (!p.m_sVehicle.IsEmpty())
			veh = " + " + FilePath.StripExtension(FilePath.StripPath(p.m_sVehicle));
		string txt = string.Format("%1 · %2 men%3 · %4 %5", p.m_sName, p.MemberCount(), veh, PBExpName(p.m_iExperience), PSourceTag(p.m_sWhere));
		if (!p.m_sMissing.IsEmpty())
			txt += "  <color rgba=\"220,70,60,255\">Missing here: " + p.m_sMissing + "</color>";
		Cell(m_wDyn, x0 + 8, y + 2, 700, 30, txt, m_cClear, m_cText, 0, 18, 0);
		SpButton(x0 + 760, y, 110, "Add", 102, line.m_iRow, m_cOrange, m_cDark);
	}

	protected void PBReset()
	{
		m_aPBM.Clear();
		m_sPBName = string.Empty;
		m_bPBNamed = false;
		m_sPBVeh = string.Empty;
		m_iPBRole = 0;
		m_iPBExp = 1;
		m_iPBMor = 1;
		m_iPBShape = 0;
		m_iPBArr = 0;
		m_iPBSel = -1;
		m_sPBUndo = string.Empty;
		m_sPBUndoText = string.Empty;
		m_bPBDirty = false;
		m_bPBAskDiscard = false;
		m_bPBAskReplace = false;
		m_sPBEditing = string.Empty;
		m_sPBEditWhere = string.Empty;
		m_sPBMsg = string.Empty;
		m_iPBDest = 0;
		if (DCO_GroupPresets.IsSinglePc())
			m_iPBDest = 1;
		else
			m_iPBDest = PBDestPref();
	}

	protected void PBLoadVehicles()
	{
		m_aPBVehicles.Clear();
		FactionManager fm = GetGame().GetFactionManager();
		if (!fm)
			return;
		SCR_Faction fac = SCR_Faction.Cast(fm.GetFactionByKey(m_sPBFaction));
		if (!fac)
			return;
		SCR_EntityCatalog vc = fac.GetFactionEntityCatalogOfType(EEntityCatalogType.VEHICLE, false);
		if (!vc)
			return;
		array<SCR_EntityCatalogEntry> vl = {};
		vc.GetEntityList(vl);
		foreach (SCR_EntityCatalogEntry ve : vl)
		{
			if (ve && ve.IsEnabled() && m_aPBVehicles.Count() < 16)
				m_aPBVehicles.Insert(ve.GetPrefab());
		}
	}

	protected void PBOpen(int from, DCO_GroupPreset src, string faction)
	{
		PBReset();
		m_bPB = true;
		m_iPBFrom = from;
		m_sPBFaction = faction;
		m_bPBFacFixed = from == 0;
		if (m_sPBFaction.IsEmpty())
			m_sPBFaction = "US";
		if (src)
		{
			src.Members(m_aPBM);
			m_sPBName = src.m_sName;
			m_bPBNamed = true;
			m_sPBFaction = src.m_sFaction;
			m_sPBVeh = src.m_sVehicle;
			m_iPBRole = src.m_iRole;
			m_iPBExp = src.m_iExperience;
			m_iPBMor = src.m_iMorale;
			m_iPBShape = src.m_iShape;
			m_iPBArr = src.m_iArrival;
		}
		else
			PBTemplate(0, false);
		PBLoadVehicles();
		string fromName = "global";
		if (from == 0)
			fromName = "spawner";
		else if (from == 2)
			fromName = "world";
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("group_preset_open from=%1 gm=%2", fromName, DCO_GroupPresets.LocalPlayerId()));
		m_iScroll = 0;
		RequestRender();
	}

	protected string PBSnap()
	{
		string m;
		foreach (int i, string t : m_aPBM)
		{
			if (i > 0)
				m += ",";
			m += t;
		}
		return m_sPBFaction + "~" + m + "~" + m_sPBVeh;
	}

	protected void PBRestore(string snap)
	{
		array<string> f = {};
		snap.Split("~", f, false);
		if (f.Count() < 3)
			return;
		m_sPBFaction = f[0];
		m_aPBM.Clear();
		if (!f[1].IsEmpty())
			f[1].Split(",", m_aPBM, true);
		m_sPBVeh = f[2];
		PBLoadVehicles();
	}

	protected void PBTemplate(int which, bool undoable)
	{
		array<string> names = {"rifle squad", "fire team", "AT team", "recon team", "empty"};
		if (undoable && !m_aPBM.IsEmpty())
		{
			m_sPBUndo = PBSnap();
			string label = "an empty group";
			if (which < 4)
				label = names[which];
			m_sPBUndoText = string.Format("Members replaced with %1.", label);
		}
		m_aPBM.Clear();
		if (which == 0)
			m_aPBM.Copy({"SL", "AR", "GL", "RIF", "MED", "TL", "AR", "GL", "RIF"});
		else if (which == 1)
			m_aPBM.Copy({"TL", "AR", "GL", "RIF"});
		else if (which == 2)
			m_aPBM.Copy({"TL", "AT", "AT", "RIF"});
		else if (which == 3)
			m_aPBM.Copy({"TL", "MRK", "RIF", "RTO"});
		if (which < 4)
			m_sPBTemplate = names[which];
		m_iPBSel = -1;
	}

	protected string PBMembers()
	{
		string m;
		foreach (int i, string t : m_aPBM)
		{
			if (i > 0)
				m += ",";
			m += t;
		}
		return m;
	}

	protected string PBName()
	{
		if (m_bPBNamed)
			return DCO_GroupPresets.CleanName(m_sPBName);
		string n = PBExpName(m_iPBExp) + " " + m_sPBTemplate;
		return n;
	}

	protected DCO_GroupPreset PBPreset()
	{
		DCO_GroupPreset p = new DCO_GroupPreset();
		p.m_sName = PBName();
		p.m_sFaction = m_sPBFaction;
		p.m_sMembers = PBMembers();
		p.m_sVehicle = m_sPBVeh;
		p.m_iRole = m_iPBRole;
		p.m_iExperience = m_iPBExp;
		p.m_iMorale = m_iPBMor;
		p.m_iShape = m_iPBShape;
		p.m_iArrival = m_iPBArr;
		p.m_sMaker = GetGame().GetPlayerManager().GetPlayerName(DCO_GroupPresets.LocalPlayerId());
		return p;
	}

	protected int PBSeats()
	{
		if (m_sPBVeh.IsEmpty())
			return -1;
		int pilot;
		int turret;
		int cargo;
		DCO_GroupPresets.SeatsOf(m_sPBVeh, pilot, turret, cargo);
		return pilot + turret + cargo;
	}

	protected string PBVehClass(ResourceName veh)
	{
		int c = DCO_CommanderSpawnerComponent.VehicleCost(veh);
		if (c >= 16)
			return "heavy armor";
		if (c >= 8)
			return "light armor";
		if (c >= 4)
			return "armed car";
		return "unarmed";
	}

	protected int PBCount(string t)
	{
		int n;
		foreach (string m : m_aPBM)
		{
			if (m == t)
				n++;
		}
		return n;
	}

	protected bool PBNameTaken()
	{
		string n = PBName();
		if (n.IsEmpty() || n == m_sPBEditing)
			return false;
		array<ref DCO_GroupPreset> all = {};
		PMerge(all);
		foreach (DCO_GroupPreset p : all)
		{
			if (!DCO_GroupPresets.SameKey(p.m_sName, n) || p.m_sWhere == "builtin")
				continue;
			if (m_iPBDest == 0 && p.m_sWhere != "pc")
				return true;
			if (m_iPBDest == 1 && p.m_sWhere != "server")
				return true;
			if (m_iPBDest == 2)
				return true;
		}
		return false;
	}

	protected void PBBuildLines()
	{
		DCO_SetLine head = new DCO_SetLine();
		head.m_iType = 0;
		head.m_fH = 40;
		if (m_iPBFrom == 0)
			head.m_sText = "Spawner › Force pool › " + PBTitle();
		else
			head.m_sText = "DCO Settings › Global › Group presets › " + PBTitle();
		m_aLines.Insert(head);
		SpLine(20, -1, 44, string.Empty);
		SpLine(21, -1, 44, string.Empty);
		SpLine(22, -1, 404, string.Empty);
		SpLine(23, -1, 70, string.Empty);
		SpLine(24, -1, 92, string.Empty);
	}

	protected string PBTitle()
	{
		if (!m_sPBEditing.IsEmpty())
			return "Edit group preset";
		return "New group preset";
	}

	protected void PBRender(DCO_SetLine line, float y)
	{
		float x0 = m_fSheetX + 16;
		float w = m_fSheetW - 32;
		if (line.m_iType == 20)
		{
			float bx = SpButton(x0, y, 100, "← Back", 101, 0, m_cCell, m_cText);
			Cell(m_wDyn, bx, y, 60, 34, "Name", m_cClear, m_cDim, 0, 18, 0);
			Widget edit = GetGame().GetWorkspace().CreateWidgets(m_sEditLayout, m_wDyn);
			if (edit)
			{
				FrameSlot.SetPos(edit, bx + 62, y);
				FrameSlot.SetSize(edit, 330, 34);
				ImageWidget eb = ImageWidget.Cast(edit.FindAnyWidget("Bg"));
				if (eb)
					eb.SetColor(m_cCell);
				EditBoxWidget box = EditBoxWidget.Cast(edit.FindAnyWidget("Box"));
				if (box)
				{
					box.SetText(PBName());
					box.SetPlaceholderText("Preset name");
					box.SetVirtualKeyboardTitle("Preset name");
				}
				Tag(edit, Act(DCO_ESetAct.SPAWNER, 140, 0, "Suggested from the experience and the starting group until you type your own name."));
			}
			float fx = bx + 410;
			if (m_bPBFacFixed)
			{
				string feeds;
				DCO_CommanderSpawnerComponent s = SpEdited();
				if (s)
					feeds = s.StatusField(1);
				Cell(m_wDyn, fx, y, w - (fx - x0), 34, string.Format("Faction %1 from %2", m_sPBFaction, feeds), m_cClear, m_cDim, 0, 18, 0);
			}
			else
			{
				Cell(m_wDyn, fx, y, 80, 34, "Faction", m_cClear, m_cDim, 0, 18, 0);
				fx += 82;
				array<string> facs = {"US", "USSR", "FIA"};
				foreach (int fi, string fn : facs)
				{
					Color fbg = m_cCell;
					if (fn == m_sPBFaction)
						fbg = m_cTileOn;
					fx = SpButton(fx, y, 80, fn, 126, fi, fbg, m_cText);
				}
			}
			return;
		}
		if (line.m_iType == 21)
		{
			Cell(m_wDyn, x0, y, 110, 34, "Start from", m_cClear, m_cDim, 0, 18, 0);
			float tx = x0 + 112;
			array<string> tmpl = {"Rifle squad", "Fire team", "AT team", "Recon team", "Empty"};
			foreach (int ti, string tn : tmpl)
				tx = SpButton(tx, y, 116, tn, 110, ti, m_cCell, m_cText);
			if (!m_sPBUndoText.IsEmpty())
			{
				Cell(m_wDyn, tx + 4, y, 220, 34, m_sPBUndoText, m_cClear, m_cAmber, 0, 16, 0);
				SpButton(tx + 228, y, 80, "Undo", 118, 0, m_cOrange, m_cDark);
			}
			return;
		}
		if (line.m_iType == 22)
		{
			PBRenderColumns(x0, y, w);
			return;
		}
		if (line.m_iType == 23)
		{
			float dx = x0;
			string role = "Role: " + PBRoleName(m_iPBRole);
			if (m_iPBRole == 0)
				role = "Role: " + PBRoleName(DCO_GroupPresets.AutoRole(PBMembers(), !m_sPBVeh.IsEmpty())) + " · auto";
			dx = SpButton(dx, y, 220, role, 120, (m_iPBRole + 1) % 6, m_cCell, m_cText);
			array<string> skill = {"DCO skill Rookie", "DCO skill Regular", "DCO skill Veteran", "DCO skill Expert"};
			dx = SpButton(dx, y, 170, PBExpName(m_iPBExp), 121, (m_iPBExp + 1) % 4, m_cCell, m_cText);
			array<string> mor = {"Low morale", "Normal morale", "High morale"};
			dx = SpButton(dx, y, 170, mor[m_iPBMor], 122, (m_iPBMor + 1) % 3, m_cCell, m_cText);
			array<string> sh = {"Rested", "Reduced"};
			dx = SpButton(dx, y, 120, sh[m_iPBShape], 123, (m_iPBShape + 1) % 2, m_cCell, m_cText);
			string arr = "Arrival now";
			if (m_iPBArr > 0)
				arr = string.Format("Arrival %1 min", m_iPBArr);
			int next = 10;
			if (m_iPBArr >= 40)
				next = 0;
			else if (m_iPBArr >= 25)
				next = 40;
			else if (m_iPBArr >= 10)
				next = 25;
			SpButton(dx, y, 160, arr, 124, next, m_cCell, m_cText);
			string morEff = "Morale ×1.0";
			if (m_iPBMor == 0)
				morEff = "Morale ×0.8, starts shaken";
			else if (m_iPBMor == 2)
				morEff = "Morale ×1.2";
			string shapeEff = "Full stamina and magazines";
			if (m_iPBShape == 1)
				shapeEff = "Stamina 50%, spare magazines 60% (at least 1)";
			Cell(m_wDyn, x0, y + 38, w, 26, skill[Math.ClampInt(m_iPBExp, 0, 3)] + " · " + morEff + " · " + shapeEff, m_cClear, m_cDim, 0, 15, 0);
			return;
		}
		if (line.m_iType == 24)
			PBRenderFooter(x0, y, w);
	}

	protected void PBRenderColumns(float x0, float y, float w)
	{
		float lx = x0;
		float lw = 300;
		Cell(m_wDyn, lx, y, lw, 26, "Add soldiers", m_cClear, m_cOrange, 1, 18, 0);
		array<string> types = {};
		DCO_GroupPresets.Types(types);
		float cy = y + 28;
		foreach (int ti, string t : types)
		{
			int n = PBCount(t);
			Color cbg = m_cRowBg;
			if (m_aPBM.Count() >= 12)
				cbg = m_cPanel;
			string gear = DCO_GroupPresets.GearText(m_sPBFaction, t);
			string cardTip = DCO_GroupPresets.TypeName(t) + ". Click to add at the end.";
			if (gear != DCO_GroupPresets.TypeName(t))
				cardTip = string.Format("%1: %2. Click to add at the end.", DCO_GroupPresets.TypeName(t), gear);
			int addId = Act(DCO_ESetAct.SPAWNER, 111, ti, cardTip);
			Cell(m_wDyn, lx, cy, lw - 46, 34, string.Empty, cbg, m_cText, 0, 10, addId);
			Cell(m_wDyn, lx + 4, cy + 3, 40, 28, DCO_GroupPresets.Badge(t), m_cCell, m_cText, 2, 16, addId);
			Cell(m_wDyn, lx + 48, cy + 3, 170, 28, DCO_GroupPresets.TypeName(t), m_cClear, m_cText, 0, 18, addId);
			string cnt;
			if (n > 0)
				cnt = n.ToString();
			Cell(m_wDyn, lx + 218, cy + 3, 32, 28, cnt, m_cClear, m_cOrange, 2, 18, addId);
			if (n > 0)
				Cell(m_wDyn, lx + lw - 42, cy, 40, 34, "−", m_cCell, m_cText, 3, 18, Act(DCO_ESetAct.SPAWNER, 112, ti, string.Empty));
			cy += 37;
		}

		float mx = x0 + 316;
		float mw = 376;
		Cell(m_wDyn, mx, y, mw, 26, string.Format("Group · %1 of 12 men", m_aPBM.Count()), m_cClear, m_cOrange, 1, 18, 0);
		int seats = PBSeats();
		float sx = mx;
		float slotW = (mw - 22) / 12;
		for (int si = 0; si < 12; si++)
		{
			string badge;
			Color sbg = m_cPanel;
			if (si < m_aPBM.Count())
			{
				badge = DCO_GroupPresets.Badge(m_aPBM[si]);
				sbg = m_cCell;
				if (seats >= 0 && si >= seats)
					sbg = m_cWarm;
			}
			Cell(m_wDyn, sx, y + 30, slotW, 28, badge, sbg, m_cText, 2, 14, 0);
			if (si == 0 && !m_aPBM.IsEmpty())
				Cell(m_wDyn, sx, y + 56, slotW, 3, string.Empty, m_cOrange, m_cText, 0, 10, 0);
			sx += slotW + 2;
		}
		float ly = y + 64;
		if (m_aPBM.IsEmpty())
		{
			Cell(m_wDyn, mx, ly, mw, 30, "No soldiers yet. Add them from the list on the left.", m_cClear, m_cDim, 0, 18, 0);
			ly += 32;
		}
		foreach (int mi, string mt : m_aPBM)
		{
			Color rbg = m_cRowBg;
			if (mi == m_iPBSel)
				rbg = m_cTileOn;
			int selId = Act(DCO_ESetAct.SPAWNER, 113, mi, string.Empty);
			Cell(m_wDyn, mx, ly, mw, 22, string.Empty, rbg, m_cText, 0, 10, selId);
			string lead;
			if (mi == 0)
				lead = "  LEADS";
			Cell(m_wDyn, mx + 4, ly, 220, 22, string.Format("%1  %2  %3%4", mi + 1, DCO_GroupPresets.Badge(mt), DCO_GroupPresets.TypeName(mt), lead), m_cClear, m_cText, 0, 16, selId);
			float bx = mx + mw - 120;
			Cell(m_wDyn, bx, ly, 28, 22, "↑", m_cCell, m_cText, 3, 16, Act(DCO_ESetAct.SPAWNER, 114, mi, "Move up"));
			Cell(m_wDyn, bx + 30, ly, 28, 22, "↓", m_cCell, m_cText, 3, 16, Act(DCO_ESetAct.SPAWNER, 115, mi, "Move down"));
			Cell(m_wDyn, bx + 60, ly, 28, 22, "★", m_cCell, m_cText, 3, 16, Act(DCO_ESetAct.SPAWNER, 116, mi, "Make leader"));
			Cell(m_wDyn, bx + 90, ly, 28, 22, "✕", m_cCell, m_cText, 3, 16, Act(DCO_ESetAct.SPAWNER, 117, mi, "Remove"));
			ly += 23;
		}
		float vy = y + 64 + 12 * 23 + 4;
		string veh = "None";
		if (!m_sPBVeh.IsEmpty())
			veh = FilePath.StripExtension(FilePath.StripPath(m_sPBVeh));
		string vehText = "Vehicle: " + veh;
		if (seats >= 0)
			vehText += string.Format(" · %1 seats · %2", seats, PBVehClass(m_sPBVeh));
		SpButton(mx, vy, mw, vehText, 119, 0, m_cCell, m_cText);
		string seatLine = "The group walks.";
		if (seats >= 0)
		{
			int men = m_aPBM.Count();
			if (men > seats)
				seatLine = string.Format("%1 men, %2 seats: %3 walk next to the vehicle (shaded in the bar above).", men, seats, men - seats);
			else
				seatLine = string.Format("%1 men, %2 seats: everyone rides.", men, seats);
		}
		Cell(m_wDyn, mx, vy + 36, mw, 26, seatLine, m_cClear, m_cDim, 0, 16, 0);

		PBRenderSummary(x0 + 708, y, w - 708);
	}

	protected void PBRenderSummary(float rx, float y, float rw)
	{
		string name = PBName();
		Cell(m_wDyn, rx, y, rw, 26, name, m_cClear, m_cOrange, 1, 18, 0);
		string dest = "Server: ";
		if (m_iPBDest == 1)
			dest = "My PC: ";
		else if (m_iPBDest == 2)
			dest = "Server + my PC: ";
		Cell(m_wDyn, rx, y + 26, rw, 22, dest + "$profile:DCO/GroupPresets/" + DCO_GroupPresets.FileName(name), m_cClear, m_cDim, 0, 14, 0);
		float cy = y + 52;
		Cell(m_wDyn, rx, cy, rw, 22, "Can do", m_cClear, m_cDim, 0, 16, 0);
		cy += 24;
		int leaders = PBCount("SL") + PBCount("TL");
		int at = PBCount("AT");
		int mg = PBCount("AR") + PBCount("MG");
		int gren = PBCount("GL") + PBCount("TL");
		array<string> chips = {};
		array<bool> has = {};
		chips.Insert("Leader");
		has.Insert(leaders > 0);
		chips.Insert("AT ×" + at.ToString());
		has.Insert(at > 0);
		chips.Insert("MG ×" + mg.ToString());
		has.Insert(mg > 0);
		chips.Insert("Grenades ×" + gren.ToString());
		has.Insert(gren > 0);
		chips.Insert("Medic");
		has.Insert(PBCount("MED") > 0);
		chips.Insert("Radio");
		has.Insert(PBCount("RTO") > 0);
		chips.Insert("Marksman");
		has.Insert(PBCount("MRK") > 0);
		float chx = rx;
		foreach (int ci, string chip : chips)
		{
			string ct = chip;
			Color cbg = m_cCell;
			Color cfg = m_cText;
			if (!has[ci])
			{
				int cut = ct.IndexOf(" ×");
				if (cut > 0)
					ct = ct.Substring(0, cut);
				ct += " · none";
				cbg = m_cClear;
				cfg = m_cDim;
			}
			float cwid = 14 + ct.Length() * 9;
			if (chx + cwid > rx + rw)
			{
				chx = rx;
				cy += 30;
			}
			Cell(m_wDyn, chx, cy, cwid, 26, ct, cbg, cfg, 2, 14, 0);
			chx += cwid + 6;
		}
		cy += 34;
		bool vehicle = !m_sPBVeh.IsEmpty();
		DCO_ESpawnerRole autoRole = DCO_GroupPresets.AutoRole(PBMembers(), vehicle);
		string roleText;
		if (m_iPBRole == 0)
			roleText = string.Format("%1 · auto, because %2. The commander picks this group when it needs %3.", PBRoleName(autoRole), DCO_GroupPresets.AutoRoleReason(PBMembers(), vehicle), PBNeed(autoRole));
		else
			roleText = string.Format("%1 · set by you. The commander picks this group when it needs %2.", PBRoleName(m_iPBRole), PBNeed(m_iPBRole));
		Widget rc = Cell(m_wDyn, rx, cy, rw, 48, roleText, m_cClear, m_cText, 0, 16, 0);
		WrapCell(rc);
		cy += 52;
		int men = m_aPBM.Count();
		float expF = DCO_CommanderSpawnerComponent.ExperienceCostPct(m_iPBExp) / 100.0;
		float raw = expF * men;
		string mods = string.Format("%1 men × %2 (%3)", men, expF.ToString(-1, 1), PBExpName(m_iPBExp));
		if (m_iPBMor == 0)
		{
			raw *= 0.8;
			mods += " × 0.8 (low morale)";
		}
		if (m_iPBShape == 1)
		{
			raw *= 0.8;
			mods += " × 0.8 (reduced)";
		}
		int baseCost = Math.Round(raw);
		int vehCost = DCO_CommanderSpawnerComponent.VehicleCost(m_sPBVeh);
		string costText = string.Format("%1 = %2 → %3", mods, raw.ToString(-1, 1), baseCost);
		if (vehicle)
			costText += string.Format("\n+ vehicle %1 (%2)", vehCost, PBVehClass(m_sPBVeh));
		costText += string.Format("\n= %1 points", baseCost + vehCost);
		Widget cc = Cell(m_wDyn, rx, cy, rw, 66, costText, m_cClear, m_cText, 0, 16, 0);
		WrapCell(cc);
		cy += 70;
		string role = PBRoleName(autoRole);
		if (m_iPBRole != 0)
			role = PBRoleName(m_iPBRole);
		string arr = "now";
		if (m_iPBArr > 0)
			arr = string.Format("%1 min", m_iPBArr);
		string morName = "Normal morale";
		if (m_iPBMor == 0)
			morName = "Low morale";
		else if (m_iPBMor == 2)
			morName = "High morale";
		string shapeName = "Rested";
		if (m_iPBShape == 1)
			shapeName = "Reduced";
		string poolName = name;
		if (m_iPBExp != DCO_ESpawnerExperience.REGULAR)
			poolName += " · " + PBExpName(m_iPBExp);
		Widget pc = Cell(m_wDyn, rx, cy, rw, 44, string.Format("In the pool it reads: %1 · %2 · %3 · %4 · %5 · %6 pts", poolName, role, morName, shapeName, arr, baseCost + vehCost), m_cClear, m_cDim, 0, 16, 0);
		WrapCell(pc);
	}

	protected string PBNeed(int role)
	{
		if (role == DCO_ESpawnerRole.AT)
			return "anti-armor";
		if (role == DCO_ESpawnerRole.RECON)
			return "recon for unclear contacts";
		if (role == DCO_ESpawnerRole.MOTORIZED)
			return "a mobile group";
		if (role == DCO_ESpawnerRole.FIRE_TEAM)
			return "a small team";
		return "assault strength";
	}

	protected string PBChecks(out int level)
	{
		level = 0;
		int men = m_aPBM.Count();
		if (men == 0)
		{
			level = 2;
			return "Add at least one soldier.";
		}
		if (PBName().IsEmpty() || DCO_GroupPresets.FileKey(PBName()).IsEmpty())
		{
			level = 2;
			return "Give the preset a name.";
		}
		string nameCode;
		string nameErr = DCO_GroupPresets.NameProblem(PBName(), nameCode);
		if (!nameErr.IsEmpty())
		{
			level = 2;
			return nameErr;
		}
		string warn;
		if (!DCO_GroupPresets.IsLeaderType(m_aPBM[0]))
			warn = string.Format("A %1 leads. Put a Squad leader or Team leader first with ★.", DCO_GroupPresets.TypeName(m_aPBM[0]));
		int seats = PBSeats();
		if (seats >= 0 && men > seats)
		{
			string sw = string.Format("%1 men walk: the %2 has %3 seats.", men - seats, FilePath.StripExtension(FilePath.StripPath(m_sPBVeh)), seats);
			if (warn.IsEmpty())
				warn = sw;
			else
				warn += " " + sw;
		}
		if (PBNameTaken())
		{
			string nw = string.Format("\"%1\" already exists. Saving asks to replace it.", PBName());
			if (warn.IsEmpty())
				warn = nw;
			else
				warn += " " + nw;
		}
		if (!warn.IsEmpty())
		{
			level = 1;
			return warn;
		}
		if (men >= 6)
		{
			string info;
			if (PBCount("MED") == 0)
				info = "No medic: wounded wait for a medic from another group.";
			if (PBCount("RTO") == 0)
			{
				if (!info.IsEmpty())
					info += " ";
				info += "No radio operator: the leader reports by the group radio only.";
			}
			if (!info.IsEmpty())
				return info;
		}
		return "Ready to save.";
	}

	protected void PBRenderFooter(float x0, float y, float w)
	{
		if (m_bPBAskDiscard)
		{
			Cell(m_wDyn, x0, y, 420, 34, "Discard this preset? The changes are lost.", m_cClear, m_cAmber, 0, 18, 0);
			float dx = SpButton(x0 + 430, y, 120, "Discard", 103, 1, m_cDanger, m_cWhite);
			SpButton(dx, y, 150, "Keep editing", 103, 0, m_cCell, m_cText);
			return;
		}
		if (m_bPBAskReplace)
		{
			Cell(m_wDyn, x0, y, 520, 34, string.Format("Replace \"%1\"? Spawners that already use it keep their copy.", PBName()), m_cClear, m_cAmber, 0, 18, 0);
			float rx = SpButton(x0 + 530, y, 120, "Replace", 133, 1, m_cOrange, m_cDark);
			SpButton(rx, y, 110, "Cancel", 133, 0, m_cCell, m_cText);
			return;
		}
		int level;
		string check = PBChecks(level);
		if (!m_sPBMsg.IsEmpty())
			check = m_sPBMsg;
		Color cfg = m_cDim;
		if (level == 1)
			cfg = m_cAmber;
		else if (level == 2)
			cfg = m_cDanger;
		Cell(m_wDyn, x0, y, w, 28, check, m_cClear, cfg, 0, 18, 0);
		float by = y + 34;
		float bx = x0;
		if (!DCO_GroupPresets.IsSinglePc())
		{
			Cell(m_wDyn, bx, by, 80, 34, "Save to", m_cClear, m_cDim, 0, 18, 0);
			bx += 82;
			array<string> dests = {"This server", "My PC", "Both"};
			array<string> tips = {"Saved on this server: every GM here can use it.", "Saved in your own profile: you take it to every server.", "Saved on this server and on your PC."};
			foreach (int di, string dn : dests)
			{
				Color dbg = m_cCell;
				if (di == m_iPBDest)
					dbg = m_cTileOn;
				Cell(m_wDyn, bx, by, 120, 34, dn, dbg, m_cText, 3, 17, Act(DCO_ESetAct.SPAWNER, 125, di, tips[di]));
				bx += 128;
			}
			bx += 12;
		}
		Color saveBg = m_cOrange;
		if (level == 2)
			saveBg = m_cCell;
		if (m_iPBFrom == 0)
		{
			string sname;
			DCO_CommanderSpawnerComponent s = SpEdited();
			if (s)
				sname = s.Name();
			bx = SpButton(bx, by, 250, "Save and add to " + sname, 130, 0, saveBg, m_cDark);
			bx = SpButton(bx, by, 120, "Save only", 131, 0, m_cCell, m_cText);
			SpButton(bx, by, 200, "Add to this pool only", 132, 0, m_cCell, m_cText);
		}
		else
			SpButton(bx, by, 160, "Save preset", 130, 0, saveBg, m_cDark);
	}

	protected void PBTouch()
	{
		m_bPBDirty = true;
		m_sPBMsg = string.Empty;
		m_bPBAskReplace = false;
	}

	protected void PBBack()
	{
		if (m_bPBDirty && !m_bPBAskDiscard)
		{
			m_bPBAskDiscard = true;
			RequestRender();
			return;
		}
		if (m_bPBAskDiscard)
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("group_preset_discard gm=%1", DCO_GroupPresets.LocalPlayerId()));
		m_bPB = false;
		m_bPBAskDiscard = false;
		m_iScroll = 0;
		RequestRender();
	}

	protected void PBSave(int mode)
	{
		int level;
		PBChecks(level);
		if (level == 2)
		{
			RequestRender();
			return;
		}
		if (mode != 2 && PBNameTaken() && !m_bPBAskReplace)
		{
			m_bPBAskReplace = true;
			m_iPBPending = mode;
			RequestRender();
			return;
		}
		bool replace = m_bPBAskReplace || (!m_sPBEditing.IsEmpty() && PBName() == m_sPBEditing);
		m_bPBAskReplace = false;
		DCO_GroupPreset p = PBPreset();
		p.m_sMembers = DCO_GroupPresets.LeaderFirst(p.m_sMembers);
		string from = "builder_global";
		if (m_iPBFrom == 0)
			from = "builder_spawner";
		else if (m_iPBFrom == 2)
			from = "world";
		string saved;
		if (mode != 2)
		{
			bool toServer = m_iPBDest == 0 || m_iPBDest == 2;
			bool toPc = m_iPBDest == 1 || m_iPBDest == 2 || DCO_GroupPresets.IsSinglePc();
			if (DCO_GroupPresets.IsSinglePc())
				toServer = false;
			if (toPc)
			{
				string op = "save";
				if (DCO_GroupPresets.Exists(p.m_sName))
					op = "replace";
				bool ok = DCO_GroupPresets.Save(p);
				string dest = "pc";
				if (m_iPBDest == 2)
					dest = "both";
				DCO_GroupPresets.LogOp(op, p, from, dest, DCO_GroupPresets.LocalPlayerId());
				if (ok)
					saved = "My PC";
				if (DCO_GroupPresets.IsSinglePc())
					DCO_GroupPresets.Publish();
			}
			if (toServer && m_Manager)
			{
				string arg = "from=" + from;
				if (replace)
					arg += ",replace";
				if (m_iPBFrom == 0 && mode == 0)
					DCO_GroupPresets.NotePoolAdd();
				m_Manager.DCO_PresetRequest(1, p.Pack(), arg);
				if (saved.IsEmpty())
					saved = "This server";
				else
					saved = "This server and my PC";
			}
			if (!m_sPBEditing.IsEmpty() && m_sPBEditing != p.m_sName && m_sPBEditWhere == "pc")
				DCO_GroupPresets.Delete(m_sPBEditing);
			PBSetDestPref(m_iPBDest);
		}
		if (m_iPBFrom == 0 && mode != 1)
		{
			DCO_CommanderSpawnerComponent s = SpEdited();
			Managed item = SpItem();
			if (s && item && m_Manager)
			{
				string pack = p.Pack();
				if (mode == 2 && !m_bPBNamed)
					p.m_sName = "Custom group";
				m_Manager.DCO_RequestSpawnerAction(item, 23, 0, 0, p.Pack());
				array<string> pool = {};
				string pstr = s.GetRplPool();
				if (!pstr.IsEmpty())
					pstr.Split(";", pool, true);
				m_iSpRow = pool.Count();
				m_bSpPicker = false;
			}
		}
		m_bPB = false;
		m_bPBDirty = false;
		m_iScroll = 0;
		if (!saved.IsEmpty())
		{
			DCO_GroupPresets.SetMsg(string.Format("\"%1\" saved. Saved to: %2.", p.m_sName, saved));
			m_iGPMsgSeq = -1;
		}
		GetGame().GetCallqueue().CallLater(RequestRender, 700, false);
		RequestRender();
	}

	protected void PCopyToServer(int mode)
	{
		if (!m_Manager)
			return;
		string conflictPack = DCO_GroupPresets.PeekConflict();
		if ((mode == 1 || mode == 2) && !conflictPack.IsEmpty())
		{
			string carg = "replace";
			if (mode == 2)
				carg = "keep";
			m_Manager.DCO_PresetRequest(5, conflictPack, carg);
			DCO_GroupPreset cp = DCO_GroupPreset.Unpack(conflictPack);
			string cname = "-";
			if (cp)
				cname = cp.m_sName;
			DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", string.Format("preset_conflict_resolve name=%1 mode=%2", cname, carg));
			m_iGPConflict = 0;
			DCO_GroupPresets.TakeConflict();
			m_sGPMsg = "Copying to the server…";
			return;
		}
		if (!m_aGP.IsIndexValid(m_iGPSel))
			return;
		DCO_GroupPreset p = m_aGP[m_iGPSel];
		if (p.m_sWhere != "pc" && p.m_sWhere != "differ" && p.m_sWhere != "both")
		{
			m_sGPMsg = "Pick a preset that is on your PC.";
			return;
		}
		DCO_GroupPreset local = DCO_GroupPresets.Load(p.m_sName);
		if (!local)
			return;
		bool vehOnly;
		string missing = DCO_GroupPresets.MissingItem(local, vehOnly);
		if (!missing.IsEmpty() && !(vehOnly && mode == 3))
		{
			string reason = "missing_prefab";
			if (missing.Contains("faction"))
				reason = "missing_faction";
			DCO_GroupPresets.LogCopy("to_server", local.m_sName, DCO_GroupPresets.LocalPlayerId(), "blocked", reason);
			m_sGPMsg = string.Format("\"%1\" cannot be copied as it is: %2 is from a mod that is not loaded on this server.", local.m_sName, missing);
			return;
		}
		string arg;
		if (mode == 1)
			arg = "replace";
		else if (mode == 2)
			arg = "keep";
		else if (mode == 3)
			arg = "novehicle";
		m_Manager.DCO_PresetRequest(5, local.Pack(), arg);
		m_iGPConflict = 0;
		DCO_GroupPresets.TakeConflict();
		m_sGPMsg = "Copying to the server…";
	}

	protected void PCopyToPc(int mode)
	{
		if (!m_aGP.IsIndexValid(m_iGPSel) || !m_Manager)
			return;
		DCO_GroupPreset p = m_aGP[m_iGPSel];
		if (p.m_sWhere == "pc")
		{
			m_sGPMsg = "This preset is already only on your PC.";
			return;
		}
		if (mode == 0 && (p.m_sWhere == "differ" || (p.m_sWhere != "both" && DCO_GroupPresets.Exists(p.m_sName))))
		{
			m_iGPConflict = 2;
			m_sGPConflictName = p.m_sName;
			return;
		}
		string arg = "replace";
		if (mode == 2)
			arg = "keep";
		m_Manager.DCO_PresetRequest(6, p.m_sName, arg);
		m_iGPConflict = 0;
		m_sGPMsg = "Copying to your PC…";
	}

	protected void PDeleteSelected()
	{
		if (!m_aGP.IsIndexValid(m_iGPSel))
			return;
		DCO_GroupPreset p = m_aGP[m_iGPSel];
		if (p.m_sWhere == "builtin")
			return;
		if (p.m_sWhere == "pc" || p.m_sWhere == "both" || p.m_sWhere == "differ" || DCO_GroupPresets.IsSinglePc())
		{
			DCO_GroupPreset local = DCO_GroupPresets.Load(p.m_sName);
			DCO_GroupPresets.Delete(p.m_sName);
			string dest = "pc";
			if (p.m_sWhere != "pc")
				dest = "both";
			DCO_GroupPresets.LogOp("delete", local, "builder_global", dest, DCO_GroupPresets.LocalPlayerId());
			if (DCO_GroupPresets.IsSinglePc())
				DCO_GroupPresets.Publish();
		}
		if (!DCO_GroupPresets.IsSinglePc() && p.m_sWhere != "pc" && m_Manager)
			m_Manager.DCO_PresetRequest(2, p.m_sName, string.Empty);
		m_sGPMsg = string.Format("\"%1\" deleted. Spawners that use it keep their copy.", p.m_sName);
		m_iGPSel = -1;
	}

	protected void PRenameSelected()
	{
		if (!m_aGP.IsIndexValid(m_iGPSel))
			return;
		DCO_GroupPreset p = m_aGP[m_iGPSel];
		string nn = DCO_GroupPresets.CleanName(m_sGPRename);
		if (nn.IsEmpty() || DCO_GroupPresets.FileKey(nn).IsEmpty())
		{
			m_sGPMsg = "Rename refused: the name is empty.";
			return;
		}
		array<ref DCO_GroupPreset> all = {};
		PMerge(all);
		foreach (DCO_GroupPreset o : all)
		{
			if (o.m_sName == nn && o.m_sName != p.m_sName)
			{
				m_sGPMsg = string.Format("Rename refused: \"%1\" already exists.", nn);
				return;
			}
		}
		if (p.m_sWhere == "pc" || p.m_sWhere == "both" || p.m_sWhere == "differ" || DCO_GroupPresets.IsSinglePc())
		{
			DCO_GroupPreset local = DCO_GroupPresets.Load(p.m_sName);
			if (local)
			{
				local.m_sName = nn;
				if (DCO_GroupPresets.Save(local) && DCO_GroupPresets.FileKey(nn) != DCO_GroupPresets.FileKey(p.m_sName))
					DCO_GroupPresets.Delete(p.m_sName);
				DCO_GroupPresets.LogOp("rename", local, "builder_global", "pc", DCO_GroupPresets.LocalPlayerId());
				if (DCO_GroupPresets.IsSinglePc())
					DCO_GroupPresets.Publish();
			}
		}
		if (!DCO_GroupPresets.IsSinglePc() && p.m_sWhere != "pc" && m_Manager)
			m_Manager.DCO_PresetRequest(3, p.m_sName, nn);
		m_sGPMsg = string.Format("Renamed to \"%1\".", nn);
		m_bGPRename = false;
	}

	protected void PDuplicateSelected()
	{
		if (!m_aGP.IsIndexValid(m_iGPSel))
			return;
		DCO_GroupPreset p = m_aGP[m_iGPSel];
		if (DCO_GroupPresets.IsSinglePc() || p.m_sWhere == "pc")
		{
			DCO_GroupPreset dup = p.Copy();
			dup.m_sName = DCO_GroupPresets.UniqueName(p.m_sName + " (copy)");
			dup.m_sMaker = GetGame().GetPlayerManager().GetPlayerName(DCO_GroupPresets.LocalPlayerId());
			dup.m_iCreated = 0;
			DCO_GroupPresets.Save(dup);
			DCO_GroupPresets.LogOp("duplicate", dup, "builder_global", "pc", DCO_GroupPresets.LocalPlayerId());
			if (DCO_GroupPresets.IsSinglePc())
				DCO_GroupPresets.Publish();
			m_sGPMsg = string.Format("Duplicated as \"%1\".", dup.m_sName);
			return;
		}
		if (m_Manager)
			m_Manager.DCO_PresetRequest(4, p.m_sName, string.Empty);
		m_sGPMsg = "Duplicating on the server…";
	}

	protected void PAct(int code, int b)
	{
		if (code == 100)
		{
			DCO_CommanderSpawnerComponent s = SpEdited();
			string fac = "US";
			if (s)
				fac = s.GetRplFaction();
			PBOpen(0, null, fac);
			return;
		}
		if (code == 101)
		{
			PBBack();
			return;
		}
		if (code == 102)
		{
			if (!m_aPBPick.IsIndexValid(b))
				return;
			Managed item = SpItem();
			if (item && m_Manager)
				m_Manager.DCO_RequestSpawnerAction(item, 23, 0, 0, m_aPBPick[b].Pack());
			m_bSpPicker = false;
			GetGame().GetCallqueue().CallLater(RequestRender, 600, false);
			RequestRender();
			return;
		}
		if (code == 103)
		{
			if (b == 1)
				PBBack();
			else
			{
				m_bPBAskDiscard = false;
				RequestRender();
			}
			return;
		}
		if (code >= 110 && code <= 126)
		{
			PBEdit(code, b);
			RequestRender();
			return;
		}
		if (code >= 130 && code <= 132)
		{
			PBSave(code - 130);
			return;
		}
		if (code == 133)
		{
			if (b == 1)
			{
				m_bPBAskReplace = true;
				PBSave(m_iPBPending);
				return;
			}
			m_bPBAskReplace = false;
			RequestRender();
			return;
		}
		PPageAct(code, b);
		RequestRender();
	}

	protected void PBEdit(int code, int b)
	{
		if (code == 110)
		{
			PBTemplate(b, true);
			PBTouch();
			return;
		}
		if (code == 111)
		{
			array<string> types = {};
			DCO_GroupPresets.Types(types);
			if (!types.IsIndexValid(b))
				return;
			if (m_aPBM.Count() >= 12)
			{
				m_sPBMsg = "The group is full: 12 men is the most one group can have. Remove someone first or make a second preset.";
				return;
			}
			m_aPBM.Insert(types[b]);
			PBTouch();
			return;
		}
		if (code == 112)
		{
			array<string> types2 = {};
			DCO_GroupPresets.Types(types2);
			if (!types2.IsIndexValid(b))
				return;
			for (int i = m_aPBM.Count() - 1; i >= 0; i--)
			{
				if (m_aPBM[i] == types2[b])
				{
					m_sPBUndo = PBSnap();
					m_sPBUndoText = DCO_GroupPresets.TypeName(types2[b]) + " removed.";
					m_aPBM.RemoveOrdered(i);
					break;
				}
			}
			PBTouch();
			return;
		}
		if (code == 113)
		{
			if (m_iPBSel == b)
				m_iPBSel = -1;
			else
				m_iPBSel = b;
			return;
		}
		if (code == 114 && b > 0 && b < m_aPBM.Count())
		{
			m_aPBM.SwapItems(b, b - 1);
			m_iPBSel = b - 1;
			PBTouch();
			return;
		}
		if (code == 115 && b >= 0 && b < m_aPBM.Count() - 1)
		{
			m_aPBM.SwapItems(b, b + 1);
			m_iPBSel = b + 1;
			PBTouch();
			return;
		}
		if (code == 116 && m_aPBM.IsIndexValid(b) && b > 0)
		{
			string t = m_aPBM[b];
			m_aPBM.RemoveOrdered(b);
			m_aPBM.InsertAt(t, 0);
			m_iPBSel = 0;
			PBTouch();
			return;
		}
		if (code == 117 && m_aPBM.IsIndexValid(b))
		{
			m_sPBUndo = PBSnap();
			m_sPBUndoText = DCO_GroupPresets.TypeName(m_aPBM[b]) + " removed.";
			m_aPBM.RemoveOrdered(b);
			m_iPBSel = -1;
			PBTouch();
			return;
		}
		if (code == 118)
		{
			if (!m_sPBUndo.IsEmpty())
				PBRestore(m_sPBUndo);
			m_sPBUndo = string.Empty;
			m_sPBUndoText = string.Empty;
			PBTouch();
			return;
		}
		if (code == 119)
		{
			int cur = m_aPBVehicles.Find(m_sPBVeh);
			cur++;
			if (cur >= m_aPBVehicles.Count())
				m_sPBVeh = string.Empty;
			else
				m_sPBVeh = m_aPBVehicles[cur];
			PBTouch();
			return;
		}
		if (code == 120)
			m_iPBRole = Math.ClampInt(b, 0, 5);
		else if (code == 121)
			m_iPBExp = Math.ClampInt(b, 0, 3);
		else if (code == 122)
			m_iPBMor = Math.ClampInt(b, 0, 2);
		else if (code == 123)
			m_iPBShape = Math.ClampInt(b, 0, 1);
		else if (code == 124)
			m_iPBArr = b;
		else if (code == 125)
		{
			m_iPBDest = Math.ClampInt(b, 0, 2);
			m_bPBAskReplace = false;
			return;
		}
		else if (code == 126)
		{
			array<string> facs = {"US", "USSR", "FIA"};
			if (!facs.IsIndexValid(b) || m_bPBFacFixed)
				return;
			m_sPBUndo = PBSnap();
			m_sPBUndoText = "Faction changed to " + facs[b] + ".";
			m_sPBFaction = facs[b];
			PBLoadVehicles();
			if (!m_sPBVeh.IsEmpty() && !m_aPBVehicles.Contains(m_sPBVeh))
				m_sPBVeh = string.Empty;
		}
		PBTouch();
	}

	protected void PPageAct(int code, int b)
	{
		if (code != 156 && code != 141)
			m_bGPRename = false;
		if (code != 157)
			m_bGPAskDelete = false;
		if (code == 150)
		{
			m_iGPFac = Math.ClampInt(b, 0, 3);
			m_iGPSel = -1;
			return;
		}
		if (code == 151)
		{
			m_iGPWhere = Math.ClampInt(b, 0, 3);
			m_iGPSel = -1;
			return;
		}
		if (code == 152)
		{
			m_iGPSel = b;
			m_sGPMsg = string.Empty;
			m_iGPConflict = 0;
			return;
		}
		if (code == 153)
		{
			string fac = "US";
			array<string> keys = {"US", "US", "USSR", "FIA"};
			if (m_iGPFac > 0)
				fac = keys[m_iGPFac];
			PBOpen(1, null, fac);
			return;
		}
		bool has = m_aGP.IsIndexValid(m_iGPSel);
		if (!has && code != 160)
		{
			m_sGPMsg = "Pick a preset in the list first.";
			return;
		}
		DCO_GroupPreset sel;
		if (has)
			sel = m_aGP[m_iGPSel];
		if (code == 154)
		{
			if (sel.m_sWhere == "builtin")
			{
				m_sGPMsg = "Built-in presets are read-only. Duplicate it to make your own copy, then edit that.";
				return;
			}
			DCO_GroupPreset src = sel;
			if (sel.m_sWhere != "server")
			{
				DCO_GroupPreset local = DCO_GroupPresets.Load(sel.m_sName);
				if (local)
					src = local;
			}
			PBOpen(1, src, sel.m_sFaction);
			m_sPBEditing = sel.m_sName;
			m_sPBEditWhere = sel.m_sWhere;
			if (!DCO_GroupPresets.IsSinglePc())
			{
				if (sel.m_sWhere == "server")
					m_iPBDest = 0;
				else if (sel.m_sWhere == "pc")
					m_iPBDest = 1;
				else
					m_iPBDest = 2;
			}
			return;
		}
		if (code == 155)
		{
			PDuplicateSelected();
			return;
		}
		if (code == 156)
		{
			if (sel.m_sWhere == "builtin")
			{
				m_sGPMsg = "Built-in presets are read-only. Duplicate it to make your own copy, then edit that.";
				return;
			}
			if (b == 1)
			{
				PRenameSelected();
				return;
			}
			if (b == 2)
			{
				m_bGPRename = false;
				return;
			}
			m_bGPRename = true;
			m_sGPRename = sel.m_sName;
			return;
		}
		if (code == 157)
		{
			if (sel.m_sWhere == "builtin")
			{
				m_sGPMsg = "Built-in presets are read-only. Duplicate it to make your own copy, then edit that.";
				return;
			}
			if (b == 1)
			{
				m_bGPAskDelete = false;
				PDeleteSelected();
				return;
			}
			if (b == 2)
			{
				m_bGPAskDelete = false;
				return;
			}
			m_bGPAskDelete = true;
			return;
		}
		if (code == 158)
		{
			PCopyToServer(b);
			return;
		}
		if (code == 159)
		{
			PCopyToPc(b);
			return;
		}
		if (code == 160)
		{
			if (m_iGPConflict == 1 && has)
				DCO_GroupPresets.LogCopy("to_server", m_sGPConflictName, DCO_GroupPresets.LocalPlayerId(), "blocked", "cancel");
			m_iGPConflict = 0;
			DCO_GroupPresets.TakeConflict();
			DCO_GroupPresets.ClearPoolNote();
			m_sGPMsg = string.Empty;
		}
	}
}
