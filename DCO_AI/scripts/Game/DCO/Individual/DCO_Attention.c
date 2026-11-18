modded class SCR_AIUtilityComponent
{
	ref DCO_Attention m_DCOAtt;
}

class DCO_Attention
{
	protected static bool s_bIssuing;

	float m_aThreat[8];
	float m_aLookAt_ms[8];
	vector m_aPos[8];
	float m_aPosAt_ms[8];
	string m_aWhy[8];
	float m_fNext_ms;
	float m_fPickUntil_ms;
	float m_fLogAt_ms;
	float m_fRearAt_ms;
	bool m_bRear;
	float m_fOwnAt_ms = -100000.0;
	bool m_bOwn;
	vector m_vOwn;
	int m_iOwnSector = -1;
	int m_iAcute = -1;
	int m_iCur = -1;
	ref array<IEntity> m_aTrkEnt;
	ref array<vector> m_aTrkPos;
	ref array<float> m_aTrkAt_ms;
	ref array<vector> m_aTrkVel;
	ref array<bool> m_aTrkLost;

	static bool On()
	{
		return DCO_Ctx.TuneG("att.on", 1.0) > 0;
	}

	static DCO_Attention Of(SCR_AIUtilityComponent u)
	{
		if (!u)
			return null;
		if (!u.m_DCOAtt)
			u.m_DCOAtt = new DCO_Attention();
		return u.m_DCOAtt;
	}

	static int SectorOf(vector from, vector to)
	{
		vector d = to - from;
		float deg = Math.Atan2(d[0], d[2]) * Math.RAD2DEG + 22.5;
		while (deg < 0)
			deg += 360.0;
		while (deg >= 360.0)
			deg -= 360.0;
		int idx = Math.Floor(deg / 45.0);
		return Math.ClampInt(idx, 0, 7);
	}

	static vector DirOf(int idx)
	{
		float a = idx * 45.0 * Math.DEG2RAD;
		return Vector(Math.Sin(a), 0, Math.Cos(a));
	}

	static void Feed(SCR_AIUtilityComponent u, vector pos, float weight, string why)
	{
		if (!On() || !u || !u.m_OwnerEntity)
			return;
		DCO_Attention a = Of(u);
		a.FeedAt(u.m_OwnerEntity.GetOrigin(), pos, weight, why);
	}

	void FeedAt(vector me, vector pos, float weight, string why)
	{
		int i = SectorOf(me, pos);
		float now = GetGame().GetWorld().GetWorldTime();
		if (weight > m_aThreat[i])
		{
			m_aThreat[i] = weight;
			m_aWhy[i] = why;
		}
		m_aPos[i] = pos;
		m_aPosAt_ms[i] = now;
		DCO_Perf.Count("att:feed_" + why);
		if (weight >= 0.95 && i != m_iCur)
			m_iAcute = i;
	}

	static bool Intercept(SCR_AIUtilityComponent u, vector pos, float priority)
	{
		if (s_bIssuing || !On() || !u || !u.m_OwnerEntity)
			return false;
		DCO_Attention a = Of(u);
		if (priority >= 50.0)
		{
			a.Fresh(SectorOf(u.m_OwnerEntity.GetOrigin(), pos));
			return false;
		}
		float w = 0.45 + Math.Clamp((priority - 20.0) / 30.0, 0, 1) * 0.45;
		a.FeedAt(u.m_OwnerEntity.GetOrigin(), pos, w, "look");
		DCO_Perf.Count("att:swallow");
		return true;
	}

	void Fresh(int i)
	{
		m_aLookAt_ms[i] = GetGame().GetWorld().GetWorldTime();
	}

	void FreshArc(int i)
	{
		Fresh(i);
		Fresh((i + 1) % 8);
		Fresh((i + 7) % 8);
	}

