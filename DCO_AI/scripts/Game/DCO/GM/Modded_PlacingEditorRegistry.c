modded class SCR_PlacingEditorComponentClass
{
	protected static const ResourceName DCO_REGISTRY = "{5B8E2C71D0A94F36}Configs/Editor/PlaceableEntities/DCO/DCO_Commander.conf";
	protected static const ResourceName DCO_SYSTEMS_SOURCE_DIR = "{DCF44154CA43E359}PrefabsEditable/System";
	protected static const ResourceName DCO_COMMANDER_PREFAB = "{9D54B8D0C2D6CD12}PrefabsEditable/DCO/Commander/E_AICommander.et";

	protected static const ResourceName DCO_WAYPOINT_REGISTRY = "{3F3CDF4FDF9A8ACB}Configs/Editor/PlaceableEntities/DCO/DCO_Waypoints.conf";
	protected static const ResourceName DCO_VANILLA_DEFEND_WP = "{D9C14ECEC9772CC6}PrefabsEditable/Auto/AI/Waypoints/E_AIWaypoint_Defend.et";
	static const ResourceName DCO_GARRISON_WP = "{BAF7BBB50B01F551}PrefabsEditable/Auto/AI/Waypoints/E_AIWaypoint_Garrison.et";

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

			if (prefabs.Contains(DCO_COMMANDER_PREFAB))
				hasCommander = true;

			if (prefabs.Contains(DCO_GARRISON_WP))
				hasGarrison = true;

			if (prefabs.Contains(DCO_VANILLA_DEFEND_WP))
				isCommandMode = true;

			if (registry.GetSourceDirectory() == DCO_SYSTEMS_SOURCE_DIR)
				isEditMode = true;
		}

		if (isEditMode && !hasCommander && DCO_InjectRegistry(DCO_REGISTRY))
			Print("[DCO] GM content browser: AI Commander prefabs registered");

		if (isCommandMode && !hasGarrison && DCO_InjectRegistry(DCO_WAYPOINT_REGISTRY))
			Print("[DCO] GM commanding: Garrison waypoint registered");
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
	protected static const ResourceName DCO_COMMAND_ACTIONS = "{D3412AC15F7620F6}Configs/Editor/ActionLists/Command/DCO_Command.conf";

	protected ref SCR_EditorActionList m_DCOActions;

	void SCR_BaseActionsEditorComponentClass(IEntityComponentSource componentSource, IEntitySource parentSource, IEntitySource prefabSource)
	{
		int defendIndex = -1;
		foreach (int i, SCR_BaseEditorAction action : m_ActionsSorted)
		{
			SCR_BaseCommandAction cmd = SCR_BaseCommandAction.Cast(action);
			if (!cmd)
				continue;

			ResourceName prefab = cmd.GetCommandPrefab();
			if (prefab == SCR_PlacingEditorComponentClass.DCO_GARRISON_WP)
				return;

			if (prefab.Contains("E_AIWaypoint_Defend.et"))
				defendIndex = i;
		}

		if (defendIndex < 0)
			return;

		Resource res = BaseContainerTools.LoadContainer(DCO_COMMAND_ACTIONS);
		if (!res || !res.IsValid())
			return;

		m_DCOActions = SCR_EditorActionList.Cast(BaseContainerTools.CreateInstanceFromContainer(res.GetResource().ToBaseContainer()));
		if (!m_DCOActions || !m_DCOActions.m_Actions)
			return;

		foreach (int k, SCR_BaseEditorAction dcoAction : m_DCOActions.m_Actions)
			m_ActionsSorted.InsertAt(dcoAction, defendIndex + 1 + k);
	}
}
