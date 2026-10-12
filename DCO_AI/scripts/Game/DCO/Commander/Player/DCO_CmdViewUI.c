class DCO_CVCmd
{
	int m_iIdx;
	string m_sUid;
	string m_sFac;
	int m_iColor;
	float m_fX;
	float m_fY;
	float m_fZ;
	string m_sTitle;
	string m_sNow;
	string m_sPlans;
	string m_sDecs;
	string m_sForces;
}

class DCO_CVObj
{
	int m_iCi;
	int m_iKey;
	float m_fX;
	float m_fZ;
	float m_fR;
	int m_iDec;
	string m_sName;
	string m_sCard;
	string m_sTip;
	int m_iGc;
	string m_sSync;
}

class DCO_CVGrp
{
	int m_iCi;
	int m_iKey;
	float m_fX;
	float m_fZ;
	int m_iRole;
	int m_iFlags;
	float m_fDX;
	float m_fDZ;
	float m_fAge;
	string m_sCs;
	string m_sId;
	string m_sTask;
	float m_fY;
	string m_sLine;
}

class DCO_CVGeo
{
	int m_iCi;
	int m_iKey;
	int m_iKind;
	float m_fX0;
	float m_fZ0;
	float m_fX1;
	float m_fZ1;
	bool m_bPast;
	float m_fW;
}

class DCO_CVClu
{
	int m_iCi;
	float m_fX;
	float m_fZ;
	float m_fR;
	float m_fEst;
	float m_fAge;
	int m_iSrc;
	float m_fConf;
	int m_iId;
}

class DCO_CVTru
{
	float m_fX;
	float m_fZ;
	int m_iFi;
}

class DCO_CVDec
{
	int m_iCi;
	string m_sObj;
	float m_fT;
	string m_sText;
}

class DCO_CVHit
{
	int m_iType;
	int m_iCi;
	string m_sId;
	float m_fX0;
	float m_fY0;
	float m_fX1;
	float m_fY1;
	string m_sTip;
	int m_iColor;
	float m_fWX;
	float m_fWZ;
}

class DCO_CmdViewUI : DCO_CVPainter
{
	protected static ref DCO_CmdViewUI s_Inst;

	protected ref array<ref DCO_CVCmd> m_aCmds = {};
	protected ref array<ref DCO_CVObj> m_aObjs = {};
	protected ref array<ref DCO_CVGrp> m_aGrps = {};
	protected ref array<ref DCO_CVGeo> m_aGeos = {};
	protected ref array<ref DCO_CVClu> m_aClus = {};
	protected ref array<ref DCO_CVTru> m_aTrus = {};
	protected ref array<ref DCO_CVDec> m_aDecs = {};
	protected ref array<int> m_aHidden = {};
	protected ref array<string> m_aDetail = {};
	protected bool m_bOn;
	protected bool m_bDenied;
	protected bool m_bListening;
	protected bool m_bAllowed;
	protected float m_fAllowedAt = -100;
	protected int m_iLayers = 7;
	protected string m_sObjFilter = "";
	protected string m_sObjFilterName = "All";
	protected int m_iSelType;
	protected string m_sSelId = "";
	protected int m_iSelCi = -1;
	protected int m_iSelKey = -1;
	protected float m_fDetailAt;
	protected int m_iZoom;
	protected float m_fPanelX0 = -1;
	protected float m_fNow;
	protected bool m_bWorldWant;
	protected bool m_bSubbed;
	protected int m_iSubLayers = -1;
	protected float m_fRecvAt;

	static DCO_CmdViewUI Inst()
	{
		EnsureHooked();
		return s_Inst;
	}

	array<ref DCO_CVObj> Objs()		{ return m_aObjs; }
	array<ref DCO_CVGrp> Grps()		{ return m_aGrps; }
	array<ref DCO_CVDec> Decs()		{ return m_aDecs; }
	array<ref DCO_CVGeo> Geos()		{ return m_aGeos; }
	array<ref DCO_CVCmd> Cmds()		{ return m_aCmds; }
	bool IsDenied()					{ return m_bDenied; }
	float RecvAt()					{ return m_fRecvAt; }

	void SetWorldWant(bool want)
	{
		m_bWorldWant = want;
		UpdateSub();
	}

	int CmdColor(int ci)
	{
		return ColorOf(ci);
	}

	bool CmdVisible(int ci)
	{
		return Visible(ci);
	}

	bool IsAllowedLocal()
	{
		return Allowed();
	}

	protected void UpdateSub()
	{
		bool want = (m_wRoot && m_bOn && m_bAllowed) || m_bWorldWant;
		int layers = m_iLayers;
		if (m_bWorldWant)
			layers = layers | DCO_ECVLayer.PLAN | DCO_ECVLayer.GROUPS;
		if (want == m_bSubbed && (!want || layers == m_iSubLayers))
			return;
		m_bSubbed = want;
		m_iSubLayers = layers;
		SCR_PlayerControllerGroupComponent comp = SCR_PlayerControllerGroupComponent.GetLocalPlayerControllerGroupComponent();
		if (comp)
			comp.DCO_CVSubscribe(want, layers);
	}

	static void EnsureHooked()
	{
		if (System.IsConsoleApp() || s_Inst)
			return;
		s_Inst = new DCO_CmdViewUI();
		SCR_MapEntity.GetOnMapOpen().Insert(s_Inst.OnMapOpen);
		SCR_MapEntity.GetOnMapClose().Insert(s_Inst.OnMapClose);
		DCO_CmdViewWorld.Start();
	}

	static void Receive(array<float> f, array<string> s)
	{
		EnsureHooked();
		if (s_Inst)
			s_Inst.Parse(f, s);
	}

	static void ReceiveDetail(int type, string id, array<string> rows)
	{
		DCO_CmdViewWorld.OnDetail(type, id, rows);
		if (!s_Inst || type != s_Inst.m_iSelType || id != s_Inst.m_sSelId)
			return;
		s_Inst.m_aDetail.Copy(rows);
	}

	static float Now()
	{
		return System.GetTickCount() / 1000.0;
	}

	protected bool Allowed()
	{
#ifdef WORKBENCH
		return true;
#else
		SCR_EditorManagerCore core = SCR_EditorManagerCore.Cast(SCR_EditorManagerCore.GetInstance(SCR_EditorManagerCore));
		if (!core)
			return false;
		SCR_EditorManagerEntity em = core.GetEditorManager();
		return em && !em.IsLimited();
#endif
	}

