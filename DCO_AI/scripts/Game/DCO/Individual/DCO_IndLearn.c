class DCO_IndLearnEp
{
	string m_sKey;
	float m_fKeyAt_ms = -100000.0;
	float m_fExpS;
	float m_fNext_ms;
	float m_fLastTick_ms = -1;
	string m_sVs = "a";
	string m_sFk;
	float m_fVsAt_ms = -100000.0;
	string m_sGood;
	vector m_vGoodPos;
	float m_fGoodStart_ms;
	int m_iHits;
	bool m_bWounded;
	float m_fWoundAt_ms = -100000.0;
	float m_fEnemyDmgAt_ms = -100000.0;
	vector m_vAtkPos;
	float m_fLogAt_ms;
}

class DCO_IndLearn
{
	protected static ref DCO_IndLearn s_Inst;

	protected BaseWorld m_World;
	protected ref DCO_LearnStore m_Store;
	protected ref map<IEntity, ref DCO_IndLearnEp> m_mEp = new map<IEntity, ref DCO_IndLearnEp>();
	protected bool m_bTest;
	protected float m_fNextSave_ms;
	protected float m_fBaseAt_ms = -100000.0;
	protected float m_fBaseP = 0.05;
	protected float m_fBaseA = 0.05;
	protected bool m_bKillHook;
	protected ref map<IEntity, float> m_mDeadAt = new map<IEntity, float>();
	protected ref DCO_EngageInfo m_Enemy = new DCO_EngageInfo();

	static bool On()
	{
		return DCO_Ctx.TuneG("learn.ind_on", 1.0) > 0;
	}

	protected static float Now()
	{
		return GetGame().GetWorld().GetWorldTime();
	}

	protected static float Cap()
	{
		return Math.Clamp(DCO_Ctx.TuneG("learn.ind_cap", 0.25), 0.0, 0.5);
	}

	static bool Explore()
	{
		return Math.RandomFloat01() < Math.Clamp(DCO_Ctx.TuneG("learn.ind_explore", 0.12), 0.0, 0.5);
	}

	static DCO_IndLearn Get()
	{
		if (!Replication.IsServer() || !GetGame().GetWorld())
			return null;
		if (s_Inst && s_Inst.m_World != GetGame().GetWorld())
		{
			SaveAll();
			s_Inst = null;
		}
		if (!s_Inst)
		{
			s_Inst = new DCO_IndLearn();
			s_Inst.m_World = GetGame().GetWorld();
			s_Inst.m_bTest = DCO_TestCaseComponent.Active();
			string file = "$profile:DCO/IndLearn/" + DCO_LearnStore.WorldName() + ".json";
			if (s_Inst.m_bTest)
				file = "$profile:DCO/IndLearn/_test/" + DCO_LearnStore.WorldName() + ".json";
			bool frozen = DCO_Ctx.TuneG("learn.ind_freeze", 0.0) > 0;
			if (frozen)
				file = "$profile:DCO/IndLearn/_frozen/" + DCO_LearnStore.WorldName() + ".json";
			s_Inst.m_Store = new DCO_LearnStore(file);
			s_Inst.m_Store.SetReadOnly(frozen);
			s_Inst.m_fNextSave_ms = Now() + 300000.0;
			s_Inst.HookKills();
			s_Inst.Summary("start");
		}
		return s_Inst;
	}

	static DCO_LearnStore Store()
	{
		DCO_IndLearn L = Get();
		if (!L)
			return null;
		return L.m_Store;
	}

	static void SaveAll()
	{
		if (!s_Inst || !s_Inst.m_Store.IsDirty())
			return;
		s_Inst.m_Store.Save("end");
	}

	static int ResetAll()
	{
		DCO_IndLearn L = Get();
		if (!L)
			return 0;
		L.m_mEp.Clear();
		L.m_fBaseAt_ms = -100000.0;
		return L.m_Store.Clear();
	}