	static void Tick(SCR_AIUtilityComponent u, float now)
	{
		if (!On() || !u || !u.m_OwnerEntity || !u.m_LookAction)
			return;
		ChimeraCharacter me = ChimeraCharacter.Cast(u.m_OwnerEntity);
		if (!me || me.IsInVehicle() || !DCO_Util.IsAlive(me))
			return;
		DCO_Attention a = Of(u);
		if (now < a.m_fNext_ms && a.m_iAcute < 0)
			return;
		float dt = 0.25;
		if (a.m_fNext_ms > 0)
			dt = Math.Clamp((now - a.m_fNext_ms + 250.0) * 0.001, 0.05, 2.0);
		a.m_fNext_ms = now + 250.0;
		a.Update(u, me, now, dt);
	}

	void Update(SCR_AIUtilityComponent u, ChimeraCharacter me, float now, float dt)
	{
		vector mp = me.GetOrigin();
		float decay = dt * 0.08;
		for (int i = 0; i < 8; i++)
		{
			if (m_aThreat[i] > 0)
				m_aThreat[i] = Math.Max(m_aThreat[i] - decay, 0);
		}
		vector aim = me.GetTransformAxis(2);
		CharacterControllerComponent cc = me.GetCharacterController();
		if (cc && cc.GetAimingComponent())
			aim = cc.GetAimingComponent().GetAimingDirectionWorld();
		aim[1] = 0;
		if (aim.LengthSq() > 0.01)
			FreshArc(SectorOf(mp, mp + aim * 10.0));
		SensePresence(u, mp, now);
		Predict(u, me, mp, now);
		BaseTarget cur;
		if (u.m_CombatComponent)
			cur = u.m_CombatComponent.GetCurrentTarget();
		if (cur && cur.GetTimeSinceSeen() < 1.0)
		{
			m_iAcute = -1;
			DCO_Perf.Count("att:busy_target");
			return;
		}
		if (m_iAcute < 0 && now < m_fPickUntil_ms)
			return;
		Pick(u, me, mp, now);
	}

	bool RearGuard(SCR_AIUtilityComponent u, float now)
	{
		if (now - m_fRearAt_ms < 5000.0 && now >= m_fRearAt_ms)
			return m_bRear;
		m_fRearAt_ms = now;
		m_bRear = false;
		AIAgent ag = u.GetAIAgent();
		if (!ag || !ag.GetParentGroup() || ag.GetParentGroup().GetAgentsCount() < 2)
			return false;
		m_bRear = DCO_FrontHold.IsRearGuard(u);
		return m_bRear;
	}

