[BaseContainerProps(), SCR_BaseContainerCustomTitleUIInfo("m_Info")]
modded class SCR_OpenAttributeWindowContextAction
{
	protected bool m_bDCO_Shortcut;

	override protected void OnShortcut()
	{
		m_bDCO_Shortcut = true;
		super.OnShortcut();
		m_bDCO_Shortcut = false;
	}

	override void Perform(SCR_EditableEntityComponent hoveredEntity, notnull set<SCR_EditableEntityComponent> selectedEntities, vector cursorWorldPosition, int flags, int param = -1)
	{
		SCR_AttributesManagerEditorComponent manager = SCR_AttributesManagerEditorComponent.DCO_Get();
		if (!manager)
		{
			super.Perform(hoveredEntity, selectedEntities, cursorWorldPosition, flags, param);
			return;
		}

		array<Managed> items = {};
		set<SCR_EditableEntityComponent> children = new set<SCR_EditableEntityComponent>();
		foreach (SCR_EditableEntityComponent entity : selectedEntities)
		{
			items.Insert(entity);
			entity.GetChildren(children);
			foreach (SCR_EditableEntityComponent child : children)
			{
				if (child.GetEntityType() == EEditableEntityType.SYSTEM)
					items.Insert(child);
			}
		}

		if (!manager.DCO_CanOpen(items, true))
		{
			super.Perform(hoveredEntity, selectedEntities, cursorWorldPosition, flags, param);
			return;
		}
		bool hasVanilla = manager.CanOpenAttributeDialog(items);
		if (hasVanilla && !m_bDCO_Shortcut)
		{
			DCO_SettingsChooserUI.LogOpenPath("properties_vanilla");
			super.Perform(hoveredEntity, selectedEntities, cursorWorldPosition, flags, param);
			return;
		}
		if (DidCursorMoveDuringDoubleClick())
			return;
		if (!hasVanilla)
		{
			if (m_bDCO_Shortcut)
				DCO_SettingsChooserUI.LogOpenPath("shortcut");
			else
				DCO_SettingsChooserUI.LogOpenPath("properties");
			manager.DCO_Open(items);
			return;
		}
		DCO_SettingsChooserUI.LogOpenPath("shortcut_chooser");
		DCO_SettingsChooserUI.Open(items);
	}
}

class DCO_SettingsChooserUI : EditorMenuBase
{
	protected static ref array<Managed> s_aItems;
	protected static int s_iChoice;
	protected static int s_iLast;

	protected ref DCO_ChooserInput m_Input;
	protected ResourceName m_sCellLayout = "{642F3127BF4E4FE7}UI/layouts/DCO/Settings/DCO_SetCell.layout";
	protected ref array<Widget> m_aHoverW = {};
	protected ref array<int> m_aHoverId = {};
	protected bool m_bPicked;
	protected ref Color m_cClear;
	protected ref Color m_cWhite;
	protected ref Color m_cOrange;
	protected ref Color m_cDim;
	protected ref Color m_cRule;
	protected ref Color m_cTile;
	protected ref Color m_cWarm;
	protected ref Color m_cText;
	protected ref Color m_cInk;
	protected ref Color m_cBadge;
	protected ref Color m_cDark;
	protected bool m_bPad;
	protected int m_iFocus;
	protected ref array<Widget> m_aKbmW = {};
	protected ref array<Widget> m_aPadW = {};
	protected array<Widget> m_aCollect;

	static bool PadInput()
	{
		InputManager im = GetGame().GetInputManager();
		return im && !im.IsUsingMouseAndKeyboard();
	}

	static bool PlayStation()
	{
		EPlatform platform = System.GetPlatform();
		return platform == EPlatform.PS4 || platform == EPlatform.PS5 || platform == EPlatform.PS5_PRO;
	}

	static ResourceName PadSet()
	{
		if (PlayStation())
			return "{C93D67F60DE59ED1}UI/Imagesets/buttons/playstation_buttons.imageset";
		return "{F7FD1672FECA05E8}UI/Textures/Icons/icons_gamepad_64.imageset";
	}

	static string PadGlyphName(string name)
	{
		if (!PlayStation())
			return name;
		switch (name)
		{
			case "A":
				return "cross";
			case "B":
				return "circle";
			case "X":
				return "square";
			case "Y":
				return "triangle";
			case "LB":
				return "L1";
			case "RB":
				return "R1";
			case "LT":
				return "L2";
			case "RT":
				return "R2";
			case "LS":
				return "L3";
			case "RS":
				return "R3";
			case "view":
				return "share";
			case "menu":
				return "options";
			case "DPAD_horizontal":
				return "DPAD_right";
		}
		return name;
	}

