class DCO_WindowSuppress
{
	protected static ref map<AIGroup, float> s_mNextAt;

	static void OnShot(SCR_AIUtilityComponent u, AIGroup grp, IEntity shooter, vector estPos, float now_ms, string src = "shot")
	{
		if (!u || !grp || !shooter)
			return;
		if (!s_mNextAt)
			s_mNextAt = new map<AIGroup, float>();
		float next;
		if (s_mNextAt.Find(grp, next) && now_ms < next)
			return;
		string method;
		IEntity bld = SCR_CoverManagerComponent.DCO_GetBuildingAtPos(estPos, null, method);
		if (!bld)
			return;
		bool stackCover = src == "cqb_stack";
		array<AIAgent> agents = {};
		grp.GetAgents(agents);
		foreach (AIAgent a : agents)
		{
			if (stackCover)
				break;
			SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
			if (!ca || !ca.m_UtilityComponent || !ca.m_UtilityComponent.m_CombatComponent)
				continue;
			BaseTarget t = ca.m_UtilityComponent.m_CombatComponent.GetCurrentTarget();
			if (t && t.GetTargetEntity() == shooter && t.GetTimeSinceSeen() < 3.0)
			{
				DCO_Perf.Count("window_supp:shooter_seen");
				return;
			}
		}
		float cool = 20000.0;
		float dur = 3.0;
		if (stackCover)
		{
			cool = 10000.0;
			dur = 5.0;
		}
		s_mNextAt.Set(grp, now_ms + cool);
		int shooters;
		int gls;
		foreach (AIAgent a2 : agents)
		{
			SCR_ChimeraAIAgent ca2 = SCR_ChimeraAIAgent.Cast(a2);
			if (!ca2 || !ca2.m_UtilityComponent || !ca2.m_UtilityComponent.m_CombatComponent)
				continue;
			SCR_AIUtilityComponent mu = ca2.m_UtilityComponent;
			IEntity me = ca2.GetControlledEntity();
			if (!me || !DCO_Util.IsAlive(me) || mu.m_CombatComponent.GetCombatMode() == EAIGroupCombatMode.HOLD_FIRE)
				continue;
			int roles = DCO_Roles.Get(mu);
			bool mg = (roles & 65) != 0;
			bool gl = (roles & 8) != 0;
			if (!mg && !gl && !stackCover)
				continue;
			if (stackCover && (shooters >= 3 || mu.FindActionOfType(DCO_CQBMoveSlotBehavior)))
				continue;
			if (mu.FindActionOfType(SCR_AISuppressBehavior))
				continue;
			float d = vector.DistanceXZ(me.GetOrigin(), estPos);
			if (d < 30.0 || d > 400.0)
				continue;
			if (!DCO_SuppressGate.Allowed(mu, estPos, "window"))
				continue;
			vector bbMin;
			vector bbMax;
			SCR_AISuppressionVolumeBase.CreateSuppressionBox(estPos, 2.0, 2.5, bbMin, bbMax);
			SCR_AISuppressionObjectVolumeBox vol = new SCR_AISuppressionObjectVolumeBox(bbMin, bbMax);
			mu.AddAction(new SCR_AISuppressBehavior(mu, null, vol, dur, 1.5));
			shooters++;
			if (gl)
				gls++;
		}
		if (shooters == 0)
		{
			DCO_Perf.Count("window_supp:none");
			return;
		}
		DCO_Perf.Count("window_supp:go");
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdGroup(grp), string.Format("window_suppress grp=%1 pos=%2 bld=%3 shooters=%4 gl=%5 method=%6 src=%7", DCO_Log.IdGroup(grp), DCO_Util.XZ(estPos), DCO_Util.PrefabShort(bld), shooters, gls, method, src));
	}
}
