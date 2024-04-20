modded class SCR_AIInfoComponent : SCR_AIInfoBaseComponent
{
	protected DCO_AIMoraleSystem m_DCOMoraleSystem;

	void InitMoraleSystem(DCO_AIMoraleSystem moraleSystem)
	{
		m_DCOMoraleSystem = moraleSystem;	
	}

}