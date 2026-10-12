modded class SCR_AIUpdateTargetSuppressionData
{
	protected static const string PORT_FIRE_TREE_ID = "FireTreeId";
	protected static const string PORT_SUPPRESSION_VOLUME = "SuppressionVolume";

	protected static const string PORT_VISIBLE = "Visible";
	protected static const string PORT_TIME_LAST_SEEN = "TimeLastSeen_ms";

	protected const int FIRE_TREE_INVALID 		= -1;
	protected const int FIRE_TREE_LOOK			= 0;
	protected const int FIRE_TREE_SUPPRESSIVE	= 1;

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
	protected float m_fDCOLaneAt_ms = -100000;
	protected int m_iDCOLane;

	protected bool DCO_LaneBlocked(SCR_AISuppressionVolumeBase vol)
	{
		float now = GetGame().GetWorld().GetWorldTime();
		if (now - m_fDCOLaneAt_ms < 250.0)
			return m_iDCOLane != 0;
		m_fDCOLaneAt_ms = now;
		IEntity blocker;
		vector target = vol.GetCenterPosition();
		m_iDCOLane = DCO_Util.FireLaneCheck(m_UtilityComponent.m_OwnerEntity, target, DCO_Util.VolumeRadius(vol), blocker);
		if (m_iDCOLane != 0)
			DCO_Util.LaneEvent(m_UtilityComponent.m_OwnerEntity, target, m_iDCOLane, blocker, "cease");
		return m_iDCOLane != 0;
	}

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

	protected bool DCO_AreaFireOrdered()
	{
		SCR_AIActionBase act = SCR_AIActionBase.Cast(m_UtilityComponent.GetExecutedAction());
		if (m_UtilityComponent.m_CombatComponent && GetGame().GetWorld().GetWorldTime() < m_UtilityComponent.m_CombatComponent.m_fDCORetFireUntil_ms)
			return true;
		SCR_AISuppressBehavior sb = SCR_AISuppressBehavior.Cast(act);
		if (sb && sb.GetRelatedGroupActivity())
			return true;
		AIAgent ag = m_UtilityComponent.GetAIAgent();
		if (ag && ag.GetParentGroup())
		{
			DCO_GroupUtilityComponent g = DCO_GroupUtilityComponent.Cast(ag.GetParentGroup().FindComponent(DCO_GroupUtilityComponent));
			if (g && g.GetTask() == DCO_EGroupTask.SUPPORT_BY_FIRE)
				return true;
		}
		return false;
	}

	protected bool DCO_NearSegmentClear()
	{
		if (!suppressionVolume || !m_UtilityComponent.m_OwnerEntity)
			return false;
		vector eye = m_UtilityComponent.m_OwnerEntity.GetOrigin() + Vector(0, 1.2, 0);
		vector to = suppressionVolume.GetCenterPosition() + Vector(0, 1.0, 0);
		float len = vector.Distance(eye, to);
		if (len < 1.0)
			return false;
		float near = Math.Min(len, DCO_Stress.T(m_UtilityComponent, "tm.area_fire_clear_m", 40.0));
		vector end = eye + (to - eye) * (near / len);
		return DCO_Util.TraceFrac(eye, end, null, TraceFlags.WORLD | TraceFlags.ENTS, -1, "t:area_fire_near") >= 0.95;
	}

	override int ResolveFireTree(bool targetVisible)
	{
		SCR_AIBehaviorBase executedBehavior = SCR_AIBehaviorBase.Cast(m_UtilityComponent.GetExecutedAction());
		if (executedBehavior && executedBehavior.m_bUseCombatMove && !m_UtilityComponent.m_CombatMoveState.m_bAimAtTarget)
			return FIRE_TREE_INVALID;

		if (m_UtilityComponent.m_CombatComponent && (m_UtilityComponent.m_CombatComponent.DCO_IsPinned() || m_UtilityComponent.m_CombatComponent.DCO_IsPinRecovering()))
			return FIRE_TREE_LOOK;

		DCO_LiveSuppressVolume live = DCO_LiveSuppressVolume.Cast(suppressionVolume);
		if (live && live.m_bLifted)
		{
			DCO_Perf.Count("sbf:fire_lifted");
			return FIRE_TREE_LOOK;
		}

		vector grenadeThrowPos;

		BaseWeaponComponent glWeapon;
		int glMuzzleId;
		if (m_UtilityComponent.m_CombatComponent)
			m_UtilityComponent.m_CombatComponent.GetSelectedWeapon(glWeapon, glMuzzleId);

		if (m_PerceptionComponent.GetFriendlyInLineOfFire() || DCO_LaneBlocked(suppressionVolume))
			return FIRE_TREE_LOOK;

		if (targetVisible)
			return FIRE_TREE_SUPPRESSIVE;
		if (DCO_Stress.T(m_UtilityComponent, "tm.area_fire", 1.0) > 0 && DCO_AreaFireOrdered() && DCO_NearSegmentClear())
		{
			DCO_Perf.Count("supp:area_fire_unseen");
			return FIRE_TREE_SUPPRESSIVE;
		}
		if (!targetVisible && m_UtilityComponent.m_CombatComponent.HasWeaponOfType(EWeaponType.WT_ROCKETLAUNCHER) && vector.Distance(m_UtilityComponent.GetOrigin(), suppressionVolume.GetCenterPosition()) > 10
			&& DCO_RpgUnseen.Allowed(m_UtilityComponent, null, suppressionVolume.GetCenterPosition()))
			return 3;
		else if (!targetVisible
			&& DCO_UGLUtility.ShouldUseGL(m_UtilityComponent, glWeapon, suppressionVolume.GetCenterPosition()))
			return 4;

		else if (!targetVisible
			&& m_UtilityComponent.m_AIInfo
			&& m_UtilityComponent.m_AIInfo.HasRole(EUnitRole.HAS_FRAG_GRENADE)
			&& DCO_GrenadeUtility.CanThrowGrenadeNow(m_UtilityComponent)
			&& DCO_GrenadeUtility.ResolveThrowPos(m_UtilityComponent, suppressionVolume.GetCenterPosition(), grenadeThrowPos))
		{
			SCR_AIThrowGrenadeToBehavior gren = new SCR_AIThrowGrenadeToBehavior(m_UtilityComponent, null, grenadeThrowPos, EWeaponType.WT_FRAGGRENADE, 1, SCR_AIThrowGrenadeToBehavior.PRIORITY_BEHAVIOR_THROW_GRENADE +
			SCR_AIThrowGrenadeToBehavior.PRIORITY_LEVEL_PLAYER);
			m_UtilityComponent.AddAction(gren);
			DCO_GrenadeUtility.NotifyGrenadeThrown(m_UtilityComponent);
			return FIRE_TREE_LOOK;
		}

		return FIRE_TREE_LOOK;
	}

	protected static ref TStringArray s_aVarsIn;
	protected static TStringArray GetVarsIn()
	{
		if (!s_aVarsIn)
			s_aVarsIn = { PORT_SUPPRESSION_VOLUME };
		return s_aVarsIn;
	}
	override TStringArray GetVariablesIn() { return GetVarsIn(); }

	protected static ref TStringArray s_aVarsOut;
	protected static TStringArray GetVarsOut()
	{
		if (!s_aVarsOut)
			s_aVarsOut = { PORT_VISIBLE, PORT_TIME_LAST_SEEN, "FireTreeId" };
		return s_aVarsOut;
	}
	override TStringArray GetVariablesOut() { return GetVarsOut(); }

	static override bool VisibleInPalette() { return true; }
}