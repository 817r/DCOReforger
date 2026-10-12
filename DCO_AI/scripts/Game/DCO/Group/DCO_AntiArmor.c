class DCO_ATClaim
{
	IEntity m_Agent;
	float m_fSide;
	float m_fUntil;
}

class DCO_AntiArmor
{
	protected static ref map<IEntity, ref DCO_ATClaim> s_mClaims;
	protected static map<IEntity, ref DCO_ATClaim> GetClaims()
	{
		if (!s_mClaims)
			s_mClaims = new map<IEntity, ref DCO_ATClaim>();
		return s_mClaims;
	}

	static float ClaimSide(IEntity armor, IEntity me, float preferred, float now, out bool paired)
	{
		paired = false;
		DCO_ATClaim c = GetClaims().Get(armor);
		if (c && c.m_Agent && c.m_Agent != me && now < c.m_fUntil && DCO_Util.IsAlive(c.m_Agent))
		{
			paired = true;
			DCO_Perf.Count("ev:at_pair");
			return -c.m_fSide;
		}

		if (GetClaims().Count() > 32)
		{
			array<IEntity> stale = {};
			foreach (IEntity k, DCO_ATClaim v : GetClaims())
			{
				if (!k || !v || now >= v.m_fUntil)
					stale.Insert(k);
			}
			foreach (IEntity sk : stale)
				GetClaims().Remove(sk);
		}

		if (!c)
		{
			c = new DCO_ATClaim();
			GetClaims().Set(armor, c);
		}
		c.m_Agent = me;
		c.m_fSide = preferred;
		c.m_fUntil = now + 60000.0;
		return preferred;
	}

	static bool Update(SCR_AIGroupUtilityComponent u, SCR_AIGroup grp, DCO_GroupUtilityComponent g)
	{
		if (!g || !u.m_Perception || g.IsPlayerGroup() || g.IsArmor() || g.IsInTransport() || g.HasAT())
			return false;
		IEntity leader = grp.GetLeaderEntity();
		PerceptionManager pm = GetGame().GetPerceptionManager();
		if (!leader || !pm)
			return false;

		float pmNow = pm.GetTime();
		vector lp = leader.GetOrigin();
		IEntity armor;
		float bestD = 300.0;
		foreach (SCR_AITargetInfo t : u.m_Perception.m_aTargets)
		{
			if (!t || !t.m_Entity || pmNow - t.m_fTimestamp > 10.0)
				continue;
			IEntity veh = DCO_VehicleCombat.TargetVehicle(t.m_Entity);
			if (!veh || !DCO_VehicleCombat.IsArmored(veh))
				continue;
			float d = vector.DistanceXZ(lp, t.m_vWorldPos);
			if (d < bestD)
			{
				bestD = d;
				armor = veh;
			}
		}
		if (!armor)
			return false;

		vector ap = armor.GetOrigin();
		DCO_GarrisonRegistry reg = DCO_GarrisonRegistry.GetInstance();
		array<AIAgent> agents = {};
		grp.GetAgents(agents);
		int moved;
		foreach (AIAgent a : agents)
		{
			SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
			if (!ca || !ca.m_UtilityComponent)
				continue;
			IEntity me = ca.GetControlledEntity();
			if (!me || !DCO_Util.IsAlive(me))
				continue;
			vector mp = me.GetOrigin();
			vector away = mp - ap;
			away[1] = 0;
			if (away.Length() < 1)
				continue;
			away.Normalize();

			vector pos = DCO_Util.Surface(mp + away * 15.0);
			if (reg)
			{
				array<IEntity> buildings = {};
				reg.FindBuildings(mp, 40.0, buildings);
				float bestB = float.MAX;
				foreach (IEntity b : buildings)
				{
					vector bp = b.GetOrigin();
					if (vector.DistanceXZ(bp, ap) < vector.DistanceXZ(mp, ap) - 5.0)
						continue;
					float bd = vector.DistanceXZ(bp, mp);
					if (bd < bestB)
					{
						bestB = bd;
						vector off = bp - ap;
						off[1] = 0;
						off.Normalize();
						pos = DCO_Util.Surface(bp + off * 6.0);
					}
				}
			}
			DCO_RelocateBehavior.Start(ca.m_UtilityComponent, pos, ap, true, ECharacterStance.CROUCH, vector.Distance(mp, pos) / 3.0 + 2.0, "at_hunt");
			moved++;
		}
		DCO_Perf.Count("ev:inf_armor_cover");
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, "-", string.Format("inf_armor_cover grp=%1 armor=%2 dist=%3 moved=%4", grp, armor, Math.Round(bestD), moved));

		AICommander_BaseComponent cmd = g.GetMyCommander();
		if (cmd && cmd.GetThreatResponseComponent())
		{
			CMD_ContactReport report = new CMD_ContactReport(ap, 1, DCO_Util.NowS(), "ARMOR:" + grp.GetName());
			report.m_bArmorSeen = true;
			report.m_iArmor = 1;
			cmd.GetThreatResponseComponent().ReceiveContactReport(report, g);
			DCO_Perf.Count("ev:inf_armor_request");
		}
		return true;
	}
}
