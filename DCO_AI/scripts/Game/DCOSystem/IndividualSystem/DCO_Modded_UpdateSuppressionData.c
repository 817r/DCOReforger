modded class SCR_AIUpdateTargetSuppressionData
{
	// Inputs
	protected static const string PORT_SUPPRESSION_VOLUME = "SuppressionVolume";
	
	// Outputs
	protected static const string PORT_VISIBLE = "Visible";
	protected static const string PORT_TIME_LAST_SEEN = "TimeLastSeen_ms";
	protected static const string PORT_FIRE_TREE_ID = "FireTreeId";	
	
	// These IDs must match to actual trees in the tree
	protected const int FIRE_TREE_INVALID 		= -1;	// No aiming or firing is allowed at all
	protected const int FIRE_TREE_LOOK			= 0;	// Looking at target without firing
	protected const int FIRE_TREE_SUPPRESSIVE	= 1;
	protected const int FIRE_TREE_GRENADE		= 2;
	protected const int FIRE_TREE_RPG			= 3;
	
	// Related to visibility check
	protected float m_fVisibilityCheckTimer = VISIBILITY_CHECK_INTERVAL_S; // We need the vision check to run right on start. This data is required by movement logic.
	protected bool m_bTargetVisible = false;
	protected float m_fTargetLastSeenTime_ms = 0; // World time
	protected ref TraceParam m_TraceParam;
	protected ref array<IEntity> m_TraceParamExcludeArray;
	protected const float VISIBILITY_CHECK_INTERVAL_S = 0.75;
	protected const float VISIBILITY_CHECK_TRACE_RESULT_THRESHOLD = 0.5;
	
	// Other
	protected SCR_AIUtilityComponent m_UtilityComponent;
	protected PerceptionComponent m_PerceptionComponent;
	protected SCR_AISuppressionVolumeBase suppressionVolume;
	
	#ifdef WORKBENCH
	protected ref array<ref Shape> m_aDebugShapes = {};
	#endif
	
	//---------------------------------------------------------------------------
	override void OnInit(AIAgent owner)
	{
		m_UtilityComponent = SCR_AIUtilityComponent.Cast(owner.FindComponent(SCR_AIUtilityComponent));
		IEntity myEntity = owner.GetControlledEntity();
		if (myEntity)
			m_PerceptionComponent = PerceptionComponent.Cast(myEntity.FindComponent(PerceptionComponent));
	}
	
	//---------------------------------------------------------------------------
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		
		if (!GetVariableIn(PORT_SUPPRESSION_VOLUME, suppressionVolume) || !suppressionVolume)
			return ENodeResult.FAIL;
		
		IEntity myEntity = owner.GetControlledEntity();
		if (!myEntity || !m_UtilityComponent || !m_PerceptionComponent)
			return ENodeResult.FAIL;
		
		m_fVisibilityCheckTimer += dt;		
		if (m_fVisibilityCheckTimer >= VISIBILITY_CHECK_INTERVAL_S)
		{
			m_bTargetVisible = CheckTargetVisibility(myEntity, suppressionVolume);
			
			if (m_bTargetVisible)
				m_fTargetLastSeenTime_ms = GetGame().GetWorld().GetWorldTime();
			
			m_fVisibilityCheckTimer = 0;
		}
		
		int fireTreeid = ResolveFireTree(m_bTargetVisible);
		
		// Write data to ports
		SetVariableOut(PORT_VISIBLE, m_bTargetVisible);
		SetVariableOut(PORT_TIME_LAST_SEEN, m_fTargetLastSeenTime_ms);
		SetVariableOut(PORT_FIRE_TREE_ID, fireTreeid);
		
		return ENodeResult.SUCCESS;
	}
	
	//---------------------------------------------------------------------------
	override int ResolveFireTree(bool targetVisible)
	{
		// Is aiming forbidden by combat move?
		SCR_AIBehaviorBase executedBehavior = SCR_AIBehaviorBase.Cast(m_UtilityComponent.GetExecutedAction());
		if (executedBehavior && executedBehavior.m_bUseCombatMove && !m_UtilityComponent.m_CombatMoveState.m_bAimAtTarget)
			return FIRE_TREE_INVALID;
		
		// Friendly in aim?
		if (m_PerceptionComponent.GetFriendlyInLineOfFire())
			return FIRE_TREE_LOOK;
		
		if (targetVisible)
			return FIRE_TREE_SUPPRESSIVE;
		else if (!targetVisible && m_UtilityComponent.m_CombatComponent.HasWeaponOfType(EWeaponType.WT_ROCKETLAUNCHER) && vector.Distance(m_UtilityComponent.GetOrigin(), suppressionVolume.GetCenterPosition()) > 10)
			return FIRE_TREE_RPG;
		else if (!targetVisible && m_UtilityComponent.m_AIInfo.HasRole(EUnitRole.HAS_FRAG_GRENADE) && vector.Distance(m_UtilityComponent.GetOrigin(), suppressionVolume.GetCenterPosition()) < 30)
		{
			SCR_AIThrowGrenadeToBehavior gren = new SCR_AIThrowGrenadeToBehavior(m_UtilityComponent, null, suppressionVolume.GetCenterPosition(), EWeaponType.WT_FRAGGRENADE, 1, SCR_AIThrowGrenadeToBehavior.PRIORITY_BEHAVIOR_THROW_GRENADE + 
			SCR_AIThrowGrenadeToBehavior.PRIORITY_LEVEL_PLAYER);
			m_UtilityComponent.AddAction(gren);		
			return FIRE_TREE_LOOK;
		}
		
		return FIRE_TREE_LOOK;
	}
	
	//---------------------------------------------------------------------------
	protected static ref TStringArray s_aVarsIn = { PORT_SUPPRESSION_VOLUME };
	override TStringArray GetVariablesIn() { return s_aVarsIn; }
	
	protected static ref TStringArray s_aVarsOut = { PORT_VISIBLE, PORT_TIME_LAST_SEEN, PORT_FIRE_TREE_ID };
	override TStringArray GetVariablesOut() { return s_aVarsOut; }
	
	static override bool VisibleInPalette() { return true; }
}