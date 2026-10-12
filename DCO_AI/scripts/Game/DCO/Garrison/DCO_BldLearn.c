class DCO_BldEpisode
{
	IEntity m_Ent;
	IEntity m_Building;
	ResourceName m_rPrefab;
	string m_sPid;
	string m_sInst;
	string m_sSlot;
	string m_sUse;
	string m_sUnit;
	vector m_vPos;
	vector m_vLocal;
	float m_fCover;
	float m_fFire;
	float m_fScore;
	float m_fAssigned;
	float m_fArrived = -1;
	float m_fHp0 = 1;
	int m_iShots0;
	int m_iKills0;
	int m_iHits0;
	int m_iNear;
	bool m_bHit;
	float m_fContact;
	float m_fSeen;
	vector m_vThreat;
	bool m_bThreat;
	string m_sThreatSrc = "-";
	float m_fFireStart = -1;
	float m_fHitAt = -1;
	float m_fGroupContact;
}

class DCO_BldLearn
{
	protected static ref DCO_BldLearn s_Inst;

	protected BaseWorld m_World;
	protected ref DCO_LearnStore m_Prefab;
	protected ref DCO_LearnStore m_Inst;
	protected ref map<IEntity, ref DCO_BldEpisode> m_mEp = new map<IEntity, ref DCO_BldEpisode>();
	protected ref map<string, float> m_mSessAdd = new map<string, float>();
	protected ref set<string> m_PrunedPid = new set<string>();
	protected float m_fNextSave;

	static DCO_BldLearn Get()
	{
		if (!Replication.IsServer() || !GetGame().GetWorld())
			return null;
		if (s_Inst && s_Inst.m_World != GetGame().GetWorld())
		{
			SaveAll();
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", "learn_world_switch store=bld");
			s_Inst = null;
		}
		if (!s_Inst)
		{
			s_Inst = new DCO_BldLearn();
			s_Inst.m_World = GetGame().GetWorld();
			string bldDir = "$profile:DCO/BuildingLearn/";
			if (DCO_TestCaseComponent.Active())
				bldDir = "$profile:DCO/BuildingLearn/_test/";
			s_Inst.m_Prefab = new DCO_LearnStore(bldDir + "Prefabs.json");
			s_Inst.m_Inst = new DCO_LearnStore(bldDir + DCO_LearnStore.WorldName() + ".json");
			s_Inst.m_fNextSave = DCO_Util.NowS() + 300.0;
			GetGame().GetCallqueue().Remove(TickAll);
			GetGame().GetCallqueue().CallLater(TickAll, 2000, true);
		}
		return s_Inst;
	}

	static void SaveAll()
	{
		if (!s_Inst)
			return;
		s_Inst.m_Prefab.Save("end");
		s_Inst.m_Inst.Save("end");
	}

