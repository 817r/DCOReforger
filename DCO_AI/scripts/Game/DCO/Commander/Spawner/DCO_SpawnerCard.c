class DCO_SpawnerCardInput : ScriptedWidgetEventHandler
{
	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (w)
			DCO_SpawnerCard.OnButton(w.GetUserID());
		return true;
	}
}

class DCO_SpawnerCardItem
{
	Widget m_wRoot;
	int m_iSpawner;
	int m_iCommander;
	int m_iKind;
	float m_fUntil;
}

class DCO_SpawnerCard
{
	protected static ref DCO_SpawnerCard s_Inst;
	protected ref array<ref DCO_SpawnerCardItem> m_aItems = {};
	protected ref DCO_SpawnerCardInput m_Input = new DCO_SpawnerCardInput();
	protected ref array<Widget> m_aBtnRoot = {};
	protected ref array<int> m_aBtnTarget = {};
	protected ref array<int> m_aBtnAct = {};

	static void Show(int spawnerId, int cmdId, int kind, string title, string text)
	{
		if (System.IsConsoleApp())
			return;
		if (!s_Inst)
		{
			s_Inst = new DCO_SpawnerCard();
			GetGame().GetCallqueue().CallLater(s_Inst.Expire, 1000, true);
		}
		s_Inst.Add(spawnerId, cmdId, kind, title, text);
	}

	static void OnButton(int id)
	{
		if (s_Inst)
			s_Inst.Handle(id);
	}

	protected int Btn(Widget root, int target, int act)
	{
		m_aBtnRoot.Insert(root);
		m_aBtnTarget.Insert(target);
		m_aBtnAct.Insert(act);
		return 900000 + m_aBtnAct.Count() - 1;
	}

