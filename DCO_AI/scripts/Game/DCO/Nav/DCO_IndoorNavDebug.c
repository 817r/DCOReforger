class DCO_IndoorNavDebug
{
	protected static IEntity s_Drawn;
	protected static int s_iDrawnMode;
	protected static ref array<ref Shape> s_aShapes;
	protected static array<ref Shape> GetShapes()
	{
		if (!s_aShapes)
			s_aShapes = {};
		return s_aShapes;
	}
	protected static ref array<ref DebugTextWorldSpace> s_aTexts;
	protected static array<ref DebugTextWorldSpace> GetTexts()
	{
		if (!s_aTexts)
			s_aTexts = {};
		return s_aTexts;
	}
	protected static int s_iWaitLog;
	protected static int s_iMissLog;

	static void Start()
	{
		if (!GetGame().InPlayMode())
			return;
		ClearDraw();
		GetGame().GetCallqueue().Remove(Tick);
		GetGame().GetCallqueue().CallLater(Tick, 1000, true);
		DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", "inav_debug state=start");
	}

	static void ClearDraw()
	{
		GetShapes().Clear();
		GetTexts().Clear();
		s_Drawn = null;
		s_iDrawnMode = 0;
	}

	static void Tick()
	{
		BaseWorld world = GetGame().GetWorld();
		DCO_GlobalAIComponent gai = DCO_GlobalAIComponent.GetInstance();
		if (!world || !gai)
		{
			if (s_Drawn)
				ClearDraw();
			return;
		}
		int mode = Math.Round(gai.DCO_TuneF("nav.debug", 0));
		if (!DCO_TestCaseComponent.RunOn())
			mode = 0;
		if (mode <= 0)
		{
			if (s_Drawn)
			{
				ClearDraw();
				DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", "inav_debug state=off");
			}
			return;
		}
		vector cam[4];
		world.GetCurrentCamera(cam);
		IEntity b = DCO_IndoorNav.NearestBuilding(cam[3], 40);
		if (!b)
		{
			s_iMissLog++;
			if (s_iMissLog % 10 == 1)
				DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", "inav_debug state=no_building cam=" + cam[3].ToString(false));
			return;
		}
		if (b == s_Drawn && mode == s_iDrawnMode)
			return;
		DCO_INavGraph g = DCO_IndoorNav.Get(b, false);
		if (!g)
		{
			g = DCO_IndoorNav.Get(b, true);
			if (!g)
			{
				s_iWaitLog++;
				if (s_iWaitLog % 10 == 1)
					DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", "inav_debug state=build_wait prefab=" + SCR_ResourceNameUtils.GetPrefabName(b));
				return;
			}
			DCO_IndoorNav.Dump(g);
		}
		Draw(g, mode);
		s_Drawn = b;
		s_iDrawnMode = mode;
		DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("inav_debug state=draw mode=%1 prefab=%2 shapes=%3 texts=%4", mode, g.m_sPrefab, GetShapes().Count(), GetTexts().Count()));
	}

	protected static int RoomColor(int id)
	{
		switch (id % 8)
		{
			case 0: return 0xFF7F77DD;
			case 1: return 0xFF1D9E75;
			case 2: return 0xFFD85A30;
			case 3: return 0xFFD4537E;
			case 4: return 0xFF639922;
			case 5: return 0xFFEF9F27;
			case 6: return 0xFF378ADD;
		}
		return 0xFF888780;
	}

	protected static string KindName(int kind)
	{
		if (kind == 1)
			return "Corridor";
		if (kind == 2)
			return "Stairs";
		if (kind == 3)
			return "Alcove";
		return "Room";
	}

	protected static void Line(int color, vector a, vector b)
	{
		vector seg[2];
		seg[0] = a;
		seg[1] = b;
		GetShapes().Insert(Shape.CreateLines(color, ShapeFlags.NOZBUFFER | ShapeFlags.TRANSP, seg, 2));
	}

	protected static void Ball(int color, vector p, float r)
	{
		GetShapes().Insert(Shape.CreateSphere(color, ShapeFlags.NOZBUFFER | ShapeFlags.TRANSP, p, r));
	}

	protected static void Text(vector p, string text, float size, int color)
	{
		DebugTextWorldSpace t = DCO_DebugDraw.SpawnText(p, text, size, color);
		if (t)
			GetTexts().Insert(t);
	}

	static void Draw(DCO_INavGraph g, int mode)
	{
		GetShapes().Clear();
		GetTexts().Clear();
		vector lift = 0.15 * vector.Up;
		foreach (DCO_INavSample s : g.m_aSamples)
		{
			int col = RoomColor(s.m_iRoom);
			if (s.m_iFloor < 0)
				col = 0xFF534AB7;
			Ball(col, g.W(s.m_vL) + lift, 0.07);
		}
		foreach (int li, int a : g.m_aLinkA)
		{
			int kind = g.m_aLinkKind[li];
			if (kind == 0 && mode < 2)
				continue;
			int lc = 0x80888780;
			if (kind == 1)
				lc = 0xFFE24B4A;
			else if (kind == 2)
				lc = 0xFF7F77DD;
			else if (kind == 3)
				lc = 0xFFBA7517;
			Line(lc, g.W(g.m_aSamples[a].m_vL) + lift, g.W(g.m_aSamples[g.m_aLinkB[li]].m_vL) + lift);
		}
		foreach (DCO_INavRoom r : g.m_aRooms)
		{
			if (r.m_iCenter < 0)
				continue;
			string label = string.Format("%1 %2 · %3 access · %4 windows", KindName(r.m_iKind), r.m_iId, r.m_iAccess, r.m_iWindows);
			Text(g.W(g.m_aSamples[r.m_iCenter].m_vL) + 1.6 * vector.Up, label, 13, RoomColor(r.m_iId));
		}
		foreach (DCO_INavPoint p : g.m_aPoints)
		{
			vector w = g.W(p.m_vL);
			switch (p.m_iType)
			{
				case 0:
					Ball(0xFFFFFFFF, w + 0.3 * vector.Up, 0.18);
					break;
				case 1:
					Ball(0xFF5F5E5A, w + 0.3 * vector.Up, 0.14);
					break;
				case 2:
					Ball(0xFF7F77DD, w + 0.3 * vector.Up, 0.16);
					break;
				case 3:
					Ball(0xFF1D9E75, w + 0.3 * vector.Up, 0.2);
					Text(w + 0.8 * vector.Up, p.m_iOrder.ToString(), 14, 0xFF1D9E75);
					break;
				case 4:
				{
					int tc = 0xFFFAC775;
					if (p.m_iLevel == 3)
						tc = 0xFFE24B4A;
					else if (p.m_iLevel == 2)
						tc = 0xFFEF9F27;
					Ball(tc, w, 0.16);
					break;
				}
			}
		}
		foreach (DCO_INavDoor d : g.m_aDoors)
		{
			vector c = g.W(d.m_vL + 0.05 * vector.Up);
			vector side = g.m_Building.VectorToParent(d.m_vS) * d.m_fHW;
			int dc = 0xFFE24B4A;
			if (!d.m_bReal)
				dc = 0xFFBA7517;
			Line(dc, c - side, c + side);
		}
		if (mode < 3)
			return;
		foreach (int ri, vector rp : g.m_aRejP)
		{
			Ball(RejColor(g.m_aRejWhy[ri]), g.W(rp) + 0.05 * vector.Up, 0.09);
		}
	}

	protected static int RejColor(int why)
	{
		switch (why)
		{
			case 1: return 0xFF00E5FF;
			case 2: return 0xFFFFFFFF;
			case 4: return 0xFF202020;
			case 5: return 0xFFFF00FF;
			case 6: return 0xFF3060FF;
			case 7: return 0xFFFFFF00;
		}
		return 0xFF808080;
	}
}

modded class DCO_GlobalAIComponent
{
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		DCO_IndoorNavDebug.Start();
	}
}
