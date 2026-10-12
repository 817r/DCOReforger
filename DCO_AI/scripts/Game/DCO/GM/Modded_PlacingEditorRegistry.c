modded class SCR_PlacingEditorComponentClass
{


	void SCR_PlacingEditorComponentClass(IEntityComponentSource componentSource, IEntitySource parentSource, IEntitySource prefabSource)
	{
		if (!m_Registries)
			return;

		bool isEditMode = false;
		bool isCommandMode = false;
		bool hasCommander = false;
		bool hasGarrison = false;
		foreach (SCR_PlaceableEntitiesRegistry registry : m_Registries)
		{
			if (!registry || !registry.GetPrefabs())
				continue;

			array<ResourceName> prefabs = registry.GetPrefabs();

			if (prefabs.Contains("{9D54B8D0C2D6CD12}PrefabsEditable/DCO/Commander/E_AICommander.et"))
				hasCommander = true;

			if (prefabs.Contains("{BAF7BBB50B01F551}PrefabsEditable/Auto/AI/Waypoints/E_AIWaypoint_Garrison.et"))
				hasGarrison = true;

			if (prefabs.Contains("{D9C14ECEC9772CC6}PrefabsEditable/Auto/AI/Waypoints/E_AIWaypoint_Defend.et"))
				isCommandMode = true;

			if (registry.GetSourceDirectory() == "{DCF44154CA43E359}PrefabsEditable/System")
				isEditMode = true;
		}

		if (isEditMode && !hasCommander && DCO_InjectRegistry("{5B8E2C71D0A94F36}Configs/Editor/PlaceableEntities/DCO/DCO_Commander.conf"))
			DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", "GM content browser: AI Commander prefabs registered");

		if (isCommandMode && !hasGarrison && DCO_InjectRegistry("{3F3CDF4FDF9A8ACB}Configs/Editor/PlaceableEntities/DCO/DCO_Waypoints.conf"))
			DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", "GM commanding: Garrison waypoint registered");
	}

	protected bool DCO_InjectRegistry(ResourceName config)
	{
		Resource res = BaseContainerTools.LoadContainer(config);
		if (!res || !res.IsValid())
			return false;

		SCR_PlaceableEntitiesRegistry dcoRegistry = SCR_PlaceableEntitiesRegistry.Cast(BaseContainerTools.CreateInstanceFromContainer(res.GetResource().ToBaseContainer()));
		if (!dcoRegistry || !dcoRegistry.GetPrefabs())
			return false;

		m_Registries.Insert(dcoRegistry);
		m_aIndexes.Insert(m_iPrefabCount);
		m_iPrefabCount += dcoRegistry.GetPrefabs().Count();
		return true;
	}
}

modded class SCR_BaseActionsEditorComponentClass
{

	protected ref SCR_EditorActionList m_DCOActions;

	void SCR_BaseActionsEditorComponentClass(IEntityComponentSource componentSource, IEntitySource parentSource, IEntitySource prefabSource)
	{
		bool hasDefend;
		int lastWaypoint = -1;
		foreach (int i, SCR_BaseEditorAction action : m_ActionsSorted)
		{
			SCR_BaseCommandAction cmd = SCR_BaseCommandAction.Cast(action);
			if (!cmd)
				continue;

			ResourceName prefab = cmd.GetCommandPrefab();
			if (prefab == "{BAF7BBB50B01F551}PrefabsEditable/Auto/AI/Waypoints/E_AIWaypoint_Garrison.et")
				return;

			if (prefab.Contains("E_AIWaypoint_Defend.et"))
				hasDefend = true;
			if (action.GetActionGroup() == EEditorActionGroup.COMMAND_WAYPOINT)
				lastWaypoint = i;
		}

		if (!hasDefend || lastWaypoint < 0)
			return;

		Resource res = BaseContainerTools.LoadContainer("{D3412AC15F7620F6}Configs/Editor/ActionLists/Command/DCO_Command.conf");
		if (!res || !res.IsValid())
			return;

		m_DCOActions = SCR_EditorActionList.Cast(BaseContainerTools.CreateInstanceFromContainer(res.GetResource().ToBaseContainer()));
		if (!m_DCOActions || !m_DCOActions.m_Actions)
			return;

		foreach (int k, SCR_BaseEditorAction dcoAction : m_DCOActions.m_Actions)
			m_ActionsSorted.InsertAt(dcoAction, lastWaypoint + 1 + k);
	}
}

class DCO_WpCategoryEntry : ScriptedWidgetEventHandler
{
	protected SCR_CommandToolbarEditorUIComponent m_Toolbar;
	protected int m_iPage;

	void DCO_WpCategoryEntry(SCR_CommandToolbarEditorUIComponent toolbar, int page)
	{
		m_Toolbar = toolbar;
		m_iPage = page;
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (button == 0 && m_Toolbar)
			m_Toolbar.DCO_SelectPage(m_iPage);
		return true;
	}

	override bool OnMouseEnter(Widget w, int x, int y)
	{
		Highlight(w, true);
		return false;
	}

	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		Highlight(w, false);
		return false;
	}

	protected void Highlight(Widget w, bool on)
	{
		Widget bg = w.FindAnyWidget("Bg");
		if (!bg)
			return;
		if (on)
			bg.SetColor(new Color(0.016, 0.552, 0.905, 1));
		else
			bg.SetColor(new Color(0, 0, 0, 1));
	}
}

