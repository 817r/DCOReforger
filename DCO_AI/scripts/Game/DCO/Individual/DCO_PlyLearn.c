class DCO_PlySpot
{
	vector m_vPos;
	string m_sFk;
	string m_sKind;
	int m_iN;
	float m_fErr;
	float m_fLastAt_ms;
	float m_fSuppNext_ms;
	bool m_bAnnounced;
	bool m_bPersist;
}

class DCO_PlyLearn
{
	protected static ref array<ref DCO_PlySpot> s_aSpots;
	protected static ref map<IEntity, float> s_mNext;
	protected static BaseWorld s_World;
	protected static float s_fLoadAt;

	static bool On()
	{
		return DCO_Ctx.TuneG("learn.ply_on", 0.0) > 0;
	}

	protected static float Now()
	{
		return GetGame().GetWorld().GetWorldTime();
	}

	protected static bool Ready()
	{
		if (!Replication.IsServer() || !GetGame().GetWorld())
			return false;
		if (s_aSpots && s_World == GetGame().GetWorld())
			return true;
		s_World = GetGame().GetWorld();
		s_aSpots = {};
		s_mNext = new map<IEntity, float>();
		s_fLoadAt = Now();
		LoadPersist();
		return true;
	}

	protected static void LoadPersist()
	{
		if (DCO_Ctx.TuneG("learn.ply_memory", 1.0) <= 0)
			return;
		DCO_LearnStore st = DCO_IndLearn.Store();
		if (!st)
			return;
		st.Scale("S|", 0.7);
		array<string> keys = {};
		st.Keys("S|", keys);
		int loaded;
		foreach (string k : keys)
		{
			vector r = st.Raw(k);
			if (r[0] < 0.65)
				continue;
			array<string> parts = {};
			k.Split("|", parts, false);
			if (parts.Count() < 5)
				continue;
			array<string> xz = {};
			parts[3].Split(",", xz, false);
			if (xz.Count() < 2)
				continue;
			DCO_PlySpot s = new DCO_PlySpot();
			s.m_sFk = parts[1];
			s.m_sKind = parts[2];
			s.m_vPos = Vector(xz[0].ToFloat(), parts[4].ToFloat(), xz[1].ToFloat());
			s.m_fErr = 8.0;
			s.m_fLastAt_ms = -100000000.0;
			s.m_bPersist = true;
			s.m_bAnnounced = true;
			s_aSpots.Insert(s);
			loaded++;
			if (loaded >= 30)
				break;
		}
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, "-", string.Format("ilearn_ply_load keys=%1 loaded=%2 decay=0.7", keys.Count(), loaded));
	}

	protected static string KindOf(vector pos)
	{
		if (DCO_Ctx.EnvAtPos(pos) == DCO_EEnv.INDOOR)
			return "bld";
		if (pos[1] - GetGame().GetWorld().GetSurfaceY(pos[0], pos[2]) > 3.0)
			return "high";
		return "gnd";
	}

	static void Note(IEntity observer, vector pos, float err, string src)
	{
		if (!observer || !On() || !Ready())
			return;
		string fk = DCO_Util.FactionKeyOf(observer, true);
		if (fk.IsEmpty())
			return;
		float now = Now();
		DCO_PlySpot best;
		float bestD = 1000000.0;
		foreach (DCO_PlySpot s : s_aSpots)
		{
			if (s.m_sFk != fk)
				continue;
			float d = vector.Distance(s.m_vPos, pos);
			if (d < bestD && d <= Math.Max(6.0, Math.Max(err, s.m_fErr)))
			{
				bestD = d;
				best = s;
			}
		}
		string what = "merge";
		if (!best)
		{
			best = new DCO_PlySpot();
			best.m_sFk = fk;
			best.m_vPos = pos;
			best.m_fErr = err;
			best.m_sKind = KindOf(pos);
			s_aSpots.Insert(best);
			if (s_aSpots.Count() > 40)
				s_aSpots.RemoveOrdered(0);
			what = "new";
		}
		else
		{
			float w = best.m_fErr / (best.m_fErr + err);
			best.m_vPos = best.m_vPos + (pos - best.m_vPos) * w;
			best.m_fErr = Math.Min(best.m_fErr, err);
		}
		if (now - best.m_fLastAt_ms > 3000.0)
			best.m_iN = best.m_iN + 1;
		best.m_fLastAt_ms = now;
		DCO_Perf.Count("ilearn:ply_note_" + src);
		if (best.m_iN == 2)
		{
			if (!best.m_bAnnounced)
				Announce(observer, best);
			best.m_bAnnounced = true;
			if (DCO_Ctx.TuneG("learn.ply_memory", 1.0) > 0)
			{
				DCO_LearnStore st = DCO_IndLearn.Store();
				if (st)
				{
					int y = Math.Round(best.m_vPos[1]);
					st.AddRaw("S|" + fk + "|" + best.m_sKind + "|" + DCO_LearnStore.PosKey(best.m_vPos, 15.0) + "|" + y.ToString(), 1.0, 0);
				}
			}
			what = "confirm";
		}
		if (what != "merge" && DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO))
			DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(observer), string.Format("ilearn_ply_spot what=%1 src=%2 by=%3 fk=%4 kind=%5 n=%6 err=%7 pos=%8", what, src, DCO_Log.IdUnit(observer), fk, best.m_sKind, best.m_iN, Math.Round(best.m_fErr), DCO_Util.XZ(best.m_vPos)));
	}

	protected static void Announce(IEntity observer, DCO_PlySpot s)
	{
		AIControlComponent ctrl = AIControlComponent.Cast(observer.FindComponent(AIControlComponent));
		if (!ctrl)
			return;
		SCR_ChimeraAIAgent ag = SCR_ChimeraAIAgent.Cast(ctrl.GetAIAgent());
		if (!ag || !ag.m_UtilityComponent || !ag.m_UtilityComponent.m_CommsHandler)
			return;
		if (!DCO_Opp.SquadVoiceOk(ag.GetParentGroup(), observer, 6000.0))
			return;
		ag.m_UtilityComponent.m_CommsHandler.AddRequest(new SCR_AITalkRequest(ECommunicationType.REPORT_CONTACT, null, s.m_vPos, 0, false, false, SCR_EAITalkRequestPreset.MEDIUM));
		DCO_Perf.Count("ilearn:ply_callout");
	}

	static void OnDamage(IEntity victim, IEntity attacker)
	{
		if (!victim || !attacker || !On() || !EntityUtils.IsPlayer(attacker) || EntityUtils.IsPlayer(victim))
			return;
		float d = vector.Distance(victim.GetOrigin(), attacker.GetOrigin());
		float err = Math.Clamp(0.1 * d, 3.0, 25.0);
		vector off = Vector(Math.RandomFloat(-err, err), 0, Math.RandomFloat(-err, err));
		Note(victim, attacker.GetOrigin() + off, err, "hit");
	}

	protected static DCO_PlySpot Pick(string fk, vector me, float now, out float bestD)
	{
		DCO_PlySpot best;
		float bestS;
		bestD = 0;
		foreach (DCO_PlySpot s : s_aSpots)
		{
			if (s.m_sFk != fk)
				continue;
			float d = vector.Distance(me, s.m_vPos);
			if (d < 20.0 || d > 350.0)
				continue;
			float sc;
			if (s.m_bPersist && s.m_iN == 0)
			{
				if (now - s_fLoadAt > 1200000.0)
					continue;
				sc = 0.5;
			}
			else
			{
				if (s.m_iN < 2 || now - s.m_fLastAt_ms > 600000.0)
					continue;
				sc = s.m_iN / (1.0 + (now - s.m_fLastAt_ms) / 60000.0);
			}
			if (sc > bestS)
			{
				bestS = sc;
				best = s;
				bestD = d;
			}
		}
		return best;
	}

	protected static void PruneNext()
	{
		array<IEntity> drop = {};
		foreach (IEntity e, float v : s_mNext)
		{
			if (!e || !DCO_Util.IsAlive(e))
				drop.Insert(e);
		}
		foreach (IEntity d : drop)
			s_mNext.Remove(d);
		DCO_Perf.CountN("ilearn:ply_next_pruned", drop.Count());
	}

	static void Tick(SCR_AIUtilityComponent u, float now)
	{
		if (!u || !u.m_OwnerEntity || !On() || !Ready())
			return;
		IEntity me = u.m_OwnerEntity;
		float next;
		if (s_mNext.Find(me, next) && now < next)
			return;
		s_mNext.Set(me, now + 2000.0);
		if (s_mNext.Count() > 400)
			PruneNext();
		if (!u.m_CombatComponent)
			return;
		BaseTarget t = u.m_CombatComponent.GetCurrentTarget();
		if (t && t.GetTimeSinceSeen() < 1.5)
		{
			IEntity te = t.GetTargetEntity();
			if (te && EntityUtils.IsPlayer(te) && now - DCO_ShotCounter.LastShotAt(te) < 3000.0)
				Note(me, t.GetLastSeenPosition(), 2.0, "seen");
			return;
		}
		if (s_aSpots.IsEmpty() || !u.m_ThreatSystem || u.m_ThreatSystem.GetState() == EAIThreatState.SAFE)
			return;
		if (DCO_Opp.Gate(u, false) != "")
			return;
		string fk = DCO_Util.FactionKeyOf(me, true);
		float d;
		DCO_PlySpot s = Pick(fk, me.GetOrigin(), now, d);
		if (!s)
			return;
		s_mNext.Set(me, now + 8000.0);
		string what = "watch";
		bool fresh = !s.m_bPersist || s.m_iN > 0;
		if (fresh && s.m_iN >= 3 && now - s.m_fLastAt_ms < 120000.0 && now >= s.m_fSuppNext_ms && DCO_Ctx.TuneG("learn.ply_suppress", 1.0) > 0 && (!t || t.GetTimeSinceSeen() > 5.0))
		{
			vector bbMin, bbMax;
			SCR_AISuppressionVolumeBase.CreateSuppressionBox(s.m_vPos, Math.Clamp(s.m_fErr, 2.0, 6.0), 3, bbMin, bbMax);
			SCR_AISuppressionObjectVolumeBox vol = new SCR_AISuppressionObjectVolumeBox(bbMin, bbMax);
			u.AddAction(new SCR_AISuppressBehavior(u, null, vol, 8.0, 1.5));
			s.m_fSuppNext_ms = now + 20000.0;
			what = "suppress";
		}
		else
			u.m_LookAction.LookAt(s.m_vPos + vector.Up * 1.2, 20.0, 4.0);
		DCO_Perf.Count("ilearn:ply_" + what);
		if (DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG))
			DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(me), string.Format("ilearn_ply_use unit=%1 what=%2 d=%3 n=%4 age_s=%5 persist=%6 kind=%7 pos=%8", DCO_Log.IdUnit(me), what, Math.Round(d), s.m_iN, Math.Round((now - s.m_fLastAt_ms) / 1000.0), DCO_Util.B01(s.m_bPersist), s.m_sKind, DCO_Util.XZ(s.m_vPos)));
	}
}
