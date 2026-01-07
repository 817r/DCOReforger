class DCO_IsWeaponDisposeable : AITaskScripted
{
	// Output ports
	protected static const string PORT_DISPOSEABLE = "IsDisposeable";
	protected static const string PORT_WEAPON_COMPONENT = "WeaponComponent";

	protected SCR_AICombatComponent m_CombatComponent;
	
	//------------------------------------------------------------------------------------------------
	override void OnInit(AIAgent owner)
	{

	}
	
	//------------------------------------------------------------------------------------------------
	/*
	It's not very nice that we are moving weapon properties data all the way through weapon selection, combat component, and then this node.
	Ideally we should be able to get all this from weapon, muzzle and magazine directly.
	But ATM this is not possible, therefore it's done this way so that the values are consistent with what weapon selector is using
	to evaluate weapons and targets.
	*/
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
	
	//------------------------------------------------------------------------------------------------
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
	
	//------------------------------------------------------------------------------------------------
	override TStringArray GetVariablesOut()
	{
		return s_aVarsOut;
	}
	
	override TStringArray GetVariablesIn()
	{
		return s_aVarsIn;
	}
}
