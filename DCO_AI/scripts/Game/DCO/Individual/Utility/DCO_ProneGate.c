class DCO_ProneGate
{
	protected static ref map<IEntity, float> s_mLogAt;

	static bool Allow(AIAgent owner, string src, out string why)
	{
		why = "";
		if (!owner)
			return true;
		IEntity me = owner.GetControlledEntity();
		SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(owner);
		if (!me || !ca || !ca.m_UtilityComponent)
			return true;
		SCR_AIUtilityComponent u = ca.m_UtilityComponent;
		if (DCO_Stress.T(u, "stance.prone_gate", 1.0) <= 0)
			return true;
		if (u.m_ThreatSystem && u.m_ThreatSystem.GetState() == EAIThreatState.THREATENED)
		{
			why = "threatened";
			return true;
		}
		if (DCO_Stress.Level(u) >= 1)
		{
			why = "under_fire";
			return true;
		}
		if (u.m_ThreatSystem && u.m_ThreatSystem.GetSuppressionMeasure() > 0.3)
		{
			why = "suppressed";
			return true;
		}
		AIGroup grp = owner.GetParentGroup();
		if (grp)
		{
			SCR_AIGroupUtilityComponent gu = SCR_AIGroupUtilityComponent.Cast(grp.FindComponent(SCR_AIGroupUtilityComponent));
			if (gu)
			{
				DCO_GroupTactics post = gu.DCO_GetPosture();
				if (post == DCO_GroupTactics.AMBUSH || post == DCO_GroupTactics.EVASIVE)
				{
					why = "posture";
					return true;
				}
			}
		}
		if (DCO_Util.IsMGShooter(me) && u.m_CombatComponent && u.m_CombatComponent.GetCurrentTarget())
		{
			why = "mg_target";
			return true;
		}
		why = "not_threatened";
		DCO_Perf.Count("prone_gate:" + src);
		float now = DCO_Util.NowS();
		if (!s_mLogAt)
			s_mLogAt = new map<IEntity, float>();
		if (s_mLogAt.Count() > 400)
			s_mLogAt.Clear();
		float last;
		if (s_mLogAt.Find(me, last) && now - last < 20.0)
			return false;
		s_mLogAt.Set(me, now);
		DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("prone_gate unit=%1 src=%2 to=CROUCH why=%3", DCO_Log.IdUnit(me), src, why));
		return false;
	}
}