	static int ResetAll()
	{
		DCO_BldLearn L = Get();
		if (!L)
			return 0;
		int eps = L.m_mEp.Count();
		L.m_mEp.Clear();
		L.m_mSessAdd.Clear();
		L.m_PrunedPid.Clear();
		int n = L.m_Prefab.Clear() + L.m_Inst.Clear();
		int gone = 0;
		DCO_GarrisonRegistry reg = DCO_GarrisonRegistry.GetInstance();
		if (reg)
			gone = reg.ClearGone();
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("bldlearn_reset keys=%1 episodes=%2 gone_cleared=%3", n, eps, gone));
		return n;
	}

	static string SlotKey(DCO_GarrisonSlot s)
	{
		vector l = s.m_vLocalPos;
		float x = Math.Round(l[0] * 2.0) * 0.5;
		float y = Math.Round(l[1] * 2.0) * 0.5;
		float z = Math.Round(l[2] * 2.0) * 0.5;
		return x.ToString(-1, 1) + "_" + y.ToString(-1, 1) + "_" + z.ToString(-1, 1);
	}

	static int Sector(IEntity building, vector from, vector threat)
	{
		if (!building)
			return -1;
		vector d = building.VectorToLocal(threat - from);
		d[1] = 0;
		if (d.LengthSq() < 1.0)
			return -1;
		int s = Math.Round(Math.Repeat(d.ToYaw(), 360.0) / 45.0);
		return s % 8;
	}

	static float WeightE(string use)
	{
		if (use == "cover")
			return 0;
		if (use == "cqb")
			return 0.5;
		return 0.6;
	}

	static float PriorP(DCO_GarrisonSlot s)
	{
		return 0.3 + 0.5 * Math.Clamp(s.m_fCover, 0, 1);
	}

	static float PriorE(DCO_GarrisonSlot s)
	{
		return 0.3 + 0.5 * Math.Clamp(s.m_fFire, 0, 1);
	}

	static int Session()
	{
		DCO_BldLearn L = Get();
		if (!L)
			return 0;
		return L.m_Prefab.Sessions();
	}

	static float BuildingMult(IEntity building, ResourceName prefab, out float n)
	{
		n = 0;
		DCO_BldLearn L = Get();
		if (!L || !building || prefab.IsEmpty())
			return 1.0;
		float gn = 0;
		float pn = 0;
		float g = L.m_Prefab.Post("ALL|B|a", 0.5, gn);
		float pp = L.m_Prefab.Post(DCO_BldCache.Id(prefab) + "|B|a", g, pn);
		float post = L.m_Inst.Post(DCO_Util.XZ(building.GetOrigin()) + "|B|a", pp, n);
		n = n + pn;
		return DCO_LearnStore.MultOf(post, g);
	}

	static float PrefabRate(ResourceName prefab, DCO_GarrisonSlot s)
	{
		float prior = Math.Clamp(s.m_fScore, 0.05, 1.0);
		DCO_BldLearn L = Get();
		if (!L)
			return prior;
		float n = 0;
		return L.m_Prefab.Post(DCO_BldCache.Id(prefab) + "|" + SlotKey(s) + "|R|a", prior, n);
	}

	static bool Explore()
	{
		return Math.RandomFloat01() < 0.12;
	}

	static SCR_AIUtilityComponent UtilOf(IEntity e)
	{
		if (!e)
			return null;
		AIControlComponent ctrl = AIControlComponent.Cast(e.FindComponent(AIControlComponent));
		if (!ctrl || !ctrl.GetControlAIAgent())
			return null;
		return SCR_AIUtilityComponent.Cast(ctrl.GetControlAIAgent().FindComponent(SCR_AIUtilityComponent));
	}

	static bool ThreatOf(IEntity e, out vector pos)
	{
		pos = vector.Zero;
		SCR_AIUtilityComponent u = UtilOf(e);
		if (!u || !u.m_CombatComponent)
			return false;
		BaseTarget t = u.m_CombatComponent.GetCurrentTarget();
		if (!t)
			return false;
		pos = t.GetLastSeenPosition();
		return pos != vector.Zero;
	}

	static bool GroupThreat(IEntity e, vector from, out vector pos)
	{
		pos = vector.Zero;
		AIControlComponent ctrl = AIControlComponent.Cast(e.FindComponent(AIControlComponent));
		if (!ctrl || !ctrl.GetControlAIAgent())
			return false;
		SCR_AIGroup g = SCR_AIGroup.Cast(ctrl.GetControlAIAgent().GetParentGroup());
		if (!g)
			return false;
		SCR_AIGroupUtilityComponent gu = g.GetGroupUtilityComponent();
		if (!gu || !gu.m_Perception)
			return false;
		float best = 160000.0;
		foreach (SCR_AIGroupTargetCluster c : gu.m_Perception.m_aTargetClusters)
		{
			if (!c || !c.m_State || c.m_State.m_iCountAlive <= 0 || c.m_State.GetTimeSinceLastNewInformation() > 20.0)
				continue;
			vector p = c.m_State.GetCenterPosition();
			float dSq = vector.DistanceSqXZ(p, from);
			if (dSq < best)
			{
				best = dSq;
				pos = p;
			}
		}
		return pos != vector.Zero;
	}

	static string UseFor(IEntity e)
	{
		SCR_AIUtilityComponent u = UtilOf(e);
		if (u && u.m_CombatComponent && u.m_CombatComponent.GetCurrentTarget())
			return "fire";
		return "cover";
	}

	protected float Rate(string pid, string inst, string slot, string kind, string dir, float prior, out float n)
	{
		float pn = 0;
		float pp = m_Prefab.Post(pid + "|" + slot + "|" + kind + "|" + dir, prior, pn);
		float post = m_Inst.Post(inst + "|" + slot + "|" + kind + "|" + dir, pp, n);
		n = n + pn;
		return post;
	}

	static float SlotMult(DCO_GarrisonBuilding b, DCO_GarrisonSlot s, vector threat, bool hasThreat, string use, out float n)
	{
		n = 0;
		DCO_BldLearn L = Get();
		if (!L || !b || !s || !b.m_Entity || b.m_sPrefab.IsEmpty())
			return 1.0;

		string dir = "a";
		if (hasThreat)
		{
			int sec = Sector(b.m_Entity, s.m_vWorldPos, threat);
			if (sec >= 0)
				dir = sec.ToString();
		}

		string pid = DCO_BldCache.Id(b.m_sPrefab);
		string inst = DCO_Util.XZ(b.m_Entity.GetOrigin());
		string slot = SlotKey(s);
		float wE = WeightE(use);
		float p0 = PriorP(s);
		float e0 = PriorE(s);
		float nP = 0;
		float nE = 0;
		float postP = L.Rate(pid, inst, slot, "P", dir, p0, nP);
		float postE = e0;
		if (wE > 0)
			postE = L.Rate(pid, inst, slot, "E", dir, e0, nE);
		n = nP + nE;
		return DCO_LearnStore.MultOf(wE * postE + (1 - wE) * postP, wE * e0 + (1 - wE) * p0);
	}

	static void OccupyAt(IEntity ent, IEntity building, vector pos, string use)
	{
		DCO_GarrisonRegistry reg = DCO_GarrisonRegistry.GetInstance();
		if (!reg || !building)
			return;
		OccupyGB(ent, reg.Peek(building), pos, use);
	}

	static void OccupyGB(IEntity ent, DCO_GarrisonBuilding b, vector pos, string use)
	{
		if (!ent || !b || !b.m_bReady)
			return;
		DCO_GarrisonSlot best;
		float bestSq = 2.25;
		foreach (DCO_GarrisonSlot s : b.m_aSlots)
		{
			if (!s)
				continue;
			float dSq = vector.DistanceSq(s.m_vWorldPos, pos);
			if (dSq < bestSq)
			{
				bestSq = dSq;
				best = s;
			}
		}
		if (best)
			Occupy(ent, b, best, use);
	}

	static void Occupy(IEntity ent, DCO_GarrisonBuilding b, DCO_GarrisonSlot s, string use)
	{
		if (!ent || !b || !s || !b.m_Entity || b.m_sPrefab.IsEmpty() || !ChimeraCharacter.Cast(ent))
			return;
		DCO_BldLearn L = Get();
		if (!L)
			return;

		string slot = SlotKey(s);
		DCO_BldEpisode old = L.m_mEp.Get(ent);
		if (old)
		{
			if (old.m_Building == b.m_Entity && old.m_sSlot == slot)
				return;
			L.Finish(old, "reassigned");
		}

		DCO_BldEpisode ep = new DCO_BldEpisode();
		ep.m_Ent = ent;
		ep.m_Building = b.m_Entity;
		ep.m_rPrefab = b.m_sPrefab;
		ep.m_sPid = DCO_BldCache.Id(b.m_sPrefab);
		ep.m_sInst = DCO_Util.XZ(b.m_Entity.GetOrigin());
		ep.m_sSlot = slot;
		ep.m_sUse = use;
		ep.m_sUnit = DCO_Log.IdUnit(ent);
		ep.m_vPos = s.m_vWorldPos;
		ep.m_vLocal = s.m_vLocalPos;
		ep.m_fCover = s.m_fCover;
		ep.m_fFire = s.m_fFire;
		ep.m_fScore = s.m_fScore;
		ep.m_fAssigned = DCO_Util.NowS();
		L.m_mEp.Set(ent, ep);
	}

	static void TickAll()
	{
		if (s_Inst)
			s_Inst.Tick();
	}

	protected void Tick()
	{
		if (m_World != GetGame().GetWorld())
			return;
		float now = DCO_Util.NowS();
		float nowMs = GetGame().GetWorld().GetWorldTime();
		for (int i = m_mEp.Count() - 1; i >= 0; i--)
		{
			IEntity e = m_mEp.GetKey(i);
			DCO_BldEpisode ep = m_mEp.GetElement(i);
			if (!ep)
			{
				m_mEp.RemoveElement(i);
				continue;
			}
			if (!e)
			{
				Despawned(ep, now);
				m_mEp.RemoveElement(i);
				continue;
			}
			if (!DCO_Util.IsNotDead(e))
			{
				Finish(ep, "dead");
				m_mEp.RemoveElement(i);
				continue;
			}

			float dSq = vector.DistanceSq(e.GetOrigin(), ep.m_vPos);
			if (ep.m_fArrived < 0)
			{
				if (dSq < 1.44)
					Arrive(ep, e, now);
				else if (now - ep.m_fAssigned > 120.0)
				{
					DCO_Perf.Count("bldlearn:never_arrived");
					m_mEp.RemoveElement(i);
				}
				continue;
			}

			if (dSq > 9.0)
			{
				Finish(ep, "left");
				m_mEp.RemoveElement(i);
				continue;
			}
			if (now - ep.m_fArrived > 600.0)
			{
				Finish(ep, "timeout");
				m_mEp.RemoveElement(i);
				continue;
			}
			Sample(ep, e, nowMs, now);
		}

		if (now >= m_fNextSave)
		{
			m_fNextSave = now + 300.0;
			if (m_Prefab.IsDirty())
				m_Prefab.Save("auto");
			if (m_Inst.IsDirty())
				m_Inst.Save("auto");
		}
	}

	protected static float Hp(IEntity e)
	{
		if (!e)
			return 0;
		SCR_CharacterDamageManagerComponent dm = SCR_CharacterDamageManagerComponent.Cast(e.FindComponent(SCR_CharacterDamageManagerComponent));
		if (!dm)
			return 1;
		return dm.GetHealthScaled();
	}

	protected void Despawned(DCO_BldEpisode ep, float now)
	{
		DCO_Perf.Count("bldlearn:despawned");
		if (ep.m_fArrived < 0 || !DCO_Log.On(DCO_ELogChannel.UNIT, DCO_ELogLevel.DEBUG))
			return;
		DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.DEBUG, ep.m_sUnit, string.Format("slot_episode unit=%1 prefab=%2 inst=%3 slot=%4 use=%5 end=despawned dur_s=%6 counted=0 near=%7 contact_s=%8", ep.m_sUnit, DCO_BldCache.Name(ep.m_rPrefab), ep.m_sInst, ep.m_sSlot, ep.m_sUse, Math.Round(now - ep.m_fArrived), ep.m_iNear, Math.Round(ep.m_fContact)));
	}

	protected void Arrive(DCO_BldEpisode ep, IEntity e, float now)
	{
		ep.m_fArrived = now;
		ep.m_fHp0 = Hp(e);
		ep.m_iShots0 = DCO_ShotCounter.Total(e);
		ep.m_iKills0 = DCO_Battle.UnitKills(e);
		ep.m_iHits0 = DCO_Battle.UnitHits(e);
	}

	protected void Sample(DCO_BldEpisode ep, IEntity e, float nowMs, float now)
	{
		if (!ep.m_bHit && Hp(e) < ep.m_fHp0 - 0.01)
		{
			ep.m_bHit = true;
			ep.m_fHitAt = now;
		}
		SCR_AIUtilityComponent u = UtilOf(e);
		if (!u || !u.m_CombatComponent)
			return;
		int nn = u.m_CombatComponent.DCO_NearMissCount(nowMs, 2000.0);
		ep.m_iNear += nn;
		if (nn > 0 && ep.m_fFireStart < 0)
			ep.m_fFireStart = now;

		BaseTarget t = u.m_CombatComponent.GetCurrentTarget();
		vector gpos;
		bool group = GroupThreat(e, ep.m_vPos, gpos);
		if (t || group)
			ep.m_fContact += 2.0;
		if (!t && group)
			ep.m_fGroupContact += 2.0;
		if (t && t.GetTimeSinceSeen() < 2.0)
			ep.m_fSeen += 2.0;

		if (ep.m_bThreat)
			return;
		vector from;
		if (t && t.GetLastSeenPosition() != vector.Zero)
		{
			ep.m_vThreat = t.GetLastSeenPosition();
			ep.m_sThreatSrc = "target";
		}
		else if (u.m_CombatComponent.DCO_LastNearMissFrom(nowMs, 2000.0, from))
		{
			ep.m_vThreat = from;
			ep.m_sThreatSrc = "near_miss";
		}
		else if (group)
		{
			ep.m_vThreat = gpos;
			ep.m_sThreatSrc = "group";
		}
		else
			return;
		ep.m_bThreat = true;
		DCO_Perf.Count("bldlearn:threat_src_" + ep.m_sThreatSrc);
	}

	protected void AddRate(DCO_BldEpisode ep, string kind, int sec, float value, float weight)
	{
		string pk = ep.m_sPid + "|" + ep.m_sSlot + "|" + kind + "|";
		string ik = ep.m_sInst + "|" + ep.m_sSlot + "|" + kind + "|";
		m_Prefab.Add(pk + "a", value, weight);
		m_Inst.Add(ik + "a", value, weight);
		if (sec < 0)
			return;
		int left = (sec + 7) % 8;
		int right = (sec + 1) % 8;
		m_Prefab.Add(pk + sec.ToString(), value, weight);
		m_Inst.Add(ik + sec.ToString(), value, weight);
		m_Prefab.Add(pk + left.ToString(), value, weight * 0.5);
		m_Inst.Add(ik + left.ToString(), value, weight * 0.5);
		m_Prefab.Add(pk + right.ToString(), value, weight * 0.5);
		m_Inst.Add(ik + right.ToString(), value, weight * 0.5);
	}

	protected void Finish(DCO_BldEpisode ep, string reason)
	{
		if (!ep || ep.m_fArrived < 0)
			return;

		IEntity e = ep.m_Ent;
		float now = DCO_Util.NowS();
		float dur = now - ep.m_fArrived;
		bool dead = reason == "dead";
		bool hit = ep.m_bHit || dead;
		if (e && !dead && Hp(e) < ep.m_fHp0 - 0.01)
			hit = true;
		if (hit && ep.m_fHitAt < 0)
			ep.m_fHitAt = now;

		int shots = 0;
		int kills = 0;
		int hits = 0;
		if (e)
		{
			shots = DCO_ShotCounter.Total(e) - ep.m_iShots0;
			kills = DCO_Battle.UnitKills(e) - ep.m_iKills0;
			hits = DCO_Battle.UnitHits(e) - ep.m_iHits0;
		}

		bool forced = false;
		if (reason == "left" && ep.m_sUse != "cqb")
		{
			SCR_AIUtilityComponent u = UtilOf(e);
			if (u && u.m_CombatComponent && u.m_CombatComponent.DCO_NearMissCount(GetGame().GetWorld().GetWorldTime(), 10000.0) >= 3)
				forced = true;
		}

		bool underFire = ep.m_iNear >= 3 || hit;
		float pv = -1;
		float fireS = 0;
		if (underFire)
		{
			pv = 1;
			if (hit || forced)
			{
				float tEnd = now;
				if (hit)
					tEnd = ep.m_fHitAt;
				if (ep.m_fFireStart >= 0)
					fireS = Math.Max(tEnd - ep.m_fFireStart, 0);
				pv = Math.Clamp(fireS / 60.0, 0, 1);
			}
			else if (ep.m_fFireStart >= 0)
				fireS = now - ep.m_fFireStart;
		}

		float wE = WeightE(ep.m_sUse);
		float ev = -1;
		float ew = 1;
		if (wE > 0 && ep.m_fContact >= 20.0)
		{
			ev = 0;
			bool seen = ep.m_fSeen >= 0.2 * ep.m_fContact;
			if ((seen || shots >= 5) && dur >= 30.0)
			{
				ev = 1;
				if (kills > 0 || hits > 0)
					ew = 1.5;
			}
		}

		int sec = -1;
		if (ep.m_bThreat)
			sec = Sector(ep.m_Building, ep.m_vPos, ep.m_vThreat);

		string unit = DCO_Log.IdUnit(e);
		string seenPct = "-";
		if (ep.m_fContact > 0)
		{
			int sp = Math.Round(100.0 * ep.m_fSeen / ep.m_fContact);
			seenPct = sp.ToString();
		}

		if (pv < 0 && ev < 0)
		{
			DCO_Perf.Count("bldlearn:ignored");
			if (DCO_Log.On(DCO_ELogChannel.UNIT, DCO_ELogLevel.DEBUG))
				DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.DEBUG, unit, string.Format("slot_episode unit=%1 prefab=%2 inst=%3 slot=%4 use=%5 end=%6 dur_s=%7 counted=0 near=%8 contact_s=%9", unit, DCO_BldCache.Name(ep.m_rPrefab), ep.m_sInst, ep.m_sSlot, ep.m_sUse, reason, Math.Round(dur), ep.m_iNear, Math.Round(ep.m_fContact)));
			return;
		}

		if (pv >= 0)
			AddRate(ep, "P", sec, pv, 1);
		if (ev >= 0)
			AddRate(ep, "E", sec, ev, ew);

		float rv = pv;
		if (pv < 0)
			rv = ev;
		else if (ev >= 0)
			rv = wE * ev + (1 - wE) * pv;
		string rk = ep.m_sPid + "|" + ep.m_sSlot + "|R|a";
		m_Prefab.Add(rk, rv);
		m_mSessAdd.Set(rk, m_mSessAdd.Get(rk) + 1.0);
		m_Prefab.Add("ALL|B|a", rv);
		m_Prefab.Add(ep.m_sPid + "|B|a", rv);
		m_Inst.Add(ep.m_sInst + "|B|a", rv);

		DCO_Perf.Count("bldlearn:episode");
		DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, unit, string.Format("slot_episode unit=%1 prefab=%2 inst=%3 slot=%4 use=%5 end=%6 dur_s=%7 near=%8 hit=%9", unit, DCO_BldCache.Name(ep.m_rPrefab), ep.m_sInst, ep.m_sSlot, ep.m_sUse, reason, Math.Round(dur), ep.m_iNear, DCO_Util.B01(hit))
			+ string.Format(" forced=%1 contact_s=%2 seen_pct=%3 shots=%4 kills=%5 hits=%6 sector=%7 P=%8 E=%9", DCO_Util.B01(forced), Math.Round(ep.m_fContact), seenPct, shots, kills, hits, sec, Val(pv), Val(ev))
			+ string.Format(" e_weight=%1 r=%2 fire_s=%3 group_contact_s=%4 threat_src=%5", ew, rv.ToString(-1, 2), Math.Round(fireS), Math.Round(ep.m_fGroupContact), ep.m_sThreatSrc));

		float rn = 0;
		float rate = m_Prefab.Post(rk, Math.Clamp(ep.m_fScore, 0.05, 1.0), rn);
		if (rn >= 8.0 && rate < 0.3)
		{
			float sess = m_mSessAdd.Get(rk);
			string skip;
			if (m_PrunedPid.Contains(ep.m_sPid))
				skip = "session_cap";
			else if (rn - sess < 4.0)
				skip = "one_session";
			if (!skip.IsEmpty())
			{
				DCO_Perf.Count("bldcache:prune_brake_" + skip);
				if (DCO_Log.On(DCO_ELogChannel.SQUAD, DCO_ELogLevel.DEBUG))
					DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.DEBUG, "-", string.Format("bldcache_prune_skip prefab=%1 slot=%2 rate=%3 n=%4 n_session=%5 reason=%6", DCO_BldCache.Name(ep.m_rPrefab), ep.m_sSlot, rate.ToString(-1, 2), rn.ToString(-1, 1), sess.ToString(-1, 1), skip));
			}
			else
			{
				DCO_GarrisonRegistry reg = DCO_GarrisonRegistry.GetInstance();
				if (reg && reg.PruneSlot(ep.m_rPrefab, ep.m_vLocal, rate, rn))
					m_PrunedPid.Insert(ep.m_sPid);
			}
		}
	}

	protected static string Val(float v)
	{
		if (v < 0)
			return "-";
		return v.ToString(-1, 2);
	}
}
