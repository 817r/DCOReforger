class DCO_DarkGate
{
	static bool Block(SCR_AIUtilityComponent u, BaseTarget target, out string why)
	{
		why = "";
		if (!u || !u.m_CombatComponent || !u.m_OwnerEntity || !target || !target.GetTargetEntity())
			return false;
		if (DCO_Stress.T(u, "pc.dark_gate", 1.0) <= 0)
			return false;
		float lv = u.m_CombatComponent.m_fDCONightLv;
		if (lv > DCO_Stress.T(u, "pc.dark_gate_lv", -8.0))
			return false;
		if (target.IsEndangering())
		{
			DCO_Perf.Count("dark_gate:pass_endangering");
			return false;
		}
		IEntity e = target.GetTargetEntity();
		PerceivableComponent perc = PerceivableComponent.Cast(e.FindComponent(PerceivableComponent));
		if (perc && perc.GetIlluminationFactor() >= 0.2)
		{
			DCO_Perf.Count("dark_gate:pass_lit");
			return false;
		}
		float limit = 60.0;
		string stance = "vehicle";
		ChimeraCharacter ch = ChimeraCharacter.Cast(e);
		if (ch)
		{
			limit = 30.0;
			stance = "stand";
			CharacterControllerComponent ctrl = ch.GetCharacterController();
			if (ctrl && ctrl.GetStance() == ECharacterStance.CROUCH)
			{
				limit = 20.0;
				stance = "crouch";
			}
			else if (ctrl && ctrl.GetStance() == ECharacterStance.PRONE)
			{
				limit = 10.0;
				stance = "prone";
			}
			if (ch.GetPhysics() && ch.GetPhysics().GetVelocity().Length() > 0.5)
			{
				limit = limit * 2.0;
				stance = stance + "_moving";
			}
		}
		float d = target.GetDistance();
		if (d <= limit)
			return false;
		why = string.Format("dist=%1 limit=%2 lv=%3 target=%4", Math.Round(d), Math.Round(limit), lv.ToString(-1, 1), stance);
		return true;
	}
}
