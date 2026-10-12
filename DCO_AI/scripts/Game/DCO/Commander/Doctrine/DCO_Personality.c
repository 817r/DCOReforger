class DCO_Personality
{
	float m_fIntel;
	float m_fStrategy;
	float m_fAggression;
	float m_fLogistics;
	float m_fSupport;
	float m_fLeadership;
	int m_iCourage;
	int m_iDecision;

	static void Names(notnull array<string> outNames)
	{
		outNames.Insert("Measured");
		outNames.Insert("Careful");
		outNames.Insert("Bold");
		outNames.Insert("Scout");
	}

	static int IndexOf(string name)
	{
		array<string> names = {};
		Names(names);
		int index = names.Find(name);
		if (index < 0)
			return 0;
		return index;
	}

	static string NameAt(int index)
	{
		array<string> names = {};
		Names(names);
		if (!names.IsIndexValid(index))
			return names[0];
		return names[index];
	}

	static DCO_Personality Get(string name)
	{
		DCO_Personality p = new DCO_Personality();
		p.m_fIntel = 1;
		p.m_fStrategy = 1;
		p.m_fAggression = 1;
		p.m_fLogistics = 1;
		p.m_fSupport = 1;
		p.m_fLeadership = 1;
		p.m_iCourage = DCO_ECourage.CALCULATED;
		p.m_iDecision = DCO_EDecision.MODERATE;
		switch (name)
		{
			case "Careful":
			{
				p.m_fIntel = 1.5;
				p.m_fStrategy = 1.5;
				p.m_fAggression = 0.5;
				p.m_fLogistics = 1.5;
				p.m_fSupport = 1.5;
				p.m_iCourage = DCO_ECourage.CAREFUL;
				break;
			}
			case "Bold":
			{
				p.m_fIntel = 0.5;
				p.m_fStrategy = 0.5;
				p.m_fAggression = 2;
				p.m_iCourage = DCO_ECourage.BOLD;
				p.m_iDecision = DCO_EDecision.FAST;
				break;
			}
			case "Scout":
			{
				p.m_fIntel = 2;
				p.m_fStrategy = 1.5;
				p.m_fSupport = 1.5;
				break;
			}
		}
		return p;
	}
}
