class DCO_IsWeaponDisposeable : AITaskScripted
{

	protected SCR_AICombatComponent m_CombatComponent;

	override void OnInit(AIAgent owner)
	{
	}

	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		BaseWeaponComponent weaponComp;
		GetVariableIn("WeaponComponent", weaponComp);

		if (!weaponComp) ENodeResult.FAIL;

		BaseMuzzleComponent muzzleComp = weaponComp.GetCurrentMuzzle();

		bool isDisposeable = muzzleComp.IsDisposable();

		SetVariableOut("IsDisposeable", isDisposeable);

		return ENodeResult.SUCCESS;
	}

	static override bool VisibleInPalette()
	{
		return true;
	}

	protected static ref TStringArray s_aVarsOut;
	protected static TStringArray GetVarsOut()
	{
		if (!s_aVarsOut)
			s_aVarsOut = { "IsDisposeable", };
		return s_aVarsOut;
	}

	protected static ref TStringArray s_aVarsIn;
	protected static TStringArray GetVarsIn()
	{
		if (!s_aVarsIn)
			s_aVarsIn = { "WeaponComponent", };
		return s_aVarsIn;
	}

	override TStringArray GetVariablesOut()
	{
		return GetVarsOut();
	}

	override TStringArray GetVariablesIn()
	{
		return GetVarsIn();
	}
}
