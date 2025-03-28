class DCO_ForceThrowGrenaded : AITaskScripted
{	
	CharacterControllerComponent charCons;
	
	//------------------------------------------------------------------------------------------------
	override bool VisibleInPalette() {return true;}
	
	//------------------------------------------------------------------------------------------------
	override void OnInit(AIAgent owner)
	{
		IEntity own = owner.GetControlledEntity();
		ChimeraCharacter char = ChimeraCharacter.Cast(own);
		charCons = CharacterControllerComponent.Cast(char.FindComponent(CharacterControllerComponent));
	}
	
	//------------------------------------------------------------------------------------------------
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		if (!charCons)
			return ENodeResult.FAIL;
		
		charCons.SetThrow(true, false);
		return ENodeResult.SUCCESS;
	}
	
	//------------------------------------------------------------------------------------------------
	override protected bool CanReturnRunning() { return false; }
	
	//------------------------------------------------------------------------------------------------
	protected static ref TStringArray s_aVarsOut = {};
	override TStringArray GetVariablesOut() { return s_aVarsOut; }

	//------------------------------------------------------------------------------------------------
	protected static ref TStringArray s_aVarsIn = {};
	override TStringArray GetVariablesIn() { return s_aVarsIn; }
	
	//------------------------------------------------------------------------------------------------
	protected override string GetOnHoverDescription()
	{
		return "Force Throw Grenades";
	}	
}