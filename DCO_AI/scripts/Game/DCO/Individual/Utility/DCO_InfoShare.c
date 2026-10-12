class DCO_Designation
{
	int m_iKind;
	vector m_vPos;
	IEntity m_Target;
	IEntity m_From;
	float m_fAt_ms;
	int m_iUses;
}

class DCO_InfoQ
{
	static bool On()
	{
		return DCO_Ctx.TuneG("is.quality", 0.0) > 0;
	}

	static float EstRadius(float baseRad, float age_s, int src, int hop)
	{
		if (!On())
			return baseRad;
		float r = baseRad + 0.5 * age_s;
		if (src == 1)
			r = r + 5.0 * hop;
		return Math.Clamp(r, 4.0, 60.0);
	}

	static float EstConf(float age_s, int src, int hop, int count)
	{
		float c = 1.0;
		if (src == 1)
			c = Math.Pow(0.7, hop);
		c = c * Math.Clamp(1.0 - age_s / 30.0, 0.2, 1.0);
		c = c * Math.Min(1.0, 0.5 + 0.25 * count);
		return c;
	}

	static float TargetRadius(float age_s, bool reported)
	{
		if (!On())
			return 6.0;
		float r = 4.0 + 1.0 * age_s;
		if (reported)
			r = r + 15.0;
		return Math.Clamp(r, 4.0, 40.0);
	}

	static float SeenRadius(float age_s)
	{
		if (!On())
			return 8.0;
		return Math.Clamp(3.0 + 1.5 * age_s, 3.0, 30.0);
	}
}

class DCO_Designate
{
	protected static ref map<AIGroup, ref array<ref DCO_Designation>> s_mDes;
	protected static map<AIGroup, ref array<ref DCO_Designation>> GetDes()
	{
		if (!s_mDes)
			s_mDes = new map<AIGroup, ref array<ref DCO_Designation>>();
		return s_mDes;
	}

	static bool On(SCR_AIUtilityComponent u)
	{
		return DCO_Stress.T(u, "is.designate", 0.0) > 0;
	}

	static string KindName(int k)
	{
		if (k == 1)
			return "armor";
		if (k == 2)
			return "point";
		if (k == 4)
			return "cluster";
		return "none";
	}

