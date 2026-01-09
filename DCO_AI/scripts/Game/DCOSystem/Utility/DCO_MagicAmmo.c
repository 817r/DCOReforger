class DCO_MagicAmmo : AITaskScripted
{	
	[Attribute("2", UIWidgets.Slider, "Amount of magazines", "0 10 1" )]
	protected int m_iMagazineCount;

	private IEntity m_OwnerEntity;
	private SCR_InventoryStorageManagerComponent m_Inventory;

	
	override void OnInit(AIAgent owner)
	{
		m_OwnerEntity = owner.GetControlledEntity();
		if (!m_OwnerEntity)
			Debug.Error("Owner must be a character!");
		
		m_Inventory = SCR_InventoryStorageManagerComponent.Cast(m_OwnerEntity.FindComponent(SCR_InventoryStorageManagerComponent));		
	}	
	
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		if (!m_Inventory)
			ENodeResult.FAIL;		
		
		m_Inventory.ResupplyMagazines(m_iMagazineCount);
		return ENodeResult.SUCCESS;			
	}	
		
	protected override static bool VisibleInPalette()
	{
		return true;
	}
	
	protected override static string GetOnHoverDescription()
	{
		return "AI Spawn Magically Magazine To Their Inventory";
	}	
};