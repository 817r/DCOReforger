class DCO_UISettings
{
	static const ref array<string> POSITIONS = {"topleft", "top", "topright", "left", "center", "right", "bottomleft", "bottom", "bottomright"};
	static const ref array<string> POSITION_NAMES = {"Top left", "Top", "Top right", "Left", "Center", "Right", "Bottom left", "Bottom", "Bottom right"};


	protected static bool s_bLoaded;
	protected static float s_fScale = 0.8;
	protected static string s_sPos = "bottomleft";
	protected static float s_fOffX = 60.0;
	protected static float s_fOffY = 25.0;

	protected static bool s_bContactShow = true;
	protected static float s_fContactScale = 0.8;
	protected static string s_sContactPos = "right";
	protected static float s_fContactOffX = 40.0;
	protected static float s_fContactOffY = 0.0;
	protected static float s_fContactHold = 5.0;
	protected static bool s_bContactSound = true;
	protected static int s_iMapPalette = 0;

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
	static void SetPos(string p)			{ Load(); if (POSITIONS.Contains(p)) s_sPos = p; Save(); }
	static void SetOffset(float x, float y)	{ Load(); s_fOffX = Math.Clamp(x, -2000, 2000); s_fOffY = Math.Clamp(y, -2000, 2000); Save(); }
	static void SetContactShow(bool b)		{ Load(); s_bContactShow = b; Save(); }
	static void SetContactScale(float v)	{ Load(); s_fContactScale = Math.Clamp(v, 0.5, 1.5); Save(); }
	static void SetContactPos(string p)		{ Load(); if (POSITIONS.Contains(p)) s_sContactPos = p; Save(); }
	static void SetContactOffset(float x, float y) { Load(); s_fContactOffX = Math.Clamp(x, -2000, 2000); s_fContactOffY = Math.Clamp(y, -2000, 2000); Save(); }
	static void SetContactHold(float v)		{ Load(); s_fContactHold = Math.Clamp(v, 2, 15); Save(); }
	static void SetContactSound(bool b)		{ Load(); s_bContactSound = b; Save(); }
	static void SetMapPalette(int i)		{ Load(); s_iMapPalette = Math.ClampInt(i, 0, 2); Save(); }

	static void ResetAll()
	{
		Load();
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
		Save();
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

	static void ScaleChildren(notnull Widget w, float scale, float baseFont)
	{
		FrameSlot.SetSize(w, FrameSlot.GetSizeX(w) * scale, FrameSlot.GetSizeY(w) * scale);
		Widget c = w.GetChildren();
		while (c)
		{
			FrameSlot.SetPos(c, FrameSlot.GetPosX(c) * scale, FrameSlot.GetPosY(c) * scale);
			FrameSlot.SetSize(c, FrameSlot.GetSizeX(c) * scale, FrameSlot.GetSizeY(c) * scale);
			RichTextWidget text = RichTextWidget.Cast(c);
			if (text)
			{
				int size = Math.Max(10, Math.Round(BannerFontSize(c.GetName(), baseFont) * scale));
				text.SetDesiredFontSize(size);
				int minSize = Math.Max(8, Math.Round(size * 0.6));
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
		DCO_OrderBanner.Show("NEW ORDERS", "SAMPLE ORDER", "420 m  ·  NE", string.Empty, Color.FromRGBA(243, 178, 49, 255), string.Empty);
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
		if (ctx.ReadValue("position", s) && POSITIONS.Contains(s))
			s_sPos = s;
		if (ctx.ReadValue("offsetX", f))
			s_fOffX = f;
		if (ctx.ReadValue("offsetY", f))
			s_fOffY = f;
		if (ctx.ReadValue("contactShow", b))
			s_bContactShow = b;
		if (ctx.ReadValue("contactScale", f))
			s_fContactScale = Math.Clamp(f, 0.5, 1.5);
		if (ctx.ReadValue("contactPosition", s) && POSITIONS.Contains(s))
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
		ctx.SaveToFile("$profile:DCO_UI.json");
	}
}
