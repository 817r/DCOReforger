class DCO_CmdViewWorld : DCO_CVPainter
{
	protected static ref DCO_CmdViewWorld s_Inst;

	protected ref map<string, vector> m_mShown = new map<string, vector>();
	protected ref array<string> m_aDetail = {};
	protected bool m_bOn = true;
	protected bool m_bEditor;
	protected bool m_bListening;
	protected bool m_bDrawnOnce;
	protected string m_sSel = "";
	protected int m_iSelCi = -1;
	protected int m_iSelKey = -1;
	protected int m_iPanelType;
	protected string m_sPanelId = "";
	protected int m_iPanelCi = -1;
	protected float m_fDetailAt;
	protected float m_fLogAt;
	protected float m_fEmptyAt;
	protected float m_fLastFrame;
	protected float m_fDrawMs;
	protected float m_fDrawTickAt = -1;
	protected int m_iDrawTicks;
	protected ref set<string> m_sEmptyLogged = new set<string>();
	protected float m_fDbgLogAt;
	protected bool m_bDbgEmptyLogged;
	protected int m_iAssignDeclutter = -1;
	protected string m_sAssignAuto;

	static void Start()
	{
		if (System.IsConsoleApp() || s_Inst)
			return;
		s_Inst = new DCO_CmdViewWorld();
		GetGame().GetCallqueue().CallLater(s_Inst.Poll, 500, true);
	}

	static bool PanelOpen()
	{
		return s_Inst && s_Inst.m_wRoot && s_Inst.m_bOn && s_Inst.m_iPanelType != 0;
	}

	static void OnDetail(int type, string id, array<string> rows)
	{
		if (!s_Inst || type != s_Inst.m_iPanelType || id != s_Inst.m_sPanelId)
			return;
		s_Inst.m_aDetail.Copy(rows);
	}

	protected void Poll()
	{
		bool open = false;
		SCR_EditorManagerCore core = SCR_EditorManagerCore.Cast(SCR_EditorManagerCore.GetInstance(SCR_EditorManagerCore));
		if (core)
		{
			SCR_EditorManagerEntity em = core.GetEditorManager();
			DCO_CmdViewUI ui = DCO_CmdViewUI.Inst();
			open = em && em.IsOpened() && ui && ui.IsAllowedLocal();
		}
		if (open && m_wRoot)
		{
			float nowP = DCO_CmdViewUI.Now();
			if (m_fDrawTickAt < 0 || nowP - m_fDrawTickAt > 2.0)
			{
				EmptyOnce("draw_stalled", string.Format(" ticks=%1 canvas=%2 panel=%3", m_iDrawTicks, DCO_Util.B01(m_wCanvas != null), DCO_Util.B01(m_wPanel != null)));
				GetGame().GetCallqueue().Remove(Draw);
				GetGame().GetCallqueue().CallLater(Draw, 0, true);
				m_fDrawTickAt = nowP;
			}
			return;
		}
		if (!open && !m_bEditor)
			return;
		m_bEditor = open;
		if (open)
			Open();
		else
			Close();
	}

	protected void Open()
	{
		Close();
		WorkspaceWidget ws = GetGame().GetWorkspace();
		if (!ws)
			return;
		Widget parent = null;
		string where = "workspace";
		MenuManager mm = GetGame().GetMenuManager();
		if (mm && mm.GetTopMenu())
			where = "workspace top_menu=" + mm.GetTopMenu().ClassName();
		m_wRoot = ws.CreateWidgets("{5C3A9E1B7D4F2A60}UI/layouts/DCO/DCO_CmdView.layout", parent);
		if (!m_wRoot)
		{
			DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.WARN, "-", "intent_world_empty reason=layout_failed parent=" + where);
			return;
		}
		FrameSlot.SetAnchorMin(m_wRoot, 0, 0);
		FrameSlot.SetAnchorMax(m_wRoot, 1, 1);
		FrameSlot.SetOffsets(m_wRoot, 0, 0, 0, 0);
		m_wRoot.SetZOrder(990);
		m_wCanvas = CanvasWidget.Cast(m_wRoot.FindAnyWidget("Canvas"));
		m_wPanel = CanvasWidget.Cast(m_wRoot.FindAnyWidget("PanelCanvas"));
		m_bDrawnOnce = false;
		m_iDrawTicks = 0;
		m_fDrawTickAt = DCO_CmdViewUI.Now();
		m_sEmptyLogged.Clear();
		if (!m_wCanvas || !m_wPanel)
			EmptyOnce("no_canvas", string.Format(" canvas=%1 panel=%2", DCO_Util.B01(m_wCanvas != null), DCO_Util.B01(m_wPanel != null)));
		Listen(true);
		DCO_CmdViewUI ui = DCO_CmdViewUI.Inst();
		if (ui)
			ui.SetWorldWant(m_bOn);
		m_fLastFrame = DCO_CmdViewUI.Now();
		GetGame().GetCallqueue().CallLater(Draw, 0, true);
		DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", string.Format("intent_world editor=1 open=%1 why=editor_open parent=%2", DCO_Util.B01(m_bOn), where));
	}

	protected void Close()
	{
		GetGame().GetCallqueue().Remove(Draw);
		Listen(false);
		m_aLabels.Clear();
		if (m_wRoot)
		{
			m_wRoot.RemoveFromHierarchy();
			DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", "intent_world editor=0 open=0 why=editor_closed");
		}
		m_wRoot = null;
		m_wCanvas = null;
		m_wPanel = null;
		DCO_CmdViewUI ui = DCO_CmdViewUI.Inst();
		if (ui)
			ui.SetWorldWant(false);
	}

	protected void Listen(bool on)
	{
		InputManager im = GetGame().GetInputManager();
		if (!im || on == m_bListening)
			return;
		m_bListening = on;
		if (on)
		{
			im.AddActionListener("DCO_IntentOverlay", EActionTrigger.DOWN, OnToggle);
			im.AddActionListener("DCO_IntentClose", EActionTrigger.DOWN, OnEsc);
			im.AddActionListener("DCO_IntentWorldSelect", EActionTrigger.DOWN, OnClick);
			return;
		}
		im.RemoveActionListener("DCO_IntentOverlay", EActionTrigger.DOWN, OnToggle);
		im.RemoveActionListener("DCO_IntentClose", EActionTrigger.DOWN, OnEsc);
		im.RemoveActionListener("DCO_IntentWorldSelect", EActionTrigger.DOWN, OnClick);
	}

	protected void OnToggle(float value = 0.0, EActionTrigger reason = 0)
	{
		if (!m_wRoot)
			return;
		SCR_MapEntity mapEnt = SCR_MapEntity.GetMapInstance();
		if (mapEnt && mapEnt.IsOpen())
			return;
		m_bOn = !m_bOn;
		if (!m_bOn)
			ClearSel();
		DCO_CmdViewUI ui = DCO_CmdViewUI.Inst();
		if (ui)
			ui.SetWorldWant(m_bOn);
		DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", string.Format("intent_world editor=1 open=%1 why=toggle", DCO_Util.B01(m_bOn)));
	}

	protected void ClearSel()
	{
		m_sSel = "";
		m_iSelCi = -1;
		m_iSelKey = -1;
		m_iPanelType = 0;
		m_sPanelId = "";
		m_iPanelCi = -1;
		m_aDetail.Clear();
	}

	protected void OnEsc(float value = 0.0, EActionTrigger reason = 0)
	{
		ClearSel();
	}

	protected void OpenPanel(int type, int ci, string id)
	{
		m_iPanelType = type;
		m_iPanelCi = ci;
		m_sPanelId = id;
		m_aDetail.Clear();
		RequestDetail();
	}

	protected void RequestDetail()
	{
		m_fDetailAt = DCO_CmdViewUI.Now();
		SCR_PlayerControllerGroupComponent comp = SCR_PlayerControllerGroupComponent.GetLocalPlayerControllerGroupComponent();
		if (comp && m_iPanelType != 0)
			comp.DCO_CVRequestDetail(m_iPanelType, m_sPanelId);
	}

	protected void SelectObj(int ci, string id, string via)
	{
		m_sSel = id;
		int c = id.IndexOf(":");
		m_iSelCi = ci;
		m_iSelKey = id.Substring(c + 1, id.Length() - c - 1).ToInt();
		OpenPanel(1, ci, id);
		DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", "intent_select type=obj id=" + id + " view=world via=" + via);
	}

	protected void OnClick(float value = 0.0, EActionTrigger reason = 0)
	{
		if (!m_wRoot || !m_bOn)
			return;
		int mx, my;
		WidgetManager.GetMousePos(mx, my);
		DCO_CVHit best = null;
		float bestD = float.MAX;
		foreach (DCO_CVHit h : m_aHits)
		{
			if (mx < h.m_fX0 || mx > h.m_fX1 || my < h.m_fY0 || my > h.m_fY1)
				continue;
			if (h.m_iType >= 20)
			{
				best = h;
				break;
			}
			float cx = (h.m_fX0 + h.m_fX1) * 0.5;
			float cy = (h.m_fY0 + h.m_fY1) * 0.5;
			float dd = (cx - mx) * (cx - mx) + (cy - my) * (cy - my);
			if (dd < bestD)
			{
				bestD = dd;
				best = h;
			}
		}
		if (!best)
			return;
		if (best.m_iType == 22)
		{
			SelectObj(best.m_iCi, best.m_sId, "panel");
			return;
		}
		if (best.m_iType == 20)
			return;
		if (best.m_iType == 4)
		{
			if (m_iPanelType == 4 && m_sPanelId == best.m_sId)
			{
				ClearSel();
				DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", "intent_select type=cmd id=" + best.m_sId + " action=close view=world");
				return;
			}
			m_sSel = "";
			m_iSelCi = -1;
			m_iSelKey = -1;
			OpenPanel(4, best.m_iCi, best.m_sId);
			DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", "intent_select type=cmd id=" + best.m_sId + " view=world");
			return;
		}
		if (best.m_sId == m_sSel)
		{
			ClearSel();
			DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", "intent_select type=obj id=" + best.m_sId + " action=close view=world");
			return;
		}
		string via = "card";
		if (best.m_iType == 2)
			via = "group";
		SelectObj(best.m_iCi, best.m_sId, via);
	}

	protected bool Proj(vector wp, out float sx, out float sy)
	{
		sx = 0;
		sy = 0;
		WorkspaceWidget ws = GetGame().GetWorkspace();
		vector sp = ws.ProjWorldToScreen(wp, GetGame().GetWorld());
		if (sp[2] <= 0)
			return false;
		sx = sp[0] * m_fDpi;
		sy = sp[1] * m_fDpi;
		if (m_fSw <= 1 || m_fSh <= 1)
			return true;
		return sx > -200 && sx < m_fSw + 200 && sy > -200 && sy < m_fSh + 200;
	}

	protected float Ground(float x, float z)
	{
		return GetGame().GetWorld().GetSurfaceY(x, z);
	}

	protected void TerrainLine(float x0, float z0, float x1, float z1, int color, float width, int dashEvery)
	{
		float dx = x1 - x0;
		float dz = z1 - z0;
		float len = Math.Sqrt(dx * dx + dz * dz);
		int n = Math.Clamp(Math.Round(len / 8.0), 1, 80);
		float px = 0;
		float py = 0;
		bool prevOk = Proj(Vector(x0, Ground(x0, z0) + 0.5, z0), px, py);
		for (int i = 1; i <= n; i++)
		{
			float t = i * 1.0 / n;
			float wx = x0 + dx * t;
			float wz = z0 + dz * t;
			float qx = 0;
			float qy = 0;
			bool ok = Proj(Vector(wx, Ground(wx, wz) + 0.5, wz), qx, qy);
			bool draw = prevOk && ok;
			if (dashEvery > 0 && (i % (dashEvery * 2)) >= dashEvery)
				draw = false;
			if (draw)
				DCO_CmdViewUI.Line(m_aCmd, {px, py, qx, qy}, color, width, false);
			px = qx;
			py = qy;
			prevOk = ok;
		}
	}

	protected void PointMark(float x, float z, string text, int color, int kind)
	{
		float sx = 0;
		float sy = 0;
		if (!Proj(Vector(x, Ground(x, z) + 1.0, z), sx, sy))
			return;
		float s = 10.0 * m_fDpi;
		if (kind == 1)
			DCO_CmdViewUI.Line(m_aCmd, {sx - s, sy + s * 0.8, sx + s, sy + s * 0.8, sx, sy - s}, color, 3, true);
		else if (kind == 2)
			DCO_CmdViewUI.Line(m_aCmd, DCO_CmdViewUI.Ellipse(sx, sy, s, s, 20), color, 3, true);
		else
			DCO_CmdViewUI.Poly(m_aCmd, {sx - s * 0.8, sy + s * 0.8, sx + s * 0.8, sy + s * 0.8, sx, sy - s * 0.8}, color);
		Lbl(sx - EstW(text, 18) * 0.5, sy + s + 2.0 * m_fDpi, text, color, 18, 0xFFF4F0E2);
	}

	protected void DrawGeo(DCO_CVGeo ge, int col)
	{
		if (ge.m_bPast)
			col = DCO_CmdViewUI.Alpha(col, 0.5);
		switch (ge.m_iKind)
		{
			case DCO_ECVGeo.AXIS:
			{
				TerrainLine(ge.m_fX0, ge.m_fZ0, ge.m_fX1, ge.m_fZ1, col, 3, 1);
				float ax = 0;
				float ay = 0;
				float bx = 0;
				float by = 0;
				float ddx = ge.m_fX1 - ge.m_fX0;
				float ddz = ge.m_fZ1 - ge.m_fZ0;
				float dl = Math.Sqrt(ddx * ddx + ddz * ddz);
				if (dl > 1)
				{
					float hx = ge.m_fX1 - ddx / dl * 6.0;
					float hz = ge.m_fZ1 - ddz / dl * 6.0;
					if (Proj(Vector(hx, Ground(hx, hz) + 0.5, hz), ax, ay) && Proj(Vector(ge.m_fX1, Ground(ge.m_fX1, ge.m_fZ1) + 0.5, ge.m_fZ1), bx, by))
					{
						float sdx = bx - ax;
						float sdy = by - ay;
						float sl = Math.Sqrt(sdx * sdx + sdy * sdy);
						if (sl > 1)
						{
							sdx /= sl;
							sdy /= sl;
							float s = 9.0 * m_fDpi;
							DCO_CmdViewUI.Poly(m_aCmd, {bx, by, bx - sdx * s * 1.6 - sdy * s * 0.7, by - sdy * s * 1.6 + sdx * s * 0.7, bx - sdx * s * 1.6 + sdy * s * 0.7, by - sdy * s * 1.6 - sdx * s * 0.7}, col);
						}
					}
				}
				return;
			}
			case DCO_ECVGeo.STAGING:
			{
				PointMark(ge.m_fX0, ge.m_fZ0, "AA", col, 1);
				return;
			}
			case DCO_ECVGeo.ATTACK:
			{
				PointMark(ge.m_fX0, ge.m_fZ0, "ATK", col, 2);
				return;
			}
			case DCO_ECVGeo.RALLY:
			{
				PointMark(ge.m_fX0, ge.m_fZ0, "RP", col, 3);
				return;
			}
			case DCO_ECVGeo.LZ:
			{
				PointMark(ge.m_fX0, ge.m_fZ0, "LZ", col, 2);
				return;
			}
			case DCO_ECVGeo.SBF:
			{
				TerrainLine(ge.m_fX0, ge.m_fZ0, ge.m_fX1, ge.m_fZ1, col, 2.5, 2);
				float cdx = ge.m_fX1 - ge.m_fX0;
				float cdz = ge.m_fZ1 - ge.m_fZ0;
				float cl = Math.Sqrt(cdx * cdx + cdz * cdz);
				if (cl > 1)
				{
					float px = -cdz / cl * ge.m_fW;
					float pz = cdx / cl * ge.m_fW;
					int edge = DCO_CmdViewUI.Alpha(col, 0.6);
					TerrainLine(ge.m_fX0, ge.m_fZ0, ge.m_fX1 + px, ge.m_fZ1 + pz, edge, 1.5, 0);
					TerrainLine(ge.m_fX0, ge.m_fZ0, ge.m_fX1 - px, ge.m_fZ1 - pz, edge, 1.5, 0);
				}
				PointMark(ge.m_fX0, ge.m_fZ0, "SBF", col, 3);
				return;
			}
			case DCO_ECVGeo.TRIGGER:
			{
				TerrainLine(ge.m_fX0, ge.m_fZ0, ge.m_fX1, ge.m_fZ1, col, 3, 3);
				float tx = 0;
				float ty = 0;
				if (Proj(Vector(ge.m_fX1, Ground(ge.m_fX1, ge.m_fZ1) + 1.0, ge.m_fZ1), tx, ty))
					Lbl(tx, ty + 4.0 * m_fDpi, "TRIGGER", col, 18, 0xFFF4F0E2);
				return;
			}
		}
	}

	protected void DrawSelection(DCO_CmdViewUI ui)
	{
		foreach (DCO_CVGeo ge : ui.Geos())
		{
			if (ge.m_iCi == m_iSelCi && ge.m_iKey == m_iSelKey && ui.CmdVisible(ge.m_iCi))
				DrawGeo(ge, ui.CmdColor(ge.m_iCi));
		}
		foreach (DCO_CVObj so : ui.Objs())
		{
			if (so.m_iCi != m_iSelCi || so.m_iKey != m_iSelKey)
				continue;
			array<float> ring = {};
			for (int k = 0; k <= 32; k++)
			{
				float a = k * Math.PI2 / 32;
				float rx = so.m_fX + Math.Cos(a) * so.m_fR;
				float rz = so.m_fZ + Math.Sin(a) * so.m_fR;
				float qx = 0;
				float qy = 0;
				if (Proj(Vector(rx, Ground(rx, rz) + 0.5, rz), qx, qy))
				{
					ring.Insert(qx);
					ring.Insert(qy);
				}
				else if (ring.Count() >= 4)
				{
					DCO_CmdViewUI.Line(m_aCmd, ring, ui.CmdColor(so.m_iCi), 2.5, false);
					ring = new array<float>();
				}
			}
			if (ring.Count() >= 4)
				DCO_CmdViewUI.Line(m_aCmd, ring, ui.CmdColor(so.m_iCi), 2.5, false);
		}
	}

	protected void DrawSpawners()
	{
		array<DCO_CommanderSpawnerComponent> all = DCO_CommanderSpawnerComponent.GetAll();
		DrawSpawnerPreview(all);
		if (!all || all.IsEmpty())
			return;
		set<SCR_EditableEntityComponent> sel = new set<SCR_EditableEntityComponent>();
		SCR_BaseEditableEntityFilter.GetEnititiesStatic(sel, EEditableEntityState.SELECTED);
		set<SCR_EditableEntityComponent> hov = new set<SCR_EditableEntityComponent>();
		SCR_BaseEditableEntityFilter.GetEnititiesStatic(hov, EEditableEntityState.HOVER);
		foreach (DCO_CommanderSpawnerComponent s : all)
		{
			if (!s || !s.GetOwner())
				continue;
			IEntity o = s.GetOwner();
			vector p = o.GetOrigin();
			int st = s.GetRplState();
			int col = 0xFF3DA35D;
			if (st == DCO_ESpawnerState.WAITING)
				col = 0xFFE8892B;
			else if (st == DCO_ESpawnerState.COOLDOWN)
				col = 0xFF3B82C4;
			else if (st == DCO_ESpawnerState.OFF || st == DCO_ESpawnerState.STOPPED)
				col = 0xFFD64545;

			SCR_EditableEntityComponent ed = SCR_EditableEntityComponent.Cast(o.FindComponent(SCR_EditableEntityComponent));
			bool focus = ed && (sel.Contains(ed) || hov.Contains(ed));
			if (focus)
				DrawSpawnerAreas(s, p, st);

			float sx = 0;
			float sy = 0;
			if (!Proj(p + Vector(0, 3.0, 0), sx, sy))
				continue;
			float r = 9.0 * m_fDpi;
			if (st == DCO_ESpawnerState.WAITING)
				DCO_CmdViewUI.Poly(m_aCmd, {sx - r, sy + r * 0.8, sx + r, sy + r * 0.8, sx, sy - r}, col);
			else if (st == DCO_ESpawnerState.COOLDOWN)
				DCO_CmdViewUI.Line(m_aCmd, DCO_CmdViewUI.Ellipse(sx, sy, r, r, 20), col, 3, true);
			else if (st == DCO_ESpawnerState.OFF)
				DCO_CmdViewUI.Line(m_aCmd, {sx, sy - r, sx + r, sy, sx, sy + r, sx - r, sy}, col, 3, true);
			else if (st == DCO_ESpawnerState.STOPPED)
				DCO_CmdViewUI.Poly(m_aCmd, DCO_CmdViewUI.Rect(sx - r * 0.8, sy - r * 0.8, sx + r * 0.8, sy + r * 0.8), col);
			else
				DCO_CmdViewUI.Poly(m_aCmd, DCO_CmdViewUI.Ellipse(sx, sy, r * 0.7, r * 0.7, 16), col);
			string name = o.GetName();
			if (name.IsEmpty())
				name = "Spawner";
			string text = name;
			if (focus)
				text = name + "\n" + s.StatusField(0);
			Lbl(sx - EstW(text, 16) * 0.5, sy + r + 2.0 * m_fDpi, text, col, 16, 0xFFF4F0E2);
		}
	}

	protected void DrawSpawnerPreview(array<DCO_CommanderSpawnerComponent> all)
	{
		SCR_PreviewEntityEditorComponent prev = SCR_PreviewEntityEditorComponent.Cast(SCR_PreviewEntityEditorComponent.GetInstance(SCR_PreviewEntityEditorComponent));
		if (!prev || !prev.IsEditing())
			return;
		vector tr[4];
		if (!prev.GetPreviewTransform(tr))
			return;
		bool show = false;
		float spawnR = 30;
		float safe = 300;
		SCR_PlacingEditorComponent pl = SCR_PlacingEditorComponent.Cast(SCR_PlacingEditorComponent.GetInstance(SCR_PlacingEditorComponent));
		if (pl)
		{
			string pf = pl.GetSelectedPrefab();
			if (pf.Contains("CommanderSpawner"))
				show = true;
		}
		if (!show && all)
		{
			set<SCR_EditableEntityComponent> sel = new set<SCR_EditableEntityComponent>();
			SCR_BaseEditableEntityFilter.GetEnititiesStatic(sel, EEditableEntityState.SELECTED);
			foreach (DCO_CommanderSpawnerComponent s : all)
			{
				if (!s || !s.GetOwner())
					continue;
				SCR_EditableEntityComponent ed = SCR_EditableEntityComponent.Cast(s.GetOwner().FindComponent(SCR_EditableEntityComponent));
				if (ed && sel.Contains(ed))
				{
					show = true;
					spawnR = s.GetRplSpawnRadius();
					safe = s.GetRplSafe();
					break;
				}
			}
		}
		if (!show)
			return;
		vector p = tr[3];
		if (spawnR > 0)
			DbgRing(p[0], p[2], spawnR, 0xCC3DA35D, 3.0, 0);
		if (safe > 0)
			DbgRing(p[0], p[2], safe, 0xAAF4F0E2, 2.0, 2);
	}

	protected void DrawSpawnerAreas(DCO_CommanderSpawnerComponent s, vector p, int st)
	{
		float spawnR = s.GetRplSpawnRadius();
		if (spawnR > 0)
			DbgRing(p[0], p[2], spawnR, 0xCC3DA35D, 3.0, 0);
		float safe = s.GetRplSafe();
		if (safe > 0)
		{
			string blocker = s.StatusField(10);
			int sc = 0xAAF4F0E2;
			if (blocker == "overrun")
				sc = 0xFFD64545;
			else if (blocker == "player_near" || blocker == "enemy_near")
				sc = 0xFFE8892B;
			DbgRing(p[0], p[2], safe, sc, 2.0, 2);
			if (!blocker.IsEmpty() && blocker != "-")
			{
				float bx = 0;
				float by = 0;
				if (Proj(Vector(p[0], Ground(p[0], p[2] + safe) + 1.0, p[2] + safe), bx, by))
					Lbl(bx, by, s.StatusField(0), sc, 16, 0xFFF4F0E2);
			}
		}
		vector feed = s.GetRplFeed();
		if (feed != vector.Zero)
			TerrainLine(p[0], p[2], feed[0], feed[2], 0xFFF4F0E2, 2.5, 0);
		vector trig = s.GetRplTrigger();
		if (trig != vector.Zero)
			TerrainLine(p[0], p[2], trig[0], trig[2], 0xFFE8892B, 2.0, 1);
		vector home = s.GetRplHome();
		if (home != vector.Zero)
			TerrainLine(p[0], p[2], home[0], home[2], 0xFF3B82C4, 2.0, 1);
	}

	protected void DbgRing(float x, float z, float r, int col, float width, int dash)
	{
		int n = Math.Clamp(Math.Round(r / 10.0), 24, 64);
		float px = x + r;
		float pz = z;
		for (int k = 1; k <= n; k++)
		{
			float a = k * Math.PI2 / n;
			float qx = x + Math.Cos(a) * r;
			float qz = z + Math.Sin(a) * r;
			TerrainLine(px, pz, qx, qz, col, width, dash);
			px = qx;
			pz = qz;
		}
	}

	protected void DrawDebug(DCO_CmdViewUI ui, float now)
	{
		int t0 = System.GetTickCount();
		float pulse = Math.AbsFloat(Math.Sin(now * 3.0));
		int total = 0;
		int cmds = 0;
		int assignHidden = 0;
		int autoLines = 0;
		string autoWhy;
		bool autoLayerOff = false;
		foreach (DCO_CVCmd c : ui.Cmds())
		{
			if (!ui.CmdVisible(c.m_iIdx))
				continue;
			DCO_CDCmd cd = DCO_CmdDbg.ClientCmd(c.m_iIdx);
			if (!cd)
				continue;
			cmds++;
			bool layerOn = (cd.m_iLayers & DCO_ECDLayer.ASSIGN) != 0;
			bool cmdSel = m_iPanelType == 4 && m_iPanelCi == c.m_iIdx;
			foreach (DCO_CDShape sh : cd.m_aShapes)
			{
				bool assign = sh.m_iLayer == DCO_ECDLayer.ASSIGN;
				if (!assign && (sh.m_iLayer & cd.m_iLayers) == 0)
					continue;
				if (assign)
				{
					bool objSel = m_iSelKey >= 0 && Math.Round(sh.m_fR) == m_iSelKey;
					bool sel = cmdSel || objSel;
					bool strong = sh.m_iCol == DCO_ECDCol.PURPLE || sh.m_iCol == DCO_ECDCol.RED;
					if (!sel && !(strong && layerOn))
					{
						if (sh.m_iCol == DCO_ECDCol.WHITE)
							assignHidden++;
						continue;
					}
					if (sel && sh.m_iType == DCO_ECDShape.LINE)
					{
						autoLines++;
						if (objSel)
							autoWhy = "obj_sel";
						else if (autoWhy.IsEmpty())
							autoWhy = "cmd_sel";
					}
					if (!layerOn)
						autoLayerOff = true;
				}
				total++;
				int col = DCO_CmdDbg.Color(sh.m_iCol);
				if (assign)
				{
					col = ui.CmdColor(c.m_iIdx);
					if (sh.m_iCol == DCO_ECDCol.GREY)
						col = DCO_CmdViewUI.Alpha(col, 0.5);
				}
				int dash = 0;
				if ((sh.m_iFlags & 1) != 0)
					dash = 2;
				float width = 2.0;
				if ((sh.m_iFlags & 4) != 0)
					width = 4.0;
				if ((sh.m_iFlags & 2) != 0)
				{
					width = 2.0 + 2.0 * pulse;
					col = DCO_CmdViewUI.Alpha(col, 0.55 + 0.45 * pulse);
				}
				switch (sh.m_iType)
				{
					case DCO_ECDShape.RING:
					{
						DbgRing(sh.m_fX0, sh.m_fZ0, sh.m_fR, col, width, dash);
						break;
					}
					case DCO_ECDShape.LINE:
					{
						TerrainLine(sh.m_fX0, sh.m_fZ0, sh.m_fX1, sh.m_fZ1, col, width, dash);
						break;
					}
					case DCO_ECDShape.DOT:
					{
						float dx = 0;
						float dy = 0;
						if (!Proj(Vector(sh.m_fX0, Ground(sh.m_fX0, sh.m_fZ0) + 1.0, sh.m_fZ0), dx, dy))
							break;
						float size = 8.0;
						int dcol = col;
						if (sh.m_fX1 >= 0)
						{
							size = Math.Clamp(4.0 + sh.m_fR * 1.5, 4.0, 16.0);
							dcol = DCO_CmdViewUI.Alpha(col, Math.Clamp(1.0 - sh.m_fX1 / 300.0, 0.25, 1.0));
						}
						size *= m_fDpi;
						DCO_CmdViewUI.Poly(m_aCmd, DCO_CmdViewUI.Ellipse(dx, dy, size, size, 16), dcol);
						break;
					}
					case DCO_ECDShape.CROSS:
					{
						float cx = 0;
						float cy = 0;
						if (!Proj(Vector(sh.m_fX0, Ground(sh.m_fX0, sh.m_fZ0) + 1.0, sh.m_fZ0), cx, cy))
							break;
						float s = 7.0 * m_fDpi;
						DCO_CmdViewUI.Line(m_aCmd, {cx - s, cy - s, cx + s, cy + s}, col, 2.5, false);
						DCO_CmdViewUI.Line(m_aCmd, {cx - s, cy + s, cx + s, cy - s}, col, 2.5, false);
						break;
					}
					case DCO_ECDShape.TRI:
					{
						float tx = 0;
						float ty = 0;
						if (!Proj(Vector(sh.m_fX0, Ground(sh.m_fX0, sh.m_fZ0) + 1.0, sh.m_fZ0), tx, ty))
							break;
						float ts = 10.0 * m_fDpi;
						float tw = 3.0;
						if (dash > 0)
							tw = 1.5;
						DCO_CmdViewUI.Line(m_aCmd, {tx - ts, ty + ts * 0.8, tx + ts, ty + ts * 0.8, tx, ty - ts}, col, tw, true);
						break;
					}
					case DCO_ECDShape.LABEL:
					{
						break;
					}
					case DCO_ECDShape.LETTER:
					{
						float lx = 0;
						float ly = 0;
						if (Proj(Vector(sh.m_fX0, Ground(sh.m_fX0, sh.m_fZ0) + 2.0, sh.m_fZ0), lx, ly))
							Lbl(lx - EstW(sh.m_sLabel, 22) * 0.5, ly - 12.0 * m_fDpi, sh.m_sLabel, col, 22, 0xFF1E2226);
						break;
					}
				}
			}
		}
		string autoKey = "0";
		if (autoLines > 0)
		{
			string selId = m_iSelKey.ToString();
			if (autoWhy == "cmd_sel")
				selId = m_iPanelCi.ToString();
			autoKey = string.Format("1 why=%1 sel=%2 lines=%3 layer=%4", autoWhy, selId, autoLines, DCO_Util.B01(!autoLayerOff));
		}
		if (autoKey != m_sAssignAuto)
		{
			bool wasOn = !m_sAssignAuto.IsEmpty() && m_sAssignAuto != "0";
			m_sAssignAuto = autoKey;
			if (autoLines > 0 || wasOn)
				DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", "obj_assign_overlay auto=" + autoKey);
		}
		int declutter = 0;
		if (assignHidden > 0)
			declutter = 1;
		if (declutter != m_iAssignDeclutter)
		{
			m_iAssignDeclutter = declutter;
			DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", string.Format("obj_assign_overlay declutter=%1 planned_hidden=%2 sel_ci=%3 panel_ci=%4 color=commander", declutter, assignHidden, m_iSelCi, m_iPanelCi));
		}
		float now2 = DCO_CmdViewUI.Now();
		if (cmds == 0)
		{
			if (!m_bDbgEmptyLogged && DCO_CmdDbg.RecvAt() <= 0 && now2 - m_fLastFrame >= 0)
			{
				m_bDbgEmptyLogged = true;
				DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", string.Format("cmddbg_empty cmd=all reason=no_data client_cmds=%1", DCO_CmdDbg.ClientCount()));
			}
			return;
		}
		if (now2 >= m_fDbgLogAt)
		{
			m_fDbgLogAt = now2 + 10.0;
			DCO_Perf.CountN("cmddbg:shapes_drawn", total);
			if (DCO_Log.On(DCO_ELogChannel.SYS, DCO_ELogLevel.DEBUG))
				DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.DEBUG, "-", string.Format("cmddbg_draw cmd=%1 shapes=%2 ms=%3", cmds, total, System.GetTickCount() - t0));
		}
	}

	protected int CmdCount(DCO_CmdViewUI ui)
	{
		if (!ui)
			return 0;
		return ui.Cmds().Count();
	}

	protected int m_iEvLogRows = -1;

	protected int DrawEventLog(DCO_CmdViewUI ui)
	{
		float d = m_fDpi;
		float x0 = 16.0 * d;
		float x1 = 640.0 * d;
		float y0 = 130.0 * d;
		int maxChars = Math.Max(20, Math.Round((x1 - x0) / (15.0 * 0.52 * d)) - 4);
		array<string> lines = {};
		array<int> cols = {};
		array<ref DCO_CVDec> decs = ui.Decs();
		for (int i = 0; i < decs.Count() && lines.Count() < 12; i++)
		{
			DCO_CVDec dc = decs[i];
			if (!ui.CmdVisible(dc.m_iCi))
				continue;
			string line = "T+" + DCO_CmdViewStore.Clock(dc.m_fT) + "  " + ui.CmdName(dc.m_iCi) + "  " + dc.m_sObj + "  " + dc.m_sText;
			if (line.Length() > maxChars)
				line = line.Substring(0, maxChars) + "...";
			lines.Insert(line);
			cols.Insert(ui.CmdColor(dc.m_iCi));
		}
		float rowH = 21.0 * d;
		float y1 = y0 + 34.0 * d + Math.Max(lines.Count(), 1) * rowH;
		DCO_CmdViewUI.Poly(m_aPanelCmd, DCO_CmdViewUI.Rect(x0, y0, x1, y1), 0xC815181A);
		Lbl(x0 + 12.0 * d, y0 + 6.0 * d, "EVENT LOG", 0xFFF4F0E2, 16, 0);
		float y = y0 + 30.0 * d;
		if (lines.IsEmpty())
			Lbl(x0 + 12.0 * d, y, "No events yet.", 0xFF9AA0A5, 15, 0);
		foreach (int k, string ln : lines)
		{
			DCO_CmdViewUI.Poly(m_aPanelCmd, DCO_CmdViewUI.Rect(x0 + 6.0 * d, y + 3.0 * d, x0 + 9.0 * d, y + rowH - 3.0 * d), cols[k]);
			Lbl(x0 + 14.0 * d, y, ln, 0xFFC9CCD0, 15, 0);
			y += rowH;
		}
		Occupy(x0, y0, x1, y1);
		if (m_iEvLogRows != lines.Count())
		{
			m_iEvLogRows = lines.Count();
			DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.DEBUG, "-", string.Format("cmdview_event_log rows=%1 decs=%2", lines.Count(), decs.Count()));
		}
		return lines.Count();
	}

	protected int DrawGroups(DCO_CmdViewUI ui, vector camPos, float dt)
	{
		int markers = 0;
		array<string> seen = {};
		foreach (DCO_CVGrp g : ui.Grps())
		{
			if (!ui.CmdVisible(g.m_iCi))
				continue;
			string key = g.m_iCi.ToString() + ":" + g.m_sId;
			seen.Insert(key);
			vector target = Vector(g.m_fX, g.m_fY + 2.5, g.m_fZ);
			if (g.m_fY <= 0)
				target[1] = Ground(g.m_fX, g.m_fZ) + 2.5;
			vector shown = target;
			if (m_mShown.Find(key, shown))
			{
				if (vector.Distance(shown, target) > 60.0)
					shown = target;
				else
					shown = shown + (target - shown) * Math.Clamp(dt * 2.0, 0, 1);
			}
			m_mShown.Set(key, shown);
			float dist = vector.Distance(shown, camPos);
			if (dist > 4000.0)
				continue;
			float gx = 0;
			float gy = 0;
			if (!Proj(shown, gx, gy))
				continue;
			float scale = Math.Clamp(900.0 / Math.Max(dist, 1.0), 0.6, 1.0);
			int gcol = ui.CmdColor(g.m_iCi);
			string oid = g.m_iCi.ToString() + ":" + g.m_iKey.ToString();
			bool rel = g.m_iKey >= 0 && oid == m_sSel;
			if (rel)
				DCO_CmdViewUI.Line(m_aCmd, DCO_CmdViewUI.Ellipse(gx, gy, 24.0 * m_fDpi * scale, 24.0 * m_fDpi * scale, 24), 0xFF111111, 2, true);
			if ((g.m_iFlags & 1) != 0)
				DCO_CmdViewUI.Line(m_aCmd, DCO_CmdViewUI.Ellipse(gx, gy, 18.0 * m_fDpi * scale, 18.0 * m_fDpi * scale, 20), 0xFFD0101A, 2.5, true);
			DCO_CmdViewUI.RoleShape(m_aCmd, g.m_iRole, gx, gy, gcol, 9.0 * m_fDpi * scale);
			string letter = DCO_CmdViewServer.RoleLetter(g.m_iRole);
			int lc = 0xFFFFFFFF;
			if (g.m_iRole == 2 || g.m_iRole == 7)
				lc = gcol;
			int ls = 14;
			if (letter.Length() > 1)
				ls = 11;
			Lbl(gx - EstW(letter, ls) * 0.5, gy - 1.0 * ls * 0.62 * m_fDpi, letter, lc, ls, 0);
			markers++;
			float r = 14.0 * m_fDpi * scale;
			if (g.m_iKey >= 0)
				Hit(2, g.m_iCi, oid, gx - r, gy - r, gx + r, gy + r, "", gcol, g.m_fX, g.m_fZ);
			if (!rel)
				continue;
			string line = g.m_sLine;
			float lw = EstW(line, 16);
			float lx0 = gx - lw * 0.5;
			float ly0 = gy - 40.0 * m_fDpi * scale;
			Occupy(lx0, ly0, lx0 + lw, ly0 + 22.0 * m_fDpi);
			Lbl(lx0, ly0, line, gcol, 16, 0xFFF4F0E2);
		}
		if (m_mShown.Count() > seen.Count() + 50)
		{
			array<string> drop = {};
			foreach (string mk, vector mv : m_mShown)
			{
				if (!seen.Contains(mk))
					drop.Insert(mk);
			}
			foreach (string dk : drop)
				m_mShown.Remove(dk);
		}
		return markers;
	}

	protected void EmptyOnce(string reason, string extra)
	{
		if (m_sEmptyLogged.Contains(reason))
			return;
		m_sEmptyLogged.Insert(reason);
		DCO_Perf.Count("cmdview:world_empty:" + reason);
		DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.WARN, "-", "intent_world_empty reason=" + reason + extra);
	}

	protected void Draw()
	{
		m_fDrawTickAt = DCO_CmdViewUI.Now();
		m_iDrawTicks++;
		if (m_iDrawTicks == 1)
			DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", string.Format("intent_world_tick first=1 canvas=%1 panel=%2 root_visible=%3", DCO_Util.B01(m_wCanvas != null), DCO_Util.B01(m_wPanel != null), DCO_Util.B01(m_wRoot != null && m_wRoot.IsVisible())));
		if (!m_wCanvas || !m_wPanel)
		{
			EmptyOnce("no_canvas", "");
			return;
		}
		int t0 = System.GetTickCount();
		GetGame().GetInputManager().ActivateContext("DCO_CmdViewContext");
		WorkspaceWidget ws = GetGame().GetWorkspace();
		m_fDpi = ws.DPIScale(1000) / 1000.0;
		float sw = 0;
		float sh = 0;
		m_wCanvas.GetScreenSize(sw, sh);
		if (sw <= 1 || sh <= 1)
		{
			sw = ws.GetWidth();
			sh = ws.GetHeight();
		}
		m_fSw = sw;
		m_fSh = sh;
		m_aCmd.Clear();
		m_aPanelCmd.Clear();
		m_aOcc.Clear();
		m_aHits.Clear();
		m_iLabel = 0;
		float now = DCO_CmdViewUI.Now();
		float dt = Math.Clamp(now - m_fLastFrame, 0, 0.5);
		m_fLastFrame = now;
		int markers = 0;
		int cards = 0;
		string empty = "";

		DCO_CmdViewUI ui = DCO_CmdViewUI.Inst();
		SCR_MapEntity mapEnt = SCR_MapEntity.GetMapInstance();
		bool mapOpen = mapEnt && mapEnt.IsOpen();
		if (!mapOpen)
			DrawSpawners();
		if (!m_bOn)
			empty = "off";
		else if (!ui)
			empty = "no_ui";
		else if (ui.IsDenied())
			empty = "denied";
		else if (mapOpen)
			empty = "map_open";
		else if (ui.RecvAt() <= 0)
			empty = "no_snapshot";
		else
		{
			vector cam[4];
			BaseWorld bw = GetGame().GetWorld();
			bw.GetCamera(bw.GetCurrentCameraId(), cam);
			vector camPos = cam[3];
			if (!m_sSel.IsEmpty())
				DrawSelection(ui);
			if (m_iPanelType != 0)
			{
				Occupy(m_fSw - 456.0 * m_fDpi, 0, m_fSw, m_fSh);
				if (now - m_fDetailAt > 4.0)
					RequestDetail();
			}
			markers = DrawGroups(ui, camPos, dt);
			DrawDebug(ui, now);
			cards = DrawEventLog(ui);
			if (m_iPanelType != 0)
				DrawRows(m_aDetail, m_fSw - 456.0 * m_fDpi, 96.0 * m_fDpi, m_fSw - 16.0 * m_fDpi, m_fSh - 96.0 * m_fDpi, ui.CmdColor(m_iPanelCi), m_iPanelCi);
			if (markers + cards == 0)
			{
				if (ui.Cmds().IsEmpty())
					empty = "no_cmd";
				else if (ui.Objs().IsEmpty() && ui.Grps().IsEmpty())
					empty = "snapshot_empty";
				else
					empty = "offscreen";
			}
		}

		for (int i = m_iLabel; i < m_aLabels.Count(); i++)
			m_aLabels[i].SetVisible(false);
		m_wCanvas.SetDrawCommands(m_aCmd);
		m_wPanel.SetDrawCommands(m_aPanelCmd);
		m_fDrawMs = System.GetTickCount() - t0;

		if (markers + cards > 0 && !m_bDrawnOnce)
		{
			m_bDrawnOnce = true;
			DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", string.Format("intent_world_draw first=1 markers=%1 cards=%2 screen=%3x%4 dpi=%5", markers, cards, Math.Round(m_fSw), Math.Round(m_fSh), m_fDpi.ToString(-1, 2)));
		}
		if (!empty.IsEmpty() && empty != "off" && now >= m_fEmptyAt)
		{
			m_fEmptyAt = now + 10.0;
			int objs = 0;
			int grps = 0;
			if (ui)
			{
				objs = ui.Objs().Count();
				grps = ui.Grps().Count();
			}
			string emptyName = empty;
			if (empty == "no_snapshot")
				emptyName = "no_store";
			else if (empty == "map_open" || empty == "denied")
				emptyName = "hidden_" + empty;
			EmptyOnce(emptyName, string.Format(" objs=%1 groups=%2 screen=%3x%4", objs, grps, Math.Round(m_fSw), Math.Round(m_fSh)));
		}
		if (now >= m_fLogAt)
		{
			m_fLogAt = now + 10.0;
			DCO_Perf.CountN("cmdview:world_markers", markers);
			if (DCO_Log.On(DCO_ELogChannel.SYS, DCO_ELogLevel.DEBUG))
				DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.DEBUG, "-", string.Format("intent_world_draw cmd=%1 cards=%2 on_screen=%3 markers=%4 ms=%5 open=%6 sel=%7", CmdCount(ui), cards, markers + cards, markers, m_fDrawMs, DCO_Util.B01(m_bOn), m_sSel));
		}
	}
}
