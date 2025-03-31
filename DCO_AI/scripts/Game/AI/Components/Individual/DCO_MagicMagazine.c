class DCO_MagicAmmo : AITaskScripted
{
	static const string PORT_MAGAZINE_WELL = "MagazineWell";
	
	ref map<string, string> m_mTypenameToResourceName;
	
	[Attribute("", UIWidgets.EditBox, "Name of magazine well" )]
	protected string m_sMagazineWellType;
	
	[Attribute("3", UIWidgets.Slider, "Amount of magazines", "0 10 1" )]
	protected int m_iMagazineCount;

	private IEntity m_OwnerEntity;
	private SCR_InventoryStorageManagerComponent m_Inventory;
	
	protected static ref TStringArray s_aVarsIn = {
		PORT_MAGAZINE_WELL
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
	}	
	
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		if (!m_Inventory)
			ENodeResult.FAIL;		
		
		m_Inventory.ResupplyMagazines(m_iMagazineCount);
		
		array<IEntity> items;
		
		m_Inventory.GetItems(items);
		MagazineComponent magComp;
		
		foreach(IEntity item : items)
		{
			magComp = MagazineComponent.Cast(item.FindComponent(MagazineComponent));
			if (!magComp)
				continue;
			
			if (magComp.GetMagazineWell().ToString() == m_sMagazineWellType)
			{
				ChimeraCharacter.Cast(m_OwnerEntity).GetCharacterController().ReloadWeaponWith(item);
				break;
			}
		}
		
		return ENodeResult.FAIL;			
	}	
		
	protected override static bool VisibleInPalette()
	{
		return true;
	}
	
	protected override static string GetOnHoverDescription()
	{
		return "AI task that picks up all magazines of provided MagazineWell type in the vicinity of its inventory.";
	}	
};