	static void Post(AIGroup grp, int kind, vector pos, IEntity tgt, IEntity from, float now, string why)
	{
		if (!grp)
			return;
		array<ref DCO_Designation> list = GetDes().Get(grp);
		if (!list)
		{
			if (GetDes().Count() > 96)
				Prune(now);
			list = {};
			GetDes().Set(grp, list);
		}
		for (int i = list.Count() - 1; i >= 0; i--)
		{
			if (now - list[i].m_fAt_ms > 20000.0)
				list.Remove(i);
		}
		foreach (DCO_Designation d : list)
		{
			if (d.m_iKind == kind && vector.DistanceXZ(d.m_vPos, pos) < 15.0)
			{
				d.m_vPos = pos;
				d.m_Target = tgt;
				d.m_From = from;
				d.m_fAt_ms = now;
				DCO_Perf.Count("designate:refresh");
				return;
			}
		}
		if (list.Count() >= 6)
			list.Remove(0);
		DCO_Designation nd = new DCO_Designation();
		nd.m_iKind = kind;
		nd.m_vPos = pos;
		nd.m_Target = tgt;
		nd.m_From = from;
		nd.m_fAt_ms = now;
		list.Insert(nd);
		DCO_Perf.Count("designate:new_" + KindName(kind));
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(from), string.Format("spec_designate grp=%1 from=%2 kind=%3 why=%4 pos=%5 target=%6 dist=%7", DCO_Log.IdGroup(grp), DCO_Log.IdUnit(from), KindName(kind), why, DCO_Log.Pos(pos), DCO_Log.IdUnit(tgt), Math.Round(vector.DistanceXZ(from.GetOrigin(), pos))));
	}

	static DCO_Designation Best(AIGroup grp, int kindMask, vector myPos, float now, float maxAge_ms, float minD, float maxD)
	{
		if (!grp)
			return null;
		array<ref DCO_Designation> list = GetDes().Get(grp);
		if (!list)
			return null;
		DCO_Designation best;
		foreach (DCO_Designation d : list)
		{
			if (!(d.m_iKind & kindMask) || now - d.m_fAt_ms > maxAge_ms)
				continue;
			float dist = vector.DistanceXZ(myPos, d.m_vPos);
			if (dist < minD || dist > maxD)
				continue;
			if (!best || d.m_fAt_ms > best.m_fAt_ms)
				best = d;
		}
		return best;
	}

	static void Note(DCO_Designation d, IEntity user, string role, float now)
	{
		if (!d)
			return;
		d.m_iUses = d.m_iUses + 1;
		DCO_Perf.Count("designate:use_" + role + "_" + KindName(d.m_iKind));
		DCO_ELogLevel lvl = DCO_ELogLevel.DEBUG;
		if (d.m_iUses == 1)
			lvl = DCO_ELogLevel.INFO;
		if (DCO_Log.On(DCO_ELogChannel.COMBAT, lvl))
			DCO_Log.Write(DCO_ELogChannel.COMBAT, lvl, DCO_Log.IdUnit(user), string.Format("spec_designate_use unit=%1 role=%2 from=%3 kind=%4 age_ms=%5 uses=%6 dist=%7", DCO_Log.IdUnit(user), role, DCO_Log.IdUnit(d.m_From), KindName(d.m_iKind), Math.Round(now - d.m_fAt_ms), d.m_iUses, Math.Round(vector.DistanceXZ(user.GetOrigin(), d.m_vPos))));
	}

	protected static void Prune(float now)
	{
		array<AIGroup> drop = {};
		foreach (AIGroup g, array<ref DCO_Designation> l : GetDes())
		{
			if (!g || !l || l.IsEmpty() || now - l[l.Count() - 1].m_fAt_ms > 30000.0)
				drop.Insert(g);
		}
		foreach (AIGroup dg : drop)
		{
			GetDes().Remove(dg);
		}
	}

	static void Scan(SCR_AIUtilityComponent u, float now)
	{
		if (!u || !u.m_PerceptionComponent || !u.m_OwnerEntity)
			return;
		AIGroup grp = u.GetAIAgent().GetParentGroup();
		if (!grp)
			return;
		IEntity me = u.m_OwnerEntity;
		vector myPos = me.GetOrigin();
		array<BaseTarget> targets = {};
		u.m_PerceptionComponent.GetTargetsList(targets, ETargetCategory.ENEMY);
		array<vector> inf = {};
		array<IEntity> infEnt = {};
		bool armorDone;
		bool pointDone;
		int checks;
		foreach (BaseTarget t : targets)
		{
			if (!t || !t.GetTargetEntity() || t.GetTimeSinceSeen() > 3.0)
				continue;
			IEntity e = t.GetTargetEntity();
			vector p = t.GetLastSeenPosition();
			if (vector.DistanceXZ(myPos, p) > 600.0)
				continue;
			IEntity veh = DCO_VehicleCombat.TargetVehicle(e);
			if (veh && Vehicle.Cast(veh))
			{
				if (!armorDone && DCO_VehicleCombat.IsArmored(veh))
				{
					Post(grp, 1, veh.GetOrigin(), veh, me, now, "armor_seen");
					armorDone = true;
				}
				continue;
			}
			if (!ChimeraCharacter.Cast(e))
				continue;
			inf.Insert(p);
			infEnt.Insert(e);
			if (pointDone || checks >= 4)
				continue;
			checks++;
			if (DCO_Strength.HasWeaponInHands(e, EWeaponType.WT_MACHINEGUN))
			{
				Post(grp, 2, p, e, me, now, "enemy_mg");
				pointDone = true;
			}
			else if (SCR_CoverManagerComponent.IsEntityInsideBuilding(e))
			{
				Post(grp, 2, p, e, me, now, "in_building");
				pointDone = true;
			}
		}
		foreach (int i, vector a : inf)
		{
			vector sum = a;
			int n = 1;
			foreach (int j, vector b : inf)
			{
				if (i != j && vector.DistanceXZ(a, b) < 15.0)
				{
					sum = sum + b;
					n++;
				}
			}
			if (n >= 2)
			{
				Post(grp, 4, sum * (1.0 / n), infEnt[i], me, now, "cluster_" + n);
				break;
			}
		}
	}
}

class DCO_HeardEst
{
	vector m_vPos;
	float m_fRadius;
	float m_fConf;
	int m_iType;
	int m_iSrc;
	int m_iHop;
	float m_fAt_ms;
	vector m_vSharedPos;
	float m_fSharedAt_ms = -100000;

	protected static ref map<AIGroup, ref DCO_HeardEst> s_mHeard;
	protected static map<AIGroup, ref DCO_HeardEst> GetHeard()
	{
		if (!s_mHeard)
			s_mHeard = new map<AIGroup, ref DCO_HeardEst>();
		return s_mHeard;
	}

	static bool On()
	{
		return DCO_Ctx.TuneG("hear.est", 0.0) > 0;
	}