	static void LogOpenPath(string path)
	{
		string input = "kbm";
		if (PadInput())
			input = "pad";
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", "dco_settings_open_path path=" + path + " input=" + input);
	}

	static void Open(notnull array<Managed> items)
	{
		s_aItems = {};
		s_aItems.Copy(items);
		s_iChoice = 0;
		GetGame().GetMenuManager().OpenDialog(ChimeraMenuPreset.DCO_SettingsChooser);
	}

	protected void InitColors()
	{
		m_cClear = Color.FromSRGBA(0, 0, 0, 0);
		m_cWhite = Color.FromSRGBA(255, 255, 255, 255);
		m_cOrange = Color.FromSRGBA(201, 151, 111, 255);
		m_cDim = Color.FromSRGBA(143, 149, 154, 255);
		m_cRule = Color.FromSRGBA(255, 255, 255, 33);
		m_cTile = Color.FromSRGBA(255, 255, 255, 18);
		m_cWarm = Color.FromSRGBA(201, 151, 111, 38);
		m_cText = Color.FromSRGBA(201, 204, 208, 255);
		m_cInk = Color.FromSRGBA(241, 241, 239, 255);
		m_cBadge = Color.FromSRGBA(214, 216, 218, 255);
		m_cDark = Color.FromSRGBA(32, 33, 31, 255);
	}

	override void OnMenuOpen()
	{
		InitColors();
		Widget root = GetRootWidget();
		Widget dyn;
		Widget window;
		if (root)
		{
			dyn = root.FindAnyWidget("Dyn");
			window = root.FindAnyWidget("Window");
		}
		if (!dyn || !window || !s_aItems)
		{
			CloseSelf();
			return;
		}
		m_Input = new DCO_ChooserInput(this);
		window.AddHandler(m_Input);

		string name = ItemName();
		array<string> titleParams = {"name", name};
		string title = DCO_Radio.Resolve("set_chooser_title", 0, titleParams);
		if (s_aItems.Count() > 1)
			title += "  <color rgba=\"201,151,111,255\">×" + s_aItems.Count().ToString() + "</color>";
		Color clear = m_cClear;
		Color white = m_cWhite;
		Color orange = m_cOrange;
		Cell(dyn, 24, 18, 512, 36, title, clear, white, 1, 26, 0);
		Cell(dyn, 24, 56, 512, 26, DCO_Radio.Resolve("set_chooser_sub", 0, null), clear, m_cDim, 0, 17, 0);
		Cell(dyn, 24, 90, 512, 2, string.Empty, orange, white, 0, 10, 0);

		Tile(dyn, 24, 108, "set_chooser_vanilla", "set_chooser_vanilla_desc", "1", 1);
		Tile(dyn, 286, 108, "set_chooser_dco", "set_chooser_dco_desc", "2", 2);

		Cell(dyn, 24, 252, 512, 1, string.Empty, m_cRule, white, 0, 10, 0);
		m_aCollect = m_aKbmW;
		float x = Hint(dyn, 24, 266, "1", DCO_Radio.Resolve("set_chooser_key_vanilla", 0, null), false, 1);
		x = Hint(dyn, x + 24, 266, "2", DCO_Radio.Resolve("set_chooser_key_dco", 0, null), false, 2);
		Hint(dyn, x + 24, 266, "ESC", DCO_Radio.Resolve("set_close_btn", 0, null), true, 3);
		m_aCollect = m_aPadW;
		float px = GlyphHint(dyn, 24, 266, "A", DCO_Radio.Resolve("set_chooser_key_open", 0, null), 0);
		GlyphHint(dyn, px + 24, 266, "B", DCO_Radio.Resolve("set_close_btn", 0, null), 3);
		m_aCollect = null;
		m_iFocus = s_iLast;
		if (m_iFocus != 2)
			m_iFocus = 1;
		SetPad(PadInput());

		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", "dco_settings_chooser_open items=" + s_aItems.Count().ToString() + " name=" + DCO_SettingsBaseline.LogText(name));

		InputManager im = GetGame().GetInputManager();
		if (im)
		{
			im.AddActionListener("DCO_ChooseVanilla", EActionTrigger.DOWN, ChooseVanillaKey);
			im.AddActionListener("DCO_ChooseDCO", EActionTrigger.DOWN, ChooseDCOKey);
			im.AddActionListener("MenuBack", EActionTrigger.DOWN, CancelKey);
			im.AddActionListener("DCO_ChooseLeft", EActionTrigger.DOWN, ChooseLeftKey);
			im.AddActionListener("DCO_ChooseRight", EActionTrigger.DOWN, ChooseRightKey);
			im.AddActionListener("DCO_ChooseConfirm", EActionTrigger.DOWN, ConfirmKey);
		}
	}

