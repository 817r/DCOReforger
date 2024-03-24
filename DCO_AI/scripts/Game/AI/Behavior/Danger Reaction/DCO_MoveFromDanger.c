modded class SCR_AIMoveFromDangerBehavior : SCR_AIBehaviorBase
{
	protected vector m_DangerPos;
	
	protected IEntity m_OwnerEntity;
	
	protected bool m_DisableMovementControls;

	override void OnActionSelected()
	{
		super.OnActionSelected();

		m_OwnerEntity = m_Utility.m_OwnerEntity;
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