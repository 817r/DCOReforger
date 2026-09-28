class DCO_SettingsSubMenu : SCR_SettingsSubMenuBase
{
	protected static const int PREVIEW_DELAY_MS = 300;
	protected static const string ACTION_TEST = "MenuAdvancedKeybind";
	protected static const string ACTION_RESET = "MenuResetAllKeybind";

	protected bool m_bPreviewContact;
	protected static DCO_SettingsSubMenu s_Open;
	protected static const string CONTACT_TITLE = "Squad Contact Notifications";

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

		SCR_LabelComponent title = SCR_LabelComponent.GetComponent("TitleContact", m_wRoot);
		if (!title)
			return;
		if (on)
			title.SetText(CONTACT_TITLE);
		else
			title.SetText(CONTACT_TITLE + " - Disabled by server");
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
		SetupSpin("OrderPos");
		SetupSpin("ContactPos");
		SetupSpin("ContactShow");
		SetupSpin("ContactSound");
		SetupSpin("MapPalette");
		Refresh();
		s_Open = this;
		ApplyServerState();

		SCR_InputButtonComponent test = CreateNavigationButton(ACTION_TEST, "Test notifications", true);
		if (test)
			test.m_OnActivated.Insert(OnTest);
		SCR_InputButtonComponent reset = CreateNavigationButton(ACTION_RESET, "Reset DCO", true);
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
		SetSpin("OrderPos", DCO_UISettings.POSITIONS.Find(DCO_UISettings.GetPos()));
		SetSpin("ContactPos", DCO_UISettings.POSITIONS.Find(DCO_UISettings.GetContactPos()));
		SetSpin("ContactShow", ToIndex(DCO_UISettings.GetContactShow()));
		SetSpin("ContactSound", ToIndex(DCO_UISettings.GetContactSound()));
		SetSpin("MapPalette", DCO_UISettings.GetMapPalette());
		m_bLoadingSettings = false;
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
		}
		QueuePreview(name.StartsWith("Contact"));
	}

	protected void OnSpinChanged(SCR_SpinBoxComponent comp, int index)
	{
		if (m_bLoadingSettings || !comp)
			return;

		string name = comp.GetRootWidget().GetName();
		switch (name)
		{
			case "OrderPos":		DCO_UISettings.SetPos(DCO_UISettings.POSITIONS[index]); break;
			case "ContactPos":		DCO_UISettings.SetContactPos(DCO_UISettings.POSITIONS[index]); break;
			case "ContactShow":		DCO_UISettings.SetContactShow(index == 1); break;
			case "ContactSound":	DCO_UISettings.SetContactSound(index == 1); break;
			case "MapPalette":		DCO_UISettings.SetMapPalette(index); return;
		}
		QueuePreview(name.StartsWith("Contact"));
	}

	protected void QueuePreview(bool contact)
	{
		m_bPreviewContact = contact;
		GetGame().GetCallqueue().Remove(Preview);
		GetGame().GetCallqueue().CallLater(Preview, PREVIEW_DELAY_MS);
	}

	protected void Preview()
	{
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
	protected static const ResourceName DCO_SETTINGS_LAYOUT = "{83123D5F8F129490}UI/layouts/DCO/DCO_Settings.layout";

	override void OnMenuOpen()
	{
		super.OnMenuOpen();

		if (m_SuperMenuComponent && m_SuperMenuComponent.GetTabView())
			m_SuperMenuComponent.GetTabView().AddTab(DCO_SETTINGS_LAYOUT, "DCO", identifier: "SettingsDCO");
	}
}
