modded class SCR_PlayerControllerCommandingComponent
{
	protected ref SCR_PlayerCommandingMenuConfig m_DCO_MapMenuConfig;

	override protected void SetupMapRadialMenu()
	{
		super.SetupMapRadialMenu();
		if (!m_MapContextualMenu)
			return;

		Resource holder = BaseContainerTools.LoadContainer("{ECC45EC468D76CF5}Configs/Commanding/CommandingMenu.conf");
		if (!holder || !holder.IsValid())
			return;

		m_DCO_MapMenuConfig = SCR_PlayerCommandingMenuConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(holder.GetResource().ToBaseContainer()));
		if (!m_DCO_MapMenuConfig || !m_DCO_MapMenuConfig.GetRootCategory())
			return;

		foreach (SCR_PlayerCommandingMenuBaseElement element : m_DCO_MapMenuConfig.GetRootCategory().GetCategoryElements())
		{
			SCR_PlayerCommandingMenuCategoryElement category = SCR_PlayerCommandingMenuCategoryElement.Cast(element);
			if (category && category.GetCategoryDisplayText() == "Commander")
			{
				AddElementsFromCategoryToMap(category);
				return;
			}
		}
	}
}
