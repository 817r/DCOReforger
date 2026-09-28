class DCO_MapOverlay
{
	protected static const ResourceName LAYOUT = "{6C7254EC4F560CAC}UI/layouts/DCO/DCO_MapOverlay.layout";
	protected static const ResourceName LABEL_LAYOUT = "{0B50B84909DB2710}UI/layouts/DCO/DCO_MapLabel.layout";
	protected static const float SYMBOL_PX = 9;
	protected static const int CIRCLE_SEGMENTS = 28;

	protected static ref DCO_MapOverlay s_Instance;

	protected ref array<float> m_aGroups = {};
	protected ref array<float> m_aObjectives = {};
	protected Widget m_wRoot;
	protected CanvasWidget m_wCanvas;
	protected ref array<Widget> m_aLabels = {};
	protected ref array<ref CanvasWidgetCommand> m_aCommands = {};
	protected int m_iPalette;
	protected int m_iOutline;

	static void SetData(array<float> groups, array<float> objectives)
	{
		if (System.IsConsoleApp())
			return;

		if (!s_Instance)
		{
			s_Instance = new DCO_MapOverlay();
			SCR_MapEntity.GetOnMapOpen().Insert(s_Instance.OnMapOpen);
			SCR_MapEntity.GetOnMapClose().Insert(s_Instance.OnMapClose);
		}

		s_Instance.m_aGroups.Copy(groups);
		s_Instance.m_aObjectives.Copy(objectives);
	}

	protected void OnMapOpen(MapConfiguration config)
	{
		SCR_MapEntity mapEnt = SCR_MapEntity.GetMapInstance();
		if (!mapEnt || !mapEnt.GetMapMenuRoot())
			return;

		OnMapClose(config);
		m_wRoot = GetGame().GetWorkspace().CreateWidgets(LAYOUT, mapEnt.GetMapMenuRoot());
		if (!m_wRoot)
			return;

		FrameSlot.SetAnchorMin(m_wRoot, 0, 0);
		FrameSlot.SetAnchorMax(m_wRoot, 1, 1);
		FrameSlot.SetOffsets(m_wRoot, 0, 0, 0, 0);
		m_wCanvas = CanvasWidget.Cast(m_wRoot.FindAnyWidget("Canvas"));
		GetGame().GetCallqueue().CallLater(Draw, 0, true);
	}

	protected void OnMapClose(MapConfiguration config)
	{
		GetGame().GetCallqueue().Remove(Draw);
		m_aLabels.Clear();
		if (m_wRoot)
			m_wRoot.RemoveFromHierarchy();
		m_wRoot = null;
		m_wCanvas = null;
	}

	protected void Draw()
	{
		SCR_MapEntity mapEnt = SCR_MapEntity.GetMapInstance();
		if (!mapEnt || !mapEnt.IsOpen() || !m_wCanvas)
		{
			OnMapClose(null);
			return;
		}

		m_aCommands.Clear();
		m_iPalette = DCO_UISettings.GetMapPalette();
		m_iOutline = OutlineColor(m_iPalette);
		int label;
		WorkspaceWidget ws = GetGame().GetWorkspace();

		int stride = DCO_PlayerAwareness.OBJ_STRIDE;
		for (int i = 0; i + stride <= m_aObjectives.Count(); i += stride)
		{
			int sx, sy;
			mapEnt.WorldToScreen(m_aObjectives[i], m_aObjectives[i + 1], sx, sy, true);
			int rel = m_aObjectives[i + 3];
			if (rel == DCO_PlayerAwareness.REL_STAGING)
			{
				AddSymbol(Triangle(sx, sy, SYMBOL_PX + 2, -1), Triangle(sx, sy, SYMBOL_PX, -1), StagingColor(m_iPalette));
				label = SetLabel(label, ws, sx, sy + SYMBOL_PX, "STAGING", StagingColor(m_iPalette));
				continue;
			}

			float r = m_aObjectives[i + 2] * mapEnt.GetCurrentZoom();
			AddCircle(sx, sy, Math.Max(r, 6), RelationColor(rel, m_iPalette));
		}

		stride = DCO_PlayerAwareness.GROUP_STRIDE;
		for (int i = 0; i + stride <= m_aGroups.Count(); i += stride)
		{
			int gx, gy;
			mapEnt.WorldToScreen(m_aGroups[i], m_aGroups[i + 1], gx, gy, true);
			DCO_EGroupTask task = m_aGroups[i + 2];
			int flags = m_aGroups[i + 6];
			int color = TaskColor(task, m_iPalette);

			if (flags & DCO_PlayerAwareness.FLAG_HAS_DEST)
			{
				int dx, dy;
				mapEnt.WorldToScreen(m_aGroups[i + 3], m_aGroups[i + 4], dx, dy, true);
				if (Math.AbsFloat(dx - gx) + Math.AbsFloat(dy - gy) > SYMBOL_PX * 2)
					AddArrow(gx, gy, dx, dy, (color & 0x00FFFFFF) | 0xAA000000);
			}

			if (flags & DCO_PlayerAwareness.FLAG_ARMOR)
				AddSymbol(Square(gx, gy, SYMBOL_PX + 2), Square(gx, gy, SYMBOL_PX), color);
			else
				AddSymbol(Diamond(gx, gy, SYMBOL_PX + 3), Diamond(gx, gy, SYMBOL_PX), color);

			if (flags & DCO_PlayerAwareness.FLAG_CONTACT)
				AddCircle(gx, gy, SYMBOL_PX * 1.8, ContactColor(m_iPalette));

			string txt = string.Format("%1 x%2", TaskShort(task), m_aGroups[i + 5]);
			if (flags & DCO_PlayerAwareness.FLAG_MOUNTED)
				txt += " (mnt)";
			label = SetLabel(label, ws, gx + SYMBOL_PX + 2, gy - SYMBOL_PX, txt, color);
		}

		for (int i = label; i < m_aLabels.Count(); i++)
			m_aLabels[i].SetVisible(false);

		m_wCanvas.SetDrawCommands(m_aCommands);
	}

	protected int SetLabel(int idx, WorkspaceWidget ws, float sx, float sy, string text, int color)
	{
		if (idx >= m_aLabels.Count())
		{
			Widget w = ws.CreateWidgets(LABEL_LAYOUT, m_wRoot);
			if (!w)
				return idx;
			m_aLabels.Insert(w);
		}

		Widget lw = m_aLabels[idx];
		RichTextWidget t = RichTextWidget.Cast(lw);
		if (t)
		{
			t.SetText(text);
			t.SetColorInt(color);
			t.SetOutline(2, m_iOutline);
		}
		lw.SetVisible(true);
		FrameSlot.SetPos(lw, ws.DPIUnscale(sx), ws.DPIUnscale(sy));
		return idx + 1;
	}

	protected void AddPolygon(array<float> verts, int color)
	{
		PolygonDrawCommand p = new PolygonDrawCommand();
		p.m_iColor = color;
		p.m_Vertices = verts;
		m_aCommands.Insert(p);
	}

	protected void AddSymbol(array<float> outline, array<float> verts, int color)
	{
		AddPolygon(outline, m_iOutline);
		AddPolygon(verts, color);
	}

	protected void AddLine(array<float> verts, int color, float width, bool enclose)
	{
		LineDrawCommand o = new LineDrawCommand();
		o.m_iColor = (m_iOutline & 0x00FFFFFF) | (color & 0xFF000000);
		o.m_fWidth = width + 2;
		o.m_bShouldEnclose = enclose;
		o.m_Vertices = verts;
		m_aCommands.Insert(o);

		LineDrawCommand l = new LineDrawCommand();
		l.m_iColor = color;
		l.m_fWidth = width;
		l.m_bShouldEnclose = enclose;
		l.m_Vertices = verts;
		m_aCommands.Insert(l);
	}

	protected void AddCircle(float cx, float cy, float r, int color)
	{
		array<float> verts = {};
		for (int i = 0; i < CIRCLE_SEGMENTS; i++)
		{
			float a = i * Math.PI2 / CIRCLE_SEGMENTS;
			verts.Insert(cx + Math.Cos(a) * r);
			verts.Insert(cy + Math.Sin(a) * r);
		}
		AddLine(verts, color, 2, true);
	}

	protected void AddArrow(float x0, float y0, float x1, float y1, int color)
	{
		AddLine({x0, y0, x1, y1}, color, 2, false);

		float dx = x1 - x0;
		float dy = y1 - y0;
		float len = Math.Sqrt(dx * dx + dy * dy);
		if (len < 1)
			return;
		dx /= len;
		dy /= len;
		float s = SYMBOL_PX;
		float o = s + 2;
		AddSymbol({x1 + dx * 2, y1 + dy * 2, x1 - dx * o * 1.6 - dy * o * 0.7, y1 - dy * o * 1.6 + dx * o * 0.7, x1 - dx * o * 1.6 + dy * o * 0.7, y1 - dy * o * 1.6 - dx * o * 0.7},
			{x1, y1, x1 - dx * s * 1.6 - dy * s * 0.7, y1 - dy * s * 1.6 + dx * s * 0.7, x1 - dx * s * 1.6 + dy * s * 0.7, y1 - dy * s * 1.6 - dx * s * 0.7}, color);
	}

	protected static array<float> Diamond(float x, float y, float s)
	{
		return {x, y - s, x + s, y, x, y + s, x - s, y};
	}

	protected static array<float> Square(float x, float y, float s)
	{
		return {x - s, y - s, x + s, y - s, x + s, y + s, x - s, y + s};
	}

	protected static array<float> Triangle(float x, float y, float s, float dir)
	{
		return {x - s, y + s * dir, x + s, y + s * dir, x, y - s * dir};
	}

	protected static int OutlineColor(int palette)
	{
		if (palette == 1)
			return 0xFF000000;
		return 0xFFFFFFFF;
	}

	protected static int StagingColor(int palette)
	{
		switch (palette)
		{
			case 1: return 0xFFF3B231;
			case 2: return 0xFF101010;
		}
		return 0xFFB05A00;
	}

	protected static int ContactColor(int palette)
	{
		if (palette == 1)
			return 0xFFFF4A3C;
		return 0xFFD0101A;
	}

	protected static int TaskColor(DCO_EGroupTask task, int palette)
	{
		if (palette == 2)
			return 0xFF101010;

		bool bright = palette == 1;
		switch (task)
		{
			case DCO_EGroupTask.ATTACK:
			case DCO_EGroupTask.FLANK:
				if (bright) return 0xFFFF6A3D;
				return 0xFFC0281A;
			case DCO_EGroupTask.SUPPORT_BY_FIRE:
				if (bright) return 0xFFFFA23D;
				return 0xFFC25E00;
			case DCO_EGroupTask.DEFEND:
			case DCO_EGroupTask.GARRISON:
				if (bright) return 0xFF5AAAF0;
				return 0xFF1A56B0;
			case DCO_EGroupTask.RECON:
				if (bright) return 0xFF6ED660;
				return 0xFF1E7A28;
			case DCO_EGroupTask.REINFORCE:
				if (bright) return 0xFFE0D050;
				return 0xFF7A5C00;
		}
		if (bright)
			return 0xFFDDDDDD;
		return 0xFF333333;
	}

	protected static string TaskShort(DCO_EGroupTask task)
	{
		switch (task)
		{
			case DCO_EGroupTask.ATTACK:				return "ATK";
			case DCO_EGroupTask.FLANK:				return "FLK";
			case DCO_EGroupTask.SUPPORT_BY_FIRE:	return "SBF";
			case DCO_EGroupTask.DEFEND:				return "DEF";
			case DCO_EGroupTask.GARRISON:			return "GAR";
			case DCO_EGroupTask.RECON:				return "REC";
			case DCO_EGroupTask.PATROL:				return "PAT";
			case DCO_EGroupTask.REINFORCE:			return "RNF";
			case DCO_EGroupTask.TRANSPORT:			return "TRN";
			case DCO_EGroupTask.FIRE_MISSION:		return "ART";
		}
		return "RES";
	}

	protected static int RelationColor(int rel, int palette)
	{
		bool bright = palette == 1;
		switch (rel)
		{
			case DCO_PlayerAwareness.REL_FRIENDLY:
				if (bright) return 0xDD5AAAF0;
				return 0xEE1A56B0;
			case DCO_PlayerAwareness.REL_ENEMY:
				if (bright) return 0xDDFF4A3C;
				return 0xEEC0101A;
			case DCO_PlayerAwareness.REL_CONTESTED:
				if (bright) return 0xDDF3B231;
				return 0xEEB05A00;
		}
		if (bright)
			return 0xDDDDDDDD;
		return 0xEE404040;
	}
}
