class DCO_OrderBannerMsg
{
	string m_sHeader;
	string m_sTitle;
	string m_sSub;
	string m_sIcon;
	ref Color m_Accent;
	string m_sSound;
}

class DCO_OrderBanner
{
	protected static const ResourceName LAYOUT = "{363FD3FC6E85FE60}UI/layouts/DCO/DCO_CommanderOrder.layout";
	protected static const ResourceName ICONS = "{10C0A9A305E8B3A4}UI/Imagesets/Tasks/Task_Icons.imageset";
	protected static const float SLIDE_PX = 40;
	protected static const float LINE_W = 630;
	protected static const int TYPE_MS = 30;
	protected static const int HOLD_MS = 5500;
	protected static const int FADE_MS = 400;

	protected static ref DCO_OrderBanner s_Instance;

	protected ref array<ref DCO_OrderBannerMsg> m_aQueue = {};
	protected Widget m_wRoot;
	protected RichTextWidget m_wTitle;
	protected string m_sTitle;
	protected int m_iTyped;

	static void Show(string header, string title, string sub, string icon, Color accent, string sound)
	{
		if (System.IsConsoleApp())
			return;

		if (!s_Instance)
			s_Instance = new DCO_OrderBanner();

		DCO_OrderBannerMsg msg = new DCO_OrderBannerMsg();
		msg.m_sHeader = header;
		msg.m_sTitle = title;
		msg.m_sSub = sub;
		msg.m_sIcon = icon;
		msg.m_Accent = accent;
		msg.m_sSound = sound;
		s_Instance.m_aQueue.Insert(msg);

		if (!s_Instance.m_wRoot)
			s_Instance.ShowNext();
	}

	static void Reset()
	{
		if (!s_Instance)
			return;

		GetGame().GetCallqueue().Remove(s_Instance.TypeTick);
		GetGame().GetCallqueue().Remove(s_Instance.FadeOut);
		GetGame().GetCallqueue().Remove(s_Instance.Finish);
		if (s_Instance.m_wRoot)
			s_Instance.m_wRoot.RemoveFromHierarchy();
		s_Instance = null;
	}

	protected void ShowNext()
	{
		if (m_aQueue.IsEmpty())
			return;

		DCO_OrderBannerMsg msg = m_aQueue[0];
		m_aQueue.RemoveOrdered(0);

		m_wRoot = GetGame().GetWorkspace().CreateWidgets(LAYOUT);
		if (!m_wRoot)
			return;
		m_wRoot.SetZOrder(1000);

		Widget banner = m_wRoot.FindAnyWidget("Banner");
		RichTextWidget header = RichTextWidget.Cast(m_wRoot.FindAnyWidget("Header"));
		RichTextWidget sub = RichTextWidget.Cast(m_wRoot.FindAnyWidget("Sub"));
		ImageWidget icon = ImageWidget.Cast(m_wRoot.FindAnyWidget("Icon"));
		Widget line = m_wRoot.FindAnyWidget("HeaderLine");
		m_wTitle = RichTextWidget.Cast(m_wRoot.FindAnyWidget("Title"));

		header.SetText(msg.m_sHeader);
		sub.SetText(msg.m_sSub);
		header.SetColor(msg.m_Accent);
		m_wRoot.FindAnyWidget("Accent").SetColor(msg.m_Accent);
		line.SetColor(msg.m_Accent);
		if (msg.m_sIcon.IsEmpty())
			icon.SetVisible(false);
		else
			icon.LoadImageFromSet(0, ICONS, msg.m_sIcon);

		float x, y, slideY;
		DCO_UISettings.Apply(banner, x, y, slideY);
		float scale = DCO_UISettings.GetScale();
		FrameSlot.SetPos(banner, x, y - SLIDE_PX * scale * slideY);
		float pos[2] = {x, y};
		AnimateWidget.Position(banner, pos, 5);
		AnimateWidget.Opacity(banner, 1, 5);
		FrameSlot.SetSizeX(line, 0);
		float size[2] = {LINE_W * scale, FrameSlot.GetSizeY(line)};
		AnimateWidget.Size(line, size, 2.5);

		FitTitle(m_wTitle, sub, msg.m_sTitle, msg.m_sSub.IsEmpty(), scale);

		m_sTitle = msg.m_sTitle;
		m_iTyped = 0;
		m_wTitle.SetText("");
		GetGame().GetCallqueue().CallLater(TypeTick, TYPE_MS, true);

		if (!msg.m_sSound.IsEmpty())
			SCR_UISoundEntity.SoundEvent(msg.m_sSound);

		GetGame().GetCallqueue().CallLater(FadeOut, HOLD_MS);
	}

	protected static void FitTitle(RichTextWidget title, Widget sub, string text, bool subEmpty, float scale)
	{
		float width = FrameSlot.GetSizeX(title);
		float fit = width / Math.Max(TextEm(text + "_"), 1);
		int maxSize = Math.Round(38 * scale);
		int minSize = Math.Round(24 * scale);
		int floorSize = Math.Max(10, Math.Round(12 * scale));

		if (fit >= minSize || !subEmpty)
		{
			int size = Math.Min(maxSize, Math.Max(minSize, Math.Floor(fit)));
			title.SetTextWrapping(false);
			title.SetDesiredFontSize(size);
			title.SetMinFontSize(floorSize);
			return;
		}

		float height = FrameSlot.GetPosY(sub) + FrameSlot.GetSizeY(sub) - FrameSlot.GetPosY(title);
		FrameSlot.SetSizeY(title, height);
		int wrapSize = Math.Min(Math.Floor(fit * 1.8), Math.Floor(height / 2.6));
		wrapSize = Math.Max(10, wrapSize);
		title.SetTextWrapping(true);
		title.SetDesiredFontSize(wrapSize);
		title.SetMinFontSize(floorSize);
		sub.SetVisible(false);
	}

	protected static float TextEm(string text)
	{
		float em = 0;
		for (int i = 0; i < text.Length(); i++)
		{
			int code = text.Get(i).ToAscii();
			if (code == 32)
				em += 0.24;
			else if (code >= 97 && code <= 122)
				em += 0.47;
			else if ((code >= 65 && code <= 90) || (code >= 48 && code <= 57))
				em += 0.58;
			else
				em += 0.36;
		}
		return em;
	}

	protected void TypeTick()
	{
		m_iTyped++;
		if (!m_wTitle || m_iTyped >= m_sTitle.Length())
		{
			GetGame().GetCallqueue().Remove(TypeTick);
			if (m_wTitle)
				m_wTitle.SetText(m_sTitle);
			return;
		}

		m_wTitle.SetText(m_sTitle.Substring(0, m_iTyped) + "_");
	}

	protected void FadeOut()
	{
		if (m_wRoot)
			AnimateWidget.Opacity(m_wRoot.FindAnyWidget("Banner"), 0, 1000.0 / FADE_MS);
		GetGame().GetCallqueue().CallLater(Finish, FADE_MS);
	}

	protected void Finish()
	{
		GetGame().GetCallqueue().Remove(TypeTick);
		if (m_wRoot)
			m_wRoot.RemoveFromHierarchy();
		m_wRoot = null;
		m_wTitle = null;
		ShowNext();
	}
}
