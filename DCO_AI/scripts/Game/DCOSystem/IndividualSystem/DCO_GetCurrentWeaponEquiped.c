class SCR_AIGetCurrentWeaponComponent : AITaskScripted
{
	// Inputs
	protected static const string PORT_PREFAB_RESOURCE_NAME = "PrefabResourceName";
	protected static const string PORT_WEAPON_COMPONENT = "WeaponComponent";
	
	// Used for query
	protected ResourceName m_sQueryResourceName;
	
	protected CharacterControllerComponent m_ControlComp;
	protected BaseWeaponManagerComponent m_WeaponMgrComp;
	protected CompartmentAccessComponent m_CompartmentAccessComp;
	
	//------------------------------------------------------------
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{		
		if (!m_WeaponMgrComp || !m_ControlComp)
			return ENodeResult.FAIL;

		BaseCompartmentSlot compartmentSlot = m_CompartmentAccessComp.GetCompartment();
		BaseWeaponManagerComponent weaponMgr;
		if (compartmentSlot)
			weaponMgr = BaseWeaponManagerComponent.Cast(compartmentSlot.GetOwner().FindComponent(BaseWeaponManagerComponent));
		else
			weaponMgr = m_WeaponMgrComp;
		
		if (!weaponMgr)
			return ENodeResult.FAIL;
		
		BaseWeaponComponent selectedWeaponComp = null;
		array<BaseWeaponComponent> weaponList = {};
		weaponMgr.GetWeapons(weaponList);
		
		foreach (BaseWeaponComponent a : weaponList)
		{
			if (a.GetWeaponType() == EWeaponType.WT_RIFLE || a.GetWeaponType() == EWeaponType.WT_MACHINEGUN)
				selectedWeaponComp = a;
		}
		
		if (!selectedWeaponComp) return ENodeResult.FAIL;

		SetVariableOut(PORT_WEAPON_COMPONENT, selectedWeaponComp);
		return ENodeResult.SUCCESS;
	}
	
	override void OnInit(AIAgent owner)
	{
		IEntity controlledEnt = owner.GetControlledEntity();
		
		m_WeaponMgrComp = BaseWeaponManagerComponent.Cast(controlledEnt.FindComponent(BaseWeaponManagerComponent));
		m_ControlComp = CharacterControllerComponent.Cast(controlledEnt.FindComponent(CharacterControllerComponent));
		m_CompartmentAccessComp = CompartmentAccessComponent.Cast(controlledEnt.FindComponent(CompartmentAccessComponent));
	}
	
	//------------------------------------------------------------
	override static bool VisibleInPalette() { return true; }
	
	protected static ref TStringArray s_aVarsOut = { PORT_WEAPON_COMPONENT };
	override TStringArray GetVariablesOut() { return s_aVarsOut; }
}