class DCO_UISettings
{
	protected static string s_sStretchLogged;

	static array<string> Positions()
	{
		return {"topleft", "top", "topright", "left", "center", "right", "bottomleft", "bottom", "bottomright"};
	}


	protected static bool s_bLoaded;
	protected static float s_fScale;
	protected static string s_sPos;
	protected static float s_fOffX;
	protected static float s_fOffY;

	protected static bool s_bContactShow;
	protected static float s_fContactScale;
	protected static string s_sContactPos;
	protected static float s_fContactOffX;
	protected static float s_fContactOffY;
	protected static float s_fContactHold;
	protected static bool s_bContactSound;
	protected static int s_iMapPalette;
	protected static float s_fOfferScale;
	protected static string s_sOfferPos;
	protected static float s_fOfferOffX;
	protected static float s_fOfferOffY;
	protected static bool s_bOfferSound;
	protected static bool s_bOfferMarker;
	protected static int s_iBriefLevel;
	protected static string s_sBriefCustom;

	static float GetOfferScale()		{ Load(); return s_fOfferScale; }
	static string GetOfferPos()			{ Load(); return s_sOfferPos; }
	static float GetOfferOffX()			{ Load(); return s_fOfferOffX; }
	static float GetOfferOffY()			{ Load(); return s_fOfferOffY; }
	static bool GetOfferSound()			{ Load(); return s_bOfferSound; }
	static bool GetOfferMarker()		{ Load(); return s_bOfferMarker; }
	static void SetOfferScale(float v)		{ Load(); s_fOfferScale = Math.Clamp(v, 0.5, 1.5); Save(); }
	static void SetOfferPos(string p)		{ Load(); if (Positions().Contains(p)) s_sOfferPos = p; Save(); }
	static void SetOfferOffset(float x, float y) { Load(); s_fOfferOffX = Math.Clamp(x, -2000, 2000); s_fOfferOffY = Math.Clamp(y, -2000, 2000); Save(); }
	static void SetOfferSound(bool b)		{ Load(); s_bOfferSound = b; Save(); }
	static void SetOfferMarker(bool b)		{ Load(); s_bOfferMarker = b; Save(); }

	static string BriefPresetStr(int level)
	{
		switch (level)
		{
			case 0: return "1201021111";
			case 2: return "3022211001";
		}
		return "2002200001";
	}

	static int BriefRowMax(int row)
	{
		if (row < 1 || row > 10)
			return 0;
		string m = "3222221111";
		return m.Get(row - 1).ToInt();
	}

	static string BriefLevelName(int level)
	{
		switch (level)
		{
			case 0: return "hardcore";
			case 2: return "assisted";
			case 3: return "custom";
		}
		return "standard";
	}

	protected static bool ValidBrief(string s)
	{
		if (s.Length() != 10)
			return false;
		for (int r = 1; r <= 10; r++)
		{
			int v = s.Get(r - 1).ToInt();
			if (v < 0 || v > BriefRowMax(r))
				return false;
		}
		return true;
	}

	static int GetBriefLevel()				{ Load(); return s_iBriefLevel; }

	static int BriefPresetRow(int level, int row)
	{
		if (row < 1 || row > 10)
			return 0;
		string src = BriefPresetStr(level);
		return src.Get(row - 1).ToInt();
	}

	static int GetBriefRow(int row)
	{
		Load();
		if (row < 1 || row > 10)
			return 0;
		string src = BriefPresetStr(s_iBriefLevel);
		if (s_iBriefLevel == 3)
			src = s_sBriefCustom;
		return src.Get(row - 1).ToInt();
	}

	static int BriefChangedRows(int from, int to)
	{
		Load();
		int level = s_iBriefLevel;
		int baseLevel = level;
		if (level == 3)
			baseLevel = 1;
		int n = 0;
		for (int r = from; r <= to; r++)
		{
			if (GetBriefRow(r) != BriefPresetRow(baseLevel, r))
				n++;
		}
		return n;
	}

	static void SetBriefLevel(int level)
	{
		Load();
		s_iBriefLevel = Math.ClampInt(level, 0, 3);
		Save();
	}

	static void SetBriefRow(int row, int value)
	{
		Load();
		if (row < 1 || row > 10)
			return;
		string src = BriefPresetStr(s_iBriefLevel);
		if (s_iBriefLevel == 3)
			src = s_sBriefCustom;
		int v = Math.ClampInt(value, 0, BriefRowMax(row));
		s_sBriefCustom = src.Substring(0, row - 1) + v.ToString() + src.Substring(row, 10 - row);
		s_iBriefLevel = 3;
		Save();
	}

