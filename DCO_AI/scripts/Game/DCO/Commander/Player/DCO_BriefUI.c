class DCO_BriefUI
{
	protected static ref DCO_BriefUI s_Instance;

	protected int m_iKind = -1;
	protected int m_iFlags;
	protected string m_sType;
	protected string m_sObj;
	protected string m_sTask;
	protected string m_sHead;
	protected ref array<string> m_aLines = {};
	protected ref array<float> m_aPts = {};
	protected vector m_vObj;
	protected float m_fHHourAt = -1;
	protected bool m_bActive;
	protected float m_fDrawUntil = -1;
	protected string m_sLastLine;
	protected string m_sHeld;
	protected int m_iHeldKind = -1;
	protected bool m_bOpen;
	protected float m_fOpenUntil;
	protected int m_iDetail = -1;
	protected float m_fKeyDown = -1;
	protected bool m_bListening;
	protected bool m_bFight;
	protected float m_fFightUntil = -1;
	protected int m_iAmmo = -1;
	protected float m_fHealth = -1;
	protected IEntity m_Char;
	protected float m_fRelayUntil = -1;
	protected string m_sRelay;
	protected float m_fSlow;
	protected float m_fSubUntil = -1;
	protected bool m_bSample;
	protected bool m_bTicking;
	protected bool m_bMapCounted;
	protected float m_fToggleAt = -10;

	protected Widget m_wTab;
	protected Widget m_wCard;
	protected Widget m_wSub;
	protected ref array<Widget> m_aMarks = {};

	protected static DCO_BriefUI Get()
	{
		if (!s_Instance)
			s_Instance = new DCO_BriefUI();
		return s_Instance;
	}

	static float Now()
	{
		BaseWorld w = GetGame().GetWorld();
		if (!w)
			return 0;
		return w.GetWorldTime() / 1000.0;
	}

	static int Eff(int row)
	{
		int v = DCO_UISettings.GetBriefRow(row);
		int cap = SCR_PlayerControllerGroupComponent.DCO_LocalBriefCap();
		if (cap < 0)
			return 0;
		if (row == 1)
			return Math.Min(v, cap + 1);
		if (row == 3)
		{
			int sm = SCR_PlayerControllerGroupComponent.DCO_LocalBriefMarkers();
			if (sm == 1)
				return 0;
			if (sm == 2)
				return 2;
			return Math.Min(v, cap);
		}
		if (row == 4)
			return Math.Min(v, Math.Min(cap + 1, 2));
		return v;
	}

	static bool Capped(int row)
	{
		if (row == 3 && SCR_PlayerControllerGroupComponent.DCO_LocalBriefMarkers() != 0)
			return false;
		return Eff(row) < DCO_UISettings.GetBriefRow(row);
	}

	static bool HasOrders()
	{
		return s_Instance && s_Instance.m_bActive;
	}

	static bool IsLeader()
	{
		SCR_PlayerControllerGroupComponent c = SCR_PlayerControllerGroupComponent.GetLocalPlayerControllerGroupComponent();
		return c && c.IsPlayerLeaderOwnGroup();
	}

	static SCR_AIGroup OwnGroup()
	{
		SCR_GroupsManagerComponent gm = SCR_GroupsManagerComponent.GetInstance();
		PlayerController pc = GetGame().GetPlayerController();
		if (!gm || !pc)
			return null;
		return gm.GetPlayerGroup(pc.GetPlayerId());
	}

	static string RelayName()
	{
		SCR_AIGroup grp = OwnGroup();
		if (!grp)
			return "Squad leader";
		string cs = DCO_PlayerComms.GetCallsign(grp);
		int lid = grp.GetLeaderID();
		string name = GetGame().GetPlayerManager().GetPlayerName(lid);
		IEntity le = GetGame().GetPlayerManager().GetPlayerControlledEntity(lid);
		string rank;
		if (le)
			rank = SCR_CharacterRankComponent.GetCharacterRankNameShort(le);
		if (!rank.IsEmpty())
			name = rank + " " + name;
		return cs + ", " + name;
	}

	protected int Detail()
	{
		if (m_iDetail >= 1)
			return m_iDetail;
		int d = Eff(2);
		if (d != 0)
			return d;
		if (IsLeader())
			return 2;
		return 1;
	}

	static void Receive(int kind, int flags, string type, string obj, string task, string head, string lines, array<float> pts, vector info)
	{
		if (System.IsConsoleApp())
			return;
		Get().OnMsg(kind, flags, type, obj, task, head, lines, pts, info, false);
	}

	protected void OnMsg(int kind, int flags, string type, string obj, string task, string head, string lines, array<float> pts, vector info, bool sample)
	{
		m_bSample = sample;
		if (kind != DCO_EBriefKind.WARNING)
		{
			m_iKind = kind;
			m_iFlags = flags;
			if (!type.IsEmpty())
				m_sType = type;
			m_sObj = obj;
			if (!task.IsEmpty())
				m_sTask = task;
			m_sHead = head;
			if (!lines.IsEmpty())
			{
				m_aLines.Clear();
				lines.Split("\n", m_aLines, true);
			}
			if (pts && !pts.IsEmpty())
				m_aPts.Copy(pts);
			m_vObj = Vector(info[0], 0, info[2]);
			if (info[1] > 0)
				m_fHHourAt = Now() + info[1];
			bool isFinal = (flags & DCO_EBriefFlag.FINAL) != 0 || kind == DCO_EBriefKind.END;
			m_bActive = !isFinal;
			m_fDrawUntil = -1;
			if (isFinal)
			{
				m_fDrawUntil = Now() + 120.0;
				m_bOpen = false;
			}
			m_bMapCounted = false;
			DCO_MapOverlay.EnsureHooked();
		}
		string text = task;
		if (!head.IsEmpty())
			text = head;
		Present(kind, text);
		EnsureTick();
	}

	protected string Header(int kind)
	{
		switch (kind)
		{
			case DCO_EBriefKind.OPORD: return DCO_Radio.Resolve("brief_hdr_orders", 0, null) + "  ·  " + m_sType;
			case DCO_EBriefKind.FRAGO: return DCO_Radio.Resolve("brief_hdr_frago", 0, null) + "  ·  " + m_sType;
			case DCO_EBriefKind.WITHDRAW: return DCO_Radio.Resolve("brief_hdr_withdraw", 0, null);
			case DCO_EBriefKind.ABORT: return DCO_Radio.Resolve("brief_hdr_abort", 0, null);
			case DCO_EBriefKind.WARNING: return DCO_Radio.Resolve("brief_hdr_warning", 0, null);
		}
		return DCO_Radio.Resolve("brief_hdr_orders", 0, null);
	}

	protected static string Row(string line)
	{
		int bar = line.IndexOf("|");
		if (bar < 0)
			return line;
		return "<color rgba=\"243,178,49,255\">" + line.Substring(0, bar) + ":</color> " + line.Substring(bar + 1, line.Length() - bar - 1);
	}

	protected string FullText()
	{
		string s;
		foreach (string l : m_aLines)
		{
			if (!s.IsEmpty())
				s += "\n";
			s += Row(l);
		}
		return s;
	}

	protected string Moment(int kind)
	{
		switch (kind)
		{
			case DCO_EBriefKind.OPORD: return "arrive";
			case DCO_EBriefKind.FRAGO: return "frago";
			case DCO_EBriefKind.WITHDRAW:
			case DCO_EBriefKind.ABORT: return "withdraw";
			case DCO_EBriefKind.WARNING: return "arrive";
		}
		return "phase";
	}

	protected void Log(string shown, string moment, string supp)
	{
		int pid = -1;
		PlayerController pc = GetGame().GetPlayerController();
		if (pc)
			pid = pc.GetPlayerId();
		bool capped = Capped(1) || Capped(3) || Capped(4);
		DCO_Log.Write(DCO_ELogChannel.PLAYER, DCO_ELogLevel.INFO, "-", string.Format("brief_ui unit=%1 level=%2 shown=%3 moment=%4 suppressed_by=%5 capped=%6 leader=%7 sample=%8",
			pid, DCO_UISettings.BriefLevelName(DCO_UISettings.GetBriefLevel()), shown, moment, supp, capped, IsLeader(), m_bSample));
		DCO_Perf.Count("brief:ui:" + shown);
		if (capped)
			DCO_Perf.Count("brief:capped");
	}

	protected void Banner(int kind, string title, bool sound)
	{
		bool urgent = kind == DCO_EBriefKind.WITHDRAW || kind == DCO_EBriefKind.ABORT || kind == DCO_EBriefKind.WARNING;
		string snd;
		if (sound)
			snd = SCR_SoundEvent.TASK_CREATED;
		if (urgent)
		{
			Color red = Color.FromSRGBA(222, 72, 60, 255);
			DCO_OrderBanner.ShowUrgent(Header(kind), title, m_sObj, "", red, SCR_SoundEvent.ACTION_FAILED);
			return;
		}
		Color amber = Color.FromSRGBA(243, 178, 49, 255);
		DCO_OrderBanner.Show(Header(kind), title, m_sObj, "", amber, snd);
	}

	protected void Present(int kind, string text)
	{
		int show = Eff(1);
		bool leader = IsLeader();
		bool urgent = kind == DCO_EBriefKind.WITHDRAW || kind == DCO_EBriefKind.ABORT;
		string moment = Moment(kind);
		bool sound = Eff(10) == 1 && (kind == DCO_EBriefKind.OPORD || kind == DCO_EBriefKind.FRAGO || urgent);
		if (show == 0)
		{
			Log("none", moment, "off");
			return;
		}
		if (kind == DCO_EBriefKind.WARNING)
		{
			if (show == 1)
				Subtitle("<color rgba=\"222,72,60,255\">" + Header(kind) + ":</color> " + text, 8.0);
			else
				Banner(kind, text, true);
			Log("warning", moment, "none");
			return;
		}
		if (m_bFight && Eff(6) == 0 && !urgent)
		{
			m_sHeld = text;
			m_iHeldKind = kind;
			Log("none", moment, "firefight");
			Refresh();
			return;
		}
		string shown = "none";
		if (show == 1)
		{
			string sub = text;
			float hold = 8.0;
			if (Detail() == 2 && (kind == DCO_EBriefKind.OPORD || kind == DCO_EBriefKind.FRAGO))
			{
				sub = FullText();
				if (kind == DCO_EBriefKind.FRAGO)
					sub = text + "\n" + sub;
				hold = 16.0;
			}
			string tag = "<color rgba=\"243,178,49,255\">" + Header(kind) + ":</color> ";
			if (urgent)
				tag = "<color rgba=\"222,72,60,255\">" + Header(kind) + ":</color> ";
			m_sLastLine = tag + sub;
			Subtitle(m_sLastLine, hold);
			if (sound)
				SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.TASK_CREATED);
			shown = "subtitle";
		}
		else if (show == 2)
		{
			if (leader || urgent)
			{
				Banner(kind, text, sound);
				shown = "banner";
			}
			if (!leader)
			{
				Relay(text);
				if (shown == "none")
					shown = "relay";
			}
		}
		else
		{
			Banner(kind, text, sound);
			shown = "banner";
			int autoOpen = Eff(5);
			bool fresh = kind == DCO_EBriefKind.OPORD;
			bool important = fresh || kind == DCO_EBriefKind.FRAGO || urgent;
			if (!m_bFight && ((autoOpen == 1 && fresh) || (autoOpen == 2 && important)) && !DCO_OfferCard.IsOpen())
			{
				m_bOpen = true;
				m_fOpenUntil = Now() + 20.0;
				shown = "card";
			}
		}
		if (!leader && Eff(3) > 0 && kind != DCO_EBriefKind.END)
		{
			m_sRelay = text;
			m_fRelayUntil = Now() + 8.0;
			Log("marker", moment, "none");
		}
		Log(shown, moment, "none");
		Refresh();
	}

	protected Widget MakeLine()
	{
		Widget root = GetGame().GetWorkspace().CreateWidgets("{6D3A1F0B9C27E451}UI/layouts/DCO/DCO_ContactToast.layout");
		if (root)
			root.SetZOrder(999);
		return root;
	}

	protected void PlaceLine(Widget root, string text, string pos, float offX, float offY, int rows, int accent)
	{
		Widget toast = root.FindAnyWidget("Toast");
		RichTextWidget t = RichTextWidget.Cast(root.FindAnyWidget("Text"));
		Widget acc = root.FindAnyWidget("Accent");
		if (!toast || !t)
			return;
		float scale = DCO_UISettings.GetOfferScale();
		float h = (10.0 + 22.0 * Math.Max(rows, 1)) * scale;
		FrameSlot.SetSize(toast, 520.0 * scale, h);
		FrameSlot.SetSize(t, 500.0 * scale, h);
		FrameSlot.SetPos(t, 12.0 * scale, 0);
		if (acc)
		{
			FrameSlot.SetSize(acc, 4.0 * scale, h);
			acc.SetColorInt(Color.FromIntSRGB(accent).PackToInt());
		}
		int fs = Math.Max(10, Math.Round(17.0 * scale));
		t.SetDesiredFontSize(fs);
		t.SetMinFontSize(Math.Max(8, Math.Round(1.0 * fs * 0.6)));
		t.SetTextWrapping(true);
		t.SetText(text);
		float x, y, dirY;
		DCO_UISettings.Anchor(toast, pos, offX, offY, x, y, dirY);
		FrameSlot.SetPos(toast, x, y);
	}

	protected void Subtitle(string text, float hold)
	{
		if (!m_wSub)
			m_wSub = MakeLine();
		if (!m_wSub)
			return;
		int rows = 1;
		array<string> parts = {};
		text.Split("\n", parts, true);
		rows = parts.Count();
		PlaceLine(m_wSub, text, "bottom", 0, 150, rows, 0xFFF3B231);
		m_wSub.SetVisible(true);
		m_wSub.SetOpacity(1);
		m_fSubUntil = Now() + hold;
	}

	protected void Relay(string text)
	{
		string line = "<color rgba=\"110,214,96,255\">" + RelayName() + ":</color> " + text;
		m_sLastLine = line;
		Subtitle(line, 8.0);
	}

	protected void EnsureTick()
	{
		if (m_bTicking)
			return;
		m_bTicking = true;
		GetGame().GetCallqueue().CallLater(Tick, 0, true);
	}

	protected void Listen(bool on)
	{
		InputManager im = GetGame().GetInputManager();
		if (!im || on == m_bListening)
			return;
		m_bListening = on;
		if (on)
		{
			im.AddActionListener("DCO_ShowOrders", EActionTrigger.DOWN, OnKeyDown);
			im.AddActionListener("DCO_ShowOrders", EActionTrigger.UP, OnKeyUp);
			return;
		}
		im.RemoveActionListener("DCO_ShowOrders", EActionTrigger.DOWN, OnKeyDown);
		im.RemoveActionListener("DCO_ShowOrders", EActionTrigger.UP, OnKeyUp);
	}

	protected bool ChatOpen()
	{
		SCR_ChatPanelManager chat = SCR_ChatPanelManager.GetInstance();
		return chat && chat.DCO_AnyOpen();
	}

	protected void OnKeyDown()
	{
		if (ChatOpen())
			return;
		m_fKeyDown = Now();
	}

	protected void OnKeyUp()
	{
		if (m_fKeyDown < 0)
			return;
		float held = Now() - m_fKeyDown;
		m_fKeyDown = -1;
		if (held >= 0.5)
		{
			int d = Detail() + 1;
			if (d > 2)
				d = 1;
			m_iDetail = d;
			Log("detail_" + d.ToString(), "key", "none");
			Refresh();
			return;
		}
		Press("key");
	}

	static void Toggle()
	{
		if (!s_Instance || Now() - s_Instance.m_fToggleAt < 0.5)
			return;
		s_Instance.m_fToggleAt = Now();
		s_Instance.Press("radial");
	}

	protected void Press(string src)
	{
		if (!m_bActive)
			return;
		int show = Eff(1);
		if (show == 1)
		{
			if (!m_sLastLine.IsEmpty())
				Subtitle(m_sLastLine, 10.0);
			Log("subtitle", "key", "none");
			return;
		}
		if (show == 0)
			return;
		m_bOpen = !m_bOpen;
		m_fOpenUntil = Now() + 20.0;
		if (show == 3)
			m_fOpenUntil = Now() + 3600.0;
		string shown = "tab";
		if (m_bOpen)
			shown = "card";
		DCO_Perf.Count("brief:key:" + src);
		Log(shown, "key", "none");
		Refresh();
	}

	protected void PollFight()
	{
		IEntity ch = SCR_PlayerController.GetLocalControlledEntity();
		if (ch != m_Char)
		{
			m_Char = ch;
			m_iAmmo = -1;
			m_fHealth = -1;
		}
		if (ch)
		{
			BaseWeaponManagerComponent wm = BaseWeaponManagerComponent.Cast(ch.FindComponent(BaseWeaponManagerComponent));
			if (wm && wm.GetCurrentWeapon() && wm.GetCurrentWeapon().GetCurrentMagazine())
			{
				int ammo = wm.GetCurrentWeapon().GetCurrentMagazine().GetAmmoCount();
				if (m_iAmmo >= 0 && ammo < m_iAmmo)
					m_fFightUntil = Now() + 20.0;
				m_iAmmo = ammo;
			}
			SCR_CharacterDamageManagerComponent dm = SCR_CharacterDamageManagerComponent.Cast(ch.FindComponent(SCR_CharacterDamageManagerComponent));
			if (dm)
			{
				float hp = dm.GetHealth();
				if (m_fHealth >= 0 && hp < m_fHealth - 0.5)
					m_fFightUntil = Now() + 20.0;
				m_fHealth = hp;
			}
		}
		bool fight = Now() < m_fFightUntil;
		if (m_bFight && !fight)
		{
			m_bFight = false;
			DCO_Perf.Count("brief:fight_end");
			if (!m_sHeld.IsEmpty() && m_bActive)
			{
				string held = m_sHeld;
				m_sHeld = string.Empty;
				Present(m_iHeldKind, held);
			}
			return;
		}
		m_bFight = fight;
	}

	protected void Tick()
	{
		float now = Now();
		if (!m_bActive && now > m_fDrawUntil && now > m_fSubUntil)
		{
			Teardown();
			return;
		}
		if (m_bActive && !ChatOpen())
		{
			Listen(true);
			GetGame().GetInputManager().ActivateContext("DCO_BriefContext");
		}
		else
			Listen(false);
		if (now - m_fSlow >= 0.25)
		{
			m_fSlow = now;
			PollFight();
			Refresh();
		}
		if (m_wSub && now > m_fSubUntil)
			m_wSub.SetVisible(false);
		UpdateMarks();
	}

	protected void Teardown()
	{
		GetGame().GetCallqueue().Remove(Tick);
		m_bTicking = false;
		Listen(false);
		if (m_wTab)
			m_wTab.RemoveFromHierarchy();
		if (m_wCard)
			m_wCard.RemoveFromHierarchy();
		if (m_wSub)
			m_wSub.RemoveFromHierarchy();
		foreach (Widget mw : m_aMarks)
		{
			if (mw)
				mw.RemoveFromHierarchy();
		}
		m_aMarks.Clear();
		m_wTab = null;
		m_wCard = null;
		m_wSub = null;
		m_aPts.Clear();
		m_iKind = -1;
	}

	protected string TabText()
	{
		string s = "<color rgba=\"243,178,49,255\">" + m_sType + "</color> " + m_sObj + "  ·  " + m_sTask;
		if (!m_sHead.IsEmpty() && m_iKind != DCO_EBriefKind.OPORD)
			s += "  ·  <color rgba=\"200,200,200,255\">" + m_sHead + "</color>";
		if (m_fHHourAt > Now())
			s += "  ·  H " + HHourText();
		return s;
	}

	string HHourText()
	{
		int left = Math.Max(0, Math.Round(m_fHHourAt - Now()));
		int mm = left / 60;
		int ss = left % 60;
		string sec = ss.ToString();
		if (ss < 10)
			sec = "0" + sec;
		return mm.ToString() + ":" + sec;
	}

	protected void Refresh()
	{
		int show = Eff(1);
		bool leader = IsLeader();
		float now = Now();
		if (m_bOpen && (now > m_fOpenUntil || DCO_OfferCard.IsOpen() || !m_bActive))
			m_bOpen = false;
		bool hideFight = m_bFight && Eff(6) == 0;
		float alpha = 1.0;
		if (m_bFight && Eff(6) == 1)
			alpha = 0.4;
		bool cardOn = m_bActive && m_bOpen && show >= 2;
		bool tabOn = m_bActive && !cardOn && !hideFight && ((show == 2 && leader) || show == 3);

		float scale = DCO_UISettings.GetOfferScale();
		float offY = DCO_UISettings.GetOfferOffY();
		if (DCO_OfferCard.IsOpen())
			offY += 184.0 * scale;

		if (tabOn)
		{
			if (!m_wTab)
				m_wTab = MakeLine();
			if (m_wTab)
			{
				PlaceLine(m_wTab, TabText() + "   <action name=\"DCO_ShowOrders\"/>", DCO_UISettings.GetOfferPos(), DCO_UISettings.GetOfferOffX(), offY, 1, 0xFFF3B231);
				m_wTab.SetVisible(true);
				m_wTab.SetOpacity(alpha);
			}
		}
		else if (m_wTab)
			m_wTab.SetVisible(false);

		if (cardOn)
			BuildCard(offY, leader, alpha);
		else if (m_wCard)
		{
			m_wCard.RemoveFromHierarchy();
			m_wCard = null;
		}
	}

	protected void BuildCard(float offY, bool leader, float alpha)
	{
		float scale = DCO_UISettings.GetOfferScale();
		if (!m_wCard)
		{
			m_wCard = GetGame().GetWorkspace().CreateWidgets("{F2839C9190E03CED}UI/layouts/DCO/DCO_OfferCard.layout");
			if (!m_wCard)
				return;
			m_wCard.SetZOrder(1001);
			Widget sc = m_wCard.FindAnyWidget("Card");
			if (sc)
			{
				Widget ch = sc.GetChildren();
				while (ch)
				{
					if (FrameSlot.GetAnchorMin(ch) == FrameSlot.GetAnchorMax(ch))
					{
						FrameSlot.SetPos(ch, FrameSlot.GetPosX(ch) * scale, FrameSlot.GetPosY(ch) * scale);
						FrameSlot.SetSize(ch, FrameSlot.GetSizeX(ch) * scale, FrameSlot.GetSizeY(ch) * scale);
					}
					RichTextWidget ct = RichTextWidget.Cast(ch);
					if (ct)
					{
						int fsz = Math.Max(10, Math.Round(CardFont(ch.GetName()) * scale));
						ct.SetDesiredFontSize(fsz);
						ct.SetMinFontSize(Math.Max(8, Math.Round(1.0 * fsz * 0.6)));
					}
					ch = ch.GetSibling();
				}
			}
		}
		Widget card = m_wCard.FindAnyWidget("Card");
		if (!card)
			return;
		int detail = Detail();
		string desc = m_sTask;
		int rows = 1;
		if (detail == 2 && !m_aLines.IsEmpty())
		{
			desc = FullText();
			rows = m_aLines.Count() * 2;
		}
		float descH = 22.0 * rows;
		float h = 100.0 + descH + 40.0;
		SetCardText("Header", DCO_Radio.Resolve("brief_hdr_orders", 0, null) + "  ·  " + RelayName());
		SetCardText("Title", m_sType + " " + m_sObj);
		string chip = m_sHead;
		if (m_iKind == DCO_EBriefKind.WITHDRAW || m_iKind == DCO_EBriefKind.ABORT)
			chip = "<color rgba=\"222,72,60,255\">" + Header(m_iKind) + ": " + m_sHead + "</color>";
		else if (m_iKind == DCO_EBriefKind.FRAGO)
			chip = "<color rgba=\"243,178,49,255\">" + m_sHead + "</color>";
		if (m_fHHourAt > Now())
			chip += "   H " + HHourText();
		SetCardText("Sub", chip);
		SetCardText("Desc", desc);
		SetCardText("Keys", "<action name=\"DCO_ShowOrders\"/> " + DCO_Radio.Resolve("brief_card_keys", 0, null));
		Widget c = card.GetChildren();
		while (c)
		{
			string n = c.GetName();
			if (n == "Desc")
			{
				FrameSlot.SetSize(c, 448.0 * scale, descH * scale);
				RichTextWidget dt = RichTextWidget.Cast(c);
				if (dt)
					dt.SetTextWrapping(true);
			}
			else if (n == "Keys")
				FrameSlot.SetPos(c, 20.0 * scale, (100.0 + descH) * scale);
			else if (n == "Accent")
				FrameSlot.SetSize(c, 6.0 * scale, h * scale);
			else if (n == "Bar")
			{
				FrameSlot.SetPos(c, 0, (h - 4.0) * scale);
				float left = Math.Clamp((m_fOpenUntil - Now()) / 20.0, 0, 1);
				FrameSlot.SetSize(c, 480.0 * scale * left, 4.0 * scale);
			}
			c = c.GetSibling();
		}
		FrameSlot.SetSize(card, 480.0 * scale, h * scale);
		float x, y, dirY;
		DCO_UISettings.Anchor(card, DCO_UISettings.GetOfferPos(), DCO_UISettings.GetOfferOffX(), offY, x, y, dirY);
		FrameSlot.SetPos(card, x, y);
		card.SetOpacity(alpha);
	}

	protected static float CardFont(string name)
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

	protected static void P6(array<float> a, float t, float own, float x1, float z1, float x2, float z2)
	{
		a.Insert(t);
		a.Insert(own);
		a.Insert(x1);
		a.Insert(z1);
		a.Insert(x2);
		a.Insert(z2);
	}

	protected void SetCardText(string name, string text)
	{
		RichTextWidget t = RichTextWidget.Cast(m_wCard.FindAnyWidget(name));
		if (t)
			t.SetText(text);
	}

	protected bool FindPt(int type, bool ownOnly, bool second, out vector pos)
	{
		for (int i = 0; i + 6 <= m_aPts.Count(); i += 6)
		{
			if (m_aPts[i] != type || (ownOnly && m_aPts[i + 1] < 0.5))
				continue;
			if (second)
				pos = Vector(m_aPts[i + 4], 0, m_aPts[i + 5]);
			else
				pos = Vector(m_aPts[i + 2], 0, m_aPts[i + 3]);
			return true;
		}
		return false;
	}

	protected void UpdateMarks()
	{
		int eff = Eff(3);
		int used = 0;
		IEntity me = SCR_PlayerController.GetLocalControlledEntity();
		if (m_bActive && eff > 0 && me)
		{
			float alpha = 1.0;
			if (m_bFight && Eff(6) != 2)
				alpha = 0.4;
			int size = 14;
			if (Eff(7) == 1)
				size = 18;
			vector dest;
			bool hasDest = FindPt(DCO_EBriefPt.ROUTE, true, true, dest);
			if (!hasDest)
				hasDest = FindPt(DCO_EBriefPt.OWN, true, false, dest);
			if (hasDest)
			{
				string dtext = m_sType + " · " + Math.Round(vector.DistanceXZ(me.GetOrigin(), dest)).ToString() + " m";
				int dcol = 0xFFF3B231;
				if (Now() < m_fRelayUntil)
				{
					dtext = m_sRelay;
					dcol = 0xFF6ED660;
				}
				used = Mark(used, dest, dtext, dcol, size, alpha);
			}
			if (eff == 2)
			{
				vector pa;
				vector pb;
				if (FindPt(DCO_EBriefPt.TRIGGER, false, false, pa) && FindPt(DCO_EBriefPt.TRIGGER, false, true, pb))
					used = Mark(used, (pa + pb) * 0.5, DCO_Radio.Resolve("brief_mk_trigger", 0, null), 0xFFB0B0B0, size, alpha);
				vector rp;
				if (FindPt(DCO_EBriefPt.RP, false, false, rp))
					used = Mark(used, rp, DCO_Radio.Resolve("brief_mk_rp", 0, null), 0xFFB0B0B0, size, alpha);
				vector trp;
				if (FindPt(DCO_EBriefPt.TRP, false, false, trp))
					used = Mark(used, trp, DCO_Radio.Resolve("brief_mk_trp", 0, null), 0xFFB0B0B0, size, alpha);
			}
		}
		for (int k = used; k < m_aMarks.Count(); k++)
		{
			if (m_aMarks[k])
				m_aMarks[k].SetVisible(false);
		}
	}

	protected int Mark(int idx, vector pos, string text, int color, int size, float alpha)
	{
		WorkspaceWidget ws = GetGame().GetWorkspace();
		BaseWorld world = GetGame().GetWorld();
		if (!ws || !world)
			return idx;
		vector wp = Vector(pos[0], world.GetSurfaceY(pos[0], pos[2]) + 2.0, pos[2]);
		vector sp = ws.ProjWorldToScreen(wp, world);
		if (sp[2] <= 0)
			return idx;
		if (idx >= m_aMarks.Count())
		{
			Widget w = ws.CreateWidgets("{0B50B84909DB2710}UI/layouts/DCO/DCO_MapLabel.layout");
			if (!w)
				return idx;
			w.SetZOrder(990);
			m_aMarks.Insert(w);
		}
		Widget mw = m_aMarks[idx];
		RichTextWidget t = RichTextWidget.Cast(mw);
		if (t)
		{
			t.SetText(text);
			t.SetColorInt(Color.FromIntSRGB(color).PackToInt());
			t.SetDesiredFontSize(size);
			t.SetMinFontSize(size);
		}
		mw.SetOpacity(alpha);
		mw.SetVisible(true);
		FrameSlot.SetPos(mw, sp[0], sp[1]);
		return idx + 1;
	}

	static DCO_BriefUI MapSource()
	{
		if (!s_Instance || s_Instance.m_aPts.IsEmpty())
			return null;
		if (!s_Instance.m_bActive && Now() > s_Instance.m_fDrawUntil)
			return null;
		if (Eff(4) == 0)
			return null;
		if (!s_Instance.m_bMapCounted)
		{
			s_Instance.m_bMapCounted = true;
			DCO_Perf.Count("brief:map_draw");
		}
		return s_Instance;
	}

	array<float> GetPts()			{ return m_aPts; }
	array<string> GetLines()		{ return m_aLines; }
	int GetFlags()					{ return m_iFlags; }
	string GetObjName()				{ return m_sObj; }
	string GetTypeName()			{ return m_sType; }
	vector GetObjPos()				{ return m_vObj; }
	bool HasHHour()					{ return m_fHHourAt > Now(); }

	static void ShowSample()
	{
		DCO_BriefUI ui = Get();
		string lines = "Enemy|Enemy squad from the north-east, 450 m out.\nYour task|Area defense: hold Arleville, cover the north-east.\nFriendly|3 squads on the position, reserve in the south-west.\nFires|Hold fire until the enemy is inside 350 m.\nWithdraw|Fall back to RP 045-112 on order. Signal: LOOSE.";
		vector me = vector.Zero;
		IEntity ent = SCR_PlayerController.GetLocalControlledEntity();
		if (ent)
			me = ent.GetOrigin();
		array<float> pts = {};
		P6(pts, DCO_EBriefPt.OBJ, 0, me[0] + 150, me[2] + 150, 80, 0);
		P6(pts, DCO_EBriefPt.OWN, 1, me[0], me[2], me[0], me[2]);
		P6(pts, DCO_EBriefPt.ROUTE, 1, me[0], me[2], me[0] + 120, me[2] + 120);
		P6(pts, DCO_EBriefPt.RP, 0, me[0] - 150, me[2] - 150, me[0] - 150, me[2] - 150);
		ui.OnMsg(DCO_EBriefKind.OPORD, 0, "HOLD", "Arleville (sample)", "Hold the north-east edge", "", lines, pts, Vector(me[0] + 150, -1, me[2] + 150), true);
		GetGame().GetCallqueue().Remove(ui.EndSample);
		GetGame().GetCallqueue().CallLater(ui.EndSample, 25000);
	}

	protected void EndSample()
	{
		if (!m_bSample)
			return;
		m_bActive = false;
		m_bOpen = false;
		m_fDrawUntil = -1;
		Refresh();
	}
}
