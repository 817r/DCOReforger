class DCO_RearmState
{
	float m_fNextSearch_ms;
	float m_fStationsBlockedUntil_ms;
	int m_iInitSpare = -1;
	typename m_InitWell;
	ref map<IEntity, float> m_mFailedUntil;

	void MarkFailed(IEntity src, float now)
	{
		if (!src)
			return;
		if (!m_mFailedUntil)
			m_mFailedUntil = new map<IEntity, float>();
		m_mFailedUntil.Set(src, now + 300000.0);
	}

	bool IsBlocked(IEntity src, float now)
	{
		if (!src || !m_mFailedUntil)
			return false;
		float until;
		if (!m_mFailedUntil.Find(src, until))
			return false;
		if (now < until)
			return true;
		m_mFailedUntil.Remove(src);
		return false;
	}
}

class DCO_Rearm
{
	protected static ref array<IEntity> s_aCorpses;
	protected static ref array<IEntity> s_aStations;
	protected static ref map<IEntity, float> s_mCorpseBookedUntil;

	static void RegisterCorpse(IEntity e)
	{
		if (!e || !Replication.IsServer())
			return;
		if (!s_aCorpses)
			s_aCorpses = {};
		if (s_aCorpses.Contains(e))
			return;
		DCO_Perf.Count("rearm:corpse_registered");
		if (s_aCorpses.Count() >= 96)
			s_aCorpses.Remove(0);
		s_aCorpses.Insert(e);
	}

	static array<IEntity> Corpses()
	{
		return s_aCorpses;
	}

	static void RegisterStation(IEntity e)
	{
		if (!e)
			return;
		if (!s_aStations)
			s_aStations = {};
		if (!s_aStations.Contains(e))
			s_aStations.Insert(e);
	}

	static float StationDist(vector pos, float maxDist)
	{
		if (!s_aStations)
			return -1;
		float best = -1;
		foreach (IEntity s : s_aStations)
		{
			if (!s)
				continue;
			float d = vector.DistanceXZ(s.GetOrigin(), pos);
			if (d <= maxDist && (best < 0 || d < best))
				best = d;
		}
		return best;
	}

	static void UnregisterStation(IEntity e)
	{
		if (s_aStations)
			s_aStations.RemoveItem(e);
	}

	static int SpareMags(SCR_AICombatComponent combat, IEntity ent, out typename well)
	{
		if (!combat || !ent)
			return -1;
		BaseWeaponManagerComponent wm = BaseWeaponManagerComponent.Cast(ent.FindComponent(BaseWeaponManagerComponent));
		if (!wm || !wm.GetCurrent())
			return -1;
		array<BaseMuzzleComponent> muzzles = {};
		wm.GetCurrent().GetMuzzlesList(muzzles);
		if (muzzles.IsEmpty() || !muzzles[0] || !muzzles[0].GetMagazineWell())
			return -1;
		well = muzzles[0].GetMagazineWell().Type();
		return combat.GetMagazineCount(well, false);
	}

	static void Tick(SCR_AIUtilityComponent u, SCR_AICombatComponent combat, DCO_RearmState st, float now)
	{
		ChimeraCharacter me = ChimeraCharacter.Cast(u.m_OwnerEntity);
		if (!me || me.IsInVehicle() || !DCO_GlobalAIComponent.BehaviorOnFor(me, DCO_EBehavior.REARM))
			return;
		if (u.FindActionOfType(DCO_AIRearmBehavior))
			return;

		typename well;
		int spare = SpareMags(combat, me, well);
		if (spare < 0)
			DCO_Perf.Count("rearm:no_weapon_well");
		if (spare >= 0 && (st.m_iInitSpare < 0 || well != st.m_InitWell))
		{
			st.m_iInitSpare = spare;
			st.m_InitWell = well;
		}
		if (spare == 2 && now >= st.m_fNextSearch_ms && (DCO_Opp.Flags(u) & 32))
		{
			st.m_fNextSearch_ms = now + 15000.0;
			TryOppLoot(u, me, combat, well, spare, now, st);
			return;
		}
		if (spare < 0 || spare > 1 || now < st.m_fNextSearch_ms)
			return;
		if (spare > 0 && spare >= st.m_iInitSpare && DCO_Stress.T(u, "rearm.rel_init", 1.0) > 0.5)
		{
			st.m_fNextSearch_ms = now + 15000.0;
			DCO_Perf.Count("rearm:skip_loadout_full");
			return;
		}
		st.m_fNextSearch_ms = now + 15000.0;

		if (spare > 0 && Tasked(u))
		{
			DCO_Perf.Count("rearm:skip_tasked");
			return;
		}

		bool threatened = u.m_ThreatSystem && u.m_ThreatSystem.GetState() == EAIThreatState.THREATENED;
		float maxDist = 35.0;
		if (spare == 0)
			maxDist = 60.0;
		if (threatened || Leashed(u))
			maxDist = 15.0;

		bool station;
		IEntity src = Pick(me, well, maxDist, station, now, st);
		if (!src)
		{
			DCO_Perf.Count("rearm:no_source");
			if (DCO_Log.On(DCO_ELogChannel.UNIT, DCO_ELogLevel.DEBUG))
				DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(me), string.Format("rearm_no_source who=%1 spare=%2 max_dist=%3 corpses=%4 stations=%5 rej=%6", DCO_Log.IdUnit(me), spare, maxDist, CountOf(s_aCorpses), CountOf(s_aStations), s_sLastRej));
			return;
		}