	static float GetScale()				{ Load(); return s_fScale; }
	static string GetPos()				{ Load(); return s_sPos; }
	static float GetOffX()				{ Load(); return s_fOffX; }
	static float GetOffY()				{ Load(); return s_fOffY; }
	static bool GetContactShow()		{ Load(); return s_bContactShow; }
	static float GetContactScale()		{ Load(); return s_fContactScale; }
	static string GetContactPos()		{ Load(); return s_sContactPos; }
	static float GetContactOffX()		{ Load(); return s_fContactOffX; }
	static float GetContactOffY()		{ Load(); return s_fContactOffY; }
	static float GetContactHold()		{ Load(); return s_fContactHold; }
	static bool GetContactSound()		{ Load(); return s_bContactSound; }
	static int GetMapPalette()			{ Load(); return s_iMapPalette; }

	static void SetScale(float v)			{ Load(); s_fScale = Math.Clamp(v, 0.5, 1.5); Save(); }
	static void SetPos(string p)			{ Load(); if (Positions().Contains(p)) s_sPos = p; Save(); }
	static void SetOffset(float x, float y)	{ Load(); s_fOffX = Math.Clamp(x, -2000, 2000); s_fOffY = Math.Clamp(y, -2000, 2000); Save(); }
	static void SetContactShow(bool b)		{ Load(); s_bContactShow = b; Save(); }
	static void SetContactScale(float v)	{ Load(); s_fContactScale = Math.Clamp(v, 0.5, 1.5); Save(); }
	static void SetContactPos(string p)		{ Load(); if (Positions().Contains(p)) s_sContactPos = p; Save(); }
	static void SetContactOffset(float x, float y) { Load(); s_fContactOffX = Math.Clamp(x, -2000, 2000); s_fContactOffY = Math.Clamp(y, -2000, 2000); Save(); }
	static void SetContactHold(float v)		{ Load(); s_fContactHold = Math.Clamp(v, 2, 15); Save(); }
	static void SetContactSound(bool b)		{ Load(); s_bContactSound = b; Save(); }
	static void SetMapPalette(int i)		{ Load(); s_iMapPalette = Math.ClampInt(i, 0, 2); Save(); }

	static void ResetAll()
	{
		Load();
		Defaults();
		Save();
	}

	protected static void Defaults()
	{
		s_fScale = 0.8;
		s_sPos = "bottomleft";
		s_fOffX = 60.0;
		s_fOffY = 25.0;
		s_bContactShow = true;
		s_fContactScale = 0.8;
		s_sContactPos = "right";
		s_fContactOffX = 40.0;
		s_fContactOffY = 0.0;
		s_fContactHold = 5.0;
		s_bContactSound = true;
		s_iMapPalette = 0;
		s_fOfferScale = 0.9;
		s_sOfferPos = "topright";
		s_fOfferOffX = 40.0;
		s_fOfferOffY = 120.0;
		s_bOfferSound = true;
		s_bOfferMarker = true;
		s_iBriefLevel = 1;
		s_sBriefCustom = "2002200001";
	}

	static void Anchor(notnull Widget w, string pos, float offX, float offY, out float x, out float y, out float dirY)
	{
		float ax = 0.5;
		if (pos.Contains("left"))
			ax = 0;
		else if (pos.Contains("right"))
			ax = 1;
		float ay = 0.5;
		if (pos.StartsWith("top"))
			ay = 0;
		else if (pos.StartsWith("bottom"))
			ay = 1;

		FrameSlot.SetAnchor(w, ax, ay);
		FrameSlot.SetAlignment(w, ax, ay);

		float sx = 1;
		if (ax > 0.5)
			sx = -1;
		dirY = 1;
		if (ay > 0.5)
			dirY = -1;
		x = offX * sx;
		y = offY * dirY;
	}

	static void LogStretchSkip(Widget parent, Widget child)
	{
		DCO_Perf.Count("ui:scale_skip_stretched");
		string key = parent.GetName() + "/" + child.GetName();
		if (key == s_sStretchLogged)
			return;
		s_sStretchLogged = key;
		DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", string.Format("ui_scale_skip_stretched parent=%1 child=%2", parent.GetName(), child.GetName()));
	}

	static void ScaleChildren(notnull Widget w, float scale, float baseFont)
	{
		FrameSlot.SetSize(w, FrameSlot.GetSizeX(w) * scale, FrameSlot.GetSizeY(w) * scale);
		Widget c = w.GetChildren();
		while (c)
		{
			if (FrameSlot.GetAnchorMin(c) == FrameSlot.GetAnchorMax(c))
			{
				FrameSlot.SetPos(c, FrameSlot.GetPosX(c) * scale, FrameSlot.GetPosY(c) * scale);
				FrameSlot.SetSize(c, FrameSlot.GetSizeX(c) * scale, FrameSlot.GetSizeY(c) * scale);
			}
			else
			{
				LogStretchSkip(w, c);
			}
			RichTextWidget text = RichTextWidget.Cast(c);
			if (text)
			{
				int size = Math.Max(10, Math.Round(BannerFontSize(c.GetName(), baseFont) * scale));
				text.SetDesiredFontSize(size);
				int minSize = Math.Max(8, Math.Round(1.0 * size * 0.6));
				text.SetMinFontSize(minSize);
			}
			c = c.GetSibling();
		}
	}

