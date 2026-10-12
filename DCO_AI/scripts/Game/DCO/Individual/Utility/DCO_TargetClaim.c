class DCO_TargetClaimSet : Managed
{
	ref map<IEntity, ref map<AIAgent, float>> m_mClaims = new map<IEntity, ref map<AIAgent, float>>();
	float m_fPrune;
}

class DCO_TargetClaim
{
	protected static ref map<AIGroup, ref DCO_TargetClaimSet> s_mGroups;
	protected static bool s_bLogged;

	static bool On(SCR_AIUtilityComponent u)
	{
		return DCO_Stress.T(u, "tgt.claim", 1.0) > 0;
	}

	protected static DCO_TargetClaimSet SetOf(AIGroup g, bool create)
	{
		if (!g)
			return null;
		if (!s_mGroups)
		{
			if (!create)
				return null;
			s_mGroups = new map<AIGroup, ref DCO_TargetClaimSet>();
		}
		DCO_TargetClaimSet s = s_mGroups.Get(g);
		if (!s && create)
		{
			if (s_mGroups.Count() > 400)
				s_mGroups.Clear();
			s = new DCO_TargetClaimSet();
			s_mGroups.Set(g, s);
		}
		return s;
	}

	static void Claim(SCR_AIUtilityComponent u, IEntity target)
	{
		if (!u || !target || !u.GetAIAgent())
			return;
		if (!s_bLogged)
		{
			s_bLogged = true;
			DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", string.Format("tgt_claim_active pen=%1 ttl=%2", DCO_Stress.T(u, "tgt.claim_pen", 1.5), DCO_Stress.T(u, "tgt.claim_ttl", 3.0)));
		}
		AIAgent me = u.GetAIAgent();
		DCO_TargetClaimSet s = SetOf(me.GetParentGroup(), true);
		if (!s)
			return;
		float now = GetGame().GetWorld().GetWorldTime() * 0.001;
		foreach (IEntity k, map<AIAgent, float> m : s.m_mClaims)
		{
			if (m && k != target)
				m.Remove(me);
		}
		map<AIAgent, float> claims = s.m_mClaims.Get(target);
		if (!claims)
		{
			claims = new map<AIAgent, float>();
			s.m_mClaims.Set(target, claims);
		}
		claims.Set(me, now);
		if (now >= s.m_fPrune)
		{
			s.m_fPrune = now + 10.0;
			array<IEntity> dead = {};
			foreach (IEntity k2, map<AIAgent, float> m2 : s.m_mClaims)
			{
				if (!k2 || !m2 || m2.IsEmpty() || !DCO_Util.IsAlive(k2))
					dead.Insert(k2);
			}
			foreach (IEntity d : dead)
				s.m_mClaims.Remove(d);
		}
	}

	static int Others(SCR_AIUtilityComponent u, IEntity target)
	{
		if (!u || !target || !u.GetAIAgent())
			return 0;
		AIAgent me = u.GetAIAgent();
		DCO_TargetClaimSet s = SetOf(me.GetParentGroup(), false);
		if (!s)
			return 0;
		map<AIAgent, float> claims = s.m_mClaims.Get(target);
		if (!claims)
			return 0;
		float now = GetGame().GetWorld().GetWorldTime() * 0.001;
		float ttl = DCO_Stress.T(u, "tgt.claim_ttl", 3.0);
		int n;
		foreach (AIAgent a, float at : claims)
		{
			if (a && a != me && now - at <= ttl && DCO_Util.IsAlive(a.GetControlledEntity()))
				n++;
		}
		return n;
	}
}
