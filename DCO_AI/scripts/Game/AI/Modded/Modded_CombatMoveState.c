modded class SCR_AICombatMoveState
{
	protected ref SCR_AICombatMoveRequestBase m_OldRequest;
	
	override void ApplyNewRequest(notnull SCR_AICombatMoveRequestBase request)
	{
		
		// mark old request as cancelled if it was running, in case something else needs to know its state
		if (m_Request && m_Request.m_eState == SCR_EAICombatMoveRequestState.EXECUTING)
		{
			m_Request.m_eState = SCR_EAICombatMoveRequestState.CANCELED;
			m_OldRequest = m_Request;		
		}
		
		m_Request = request; // Old request is unreferenced here
		super.ApplyNewRequest(request);
	}
	
	SCR_AICombatMoveRequestBase GetOldRequest()
	{
		return m_OldRequest;
	}
	
	bool IsMovingToBuilding()
	{
		if (m_OldRequest)
			return m_Request.m_eType == SCR_EAICombatMoveRequestType.BUILDING || m_OldRequest.m_eType == SCR_EAICombatMoveRequestType.BUILDING;
		else if (m_Request)
			return m_Request.m_eType == SCR_EAICombatMoveRequestType.BUILDING;
		else
			return false;
	}
}

modded enum SCR_EAICombatMoveRequestFailReason
{
	NO_BUILDING_FOUND
}

modded class SCR_AICombatMoveRequestBase
{
	SCR_EAICombatMoveRequestType m_eType = SCR_EAICombatMoveRequestType.STOP;
}