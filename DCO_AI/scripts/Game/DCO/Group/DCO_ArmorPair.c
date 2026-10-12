class DCO_PairState
{
	ref array<DCO_GroupUtilityComponent> m_aInf = {};
	DCO_GroupUtilityComponent m_Armor;
	float m_fDiverge_ms = -1;
	float m_fIdle_ms = -1;
}

class DCO_ArmorPair
{
	protected static ref array<DCO_GroupUtilityComponent> s_aInf;
	protected static array<DCO_GroupUtilityComponent> GetInf()
	{
		if (!s_aInf)
			s_aInf = {};
		return s_aInf;
	}
	protected static ref array<float> s_aInfAt;
	protected static array<float> GetInfAt()
	{
		if (!s_aInfAt)
			s_aInfAt = {};
		return s_aInfAt;
	}
	protected static ref array<bool> s_aInfContact;
	protected static array<bool> GetInfContact()
	{
		if (!s_aInfContact)
			s_aInfContact = {};
		return s_aInfContact;
	}
	protected static ref map<SCR_AIGroup, AIWaypoint> s_mOwnWp;
	protected static map<SCR_AIGroup, AIWaypoint> GetOwnWp()
	{
		if (!s_mOwnWp)
			s_mOwnWp = new map<SCR_AIGroup, AIWaypoint>();
		return s_mOwnWp;
	}

	static bool ArmedCrew(DCO_GroupUtilityComponent g)
	{
		IEntity veh = g.GetGroupVehicle();
		if (!veh || DCO_VehicleActive.VehClass(veh) == 0)
			return false;
		SCR_AIGroup grp = SCR_AIGroup.Cast(g.GetOwner());
		if (!grp)
			return false;
		array<AIAgent> agents = {};
		grp.GetAgents(agents);
		foreach (AIAgent a : agents)
		{
			IEntity e = a.GetControlledEntity();
			if (e && DCO_Util.IsAlive(e) && DCO_VehicleCombat.GetVehicle(e) != veh)
				return false;
		}
		return true;
	}

	protected static SCR_AIUtilityComponent LeaderUtil(SCR_AIGroup grp)
	{
		SCR_ChimeraAIAgent la = SCR_ChimeraAIAgent.Cast(grp.GetLeaderAgent());
		if (!la)
			return null;
		return la.m_UtilityComponent;
	}

	protected static bool Eligible(DCO_GroupUtilityComponent g)
	{
		return g && !g.GetMyCommander() && !g.IsPlayerGroup() && !g.m_DCOSupportOf && !g.IsDedicatedTransport() && g.GetUnitCount() > 0;
	}

	protected static bool SameFaction(DCO_GroupUtilityComponent a, DCO_GroupUtilityComponent b)
	{
		SCR_AIGroup ga = SCR_AIGroup.Cast(a.GetOwner());
		SCR_AIGroup gb = SCR_AIGroup.Cast(b.GetOwner());
		return ga && gb && ga.GetFaction() && ga.GetFaction() == gb.GetFaction();
	}

	protected static bool InContact(SCR_AIGroupUtilityComponent u)
	{
		return u && u.m_Perception && !u.m_Perception.m_aTargetClusters.IsEmpty();
	}

	protected static bool Moving(SCR_AIGroup grp)
	{
		IEntity l = grp.GetLeaderEntity();
		return grp.GetCurrentWaypoint() || (l && l.GetPhysics() && l.GetPhysics().GetVelocity().Length() > 1.0);
	}

