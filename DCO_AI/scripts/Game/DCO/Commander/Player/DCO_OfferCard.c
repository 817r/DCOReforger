enum DCO_EOfferType
{
	SIGNAL,
	TRANSPORT,
	ROLE,
	TASK
}

enum DCO_EOfferResult
{
	PENDING,
	ACCEPTED,
	DECLINED,
	TIMEOUT,
	CANCELLED
}

[BaseContainerProps(configRoot: true)]
modded class SCR_ChatPanelManager
{
	bool DCO_AnyOpen()
	{
		foreach (SCR_ChatPanel p : m_aChatPanels)
		{
			if (p && p.IsOpen())
				return true;
		}
		return false;
	}
}

class DCO_OfferCard
{
	protected static ref DCO_OfferCard s_Instance;

	protected Widget m_wRoot;
	protected Widget m_wCard;
	protected Widget m_wBar;
	protected RichTextWidget m_wKeys;
	protected int m_iId;
	protected bool m_bLeader;
	protected bool m_bSample;
	protected bool m_bAnswered;
	protected bool m_bClosing;
	protected bool m_bListening;
	protected float m_fBarFull;
	protected float m_fTotal_ms;
	protected float m_fEnd_ms;
	protected vector m_vPos;

	static void Show(int id, int type, string callsign, string title, string sub, string desc, float x, float z, float remain_s, float total_s, bool leader)
	{
		if (System.IsConsoleApp())
			return;
		if (s_Instance && s_Instance.m_iId == id && !s_Instance.m_bClosing)
		{
			s_Instance.SetLeader(leader);
			return;
		}
		Clear();
		s_Instance = new DCO_OfferCard();
		s_Instance.Build(id, type, callsign, title, sub, desc, Vector(x, 0, z), remain_s, total_s, leader, false);
	}

	static bool IsOpen()
	{
		return s_Instance && s_Instance.m_wRoot && !s_Instance.m_bClosing;
	}

	static void Close(int id, int result)
	{
		if (!s_Instance || s_Instance.m_iId != id || s_Instance.m_bClosing)
			return;
		s_Instance.Finish(result);
	}

	static void ShowSample()
	{
		Clear();
		s_Instance = new DCO_OfferCard();
		s_Instance.Build(-1, DCO_EOfferType.ROLE, "Alpha 1-1", "SBF: suppress Arleville (Fire and Maneuver)", "Arleville  ·  420 m NE  ·  ETA 4 min",
			"Support by fire: suppress Arleville from here until the assault goes in, then shift fire.", vector.Zero, 30.0, 30.0, true, true);
	}

	static bool GetMarker(out vector pos)
	{
		if (!s_Instance || s_Instance.m_bClosing || s_Instance.m_bSample || s_Instance.m_vPos == vector.Zero || !DCO_UISettings.GetOfferMarker())
			return false;
		pos = s_Instance.m_vPos;
		return true;
	}

	static void Clear()
	{
		if (!s_Instance)
			return;
		s_Instance.Teardown();
		s_Instance = null;
	}

	protected static string TypeKey(int type)
	{
		switch (type)
		{
			case DCO_EOfferType.SIGNAL: return "offer_card_type_signal";
			case DCO_EOfferType.TRANSPORT: return "offer_card_type_transport";
			case DCO_EOfferType.ROLE: return "offer_card_type_role";
		}
		return "offer_card_type_task";
	}

	protected static float FontFor(string name)
	{
		switch (name)
		{
			case "Header": return 18;
			case "Title": return 26;
			case "Sub": return 17;
			case "Desc": return 15;
			case "Keys": return 17;
		}
		return 16;
	}