	static void Apply(notnull Widget banner, out float x, out float y, out float slideY)
	{
		Load();
		Anchor(banner, s_sPos, s_fOffX, s_fOffY, x, y, slideY);
		ScaleChildren(banner, s_fScale, 20);
	}

	protected static float BannerFontSize(string name, float baseFont)
	{
		switch (name)
		{
			case "Header": return 20;
			case "Title": return 38;
			case "Sub": return 19;
		}
		return baseFont;
	}

	static void ShowSample()
	{
		DCO_OrderBanner.Reset();
		Color accent = Color.FromSRGBA(243, 178, 49, 255);
		DCO_OrderBanner.Show("NEW ORDERS", "SAMPLE ORDER", "420 m  ·  NE", string.Empty, accent, string.Empty);
	}

	static void ShowOfferSample()
	{
		DCO_OfferCard.ShowSample();
	}

	static void ShowContactSample()
	{
		DCO_ContactToast.Show("Bravo 2", "enemy team, 250 m NE", false, true);
		DCO_ContactToast.Show("Alpha 1", "enemy squad, grid 045-112", true, true);
	}

	protected static void Load()
	{
		if (s_bLoaded)
			return;
		s_bLoaded = true;
		Defaults();
		if (!FileIO.FileExists("$profile:DCO_UI.json"))
			return;

		SCR_JsonLoadContext ctx = new SCR_JsonLoadContext();
		if (!ctx.LoadFromFile("$profile:DCO_UI.json"))
			return;

		float f;
		string s;
		bool b;
		int i;
		if (ctx.ReadValue("scale", f))
			s_fScale = Math.Clamp(f, 0.5, 1.5);
		if (ctx.ReadValue("position", s) && Positions().Contains(s))
			s_sPos = s;
		if (ctx.ReadValue("offsetX", f))
			s_fOffX = f;
		if (ctx.ReadValue("offsetY", f))
			s_fOffY = f;
		if (ctx.ReadValue("contactShow", b))
			s_bContactShow = b;
		if (ctx.ReadValue("contactScale", f))
			s_fContactScale = Math.Clamp(f, 0.5, 1.5);
		if (ctx.ReadValue("contactPosition", s) && Positions().Contains(s))
			s_sContactPos = s;
		if (ctx.ReadValue("contactOffsetX", f))
			s_fContactOffX = f;
		if (ctx.ReadValue("contactOffsetY", f))
			s_fContactOffY = f;
		if (ctx.ReadValue("contactHold", f))
			s_fContactHold = Math.Clamp(f, 2, 15);
		if (ctx.ReadValue("contactSound", b))
			s_bContactSound = b;
		if (ctx.ReadValue("mapPalette", i))
			s_iMapPalette = Math.ClampInt(i, 0, 2);
		if (ctx.ReadValue("offerScale", f))
			s_fOfferScale = Math.Clamp(f, 0.5, 1.5);
		if (ctx.ReadValue("offerPosition", s) && Positions().Contains(s))
			s_sOfferPos = s;
		if (ctx.ReadValue("offerOffsetX", f))
			s_fOfferOffX = f;
		if (ctx.ReadValue("offerOffsetY", f))
			s_fOfferOffY = f;
		if (ctx.ReadValue("offerSound", b))
			s_bOfferSound = b;
		if (ctx.ReadValue("offerMarker", b))
			s_bOfferMarker = b;
		if (ctx.ReadValue("briefLevel", i))
			s_iBriefLevel = Math.ClampInt(i, 0, 3);
		if (ctx.ReadValue("briefCustom", s) && ValidBrief(s))
			s_sBriefCustom = s;
	}

	protected static void Save()
	{
		SCR_JsonSaveContext ctx = new SCR_JsonSaveContext();
		ctx.WriteValue("scale", s_fScale);
		ctx.WriteValue("position", s_sPos);
		ctx.WriteValue("offsetX", s_fOffX);
		ctx.WriteValue("offsetY", s_fOffY);
		ctx.WriteValue("contactShow", s_bContactShow);
		ctx.WriteValue("contactScale", s_fContactScale);
		ctx.WriteValue("contactPosition", s_sContactPos);
		ctx.WriteValue("contactOffsetX", s_fContactOffX);
		ctx.WriteValue("contactOffsetY", s_fContactOffY);
		ctx.WriteValue("contactHold", s_fContactHold);
		ctx.WriteValue("contactSound", s_bContactSound);
		ctx.WriteValue("mapPalette", s_iMapPalette);
		ctx.WriteValue("offerScale", s_fOfferScale);
		ctx.WriteValue("offerPosition", s_sOfferPos);
		ctx.WriteValue("offerOffsetX", s_fOfferOffX);
		ctx.WriteValue("offerOffsetY", s_fOfferOffY);
		ctx.WriteValue("offerSound", s_bOfferSound);
		ctx.WriteValue("offerMarker", s_bOfferMarker);
		ctx.WriteValue("briefLevel", s_iBriefLevel);
		ctx.WriteValue("briefCustom", s_sBriefCustom);
		ctx.SaveToFile("$profile:DCO_UI.json");
	}
}
