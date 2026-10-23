class DCO_ContactYield
{
	protected static ref map<IEntity, float> s_mLogAt;

	static bool Should(SCR_AIUtilityComponent u, string what, out float dist)
	{
		dist = -1;
		if (!u || !u.m_CombatComponent || !u.m_OwnerEntity)
			return false;
		float maxD = DCO_Stress.T(u, "ind.yield_m", 150.0);
		if (maxD <= 0)
			return false;
		if (u.m_CombatComponent.GetCombatMode() == EAIGroupCombatMode.HOLD_FIRE)
			return false;
		BaseTarget t = u.m_CombatComponent.GetCurrentTarget();
		if (!t || !t.GetTargetEntity() || t.GetTimeSinceSeen() > 1.5)
			return false;
		dist = vector.Distance(t.GetTargetEntity().GetOrigin(), u.m_OwnerEntity.GetOrigin());
		if (dist > maxD)
			return false;
		DCO_Perf.Count("yield:" + what);
		Log(u, what, t, dist);
		return true;
	}

	protected static void Log(SCR_AIUtilityComponent u, string what, BaseTarget t, float dist)
	{
		if (!s_mLogAt)
			s_mLogAt = new map<IEntity, float>();
		if (s_mLogAt.Count() > 500)
			s_mLogAt.Clear();
		float now = DCO_Util.NowS();
		float last;
		if (s_mLogAt.Find(u.m_OwnerEntity, last) && now - last < 10.0)
			return;
		s_mLogAt.Set(u.m_OwnerEntity, now);
		DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(u.m_OwnerEntity), string.Format("move_yield unit=%1 beh=%2 tgt=%3 dist=%4", DCO_Log.IdUnit(u.m_OwnerEntity), what, DCO_Log.IdUnit(t.GetTargetEntity()), Math.Round(dist)));
	}
}

class DCO_MoveIndividuallyBehavior : SCR_AIMoveIndividuallyBehavior
{
	protected float m_fDCOBase;
	protected string m_sDCOWhat;

	void DCO_MoveIndividuallyBehavior(SCR_AIUtilityComponent utility, SCR_AIActivityBase groupActivity, vector pos, float priority = PRIORITY_BEHAVIOR_MOVE_INDIVIDUALLY, float priorityLevel = PRIORITY_LEVEL_NORMAL, IEntity ent = null, float radius = 1.0)
	{
		m_fDCOBase = priority;
		m_sDCOWhat = "move";
	}

	void DCO_SetWhat(string what)
	{
		m_sDCOWhat = what;
	}

	override float CustomEvaluate()
	{
		float d;
		if (DCO_ContactYield.Should(m_Utility, m_sDCOWhat, d))
			return Math.Min(m_fDCOBase, 69.0);
		return m_fDCOBase;
	}
}
