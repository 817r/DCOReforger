class DCO_AIEvadeBehaviorParameters : SCR_AIGetActionParameters
{
	static ref TStringArray s_aVarsOut = (new DCO_AIEvasiveBehavior(null, null, vector.Zero, null)).GetPortNames();
	override TStringArray GetVariablesOut() { return s_aVarsOut; }
	
	override bool VisibleInPalette() { return true; }
};