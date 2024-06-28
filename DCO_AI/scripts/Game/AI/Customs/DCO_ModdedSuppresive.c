// Script File//------------------------------------------------------------------------------------------------
modded class SCR_AIDecoSuppressiveFire : DecoratorScripted
{
	static const string TARGET_PORT = "BaseTargetIn";
	
	//-----------------------------------------------------------------------------------------------------
	protected override bool TestFunction(AIAgent owner)
	{
		BaseTarget target;
		GetVariableIn(TARGET_PORT,target);
		
		if (!target)
		{
			return false;
		}
		
		float traceFraction = target.GetTraceFraction();
		float timelastSeen = target.GetTimeLastSeen();
		
		return traceFraction > 0.47 && timelastSeen < 2.0;
		//return true;
	}
	
	//-----------------------------------------------------------------------------------------------------
	protected override bool VisibleInPalette()
	{
		return true;
	}	
	
	//-----------------------------------------------------------------------------------------------------
	protected static ref TStringArray s_aVarsIn = {
		TARGET_PORT
	};
	protected override TStringArray GetVariablesIn()
	{
		return s_aVarsIn;
	}
};
