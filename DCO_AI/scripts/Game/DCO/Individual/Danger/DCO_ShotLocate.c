class DCO_ShotEstState
{
	vector m_vSpot;
	int m_iShots;
	float m_fAt;
	float m_fAng;
	float m_fRng;
}

class DCO_ShotLocate
{
	protected static ref map<string, ref DCO_ShotEstState> s_mEst;

	protected static ref map<AIGroup, float> s_mLastErr;
	protected static ref map<AIGroup, float> s_mLastErrAt;

	static float LastErr(AIGroup grp, float now_ms)
	{
		if (!grp || !s_mLastErr || !s_mLastErrAt)
			return 0;
		float at;
		float err;
		if (!s_mLastErrAt.Find(grp, at) || now_ms - at > 30000.0 || !s_mLastErr.Find(grp, err))
			return 0;
		return err;
	}

	protected static void NoteErr(AIGroup grp, float err, float now_ms)
	{
		if (!s_mLastErr)
			s_mLastErr = new map<AIGroup, float>();
		if (!s_mLastErrAt)
			s_mLastErrAt = new map<AIGroup, float>();
		if (s_mLastErr.Count() > 300)
		{
			s_mLastErr.Clear();
			s_mLastErrAt.Clear();
		}
		s_mLastErr.Set(grp, err);
		s_mLastErrAt.Set(grp, now_ms);
	}

	protected static map<string, ref DCO_ShotEstState> GetEst()
	{
		if (!s_mEst)
			s_mEst = new map<string, ref DCO_ShotEstState>();
		return s_mEst;
	}

	static vector Estimate(SCR_AIUtilityComponent u, AIGroup grp, IEntity shooter, vector shotPos, float dist, float audibility, bool suppressed)
	{
		if (!u || !u.m_OwnerEntity || !grp)
			return shotPos;
		float k = DCO_Stress.T(u, "hear.shot_err", 1.0);
		if (k <= 0)
		{
			DCO_Perf.Count("hear:shot_exact_off");
			return shotPos;
		}
		string why;
		if (dist < 50.0)
			why = "close";
		else if (Seen(u, shooter))
			why = "seen";
		else if (!suppressed && dist <= 800.0 && DCO_Util.IsNight() && FlashVisible(u, shooter, shotPos))
		{
			if (dist <= DCO_Stress.T(u, "hear.flash_exact_m", 300.0))
				why = "flash";
			else
				DCO_Perf.Count("hear:shot_flash_far");
		}
		if (!why.IsEmpty())
		{
			DCO_Perf.Count("hear:shot_exact_" + why);
			return shotPos;
		}

		float now = GetGame().GetWorld().GetWorldTime();
		string key = string.Format("%1|%2", grp, shooter);
		DCO_ShotEstState st = GetEst().Get(key);
		bool fresh = false;
		if (!st || now - st.m_fAt > 30000.0 || vector.DistanceXZ(st.m_vSpot, shotPos) > 20.0)
		{
			if (GetEst().Count() > 256)
				Prune(now);
			st = new DCO_ShotEstState();
			st.m_vSpot = shotPos;
			st.m_fAng = Math.RandomFloat(-1.0, 1.0);
			st.m_fRng = Math.RandomFloat(-1.0, 1.0);
			GetEst().Set(key, st);
			fresh = true;
		}
		st.m_iShots++;
		st.m_fAt = now;

		float aud = Math.Clamp(audibility, 0.0, 1.0);
		float shrink = Math.Max(Math.Pow(0.85, st.m_iShots - 1), 0.3);
		float angDeg = (5.0 + (1.0 - aud) * 10.0) * k * shrink * st.m_fAng;
		float rngFrac = (0.15 + (1.0 - aud) * 0.35) * k * shrink * st.m_fRng;

		vector me = u.m_OwnerEntity.GetOrigin();
		vector off = shotPos - me;
		off[1] = 0;
		if (off.Length() < 1.0)
			return shotPos;
		float a = angDeg * Math.DEG2RAD;
		float c = Math.Cos(a);
		float s = Math.Sin(a);
		float scale = Math.Max(1.0 + rngFrac, 0.3);
		vector rot = Vector((off[0] * c - off[2] * s) * scale, 0, (off[0] * s + off[2] * c) * scale);
		vector est = DCO_Util.Surface(me + rot);
		string snap = "none";
		float snapM = DCO_Stress.T(u, "hear.snap_bld_m", 25.0);
		if (snapM > 0)
		{
			string bm;
			if (DCO_CoverManagerComponent.DCO_GetBuildingAtPos(est, null, bm))
				snap = "inside";
			else
			{
				DCO_GarrisonRegistry reg = DCO_GarrisonRegistry.GetInstance();
				array<IEntity> blds = {};
				if (reg)
					reg.FindBuildings(est, snapM, blds);
				IEntity nb;
				float nd = float.MAX;
				foreach (IEntity b : blds)
				{
					float bd = vector.DistanceXZ(b.GetOrigin(), est);
					if (b && bd < nd)
					{
						nd = bd;
						nb = b;
					}
				}
				if (nb)
				{
					est = DCO_Util.Surface(nb.GetOrigin());
					snap = DCO_Util.PrefabShort(nb);
					DCO_Perf.Count("hear:shot_snap_bld");
				}
			}
		}
		NoteErr(grp, vector.DistanceXZ(est, shotPos), now);
		DCO_Perf.Count("hear:shot_est");
		if (fresh)
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(grp), string.Format("gunshot_est grp=%1 shooter=%2 dist=%3 err_m=%4 ang_deg=%5 range_pct=%6 aud=%7 k=%8 snap=%9", DCO_Log.IdGroup(grp), DCO_Log.IdUnit(shooter), Math.Round(dist), Math.Round(vector.DistanceXZ(est, shotPos)), angDeg.ToString(-1, 1), Math.Round(rngFrac * 100.0), aud.ToString(-1, 2), k.ToString(-1, 2), snap));
		return est;
	}

	protected static bool Seen(SCR_AIUtilityComponent u, IEntity shooter)
	{
		if (!shooter)
			return false;
		PerceptionComponent pc = PerceptionComponent.Cast(u.m_OwnerEntity.FindComponent(PerceptionComponent));
		if (!pc)
			return false;
		BaseTarget t = pc.GetTargetPerceptionObject(shooter, ETargetCategory.ENEMY);
		return t && t.GetTimeSinceSeen() < 2.0;
	}

	protected static bool FlashVisible(SCR_AIUtilityComponent u, IEntity shooter, vector shotPos)
	{
		vector eye = u.m_OwnerEntity.GetOrigin() + Vector(0, 1.6, 0);
		return DCO_Util.TraceClear(eye, shotPos, u.m_OwnerEntity, 0.97, TraceFlags.WORLD | TraceFlags.ENTS, -1, shooter, "t:DCO_ShotFlash");
	}

	protected static void Prune(float now)
	{
		array<string> old = {};
		foreach (string k, DCO_ShotEstState st : GetEst())
		{
			if (!st || now - st.m_fAt > 30000.0)
				old.Insert(k);
		}
		foreach (string k2 : old)
			GetEst().Remove(k2);
		if (GetEst().Count() > 256)
			GetEst().Clear();
	}
}
