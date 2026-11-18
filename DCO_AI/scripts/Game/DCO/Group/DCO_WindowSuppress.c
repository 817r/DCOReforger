class DCO_WindowSuppress
{
	protected static ref map<AIGroup, float> s_mNextAt;
	protected static ref map<AIGroup, float> s_mLogAt;

	protected static void Skip(AIGroup grp, string why, vector estPos, float now_ms)
	{
		DCO_Perf.Count("window_supp:" + why);
		if (!s_mLogAt)
			s_mLogAt = new map<AIGroup, float>();
		float at;
		if (s_mLogAt.Find(grp, at) && now_ms < at)
			return;
		s_mLogAt.Set(grp, now_ms + 5000.0);
		if (s_mLogAt.Count() > 128)
			s_mLogAt.Clear();
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdGroup(grp), string.Format("window_suppress_skip grp=%1 why=%2 pos=%3", DCO_Log.IdGroup(grp), why, DCO_Util.XZ(estPos)));
	}

	static void OnShot(SCR_AIUtilityComponent u, AIGroup grp, IEntity shooter, vector estPos, float now_ms, string src = "shot")
	{
		if (!u || !grp || !shooter)
			return;
		if (!s_mNextAt)
			s_mNextAt = new map<AIGroup, float>();
		if (s_mNextAt.Count() > 128)
		{
			for (int pi = s_mNextAt.Count() - 1; pi >= 0; pi--)
			{
				if (!s_mNextAt.GetKey(pi))
					s_mNextAt.RemoveElement(pi);
			}
		}
		float next;
		if (s_mNextAt.Find(grp, next) && now_ms < next)
		{
			DCO_Perf.Count("window_supp:cooldown");
			return;
		}
		string method;
		IEntity bld = DCO_CoverManagerComponent.DCO_GetBuildingAtPos(estPos, null, method);
		if (!bld)
		{
			bld = DCO_IndoorNav.NearestBuilding(estPos, 15.0);
			if (!bld)
			{
				Skip(grp, "no_building", estPos, now_ms);
				return;
			}
			vector bMin;
			vector bMax;
			bld.GetWorldBounds(bMin, bMax);
			estPos[0] = Math.Clamp(estPos[0], bMin[0] + 0.5, bMax[0] - 0.5);
			estPos[2] = Math.Clamp(estPos[2], bMin[2] + 0.5, bMax[2] - 0.5);
			method = "near";
			DCO_Perf.Count("window_supp:near_bld");
		}
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
			if (!t || t.GetTargetEntity() != shooter || t.GetTimeSinceSeen() >= 3.0)
				continue;
			IEntity seer = ca.GetControlledEntity();
			if (seer && now_ms - DCO_ShotCounter.LastShotAt(seer) < 3000.0)
			{
				Skip(grp, "seen_engaged", estPos, now_ms);
				return;
			}
			DCO_Perf.Count("window_supp:seen_silent");
		}
		float cool = 10000.0;
		float dur = 6.0;
		if (stackCover)
		{
			cool = 10000.0;
			dur = 5.0;
		}
		s_mNextAt.Set(grp, now_ms + cool);
		int shooters;
		int gls;
		int rifles;
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
			bool rifle = !mg && !gl;
			if (rifle && !stackCover && rifles >= 2)
				continue;
			if (stackCover && (shooters >= 3 || mu.FindActionOfType(DCO_CQBMoveSlotBehavior)))
				continue;
			if (mu.FindActionOfType(SCR_AISuppressBehavior))
				continue;
			float d = vector.DistanceXZ(me.GetOrigin(), estPos);
			if (d < 15.0 || d > 400.0)
				continue;
			if (!DCO_SuppressGate.Allowed(mu, estPos, "window"))
				continue;
			float rate = 1.5;
			if (rifle && !stackCover)
				rate = 0.5;
			vector bbMin;
			vector bbMax;
			SCR_AISuppressionVolumeBase.CreateSuppressionBox(estPos, 2.0, 2.5, bbMin, bbMax);
			SCR_AISuppressionObjectVolumeBox vol = new SCR_AISuppressionObjectVolumeBox(bbMin, bbMax);
			mu.AddAction(new SCR_AISuppressBehavior(mu, null, vol, dur, rate));
			shooters++;
			if (gl)
				gls++;
			if (rifle)
				rifles++;
		}
		if (shooters == 0)
		{
			Skip(grp, "none", estPos, now_ms);
			return;
		}
		DCO_Perf.Count("window_supp:go");
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdGroup(grp), string.Format("window_suppress grp=%1 pos=%2 bld=%3 shooters=%4 gl=%5 rifles=%6 dur_s=%7 method=%8 src=%9", DCO_Log.IdGroup(grp), DCO_Util.XZ(estPos), DCO_Util.PrefabShort(bld), shooters, gls, rifles, Math.Round(dur), method, src));
	}
}