	protected void Summary(string why)
	{
		vector tp = m_Store.Totals("D|p|");
		vector ta = m_Store.Totals("D|a|");
		vector tg = m_Store.Totals("G|");
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, "-", string.Format("ilearn_summary why=%1 test=%2 frozen=" + DCO_Util.B01(m_Store.IsReadOnly()) + " cells_p=%3 exp_p_min=%4 ev_p=%5 cells_a=%6 exp_a_min=%7 ev_a=%8 good_keys=%9", why, DCO_Util.B01(m_bTest), tp[2], tp[0].ToString(-1, 1), tp[1].ToString(-1, 1), ta[2], ta[0].ToString(-1, 1), ta[1].ToString(-1, 1), tg[2]));
	}

	protected float Base(string vs, float now)
	{
		if (now >= m_fBaseAt_ms)
		{
			m_fBaseAt_ms = now + 60000.0;
			vector tp = m_Store.Totals("D|p|");
			vector ta = m_Store.Totals("D|a|");
			m_fBaseP = (tp[1] + 0.5) / (tp[0] + 10.0);
			m_fBaseA = (ta[1] + 0.5) / (ta[0] + 10.0);
		}
		if (vs == "p")
			return m_fBaseP;
		return m_fBaseA;
	}

	static string KeyOf(vector pos, vector threat, string vs, string fk)
	{
		vector d = threat - pos;
		d[1] = 0;
		if (d.LengthSq() < 1.0)
			return "";
		int s = Math.Round(Math.Repeat(d.ToYaw(), 360.0) / 45.0);
		s = s % 8;
		return vs + "|" + fk + "|" + s.ToString() + "|" + DCO_LearnStore.PosKey(pos, 15.0);
	}

	protected void RefreshVs(IEntity me, DCO_IndLearnEp ep, float now)
	{
		if (now - ep.m_fVsAt_ms < 10000.0 && !ep.m_sFk.IsEmpty())
			return;
		ep.m_fVsAt_ms = now;
		ep.m_sVs = "a";
		if (DCO_CmdLearn.VsAt(me.GetOrigin(), 400.0) == "player")
			ep.m_sVs = "p";
		ep.m_sFk = DCO_Util.FactionKeyOf(me, true);
		if (ep.m_sFk.IsEmpty())
			ep.m_sFk = "-";
	}

	protected static bool Hostile(IEntity a, IEntity b)
	{
		FactionAffiliationComponent fa = FactionAffiliationComponent.Cast(a.FindComponent(FactionAffiliationComponent));
		FactionAffiliationComponent fb = FactionAffiliationComponent.Cast(b.FindComponent(FactionAffiliationComponent));
		if (!fa || !fb)
			return false;
		Faction x = fa.GetAffiliatedFaction();
		Faction y = fb.GetAffiliatedFaction();
		return x && y && x.IsFactionEnemy(y);
	}

	protected DCO_IndLearnEp EpOf(IEntity e)
	{
		DCO_IndLearnEp ep;
		if (!m_mEp.Find(e, ep))
		{
			ep = new DCO_IndLearnEp();
			m_mEp.Set(e, ep);
		}
		return ep;
	}

	protected float MultOfKey(string key, out string info)
	{
		float cap = Cap();
		vector d = m_Store.Raw("D|" + key);
		float r0 = Base(key.Substring(0, 1), Now());
		float rate = (d[1] + r0 * 4.0) / (d[0] + 4.0);
		float ratio = rate / r0;
		float dz = Math.Clamp(Math.Log2(ratio) * 0.5, -0.4, 1.0);
		if (dz > 0)
			dz = dz * Math.Clamp(d[1] - 1.0, 0.0, 1.0);
		float md = 1.0 - cap * dz;
		vector g = m_Store.Raw("G|" + key);
		float post = (g[1] + 2.0) / (g[0] + 4.0);
		float mg = 1.0 + cap * Math.Clamp((post - 0.5) * 2.0, -1.0, 1.0);
		float m = Math.Clamp(md * mg, 1.0 - cap, 1.0 + cap);
		info = string.Format("key=%1 exp_min=%2 ev=%3 ratio=%4 good_n=%5 good=%6 mult=%7", key, d[0].ToString(-1, 1), d[1].ToString(-1, 1), ratio.ToString(-1, 2), g[0], post.ToString(-1, 2), m.ToString(-1, 2));
		return m;
	}

	static float Mult(SCR_AIUtilityComponent u, vector pos, out string info)
	{
		info = "";
		if (!u || !u.m_OwnerEntity || !On())
			return 1.0;
		DCO_IndLearn L = Get();
		if (!L)
			return 1.0;
		DCO_EngageInfo ei = DCO_Engage.Info(u);
		if (!ei || ei.m_iKnown <= 0 || ei.m_vNearest == vector.Zero)
			return 1.0;
		DCO_IndLearnEp ep = L.EpOf(u.m_OwnerEntity);
		L.RefreshVs(u.m_OwnerEntity, ep, Now());
		string key = KeyOf(pos, ei.m_vNearest, ep.m_sVs, ep.m_sFk);
		if (key.IsEmpty())
			return 1.0;
		return L.MultOfKey(key, info);
	}

	static bool PickOn()
	{
		return On() && DCO_Ctx.TuneG("learn.ind_pick", 0.0) > 0;
	}

	static void NotePick(SCR_AIUtilityComponent u, int n, int chosen, float firstM, float bestM, string info)
	{
		DCO_Perf.Count("ilearn:cover_pick");
		if (chosen <= 0)
			return;
		DCO_Perf.Count("ilearn:cover_pick_better");
		LogUse(u, "cover_pick", "cands=" + n.ToString() + " chosen=" + chosen.ToString() + " first_m=" + firstM.ToString(-1, 2) + " best_m=" + bestM.ToString(-1, 2) + " " + info);
	}

	static bool CoverOk(SCR_AIUtilityComponent u, vector pos)
	{
		if (!u || !On())
			return true;
		string info;
		float m = Mult(u, pos, info);
		if (m >= 1.0 - 0.6 * Cap())
			return true;
		if (Explore())
		{
			DCO_Perf.Count("ilearn:explore");
			LogUse(u, "explore", info);
			return true;
		}
		DCO_Perf.Count("ilearn:cover_skip");
		LogUse(u, "cover_skip", info);
		return false;
	}

	static void LogUse(SCR_AIUtilityComponent u, string what, string info)
	{
		DCO_IndLearn L = Get();
		if (!L || !u.m_OwnerEntity)
			return;
		DCO_IndLearnEp ep = L.EpOf(u.m_OwnerEntity);
		float now = Now();
		if (now < ep.m_fLogAt_ms || !DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO))
			return;
		ep.m_fLogAt_ms = now + 5000.0;
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(u.m_OwnerEntity), string.Format("ilearn_use unit=%1 what=%2 %3", DCO_Log.IdUnit(u.m_OwnerEntity), what, info));
	}

	protected void Flush(DCO_IndLearnEp ep)
	{
		if (!ep.m_sKey.IsEmpty() && ep.m_fExpS > 0)
		{
			m_Store.AddRaw("D|" + ep.m_sKey, ep.m_fExpS / 60.0, 0);
			DCO_Perf.Count("ilearn:exp_flush");
		}
		ep.m_fExpS = 0;
	}

	protected void EndGood(IEntity me, DCO_IndLearnEp ep, string why, float now)
	{
		if (ep.m_sGood.IsEmpty())
			return;
		float dur = (now - ep.m_fGoodStart_ms) / 1000.0;
		bool fired = DCO_ShotCounter.LastShotAt(me) >= ep.m_fGoodStart_ms;
		float v = -1.0;
		if (why == "killed")
			v = 0;
		else if (ep.m_bWounded)
		{
			v = 0.1;
			if (ep.m_iHits > 0)
				v = 0.3;
		}
		else if (dur >= 5.0 && ep.m_iHits > 0)
			v = 1.0;
		else if (dur >= 5.0 && fired)
			v = 0.6;
		if (v >= 0)
		{
			m_Store.Add("G|" + ep.m_sGood, v);
			DCO_Perf.Count("ilearn:good_add");
			if (DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG))
				DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(me), string.Format("ilearn_good unit=%1 key=%2 why=%3 dur_s=%4 hits=%5 fired=%6 wounded=%7 value=%8", DCO_Log.IdUnit(me), ep.m_sGood, why, Math.Round(dur), ep.m_iHits, DCO_Util.B01(fired), DCO_Util.B01(ep.m_bWounded), v.ToString(-1, 1)));
		}
		else
			DCO_Perf.Count("ilearn:good_skip");
		ep.m_sGood = "";
		ep.m_iHits = 0;
		ep.m_bWounded = false;
	}

	protected void Prune()
	{
		array<IEntity> drop = {};
		foreach (IEntity e, DCO_IndLearnEp ep : m_mEp)
		{
			if (!e || !DCO_Util.IsAlive(e))
				drop.Insert(e);
		}
		foreach (IEntity d : drop)
			m_mEp.Remove(d);
		if (!drop.IsEmpty())
			DCO_Perf.CountN("ilearn:ep_pruned", drop.Count());
		vector td = m_Store.Totals("D|");
		if (td[2] > 15000)
		{
			int cut = m_Store.PruneSmall("D|", 0.5);
			DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, "-", string.Format("ilearn_prune keys=%1 removed=%2 min_n=0.5", td[2], cut));
		}
	}

	protected string EventKey(IEntity victim, DCO_IndLearnEp ep, float now)
	{
		if (!ep.m_sKey.IsEmpty() && now - ep.m_fKeyAt_ms <= 4000.0)
			return ep.m_sKey;
		RefreshVs(victim, ep, now);
		return KeyOf(victim.GetOrigin(), ep.m_vAtkPos, ep.m_sVs, ep.m_sFk);
	}

	static void Tick(SCR_AIUtilityComponent u, float now)
	{
		if (!u || !u.m_OwnerEntity || !On())
			return;
		DCO_IndLearn L = Get();
		if (!L)
			return;
		L.HookKills();
		IEntity me = u.m_OwnerEntity;
		DCO_IndLearnEp ep = L.EpOf(me);
		if (now < ep.m_fNext_ms)
			return;
		ep.m_fNext_ms = now + 2000.0;
		float dt = 2.0;
		if (ep.m_fLastTick_ms >= 0)
			dt = Math.Clamp((now - ep.m_fLastTick_ms) / 1000.0, 0.0, 5.0);
		ep.m_fLastTick_ms = now;
		if (now >= L.m_fNextSave_ms)
		{
			L.m_fNextSave_ms = now + 300000.0;
			L.Prune();
			if (L.m_Store.IsDirty())
				L.m_Store.Save("periodic");
			L.Summary("periodic");
		}
		ChimeraCharacter ch = ChimeraCharacter.Cast(me);
		DCO_EngageInfo ei = L.m_Enemy;
		DCO_Engage.ComputeEnemies(u, ei);
		DCO_Perf.Count("ilearn:enemies_only");
		vector pos = me.GetOrigin();
		if (!ch || ch.IsInVehicle() || !ei || ei.m_iKnown <= 0 || ei.m_vNearest == vector.Zero || vector.Distance(pos, ei.m_vNearest) > 400.0)
		{
			L.Flush(ep);
			ep.m_sKey = "";
			L.EndGood(me, ep, "no_contact", now);
			return;
		}
		L.RefreshVs(me, ep, now);
		string key = KeyOf(pos, ei.m_vNearest, ep.m_sVs, ep.m_sFk);
		if (key != ep.m_sKey)
		{
			L.Flush(ep);
			ep.m_sKey = key;
		}
		ep.m_fKeyAt_ms = now;
		ep.m_fExpS = ep.m_fExpS + dt;
		if (ep.m_fExpS >= 60.0)
			L.Flush(ep);
		if (ep.m_sGood.IsEmpty())
		{
			if (!key.IsEmpty() && DCO_Opp.Speed(me) < 0.5)
			{
				ep.m_sGood = key;
				ep.m_vGoodPos = pos;
				ep.m_fGoodStart_ms = now;
				ep.m_iHits = 0;
				ep.m_bWounded = false;
			}
		}
		else if (vector.DistanceXZ(pos, ep.m_vGoodPos) > 5.0)
			L.EndGood(me, ep, "moved", now);
		else if (now - ep.m_fGoodStart_ms >= 90000.0)
			L.EndGood(me, ep, "time", now);
	}

	static void OnDamage(IEntity victim, IEntity attacker)
	{
		if (!victim || !attacker || victim == attacker || !On())
			return;
		ChimeraCharacter vc = ChimeraCharacter.Cast(victim);
		if (!vc || vc.IsInVehicle() || EntityUtils.IsPlayer(victim))
			return;
		if (!Hostile(victim, attacker))
		{
			DCO_Perf.Count("ilearn:reject_not_enemy");
			return;
		}
		DCO_IndLearn L = Get();
		if (!L)
			return;
		float now = Now();
		DCO_IndLearnEp ep = L.EpOf(victim);
		ep.m_fEnemyDmgAt_ms = now;
		ep.m_vAtkPos = attacker.GetOrigin();
		ep.m_bWounded = true;
		DCO_IndLearnEp ae;
		if (L.m_mEp.Find(attacker, ae) && !ae.m_sGood.IsEmpty())
			ae.m_iHits++;
		if (now - ep.m_fWoundAt_ms < 5000.0)
			return;
		ep.m_fWoundAt_ms = now;
		string key = L.EventKey(victim, ep, now);
		if (key.IsEmpty())
			return;
		L.m_Store.AddRaw("D|" + key, 0, 0.3);
		DCO_Perf.Count("ilearn:wound");
		if (DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG))
			DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(victim), string.Format("ilearn_event kind=wound unit=%1 key=%2 atk_d=%3", DCO_Log.IdUnit(victim), key, Math.Round(vector.Distance(victim.GetOrigin(), ep.m_vAtkPos))));
	}

	protected void HookKills()
	{
		if (m_bKillHook)
			return;
		SCR_BaseGameMode gm = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (!gm)
			return;
		gm.GetOnControllableDestroyed().Insert(OnKill);
		m_bKillHook = true;
	}

	void OnKill(notnull SCR_InstigatorContextData data)
	{
		if (!On())
			return;
		IEntity victim = data.GetVictimEntity();
		ChimeraCharacter vc = ChimeraCharacter.Cast(victim);
		if (!vc || vc.IsInVehicle() || EntityUtils.IsPlayer(victim))
			return;
		IEntity killer = data.GetKillerEntity();
		if (killer && killer != victim && Hostile(victim, killer))
		{
			DCO_IndLearnEp ep = EpOf(victim);
			ep.m_fEnemyDmgAt_ms = Now();
			ep.m_vAtkPos = killer.GetOrigin();
		}
		DCO_Perf.Count("ilearn:kill_event");
		DoDeath(victim, "kill_event");
	}

	static void OnDeath(IEntity victim)
	{
		if (!victim || !On())
			return;
		DCO_IndLearn L = Get();
		if (!L)
			return;
		L.DoDeath(victim, "agent");
	}

	protected void DoDeath(IEntity victim, string src)
	{
		DCO_IndLearn L = this;
		float now = Now();
		float deadAt;
		if (m_mDeadAt.Find(victim, deadAt))
		{
			DCO_Perf.Count("ilearn:death_dup");
			return;
		}
		if (m_mDeadAt.Count() > 300)
			m_mDeadAt.Clear();
		m_mDeadAt.Set(victim, now);
		DCO_IndLearnEp ep;
		if (!L.m_mEp.Find(victim, ep) || now - ep.m_fEnemyDmgAt_ms > 120000.0)
		{
			DCO_Perf.Count("ilearn:reject_no_enemy_dmg");
			DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(victim), string.Format("ilearn_reject unit=%1 reason=no_enemy_dmg src=%2", DCO_Log.IdUnit(victim), src));
			if (ep)
			{
				L.Flush(ep);
				L.m_mEp.Remove(victim);
			}
			return;
		}
		string key = L.EventKey(victim, ep, now);
		if (!key.IsEmpty())
		{
			float add = 1.0;
			if (now - ep.m_fWoundAt_ms < 5000.0)
				add = 0.7;
			L.m_Store.AddRaw("D|" + key, 0, add);
			DCO_Perf.Count("ilearn:death");
			DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(victim), string.Format("ilearn_event kind=death unit=%1 key=%2 atk_d=%3 add=%4 moving=%5 since_dmg_s=%6 src=%7", DCO_Log.IdUnit(victim), key, Math.Round(vector.Distance(victim.GetOrigin(), ep.m_vAtkPos)), add.ToString(-1, 1), DCO_Util.B01(DCO_Opp.Speed(victim) > 0.5), Math.Round((now - ep.m_fEnemyDmgAt_ms) / 1000.0), src));
		}
		L.EndGood(victim, ep, "killed", now);
		L.Flush(ep);
		L.m_mEp.Remove(victim);
	}
}
