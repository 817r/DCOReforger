modded class SCR_AIMoveFromDangerBehavior : SCR_AIBehaviorBase
{
	protected vector m_DangerPos;
	protected IEntity m_OwnerEntity;

	protected bool m_DisableMovementControls;
	
	override void InitParameters(IEntity dangerEntity, vector dangerPos)
	{
		m_DangerPosition.Init(this, dangerPos);
		m_DangerEntity.Init(this, dangerEntity);
		m_Stance.Init(this, ECharacterStance.STAND);
		m_MovementType.Init(this, EMovementType.RUN);
	}
	
	void SCR_AIMoveFromDangerBehavior(SCR_AIUtilityComponent utility, SCR_AIActivityBase groupActivity, vector dangerPos, IEntity dangerEntity)
	{
		SetPriority(PRIORITY_BEHAVIOR_MOVE_FROM_DANGER);
		InitParameters(dangerEntity, dangerPos);
		if (dangerEntity)
		{
			m_DangerPosition.m_Value = dangerEntity.GetOrigin();
		}
		
		m_sBehaviorTree = "{D12937CF422B639B}AI/BehaviorTrees/Chimera/Soldier/MoveFromDanger_Position.bt";
		m_bResetLook = true;
		m_bAllowLook = false;
	}

	override void OnActionSelected()
	{
		super.OnActionSelected();
		SetActionInterruptable(false);
	}
}