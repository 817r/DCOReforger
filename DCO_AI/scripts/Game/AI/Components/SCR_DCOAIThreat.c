enum DCO_ThreatSystem
{
	SAFE,
	ALERTED,
	VIGILANT,
	SUPPRESSED,
	PINNED
};

enum DCO_MoraleSystem
{
};

typedef func SCR_DCOAIThreatStateChangedCallback;
void SCR_DCOAIThreatStateChangedCallback(EAIThreatState prevState, EAIThreatState newState);
typedef ScriptInvokerBase<SCR_DCOAIThreatStateChangedCallback> SCR_DCOAIThreatStateChangedInvoker;

class SCR_DCOAIThreat
{
	private static const float BaseDropRate = 1.2;
	private static const float SuppressionDropRate = 0.2;
	private static const float PinnedDropRate = 0.1;
	
	private static const float fixedBullet = 5;
	private static const float fixedBleed = 3;
	private static const float fixedExplosion = 10;
	
	private static const float FAR_INCREMENT = 1;
	private static const float CLOSE_INCREMENT = 2.5;
	
	private static const float ALERTED_INCREMENT = 1;
	private static const float VIGILANT_INCREMENT = 1.2;
	private static const float SUPPRESSED_INCREMENT = 1.5;
	private static const float PINNED_INCREMENT = 2;
	
	private static const float ALERTED_THRESHOLD = 12;
	private static const float VIGILANT_THRESHOLD = 24;
	private static const float SUPPRESSED_THRESHOLD = 75;
	private static const float PINNED_THRESHOLD = 120;
};