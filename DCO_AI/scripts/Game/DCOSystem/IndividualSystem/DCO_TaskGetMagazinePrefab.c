class SCR_AIGetCurrentMagazinePrefab : AITaskScripted
{
	// Inputs
	protected static const string PORT_PREFAB_RESOURCE_NAME = "PrefabResourceName";
	protected static const string PORT_WEAPON_COMPONENT = "WeaponComponent";
	protected static const string PORT_MAGAZINE_WELL = "MagazineWellArray";
	
	// Used for query
	protected ResourceName m_sQueryResourceName;
	
	protected CharacterControllerComponent m_ControlComp;
	protected BaseWeaponManagerComponent m_WeaponMgrComp;
	protected CompartmentAccessComponent m_CompartmentAccessComp;
	
	protected typename magazineWell;
	
	IEntity controlledEnt;
	
	//------------------------------------------------------------
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{		
		if (!m_WeaponMgrComp || !m_ControlComp)
			return ENodeResult.FAIL;
		
		BaseWeaponComponent newWeaponComp = null;
		GetVariableIn(PORT_WEAPON_COMPONENT, newWeaponComp);
		// Resolve which weapon manager to use
		BaseCompartmentSlot compartmentSlot = m_CompartmentAccessComp.GetCompartment();
		BaseWeaponManagerComponent weaponMgr;
		if (compartmentSlot)
			weaponMgr = BaseWeaponManagerComponent.Cast(compartmentSlot.GetOwner().FindComponent(BaseWeaponManagerComponent));
		else
			weaponMgr = m_WeaponMgrComp;

		if (!weaponMgr)
			return ENodeResult.FAIL;
		
		BaseWeaponComponent selectedWeaponComp = null;
		if (newWeaponComp)
			selectedWeaponComp = newWeaponComp;
		else
			selectedWeaponComp = weaponMgr.GetCurrentWeapon();
		
		if (selectedWeaponComp)
		{
			BaseMuzzleComponent muzz = selectedWeaponComp.GetCurrentMuzzle();
			if (muzz)
			{
				magazineWell = muzz.GetMagazineWell().Type();
			}
		}
		
		DCO_AIResupplyConfigComponent resConf = DCO_AIResupplyConfigComponent.Cast(controlledEnt.FindComponent(DCO_AIResupplyConfigComponent));
		if (resConf)
			m_sQueryResourceName = resConf.GetRandomMagazinePrefab(selectedWeaponComp.GetWeaponType());		
		
		if (!magazineWell)
			Print("FOUND NO MAGAZINE WELL");
		else
			Print("FOUND MAGAZINE WELL " + magazineWell.ToString());
		
		SetVariableOut(PORT_MAGAZINE_WELL, magazineWell);
		SetVariableOut(PORT_PREFAB_RESOURCE_NAME, m_sQueryResourceName);
		
		return ENodeResult.SUCCESS;
	}
	
	override void OnInit(AIAgent owner)
	{
		controlledEnt = owner.GetControlledEntity();
		
		m_WeaponMgrComp = BaseWeaponManagerComponent.Cast(controlledEnt.FindComponent(BaseWeaponManagerComponent));
		m_ControlComp = CharacterControllerComponent.Cast(controlledEnt.FindComponent(CharacterControllerComponent));
		m_CompartmentAccessComp = CompartmentAccessComponent.Cast(controlledEnt.FindComponent(CompartmentAccessComponent));
	}
	
	//------------------------------------------------------------
	override static bool VisibleInPalette() { return true; }
	
	protected static ref TStringArray s_aVarsIn = {PORT_WEAPON_COMPONENT};
	override TStringArray GetVariablesIn() { return s_aVarsIn; }
	
	protected static ref TStringArray s_aVarsOut = { PORT_PREFAB_RESOURCE_NAME, PORT_MAGAZINE_WELL };
	override TStringArray GetVariablesOut() { return s_aVarsOut; }
}