enum PERSONALITY_TYPE
{
	BRAVE
}

[BaseContainerProps(), SCR_BaseContainerCustomTitleField("PersonalityName")]
class DCO_Personality
{
	[Attribute("", uiwidget: UIWidgets.Auto, desc: "Unique Name for Personality")]
	string PersonalityName;
	
	[Attribute("0", UIWidgets.ComboBox, "AI Personality", "", ParamEnumArray.FromEnum(PERSONALITY_TYPE) )]
	PERSONALITY_TYPE type;
	
	[Attribute( defvalue: "5", uiwidget: UIWidgets.Slider, desc: "How Aggresive the unit", params: "1 10 1" )]
	float m_Aggresion;
	
	[Attribute( defvalue: "5", uiwidget: UIWidgets.Slider, desc: "How Frequent the unit use Cover", params: "1 10 1" )]
	float m_CoverUsage;
		
	[Attribute( defvalue: "5", uiwidget: UIWidgets.Slider, desc: "How fast the unit decide what to do", params: "1 10 1" )]
	float m_DecisionTime;
}