	protected void SetPad(bool pad)
	{
		m_bPad = pad;
		foreach (Widget kbmW : m_aKbmW)
		{
			if (kbmW)
				kbmW.SetVisible(!pad);
		}
		foreach (Widget padW : m_aPadW)
		{
			if (padW)
				padW.SetVisible(pad);
		}
		Hover(0);
	}

	protected string KeyInput()
	{
		if (m_bPad)
			return "pad";
		return "key";
	}

	void ChooseLeftKey()
	{
		m_iFocus = 1;
		Hover(1);
	}

	void ChooseRightKey()
	{
		m_iFocus = 2;
		Hover(2);
	}

	void ConfirmKey()
	{
		Hover(m_iFocus);
		Pick(m_iFocus, KeyInput());
	}

	protected float GlyphHint(Widget parent, float x, float y, string glyph, string label, int id)
	{
		Widget c = Cell(parent, x, y - 4, 28, 28, string.Empty, m_cClear, m_cWhite, 0, 10, id);
		if (c)
		{
			ImageWidget icon = ImageWidget.Cast(c.FindAnyWidget("Icon"));
			if (icon)
			{
				icon.LoadImageFromSet(0, PadSet(), PadGlyphName(glyph));
				icon.SetColor(m_cWhite);
				icon.SetVisible(true);
			}
		}
		float lw = label.Length() * 9.2 + 12;
		Cell(parent, x + 34, y - 4, lw, 30, label, m_cClear, m_cInk, 0, 17, id);
		return x + 34 + lw;
	}

	override void OnMenuUpdate(float tDelta)
	{
		InputManager im = GetGame().GetInputManager();
		if (im)
			im.ActivateContext("DCO_SettingsChooserContext");
		bool pad = PadInput();
		if (pad != m_bPad)
			SetPad(pad);
	}

	override void OnMenuClose()
	{
		InputManager im = GetGame().GetInputManager();
		if (im)
		{
			im.RemoveActionListener("DCO_ChooseVanilla", EActionTrigger.DOWN, ChooseVanillaKey);
			im.RemoveActionListener("DCO_ChooseDCO", EActionTrigger.DOWN, ChooseDCOKey);
			im.RemoveActionListener("MenuBack", EActionTrigger.DOWN, CancelKey);
			im.RemoveActionListener("DCO_ChooseLeft", EActionTrigger.DOWN, ChooseLeftKey);
			im.RemoveActionListener("DCO_ChooseRight", EActionTrigger.DOWN, ChooseRightKey);
			im.RemoveActionListener("DCO_ChooseConfirm", EActionTrigger.DOWN, ConfirmKey);
		}
		if (s_iChoice != 0)
			GetGame().GetCallqueue().CallLater(StartChosen, 0, false);
	}

	protected string ItemName()
	{
		if (!s_aItems || s_aItems.IsEmpty())
			return string.Empty;
		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(s_aItems[0]);
		if (!editable)
			return string.Empty;
		string name = WidgetManager.Translate(editable.GetDisplayName());
		if (name.IsEmpty() && editable.GetOwner())
			name = editable.GetOwner().GetName();
		return name;
	}

	protected static void StartChosen()
	{
		int choice = s_iChoice;
		s_iChoice = 0;
		SCR_AttributesManagerEditorComponent manager = SCR_AttributesManagerEditorComponent.DCO_Get();
		if (!manager || !s_aItems || s_aItems.IsEmpty())
			return;
		array<Managed> items = {};
		items.Copy(s_aItems);
		s_aItems = null;
		if (choice == 2)
			manager.DCO_Open(items);
		else if (choice == 1)
			manager.StartEditing(items);
	}

	protected void Pick(int choice, string input)
	{
		if (m_bPicked)
			return;
		m_bPicked = true;
		s_iChoice = choice;
		if (choice == 1 || choice == 2)
			s_iLast = choice;
		string choiceName = "cancel";
		if (choice == 1)
			choiceName = "standard";
		else if (choice == 2)
			choiceName = "dco";
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", "dco_settings_chooser_pick choice=" + choiceName + " input=" + input);
		CloseSelf();
	}

	void ChooseVanillaKey()
	{
		Hover(1);
		Pick(1, "key");
	}

	void ChooseDCOKey()
	{
		Hover(2);
		Pick(2, "key");
	}

	void CancelKey()
	{
		if (m_bPad)
			Pick(0, "pad");
		else
			Pick(0, "esc");
	}

	void OnMouseClick(int id)
	{
		if (id == 1 || id == 2)
			Pick(id, "mouse");
		else if (id == 3)
			Pick(0, "mouse");
	}