		if (!station)
			Book(src, now + 95000.0);
		DCO_AIRearmBehavior rb = new DCO_AIRearmBehavior(u, null, src, station, well, spare);
		rb.DCO_SetState(st);
		u.AddAction(rb);
	}

	protected static void TryOppLoot(SCR_AIUtilityComponent u, ChimeraCharacter me, SCR_AICombatComponent combat, typename well, int spare, float now, DCO_RearmState st)
	{
		if (u.m_ThreatSystem && u.m_ThreatSystem.GetState() == EAIThreatState.THREATENED)
		{
			DCO_Perf.Count("rearm:opp_loot_skip_threatened");
			return;
		}
		string gate = DCO_Opp.Gate(u, true);
		if (gate != "")
		{
			DCO_Perf.Count("rearm:opp_loot_skip_" + gate);
			return;
		}
		bool station;
		IEntity src = Pick(me, well, 10.0, station, now, st);
		if (!src || station)
			return;
		if (!DCO_Opp.LootSafe(u, src))
		{
			DCO_Perf.Count("rearm:opp_loot_skip_exposed");
			return;
		}
		Book(src, now + 95000.0);
		DCO_AIRearmBehavior rb = new DCO_AIRearmBehavior(u, null, src, false, well, spare);
		rb.DCO_SetState(st);
		u.AddAction(rb);
		DCO_Perf.Count("rearm:opp_loot");
		DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("opp_loot unit=%1 spare=%2 corpse_d=%3",
			DCO_Log.IdUnit(me), spare, Math.Round(vector.Distance(me.GetOrigin(), src.GetOrigin()))));
	}

	static void Book(IEntity src, float until_ms)
	{
		if (!s_mCorpseBookedUntil)
			s_mCorpseBookedUntil = new map<IEntity, float>();
		s_mCorpseBookedUntil.Set(src, until_ms);
	}

	static void Unbook(IEntity src)
	{
		if (s_mCorpseBookedUntil && src)
			s_mCorpseBookedUntil.Remove(src);
	}

	protected static bool Booked(IEntity src, float now)
	{
		if (!s_mCorpseBookedUntil)
			return false;
		float until;
		if (!s_mCorpseBookedUntil.Find(src, until))
			return false;
		if (until < now)
		{
			s_mCorpseBookedUntil.Remove(src);
			return false;
		}
		return true;
	}

	protected static bool Leashed(SCR_AIUtilityComponent u)
	{
		AIAgent agent = u.GetAIAgent();
		if (!agent || !agent.GetParentGroup())
			return false;
		DCO_GroupUtilityComponent g = DCO_GroupUtilityComponent.Cast(agent.GetParentGroup().FindComponent(DCO_GroupUtilityComponent));
		return g && (g.GetTask() == DCO_EGroupTask.DEFEND || g.GetTask() == DCO_EGroupTask.GARRISON);
	}

	protected static bool Tasked(SCR_AIUtilityComponent u)
	{
		AIAgent agent = u.GetAIAgent();
		if (!agent || !agent.GetParentGroup())
			return false;
		DCO_GroupUtilityComponent g = DCO_GroupUtilityComponent.Cast(agent.GetParentGroup().FindComponent(DCO_GroupUtilityComponent));
		if (!g)
			return false;
		DCO_EGroupTask t = g.GetTask();
		return t == DCO_EGroupTask.ATTACK || t == DCO_EGroupTask.FLANK || t == DCO_EGroupTask.SUPPORT_BY_FIRE || t == DCO_EGroupTask.RECON || t == DCO_EGroupTask.REINFORCE;
	}

	protected static Faction FactionOf(IEntity e)
	{
		FactionAffiliationComponent fac = FactionAffiliationComponent.Cast(e.FindComponent(FactionAffiliationComponent));
		if (!fac)
			return null;
		return fac.GetAffiliatedFaction();
	}

	static bool HasMags(IEntity e, typename well)
	{
		if (!e)
			return false;
		SCR_InventoryStorageManagerComponent inv = SCR_InventoryStorageManagerComponent.Cast(e.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!inv)
			return false;
		array<IEntity> items = {};
		array<typename> comps = {};
		comps.Insert(MagazineComponent);
		inv.FindItemsWithComponents(items, comps);
		foreach (IEntity it : items)
		{
			MagazineComponent mag = MagazineComponent.Cast(it.FindComponent(MagazineComponent));
			if (mag && mag.GetMagazineWell() && mag.GetMagazineWell().Type() == well && mag.GetAmmoCount() > 0)
				return true;
		}
		return false;
	}

	protected static string s_sLastRej;

	protected static int CountOf(array<IEntity> a)
	{
		if (!a)
			return 0;
		return a.Count();
	}

	protected static IEntity Pick(IEntity me, typename well, float maxDist, out bool station, float now, DCO_RearmState st)
	{
		vector mp = me.GetOrigin();
		float maxSq = maxDist * maxDist;
		int rFar;
		int rBooked;
		int rFaction;
		int rNoMags;
		IEntity best;
		float bestScore = float.MAX;

		bool stationsOk = !st || now >= st.m_fStationsBlockedUntil_ms;
		if (!stationsOk)
			DCO_Perf.Count("rearm:skip_stations_blocked");
		if (s_aStations && stationsOk)
		{
			for (int i = s_aStations.Count() - 1; i >= 0; i--)
			{
				IEntity s = s_aStations[i];
				if (!s)
				{
					s_aStations.Remove(i);
					continue;
				}
				float dSq = vector.DistanceSqXZ(mp, s.GetOrigin());
				if (dSq > maxSq)
					continue;
				if (st && st.IsBlocked(s, now))
				{
					DCO_Perf.Count("rearm:skip_blocked");
					continue;
				}
				float score = dSq * 0.64;
				if (score < bestScore)
				{
					bestScore = score;
					best = s;
					station = true;
				}
			}
		}

		if (s_aCorpses)
		{
			Faction mine = FactionOf(me);
			bool enemyOk = DCO_GlobalAIComponent.BehaviorOnFor(me, DCO_EBehavior.REARM_ENEMY_SOURCES);
			for (int j = s_aCorpses.Count() - 1; j >= 0; j--)
			{
				IEntity c = s_aCorpses[j];
				if (!c)
				{
					s_aCorpses.Remove(j);
					continue;
				}
				float cSq = vector.DistanceSqXZ(mp, c.GetOrigin());
				if (cSq > maxSq || cSq >= bestScore)
				{
					rFar++;
					continue;
				}
				if (Booked(c, now) || (st && st.IsBlocked(c, now)))
				{
					rBooked++;
					continue;
				}
				Faction f = FactionOf(c);
				if (!enemyOk && mine && f && !mine.IsFactionFriendly(f))
				{
					rFaction++;
					continue;
				}
				if (!HasMags(c, well))
				{
					rNoMags++;
					continue;
				}
				bestScore = cSq;
				best = c;
				station = false;
			}
		}
		s_sLastRej = string.Format("far:%1|booked:%2|faction:%3|nomags:%4", rFar, rBooked, rFaction, rNoMags);
		return best;
	}
}

