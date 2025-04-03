class DCO_PickMagazine : AITaskScripted
{	
	[Attribute("3", UIWidgets.Slider, "Amount of magazines", "0 10 1" )]
	protected int m_iMagazineCount;

	private IEntity m_OwnerEntity;
	private SCR_InventoryStorageManagerComponent m_Inventory;
	private SCR_AICombatComponent m_CombatComponent;
	
	protected static ref TStringArray s_aVarsIn = {
	};
	
	override TStringArray GetVariablesIn()
    {
        return s_aVarsIn;
    }
	
	override void OnInit(AIAgent owner)
	{
		m_OwnerEntity = owner.GetControlledEntity();
		if (!m_OwnerEntity)
			Debug.Error("Owner must be a character!");
		
		m_Inventory = SCR_InventoryStorageManagerComponent.Cast(m_OwnerEntity.FindComponent(SCR_InventoryStorageManagerComponent));		
		m_CombatComponent = SCR_AICombatComponent.Cast(m_OwnerEntity.FindComponent(SCR_AICombatComponent));
	}	
	
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		if (!m_Inventory)
			ENodeResult.FAIL;		

		BaseWeaponComponent weap = m_CombatComponent.GetCurrentWeaponManager().GetCurrentWeapon();
		//SCR_AIDebugVisualization.VisualizeMessage(m_OwnerEntity, weap.GetUIInfo().GetName(), EAIDebugCategory.INFO, 1.4, Color.White);		

		int lowMagThreshold = m_CombatComponent.GetWeaponLowMagThreshold(weap);

		BaseMagazineWell m_sMagazineWellType = weap.GetCurrentMagazine().GetMagazineWell();
		m_CombatComponent.getInventoryStorageMan().ResupplyMagazines(lowMagThreshold + m_iMagazineCount);
		m_CombatComponent.getCharacterController().TryPlayItemGesture(EItemGesture.EItemGesturePickUp);
		// m_CombatComponent.getCharacterController().ReloadWeapon();
		
		return ENodeResult.SUCCESS;			
	}	
		
	protected override static bool VisibleInPalette()
	{
		return true;
	}
};