	void Hover(int id)
	{
		int shown = id;
		if (id == 1 || id == 2)
			m_iFocus = id;
		else if (m_bPad)
			shown = m_iFocus;
		foreach (int i, Widget w : m_aHoverW)
		{
			if (w)
				w.SetVisible(m_aHoverId[i] == shown);
		}
	}

	protected void Tile(Widget parent, float x, float y, string nameKey, string descKey, string key, int id)
	{
		float w = 250;
		float h = 128;
		Cell(parent, x, y, w, h, string.Empty, m_cTile, m_cWhite, 0, 10, id);
		Widget grad = Cell(parent, x, y, w, h, string.Empty, m_cWarm, m_cWhite, 0, 10, id);
		if (grad)
		{
			grad.SetVisible(false);
			m_aHoverW.Insert(grad);
			m_aHoverId.Insert(id);
		}
		Widget underline = Cell(parent, x, y + h - 2, w, 2, string.Empty, m_cWhite, m_cWhite, 0, 10, id);
		if (underline)
		{
			underline.SetVisible(false);
			m_aHoverW.Insert(underline);
			m_aHoverId.Insert(id);
		}
		Cell(parent, x + 16, y + 14, w - 32, 30, DCO_Radio.Resolve(nameKey, 0, null), m_cClear, m_cWhite, 1, 21, id);
		Widget desc = Cell(parent, x + 16, y + 46, w - 32, 44, DCO_Radio.Resolve(descKey, 0, null), m_cClear, m_cText, 0, 16, id);
		if (desc)
		{
			RichTextWidget t = RichTextWidget.Cast(desc.FindAnyWidget("Text"));
			if (t)
				t.SetTextWrapping(true);
		}
		Badge(parent, x + 16, y + 94, key, false, id);
	}

	protected float Badge(Widget parent, float x, float y, string key, bool orange, int id)
	{
		float w = Math.Max(26, key.Length() * 9 + 12);
		Color bg = m_cBadge;
		if (orange)
			bg = m_cOrange;
		Cell(parent, x, y, w, 22, key, bg, m_cDark, 3, 15, id);
		return w;
	}

	protected float Hint(Widget parent, float x, float y, string key, string label, bool orange, int id)
	{
		float bw = Badge(parent, x, y, key, orange, id);
		float lw = label.Length() * 9.2 + 12;
		Cell(parent, x + bw + 6, y - 4, lw, 30, label, m_cClear, m_cInk, 0, 17, id);
		return x + bw + 6 + lw;
	}

	protected Widget Cell(Widget parent, float x, float y, float w, float h, string text, Color bg, Color fg, int style, int fontSize, int id)
	{
		Widget c = GetGame().GetWorkspace().CreateWidgets(m_sCellLayout, parent);
		if (!c)
			return null;
		FrameSlot.SetPos(c, x, y);
		FrameSlot.SetSize(c, w, h);
		ImageWidget bgw = ImageWidget.Cast(c.FindAnyWidget("Bg"));
		if (bgw)
			bgw.SetColor(bg);
		Widget icon = c.FindAnyWidget("Icon");
		if (icon)
			icon.SetVisible(false);
		if (!fg)
			fg = m_cText;
		array<string> names = {"Text", "TextB", "TextC", "TextCB"};
		foreach (int i, string n : names)
		{
			RichTextWidget t = RichTextWidget.Cast(c.FindAnyWidget(n));
			if (!t)
				continue;
			if (i != style)
			{
				t.SetVisible(false);
				continue;
			}
			t.SetText(text);
			t.SetColor(fg);
			t.SetDesiredFontSize(fontSize);
		}
		SetIds(c, id);
		if (m_aCollect)
			m_aCollect.Insert(c);
		return c;
	}

	protected void SetIds(Widget w, int id)
	{
		if (!w)
			return;
		w.SetUserID(id);
		Widget child = w.GetChildren();
		while (child)
		{
			SetIds(child, id);
			child = child.GetSibling();
		}
	}
}

class DCO_ChooserInput : ScriptedWidgetEventHandler
{
	protected DCO_SettingsChooserUI m_Menu;

	void DCO_ChooserInput(DCO_SettingsChooserUI menu)
	{
		m_Menu = menu;
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (m_Menu && w)
			m_Menu.OnMouseClick(w.GetUserID());
		return true;
	}

	override bool OnMouseEnter(Widget w, int x, int y)
	{
		if (m_Menu && w)
			m_Menu.Hover(w.GetUserID());
		return false;
	}

	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		if (!m_Menu || !w)
			return false;
		if (enterW && enterW.GetUserID() == w.GetUserID())
			return false;
		int next = 0;
		if (enterW)
			next = enterW.GetUserID();
		m_Menu.Hover(next);
		return false;
	}
}
