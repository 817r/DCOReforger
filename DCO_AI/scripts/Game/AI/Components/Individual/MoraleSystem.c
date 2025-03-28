class DCO_AIMoraleSystemComponentClass : ScriptComponentClass
{
}

enum MoraleState
{
	FRESH,
	NORMAL,
	STRESSED,
	PRESSURED,
	BREAK
}

class DCO_AIMoraleSystemComponent : ScriptComponent
{
	[Attribute("0", UIWidgets.ComboBox, "AI Default Morale", "", ParamEnumArray.FromEnum(MoraleState) )]
	protected MoraleState m_MoraleState;
	
	protected float COMBAT_MORALE_DECREASE 				= 0.003 * 0.001;
	protected float COMBAT_MORALE_KIA					= 0.005 * 0.001;
	protected float COMBAT_MORALE_SUPPRESSED			= 0.002 * 0.001;
	
	protected float MORALE_RECOVERY_FIXED				= 0.004 * 0.001;
	protected float MORALE_BOOST_RECOVERY_LEADER_NEAR	= 0.0015 * 0.001;
}