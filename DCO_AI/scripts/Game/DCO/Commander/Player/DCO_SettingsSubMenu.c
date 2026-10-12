class DCO_SettingsSubMenu : SCR_SettingsSubMenuBase
{

	protected bool m_bPreviewContact;
	protected bool m_bPreviewOffer;
	protected static DCO_SettingsSubMenu s_Open;

	static void OnServerShareChanged()
	{
		if (s_Open)
			s_Open.ApplyServerState();
	}

	protected void ApplyServerState()
	{
		bool on = SCR_PlayerControllerGroupComponent.DCO_LocalServerShare();
		array<string> names = {"ContactShow", "ContactSize", "ContactPos", "ContactOffX", "ContactOffY", "ContactHold", "ContactSound"};
		foreach (string name : names)
		{
			Widget w = m_wRoot.FindAnyWidget(name);
			if (w)
				w.SetEnabled(on);
		}

		ApplyBriefServer();
		SCR_LabelComponent title = SCR_LabelComponent.GetComponent("TitleContact", m_wRoot);
		if (!title)
			return;
		if (on)
			title.SetText("Squad Contact Notifications");
		else
			title.SetText("Squad Contact Notifications" + " - Disabled by server");
	}

	override void OnTabCreate(Widget menuRoot, ResourceName buttonsLayout, int index)
	{
		super.OnTabCreate(menuRoot, buttonsLayout, index);

		SetupSlider("OrderSize", 50, 150, 5);
		SetupSlider("OrderOffX", 0, 800, 5);
		SetupSlider("OrderOffY", 0, 800, 5);
		SetupSlider("ContactSize", 50, 150, 5);
		SetupSlider("ContactOffX", 0, 800, 5);
		SetupSlider("ContactOffY", 0, 800, 5);
		SetupSlider("ContactHold", 2, 15, 1);
		SetupSlider("OfferSize", 50, 150, 5);
		SetupSlider("OfferOffX", 0, 800, 5);
		SetupSlider("OfferOffY", 0, 800, 5);
		SetupSpin("OfferPos");
		SetupSpin("OfferSound");
		SetupSpin("OfferMarker");
		SetupSpin("OrderPos");
		SetupSpin("ContactPos");
		SetupSpin("ContactShow");
		SetupSpin("ContactSound");
		SetupSpin("MapPalette");
		SetupSpin("BriefLevel");
		SetupSpin("BriefMore");
		for (int br = 1; br <= 10; br++)
			SetupSpin(BriefRowName(br));
		Refresh();
		s_Open = this;
		ApplyServerState();

		SCR_InputButtonComponent test = CreateNavigationButton("MenuAdvancedKeybind", "Preview notifications + offer card", true);
		if (test)
			test.m_OnActivated.Insert(OnTest);
		SCR_InputButtonComponent brief = CreateNavigationButton("MenuSelect", "Preview orders", true);
		if (brief)
			brief.m_OnActivated.Insert(OnBriefPreview);
		SCR_InputButtonComponent reset = CreateNavigationButton("MenuResetAllKeybind", "Reset DCO", true);
		if (reset)
			reset.m_OnActivated.Insert(OnReset);
	}

	override void OnTabRemove()
	{
		if (s_Open == this)
			s_Open = null;
		GetGame().GetCallqueue().Remove(Preview);
		super.OnTabRemove();
	}

	protected void SetupSlider(string name, float min, float max, float step)
	{
		SCR_SliderComponent s = SCR_SliderComponent.GetSliderComponent(name, m_wRoot);
		if (!s)
			return;
		s.SetMin(min);
		s.SetMax(max);
		s.SetStep(step);
		s.m_OnChanged.Insert(OnSliderChanged);
	}

	protected void SetupSpin(string name)
	{
		SCR_SpinBoxComponent s = SCR_SpinBoxComponent.GetSpinBoxComponent(name, m_wRoot);
		if (s)
			s.m_OnChanged.Insert(OnSpinChanged);
	}

	protected void Refresh()
	{
		m_bLoadingSettings = true;
		SetSlider("OrderSize", DCO_UISettings.GetScale() * 100);
		SetSlider("OrderOffX", DCO_UISettings.GetOffX());
		SetSlider("OrderOffY", DCO_UISettings.GetOffY());
		SetSlider("ContactSize", DCO_UISettings.GetContactScale() * 100);
		SetSlider("ContactOffX", DCO_UISettings.GetContactOffX());
		SetSlider("ContactOffY", DCO_UISettings.GetContactOffY());
		SetSlider("ContactHold", DCO_UISettings.GetContactHold());
		SetSpin("OrderPos", DCO_UISettings.Positions().Find(DCO_UISettings.GetPos()));
		SetSpin("ContactPos", DCO_UISettings.Positions().Find(DCO_UISettings.GetContactPos()));
		SetSpin("ContactShow", ToIndex(DCO_UISettings.GetContactShow()));
		SetSpin("ContactSound", ToIndex(DCO_UISettings.GetContactSound()));
		SetSpin("MapPalette", DCO_UISettings.GetMapPalette());
		SetSlider("OfferSize", DCO_UISettings.GetOfferScale() * 100);
		SetSlider("OfferOffX", DCO_UISettings.GetOfferOffX());
		SetSlider("OfferOffY", DCO_UISettings.GetOfferOffY());
		SetSpin("OfferPos", DCO_UISettings.Positions().Find(DCO_UISettings.GetOfferPos()));
		SetSpin("OfferSound", ToIndex(DCO_UISettings.GetOfferSound()));
		SetSpin("OfferMarker", ToIndex(DCO_UISettings.GetOfferMarker()));
		SetSpin("BriefLevel", DCO_UISettings.GetBriefLevel());
		for (int rr = 1; rr <= 10; rr++)
			SetSpin(BriefRowName(rr), DCO_UISettings.GetBriefRow(rr));
		m_bLoadingSettings = false;
		UpdateBriefRows();
	}

	protected void SetSlider(string name, float value)
	{
		SCR_SliderComponent s = SCR_SliderComponent.GetSliderComponent(name, m_wRoot);
		if (s)
			s.SetValue(value);
	}

	protected static int ToIndex(bool b)
	{
		if (b)
			return 1;
		return 0;
	}

	protected void SetSpin(string name, int index)
	{
		SCR_SpinBoxComponent s = SCR_SpinBoxComponent.GetSpinBoxComponent(name, m_wRoot);
		if (index < 0)
			index = 0;
		if (s)
			s.SetCurrentItem(index, false, false, false);
	}

	protected void OnSliderChanged(SCR_SliderComponent comp, float value)
	{
		if (m_bLoadingSettings || !comp)
			return;

		string name = comp.GetRootWidget().GetName();
		switch (name)
		{
			case "OrderSize":	DCO_UISettings.SetScale(value / 100); break;
			case "OrderOffX":	DCO_UISettings.SetOffset(value, DCO_UISettings.GetOffY()); break;
			case "OrderOffY":	DCO_UISettings.SetOffset(DCO_UISettings.GetOffX(), value); break;
			case "ContactSize":	DCO_UISettings.SetContactScale(value / 100); break;
			case "ContactOffX":	DCO_UISettings.SetContactOffset(value, DCO_UISettings.GetContactOffY()); break;
			case "ContactOffY":	DCO_UISettings.SetContactOffset(DCO_UISettings.GetContactOffX(), value); break;
			case "ContactHold":	DCO_UISettings.SetContactHold(value); break;
			case "OfferSize":	DCO_UISettings.SetOfferScale(value / 100); break;
			case "OfferOffX":	DCO_UISettings.SetOfferOffset(value, DCO_UISettings.GetOfferOffY()); break;
			case "OfferOffY":	DCO_UISettings.SetOfferOffset(DCO_UISettings.GetOfferOffX(), value); break;
		}
		m_bPreviewOffer = name.StartsWith("Offer");
		QueuePreview(name.StartsWith("Contact"));
	}

	protected void OnSpinChanged(SCR_SpinBoxComponent comp, int index)
	{
		if (m_bLoadingSettings || !comp)
			return;

		string name = comp.GetRootWidget().GetName();
		if (name.StartsWith("Brief"))
		{
			OnBriefSpin(name, index);
			return;
		}
		switch (name)
		{
			case "OrderPos":		DCO_UISettings.SetPos(DCO_UISettings.Positions()[index]); break;
			case "ContactPos":		DCO_UISettings.SetContactPos(DCO_UISettings.Positions()[index]); break;
			case "ContactShow":		DCO_UISettings.SetContactShow(index == 1); break;
			case "ContactSound":	DCO_UISettings.SetContactSound(index == 1); break;
			case "MapPalette":		DCO_UISettings.SetMapPalette(index); return;
			case "OfferPos":		DCO_UISettings.SetOfferPos(DCO_UISettings.Positions()[index]); break;
			case "OfferSound":		DCO_UISettings.SetOfferSound(index == 1); break;
			case "OfferMarker":	DCO_UISettings.SetOfferMarker(index == 1); return;
		}
		m_bPreviewOffer = name.StartsWith("Offer");
		QueuePreview(name.StartsWith("Contact"));
	}

	protected void QueuePreview(bool contact)
	{
		m_bPreviewContact = contact;
		GetGame().GetCallqueue().Remove(Preview);
		GetGame().GetCallqueue().CallLater(Preview, 300);
	}

	protected void Preview()
	{
		if (m_bPreviewOffer)
		{
			DCO_UISettings.ShowOfferSample();
			return;
		}
		if (!m_bPreviewContact)
		{
			DCO_UISettings.ShowSample();
			return;
		}

		DCO_ContactToast.Clear();
		DCO_UISettings.ShowContactSample();
	}

	protected void OnTest()
	{
		DCO_ContactToast.Clear();
		DCO_UISettings.ShowSample();
		DCO_UISettings.ShowContactSample();
		DCO_UISettings.ShowOfferSample();
	}

	protected bool m_bBriefMore;

	protected static string BriefRowName(int row)
	{
		switch (row)
		{
			case 1: return "BriefShow";
			case 2: return "BriefDetail";
			case 3: return "BriefMarkers";
			case 4: return "BriefMap";
			case 5: return "BriefAuto";
			case 6: return "BriefFight";
			case 7: return "BriefSize";
			case 8: return "BriefStyle";
			case 9: return "BriefNote";
			case 10: return "BriefSound";
		}
		return "BriefNone";
	}

	protected static string BriefLevelLabel(int level)
	{
		switch (level)
		{
			case 0: return "Hardcore";
			case 2: return "Assisted";
			case 3: return "Custom";
		}
		return "Standard";
	}

	protected void OnBriefSpin(string name, int index)
	{
		if (name == "BriefMore")
		{
			m_bBriefMore = index == 1;
			UpdateBriefRows();
			return;
		}
		if (name == "BriefLevel")
			DCO_UISettings.SetBriefLevel(index);
		else
		{
			for (int r = 1; r <= 10; r++)
			{
				if (BriefRowName(r) == name)
					DCO_UISettings.SetBriefRow(r, index);
			}
		}
		Refresh();
		LogBrief();
		GetGame().GetCallqueue().Remove(OnBriefPreview);
		GetGame().GetCallqueue().CallLater(OnBriefPreview, 400);
	}

	protected void LogBrief()
	{
		int pid = -1;
		PlayerController pc = GetGame().GetPlayerController();
		if (pc)
			pid = pc.GetPlayerId();
		int cap = SCR_PlayerControllerGroupComponent.DCO_LocalBriefCap();
		string capName = "off";
		if (cap >= 0)
			capName = DCO_UISettings.BriefLevelName(cap);
		string markers = "level";
		int sm = SCR_PlayerControllerGroupComponent.DCO_LocalBriefMarkers();
		if (sm == 1)
			markers = "off";
		else if (sm == 2)
			markers = "on";
		DCO_Log.Write(DCO_ELogChannel.PLAYER, DCO_ELogLevel.INFO, "-", string.Format("brief_settings player=%1 level=%2 changed_rows=%3 server_cap=%4 server_markers=%5 rows=%6",
			pid, DCO_UISettings.BriefLevelName(DCO_UISettings.GetBriefLevel()), DCO_UISettings.BriefChangedRows(1, 10), capName, markers, BriefRowsText()));
	}

	protected string BriefRowsText()
	{
		string s;
		for (int r = 1; r <= 10; r++)
			s += DCO_UISettings.GetBriefRow(r).ToString();
		return s;
	}

	protected void ShowRow(string name, bool visible)
	{
		Widget w = m_wRoot.FindAnyWidget(name);
		if (w)
			w.SetVisible(visible);
	}

	protected void UpdateBriefRows()
	{
		if (!m_wRoot)
			return;
		int show = DCO_UISettings.GetBriefRow(1);
		bool more = m_bBriefMore;
		ShowRow("BriefDetail", show != 0);
		ShowRow("BriefAuto", more && show == 3);
		ShowRow("BriefFight", more && show != 0);
		ShowRow("BriefSize", more && DCO_BriefUI.Eff(3) > 0);
		ShowRow("BriefStyle", more && DCO_UISettings.GetBriefRow(4) > 0);
		ShowRow("BriefNote", more);
		ShowRow("BriefSound", more);
		SCR_SpinBoxComponent m = SCR_SpinBoxComponent.GetSpinBoxComponent("BriefMore", m_wRoot);
		if (m)
			m.SetLabel(string.Format("More options (6 rows, %1 changed)", DCO_UISettings.BriefChangedRows(5, 10)));
	}

	protected void Relabel(string name, array<string> items, int limit, string suffix)
	{
		SCR_SpinBoxComponent s = SCR_SpinBoxComponent.GetSpinBoxComponent(name, m_wRoot);
		if (!s)
			return;
		int cur = s.GetCurrentIndex();
		s.ClearAll();
		foreach (int i, string item : items)
		{
			if (i > limit)
				s.AddItem(item + suffix);
			else
				s.AddItem(item);
		}
		if (cur >= 0)
			s.SetCurrentItem(cur, false, false, false);
	}

	protected void ApplyBriefServer()
	{
		int cap = SCR_PlayerControllerGroupComponent.DCO_LocalBriefCap();
		int sm = SCR_PlayerControllerGroupComponent.DCO_LocalBriefMarkers();
		bool on = cap >= 0;
		int lim = cap;
		if (!on)
			lim = 2;
		bool wasLoading = m_bLoadingSettings;
		m_bLoadingSettings = true;
		string tag = " (server limit)";
		Relabel("BriefLevel", {"Hardcore", "Standard", "Assisted", "Custom"}, lim, tag);
		SCR_SpinBoxComponent lv = SCR_SpinBoxComponent.GetSpinBoxComponent("BriefLevel", m_wRoot);
		if (lv && lim < 2)
		{
			lv.RemoveItem(3);
			lv.AddItem("Custom");
			lv.SetCurrentItem(DCO_UISettings.GetBriefLevel(), false, false, false);
		}
		Relabel("BriefShow", {"Off", "Radio subtitles", "Contextual tab", "Orders card"}, lim + 1, tag);
		Relabel("BriefMarkers", {"Off", "Your destination", "All control measures"}, lim, tag);
		Relabel("BriefMap", {"Off", "Your group", "All groups"}, Math.Min(lim + 1, 2), tag);
		m_bLoadingSettings = wasLoading;

		array<string> names = {"BriefLevel", "BriefShow", "BriefDetail", "BriefMarkers", "BriefMap", "BriefMore", "BriefAuto", "BriefFight", "BriefSize", "BriefStyle", "BriefNote", "BriefSound"};
		foreach (string n : names)
		{
			Widget w = m_wRoot.FindAnyWidget(n);
			if (w)
				w.SetEnabled(on);
		}
		SCR_SpinBoxComponent mk = SCR_SpinBoxComponent.GetSpinBoxComponent("BriefMarkers", m_wRoot);
		if (mk)
		{
			if (sm != 0)
			{
				mk.SetLabel("Markers in the world (set by server)");
				mk.SetEnabled(false);
			}
			else
				mk.SetLabel("Markers in the world");
		}

		SCR_LabelComponent title = SCR_LabelComponent.GetComponent("TitleBrief", m_wRoot);
		if (title)
		{
			if (!on)
				title.SetText("Commander Briefings - Disabled by server");
			else if (cap < 2)
				title.SetText("Commander Briefings - This server allows up to " + BriefLevelLabel(cap));
			else
				title.SetText("Commander Briefings");
		}
		UpdateBriefRows();
	}

	protected void OnBriefPreview()
	{
		DCO_BriefUI.ShowSample();
	}

	protected void OnReset()
	{
		DCO_UISettings.ResetAll();
		Refresh();
		OnTest();
	}
}

modded class SCR_SettingsSuperMenu
{

	override void OnMenuOpen()
	{
		super.OnMenuOpen();

		if (m_SuperMenuComponent && m_SuperMenuComponent.GetTabView())
			m_SuperMenuComponent.GetTabView().AddTab("{83123D5F8F129490}UI/layouts/DCO/DCO_Settings.layout", "DCO", identifier: "SettingsDCO");
	}
}