	static void Tick(DCO_GroupUtilityComponent g, SCR_AIGroup grp, SCR_AIGroupUtilityComponent u)
	{
		float now = GetGame().GetWorld().GetWorldTime();
		if (!Eligible(g))
		{
			if (g && g.m_DCOPair)
				Break(g.m_DCOPair, "commander");
			return;
		}

		if (g.IsArmor() || ArmedCrew(g))
		{
			if (g.m_DCOPair && g.m_DCOPair.m_Armor != g)
			{
				DCO_Perf.Count("pair:break_inf_now_armor");
				Break(g.m_DCOPair, "inf_now_armor");
			}
			TickArmor(g, grp, u, now);
			return;
		}
		if (!g.IsInfantry() || g.GetGroupVehicle())
		{
			if (g.m_DCOPair && g.m_DCOPair.m_Armor != g)
			{
				DCO_Perf.Count("pair:break_inf_mounted");
				Break(g.m_DCOPair, "inf_mounted");
			}
			return;
		}

		bool contact = InContact(u);
		bool moving = Moving(grp);
		if (!contact && !moving)
			return;
		int idx = GetInf().Find(g);
		if (idx < 0)
		{
			if (GetInf().Count() >= 64)
				Prune(now, true);
			GetInf().Insert(g);
			GetInfAt().Insert(now);
			GetInfContact().Insert(contact);
		}
		else
		{
			GetInfAt()[idx] = now;
			GetInfContact()[idx] = contact;
		}
	}

	protected static void Prune(float now, bool force)
	{
		for (int i = GetInf().Count() - 1; i >= 0; i--)
		{
			if (!GetInf()[i] || now - GetInfAt()[i] > 10000.0 || (force && i == 0))
			{
				GetInf().Remove(i);
				GetInfAt().Remove(i);
				GetInfContact().Remove(i);
			}
		}
	}

	protected static void TickArmor(DCO_GroupUtilityComponent g, SCR_AIGroup grp, SCR_AIGroupUtilityComponent u, float now)
	{
		Prune(now, false);
		IEntity veh = g.GetGroupVehicle();
		if (!veh)
		{
			if (g.m_DCOPair)
				Break(g.m_DCOPair, "destroyed");
			return;
		}
		vector ap = veh.GetOrigin();

		DCO_PairState st = g.m_DCOPair;
		if (!st)
		{
			DCO_GroupUtilityComponent best;
			float bestD = 150.0;
			foreach (DCO_GroupUtilityComponent inf : GetInf())
			{
				if (!Eligible(inf) || inf.m_DCOPair || !SameFaction(inf, g) || inf.GetGroupVehicle() || inf.IsArmor())
					continue;
				float d = vector.DistanceXZ(inf.GetOwner().GetOrigin(), ap);
				if (d < bestD)
				{
					bestD = d;
					best = inf;
				}
			}
			if (best)
			{
				st = new DCO_PairState();
				st.m_Armor = g;
				st.m_aInf.Insert(best);
				g.m_DCOPair = st;
				best.m_DCOPair = st;
				foreach (DCO_GroupUtilityComponent inf2 : GetInf())
				{
					if (inf2 && inf2 != best && !inf2.m_DCOPair && Eligible(inf2) && SameFaction(inf2, g) && !inf2.GetGroupVehicle() && !inf2.IsArmor()
						&& vector.DistanceXZ(inf2.GetOwner().GetOrigin(), ap) < 150.0)
					{
						st.m_aInf.Insert(inf2);
						inf2.m_DCOPair = st;
						break;
					}
				}
				DCO_Perf.Count("pair:form");
				DCO_Log.Write(DCO_ELogChannel.VEH, DCO_ELogLevel.INFO, "-", string.Format("pair_form armor=%1 inf=%2 count=%3 dist=%4", grp, best.GetOwner(), st.m_aInf.Count(), Math.Round(bestD)));
			}
			else
			{
				IdleSupport(g, grp, ap, now);
				return;
			}
		}

		for (int i = st.m_aInf.Count() - 1; i >= 0; i--)
		{
			DCO_GroupUtilityComponent inf = st.m_aInf[i];
			if (!inf || inf.GetUnitCount() <= 0 || inf.GetMyCommander() || vector.DistanceXZ(inf.GetOwner().GetOrigin(), ap) > 300.0)
			{
				if (inf)
					inf.m_DCOPair = null;
				st.m_aInf.Remove(i);
			}
		}
		if (st.m_aInf.IsEmpty())
		{
			Break(st, "lost_infantry");
			return;
		}

		DCO_GroupUtilityComponent lead = st.m_aInf[0];
		SCR_AIGroup infGrp = SCR_AIGroup.Cast(lead.GetOwner());
		IEntity il = infGrp.GetLeaderEntity();
		if (!il)
			return;
		vector lp = il.GetOrigin();
		SCR_AIGroupUtilityComponent iu = infGrp.GetGroupUtilityComponent();
		bool contact = InContact(iu) || InContact(u);

		if (!contact && !Moving(infGrp))
		{
			if (st.m_fIdle_ms < 0)
				st.m_fIdle_ms = now;
			else if (now - st.m_fIdle_ms > 60000.0)
			{
				Break(st, "idle");
				return;
			}
		}
		else
			st.m_fIdle_ms = -1;

		vector heading = Heading(infGrp, il);
		vector av;
		if (veh.GetPhysics())
			av = veh.GetPhysics().GetVelocity();
		av[1] = 0;
		vector iv;
		if (il.GetPhysics())
			iv = il.GetPhysics().GetVelocity();
		iv[1] = 0;
		if (av.Length() > 1.0 && iv.Length() > 1.0 && vector.Dot(av.Normalized(), iv.Normalized()) < 0)
		{
			if (st.m_fDiverge_ms < 0)
				st.m_fDiverge_ms = now;
			else if (now - st.m_fDiverge_ms > 20000.0)
			{
				Break(st, "diverge");
				return;
			}
		}
		else
			st.m_fDiverge_ms = -1;

		bool close = CloseTerrain(ap);
		bool at = DCO_VehicleCombat.IsATKnown(veh, LeaderUtil(grp));
		vector side = Vector(heading[2], 0, -heading[0]);
		vector desired;
		if (at)
			desired = lp - heading * 60.0;
		else if (close)
		{
			desired = lp - heading * 50.0;
			DCO_Perf.Count("pair:inf_lead_urban");
		}
		else
			desired = lp + heading * 30.0 + side * 20.0;
		desired = DCO_Util.Surface(desired);

		if (contact && !close)
			desired = HullDown(desired, EnemyCenter(iu, u));
		if (vector.DistanceXZ(ap, desired) > 30.0)
		{
			Move(grp, desired);
			DCO_Perf.Count("pair:armor_follow_inf");
		}

		MarkTargets(iu, u, now);
	}

