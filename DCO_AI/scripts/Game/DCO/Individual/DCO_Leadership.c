class DCO_Leadership
{
	static int Of(SCR_AIUtilityComponent u)
	{
		return DCO_AIConfigComponent.LeadershipOf(u);
	}

	static int OfEntity(IEntity e)
	{
		if (!e)
			return DCO_ELeadership.AVERAGE;
		SCR_AICombatComponent cc = SCR_AICombatComponent.Cast(e.FindComponent(SCR_AICombatComponent));
		if (!cc)
			return DCO_ELeadership.AVERAGE;
		return Of(cc.GetUtilityComponent());
	}

	static int OfLeader(AIGroup g)
	{
		if (!g)
			return DCO_ELeadership.AVERAGE;
		return OfEntity(g.GetLeaderEntity());
	}

	static int OfGroupOf(SCR_AIUtilityComponent u)
	{
		if (!u)
			return DCO_ELeadership.AVERAGE;
		AIAgent ag = u.GetAIAgent();
		if (!ag)
			return DCO_ELeadership.AVERAGE;
		return OfLeader(ag.GetParentGroup());
	}

	static string Name(int lvl)
	{
		switch (lvl)
		{
			case DCO_ELeadership.POOR: return "poor";
			case DCO_ELeadership.GOOD: return "good";
			case DCO_ELeadership.INSPIRING: return "inspiring";
		}
		return "average";
	}

	static string PersName(int p)
	{
		switch (p)
		{
			case DCO_EAIPersonality.CAUTIOUS: return "cautious";
			case DCO_EAIPersonality.AGGRESSIVE: return "aggressive";
			case DCO_EAIPersonality.RECKLESS: return "reckless";
		}
		return "measured";
	}

	static int PackTraits(SCR_AIUtilityComponent u)
	{
		if (!u)
			return -1;
		int pers = DCO_AIConfigComponent.PersonalityOf(u);
		int skill = DCO_AIConfigComponent.SkillOf(u);
		int lead = Of(u);
		return pers + skill * 4 + lead * 32;
	}

	static string TraitsText(int traits)
	{
		if (traits < 0)
			return "-";
		int pers = traits - 4 * (traits / 4);
		int skill = (traits / 4) - 8 * (traits / 32);
		int lead = traits / 32;
		string s = PersName(pers);
		s.ToUpper();
		string p1 = s.Substring(0, 1);
		string pRest = s.Substring(1, s.Length() - 1);
		pRest.ToLower();
		string l = Name(lead);
		string l1 = l.Substring(0, 1);
		l1.ToUpper();
		string lRest = l.Substring(1, l.Length() - 1);
		string sk = typename.EnumToString(DCO_AISKILL, skill);
		return p1 + pRest + " / " + sk + " / Leadership " + l1 + lRest;
	}

	static float Aura(int lvl)
	{
		switch (lvl)
		{
			case DCO_ELeadership.POOR: return 0.05;
			case DCO_ELeadership.GOOD: return 0.25;
			case DCO_ELeadership.INSPIRING: return 0.4;
		}
		return 0.15;
	}

	static float AuraRadius(int lvl)
	{
		switch (lvl)
		{
			case DCO_ELeadership.POOR: return 60.0;
			case DCO_ELeadership.GOOD: return 120.0;
			case DCO_ELeadership.INSPIRING: return 150.0;
		}
		return 100.0;
	}

	static float RallyMul(int lvl)
	{
		switch (lvl)
		{
			case DCO_ELeadership.POOR: return 1.5;
			case DCO_ELeadership.GOOD: return 0.6;
			case DCO_ELeadership.INSPIRING: return 0.3;
		}
		return 1.0;
	}

	static float ShoutDamp(int lvl)
	{
		switch (lvl)
		{
			case DCO_ELeadership.GOOD: return 0.25;
			case DCO_ELeadership.INSPIRING: return 0.5;
		}
		return 0.0;
	}

	static float PinRecMul(int lvl)
	{
		switch (lvl)
		{
			case DCO_ELeadership.POOR: return 0.8;
			case DCO_ELeadership.GOOD: return 0.5;
			case DCO_ELeadership.INSPIRING: return 0.4;
		}
		return 0.6;
	}

	static float RatioMul(int lvl)
	{
		switch (lvl)
		{
			case DCO_ELeadership.POOR: return 1.15;
			case DCO_ELeadership.GOOD: return 0.9;
			case DCO_ELeadership.INSPIRING: return 0.8;
		}
		return 1.0;
	}

	static float ReportMul(int lvl)
	{
		switch (lvl)
		{
			case DCO_ELeadership.POOR: return 0.8;
			case DCO_ELeadership.GOOD: return 1.1;
			case DCO_ELeadership.INSPIRING: return 1.2;
		}
		return 1.0;
	}

	static float LossStress(int lvl)
	{
		switch (lvl)
		{
			case DCO_ELeadership.POOR: return 0.3;
			case DCO_ELeadership.GOOD: return 0.75;
			case DCO_ELeadership.INSPIRING: return 0.9;
		}
		return 0.6;
	}

	static float LossConfuseMin(int lvl)
	{
		switch (lvl)
		{
			case DCO_ELeadership.POOR: return 3.0;
			case DCO_ELeadership.GOOD: return 8.0;
			case DCO_ELeadership.INSPIRING: return 10.0;
		}
		return 5.0;
	}

	static float LossConfuseMax(int lvl)
	{
		switch (lvl)
		{
			case DCO_ELeadership.POOR: return 5.0;
			case DCO_ELeadership.GOOD: return 15.0;
			case DCO_ELeadership.INSPIRING: return 20.0;
		}
		return 10.0;
	}

	static float DisciplineMul(int lvl)
	{
		switch (lvl)
		{
			case DCO_ELeadership.POOR: return 1.3;
			case DCO_ELeadership.GOOD: return 0.8;
			case DCO_ELeadership.INSPIRING: return 0.6;
		}
		return 1.0;
	}

	static float AmmoMul(int lvl)
	{
		switch (lvl)
		{
			case DCO_ELeadership.POOR: return 0.85;
			case DCO_ELeadership.GOOD: return 1.1;
			case DCO_ELeadership.INSPIRING: return 1.25;
		}
		return 1.0;
	}
}
