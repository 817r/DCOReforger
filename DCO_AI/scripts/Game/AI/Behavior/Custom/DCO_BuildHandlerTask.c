class DCO_GetBuildingType: AITaskScripted
{
	static const string BUILDING_TYPE_PORT = "Building Types";
	
	//------------------------------------------------------------------------------------------------
	override bool VisibleInPalette() {return true;}
	
	//------------------------------------------------------------------------------------------------
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		string bType = "SandbagBarrier";
		SetVariableOut(BUILDING_TYPE_PORT, bType);
		return ENodeResult.SUCCESS;
	}
	
	//------------------------------------------------------------------------------------------------
	protected static ref TStringArray s_aVarsOut = { BUILDING_TYPE_PORT };
	override TStringArray GetVariablesOut() { return s_aVarsOut; }
	
	protected static ref TStringArray s_aVarsIn = { };
	override TStringArray GetVariablesIn() { return s_aVarsIn; }
	

	//------------------------------------------------------------------------------------------------
	override string GetOnHoverDescription() 
	{ 
		return "Get Building Type";	
	};
};