	static vector HullDown(vector pos, vector threat)
	{
		if (threat == vector.Zero || !DCO_GlobalAIComponent.BehaviorOn(DCO_EBehavior.COMBINED_ARMS) || vector.DistanceXZ(pos, threat) < 80.0)
			return pos;
		vector eye = DCO_Util.Surface(threat) + Vector(0, 1.7, 0);
		vector best = vector.Zero;
		float bestScore = -float.MAX;
		for (int i = 0; i < 12; i++)
		{
			float r = 12.0;
			if (i >= 6)
				r = 28.0;
			float a = 1.0 * i * Math.PI / 3.0 + 0.5 * (i / 6);
			vector c = DCO_Util.Surface(pos + Vector(Math.Cos(a) * r, 0, Math.Sin(a) * r));
			if (DCO_Util.TraceFrac(eye, c + Vector(0, 1.0, 0), null, TraceFlags.WORLD, -1, "t:DCO_HullDown") >= 0.98)
				continue;
			if (DCO_Util.TraceFrac(eye, c + Vector(0, 2.6, 0), null, TraceFlags.WORLD, -1, "t:DCO_HullDown") < 0.99)
				continue;
			float s = -vector.DistanceXZ(c, pos) - Math.Max(0, vector.DistanceXZ(pos, threat) - vector.DistanceXZ(c, threat)) * 2.0;
			if (s > bestScore)
			{
				bestScore = s;
				best = c;
			}
		}
		if (best == vector.Zero)
			return pos;
		DCO_Perf.Count("armor_hull_down");
		return best;
	}

	protected static vector Heading(SCR_AIGroup grp, IEntity leader)
	{
		vector h;
		if (grp.GetCurrentWaypoint())
			h = grp.GetCurrentWaypoint().GetOrigin() - leader.GetOrigin();
		else if (leader.GetPhysics())
			h = leader.GetPhysics().GetVelocity();
		h[1] = 0;
		if (h.Length() < 0.5)
			h = leader.GetWorldTransformAxis(2);
		h[1] = 0;
		return h.Normalized();
	}