	protected void Build(int id, int type, string callsign, string title, string sub, string desc, vector pos, float remain_s, float total_s, bool leader, bool sample)
	{
		m_iId = id;
		m_bSample = sample;
		m_vPos = pos;
		m_fTotal_ms = Math.Max(total_s, 1.0) * 1000.0;
		m_fEnd_ms = GetGame().GetWorld().GetWorldTime() + remain_s * 1000.0;
		DCO_MapOverlay.EnsureHooked();

		m_wRoot = GetGame().GetWorkspace().CreateWidgets("{F2839C9190E03CED}UI/layouts/DCO/DCO_OfferCard.layout");
		if (!m_wRoot)
			return;
		m_wRoot.SetZOrder(1001);
		m_wCard = m_wRoot.FindAnyWidget("Card");
		m_wBar = m_wRoot.FindAnyWidget("Bar");
		m_wKeys = RichTextWidget.Cast(m_wRoot.FindAnyWidget("Keys"));
		if (!m_wCard || !m_wBar || !m_wKeys)
			return;

		float scale = DCO_UISettings.GetOfferScale();
		FrameSlot.SetSize(m_wCard, FrameSlot.GetSizeX(m_wCard) * scale, FrameSlot.GetSizeY(m_wCard) * scale);
		Widget c = m_wCard.GetChildren();
		while (c)
		{
			if (FrameSlot.GetAnchorMin(c) == FrameSlot.GetAnchorMax(c))
			{
				FrameSlot.SetPos(c, FrameSlot.GetPosX(c) * scale, FrameSlot.GetPosY(c) * scale);
				FrameSlot.SetSize(c, FrameSlot.GetSizeX(c) * scale, FrameSlot.GetSizeY(c) * scale);
			}
			else
			{
				DCO_UISettings.LogStretchSkip(m_wCard, c);
			}
			RichTextWidget t = RichTextWidget.Cast(c);
			if (t)
			{
				int size = Math.Max(10, Math.Round(FontFor(c.GetName()) * scale));
				t.SetDesiredFontSize(size);
				t.SetMinFontSize(Math.Max(8, Math.Round(1.0 * size * 0.6)));
			}
			c = c.GetSibling();
		}
		m_fBarFull = FrameSlot.GetSizeX(m_wBar);

		float x, y, dirY;
		DCO_UISettings.Anchor(m_wCard, DCO_UISettings.GetOfferPos(), DCO_UISettings.GetOfferOffX(), DCO_UISettings.GetOfferOffY(), x, y, dirY);
		FrameSlot.SetPos(m_wCard, x, y);

		string header = DCO_Radio.Resolve("offer_card_header", 0, null);
		if (!callsign.IsEmpty())
			header = header + "  ·  " + callsign;
		SetText("Header", header);
		SetText("Title", DCO_Radio.Resolve(TypeKey(type), 0, null) + ": " + title);
		SetText("Sub", sub);
		RichTextWidget d = RichTextWidget.Cast(m_wRoot.FindAnyWidget("Desc"));
		if (d)
		{
			d.SetTextWrapping(true);
			d.SetText(desc);
		}
		SetLeader(leader);

		m_wCard.SetOpacity(0);
		AnimateWidget.Opacity(m_wCard, 1, 1000.0 / 250);
		if (DCO_UISettings.GetOfferSound())
			SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.HINT);
		GetGame().GetCallqueue().CallLater(Tick, 0, true);
	}

	protected void SetText(string name, string text)
	{
		RichTextWidget t = RichTextWidget.Cast(m_wRoot.FindAnyWidget(name));
		if (t)
			t.SetText(text);
	}

	protected void SetLeader(bool leader)
	{
		m_bLeader = leader;
		if (!m_wKeys || m_bClosing)
			return;
		if (leader && !m_bAnswered)
		{
			m_wKeys.SetText("<action name=\"DCO_AcceptOffer\"/> " + DCO_Radio.Resolve("offer_card_accept", 0, null) + "      <action name=\"DCO_DeclineOffer\"/> " + DCO_Radio.Resolve("offer_card_decline", 0, null));
			Listen(true);
			return;
		}
		Listen(false);
		if (!leader)
			m_wKeys.SetText(DCO_Radio.Resolve("offer_card_wait", 0, null));
	}

	protected void Listen(bool on)
	{
		InputManager im = GetGame().GetInputManager();
		if (!im || on == m_bListening)
			return;
		m_bListening = on;
		if (on)
		{
			im.AddActionListener("DCO_AcceptOffer", EActionTrigger.DOWN, OnAccept);
			im.AddActionListener("DCO_DeclineOffer", EActionTrigger.DOWN, OnDecline);
			return;
		}
		im.RemoveActionListener("DCO_AcceptOffer", EActionTrigger.DOWN, OnAccept);
		im.RemoveActionListener("DCO_DeclineOffer", EActionTrigger.DOWN, OnDecline);
	}

	protected bool ChatOpen()
	{
		SCR_ChatPanelManager chat = SCR_ChatPanelManager.GetInstance();
		return chat && chat.DCO_AnyOpen();
	}

	protected void OnAccept()
	{
		Answer(true);
	}

	protected void OnDecline()
	{
		Answer(false);
	}

	protected void Answer(bool accept)
	{
		if (!m_bLeader || m_bAnswered || m_bClosing || ChatOpen())
			return;
		m_bAnswered = true;
		Listen(false);
		if (m_bSample)
		{
			int res = DCO_EOfferResult.DECLINED;
			if (accept)
				res = DCO_EOfferResult.ACCEPTED;
			Finish(res);
			return;
		}
		m_wKeys.SetText(DCO_Radio.Resolve("offer_card_sent", 0, null));
		SCR_PlayerControllerGroupComponent comp = SCR_PlayerControllerGroupComponent.GetLocalPlayerControllerGroupComponent();
		if (comp)
			comp.DCO_AnswerOffer(m_iId, accept);
	}

	protected void Tick()
	{
		if (!m_wRoot || !m_wBar)
		{
			Clear();
			return;
		}
		float now = GetGame().GetWorld().GetWorldTime();
		if (!m_bClosing)
		{
			float left = Math.Clamp((m_fEnd_ms - now) / m_fTotal_ms, 0, 1);
			FrameSlot.SetSizeX(m_wBar, m_fBarFull * left);
			if (m_bListening && !ChatOpen())
				GetGame().GetInputManager().ActivateContext("DCO_OfferContext");
			if ((m_bSample && left <= 0) || now > m_fEnd_ms + 10000.0)
				Finish(DCO_EOfferResult.TIMEOUT);
		}
	}

	protected void Finish(int result)
	{
		m_bClosing = true;
		Listen(false);
		string key = "offer_card_cancelled";
		Color col = Color.FromSRGBA(200, 200, 200, 255);
		switch (result)
		{
			case DCO_EOfferResult.ACCEPTED:
				key = "offer_card_accepted";
				col = Color.FromSRGBA(110, 220, 110, 255);
				break;
			case DCO_EOfferResult.DECLINED:
				key = "offer_card_declined";
				col = Color.FromSRGBA(240, 120, 90, 255);
				break;
			case DCO_EOfferResult.TIMEOUT:
				key = "offer_card_timeout";
				col = Color.FromSRGBA(240, 120, 90, 255);
				break;
		}
		if (m_wKeys)
		{
			m_wKeys.SetText(DCO_Radio.Resolve(key, 0, null));
			m_wKeys.SetColor(col);
		}
		GetGame().GetCallqueue().CallLater(FadeOut, 2000);
	}

	protected void FadeOut()
	{
		if (m_wCard)
			AnimateWidget.Opacity(m_wCard, 0, 1000.0 / 400);
		GetGame().GetCallqueue().CallLater(Remove, 400);
	}

	protected void Remove()
	{
		if (s_Instance == this)
			Clear();
		else
			Teardown();
	}

	protected void Teardown()
	{
		Listen(false);
		GetGame().GetCallqueue().Remove(Tick);
		GetGame().GetCallqueue().Remove(FadeOut);
		GetGame().GetCallqueue().Remove(Remove);
		if (m_wRoot)
			m_wRoot.RemoveFromHierarchy();
		m_wRoot = null;
		m_wCard = null;
		m_wBar = null;
		m_wKeys = null;
	}
}