	protected void OnMapOpen(MapConfiguration config)
	{
		SCR_MapEntity mapEnt = SCR_MapEntity.GetMapInstance();
		if (!mapEnt || !mapEnt.GetMapMenuRoot())
			return;
		Teardown();
		m_bAllowed = Allowed();
		m_fAllowedAt = Now();
		if (!m_bAllowed)
			return;
		m_wRoot = GetGame().GetWorkspace().CreateWidgets("{5C3A9E1B7D4F2A60}UI/layouts/DCO/DCO_CmdView.layout", mapEnt.GetMapMenuRoot());
		if (!m_wRoot)
			return;
		FrameSlot.SetAnchorMin(m_wRoot, 0, 0);
		FrameSlot.SetAnchorMax(m_wRoot, 1, 1);
		FrameSlot.SetOffsets(m_wRoot, 0, 0, 0, 0);
		m_wCanvas = CanvasWidget.Cast(m_wRoot.FindAnyWidget("Canvas"));
		m_wPanel = CanvasWidget.Cast(m_wRoot.FindAnyWidget("PanelCanvas"));
		Listen(true);
		UpdateSub();
		GetGame().GetCallqueue().CallLater(Draw, 0, true);
	}

	protected void OnMapClose(MapConfiguration config)
	{
		Teardown();
		UpdateSub();
	}

