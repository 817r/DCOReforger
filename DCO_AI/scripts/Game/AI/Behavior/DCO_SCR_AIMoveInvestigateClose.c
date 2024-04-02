class SCR_AIMoveAndInvestigateCloseBehavior : SCR_AIMoveBehaviorBase
{
	ref SCR_BTParam<bool> m_bIsDangerous = new SCR_BTParam<bool>(SCR_AIActionTask.IS_DANGEROUS_PORT);
	ref SCR_BTParam<float> m_fRadius = new SCR_BTParam<float>(SCR_AIActionTask.RADIUS_PORT);
	ref SCR_BTParam<bool> m_bResetTimer = new SCR_BTParam<bool>(SCR_AIActionTask.RESET_TIMER_PORT);
	ref SCR_BTParam<float> m_fDuration = new SCR_BTParam<float>("Duration"); // How much to investigate once we have arrived
	
	EAIUnitType m_eTargetUnitType;
	float m_fTimeStamp;													// world time when constructor of behavior is called
	bool m_bCanTimeout = true;											// can timeout when not executed?
	protected static const float INVESTIGATION_TIMEOUT_MS = 20000;		// how long it can take NOT to investigate before the investigation becomes obsolete in ms
	
	//-----------------------------------------------------------------------------------------------------
	void SCR_AIMoveAndInvestigateCloseBehavior(SCR_AIUtilityComponent utility, SCR_AIActivityBase groupActivity, vector pos, float priority = PRIORITY_CUSTOM_BEHAVIOR_INVESTIGATE_CLOSE, float priorityLevel = PRIORITY_LEVEL_NORMAL, float radius = 10, bool isDangerous = true, EAIUnitType targetUnitType = EAIUnitType.UnitType_Infantry, float duration = 10.0)
	{
		m_bIsDangerous.Init(this, isDangerous);
		m_fRadius.Init(this, radius);
		m_bResetTimer.Init(this, false);
		//m_fTimeOut.Init(this, Math.RandomFloat(20,50));
		m_fDuration.Init(this, Math.RandomFloat(0.8*duration, 1.2*duration));
		m_eTargetUnitType = targetUnitType;
		
		m_sBehaviorTree = "{802BA7E7B6A62323}AI/BehaviorTrees/Chimera/Soldier/Custom/DCO_InvestigateClose.bt";
		if (m_Utility)
		{
			// marking time of creation of this move and investigate (world time)
			m_fTimeStamp = GetGame().GetWorld().GetWorldTime();
		}
		
		// If target is dangerous, during execution of this action we will increase our threat level
		// Aim of this is to be in alerted state through the action, so that when we encounter enemy again,
		// We are not 'surprised' by enemy and there will be no extra delay added
		if (isDangerous)
			m_fThreat = 1.01 * SCR_AIThreatSystem.VIGILANT_THRESHOLD;
	}
	
	//-----------------------------------------------------------------------------------------------------
	override void OnActionSelected()
	{
		super.OnActionSelected();
		m_bCanTimeout = false;
		m_Utility.m_CombatComponent.SetExpectedEnemyType(m_eTargetUnitType);
	}
};

class SCR_AIMoveAndInvestigateCloseBehaviorParameters : SCR_AISetActionParameters
{
	protected static ref TStringArray s_aVarsIn = (new SCR_AIMoveAndInvestigateCloseBehavior(null, null, vector.Zero)).GetPortNames();
	override TStringArray GetVariablesIn() { return s_aVarsIn; }
	override bool VisibleInPalette() { return true; }
};

class SCR_AIGetInvestigateCloseBehaviorParameters : SCR_AIGetActionParameters
{
	static ref TStringArray s_aVarsOut = (new SCR_AIMoveAndInvestigateCloseBehavior(null, null, vector.Zero)).GetPortNames();
	override TStringArray GetVariablesOut() { return s_aVarsOut; }
	
	override bool VisibleInPalette() { return true; }
};