	static float BaseRadius(int type, float dist)
	{
		if (type == 1)
			return 40.0;
		if (type == 2 || type == 3)
			return 3.0;
		return 3.0 + 0.15 * dist;
	}

	static float BaseConf(int type)
	{
		if (type == 1)
			return 0.8;
		if (type == 0)
			return 0.6;
		return 0.7;
	}

	static void Report(AIGroup grp, vector pos, float radius, float conf, int type, IEntity listener, float dist, string mask, float now)
	{
		if (!grp || !On())
			return;
		if (GetHeard().Count() > 64)
			Prune(now);
		DCO_HeardEst e = GetHeard().Get(grp);
		if (!e)
		{
			e = new DCO_HeardEst();
			GetHeard().Set(grp, e);
		}
		e.m_vPos = pos;
		e.m_fRadius = radius;
		e.m_fConf = conf;
		e.m_iType = type;
		e.m_iSrc = 0;
		e.m_iHop = 0;
		e.m_fAt_ms = now;
		DCO_Perf.Count("heard_est:own_" + DCO_Hearing.TypeName(type));
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(grp), string.Format("heard_est grp=%1 listener=%2 type=%3 dist=%4 est=%5 radius=%6 conf=%7 mask=%8", DCO_Log.IdGroup(grp), DCO_Log.IdUnit(listener), DCO_Hearing.TypeName(type), Math.Round(dist), DCO_Log.Pos(pos), Math.Round(radius), conf.ToString(-1, 2), mask));
		MaybeShare(grp, e, now);
	}

	protected static void MaybeShare(AIGroup grp, DCO_HeardEst e, float now)
	{
		if (DCO_Ctx.TuneFor(grp, "is.est_share", 0.0) <= 0 || e.m_fConf < 0.4)
			return;
		if (now - e.m_fSharedAt_ms < 10000.0 && vector.DistanceXZ(e.m_vPos, e.m_vSharedPos) < 30.0)
			return;
		e.m_fSharedAt_ms = now;
		e.m_vSharedPos = e.m_vPos;
		DCO_ContactSharing.ShareEstimate(SCR_AIGroup.Cast(grp), e.m_vPos, e.m_fRadius, 1, e.m_iHop, true);
	}

	static void Receive(AIGroup grp, vector pos, float radius, int hop, AIGroup from, bool radio, float now)
	{
		if (!grp || !On())
			return;
		DCO_HeardEst e = GetHeard().Get(grp);
		string result = "new";
		if (e && now - e.m_fAt_ms < 20000.0 && e.m_iSrc == 0)
			result = "keep_own";
		else
		{
			if (!e)
			{
				e = new DCO_HeardEst();
				GetHeard().Set(grp, e);
			}
			e.m_vPos = pos;
			e.m_fRadius = Math.Clamp(radius, 3.0, 60.0);
			e.m_fConf = 0.6 * Math.Pow(0.7, hop);
			e.m_iType = 5;
			e.m_iSrc = 1;
			e.m_iHop = hop;
			e.m_fAt_ms = now;
			e.m_fSharedAt_ms = -100000;
		}
		DCO_Perf.Count("heard_est:recv_" + result);
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(grp), string.Format("heard_est_recv grp=%1 from=%2 hop=%3 radio=%4 est=%5 radius=%6 result=%7", DCO_Log.IdGroup(grp), DCO_Log.IdGroup(from), hop, DCO_Util.B01(radio), DCO_Log.Pos(pos), Math.Round(radius), result));
		if (result != "keep_own")
			MaybeShare(grp, e, now);
	}

	static bool Get(AIGroup grp, float now, float maxAge_ms, out vector pos, out float radius, out float conf, out int type)
	{
		if (!grp)
			return false;
		DCO_HeardEst e = GetHeard().Get(grp);
		if (!e || now - e.m_fAt_ms > maxAge_ms)
			return false;
		float age = (now - e.m_fAt_ms) / 1000.0;
		pos = e.m_vPos;
		radius = DCO_InfoQ.EstRadius(e.m_fRadius, age, e.m_iSrc, e.m_iHop);
		conf = e.m_fConf * Math.Clamp(1.0 - age / 30.0, 0.2, 1.0);
		type = e.m_iType;
		return true;
	}

	protected static void Prune(float now)
	{
		array<AIGroup> drop = {};
		foreach (AIGroup g, DCO_HeardEst e : GetHeard())
		{
			if (!g || !e || now - e.m_fAt_ms > 60000.0)
				drop.Insert(g);
		}
		foreach (AIGroup d : drop)
		{
			GetHeard().Remove(d);
		}
	}
}