	protected Widget Cell(Widget parent, float x, float y, float w, float h, string text, Color bg, Color fg, int style, int size, int id)
	{
		Widget c = GetGame().GetWorkspace().CreateWidgets("{642F3127BF4E4FE7}UI/layouts/DCO/Settings/DCO_SetCell.layout", parent);
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
			t.SetDesiredFontSize(size);
			t.SetTextWrapping(true);
		}
		SetId(c, id);
		return c;
	}

	protected void SetId(Widget w, int id)
	{
		if (!w)
			return;
		w.SetUserID(id);
		Widget ch = w.GetChildren();
		while (ch)
		{
			SetId(ch, id);
			ch = ch.GetSibling();
		}
	}

	protected void Add(int spawnerId, int cmdId, int kind, string title, string text)
	{
		Widget root = GetGame().GetWorkspace().CreateWidgets("{95823CD31C3487DC}UI/layouts/DCO/DCO_SpawnerCard.layout");
		if (!root)
			return;
		FrameSlot.SetAnchorMin(root, 1, 0);
		FrameSlot.SetAnchorMax(root, 1, 0);
		FrameSlot.SetSize(root, 520, 150);
		root.SetZOrder(1001);
		root.AddHandler(m_Input);

		if (m_aItems.IsEmpty() && m_aBtnAct.Count() > 60)
		{
			m_aBtnRoot.Clear();
			m_aBtnTarget.Clear();
			m_aBtnAct.Clear();
			DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", "spawner_card_ids_reset");
		}
		else if (m_aBtnAct.Count() > 60)
		{
			int freed = 0;
			foreach (int bi, Widget br : m_aBtnRoot)
			{
				if (!br)
					continue;
				bool live = false;
				foreach (DCO_SpawnerCardItem it : m_aItems)
				{
					if (it && it.m_wRoot == br)
					{
						live = true;
						break;
					}
				}
				if (!live)
				{
					m_aBtnRoot[bi] = null;
					freed++;
				}
			}
			if (freed > 0)
				DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", string.Format("spawner_card_ids_freed n=%1 total=%2", freed, m_aBtnAct.Count()));
		}
		DCO_SpawnerCardItem item = new DCO_SpawnerCardItem();
		item.m_wRoot = root;
		item.m_iSpawner = spawnerId;
		item.m_iCommander = cmdId;
		item.m_iKind = kind;
		float hold = 12.0;
		if (kind > 0)
			hold = 40.0;
		item.m_fUntil = GetGame().GetWorld().GetWorldTime() + hold * 1000.0;
		m_aItems.InsertAt(item, 0);
		int cardIndex = m_aItems.Count() - 1;

		Color accent = Color.FromSRGBA(232, 137, 43, 255);
		if (kind == 2 || kind == 3)
			accent = Color.FromSRGBA(214, 69, 69, 255);
		else if (kind == 0)
			accent = Color.FromSRGBA(61, 163, 93, 255);
		Cell(root, 0, 0, 520, 150, string.Empty, Color.FromSRGBA(18, 20, 22, 235), Color.FromSRGBA(255, 255, 255, 255), 0, 10, 0);
		Cell(root, 0, 0, 5, 150, string.Empty, accent, Color.FromSRGBA(255, 255, 255, 255), 0, 10, 0);
		Cell(root, 16, 6, 500, 26, title, Color.FromSRGBA(0, 0, 0, 0), accent, 1, 18, 0);
		Cell(root, 16, 32, 496, 72, text, Color.FromSRGBA(0, 0, 0, 0), Color.FromSRGBA(232, 232, 228, 255), 0, 16, 0);

		float bx = 16;
		float by = 108;
		Color btnBg = Color.FromSRGBA(232, 163, 61, 255);
		Color btnFg = Color.FromSRGBA(18, 20, 22, 255);
		Color altBg = Color.FromSRGBA(58, 62, 66, 255);
		Color altFg = Color.FromSRGBA(232, 232, 228, 255);
		if (kind == 1)
		{
			Cell(root, bx, by, 160, 34, "Spawn anyway", btnBg, btnFg, 3, 17, Btn(root, spawnerId, 1));
			bx += 168;
			Cell(root, bx, by, 120, 34, "Cancel", altBg, altFg, 3, 17, Btn(root, spawnerId, 3));
			bx += 128;
		}
		if (kind == 2 || kind == 3)
		{
			Cell(root, bx, by, 150, 34, "Show on map", btnBg, btnFg, 3, 17, Btn(root, spawnerId, 100));
			bx += 158;
		}
		if (kind == 3 && cmdId != -1)
		{
			Cell(root, bx, by, 170, 34, "Open commander", altBg, altFg, 3, 17, Btn(root, cmdId, 101));
			bx += 178;
		}
		Cell(root, 520 - 104, by, 88, 34, "Dismiss", altBg, altFg, 3, 16, Btn(root, 0, 102));

		while (m_aItems.Count() > 3)
		{
			DCO_SpawnerCardItem old = m_aItems[m_aItems.Count() - 1];
			m_aItems.Remove(m_aItems.Count() - 1);
			if (old && old.m_wRoot)
				old.m_wRoot.RemoveFromHierarchy();
		}
		Relayout();
	}

	protected void Relayout()
	{
		foreach (int i, DCO_SpawnerCardItem it : m_aItems)
		{
			if (it && it.m_wRoot)
				FrameSlot.SetPos(it.m_wRoot, -540, 130 + i * 160);
		}
	}

	protected void Expire()
	{
		float now = GetGame().GetWorld().GetWorldTime();
		bool changed;
		for (int i = m_aItems.Count() - 1; i >= 0; i--)
		{
			DCO_SpawnerCardItem it = m_aItems[i];
			if (it && it.m_wRoot && now < it.m_fUntil)
				continue;
			if (it && it.m_wRoot)
				it.m_wRoot.RemoveFromHierarchy();
			m_aItems.Remove(i);
			changed = true;
		}
		if (changed)
			Relayout();
	}

	protected void Close(Widget root)
	{
		for (int i = m_aItems.Count() - 1; i >= 0; i--)
		{
			if (m_aItems[i] && m_aItems[i].m_wRoot == root)
			{
				root.RemoveFromHierarchy();
				m_aItems.Remove(i);
			}
		}
		Relayout();
	}

	protected void Handle(int id)
	{
		int idx = id - 900000;
		if (idx < 0 || idx >= m_aBtnAct.Count())
			return;
		int target = m_aBtnTarget[idx];
		int act = m_aBtnAct[idx];
		Widget owner = m_aBtnRoot[idx];
		bool live = false;
		foreach (DCO_SpawnerCardItem it : m_aItems)
		{
			if (it && it.m_wRoot == owner)
				live = true;
		}
		if (!live)
		{
			DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.WARN, "-", string.Format("spawner_card_click_stale act=%1 target=%2", act, target));
			return;
		}
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", string.Format("spawner_card_click act=%1 target=%2", act, target));
		if (act == 102)
		{
			if (owner)
				Close(owner);
			return;
		}
		SCR_EditableEntityComponent ed = SCR_EditableEntityComponent.Cast(Replication.FindItem(target));
		if (!ed || !ed.GetOwner())
			return;
		if (act == 100)
		{
			SCR_ManualCamera cam = SCR_CameraEditorComponent.GetCameraInstance();
			if (cam)
			{
				SCR_TeleportToCursorManualCameraComponent tc = SCR_TeleportToCursorManualCameraComponent.Cast(cam.FindCameraComponent(SCR_TeleportToCursorManualCameraComponent));
				if (tc)
					tc.TeleportCamera(ed.GetOwner().GetOrigin());
			}
			return;
		}
		if (act == 101)
		{
			set<SCR_EditableEntityComponent> one = new set<SCR_EditableEntityComponent>();
			one.Insert(ed);
			SCR_AttributesManagerEditorComponent.DCO_OpenEntities(one);
			if (owner)
				Close(owner);
			return;
		}
		SCR_AttributesManagerEditorComponent mgr = SCR_AttributesManagerEditorComponent.DCO_Get();
		if (mgr)
			mgr.DCO_RequestSpawnerAction(ed, act, 0, 0, string.Empty);
		if (owner)
			Close(owner);
	}
}
