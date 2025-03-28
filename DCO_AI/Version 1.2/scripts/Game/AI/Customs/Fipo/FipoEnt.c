class DCO_FiPoClass : SCR_AISmartActionComponentClass
{
}

class DCO_FiPo : SCR_AISmartActionComponent
{		
	[Attribute("0 0 0", UIWidgets.EditBox, desc: "Position where AI will look from action offset (in local coords of the object entity)", params: "inf inf 0 purpose=coords space=entity")]
	protected vector m_vLookPosition;
	
	[Attribute("180", UIWidgets.Coords, desc: "Range of rotation within which AI will restrict their observing")]
	protected float m_fLookDirectionRange;
	
	[Attribute("40", UIWidgets.Coords, desc: "Range of Overrun")]
	protected float m_fOverrunDistance;
	
	[Attribute("0", uiwidget: UIWidgets.ComboBox, "AI Peeking Stance", "", ParamEnumArray.FromEnum(ECharacterStance))]
	protected int m_iHCharacterStance;
	
	[Attribute("1", uiwidget: UIWidgets.ComboBox, "AI Optimal Shooting Stance", "", ParamEnumArray.FromEnum(ECharacterStance))]
	protected int m_iSCharacterStance;
	
	[Attribute("0", uiwidget: UIWidgets.CheckBox, "Whether the AI will be using binoculars during observation", "")]
	protected bool m_bUseBinoculars;
	
	[Attribute("0", uiwidget: UIWidgets.ComboBox, "Whether the AI will occasionally lean to side", "", ParamEnumArray.FromEnum(ELeaningType))]
	protected ELeaningType m_eLeaningType;
	
	//------------------------------------------------------------------------------------------------
	//! \return
	vector GetLookPosition()
	{
		return m_vLookPosition;
	}
	
	//------------------------------------------------------------------------------------------------
	//! \return
	float GetLookDirectionRange()
	{
		return m_fLookDirectionRange;
	}
	
	float GetOverrunDistance()
	{
		return m_fOverrunDistance;
	}
	
	//------------------------------------------------------------------------------------------------
	//! \return
	int GetHidingDesiredStance()
	{
		return m_iHCharacterStance;
	}
	
	int GetShootingDesiredStance()
	{
		return m_iSCharacterStance;
	}
	
	//------------------------------------------------------------------------------------------------
	//! \return
	bool GetUseBinoculars()
	{
		return m_bUseBinoculars;
	}
	
	//------------------------------------------------------------------------------------------------
	//! \param[in] useBinoculars
	void SetUseBinoculars(bool useBinoculars)
	{
		m_bUseBinoculars = useBinoculars;
	}
}