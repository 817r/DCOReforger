class DCO_IsWeaponDisposeable : AITaskScripted
{
	protected static const string PORT_DISPOSEABLE = "IsDisposeable";
	protected static const string PORT_WEAPON_COMPONENT = "WeaponComponent";

	protected SCR_AICombatComponent m_CombatComponent;

	override void OnInit(AIAgent owner)
	{
	}

	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		BaseWeaponComponent weaponComp;
		GetVariableIn(PORT_WEAPON_COMPONENT, weaponComp);

		if (!weaponComp) ENodeResult.FAIL;

		BaseMuzzleComponent muzzleComp = weaponComp.GetCurrentMuzzle();

		bool isDisposeable = muzzleComp.IsDisposable();

		SetVariableOut(PORT_DISPOSEABLE, isDisposeable);

		return ENodeResult.SUCCESS;
	}

	static override bool VisibleInPalette()
	{
		return true;
	}

	protected static ref TStringArray s_aVarsOut = {
		PORT_DISPOSEABLE,
	};

	protected static ref TStringArray s_aVarsIn = {
		PORT_WEAPON_COMPONENT,
	};

	override TStringArray GetVariablesOut()
	{
		return s_aVarsOut;
	}

	override TStringArray GetVariablesIn()
	{
		return s_aVarsIn;
	}
}
