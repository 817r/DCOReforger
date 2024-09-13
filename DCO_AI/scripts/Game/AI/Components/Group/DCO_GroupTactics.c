enum DCO_GroupTactic{
	EVASIVE,
	DEFENSIVE,
	BALANCE,
	ASSAULT,
	FALLBACK
};

class DCO_GroupTacticComponentClass : ScriptComponentClass
{
	
}


class DCO_GroupTacticComponent : ScriptComponent
{
	[Attribute(defvalue: "2", uiwidget: UIWidgets.ComboBox, desc: "DCO Group Identifer", enums: ParamEnumArray.FromEnum(DCO_GroupTactic))]
	DCO_GroupTactic m_tac;
	DCO_GroupTactic automatedTac = 2;
	
	[Attribute(defvalue: "1", uiwidget: UIWidgets.Auto, desc: "is Tactics Automated?")]
	bool AutomatedTactics;
	
	SCR_AIGroup m_Group;
	
	static DCO_GroupTactic SetTactic(IEntity unit, DCO_GroupTactic groups)
	{
		if (!unit)
			return DCO_GroupTactic.BALANCE;
		
		DCO_GroupTacticComponent comp = GetGroupTacticComponent(unit);
		SCR_AIGroupUtilityComponent gUtil = GetGroupUtilComp(unit);
		
		if (!comp)
			return DCO_GroupTactic.BALANCE;
		
		return comp.SetGroupTactic(groups, gUtil);
	}
	
	static DCO_GroupTactic SetManualTactics(IEntity unit, DCO_GroupTactic groups)
	{
		if (!unit)
			return DCO_GroupTactic.BALANCE;
		
		DCO_GroupTacticComponent comp = GetGroupTacticComponent(unit);
		SCR_AIGroupUtilityComponent gUtil = GetGroupUtilComp(unit);
		
		if (!comp)
			return DCO_GroupTactic.BALANCE;
		
		return comp.SetManualTactic(groups, gUtil);
	}

	static DCO_GroupTactic GetGroupTactic(IEntity unit)
	{		
		if (!unit)
			return DCO_GroupTactic.BALANCE;
		
		DCO_GroupTacticComponent comp = GetGroupTacticComponent(unit);
		
		if (!comp)
			return DCO_GroupTactic.BALANCE;
		
		return comp.GetGroupTactic();
	}
	
	static DCO_GroupTactic GetManualTactic(IEntity unit)
	{		
		if (!unit)
			return DCO_GroupTactic.BALANCE;
		
		DCO_GroupTacticComponent comp = GetGroupTacticComponent(unit);
		
		if (!comp)
			return DCO_GroupTactic.BALANCE;
		
		return comp.GetManualTac();
	}
	
	static DCO_GroupTactic setAutomated(IEntity unit, bool tf)
	{
		if (!unit)
			return true;
		
		DCO_GroupTacticComponent comp = GetGroupTacticComponent(unit);
		SCR_AIGroupUtilityComponent gUtil = GetGroupUtilComp(unit);
		
		if (!comp)
			return true;
		
		
		return comp.setAuto(tf, gUtil);
	}

	static DCO_GroupTactic getAutomated(IEntity unit)
	{		
		if (!unit)
			return true;
		
		DCO_GroupTacticComponent comp = GetGroupTacticComponent(unit);
		
		if (!comp)
			return true;
		
		return comp.getAuto();
	}
	
	static DCO_GroupTacticComponent GetGroupTacticComponent(IEntity unit)
	{
		return DCO_GroupTacticComponent.Cast(unit.FindComponent(DCO_GroupTacticComponent));
	}
	
	static SCR_AIGroupUtilityComponent GetGroupUtilComp(IEntity unit)
	{
		return SCR_AIGroupUtilityComponent.Cast(unit.FindComponent(SCR_AIGroupUtilityComponent));
	}

	protected DCO_GroupTactic SetGroupTactic(DCO_GroupTactic groups, SCR_AIGroupUtilityComponent gutil)
	{
		if (AutomatedTactics)
		{
			automatedTac = groups;
			gutil.UpdateTactics();
			return groups;
		} else if (!AutomatedTactics)
		{
			m_tac = groups;
			gutil.UpdateTactics();
			return groups;
		}
			
		return groups;
	}
	
	protected DCO_GroupTactic SetManualTactic(DCO_GroupTactic groups, SCR_AIGroupUtilityComponent gutil)
	{
		m_tac = groups;
		gutil.UpdateTactics();
		return groups;
	}
	
	protected DCO_GroupTactic GetGroupTactic()
	{
		if (AutomatedTactics)
		{
			return automatedTac;
		} else if (!AutomatedTactics)
		{
			return m_tac;
		}
		
		return automatedTac;
	}
	
	protected DCO_GroupTactic GetManualTac()
	{
		return m_tac;
	}
	
	DCO_GroupTactic setAutomatedTac(DCO_GroupTactic tacs, SCR_AIGroupUtilityComponent gutil)
	{
		automatedTac = tacs;
		gutil.UpdateTactics();
		return tacs;
	}
	
	DCO_GroupTactic getAutomatedTac()
	{
		return automatedTac;
	}
	
	bool setAuto(bool tf, SCR_AIGroupUtilityComponent gutil)
	{
		AutomatedTactics = tf;
		gutil.UpdateTactics();
		return tf;
	}
	
	bool getAuto()
	{
		return AutomatedTactics;
	}
	
	void DCO_GroupTacticComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_Group = SCR_AIGroup.Cast(ent);
	}
}