class SCR_AIGetCurrentMagazinePrefab : AITaskScripted
{
	protected static string PORT_PREFAB_RESOURCE_NAME() { return "PrefabResourceName"; }
	protected static string PORT_WEAPON_COMPONENT() { return "WeaponComponent"; }

	protected ResourceName m_sQueryResourceName;

	protected CharacterControllerComponent m_ControlComp;
	protected BaseWeaponManagerComponent m_WeaponMgrComp;
	protected CompartmentAccessComponent m_CompartmentAccessComp;

	protected typename magazineWell;

	IEntity controlledEnt;

	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		if (!m_WeaponMgrComp || !m_ControlComp)
			return ENodeResult.FAIL;

		BaseWeaponComponent newWeaponComp = null;
		GetVariableIn(PORT_WEAPON_COMPONENT(), newWeaponComp);
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

		SetVariableOut("MagazineWellArray", magazineWell);
		SetVariableOut(PORT_PREFAB_RESOURCE_NAME(), m_sQueryResourceName);

		return ENodeResult.SUCCESS;
	}

	override void OnInit(AIAgent owner)
	{
		controlledEnt = owner.GetControlledEntity();

		m_WeaponMgrComp = BaseWeaponManagerComponent.Cast(controlledEnt.FindComponent(BaseWeaponManagerComponent));
		m_ControlComp = CharacterControllerComponent.Cast(controlledEnt.FindComponent(CharacterControllerComponent));
		m_CompartmentAccessComp = CompartmentAccessComponent.Cast(controlledEnt.FindComponent(CompartmentAccessComponent));
	}

	override static bool VisibleInPalette() { return true; }

	protected static ref TStringArray s_aVarsIn;
	protected static TStringArray GetVarsIn()
	{
		if (!s_aVarsIn)
			s_aVarsIn = {PORT_WEAPON_COMPONENT()};
		return s_aVarsIn;
	}
	override TStringArray GetVariablesIn() { return GetVarsIn(); }

	protected static ref TStringArray s_aVarsOut;
	protected static TStringArray GetVarsOut()
	{
		if (!s_aVarsOut)
			s_aVarsOut = { PORT_PREFAB_RESOURCE_NAME(), "MagazineWellArray" };
		return s_aVarsOut;
	}
	override TStringArray GetVariablesOut() { return GetVarsOut(); }
}