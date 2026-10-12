[BaseContainerProps(), SCR_BaseContainerCustomTitleUIInfo("m_Info")]
class DCO_SettingsToolbarAction : SCR_EditorToolbarAction
{
	override bool IsServer()
	{
		return false;
	}

	override bool CanBeShown(SCR_EditableEntityComponent hoveredEntity, notnull set<SCR_EditableEntityComponent> selectedEntities, vector cursorWorldPosition, int flags)
	{
		return true;
	}

	override bool CanBePerformed(SCR_EditableEntityComponent hoveredEntity, notnull set<SCR_EditableEntityComponent> selectedEntities, vector cursorWorldPosition, int flags)
	{
		return GetGame().GetGameMode() != null;
	}

	override void Perform(SCR_EditableEntityComponent hoveredEntity, notnull set<SCR_EditableEntityComponent> selectedEntities, vector cursorWorldPosition, int flags, int param = -1)
	{
		DCO_SettingsChooserUI.LogOpenPath("toolbar");
		SCR_AttributesManagerEditorComponent.DCO_OpenGlobal();
	}
}

[BaseContainerProps(), SCR_BaseContainerCustomTitleUIInfo("m_Info")]
class DCO_SettingsContextAction : SCR_SelectedEntitiesContextAction
{
	override bool CanBeShown(SCR_EditableEntityComponent selectedEntity, vector cursorWorldPosition, int flags)
	{
		if (!selectedEntity)
			return false;
		SCR_AttributesManagerEditorComponent manager = SCR_AttributesManagerEditorComponent.DCO_Get();
		if (!manager)
			return false;
		array<Managed> items = {selectedEntity};
		return manager.DCO_CanOpen(items, true);
	}

	override bool CanBePerformed(SCR_EditableEntityComponent selectedEntity, vector cursorWorldPosition, int flags)
	{
		return CanBeShown(selectedEntity, cursorWorldPosition, flags);
	}

	override void Perform(SCR_EditableEntityComponent hoveredEntity, notnull set<SCR_EditableEntityComponent> selectedEntities, vector cursorWorldPosition, int flags, int param = -1)
	{
		DCO_SettingsChooserUI.LogOpenPath("context");
		if (selectedEntities.IsEmpty() && hoveredEntity)
		{
			set<SCR_EditableEntityComponent> single = new set<SCR_EditableEntityComponent>();
			single.Insert(hoveredEntity);
			SCR_AttributesManagerEditorComponent.DCO_OpenEntities(single);
			return;
		}
		SCR_AttributesManagerEditorComponent.DCO_OpenEntities(selectedEntities);
	}
}
