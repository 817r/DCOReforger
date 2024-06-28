modded class SCR_AIInfoComponent : SCR_AIInfoBaseComponent
{
	protected DCO_AIMoraleSystem m_DCOMoraleSystem;

	void InitMoraleSystem(DCO_AIMoraleSystem moraleSystem)
	{
		m_DCOMoraleSystem = moraleSystem;	
	}
	
	moraleState getMoraleState()
	{
		if(m_DCOMoraleSystem)
			return m_DCOMoraleSystem.GetState();
		else
			return moraleState.NORMAL;
	}
	
	DCO_AIMoraleSystem getMoraleSystem()
	{
		return m_DCOMoraleSystem;
	}

}