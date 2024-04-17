enum moraleState
{
	BREAK,
	NORMAL,
	WISE,
	MOTIVATED,
	MANIAC
}

typedef func SCR_AIMoraleStateChangedCallback;
void SCR_AIMoraleStateChangedCallback(moraleState prevState, moraleState newState);
typedef ScriptInvokerBase<SCR_AIMoraleStateChangedCallback> SCR_AIMoraleStateChangedInvoker;

class DCO_AIMoraleSystem
{
	// it should be like this DROP is the number going up 
	// RECOVERY is the number going down
	
	// Courage is the resistance to Morale 
	
	// Round classification (EWeaponType) classification
	
	private static const float MORALE_SHOT_RECOVERY 			= 			0.1 * 0.001;	//!< Falloff (percentual drop per milisecond)
	private static const float MORALE_SUPPRESSION_RECOVERY 		= 			0.1 * 0.001;
	private static const float MORALE_ENDANGERED_RECOVERY 		= 			0.2 * 0.001;
	
	private static const float MORALE_BOOST_LEADER_DISTANCE		=			50;
	private static const float MORALE_BOOST_FRIENDLY_DISTANCE	=			10;
	
	private static const float MORALE_BOOST_LEADER_VALUE		=			0.11;
	private static const float MORALE_BOOST_FRIENDLY_VALUE		=			0.035;
	
	private static const float MORALE_DROP_SUPPRESSION			=			0.12;
	private static const float MORALE_DROP_FIREFIGHT_FIXED		=			0.2;

	
}