	protected static vector EnemyCenter(SCR_AIGroupUtilityComponent a, SCR_AIGroupUtilityComponent b)
	{
		SCR_AIGroupUtilityComponent u = a;
		if (!u || !u.m_Perception || u.m_Perception.m_aTargetClusters.IsEmpty())
			u = b;
		if (!u || !u.m_Perception)
			return vector.Zero;
		foreach (SCR_AIGroupTargetCluster c : u.m_Perception.m_aTargetClusters)
		{
			if (c.m_State && c.m_State.m_iCountAlive > 0)
				return c.m_State.GetCenterPosition();
		}
		return vector.Zero;
	}

	protected static bool CloseTerrain(vector pos)
	{
		DCO_GarrisonRegistry reg = DCO_GarrisonRegistry.GetInstance();
		if (reg)
		{
			array<IEntity> buildings = {};
			reg.FindBuildings(pos, 60.0, buildings);
			if (buildings.Count() >= 4)
				return true;
		}
		array<DCO_TerrainPoint> pts = {};
		return DCO_TerrainCache.Query(DCO_ETerrainFlag.FOREST_EDGE, pos, 80.0, pts) >= 3;
	}

	static void MarkTargets(SCR_AIGroupUtilityComponent iu, SCR_AIGroupUtilityComponent au, float now)
	{
		PerceptionManager pm = GetGame().GetPerceptionManager();
		if (!pm || !iu || !au || !iu.m_Perception || !au.m_Perception)
			return;
		float pmNow = pm.GetTime();
		int n;
		foreach (SCR_AITargetInfo t : iu.m_Perception.m_aTargets)
		{
			if (n >= 3)
				break;
			if (!t || !t.m_Entity || pmNow - t.m_fTimestamp > 5.0)
				continue;
			if (t.m_eCategory != EAITargetInfoCategory.DETECTED && t.m_eCategory != EAITargetInfoCategory.IDENTIFIED)
				continue;
			FactionAffiliationComponent fac = FactionAffiliationComponent.Cast(t.m_Entity.FindComponent(FactionAffiliationComponent));
			if (!fac || !fac.GetAffiliatedFaction())
				continue;
			au.m_Perception.AddOrUpdateGunshot(t.m_Entity, t.m_vWorldPos, fac.GetAffiliatedFaction(), pmNow, false);
			n++;
		}
		if (n > 0)
			DCO_Perf.Count("pair:marked_target_fire");
	}

	protected static void IdleSupport(DCO_GroupUtilityComponent g, SCR_AIGroup grp, vector ap, float now)
	{
		if (grp.GetCurrentWaypoint())
			return;

		DCO_GroupUtilityComponent target;
		float bestD = 300.0;
		foreach (int i, DCO_GroupUtilityComponent inf : GetInf())
		{
			if (!inf || !GetInfContact()[i] || !SameFaction(inf, g))
				continue;
			float d = vector.DistanceXZ(inf.GetOwner().GetOrigin(), ap);
			if (d < bestD)
			{
				bestD = d;
				target = inf;
			}
		}

		DCO_ArmorSupportMemo memo = DCO_ArmorSupportMemo.Of(g);
		if (target)
		{
			if (!memo.m_Inf)
			{
				memo.m_vHome = ap;
				DCO_Perf.Count("pair:idle_armor_support");
				DCO_Log.Write(DCO_ELogChannel.VEH, DCO_ELogLevel.INFO, "-", string.Format("idle_armor_support armor=%1 inf=%2 dist=%3", grp, target.GetOwner(), Math.Round(bestD)));
			}
			memo.m_Inf = target;
			memo.m_fQuiet_ms = -1;
			IEntity il = SCR_AIGroup.Cast(target.GetOwner()).GetLeaderEntity();
			if (il)
			{
				vector back = ap - il.GetOrigin();
				back[1] = 0;
				if (back.Length() > 1)
					back.Normalize();
				vector pos = DCO_Util.Surface(il.GetOrigin() + back * 80.0);
				if (vector.DistanceXZ(ap, pos) > 40.0)
					Move(grp, pos);
			}
			return;
		}

		if (!memo.m_Inf)
			return;
		if (memo.m_fQuiet_ms < 0)
			memo.m_fQuiet_ms = now;
		else if (now - memo.m_fQuiet_ms > 60000.0)
		{
			Move(grp, memo.m_vHome);
			memo.m_Inf = null;
			DCO_Log.Write(DCO_ELogChannel.VEH, DCO_ELogLevel.INFO, "-", string.Format("idle_armor_return armor=%1", grp));
		}
	}

