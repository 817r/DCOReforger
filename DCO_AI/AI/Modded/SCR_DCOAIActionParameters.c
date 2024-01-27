class DCOAITest : AITaskScripted
{
	// Member variables
	// Exposing variable m_Radius as attribute will make it visible as parameter inside BT Editor.
	// Note: Functionality not completed now
	[Attribute("10", "editbox", "Radius")]
	float m_Radius;
	IEntity m_Self;

	// Constructor can be used for initializing default values,
	// but for any other operation, use EOnTaskSimulate
	void Init()
	{
		m_Radius = 100;
	}

	// Make scripted node visible or hidden in nodes palette
	bool VisibleInPalette()
	{
		return true;
	}

	// Sets up input variables, as array of strings
	override TStringArray GetVariablesIn()
	{
		// Make sure variables are initialised only once
		if (!m_aVariablesIn)
		{
			m_aVariablesIn = new TStringArray;
			m_aVariablesIn.Insert("Distance");
		}
		return m_aVariablesIn;
	}

	// Sets up output variables
	override TStringArray GetVariablesOut()
	{
		if (!m_aVariablesOut)
		{
			m_aVariablesOut = new TStringArray;
			m_aVariablesOut.Insert("Target");
		}

		return m_aVariablesOut;
	}

	// Main method to perform all operations handled by task
	// This is just example, to see important parts, without any actual calculations
	ENodeResult EOnTaskSimulate(IEntity owner, float dt)
	{
		// This is engine method to read variables from Behavior tree
		GetVariableIn("Distance",m_Radius);

		IEntity target = GetValidTarget(m_Radius);

		// This is method to output variable into Behavior Tree
		SetVariableOut("Target", target);

		if (target)
			return ENodeResult.Fail;
		else
			return ENodeResult.Success;
	}
}