	bool OwnSector(SCR_AIUtilityComponent u, ChimeraCharacter me, vector mp, float now)
	{
		if (now >= m_fOwnAt_ms && now - m_fOwnAt_ms < 3000.0)
			return m_bOwn;
		m_fOwnAt_ms = now;
		bool was = m_bOwn;
		int wasSector = m_iOwnSector;
		m_bOwn = false;
		m_iOwnSector = -1;
		float distM = -1;
		string src = "off";
		if (DCO_Ctx.TuneForU(u, "att.formation", 1.0) > 0)
		{
			src = "alone";
			AIAgent ag = u.GetAIAgent();
			AIGroup grp;
			if (ag)
				grp = ag.GetParentGroup();
			if (grp && grp.GetAgentsCount() >= 2)
			{
				vector d = mp - grp.GetCenterOfMass();
				d[1] = 0;
				distM = d.Length();
				src = "centre";
				if (distM >= 2.0)
				{
					m_vOwn = d * (1.0 / distM);
					m_bOwn = true;
					m_iOwnSector = SectorOf(mp, mp + m_vOwn * 10.0);
					src = "squad";
				}
			}
		}
		DCO_Perf.Count("att:own_" + src);
		if ((was != m_bOwn || wasSector != m_iOwnSector) && DCO_Log.On(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO))
			DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("attention_own unit=%1 sector=%2 src=%3 from_centre_m=%4 was=%5", DCO_Log.IdUnit(me), m_iOwnSector, src, Math.Round(distM), wasSector));
		return m_bOwn;
	}

	void Pick(SCR_AIUtilityComponent u, ChimeraCharacter me, vector mp, float now)
	{
		float scanMs = Math.Max(DCO_Ctx.TuneForU(u, "att.scan_s", 15.0), 1.0) * 1000.0;
		float bias = DCO_Ctx.TuneForU(u, "att.front_bias", 0.3);
		bool contact = u.m_ThreatSystem && u.m_ThreatSystem.GetState() == EAIThreatState.THREATENED;
		float staleW = 0.5;
		if (contact)
			staleW = 0.25;
		vector front;
		bool hasFront = DCO_FrontHold.Front(u, front);
		if (!hasFront)
		{
			front = me.GetTransformAxis(2);
			front[1] = 0;
			hasFront = front.LengthSq() > 0.01;
		}
		bool ownOn = OwnSector(u, me, mp, now);
		bool rearMan = ownOn && hasFront && vector.Dot(m_vOwn, front) <= -0.5;
		bool ownScan = !contact || rearMan;
		if (!ownOn && hasFront && RearGuard(u, now))
			front = front * -1.0;
		float arcCos = Math.Cos(DCO_Ctx.TuneForU(u, "front.arc_deg", 60.0) * Math.DEG2RAD);
		int best = -1;
		float bestScore = -1;
		float bestStale = 0;
		string why = "stale";
		if (m_iAcute >= 0)
		{
			best = m_iAcute;
			why = m_aWhy[best];
		}
		else
		{
			for (int i = 0; i < 8; i++)
			{
				float stale = Math.Clamp((now - m_aLookAt_ms[i]) / scanMs, 0, 1);
				float s = m_aThreat[i];
				if (ownOn)
				{
					if (ownScan && vector.Dot(DirOf(i), m_vOwn) >= 0.3827)
						s += stale * staleW + bias;
					else
						stale = 0;
				}
				else
				{
					s += stale * staleW;
					if (hasFront && vector.Dot(DirOf(i), front) >= arcCos)
						s += bias;
				}
				if (s > bestScore)
				{
					bestScore = s;
					best = i;
					bestStale = stale;
				}
			}
			if (best < 0)
				return;
			if (m_aThreat[best] < 0.3 && bestStale < 1.0)
			{
				DCO_Perf.Count("att:hold");
				return;
			}
			if (m_aThreat[best] >= 0.3)
				why = m_aWhy[best];
		}
		m_iAcute = -1;
		vector pos = mp + DirOf(best) * 25.0 + Vector(0, 1.5, 0);
		if (m_aThreat[best] > 0 && now - m_aPosAt_ms[best] < 10000.0)
			pos = m_aPos[best];
		float thr = Math.Clamp(m_aThreat[best], 0, 1);
		float prio = 22.0 + thr * 26.0;
		float dur = 1.5 + Math.RandomFloat01();
		s_bIssuing = true;
		u.m_LookAction.LookAt(pos, prio, dur);
		s_bIssuing = false;
		FreshArc(best);
		m_fPickUntil_ms = now + dur * 1000.0;
		DCO_Perf.Count("att:pick_" + why);
		bool changed = best != m_iCur;
		m_iCur = best;
		if (!changed || now < m_fLogAt_ms || !DCO_Log.On(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO))
			return;
		m_fLogAt_ms = now + 2000.0;
		DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("attention_pick unit=%1 sector=%2 why=%3 threat=%4 stale_s=%5 prio=%6 pos=%7 own=%8 rear=%9", DCO_Log.IdUnit(me), best, why, thr.ToString(-1, 2), Math.Round(bestStale * scanMs * 0.001), Math.Round(prio), DCO_Log.Pos(pos), m_iOwnSector, DCO_Util.B01(rearMan)));
	}

	void SensePresence(SCR_AIUtilityComponent u, vector mp, float now)
	{
		float nearM = DCO_Ctx.TuneForU(u, "att.near_m", 15.0);
		if (nearM <= 0 || !u.m_PerceptionComponent)
			return;
		array<BaseTarget> det = {};
		u.m_PerceptionComponent.GetTargetsList(det, ETargetCategory.DETECTED);
		array<BaseTarget> ene = {};
		u.m_PerceptionComponent.GetTargetsList(ene, ETargetCategory.ENEMY);
		foreach (BaseTarget e : ene)
			det.Insert(e);
		foreach (BaseTarget t : det)
		{
			if (!t || t.GetDistance() > nearM || t.GetTimeSinceSeen() < 0.5)
				continue;
			if (t.GetTimeSinceDetected() > 3.0)
				continue;
			FeedAt(mp, t.GetLastDetectedPosition() + Vector(0, 1.5, 0), 1.0, "near");
			return;
		}
	}

	void Predict(SCR_AIUtilityComponent u, ChimeraCharacter me, vector mp, float now)
	{
		float predS = DCO_Ctx.TuneForU(u, "att.pred_s", 8.0);
		if (predS <= 0 || !u.m_PerceptionComponent)
			return;
		if (!m_aTrkEnt)
		{
			m_aTrkEnt = {};
			m_aTrkPos = {};
			m_aTrkAt_ms = {};
			m_aTrkVel = {};
			m_aTrkLost = {};
		}
		array<BaseTarget> ts = {};
		u.m_PerceptionComponent.GetTargetsList(ts, ETargetCategory.ENEMY);
		foreach (BaseTarget t : ts)
		{
			IEntity e = t.GetTargetEntity();
			if (!e)
				continue;
			float since = t.GetTimeSinceSeen();
			int k = m_aTrkEnt.Find(e);
			if (since < 0.6)
			{
				vector p = t.GetLastSeenPosition();
				if (k < 0)
				{
					if (m_aTrkEnt.Count() >= 4)
						TrkRemove(0);
					m_aTrkEnt.Insert(e);
					m_aTrkPos.Insert(p);
					m_aTrkAt_ms.Insert(now);
					m_aTrkVel.Insert(vector.Zero);
					m_aTrkLost.Insert(false);
					continue;
				}
				float dtv = (now - m_aTrkAt_ms[k]) * 0.001;
				if (dtv >= 0.4)
				{
					vector v = (p - m_aTrkPos[k]) * (1.0 / dtv);
					float vl = v.Length();
					if (vl > 8.0)
						v = v * (8.0 / vl);
					m_aTrkVel[k] = v;
					m_aTrkPos[k] = p;
					m_aTrkAt_ms[k] = now;
				}
				m_aTrkLost[k] = false;
				continue;
			}
			if (k < 0 || since < 1.0 || since > predS)
				continue;
			vector vel = m_aTrkVel[k];
			if (vel.LengthSq() < 0.25)
				continue;
			vector pred = DCO_Util.Surface(t.GetLastSeenPosition() + vel * since) + Vector(0, 1.5, 0);
			string why = "predict";
			float w = 0.8;
			if (!m_aTrkLost[k])
			{
				m_aTrkLost[k] = true;
				w = 0.95;
				vector look;
				float off;
				bool corner = DCO_Search.Corner(me, mp, pred, look, off);
				if (corner)
				{
					pred = look;
					why = "predict_corner";
				}
				DCO_Perf.Count("att:predict_start");
				DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("attention_predict unit=%1 tgt=%2 since_s=%3 vel_ms=%4 pos=%5 corner=%6", DCO_Log.IdUnit(me), DCO_Log.IdUnit(e), since.ToString(-1, 1), vel.Length().ToString(-1, 1), DCO_Log.Pos(pred), DCO_Util.B01(corner)));
			}
			FeedAt(mp, pred, w, why);
		}
		for (int j = m_aTrkEnt.Count() - 1; j >= 0; j--)
		{
			if (!m_aTrkEnt[j] || now - m_aTrkAt_ms[j] > 60000.0)
				TrkRemove(j);
		}
	}

	protected void TrkRemove(int j)
	{
		m_aTrkEnt.Remove(j);
		m_aTrkPos.Remove(j);
		m_aTrkAt_ms.Remove(j);
		m_aTrkVel.Remove(j);
		m_aTrkLost.Remove(j);
	}
}
