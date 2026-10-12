class DCO_Air
{
	static bool On()
	{
		return DCO_Ctx.TuneG("air.on", 0.0) > 0;
	}

	static bool IsAir(IEntity e)
	{
		if (!e)
			return false;
		IEntity root = e.GetRootParent();
		if (!root)
			root = e;
		PerceivableComponent p = PerceivableComponent.Cast(root.FindComponent(PerceivableComponent));
		return p && p.GetUnitType() == EAIUnitType.UnitType_Aircraft;
	}

	static bool IsAirTarget(BaseTarget t)
	{
		return t && On() && IsAir(t.GetTargetEntity());
	}

	static bool InfoIsAir(SCR_AITargetInfo i)
	{
		return i && i.m_Perceivable && i.m_Perceivable.GetUnitType() == EAIUnitType.UnitType_Aircraft;
	}

	static float ClusterAirDist(SCR_AITargetClusterState s, float freshS)
	{
		if (!s || !s.m_Cluster || s.m_iCountAlive <= 0 || s.GetTimeSinceLastNewInformation() > freshS)
			return float.MAX;
		foreach (SCR_AITargetInfo i : s.m_Cluster.m_aTargets)
		{
			if (InfoIsAir(i))
				return s.m_fDistMin;
		}
		return float.MAX;
	}

	static float OpenRange()
	{
		return DCO_Ctx.TuneG("air.open_m", 750.0);
	}

	static float CapRate(BaseTarget t, float rate, float dist, EWeaponType wt)
	{
		if (IsAirTarget(t))
		{
			DCO_Perf.Count("air:cap_skip");
			return rate;
		}
		return DCO_LongRange.CapFireRate(rate, dist, wt);
	}

	static void Note(string what, string grp, float dist)
	{
		DCO_Perf.Count("air:" + what);
		if (DCO_Log.On(DCO_ELogChannel.SQUAD, DCO_ELogLevel.DEBUG))
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.DEBUG, grp, string.Format("air_%1 grp=%2 dist=%3", what, grp, Math.Round(dist)));
	}
}