class DCO_TakeBehavior : SCR_AIBehaviorBase
{
	int DCO_DoTake()
	{
		return 0;
	}

	float DCO_CurrentPrio()
	{
		return 0;
	}
}

class DCO_RearmMoveBehavior : SCR_AIMoveIndividuallyBehavior
{
	protected DCO_TakeBehavior m_DCOParent;
	protected vector m_vDCOTarget;

	void DCO_RearmMoveBehavior(SCR_AIUtilityComponent utility, SCR_AIActivityBase groupActivity, vector pos, float priority = PRIORITY_BEHAVIOR_MOVE_INDIVIDUALLY, float priorityLevel = PRIORITY_LEVEL_NORMAL, IEntity ent = null, float radius = 1.0)
	{
		m_vDCOTarget = pos;
	}

	void DCO_SetParent(DCO_TakeBehavior parent)
	{
		m_DCOParent = parent;
	}

	override float CustomEvaluate()
	{
		EAIActionState st = GetActionState();
		if (st == EAIActionState.COMPLETED || st == EAIActionState.FAILED)
			return 0;
		if (!m_DCOParent)
			return 0;
		float p = m_DCOParent.DCO_CurrentPrio();
		if (p <= 0)
			return 0;
		if (m_Utility && m_Utility.m_OwnerEntity && vector.DistanceXZ(m_Utility.m_OwnerEntity.GetOrigin(), m_vDCOTarget) <= DCO_Stress.T(m_Utility, "rearm.reach_m", 2.5))
		{
			DCO_Perf.Count("rearm:move_reach_done");
			Complete();
			return 0;
		}
		return p + 0.1;
	}
}