	static void Break(DCO_PairState st, string why)
	{
		if (!st)
			return;
		foreach (DCO_GroupUtilityComponent inf : st.m_aInf)
		{
			if (inf && inf.m_DCOPair == st)
				inf.m_DCOPair = null;
		}
		st.m_aInf.Clear();
		if (st.m_Armor && st.m_Armor.m_DCOPair == st)
		{
			SCR_AIGroup ag = SCR_AIGroup.Cast(st.m_Armor.GetOwner());
			if (ag && !st.m_Armor.GetMyCommander())
			{
				ClearOwn(ag);
				array<AIWaypoint> left = {};
				ag.GetWaypoints(left);
				DCO_Log.Write(DCO_ELogChannel.VEH, DCO_ELogLevel.INFO, "-", string.Format("pair_restore_wp armor=%1 n=%2", DCO_Log.IdGroup(ag), left.Count()));
			}
			st.m_Armor.m_DCOPair = null;
		}
		DCO_Perf.Count("pair:break:" + why);
		DCO_Log.Write(DCO_ELogChannel.VEH, DCO_ELogLevel.INFO, "-", string.Format("pair_break why=%1", why));
	}

	protected static void Move(SCR_AIGroup grp, vector pos)
	{
		Resource res = Resource.Load("{750A8D1695BD6998}Prefabs/AI/Waypoints/AIWaypoint_Move.et");
		if (!res || !res.IsValid())
			return;
		pos[1] = GetGame().GetWorld().GetSurfaceY(pos[0], pos[2]);
		EntitySpawnParams params = EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		Math3D.MatrixIdentity4(params.Transform);
		params.Transform[3] = pos;
		SCR_AIWaypoint wp = SCR_AIWaypoint.Cast(GetGame().SpawnEntityPrefab(res, GetGame().GetWorld(), params));
		if (!wp)
			return;
		wp.SetCompletionRadius(15.0);
		ClearOwn(grp);
		grp.AddWaypointAt(wp, 0);
		GetOwnWp().Set(grp, wp);
	}

	protected static void ClearOwn(SCR_AIGroup grp)
	{
		AIWaypoint old;
		if (!GetOwnWp().Find(grp, old))
			return;
		GetOwnWp().Remove(grp);
		if (!old)
			return;
		array<AIWaypoint> wps = {};
		grp.GetWaypoints(wps);
		if (wps.Contains(old))
			grp.RemoveWaypoint(old);
		SCR_EntityHelper.DeleteEntityAndChildren(old);
	}
}

class DCO_ArmorSupportMemo
{
	DCO_GroupUtilityComponent m_Inf;
	vector m_vHome;
	float m_fQuiet_ms = -1;

	protected static ref map<DCO_GroupUtilityComponent, ref DCO_ArmorSupportMemo> s_mMemo;
	protected static map<DCO_GroupUtilityComponent, ref DCO_ArmorSupportMemo> GetMemo()
	{
		if (!s_mMemo)
			s_mMemo = new map<DCO_GroupUtilityComponent, ref DCO_ArmorSupportMemo>();
		return s_mMemo;
	}

	static DCO_ArmorSupportMemo Of(DCO_GroupUtilityComponent g)
	{
		DCO_ArmorSupportMemo m = GetMemo().Get(g);
		if (!m)
		{
			if (GetMemo().Count() > 64)
			{
				array<DCO_GroupUtilityComponent> stale = {};
				foreach (DCO_GroupUtilityComponent k, DCO_ArmorSupportMemo v : GetMemo())
				{
					if (!k)
						stale.Insert(k);
				}
				foreach (DCO_GroupUtilityComponent sk : stale)
					GetMemo().Remove(sk);
			}
			m = new DCO_ArmorSupportMemo();
			GetMemo().Set(g, m);
		}
		return m;
	}
}