	protected void Teardown()
	{
		GetGame().GetCallqueue().Remove(Draw);
		Listen(false);
		m_aLabels.Clear();
		if (m_wRoot)
			m_wRoot.RemoveFromHierarchy();
		m_wRoot = null;
		m_wCanvas = null;
		m_wPanel = null;
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
			im.AddActionListener("MapSelect", EActionTrigger.DOWN, OnClick);
			return;
		}
		im.RemoveActionListener("DCO_IntentOverlay", EActionTrigger.DOWN, OnToggle);
		im.RemoveActionListener("DCO_IntentClose", EActionTrigger.DOWN, OnEsc);
		im.RemoveActionListener("MapSelect", EActionTrigger.DOWN, OnClick);
	}


	protected string CmdList()
	{
		string list = "";
		foreach (DCO_CVCmd c : m_aCmds)
		{
			if (m_aHidden.Contains(c.m_iIdx))
				continue;
			if (!list.IsEmpty())
				list += "|";
			list += c.m_sUid;
		}
		if (list.IsEmpty())
			return "-";
		return list;
	}

	protected string LayerList()
	{
		string l = "";
		if (m_iLayers & DCO_ECVLayer.PLAN)
			l += "plan|";
		if (m_iLayers & DCO_ECVLayer.GROUPS)
			l += "groups|";
		if (m_iLayers & DCO_ECVLayer.ENEMY)
			l += "enemy|";
		if (m_iLayers & DCO_ECVLayer.DECISIONS)
			l += "decisions|";
		if (m_iLayers & DCO_ECVLayer.TRUTH)
			l += "truth|";
		if (l.IsEmpty())
			return "-";
		return l.Substring(0, l.Length() - 1);
	}

	protected void LogState(string why)
	{
		DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", string.Format("intent_overlay open=%1 zoom=%2 commanders=%3 layers=%4 obj=%5 why=%6", DCO_Util.B01(m_bOn), m_iZoom, CmdList(), LayerList(), m_sObjFilterName, why));
	}

	protected void OnToggle(float value = 0.0, EActionTrigger reason = 0)
	{
		if (!m_wRoot || !m_bAllowed)
			return;
		m_bOn = !m_bOn;
		if (!m_bOn)
			ClearSel();
		m_bDenied = false;
		UpdateSub();
		LogState("toggle");
	}

	protected void OnEsc(float value = 0.0, EActionTrigger reason = 0)
	{
		if (m_iSelType != 0)
			ClearSel();
	}

	protected void ClearSel()
	{
		m_iSelType = 0;
		m_sSelId = "";
		m_iSelCi = -1;
		m_iSelKey = -1;
		m_aDetail.Clear();
	}

	protected static int I(float v)
	{
		return Math.Round(v);
	}

	protected static string S(array<string> s, float idx)
	{
		int i = Math.Round(idx);
		if (i < 0 || i >= s.Count())
			return "";
		return s[i];
	}

	protected void Parse(array<float> f, array<string> s)
	{
		if (f.Count() < 3)
			return;
		m_bDenied = (I(f[1]) & 1) != 0;
		m_fRecvAt = Now();
		m_aCmds.Clear();
		m_aObjs.Clear();
		m_aGrps.Clear();
		m_aGeos.Clear();
		m_aClus.Clear();
		m_aTrus.Clear();
		m_aDecs.Clear();
		int i = 3;
		int n = f.Count();
		while (i < n)
		{
			int type = I(f[i]);
			if (type == DCO_ECVRec.CMD && i + 12 < n)
			{
				DCO_CVCmd c = new DCO_CVCmd();
				c.m_iIdx = I(f[i + 1]);
				c.m_sUid = S(s, f[i + 2]);
				c.m_sFac = S(s, f[i + 3]);
				c.m_iColor = 0xFF000000 | I(f[i + 4]);
				c.m_fX = f[i + 5];
				c.m_fY = f[i + 6];
				c.m_fZ = f[i + 7];
				c.m_sTitle = S(s, f[i + 8]);
				c.m_sNow = S(s, f[i + 9]);
				c.m_sPlans = S(s, f[i + 10]);
				c.m_sDecs = S(s, f[i + 11]);
				c.m_sForces = S(s, f[i + 12]);
				m_aCmds.Insert(c);
				i += 13;
			}
			else if (type == DCO_ECVRec.OBJ && i + 11 < n)
			{
				DCO_CVObj o = new DCO_CVObj();
				o.m_iCi = I(f[i + 1]);
				o.m_iKey = I(f[i + 2]);
				o.m_fX = f[i + 3];
				o.m_fZ = f[i + 4];
				o.m_fR = f[i + 5];
				o.m_iDec = I(f[i + 6]);
				o.m_sName = S(s, f[i + 7]);
				o.m_sCard = S(s, f[i + 8]);
				o.m_sTip = S(s, f[i + 9]);
				o.m_iGc = I(f[i + 10]);
				o.m_sSync = S(s, f[i + 11]);
				m_aObjs.Insert(o);
				i += 12;
			}
			else if (type == DCO_ECVRec.GRP && i + 14 < n)
			{
				DCO_CVGrp g = new DCO_CVGrp();
				g.m_iCi = I(f[i + 1]);
				g.m_iKey = I(f[i + 2]);
				g.m_fX = f[i + 3];
				g.m_fZ = f[i + 4];
				g.m_iRole = I(f[i + 5]);
				g.m_iFlags = I(f[i + 6]);
				g.m_fDX = f[i + 7];
				g.m_fDZ = f[i + 8];
				g.m_fAge = f[i + 9];
				g.m_sCs = S(s, f[i + 10]);
				g.m_sId = S(s, f[i + 11]);
				g.m_sTask = S(s, f[i + 12]);
				g.m_fY = f[i + 13];
				g.m_sLine = S(s, f[i + 14]);
				m_aGrps.Insert(g);
				i += 15;
			}
			else if (type == DCO_ECVRec.GEO && i + 9 < n)
			{
				DCO_CVGeo ge = new DCO_CVGeo();
				ge.m_iCi = I(f[i + 1]);
				ge.m_iKey = I(f[i + 2]);
				ge.m_iKind = I(f[i + 3]);
				ge.m_fX0 = f[i + 4];
				ge.m_fZ0 = f[i + 5];
				ge.m_fX1 = f[i + 6];
				ge.m_fZ1 = f[i + 7];
				ge.m_bPast = f[i + 8] > 0.5;
				ge.m_fW = f[i + 9];
				m_aGeos.Insert(ge);
				i += 10;
			}
			else if (type == DCO_ECVRec.CLU && i + 9 < n)
			{
				DCO_CVClu cl = new DCO_CVClu();
				cl.m_iCi = I(f[i + 1]);
				cl.m_fX = f[i + 2];
				cl.m_fZ = f[i + 3];
				cl.m_fR = f[i + 4];
				cl.m_fEst = f[i + 5];
				cl.m_fAge = f[i + 6];
				cl.m_iSrc = I(f[i + 7]);
				cl.m_fConf = f[i + 8];
				cl.m_iId = I(f[i + 9]);
				m_aClus.Insert(cl);
				i += 10;
			}
			else if (type == DCO_ECVRec.TRU && i + 3 < n)
			{
				DCO_CVTru t = new DCO_CVTru();
				t.m_fX = f[i + 1];
				t.m_fZ = f[i + 2];
				t.m_iFi = I(f[i + 3]);
				m_aTrus.Insert(t);
				i += 4;
			}
			else if (type == DCO_ECVRec.DEC && i + 4 < n)
			{
				DCO_CVDec d = new DCO_CVDec();
				d.m_iCi = I(f[i + 1]);
				d.m_sObj = S(s, f[i + 2]);
				d.m_fT = f[i + 3];
				d.m_sText = S(s, f[i + 4]);
				m_aDecs.Insert(d);
				i += 5;
			}
			else
				break;
		}
	}

	protected DCO_CVCmd CmdAt(int ci)
	{
		foreach (DCO_CVCmd c : m_aCmds)
		{
			if (c.m_iIdx == ci)
				return c;
		}
		return null;
	}

	protected int ColorOf(int ci)
	{
		DCO_CVCmd c = CmdAt(ci);
		if (!c)
			return 0xFF888888;
		return c.m_iColor;
	}

	protected bool Visible(int ci)
	{
		return ci < 0 || !m_aHidden.Contains(ci);
	}

	static int Alpha(int color, float a)
	{
		int al = Math.Clamp(Math.Round(a * 255.0), 0, 255);
		return (color & 0x00FFFFFF) | (al << 24);
	}

	static int Lin(int srgb)
	{
		return Color.FromIntSRGB(srgb).PackToInt();
	}

	static void Poly(array<ref CanvasWidgetCommand> cmds, array<float> verts, int color)
	{
		PolygonDrawCommand p = new PolygonDrawCommand();
		p.m_iColor = Lin(color);
		p.m_Vertices = verts;
		cmds.Insert(p);
	}

	static void Line(array<ref CanvasWidgetCommand> cmds, array<float> verts, int color, float width, bool enclose)
	{
		LineDrawCommand l = new LineDrawCommand();
		l.m_iColor = Lin(color);
		l.m_fWidth = width;
		l.m_bShouldEnclose = enclose;
		l.m_Vertices = verts;
		cmds.Insert(l);
	}

	static array<float> Rect(float x0, float y0, float x1, float y1)
	{
		return {x0, y0, x1, y0, x1, y1, x0, y1};
	}

	static array<float> Ellipse(float cx, float cy, float rx, float ry, int n)
	{
		array<float> v = {};
		for (int i = 0; i < n; i++)
		{
			float a = i * Math.PI2 / n;
			v.Insert(cx + Math.Cos(a) * rx);
			v.Insert(cy + Math.Sin(a) * ry);
		}
		return v;
	}

	protected void Dashed(float x0, float y0, float x1, float y1, int color, float width, float dash, float gap)
	{
		float dx = x1 - x0;
		float dy = y1 - y0;
		float len = Math.Sqrt(dx * dx + dy * dy);
		if (len < 1)
			return;
		dx /= len;
		dy /= len;
		float t = 0;
		int guard = 0;
		while (t < len && guard < 400)
		{
			guard++;
			float e = Math.Min(len, t + dash);
			Line(m_aCmd, {x0 + dx * t, y0 + dy * t, x0 + dx * e, y0 + dy * e}, color, width, false);
			t = e + gap;
		}
	}

	protected void DashedCircle(float cx, float cy, float r, int color, float width)
	{
		int n = Math.Clamp(Math.Round(r / 4.0), 8, 48);
		for (int i = 0; i < n; i += 2)
		{
			float a0 = i * Math.PI2 / n;
			float a1 = (i + 1) * Math.PI2 / n;
			Line(m_aCmd, {cx + Math.Cos(a0) * r, cy + Math.Sin(a0) * r, cx + Math.Cos(a1) * r, cy + Math.Sin(a1) * r}, color, width, false);
		}
	}

	protected void Arrow(float x0, float y0, float x1, float y1, int color, float width)
	{
		Line(m_aCmd, {x0, y0, x1, y1}, color, width, false);
		ArrowHead(x0, y0, x1, y1, color);
	}

	protected void ArrowHead(float x0, float y0, float x1, float y1, int color)
	{
		float dx = x1 - x0;
		float dy = y1 - y0;
		float len = Math.Sqrt(dx * dx + dy * dy);
		if (len < 1)
			return;
		dx /= len;
		dy /= len;
		float s = 9.0 * m_fDpi;
		Poly(m_aCmd, {x1, y1, x1 - dx * s * 1.6 - dy * s * 0.7, y1 - dy * s * 1.6 + dx * s * 0.7, x1 - dx * s * 1.6 + dy * s * 0.7, y1 - dy * s * 1.6 - dx * s * 0.7}, color);
	}

	protected bool MapLbl(float sx, float sy, string text, int color, int size, bool force)
	{
		float w = EstW(text, size);
		float h = 1.0 * size * 1.3 * m_fDpi;
		float x0 = sx - w * 0.5;
		if (!force && !Free(x0, sy, x0 + w, sy + h))
			return false;
		Occupy(x0, sy, x0 + w, sy + h);
		Lbl(x0, sy, text, color, size, 0xFFF4F0E2);
		return true;
	}

	protected void Draw()
	{
		SCR_MapEntity mapEnt = SCR_MapEntity.GetMapInstance();
		if (!mapEnt || !mapEnt.IsOpen() || !m_wCanvas || !m_wPanel)
		{
			OnMapClose(null);
			return;
		}
		m_fNow = Now();
		if (m_fNow - m_fAllowedAt > 2.0)
		{
			m_fAllowedAt = m_fNow;
			bool was = m_bAllowed;
			m_bAllowed = Allowed();
			if (was && !m_bAllowed && m_bOn)
			{
				m_bOn = false;
				ClearSel();
				UpdateSub();
				LogState("rights_lost");
			}
		}
		GetGame().GetInputManager().ActivateContext("DCO_CmdViewContext");
		WorkspaceWidget ws = GetGame().GetWorkspace();
		m_fDpi = ws.DPIScale(1000) / 1000.0;
		float sw = 0;
		float sh = 0;
		m_wCanvas.GetScreenSize(sw, sh);
		m_fSw = sw;
		m_fSh = sh;
		m_aCmd.Clear();
		m_aPanelCmd.Clear();
		m_aHits.Clear();
		m_aOcc.Clear();
		m_iLabel = 0;
		m_fPanelX0 = -1;

		DrawStrip();
		if (m_bOn && m_bAllowed)
		{
			float ppm = mapEnt.GetCurrentZoom();
			int z = 3;
			if (ppm < 0.35)
				z = 1;
			else if (ppm < 1.5)
				z = 2;
			if (z != m_iZoom)
			{
				m_iZoom = z;
				LogState("zoom");
			}
			if (m_iSelType != 0)
			{
				m_fPanelX0 = m_fSw - 456.0 * m_fDpi;
				Occupy(m_fPanelX0, 0, m_fSw, m_fSh);
				if (m_fNow - m_fDetailAt > 4.0)
					RequestDetail();
			}
			float tlTop = m_fSh;
			if ((m_iLayers & DCO_ECVLayer.DECISIONS) != 0)
			{
				tlTop = m_fSh - (16.0 + 8.0 * 22.0 + 16.0) * m_fDpi;
				float tlRight = m_fSw - 16.0 * m_fDpi;
				if (m_fPanelX0 > 0)
					tlRight = m_fPanelX0 - 16.0 * m_fDpi;
				Occupy(0, tlTop, tlRight, m_fSh);
			}
			float cy = 52.0 * m_fDpi;
			foreach (DCO_CVCmd cc : m_aCmds)
			{
				if (!Visible(cc.m_iIdx))
					continue;
				string ccid = cc.m_iIdx.ToString() + ":cmd";
				cy += DrawCmdCard(cc, 16.0 * m_fDpi, cy, false, m_iSelType == 4 && m_sSelId == ccid) + 6.0 * m_fDpi;
			}
			DrawMap(mapEnt, ppm);
			if (m_iSelType != 0)
				DrawPanel();
			if ((m_iLayers & DCO_ECVLayer.DECISIONS) != 0)
				DrawTimeline(tlTop);
			DrawTip();
		}

		for (int i = m_iLabel; i < m_aLabels.Count(); i++)
			m_aLabels[i].SetVisible(false);
		m_wCanvas.SetDrawCommands(m_aCmd);
		m_wPanel.SetDrawCommands(m_aPanelCmd);
	}

	protected float Chip(float x, float y, string text, int bg, int fg, int underline, int type, int ci, string id)
	{
		int size = 16;
		float w = EstW(text, size) + 24.0 * m_fDpi;
		float h = 30.0 * m_fDpi;
		Poly(m_aPanelCmd, Rect(x, y, x + w, y + h), bg);
		if (underline != 0)
			Poly(m_aPanelCmd, Rect(x, y + h - 3.0 * m_fDpi, x + w, y + h), underline);
		Lbl(x + 12.0 * m_fDpi, y + 5.0 * m_fDpi, text, fg, size, 0);
		Hit(type, ci, id, x, y, x + w, y + h, "", 0, 0, 0);
		return x + w + 4.0 * m_fDpi;
	}

	protected void DrawStrip()
	{
		if (!m_bAllowed)
			return;
		float x = 16.0 * m_fDpi;
		float y = 12.0 * m_fDpi;
		float h = 30.0 * m_fDpi;
		int on = 0xFFE3A13B;
		int off = 0xF0262B2F;
		string t = "INTENT OFF · Ctrl+I";
		int bg = off;
		if (m_bOn)
		{
			t = "INTENT ON · Ctrl+I";
			bg = on;
		}
		x = Chip(x, y, t, bg, 0xFFFFFFFF, 0, 10, -1, "");
		if (!m_bOn)
		{
			Occupy(0, 0, x, y + h);
			return;
		}
		if (m_bDenied)
		{
			x = Chip(x, y, "Disabled by server setting", off, 0xFF9AA0A5, 0, 0, -1, "");
			Occupy(0, 0, x, y + h);
			return;
		}
		foreach (DCO_CVCmd c : m_aCmds)
		{
			bool vis = Visible(c.m_iIdx);
			int fg = 0xFFFFFFFF;
			int ul = c.m_iColor;
			if (!vis)
			{
				fg = 0xFF9AA0A5;
				ul = 0;
			}
			x = Chip(x, y, c.m_sFac + " · " + c.m_sUid, off, fg, ul, 11, c.m_iIdx, "");
		}
		x = Chip(x, y, "Objective: " + m_sObjFilterName + " >", off, 0xFFFFFFFF, 0, 12, -1, "");
		x = LayerChip(x, y, "Plan lines", DCO_ECVLayer.PLAN);
		x = LayerChip(x, y, "Groups", DCO_ECVLayer.GROUPS);
		x = LayerChip(x, y, "Enemy picture", DCO_ECVLayer.ENEMY);
		x = LayerChip(x, y, "Decisions", DCO_ECVLayer.DECISIONS);
		x = LayerChip(x, y, "Truth (dev)", DCO_ECVLayer.TRUTH);
		Occupy(0, 0, x, y + h);
	}

	protected float LayerChip(float x, float y, string text, int bit)
	{
		bool on = (m_iLayers & bit) != 0;
		int fg = 0xFF9AA0A5;
		int ul = 0;
		if (on)
		{
			fg = 0xFFFFFFFF;
			ul = 0xFFFFFFFF;
		}
		return Chip(x, y, text, 0xF0262B2F, fg, ul, 13, -1, bit.ToString());
	}

	protected bool Dimmed(int ci, int key)
	{
		if (m_sObjFilter.IsEmpty())
			return false;
		return m_sObjFilter != ci.ToString() + ":" + key.ToString();
	}

	protected bool ObjSelected(int ci, int key)
	{
		if (m_iSelType == 1 && m_iSelCi == ci && m_iSelKey == key)
			return true;
		return !m_sObjFilter.IsEmpty() && m_sObjFilter == ci.ToString() + ":" + key.ToString();
	}

	protected void DrawMap(SCR_MapEntity mapEnt, float ppm)
	{
		if ((m_iLayers & DCO_ECVLayer.TRUTH) != 0)
		{
			foreach (DCO_CVTru tr : m_aTrus)
			{
				int tx = 0;
				int ty = 0;
				mapEnt.WorldToScreen(tr.m_fX, tr.m_fZ, tx, ty, true);
				float tr4 = 4.0 * m_fDpi;
				Poly(m_aCmd, Ellipse(tx, ty, tr4 + 1.5, tr4 + 1.5, 10), 0xFFFFFFFF);
				Poly(m_aCmd, Ellipse(tx, ty, tr4, tr4, 10), 0xFF555555);
			}
		}

		if ((m_iLayers & DCO_ECVLayer.ENEMY) != 0)
		{
			foreach (DCO_CVClu cl : m_aClus)
			{
				if (!Visible(cl.m_iCi))
					continue;
				int cx = 0;
				int cy = 0;
				mapEnt.WorldToScreen(cl.m_fX, cl.m_fZ, cx, cy, true);
				float rx = Math.Max(cl.m_fR * ppm, 14.0 * m_fDpi);
				float ry = rx * 0.7;
				float a = Math.Clamp(1.0 - cl.m_fAge / 600.0, 0.35, 1.0);
				int col = ColorOf(cl.m_iCi);
				Poly(m_aCmd, Ellipse(cx, cy, rx, ry, 28), Alpha(col, 0.12 * a));
				for (int hk = -2; hk <= 2; hk++)
				{
					float off = 1.0 * hk * rx * 0.35;
					float half = Math.Sqrt(Math.Max(0, 1.0 - (off / rx) * (off / rx))) * ry * 0.9;
					Line(m_aCmd, {cx + off - half * 0.5, cy + half, cx + off + half * 0.5, cy - half}, Alpha(col, 0.35 * a), 2, false);
				}
				array<float> ev = Ellipse(cx, cy, rx, ry, 28);
				for (int e = 0; e + 3 < ev.Count(); e += 4)
					Line(m_aCmd, {ev[e], ev[e + 1], ev[e + 2], ev[e + 3]}, Alpha(col, a), 2.5, false);
				int est = Math.Round(cl.m_fEst);
				string ctip = "~" + est.ToString() + " enemy · " + DCO_CmdViewStore.Clock(cl.m_fAge) + " old\nWhat " + CmdName(cl.m_iCi) + " believes";
				string cid = cl.m_iCi.ToString() + ":" + cl.m_iId.ToString();
				bool csel = m_iSelType == 3 && m_sSelId == cid;
				if (m_iZoom >= 2 || csel)
				{
					string ct = "~" + est.ToString();
					if (m_iZoom >= 3 || csel)
						ct += " · " + DCO_CmdViewStore.Clock(cl.m_fAge);
					MapLbl(cx, cy - ry - 24.0 * m_fDpi, ct, col, 18, csel);
				}
				Hit(3, cl.m_iCi, cid, cx - rx, cy - ry, cx + rx, cy + ry, ctip, col, cl.m_fX, cl.m_fZ);
			}
		}

		if ((m_iLayers & DCO_ECVLayer.PLAN) != 0)
		{
			foreach (DCO_CVGeo ge : m_aGeos)
			{
				if (!Visible(ge.m_iCi))
					continue;
				bool show = ObjSelected(ge.m_iCi, ge.m_iKey) || (ge.m_iKind == DCO_ECVGeo.LZ && m_iZoom >= 3);
				if (!show)
					continue;
				DrawGeo(mapEnt, ge, ppm);
			}
		}

		if ((m_iLayers & DCO_ECVLayer.GROUPS) != 0 && m_iZoom >= 2)
		{
			foreach (DCO_CVGrp g : m_aGrps)
			{
				if (!Visible(g.m_iCi))
					continue;
				DrawGroup(mapEnt, g);
			}
		}

		map<int, int> stack = new map<int, int>();
		foreach (DCO_CVObj o : m_aObjs)
		{
			if (!Visible(o.m_iCi))
				continue;
			int ox = 0;
			int oy = 0;
			mapEnt.WorldToScreen(o.m_fX, o.m_fZ, ox, oy, true);
			int col = ColorOf(o.m_iCi);
			bool dim = Dimmed(o.m_iCi, o.m_iKey);
			bool sel = ObjSelected(o.m_iCi, o.m_iKey);
			float al = 1.0;
			if (dim && !sel)
				al = 0.35;
			int st = stack.Get(o.m_iKey);
			stack.Set(o.m_iKey, st + 1);
			if (st == 0)
				DashedCircle(ox, oy, Math.Max(o.m_fR * ppm, 10.0 * m_fDpi), Alpha(0xFF2B2F33, al), 2.0);
			if (sel)
				MapLbl(ox, oy + Math.Max(o.m_fR * ppm, 10.0 * m_fDpi) + 4.0 * m_fDpi, "OBJ " + o.m_sName, col, 18, true);
			int bar = col;
			if (o.m_iDec == 3)
				bar = 0xFF6B6B6B;
			float cw = EstW(o.m_sCard, 18) + 28.0 * m_fDpi;
			float ch = 28.0 * m_fDpi;
			float cx0 = ox - cw * 0.5;
			float cy0 = oy - Math.Max(o.m_fR * ppm, 10.0 * m_fDpi) - (st + 1) * (ch + 4.0 * m_fDpi);
			Poly(m_aCmd, Rect(cx0, cy0, cx0 + cw, cy0 + ch), Alpha(0xFFFFFFFF, al));
			Poly(m_aCmd, Rect(cx0, cy0, cx0 + 6.0 * m_fDpi, cy0 + ch), Alpha(bar, al));
			if (sel)
				Line(m_aCmd, Rect(cx0, cy0, cx0 + cw, cy0 + ch), bar, 3, true);
			Occupy(cx0, cy0, cx0 + cw, cy0 + ch);
			if (m_fPanelX0 < 0 || cx0 + cw < m_fPanelX0)
				Lbl(cx0 + 14.0 * m_fDpi, cy0 + 3.0 * m_fDpi, o.m_sCard, Alpha(0xFF23272A, al), 18, 0);
			string oid = o.m_iCi.ToString() + ":" + o.m_iKey.ToString();
			Hit(1, o.m_iCi, oid, cx0, cy0, cx0 + cw, cy0 + ch, o.m_sTip, col, o.m_fX, o.m_fZ);
			float hr = 12.0 * m_fDpi;
			Hit(1, o.m_iCi, oid, ox - hr, oy - hr, ox + hr, oy + hr, o.m_sTip, col, o.m_fX, o.m_fZ);
			if (m_iZoom == 1 && (m_iLayers & DCO_ECVLayer.GROUPS) != 0 && o.m_iGc > 0)
			{
				float bx = ox - 40.0 * m_fDpi + st * 30.0 * m_fDpi;
				float by = oy + 30.0 * m_fDpi;
				float br = 13.0 * m_fDpi;
				Poly(m_aCmd, Ellipse(bx, by, br + 2, br + 2, 16), 0xFFFFFFFF);
				Poly(m_aCmd, Ellipse(bx, by, br, br, 16), col);
				if (m_fPanelX0 < 0 || bx < m_fPanelX0)
					Lbl(bx - EstW(o.m_iGc.ToString(), 16) * 0.5, by - 10.0 * m_fDpi, o.m_iGc.ToString(), 0xFFFFFFFF, 16, 0);
			}
		}
	}

	string CmdName(int ci)
	{
		DCO_CVCmd c = CmdAt(ci);
		if (!c)
			return "-";
		return c.m_sUid;
	}

	protected void DrawGeo(SCR_MapEntity mapEnt, DCO_CVGeo ge, float ppm)
	{
		int ax, ay, bx, by;
		mapEnt.WorldToScreen(ge.m_fX0, ge.m_fZ0, ax, ay, true);
		mapEnt.WorldToScreen(ge.m_fX1, ge.m_fZ1, bx, by, true);
		int col = ColorOf(ge.m_iCi);
		if (ge.m_bPast)
			col = Alpha(col, 0.5);
		float d = m_fDpi;
		switch (ge.m_iKind)
		{
			case DCO_ECVGeo.AXIS:
			{
				float dx = bx - ax;
				float dy = by - ay;
				float len = Math.Sqrt(dx * dx + dy * dy);
				if (len < 2)
					return;
				float nx = -dy / len;
				float ny = dx / len;
				if (ge.m_fW >= 2)
				{
					Dashed(ax + nx * 3.0 * d, ay + ny * 3.0 * d, bx + nx * 3.0 * d, by + ny * 3.0 * d, col, 3, 3.0 * d, 7.0 * d);
					Dashed(ax - nx * 3.0 * d, ay - ny * 3.0 * d, bx - nx * 3.0 * d, by - ny * 3.0 * d, col, 3, 3.0 * d, 7.0 * d);
				}
				else
					Dashed(ax, ay, bx, by, col, 3, 3.0 * d, 7.0 * d);
				ArrowHead(ax, ay, bx, by, col);
				return;
			}
			case DCO_ECVGeo.STAGING:
			{
				float s = 13.0 * d;
				Line(m_aCmd, {ax - s, ay + s * 0.8, ax + s, ay + s * 0.8, ax, ay - s}, col, 3, true);
				MapLbl(ax, ay + s + 2.0 * d, "AA", col, 18, true);
				return;
			}
			case DCO_ECVGeo.ATTACK:
			{
				Line(m_aCmd, Ellipse(ax, ay, 11.0 * d, 11.0 * d, 20), col, 3, true);
				MapLbl(ax, ay - 36.0 * d, "ATK", col, 18, true);
				return;
			}
			case DCO_ECVGeo.SBF:
			{
				Dashed(ax, ay, bx, by, col, 2.5, 12.0 * d, 6.0 * d);
				float sdx = bx - ax;
				float sdy = by - ay;
				float slen = Math.Sqrt(sdx * sdx + sdy * sdy);
				if (slen > 2)
				{
					float px = -sdy / slen;
					float py = sdx / slen;
					float w = ge.m_fW * ppm;
					Line(m_aCmd, {ax, ay, bx + px * w, by + py * w}, Alpha(col, 0.6), 1.5, false);
					Line(m_aCmd, {ax, ay, bx - px * w, by - py * w}, Alpha(col, 0.6), 1.5, false);
				}
				MapLbl((ax + bx) * 0.5, (ay + by) * 0.5 - 24.0 * d, "SBF", col, 18, true);
				return;
			}
			case DCO_ECVGeo.RALLY:
			{
				float r = 8.0 * d;
				Poly(m_aCmd, {ax - r, ay + r, ax + r, ay + r, ax, ay - r}, col);
				MapLbl(ax, ay + r + 2.0 * d, "RP", col, 18, true);
				return;
			}
			case DCO_ECVGeo.LZ:
			{
				Line(m_aCmd, Ellipse(ax, ay, 10.0 * d, 10.0 * d, 20), col, 2.5, true);
				MapLbl(ax, ay + 12.0 * d, "LZ", col, 18, false);
				return;
			}
			case DCO_ECVGeo.TRIGGER:
			{
				Dashed(ax, ay, bx, by, col, 3, 18.0 * d, 9.0 * d);
				MapLbl(bx, by + 6.0 * d, "TRIGGER", col, 18, true);
				return;
			}
		}
	}

	static void RoleShape(array<ref CanvasWidgetCommand> cmds, int role, float x, float y, int col, float s)
	{
		float o = s + 2.0;
		switch (role)
		{
			case 0:
			{
				Poly(cmds, Rect(x - o, y - o, x + o, y + o), 0xFFFFFFFF);
				Poly(cmds, Rect(x - s, y - s, x + s, y + s), col);
				return;
			}
			case 1:
			{
				Poly(cmds, {x, y - o * 1.25, x + o * 1.25, y + o, x - o * 1.25, y + o}, 0xFFFFFFFF);
				Poly(cmds, {x, y - s * 1.2, x + s * 1.2, y + s, x - s * 1.2, y + s}, col);
				return;
			}
			case 2:
			{
				Poly(cmds, Rect(x - s * 0.9, y - s * 0.8, x + s * 0.9, y + s * 0.8), 0xE6F4F0E2);
				Line(cmds, {x - s * 0.5, y - s, x - s * 1.2, y - s, x - s * 1.2, y + s, x - s * 0.5, y + s}, col, 4, false);
				Line(cmds, {x + s * 0.5, y - s, x + s * 1.2, y - s, x + s * 1.2, y + s, x + s * 0.5, y + s}, col, 4, false);
				return;
			}
			case 3:
			{
				Poly(cmds, {x, y - o * 1.3, x + o * 1.3, y, x, y + o * 1.3, x - o * 1.3, y}, 0xFFFFFFFF);
				Poly(cmds, {x, y - s * 1.3, x + s * 1.3, y, x, y + s * 1.3, x - s * 1.3, y}, col);
				return;
			}
			case 5:
			{
				Poly(cmds, Ellipse(x, y, o * 1.2, o * 1.2, 6), 0xFFFFFFFF);
				Poly(cmds, Ellipse(x, y, s * 1.2, s * 1.2, 6), col);
				return;
			}
			case 6:
			{
				Poly(cmds, Ellipse(x, y, o * 1.6, o * 0.9, 12), 0xFFFFFFFF);
				Poly(cmds, Ellipse(x, y, s * 1.6, s * 0.9, 12), col);
				return;
			}
			case 7:
			{
				array<float> arc = {};
				for (int i = 0; i <= 10; i++)
				{
					float a = Math.PI + 1.0 * i * Math.PI / 10.0;
					arc.Insert(x + Math.Cos(a) * s);
					arc.Insert(y + Math.Sin(a) * s);
				}
				arc.Insert(x + s);
				arc.Insert(y + s);
				arc.InsertAt(y + s, 0);
				arc.InsertAt(x - s, 0);
				Line(cmds, arc, 0xFFFFFFFF, 7, false);
				Line(cmds, arc, col, 4, false);
				return;
			}
		}
		Poly(cmds, Ellipse(x, y, o * 1.15, o * 1.15, 16), 0xFFFFFFFF);
		Poly(cmds, Ellipse(x, y, s * 1.15, s * 1.15, 16), col);
	}

	protected void DrawGroup(SCR_MapEntity mapEnt, DCO_CVGrp g)
	{
		int gx, gy;
		mapEnt.WorldToScreen(g.m_fX, g.m_fZ, gx, gy, true);
		int col = ColorOf(g.m_iCi);
		string gid = g.m_iCi.ToString() + ":" + g.m_sId;
		bool sel = m_iSelType == 2 && m_sSelId == gid;
		bool hl = sel || (m_iSelType == 1 && m_iSelCi == g.m_iCi && m_iSelKey == g.m_iKey && g.m_iKey >= 0);
		if ((g.m_iFlags & 4) != 0)
		{
			int dx, dy;
			mapEnt.WorldToScreen(g.m_fDX, g.m_fDZ, dx, dy, true);
			if (Math.AbsFloat(dx - gx) + Math.AbsFloat(dy - gy) > 24.0 * m_fDpi)
			{
				float w = 2.0;
				int ac = Alpha(col, 0.6);
				if (hl)
				{
					w = 3.5;
					ac = col;
				}
				Arrow(gx, gy, dx, dy, ac, w);
			}
		}
		if (hl)
			DashedCircle(gx, gy, 26.0 * m_fDpi, 0xFF111111, 2.0);
		if ((g.m_iFlags & 1) != 0)
			Line(m_aCmd, Ellipse(gx, gy, 19.0 * m_fDpi, 19.0 * m_fDpi, 20), 0xFFD0101A, 2.5, true);
		RoleShape(m_aCmd, g.m_iRole, gx, gy, col, 9.0 * m_fDpi);
		string letter = DCO_CmdViewServer.RoleLetter(g.m_iRole);
		int lc = 0xFFFFFFFF;
		if (g.m_iRole == 2 || g.m_iRole == 7)
			lc = col;
		int ls = 14;
		if (letter.Length() > 1)
			ls = 11;
		if (m_fPanelX0 < 0 || gx < m_fPanelX0)
			Lbl(gx - EstW(letter, ls) * 0.5, gy - 1.0 * ls * 0.62 * m_fDpi, letter, lc, ls, 0);
		string age = "-";
		if (g.m_fAge >= 0)
			age = DCO_CmdViewStore.Clock(g.m_fAge);
		if (m_iZoom >= 3 || hl)
			MapLbl(gx, gy + 18.0 * m_fDpi, g.m_sCs + " · " + age, col, 18, sel);
		string tip = g.m_sCs + " · " + DCO_CmdViewServer.RoleName(g.m_iRole) + "\n" + g.m_sTask + " · order " + age;
		float r = 15.0 * m_fDpi;
		Hit(2, g.m_iCi, gid, gx - r, gy - r, gx + r, gy + r, tip, col, g.m_fX, g.m_fZ);
	}

	protected void DrawPanel()
	{
		if (DCO_CmdViewWorld.PanelOpen())
			return;
		float d = m_fDpi;
		DrawRows(m_aDetail, m_fPanelX0, 52.0 * d, m_fSw - 16.0 * d, m_fSh - 16.0 * d, ColorOf(m_iSelCi), m_iSelCi);
	}

	protected void DrawTimeline(float top)
	{
		float d = m_fDpi;
		float x0 = 16.0 * d;
		float x1 = m_fSw - 16.0 * d;
		if (m_fPanelX0 > 0)
			x1 = m_fPanelX0 - 16.0 * d;
		float y1 = m_fSh - 16.0 * d;
		Poly(m_aPanelCmd, Rect(x0, top, x1, y1), 0xD915181A);
		float y = top + 8.0 * d;
		int shown = 0;
		int maxChars = Math.Max(20, Math.Round((x1 - x0) / (16.0 * 0.52 * d)) - 4);
		foreach (DCO_CVDec dc : m_aDecs)
		{
			if (shown >= 8)
				break;
			if (!Visible(dc.m_iCi))
				continue;
			string line = "T+" + DCO_CmdViewStore.Clock(dc.m_fT) + "  " + CmdName(dc.m_iCi) + "  " + dc.m_sObj + "  " + dc.m_sText;
			if (line.Length() > maxChars)
				line = line.Substring(0, maxChars) + "...";
			Lbl(x0 + 12.0 * d, y, line, 0xFFC9CCD0, 16, 0);
			Hit(21, dc.m_iCi, dc.m_iCi.ToString() + ":" + dc.m_sObj, x0, y, x1, y + 22.0 * d, "", 0, 0, 0);
			y += 22.0 * d;
			shown++;
		}
		if (shown == 0)
			Lbl(x0 + 12.0 * d, y, "No decisions recorded yet.", 0xFF9AA0A5, 16, 0);
	}

	protected DCO_CVHit HitAt(float mx, float my, bool mapOnly)
	{
		DCO_CVHit best = null;
		float bestD = float.MAX;
		foreach (DCO_CVHit h : m_aHits)
		{
			if (mx < h.m_fX0 || mx > h.m_fX1 || my < h.m_fY0 || my > h.m_fY1)
				continue;
			if (h.m_iType >= 10)
			{
				if (mapOnly)
					continue;
				return h;
			}
			if (m_fPanelX0 > 0 && mx >= m_fPanelX0)
				continue;
			float cx = (h.m_fX0 + h.m_fX1) * 0.5;
			float cy = (h.m_fY0 + h.m_fY1) * 0.5;
			float dd = (cx - mx) * (cx - mx) + (cy - my) * (cy - my);
			if (h.m_iType == 2)
				dd *= 0.5;
			if (dd < bestD)
			{
				bestD = dd;
				best = h;
			}
		}
		return best;
	}

	protected void DrawTip()
	{
		int mx, my;
		WidgetManager.GetMousePos(mx, my);
		if (m_fPanelX0 > 0 && mx >= m_fPanelX0)
			return;
		DCO_CVHit h = HitAt(mx, my, true);
		if (!h || h.m_sTip.IsEmpty())
			return;
		array<string> lines = {};
		h.m_sTip.Split("\n", lines, false);
		string l1 = lines[0];
		string l2 = "";
		if (lines.Count() > 1)
			l2 = lines[1];
		float d = m_fDpi;
		float w = Math.Max(EstW(l1, 18), EstW(l2, 16)) + 24.0 * d;
		w = Math.Min(w, 420.0 * d);
		float x = mx + 18.0 * d;
		float y = my + 18.0 * d;
		if (x + w > m_fSw)
			x = mx - w - 12.0 * d;
		float hgt = 30.0 * d;
		if (!l2.IsEmpty())
			hgt = 54.0 * d;
		Poly(m_aPanelCmd, Rect(x, y, x + w, y + hgt), 0xF2181B1E);
		Poly(m_aPanelCmd, Rect(x, y, x + 3.0 * d, y + hgt), h.m_iColor);
		int cap = Math.Max(10, Math.Round((w - 24.0 * d) / (18.0 * 0.52 * d)));
		if (l1.Length() > cap)
			l1 = l1.Substring(0, cap - 3) + "...";
		Lbl(x + 12.0 * d, y + 4.0 * d, l1, 0xFFF1F1EF, 18, 0);
		if (!l2.IsEmpty())
		{
			int cap2 = Math.Max(10, Math.Round((w - 24.0 * d) / (16.0 * 0.52 * d)));
			if (l2.Length() > cap2)
				l2 = l2.Substring(0, cap2 - 3) + "...";
			Lbl(x + 12.0 * d, y + 29.0 * d, l2, 0xFF9AA0A5, 16, 0);
		}
	}

	protected void OnClick(float value = 0.0, EActionTrigger reason = 0)
	{
		if (!m_wRoot || !m_bAllowed)
			return;
		int mx, my;
		WidgetManager.GetMousePos(mx, my);
		DCO_CVHit h = HitAt(mx, my, false);
		if (!h)
			return;
		if (!m_bOn && h.m_iType != 10)
			return;
		switch (h.m_iType)
		{
			case 10:
			{
				OnToggle();
				return;
			}
			case 11:
			{
				int hi = m_aHidden.Find(h.m_iCi);
				if (hi >= 0)
					m_aHidden.Remove(hi);
				else
					m_aHidden.Insert(h.m_iCi);
				if (m_iSelCi == h.m_iCi && hi < 0)
					ClearSel();
				LogState("commander_filter");
				return;
			}
			case 12:
			{
				CycleObjFilter();
				LogState("objective_filter");
				return;
			}
			case 13:
			{
				int bit = h.m_sId.ToInt();
				m_iLayers = m_iLayers ^ bit;
				UpdateSub();
				LogState("layer");
				return;
			}
			case 20:
			{
				Select(2, h.m_iCi, h.m_sId, true);
				return;
			}
			case 22:
			{
				Select(1, h.m_iCi, h.m_sId, true);
				return;
			}
			case 21:
			{
				int c = h.m_sId.IndexOf(":");
				string oname = h.m_sId.Substring(c + 1, h.m_sId.Length() - c - 1);
				foreach (DCO_CVObj o : m_aObjs)
				{
					if (o.m_iCi == h.m_iCi && o.m_sName == oname)
					{
						Select(1, o.m_iCi, o.m_iCi.ToString() + ":" + o.m_iKey.ToString(), true);
						return;
					}
				}
				foreach (DCO_CVObj o2 : m_aObjs)
				{
					if (o2.m_sName == oname)
					{
						Select(1, o2.m_iCi, o2.m_iCi.ToString() + ":" + o2.m_iKey.ToString(), true);
						return;
					}
				}
				return;
			}
		}
		if (m_iSelType == h.m_iType && m_sSelId == h.m_sId)
		{
			DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", string.Format("intent_select type=%1 id=%2 action=close", TypeName(h.m_iType), h.m_sId));
			ClearSel();
			return;
		}
		Select(h.m_iType, h.m_iCi, h.m_sId, false);
	}

	protected static string TypeName(int t)
	{
		switch (t)
		{
			case 1: return "obj";
			case 2: return "grp";
			case 3: return "clu";
			case 4: return "cmd";
		}
		return "decision";
	}

	protected void Select(int type, int ci, string id, bool pan)
	{
		m_iSelType = type;
		m_iSelCi = ci;
		m_sSelId = id;
		m_iSelKey = -1;
		if (type == 1)
		{
			int c = id.IndexOf(":");
			m_iSelKey = id.Substring(c + 1, id.Length() - c - 1).ToInt();
		}
		m_aDetail.Clear();
		RequestDetail();
		DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", string.Format("intent_select type=%1 id=%2 commander=%3 view=map", TypeName(type), id, CmdName(ci)));
		if (type != 4)
			PanTo(type, id, pan);
	}

	protected void PanTo(int type, string id, bool force)
	{
		SCR_MapEntity mapEnt = SCR_MapEntity.GetMapInstance();
		if (!mapEnt)
			return;
		float wx = 0;
		float wz = 0;
		bool found = false;
		foreach (DCO_CVHit h : m_aHits)
		{
			if (h.m_iType == type && h.m_sId == id)
			{
				wx = h.m_fWX;
				wz = h.m_fWZ;
				found = true;
				break;
			}
		}
		if (!found && type == 2)
		{
			int c = id.IndexOf(":");
			string gid = id.Substring(c + 1, id.Length() - c - 1);
			foreach (DCO_CVGrp g : m_aGrps)
			{
				if (g.m_sId == gid)
				{
					wx = g.m_fX;
					wz = g.m_fZ;
					found = true;
				}
			}
		}
		if (!found)
			return;
		int sx, sy;
		mapEnt.WorldToScreen(wx, wz, sx, sy, true);
		float panelX0 = m_fSw - 456.0 * m_fDpi;
		if (!force && sx < panelX0 - 24.0 * m_fDpi)
			return;
		float ppm = mapEnt.GetCurrentZoom();
		if (ppm <= 0)
			return;
		float cx, cz;
		mapEnt.GetMapCenterWorldPosition(cx, cz);
		float targetSx = panelX0 * 0.5;
		float newCx = wx + (m_fSw * 0.5 - targetSx) / ppm;
		float newCz = cz;
		if (force)
			newCz = wz;
		mapEnt.ZoomPanSmooth(ppm, newCx, newCz, 0.3);
	}

	protected void RequestDetail()
	{
		m_fDetailAt = Now();
		SCR_PlayerControllerGroupComponent comp = SCR_PlayerControllerGroupComponent.GetLocalPlayerControllerGroupComponent();
		if (comp && m_iSelType != 0)
			comp.DCO_CVRequestDetail(m_iSelType, m_sSelId);
	}

	protected void CycleObjFilter()
	{
		array<DCO_CVObj> list = {};
		foreach (DCO_CVObj o : m_aObjs)
		{
			if (Visible(o.m_iCi))
				list.Insert(o);
		}
		if (list.IsEmpty())
		{
			m_sObjFilter = "";
			m_sObjFilterName = "All";
			return;
		}
		int at = -1;
		foreach (int i, DCO_CVObj lo : list)
		{
			if (m_sObjFilter == lo.m_iCi.ToString() + ":" + lo.m_iKey.ToString())
				at = i;
		}
		at++;
		if (at >= list.Count())
		{
			m_sObjFilter = "";
			m_sObjFilterName = "All";
			return;
		}
		m_sObjFilter = list[at].m_iCi.ToString() + ":" + list[at].m_iKey.ToString();
		m_sObjFilterName = list[at].m_sName + " (" + CmdName(list[at].m_iCi) + ")";
	}
}
