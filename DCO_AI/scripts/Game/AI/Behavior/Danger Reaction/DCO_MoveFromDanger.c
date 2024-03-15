modded class SCR_AIMoveFromDangerBehavior : SCR_AIBehaviorBase
{
	protected int index = 1;
	
	protected vector m_DangerPos;
	
	protected IEntity m_OwnerEntity;
	
	protected bool m_DisableMovementControls;

	override void OnActionSelected()
	{
		super.OnActionSelected();
		
		m_OwnerEntity = m_Utility.m_OwnerEntity;
		
		if (m_CharacterControllerComponent)
			m_DisableMovementControls = m_CharacterControllerComponent.GetDisableMovementControls();
		
		if (m_DisableMovementControls)
			Fail();
	}

	override void OnActionDeselected()
	{
		index++;
		
		super.OnActionDeselected();
		
		string aiMoveFromDangerActionState = "SCR_AIMoveFromDangerBehavior: DESELECTED";
		
		m_Utility.m_LookAction.LookAt(m_DangerPos, m_Utility.m_LookAction.PRIO_UNKNOWN_TARGET, 2.0);
	}

	override void OnActionCompleted()
	{
		index++;
		
		super.OnActionCompleted();
		
		string aiMoveFromDangerActionState = "SCR_AIMoveFromDangerBehavior: COMPLETED";
	}

	override void OnActionRemoved()
	{
		index++;
		
		super.OnActionRemoved();
		
		string aiMoveFromDangerActionState = "SCR_AIMoveFromDangerBehavior: REMOVED";
	}

	override void InitParameters(IEntity dangerEntity, vector dangerPos)
	{
		m_DangerPos = dangerPos;

		m_DangerPosition.Init(this, dangerPos);
		m_DangerEntity.Init(this, dangerEntity);
	}
	
	void SCR_AIMoveFromDangerBehavior(SCR_AIUtilityComponent utility, SCR_AIActivityBase groupActivity, vector dangerPos, IEntity dangerEntity)
	{
		m_bResetLook = true;
		m_bAllowLook = false;
	}
};