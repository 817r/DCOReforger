modded class SCR_AIUpdateTargetSuppressionData
{
	protected static const string PORT_SUPPRESSION_VOLUME = "SuppressionVolume";

	protected static const string PORT_VISIBLE = "Visible";
	protected static const string PORT_TIME_LAST_SEEN = "TimeLastSeen_ms";

	protected const int FIRE_TREE_INVALID 		= -1;
	protected const int FIRE_TREE_LOOK			= 0;
	protected const int FIRE_TREE_SUPPRESSIVE	= 1;
	protected const int FIRE_TREE_GRENADE		= 2;
	protected const int FIRE_TREE_RPG			= 3;
	protected const int FIRE_TREE_GL			= 4;

	protected float m_fVisibilityCheckTimer = VISIBILITY_CHECK_INTERVAL_S;
	protected bool m_bTargetVisible = false;
	protected float m_fTargetLastSeenTime_ms = 0;
	protected ref TraceParam m_TraceParam;
	protected ref array<IEntity> m_TraceParamExcludeArray;
	protected const float VISIBILITY_CHECK_INTERVAL_S = 0.75;
	protected const float VISIBILITY_CHECK_TRACE_RESULT_THRESHOLD = 0.5;

	protected SCR_AIUtilityComponent m_UtilityComponent;
	protected PerceptionComponent m_PerceptionComponent;
	protected SCR_AISuppressionVolumeBase suppressionVolume;

	#ifdef WORKBENCH
	protected ref array<ref Shape> m_aDebugShapes = {};
	#endif

	override void OnInit(AIAgent owner)
	{
		m_UtilityComponent = SCR_AIUtilityComponent.Cast(owner.FindComponent(SCR_AIUtilityComponent));
		IEntity myEntity = owner.GetControlledEntity();
		if (myEntity)
			m_PerceptionComponent = PerceptionComponent.Cast(myEntity.FindComponent(PerceptionComponent));
	}

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

		SetVariableOut(PORT_VISIBLE, m_bTargetVisible);
		SetVariableOut(PORT_TIME_LAST_SEEN, m_fTargetLastSeenTime_ms);
		SetVariableOut("FireTreeId", fireTreeid);

		return ENodeResult.SUCCESS;
	}

	override int ResolveFireTree(bool targetVisible)
	{
		SCR_AIBehaviorBase executedBehavior = SCR_AIBehaviorBase.Cast(m_UtilityComponent.GetExecutedAction());
		if (executedBehavior && executedBehavior.m_bUseCombatMove && !m_UtilityComponent.m_CombatMoveState.m_bAimAtTarget)
			return FIRE_TREE_INVALID;

		vector grenadeThrowPos;

		BaseWeaponComponent glWeapon;
		int glMuzzleId;
		if (m_UtilityComponent.m_CombatComponent)
			m_UtilityComponent.m_CombatComponent.GetSelectedWeapon(glWeapon, glMuzzleId);

		if (m_PerceptionComponent.GetFriendlyInLineOfFire())
			return FIRE_TREE_LOOK;

		if (targetVisible)
			return FIRE_TREE_SUPPRESSIVE;
		else if (!targetVisible && m_UtilityComponent.m_CombatComponent.HasWeaponOfType(EWeaponType.WT_ROCKETLAUNCHER) && vector.Distance(m_UtilityComponent.GetOrigin(), suppressionVolume.GetCenterPosition()) > 10)
			return FIRE_TREE_RPG;
		else if (!targetVisible
			&& DCO_UGLUtility.ShouldUseGL(m_UtilityComponent, glWeapon, suppressionVolume.GetCenterPosition()))
			return FIRE_TREE_GL;

		else if (!targetVisible
			&& m_UtilityComponent.m_AIInfo
			&& m_UtilityComponent.m_AIInfo.HasRole(EUnitRole.HAS_FRAG_GRENADE)
			&& DCO_GrenadeUtility.CanThrowGrenadeNow(m_UtilityComponent)
			&& DCO_GrenadeUtility.ResolveThrowPos(m_UtilityComponent, DCO_AimUtility.ScatterExplosive(m_UtilityComponent, suppressionVolume.GetCenterPosition(), false), grenadeThrowPos))
		{
			SCR_AIThrowGrenadeToBehavior gren = new SCR_AIThrowGrenadeToBehavior(m_UtilityComponent, null, grenadeThrowPos, EWeaponType.WT_FRAGGRENADE, 1, SCR_AIThrowGrenadeToBehavior.PRIORITY_BEHAVIOR_THROW_GRENADE +
			SCR_AIThrowGrenadeToBehavior.PRIORITY_LEVEL_PLAYER);
			m_UtilityComponent.AddAction(gren);
			DCO_GrenadeUtility.NotifyGrenadeThrown(m_UtilityComponent);
			return FIRE_TREE_LOOK;
		}

		return FIRE_TREE_LOOK;
	}

	protected static ref TStringArray s_aVarsIn = { PORT_SUPPRESSION_VOLUME };
	override TStringArray GetVariablesIn() { return s_aVarsIn; }

	protected static ref TStringArray s_aVarsOut = { PORT_VISIBLE, PORT_TIME_LAST_SEEN, "FireTreeId" };
	override TStringArray GetVariablesOut() { return s_aVarsOut; }

	static override bool VisibleInPalette() { return true; }
}