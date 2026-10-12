class DCO_AssaultLock
{
	static SCR_AIGroupUtilityComponent GroupUtil(AIGroup g)
	{
		SCR_AIGroup sg = SCR_AIGroup.Cast(g);
		if (!sg)
			return null;
		return sg.GetGroupUtilityComponent();
	}

	static int Mode(AIGroup g)
	{
		SCR_AIGroupUtilityComponent gu = GroupUtil(g);
		if (!gu)
			return 0;
		if (gu.DCO_GetCQBActivity())
			return 1;
		return 0;
	}

	static bool InCQB(SCR_AIUtilityComponent u)
	{
		if (!u)
			return false;
		AIAgent agent = u.GetAIAgent();
		if (!agent || !agent.GetParentGroup())
			return false;
		SCR_AIGroupUtilityComponent gu = GroupUtil(agent.GetParentGroup());
		return gu && gu.DCO_GetCQBActivity();
	}

	static bool SmokeBlocked(SCR_AIUtilityComponent u, string path)
	{
		if (!u || !u.m_OwnerEntity)
			return false;
		string lim;
		int ans = DCO_CtrlLock.AskUtil(u, DCO_ECtrlReq.SMOKE, lim);
		int owner = DCO_CtrlLock.OwnerOf(u.GetAIAgent());
		bool catchAll = path == "throw_behavior";
		bool deny = ans == DCO_ECtrlAns.DENY;
		if (ans == DCO_ECtrlAns.LIMIT && lim == "planned_only" && !catchAll)
			deny = true;
		if (deny && catchAll && owner != DCO_ECtrlOwner.CQB_ENTRY && owner != DCO_ECtrlOwner.CQB_SUPPORT && owner != DCO_ECtrlOwner.STATIC_GUNNER)
			deny = false;
		string reason = "owner_" + DCO_CtrlLock.OwnerName(owner);
		if (owner == DCO_ECtrlOwner.CQB_ENTRY || owner == DCO_ECtrlOwner.CQB_SUPPORT)
			reason = "cqb_active";
		float bd = 0;
		if (!deny)
		{
			string fk = DCO_Util.FactionKeyOf(u.m_OwnerEntity, false);
			if (fk.IsEmpty() || !DCO_BuildingClear.ActiveClearNear(u.m_OwnerEntity.GetOrigin(), 100.0, fk, bd))
				return false;
			reason = "cqb_near";
		}
		DCO_Perf.Count("smoke_block:cqb");
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, "-", string.Format("smoke_block unit=%1 path=%2 reason=%3 bld_dist=%4 owner=%5", DCO_Log.IdUnit(u.m_OwnerEntity), path, reason, Math.Round(bd), DCO_CtrlLock.OwnerName(owner)));
		return true;
	}

	static bool CheckBlock(SCR_AIUtilityComponent u, string reaction)
	{
		if (!u)
			return false;
		int req = DCO_ECtrlReq.DODGE;
		if (reaction == "prone" || reaction == "pinned")
			req = DCO_ECtrlReq.PRONE;
		else if (reaction == "morale_flee")
			req = DCO_ECtrlReq.MORALE;
		string lim;
		int ans = DCO_CtrlLock.AskUtil(u, req, lim);
		if (ans == DCO_ECtrlAns.ALLOW)
			return false;
		if (req == DCO_ECtrlReq.MORALE && DCO_CtrlLock.OwnerOf(u.GetAIAgent()) == DCO_ECtrlOwner.FM_FIX && u.GetAIAgent() && DCO_CtrlLock.GroupLoss(u.GetAIAgent().GetParentGroup()) >= 0.35)
		{
			DCO_Perf.Count("ctrl:allow_FM_FIX_MORALE_losses");
			return false;
		}
		if (lim == "bound_pause_only")
		{
			DCO_BoundBehavior bb = DCO_BoundBehavior.Cast(u.GetCurrentAction());
			if (bb && bb.DCO_IsPausing())
				return false;
		}
		int owner = DCO_CtrlLock.OwnerOf(u.GetAIAgent());
		string prefix = "tr:aggr_block_";
		if (owner == DCO_ECtrlOwner.CQB_ENTRY || owner == DCO_ECtrlOwner.CQB_SUPPORT)
			prefix = "cqb:aggr_block_";
		DCO_Perf.Count(prefix + reaction);
		if (DCO_Log.On(DCO_ELogChannel.CQB, DCO_ELogLevel.DEBUG))
			DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(u.m_OwnerEntity), string.Format("aggr_block unit=%1 reaction=%2 owner=%3 limit=%4 src=%5", DCO_Log.IdUnit(u.m_OwnerEntity), reaction, DCO_CtrlLock.OwnerName(owner), lim, DCO_CtrlLock.OwnerWhy()));
		return true;
	}

	static float DodgeCap(SCR_AIUtilityComponent u)
	{
		if (!u || !u.m_OwnerEntity)
			return -1;
		string lim;
		int ans = DCO_CtrlLock.AskUtil(u, DCO_ECtrlReq.DODGE, lim);
		if (ans == DCO_ECtrlAns.ALLOW)
			return -1;
		if (lim != "radius3")
			return 0;
		AIAgent agent = u.GetAIAgent();
		vector spot = vector.Zero;
		SCR_AIGroupUtilityComponent gu;
		if (agent)
			gu = GroupUtil(agent.GetParentGroup());
		if (gu && gu.DCO_GetCQBActivity())
			spot = gu.DCO_GetCQBActivity().SupportSpotOf(agent);
		float cap = 3.0;
		if (spot != vector.Zero)
			cap = 3.0 - vector.Distance(u.m_OwnerEntity.GetOrigin(), spot);
		if (cap >= 1.0 && !(u.m_CombatMoveState && u.m_CombatMoveState.IsExecutingRequest()))
			return cap;
		DCO_Perf.Count("cqb:aggr_block_dodge_support");
		if (DCO_Log.On(DCO_ELogChannel.CQB, DCO_ELogLevel.DEBUG))
			DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(u.m_OwnerEntity), string.Format("aggr_block unit=%1 reaction=dodge owner=%2 spot_dist=%3", DCO_Log.IdUnit(u.m_OwnerEntity), DCO_CtrlLock.OwnerName(DCO_CtrlLock.OwnerOf(agent)), Math.Round(3.0 - cap)));
		return 0;
	}

	static bool LoaOk(SCR_AIGroupUtilityComponent gu, IEntity building, out string reason)
	{
		reason = "";
		return true;
	}
}