modded class SCR_CommandToolbarEditorUIComponent
{
	protected Widget m_wDCOTitle;
	protected Widget m_wDCOPopup;
	protected int m_iDCOFirstPage = -1;
	protected ref array<ref DCO_WpCategoryEntry> m_aDCOEntries = {};

	override void HandlerAttachedScripted(Widget w)
	{
		m_wDCOTitle = GetGame().GetWorkspace().CreateWidgets("{8C2E5A7D1B4F9036}UI/layouts/Editor/Toolbar/DCO_CommandMenu_PageTitle.layout", w);
		if (m_wDCOTitle)
			m_wDCOTitle.SetZOrder(-1);
		super.HandlerAttachedScripted(w);
	}

	override void HandlerDeattached(Widget w)
	{
		DCO_ClosePopup();
		super.HandlerDeattached(w);
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (m_wDCOTitle && w == m_wDCOTitle)
		{
			if (button == 0)
				DCO_TogglePopup();
			return true;
		}
		return super.OnClick(w, x, y, button);
	}

	void DCO_SelectPage(int page)
	{
		DCO_ClosePopup();
		if (m_Pagination)
			m_Pagination.SetPage(page);
	}

	protected void DCO_ClosePopup()
	{
		if (m_wDCOPopup)
			m_wDCOPopup.RemoveFromHierarchy();
		m_wDCOPopup = null;
		m_aDCOEntries.Clear();
	}

	protected void DCO_TogglePopup()
	{
		if (m_wDCOPopup)
		{
			DCO_ClosePopup();
			return;
		}

		MenuRootBase menu = GetMenu();
		if (!menu || !menu.GetRootWidget() || !m_Pagination)
			return;

		WorkspaceWidget ws = GetGame().GetWorkspace();
		m_wDCOPopup = ws.CreateWidgets("{8C2E5A7D1B4F9040}UI/layouts/Editor/Toolbar/DCO_CommandMenu_CategoryPopup.layout", menu.GetRootWidget());
		if (!m_wDCOPopup)
			return;

		DCO_AddCategory("Vanilla WP", 0);
		if (m_iDCOFirstPage > 0)
			DCO_AddCategory("DCO WP", m_iDCOFirstPage);

		float x, y;
		m_wDCOTitle.GetScreenPos(x, y);
		FrameSlot.SetAlignment(m_wDCOPopup, 0, 1);
		FrameSlot.SetSizeToContent(m_wDCOPopup, true);
		FrameSlot.SetPos(m_wDCOPopup, ws.DPIUnscale(x), ws.DPIUnscale(y) - 4);
		m_wDCOPopup.SetZOrder(1000);
	}

	protected void DCO_AddCategory(string label, int page)
	{
		Widget entry = GetGame().GetWorkspace().CreateWidgets("{8C2E5A7D1B4F9050}UI/layouts/Editor/Toolbar/DCO_CommandMenu_CategoryEntry.layout", m_wDCOPopup);
		if (!entry)
			return;

		TextWidget text = TextWidget.Cast(entry.FindAnyWidget("Text"));
		if (text)
			text.SetText(label);

		DCO_WpCategoryEntry handler = new DCO_WpCategoryEntry(this, page);
		entry.AddHandler(handler);
		m_aDCOEntries.Insert(handler);
	}

	override protected void Refresh()
	{
		int actionFlags;
		m_EditorActionsComponent.ValidateSelection(false);
		int count = m_EditorActionsComponent.GetAndEvaluateActions(vector.Zero, m_aActionData, actionFlags);
		m_aShortcuts.Clear();

		for (int i = count - 1; i >= 0; i--)
		{
			if (!m_ActionGroups.Contains(m_aActionData[i].GetAction().GetActionGroup()))
			{
				m_aActionData.Remove(i);
				count--;
			}
		}

		m_iDCOFirstPage = -1;
		int perPage;
		if (m_Pagination)
			perPage = m_Pagination.GetRows() * m_Pagination.GetColumns();
		if (perPage > 0)
		{
			for (int d = 1; d < count; d++)
			{
				if (m_aActionData[d].GetAction().GetOrder() < 1000)
					continue;
				int pad = (perPage - d % perPage) % perPage;
				for (int k = 0; k < pad; k++)
				{
					m_aActionData.InsertAt(null, d);
				}
				count += pad;
				m_iDCOFirstPage = (d + pad) / perPage;
				break;
			}
		}

		bool hasContent;
		if (m_Pagination)
		{
			m_Pagination.SetEntryCount(count);
			hasContent = m_Pagination.RefreshPage();
		}
		else
		{
			DeleteAllItems();
			ShowEntries(m_ItemsWidget, 0, int.MAX);
			hasContent = m_ItemsWidget.GetChildren() != null;
		}
		SetToolbarVisible(hasContent);
		m_queuedRefresh = false;
	}

	override protected void ShowEntries(Widget contentWidget, int indexStart, int indexEnd)
	{
		if (!GetGame().GetInputManager().IsUsingMouseAndKeyboard())
			return;

		int shortcutIndex;
		bool dcoPage;
		indexEnd = Math.Min(indexEnd, m_aActionData.Count());
		for (int i = indexStart; i < indexEnd; i++)
		{
			if (!m_aActionData[i])
				continue;
			if (shortcutIndex == 0)
				dcoPage = m_aActionData[i].GetAction().GetOrder() >= 1000;
			CreateItem(m_aActionData[i], shortcutIndex);
			shortcutIndex++;
		}

		if (!m_wDCOTitle)
			return;
		m_wDCOTitle.SetVisible(m_iDCOFirstPage > 0);
		TextWidget text = TextWidget.Cast(m_wDCOTitle.FindAnyWidget("Text"));
		if (!text)
			return;
		if (dcoPage)
			text.SetText("DCO\nWP");
		else
			text.SetText("Vanilla\nWP");
	}
}