class DCO_AIRearmBehavior : DCO_TakeBehavior
{
	protected IEntity m_DCOSource;
	protected bool m_bDCOStation;
	protected typename m_DCOWell;
	protected float m_fDCOStart_ms;
	protected float m_fDCOPrio = 62.0;
	protected bool m_bDCOPaused;
	protected bool m_bDCOLogged;
	protected string m_sDCOResult = "";
	protected int m_iDCOTaken;
	protected int m_iDCORounds;
	protected int m_iDCOSpare;
	protected ref DCO_RearmMoveBehavior m_DCOMove;
	protected DCO_RearmState m_DCOState;
	protected int m_iDCORemoves;
	protected int m_iDCOMagsFound = -1;
	protected int m_iDCOWrongWell;
	protected int m_iDCOEmpty;
	protected int m_iDCONoStorage;
	protected int m_iDCOMoveRefused;

	protected bool DCO_MoveDone()
	{
		if (!m_DCOMove)
			return true;
		EAIActionState ms = m_DCOMove.GetActionState();
		return ms == EAIActionState.COMPLETED || ms == EAIActionState.FAILED;
	}

	void DCO_SetState(DCO_RearmState st)
	{
		m_DCOState = st;
	}

	void DCO_AIRearmBehavior(SCR_AIUtilityComponent utility, SCR_AIActivityBase groupActivity, IEntity source, bool station, typename well, int spare)
	{
		m_sBehaviorTree = "AI/BehaviorTrees/Chimera/Soldier/DCO_Rearm.bt";
		SetPriority(62.0);
		SetIsUniqueInActionQueue(true);
		m_DCOSource = source;
		m_bDCOStation = station;
		m_DCOWell = well;
		m_iDCOSpare = spare;
		m_fDCOStart_ms = GetGame().GetWorld().GetWorldTime();
		if (!utility || !source)
			return;

		m_fDCOPrio = DCO_EvalPrio(spare, vector.DistanceXZ(utility.GetOrigin(), source.GetOrigin()), DCO_Threatened(utility));
		m_DCOMove = new DCO_RearmMoveBehavior(utility, null, source.GetOrigin(), m_fDCOPrio + 0.1, PRIORITY_LEVEL_NORMAL, null, 1.0);
		m_DCOMove.DCO_SetParent(this);
		utility.AddAction(m_DCOMove);

		DCO_Perf.Count("rearm:start");
		IEntity me = utility.m_OwnerEntity;
		string threat = "-";
		if (utility.m_ThreatSystem)
			threat = typename.EnumToString(EAIThreatState, utility.m_ThreatSystem.GetState());
		DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("rearm_start who=%1 src=%2 dist=%3 spare=%4 prio=%5 threat=%6",
			DCO_Log.IdUnit(me), DCO_SrcName(), Math.Round(vector.DistanceXZ(utility.GetOrigin(), source.GetOrigin())), spare, m_fDCOPrio, threat));
	}

	protected string DCO_SrcName()
	{
		if (m_bDCOStation)
			return "station";
		return "corpse";
	}

	protected static bool DCO_Threatened(SCR_AIUtilityComponent u)
	{
		return u && u.m_ThreatSystem && u.m_ThreatSystem.GetState() == EAIThreatState.THREATENED;
	}

	protected static float DCO_EvalPrio(int spare, float dist, bool threatened)
	{
		if (threatened && dist > 15.0)
			return 0;
		if (spare <= 0)
		{
			if (dist <= 15.0)
				return 118.0;
			return 94.0;
		}
		return 62.0;
	}

	override float DCO_CurrentPrio()
	{
		return m_fDCOPrio;
	}

	override int GetCause()
	{
		return SCR_EAIBehaviorCause.SELF_AID;
	}

	override float CustomEvaluate()
	{
		EAIActionState st = GetActionState();
		if (st == EAIActionState.COMPLETED || st == EAIActionState.FAILED)
			return 0;
		IEntity me = m_Utility.m_OwnerEntity;
		if (!m_DCOSource || !me)
		{
			DCO_Finish("source_gone", false);
			return 0;
		}
		float now = GetGame().GetWorld().GetWorldTime();
		if (now - m_fDCOStart_ms > 90000.0)
		{
			DCO_Finish("timeout", false);
			return 0;
		}
		typename w;
		int spare = DCO_Rearm.SpareMags(m_Utility.m_CombatComponent, me, w);
		if (spare >= 3)
		{
			DCO_Finish("enough", true);
			return 0;
		}
		if (DCO_EndRel())
		{
			int init = m_Utility.m_CombatComponent.DCO_RearmInit(w);
			if (init > 0 && spare >= init)
			{
				DCO_Perf.Count("rearm:enough_loadout");
				DCO_Finish("enough", true);
				return 0;
			}
		}
		if (!m_bDCOStation && !DCO_Rearm.HasMags(m_DCOSource, m_DCOWell))
		{
			DCO_Finish("source_gone", false);
			return 0;
		}
		float d = vector.DistanceXZ(me.GetOrigin(), m_DCOSource.GetOrigin());
		m_fDCOPrio = DCO_EvalPrio(spare, d, DCO_Threatened(m_Utility));
		if (m_fDCOPrio > 0 && d > 3.0 && DCO_MoveDone())
		{
			if (m_iDCORemoves >= 3)
			{
				DCO_Finish("unreachable", false);
				return 0;
			}
			m_iDCORemoves++;
			m_DCOMove = new DCO_RearmMoveBehavior(m_Utility, null, m_DCOSource.GetOrigin(), m_fDCOPrio + 0.1, PRIORITY_LEVEL_NORMAL, null, 1.0);
			m_DCOMove.DCO_SetParent(this);
			m_Utility.AddAction(m_DCOMove);
			DCO_Perf.Count("rearm:remove");
			if (DCO_Log.On(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO))
				DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("rearm_remove who=%1 dist=%2 n=%3 reason=move_ended_short", DCO_Log.IdUnit(me), Math.Round(d), m_iDCORemoves));
			return 0;
		}
		bool paused = m_fDCOPrio <= 0;
		if (paused != m_bDCOPaused)
		{
			m_bDCOPaused = paused;
			if (paused)
				DCO_Perf.Count("rearm:pause");
			if (DCO_Log.On(DCO_ELogChannel.UNIT, DCO_ELogLevel.DEBUG))
				DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(me), string.Format("rearm_pause who=%1 paused=%2 reason=threatened dist=%3", DCO_Log.IdUnit(me), DCO_Util.B01(paused), Math.Round(d)));
		}
		return m_fDCOPrio;
	}

	override int DCO_DoTake()
	{
		IEntity me = m_Utility.m_OwnerEntity;
		if (!me || !m_DCOSource)
			return 0;
		if (vector.DistanceXZ(me.GetOrigin(), m_DCOSource.GetOrigin()) > 3.0)
		{
			if (m_iDCOTaken > 0)
				DCO_Perf.Count("rearm:keep_taken");
			else
				m_sDCOResult = "unreachable";
			return 0;
		}
		SCR_InventoryStorageManagerComponent myInv = SCR_InventoryStorageManagerComponent.Cast(me.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!myInv)
		{
			m_sDCOResult = "no_inventory";
			return 0;
		}

		if (m_bDCOStation)
		{
			typename wb;
			int before = DCO_Rearm.SpareMags(m_Utility.m_CombatComponent, me, wb);
			myInv.ResupplyMagazines(4);
			int after = DCO_Rearm.SpareMags(m_Utility.m_CombatComponent, me, wb);
			m_iDCORounds = -1;
			if (after <= before)
			{
				if (m_iDCOTaken > 0)
				{
					DCO_Perf.Count("rearm:keep_taken");
					if (DCO_EndRel())
						DCO_Finish("taken", true);
				}
				else
					m_sDCOResult = "no_gain";
				return 0;
			}
			m_iDCOTaken += after - before;
			m_sDCOResult = "taken";
			return after - before;
		}

		SCR_InventoryStorageManagerComponent src = SCR_InventoryStorageManagerComponent.Cast(m_DCOSource.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!src)
		{
			m_sDCOResult = "source_gone";
			return 0;
		}
		array<IEntity> items = {};
		array<typename> comps = {};
		comps.Insert(MagazineComponent);
		src.FindItemsWithComponents(items, comps);
		m_iDCOMagsFound = items.Count();
		int got;
		int noRoom;
		foreach (IEntity it : items)
		{
			if (m_iDCOTaken >= 3)
				break;
			MagazineComponent mag = MagazineComponent.Cast(it.FindComponent(MagazineComponent));
			if (!mag || !mag.GetMagazineWell() || mag.GetMagazineWell().Type() != m_DCOWell)
			{
				m_iDCOWrongWell++;
				continue;
			}
			int rounds = mag.GetAmmoCount();
			if (rounds <= 0)
			{
				m_iDCOEmpty++;
				continue;
			}
			bool moved;
			BaseInventoryStorageComponent storage = myInv.FindStorageForItem(it, EStoragePurpose.PURPOSE_DEPOSIT);
			if (!storage)
			{
				m_iDCONoStorage++;
				noRoom++;
			}
			if (storage && myInv.TryMoveItemToStorage(it, storage))
			{
				moved = true;
				DCO_Perf.Count("rearm:move_storage");
			}
			else if (myInv.TryInsertItem(it, EStoragePurpose.PURPOSE_DEPOSIT))
			{
				moved = true;
				DCO_Perf.Count("rearm:move_insert");
			}
			if (!moved)
			{
				DCO_Perf.Count("rearm:move_failed");
				m_iDCOMoveRefused++;
				if (m_iDCOMoveRefused <= 3)
				{
					string parentName = "-";
					InventoryItemComponent iic = InventoryItemComponent.Cast(it.FindComponent(InventoryItemComponent));
					if (iic && iic.GetParentSlot() && iic.GetParentSlot().GetStorage())
						parentName = iic.GetParentSlot().GetStorage().ClassName();
					string itemName = it.ClassName();
					if (it.GetPrefabData())
						itemName = it.GetPrefabData().GetPrefabName();
					DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("rearm_take_fail who=%1 item=%2 storage_found=%3 src_parent=%4 rounds=%5", DCO_Log.IdUnit(me), itemName, DCO_Util.B01(storage != null), parentName, rounds));
				}
				continue;
			}
			m_iDCOTaken++;
			got++;
			m_iDCORounds += rounds;
		}
		if (m_iDCOTaken > 0)
			m_sDCOResult = "taken";
		else
			m_sDCOResult = "move_failed";
		if (got == 0 && noRoom > 0 && DCO_EndRel())
		{
			if (m_iDCOTaken <= 0)
				m_sDCOResult = "no_space";
			DCO_Perf.Count("rearm:end_full");
			DCO_Finish(m_sDCOResult, m_iDCOTaken > 0);
		}
		return m_iDCOTaken;
	}

	protected bool DCO_EndRel()
	{
		return DCO_Stress.T(m_Utility, "rearm.end_rel", 1.0) > 0.5;
	}

	protected void DCO_Finish(string result, bool ok)
	{
		if (m_sDCOResult.IsEmpty())
			m_sDCOResult = result;
		if (ok)
			Complete();
		else
			Fail();
	}

	protected void DCO_LogEnd(string fallback)
	{
		if (m_bDCOLogged)
			return;
		m_bDCOLogged = true;
		if (!m_bDCOStation)
			DCO_Rearm.Unbook(m_DCOSource);
		string result = m_sDCOResult;
		if (result.IsEmpty())
			result = fallback;
		if (result == "taken")
			DCO_Perf.Count("rearm:taken_" + DCO_SrcName());
		else
			DCO_Perf.Count("rearm:fail_" + result);
		bool blocked = result == "no_gain" || result == "unreachable" || result == "timeout" || result == "move_failed";
		if (blocked && m_DCOState)
			m_DCOState.MarkFailed(m_DCOSource, GetGame().GetWorld().GetWorldTime());
		if (m_DCOState && m_bDCOStation)
		{
			float block_ms = 180000.0;
			if (result == "no_gain")
				block_ms = 300000.0;
			m_DCOState.m_fStationsBlockedUntil_ms = GetGame().GetWorld().GetWorldTime() + block_ms;
		}
		IEntity me;
		if (m_Utility)
			me = m_Utility.m_OwnerEntity;
		DCO_ELogLevel level = DCO_ELogLevel.INFO;
		if (result == "move_failed" || result == "no_inventory" || result == "no_gain")
			level = DCO_ELogLevel.WARN;
		DCO_Log.Write(DCO_ELogChannel.UNIT, level, DCO_Log.IdUnit(me), string.Format("rearm_end who=%1 result=%2 src=%3 n=%4 rounds=%5 ms=%6 src_blocked_s=%7",
			DCO_Log.IdUnit(me), result, DCO_SrcName(), m_iDCOTaken, m_iDCORounds, Math.Round(GetGame().GetWorld().GetWorldTime() - m_fDCOStart_ms), 300 * DCO_Util.B01(blocked)) + DCO_EndDiag(me));
	}

	protected string DCO_EndDiag(IEntity me)
	{
		string cur = "-";
		if (m_Utility && m_Utility.GetCurrentAction())
			cur = m_Utility.GetCurrentAction().ClassName();
		float d = -1;
		if (me && m_DCOSource)
			d = vector.DistanceXZ(me.GetOrigin(), m_DCOSource.GetOrigin());
		string moveState = "none";
		if (m_DCOMove)
			moveState = typename.EnumToString(EAIActionState, m_DCOMove.GetActionState());
		return string.Format(" cur=%1 d=%2 removes=%3 move_state=%4 mags_found=%5 wrong_well=%6 empty=%7 no_storage=%8 move_refused=%9", cur, Math.Round(d), m_iDCORemoves, moveState, m_iDCOMagsFound, m_iDCOWrongWell, m_iDCOEmpty, m_iDCONoStorage, m_iDCOMoveRefused);
	}

	override void OnActionCompleted()
	{
		super.OnActionCompleted();
		if (DCO_Util.Teardown("rearm_completed"))
			return;
		DCO_LogEnd("taken");
		if (m_DCOMove)
			m_DCOMove.Complete();
	}

	override void OnActionFailed()
	{
		super.OnActionFailed();
		if (DCO_Util.Teardown("rearm_failed"))
			return;
		DCO_LogEnd("cancel");
		if (m_DCOMove)
			m_DCOMove.Fail();
	}

	override string GetActionDebugInfo()
	{
		float d = -1;
		if (m_Utility && m_DCOSource)
			d = vector.DistanceXZ(m_Utility.GetOrigin(), m_DCOSource.GetOrigin());
		return string.Format("DCO rearm src=%1 dist=%2 spare=%3 prio=%4", DCO_SrcName(), Math.Round(d), m_iDCOSpare, m_fDCOPrio);
	}
}

class DCO_AIRearmTake : SCR_AIActionTask
{
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		DCO_TakeBehavior b = DCO_TakeBehavior.Cast(GetExecutedAction());
		if (!b)
			return ENodeResult.FAIL;
		if (b.DCO_DoTake() > 0)
			return ENodeResult.SUCCESS;
		return ENodeResult.FAIL;
	}

	static override bool VisibleInPalette() { return true; }
	static override string GetOnHoverDescription() { return "DCO rearm: takes magazines from the booked corpse (real items, rounds kept) or resupplies at the station."; }
}

modded class SCR_ArsenalComponent
{
	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		DCO_Rearm.RegisterStation(owner);
	}

	override protected void OnDelete(IEntity owner)
	{
		DCO_Rearm.UnregisterStation(owner);
		super.OnDelete(owner);
	}
}

modded class SCR_BaseSupportStationComponent
{
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if (SCR_ResupplySupportStationComponent.Cast(this))
			DCO_Rearm.RegisterStation(owner);
	}

	override void OnDelete(IEntity owner)
	{
		DCO_Rearm.UnregisterStation(owner);
		super.OnDelete(owner);
	}
}
