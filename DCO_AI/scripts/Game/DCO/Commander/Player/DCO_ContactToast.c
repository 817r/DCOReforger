class DCO_ContactToast
{
	protected static const ResourceName LAYOUT = "{6D3A1F0B9C27E451}UI/layouts/DCO/DCO_ContactToast.layout";
	protected static const int FADE_MS = 300;

	protected static ref DCO_ContactToast s_Instance;

	protected ref array<Widget> m_aRoots = {};

	static void Show(string callsign, string text, bool radio, bool force = false)
	{
		if (System.IsConsoleApp() || !SCR_PlayerControllerGroupComponent.DCO_LocalServerShare())
			return;
		if (!force && !DCO_UISettings.GetContactShow())
			return;

		if (!s_Instance)
			s_Instance = new DCO_ContactToast();
		s_Instance.Add(callsign, text, radio);

		if (radio && DCO_UISettings.GetContactSound())
			SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.HINT);
	}

	static void Clear()
	{
		if (!s_Instance)
			return;

		GetGame().GetCallqueue().Remove(s_Instance.Expire);
		GetGame().GetCallqueue().Remove(s_Instance.RemoveRoot);
		foreach (Widget w : s_Instance.m_aRoots)
		{
			if (w)
				w.RemoveFromHierarchy();
		}
		s_Instance = null;
	}

	protected void Add(string callsign, string text, bool radio)
	{
		Widget root = GetGame().GetWorkspace().CreateWidgets(LAYOUT);
		if (!root)
			return;
		root.SetZOrder(1000);

		string label = callsign;
		if (radio)
			label = label + " (radio)";
		label.ToUpper();

		RichTextWidget t = RichTextWidget.Cast(root.FindAnyWidget("Text"));
		if (t)
			t.SetText("<color rgba=\"110,195,255,255\">" + label + "</color>  " + text);

		Widget toast = root.FindAnyWidget("Toast");
		DCO_UISettings.ScaleChildren(toast, DCO_UISettings.GetContactScale(), 17);
		toast.SetOpacity(0);
		AnimateWidget.Opacity(toast, 1, 1000.0 / FADE_MS);

		m_aRoots.InsertAt(root, 0);
		while (m_aRoots.Count() > 3)
		{
			Widget old = m_aRoots[m_aRoots.Count() - 1];
			m_aRoots.Remove(m_aRoots.Count() - 1);
			if (old)
				old.RemoveFromHierarchy();
		}
		Relayout();

		GetGame().GetCallqueue().CallLater(Expire, DCO_UISettings.GetContactHold() * 1000, false, root);
	}

	protected void Relayout()
	{
		string pos = DCO_UISettings.GetContactPos();
		float offX = DCO_UISettings.GetContactOffX();
		float offY = DCO_UISettings.GetContactOffY();

		foreach (int i, Widget root : m_aRoots)
		{
			Widget toast = root.FindAnyWidget("Toast");
			if (!toast)
				continue;

			float x, y, dirY;
			DCO_UISettings.Anchor(toast, pos, offX, offY, x, y, dirY);
			float step = (FrameSlot.GetSizeY(toast) + 4.0) * i;
			FrameSlot.SetPos(toast, x, y + step * dirY);
		}
	}

	protected void Expire(Widget root)
	{
		if (!root)
			return;
		Widget toast = root.FindAnyWidget("Toast");
		if (toast)
			AnimateWidget.Opacity(toast, 0, 1000.0 / FADE_MS);
		GetGame().GetCallqueue().CallLater(RemoveRoot, FADE_MS, false, root);
	}

	protected void RemoveRoot(Widget root)
	{
		m_aRoots.RemoveItem(root);
		if (root)
			root.RemoveFromHierarchy();
		Relayout();
	}
}
