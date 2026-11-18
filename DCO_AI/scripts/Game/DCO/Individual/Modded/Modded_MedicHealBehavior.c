modded class SCR_AIMedicHealBehavior : SCR_AIBehaviorBase
{
	protected const float MAX_THREAT_THRESHOLD = 0.05;

	override float CustomEvaluate()
	{
		EAIActionState st = GetActionState();
		if (st != EAIActionState.COMPLETED && st != EAIActionState.FAILED && m_Utility && m_EntityToHeal.m_Value != m_Utility.m_OwnerEntity && DCO_Util.IsBleeding(m_Utility.m_OwnerEntity))
		{
			DCO_Perf.Count("medic:helper_bleeding_abort");
			DCO_Log.Write(DCO_ELogChannel.MEDIC, DCO_ELogLevel.INFO, DCO_Log.IdUnit(m_Utility.m_OwnerEntity), string.Format("medic_helper_bleeding_abort helper=%1 patient=%2", DCO_Log.IdUnit(m_Utility.m_OwnerEntity), DCO_Log.IdUnit(m_EntityToHeal.m_Value)));
			Fail();
			return 0;
		}
		if (m_Utility && m_EntityToHeal.m_Value && m_EntityToHeal.m_Value != m_Utility.m_OwnerEntity && DCO_Stress.T(m_Utility, "heal.unseen_gate", 1.0) > 0 && DCO_CasualtyUnderFire(m_EntityToHeal.m_Value))
		{
			DCO_Perf.Count("heal:unseen_gate_medic");
			if (DCO_Log.On(DCO_ELogChannel.MEDIC, DCO_ELogLevel.DEBUG))
				DCO_Log.Write(DCO_ELogChannel.MEDIC, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(m_Utility.m_OwnerEntity), string.Format("heal_defer who=medic medic=%1 patient=%2 reason=unseen_shooter_close",
					DCO_Log.IdUnit(m_Utility.m_OwnerEntity), DCO_Log.IdUnit(m_EntityToHeal.m_Value)));
			return 0;
		}
		if (st != EAIActionState.COMPLETED && st != EAIActionState.FAILED && m_Utility && m_EntityToHeal.m_Value && m_EntityToHeal.m_Value != m_Utility.m_OwnerEntity)
		{
			float gateNow = GetGame().GetWorld().GetWorldTime();
			if (m_fDCOGateAt_ms < 0 || gateNow - m_fDCOGateAt_ms >= 1000.0)
			{
				m_fDCOGateAt_ms = gateNow;
				string why = DCO_RescueGate(gateNow);
				if (why != m_sDCOGateWhy)
				{
					string shown = why;
					if (shown.IsEmpty())
						shown = "clear";
					DCO_Log.Write(DCO_ELogChannel.MEDIC, DCO_ELogLevel.INFO, DCO_Log.IdUnit(m_Utility.m_OwnerEntity), string.Format("medic_gate helper=%1 patient=%2 why=%3 dist=%4", DCO_Log.IdUnit(m_Utility.m_OwnerEntity), DCO_Log.IdUnit(m_EntityToHeal.m_Value), shown, Math.Round(vector.Distance(m_Utility.m_OwnerEntity.GetOrigin(), m_EntityToHeal.m_Value.GetOrigin()))));
				}
				m_sDCOGateWhy = why;
				if (!why.IsEmpty())
					DCO_Perf.Count("medic:gate_" + why);
			}
			if (!m_sDCOGateWhy.IsEmpty())
				return 0;
		}
		return super.CustomEvaluate();
	}

	protected float m_fDCOGateAt_ms = -1;
	protected string m_sDCOGateWhy;

	string DCO_GateWhy()
	{
		return m_sDCOGateWhy;
	}

	protected string DCO_RescueGate(float now)
	{
		IEntity patient = m_EntityToHeal.m_Value;
		AIAgent myAgent = m_Utility.GetAIAgent();
		if (!myAgent)
			return "";
		SCR_AIGroup grp = SCR_AIGroup.Cast(myAgent.GetParentGroup());
		vector fpos;
		float fr;
		int fn;
		bool fire = grp && DCO_FireEstimate.Get(grp, now, 20000.0, fpos, fr, fn);
		vector pp = patient.GetOrigin();
		bool dangerOn = DCO_Stress.T(m_Utility, "medic.danger_gate", 1.0) > 0;
		if (dangerOn && (DCO_Beaten.HitRecent(patient, 20.0) || (fire && vector.DistanceXZ(fpos, pp) < 50.0)))
		{
			DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetFor(patient);
			if (!cfg || !cfg.GetMedicUnderFire())
				return "danger_spot";
			vector from = fpos;
			if (!fire)
				from = DCO_Beaten.LastAttackerPos(patient);
			bool smoke = DCO_SmokeField.Inside(pp, 1.0) != null;
			if (!smoke && from != vector.Zero)
				smoke = DCO_SmokeField.OnLine(from + vector.Up, pp + vector.Up) != null;
			if (!smoke)
				return "danger_no_smoke";
		}
		if (dangerOn)
		{
			vector me = m_Utility.m_OwnerEntity.GetOrigin();
			vector leg = pp - me;
			leg[1] = 0;
			float len = leg.Length();
			for (float d = 15.0; d < len - 15.0 && d <= 150.0; d += 15.0)
			{
				if (DCO_Beaten.Check(m_Utility, DCO_Util.Surface(me + leg * (d / len)), fpos, "medic_route"))
					return "danger_route";
			}
		}
		float keep = DCO_Stress.T(m_Utility, "medic.keep_fighting", 1.0);
		if (keep <= 0 || !grp)
			return "";
		DCO_GroupUtilityComponent gu = DCO_GroupUtilityComponent.Cast(grp.FindComponent(DCO_GroupUtilityComponent));
		if (!fire && (!gu || !gu.IsInContact()))
			return "";
		array<AIAgent> agents = {};
		grp.GetAgents(agents);
		int fighting;
		bool afterMe;
		foreach (AIAgent a : agents)
		{
			if (a == myAgent)
			{
				afterMe = true;
				continue;
			}
			SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
			if (!ca || !ca.m_UtilityComponent || !DCO_Util.IsAlive(ca.GetControlledEntity()))
				continue;
			if (!afterMe && ca.m_UtilityComponent.FindActionOfType(SCR_AIMedicHealBehavior))
				continue;
			fighting++;
		}
		if (fighting < keep)
			return "keep_fighting";
		return "";
	}

	protected bool DCO_CasualtyUnderFire(IEntity patient)
	{
		AIControlComponent ctrl = AIControlComponent.Cast(patient.FindComponent(AIControlComponent));
		if (!ctrl)
			return false;
		SCR_ChimeraAIAgent agent = SCR_ChimeraAIAgent.Cast(ctrl.GetControlAIAgent());
		if (!agent || !agent.m_UtilityComponent || !agent.m_UtilityComponent.m_ThreatSystem)
			return false;
		SCR_AIUtilityComponent pu = agent.m_UtilityComponent;
		if (pu.m_AIInfo && pu.m_AIInfo.GetBleedTimeToUnconscious() <= 8.0)
			return false;
		float at = pu.m_ThreatSystem.DCO_UnseenCloseAt();
		return at >= 0 && GetGame().GetWorld().GetWorldTime() - at < DCO_Stress.T(m_Utility, "heal.unseen_s", 6.0) * 1000.0;
	}

	override void OnActionCompleted()
	{
		super.OnActionCompleted();
		if (!DCO_Util.Teardown("medic_completed"))
			DCO_MedicDispatcher.OnMedicFinished(m_Utility.m_OwnerEntity, m_EntityToHeal.m_Value, true);
	}

	override void OnActionFailed()
	{
		super.OnActionFailed();
		if (!DCO_Util.Teardown("medic_failed"))
			DCO_MedicDispatcher.OnMedicFinished(m_Utility.m_OwnerEntity, m_EntityToHeal.m_Value, false);
	}
};
