[BaseContainerProps(), SCR_BaseContainerCustomTitleUIInfo("m_Info")]
class DCO_HoldPositionContextAction : SCR_SelectedEntitiesContextAction
{
	override int GetParam()
	{
		return GetGame().GetPlayerController().GetPlayerId();
	}
	
	override bool CanBeShown(SCR_EditableEntityComponent selectedEntity, vector cursorWorldPosition, int flags)
	{
		if (!selectedEntity)
			return false;
		
		return GetDCOConfig(selectedEntity) != null;
	}
	
	override bool CanBePerformed(SCR_EditableEntityComponent selectedEntity, vector cursorWorldPosition, int flags)
	{
		return CanBeShown(selectedEntity, cursorWorldPosition, flags);
	}
	
	override void Perform(SCR_EditableEntityComponent hoveredEntity, notnull set<SCR_EditableEntityComponent> selectedEntities, vector cursorWorldPosition, int flags, int param = -1)
	{
		if (!InitPerform())
			return;
		
		foreach (SCR_EditableEntityComponent entity : selectedEntities)
		{
			if (!CanBePerformed(entity, cursorWorldPosition, flags))
				continue;
			
			DCO_AIConfigComponent conf = GetDCOConfig(entity);
			if (!conf)
				continue;
			
			conf.ToggleHoldPosition();
		}
	}
	
	protected DCO_AIConfigComponent GetDCOConfig(SCR_EditableEntityComponent selectedEntity)
	{
		if (!selectedEntity)
			return null;
		
		IEntity owner = selectedEntity.GetOwner();
		if (!owner)
			return null;
		
		SCR_AICombatComponent combatComp = SCR_AICombatComponent.Cast(owner.FindComponent(SCR_AICombatComponent));
		if (!combatComp)
			return null;
		
		SCR_AIUtilityComponent util = combatComp.GetUtilityComponent();
		if (!util)
			return null;
		
		return util.m_DCOConfig;
	}
};