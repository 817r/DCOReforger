#define DCO_GAME_19

// One soldier of a group that went dormant. Vanilla re-materialises a dormant group from the
// prefab's m_aUnitPrefabSlots, but DCO builds most groups by hand (Group_Base has no slots) and
// moves soldiers between groups, so the snapshot keeps who was really there.
class DCO_DormantMember
{
	ResourceName m_sPrefab;
	int m_iSkill = -1;
	float m_fMoraleMul = 1.0;
	float m_fMoraleBase;
	bool m_bMorale;
	int m_iMags = -1;
	float m_fHp = 1.0;
}

class DCO_Dormant
{
	protected static ref map<AIGroup, bool> s_mSeen;

	static bool Is(AIGroup grp)
	{
		if (!grp)
			return false;
#ifdef DCO_GAME_19
		SCR_AIGroup sg = SCR_AIGroup.Cast(grp);
		if (!sg)
			return false;
		bool dormant = sg.IsDormant();
		Note(sg, dormant);
		return dormant;
#endif
		return false;
	}

	static int Count(AIGroup grp)
	{
		if (!grp)
			return 0;
#ifdef DCO_GAME_19
		SCR_AIGroup sg = SCR_AIGroup.Cast(grp);
		if (sg && sg.IsDormant() && sg.GetAgentsCount() == 0)
		{
			if (sg.DCO_FixPhantom())
				return 0;
			Note(sg, true);
			int n = sg.GetDormantAliveCount();
			if (n >= 0)
			{
				DCO_Perf.Count("dormant:count");
				return n;
			}
		}
		else if (sg)
			Note(sg, false);
#endif
		return grp.GetAgentsCount();
	}

	static bool WorldEnabled()
	{
#ifdef DCO_GAME_19
		ChimeraAIWorld aiWorld = ChimeraAIWorld.Cast(GetGame().GetAIWorld());
		return aiWorld && aiWorld.IsGroupDormancyEnabled();
#endif
		return false;
	}

	static int AliveCount(AIGroup grp)
	{
#ifdef DCO_GAME_19
		SCR_AIGroup sg = SCR_AIGroup.Cast(grp);
		if (sg)
			return sg.GetDormantAliveCount();
#endif
		return 0;
	}

	static bool SetProximity(AIGroup grp, float spawnM, float despawnM)
	{
#ifdef DCO_GAME_19
		SCR_AIGroup sg = SCR_AIGroup.Cast(grp);
		if (!sg)
			return false;
		sg.SetLifecyclePolicy(SCR_EAIGroupLifecyclePolicy.ProximityDriven, spawnM, despawnM);
		return true;
#endif
		return false;
	}

	// Hands the group back to Manual. Leaving ProximityDriven clears the group entity's FRAME
	// mask, so the caller must re-assert any component FRAME it relies on.
	static void Release(AIGroup grp)
	{
#ifdef DCO_GAME_19
		SCR_AIGroup sg = SCR_AIGroup.Cast(grp);
		if (sg)
			sg.SetLifecyclePolicy(SCR_EAIGroupLifecyclePolicy.Manual);
#endif
	}

	// Brings a dormant group back through the spawn queue (budget-aware, one member per queue
	// tick). Goes to EnqueueSpawnRequest directly instead of RequestSpawn, which would re-add the
	// prefab's static and spawned waypoints on top of the commander's orders.
	static bool Wake(AIGroup grp, string why)
	{
#ifdef DCO_GAME_19
		SCR_AIGroup sg = SCR_AIGroup.Cast(grp);
		if (!sg || sg.GetAgentsCount() > 0 || !sg.IsDormant())
			return false;
		if (sg.DCO_FixPhantom())
			return false;
		int want = sg.GetDormantAliveCount();
		if (sg.DCO_SnapCount() > 0)
			want = sg.DCO_SnapCount();
		if (want <= 0)
			return false;
		ChimeraAIWorld aiWorld = ChimeraAIWorld.Cast(GetGame().GetAIWorld());
		if (aiWorld)
			aiWorld.EnqueueSpawnRequest(sg, want, 0);
		else
			sg.SpawnMembers();
		DCO_Perf.Count("dormant:wake_" + why);
		DCO_PerfHub.s_iWakes++;
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(sg), string.Format("dormant_wake grp=%1 why=%2 want=%3 snap=%4 importance=%5", DCO_Log.IdGroup(sg), why, want, sg.DCO_SnapCount(), sg.GetImportance()));
		GetGame().GetCallqueue().CallLater(CheckWoke, 20000, false, sg, why);
		return true;
#endif
		return false;
	}

#ifdef DCO_GAME_19
	protected static void CheckWoke(SCR_AIGroup sg, string why)
	{
		if (!sg || sg.GetAgentsCount() > 0 || !sg.IsDormant())
			return;
		DCO_Perf.Count("dormant:wake_stuck");
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.WARN, DCO_Log.IdGroup(sg), string.Format("dormant_wake_stuck grp=%1 why=%2 alive=%3 snap=%4", DCO_Log.IdGroup(sg), why, sg.GetDormantAliveCount(), sg.DCO_SnapCount()));
	}
#endif

	static bool IsProximity(AIGroup grp)
	{
#ifdef DCO_GAME_19
		SCR_AIGroup sg = SCR_AIGroup.Cast(grp);
		return sg && sg.GetLifecyclePolicy() == SCR_EAIGroupLifecyclePolicy.ProximityDriven;
#endif
		return false;
	}

	// Soldiers recorded for a sleeping group (0 when awake or nothing recorded).
	static int SnapCount(AIGroup grp)
	{
#ifdef DCO_GAME_19
		SCR_AIGroup sg = SCR_AIGroup.Cast(grp);
		if (sg)
			return sg.DCO_SnapCount();
#endif
		return 0;
	}

	// World time (s) the group last came back from sleep; far in the past if never.
	static float WokeAt(AIGroup grp)
	{
#ifdef DCO_GAME_19
		SCR_AIGroup sg = SCR_AIGroup.Cast(grp);
		if (sg)
			return sg.DCO_WokeAt();
#endif
		return -100000;
	}

	// Off-screen battle losses: removes soldiers from a sleeping group's record.
	static int PaperLose(AIGroup grp, int n)
	{
#ifdef DCO_GAME_19
		SCR_AIGroup sg = SCR_AIGroup.Cast(grp);
		if (sg)
			return sg.DCO_PaperLose(n);
#endif
		return 0;
	}

	// A sleeping group wiped out off-screen: no soldiers left to bring back.
	static void Eliminate(AIGroup grp)
	{
#ifdef DCO_GAME_19
		SCR_AIGroup sg = SCR_AIGroup.Cast(grp);
		if (sg)
			sg.DCO_Eliminate();
#endif
	}

	static DCO_DormantMember Describe(AIAgent a, IEntity ch)
	{
		DCO_DormantMember m = new DCO_DormantMember();
		m.m_sPrefab = ch.GetPrefabData().GetPrefabName();
		DCO_AIConfigComponent conf = DCO_AIConfigComponent.Cast(a.FindComponent(DCO_AIConfigComponent));
		if (conf)
			m.m_iSkill = conf.GetAISkill();
		SCR_AIUtilityComponent u = SCR_AIUtilityComponent.Cast(a.FindComponent(SCR_AIUtilityComponent));
		if (u && u.GetMoraleSystem())
		{
			m.m_bMorale = true;
			m.m_fMoraleMul = u.GetMoraleSystem().DCO_GetMoraleMul();
			m.m_fMoraleBase = u.GetMoraleSystem().DCO_GetMoraleBase();
		}
		array<IEntity> mags = {};
		m.m_iMags = SpareMags(ch, mags);
		SCR_CharacterDamageManagerComponent dm = SCR_CharacterDamageManagerComponent.Cast(ch.FindComponent(SCR_CharacterDamageManagerComponent));
		if (dm)
			m.m_fHp = dm.GetHealthScaled();
		return m;
	}

	// Puts back what the spawner and the fight had changed: skill, morale level and how many
	// spare magazines the soldier still carried (fresh prefabs come with a full load).
	static void Restore(AIAgent a, IEntity ch, DCO_DormantMember m)
	{
		DCO_AIConfigComponent conf = DCO_AIConfigComponent.Cast(a.FindComponent(DCO_AIConfigComponent));
		if (conf && m.m_iSkill >= 0)
			conf.SetAISkill(m.m_iSkill);
		SCR_AIUtilityComponent u = SCR_AIUtilityComponent.Cast(a.FindComponent(SCR_AIUtilityComponent));
		if (m.m_bMorale && u && u.GetMoraleSystem())
			u.GetMoraleSystem().DCO_SetMoraleLevel(m.m_fMoraleMul, m.m_fMoraleBase);
		if (m.m_iMags >= 0)
			TrimMags(ch, m.m_iMags);
	}

	// Spare (not loaded) magazines, same filter as the spawner's reduced-shape rule.
	static int SpareMags(IEntity ch, notnull array<IEntity> mags)
	{
		mags.Clear();
		SCR_InventoryStorageManagerComponent inv = SCR_InventoryStorageManagerComponent.Cast(ch.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!inv)
			return -1;
		array<IEntity> items = {};
		inv.GetItems(items);
		foreach (IEntity it : items)
		{
			if (!it || !it.FindComponent(MagazineComponent))
				continue;
			if (it.GetParent() && it.GetParent().FindComponent(BaseWeaponComponent))
				continue;
			string n;
			if (it.GetPrefabData())
				n = it.GetPrefabData().GetPrefabName();
			n.ToLower();
			if (n.Contains("rocket") || n.Contains("grenade") || n.Contains("smoke") || n.Contains("flare"))
				continue;
			mags.Insert(it);
		}
		return mags.Count();
	}

	static int TrimMags(IEntity ch, int keep)
	{
		array<IEntity> mags = {};
		int have = SpareMags(ch, mags);
		if (have <= keep)
			return 0;
		SCR_InventoryStorageManagerComponent inv = SCR_InventoryStorageManagerComponent.Cast(ch.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!inv)
			return 0;
		int removed;
		for (int i = 0; i < have - keep; i++)
		{
			if (inv.TryDeleteItem(mags[i]))
				removed++;
		}
		return removed;
	}

#ifdef DCO_GAME_19
	protected static void Note(SCR_AIGroup sg, bool dormant)
	{
		if (!s_mSeen)
			s_mSeen = new map<AIGroup, bool>();
		bool was;
		bool known = s_mSeen.Find(sg, was);
		if (known && was == dormant)
			return;
		if (!known && !dormant)
			return;
		if (s_mSeen.Count() > 256)
			s_mSeen.Clear();
		s_mSeen.Set(sg, dormant);
		string state = "active";
		if (dormant)
			state = "dormant";
		DCO_Perf.Count("dormant:" + state);
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(sg), string.Format("grp_dormant grp=%1 state=%2 alive=%3 dead=%4 agents=%5", DCO_Log.IdGroup(sg), state, sg.GetDormantAliveCount(), sg.GetDormantDeadCount(), sg.GetAgentsCount()));
	}
#endif
}

#ifdef DCO_GAME_19
// Every dormant transition (DCO's perf.dormant policy, vanilla ProximityDriven lifecycle and the
// engine's budget eviction through DespawnMembersFromC) goes through DespawnMembers, and every
// re-materialisation through ExpandOneMember (spawn queue) or SpawnMembers. Snapshotting here
// covers all of them while keeping the queue, the budget and the editor notifications.
modded class SCR_AIGroup
{
	protected ref array<ref DCO_DormantMember> m_aDCOSnap;
	protected ref array<ref DCO_DormantMember> m_aDCOApply;
	protected float m_fDCOWokeAt_s = -100000;

	int DCO_SnapCount()
	{
		if (!m_aDCOSnap)
			return 0;
		return m_aDCOSnap.Count();
	}

	float DCO_WokeAt()
	{
		return m_fDCOWokeAt_s;
	}

	// Trims the record from the back (the leader is first and goes last) and keeps the engine's
	// dormant counts in step, so Count, Wake and the spawn queue all see the same strength.
	int DCO_PaperLose(int n)
	{
		if (!m_aDCOSnap || n <= 0 || !IsDormant() || GetAgentsCount() > 0)
			return 0;
		int lost;
		while (lost < n && !m_aDCOSnap.IsEmpty())
		{
			m_aDCOSnap.Remove(m_aDCOSnap.Count() - 1);
			lost++;
		}
		SetDormantCounts(m_aDCOSnap.Count(), GetDormantDeadCount() + lost);
		return lost;
	}

	void DCO_Eliminate()
	{
		if (!Replication.IsServer() || GetAgentsCount() > 0)
			return;
		int dead = GetDormantDeadCount() + DCO_SnapCount();
		// Keep an empty (not null) record so DCO_FixPhantom leaves the counts alone.
		m_aDCOSnap = {};
		SetDormantCounts(0, dead);
		ChimeraAIWorld aiWorld = ChimeraAIWorld.Cast(GetGame().GetAIWorld());
		if (aiWorld)
			aiWorld.PurgeSpawnRequestsForGroup(this);
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(this), string.Format("dormant_eliminated grp=%1 dead=%2 delete=%3", DCO_Log.IdGroup(this), dead, DCO_Util.B01(m_bDeleteWhenEmpty)));
		if (m_bDeleteWhenEmpty)
			GetGame().GetCallqueue().CallLater(SCR_EntityHelper.DeleteEntityAndChildren, 1, false, this);
	}

	// Groups whose members the prefab slots can't describe: no slots at all (Group_Base, which
	// the DCO spawner uses) or a group DCO commands (soldiers get merged and moved between groups).
	protected bool DCO_WantSnap()
	{
		if (!m_aUnitPrefabSlots || m_aUnitPrefabSlots.IsEmpty())
			return true;
		DCO_GroupUtilityComponent gu = DCO_GroupUtilityComponent.Cast(FindComponent(DCO_GroupUtilityComponent));
		return gu && gu.GetMyCommander();
	}

	override void DespawnMembers()
	{
		if (Replication.IsServer() && GetAgentsCount() > 0)
		{
			if (DCO_WantSnap())
				DCO_TakeSnap();
			// The soldiers are about to be deleted, not killed: let the group's garrison free
			// their building slots now, or it would count every one of them as a casualty.
			DCO_GroupUtilityComponent gu = DCO_GroupUtilityComponent.Cast(FindComponent(DCO_GroupUtilityComponent));
			if (gu)
				gu.DCO_OnDespawning();
		}
		super.DespawnMembers();
	}

	protected void DCO_TakeSnap()
	{
		array<AIAgent> agents = {};
		GetAgents(agents);
		IEntity leader = GetLeaderEntity();
		array<ref DCO_DormantMember> snap = {};
		int inVeh;
		float hp;
		foreach (AIAgent a : agents)
		{
			if (!a)
				continue;
			IEntity ch = a.GetControlledEntity();
			if (!ch || !ch.GetPrefabData())
				continue;
			DCO_DormantMember m = DCO_Dormant.Describe(a, ch);
			if (ch == leader)
				snap.InsertAt(m, 0);
			else
				snap.Insert(m);
			hp += m.m_fHp;
			ChimeraCharacter cc = ChimeraCharacter.Cast(ch);
			if (cc && cc.IsInVehicle())
				inVeh++;
		}
		if (snap.IsEmpty())
		{
			m_aDCOSnap = null;
			return;
		}
		m_aDCOSnap = snap;
		DCO_PerfHub.s_iSleeps++;
		int slots;
		if (m_aUnitPrefabSlots)
			slots = m_aUnitPrefabSlots.Count();
		DCO_Perf.Count("dormant:snap");
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(this), string.Format("dormant_snap grp=%1 members=%2 agents=%3 slots=%4 in_vehicle=%5 hp_avg=%6 policy=%7 importance=%8",
			DCO_Log.IdGroup(this), snap.Count(), agents.Count(), slots, inVeh, (hp / snap.Count()).ToString(-1, 2), typename.EnumToString(SCR_EAIGroupLifecyclePolicy, GetLifecyclePolicy()), GetImportance()));
	}

	override bool ExpandOneMember()
	{
		if (!m_aDCOSnap)
			return super.ExpandOneMember();
		int alive = GetAgentsCount();
		int total = m_aDCOSnap.Count();
		if (alive >= total)
		{
			DCO_SnapDone("queue");
			return false;
		}
		if (!SpawnGroupMember(false, alive, m_aDCOSnap[alive].m_sPrefab, false, false))
			return false;
		if (GetAgentsCount() >= total)
		{
			if (Event_OnAllDelayedEntitySpawned)
				Event_OnAllDelayedEntitySpawned.Invoke(this);
			InvokeEventOnInit();
			DCO_SnapDone("queue");
		}
		return true;
	}

	override bool IsExpandComplete()
	{
		if (!m_aDCOSnap)
			return super.IsExpandComplete();
		if (GetAgentsCount() < m_aDCOSnap.Count())
			return false;
		DCO_SnapDone("complete");
		return true;
	}

	override void SpawnMembers()
	{
		if (!m_aDCOSnap || GetAgentsCount() > 0)
		{
			super.SpawnMembers();
			return;
		}
		// Synchronous like vanilla (the engine may call this from the spawn queue itself).
		// A member that has to wait for its navmesh tile is left to the queue.
		// Fixed index: agent registration may lag the spawn, so re-reading the agent count inside
		// the loop could pick the same slot (the leader) every pass.
		int total = m_aDCOSnap.Count();
		for (int idx = GetAgentsCount(); idx < total; idx++)
		{
			if (!SpawnGroupMember(false, idx, m_aDCOSnap[idx].m_sPrefab, false, idx == total - 1))
				break;
		}
		if (GetAgentsCount() >= total)
		{
			DCO_SnapDone("sync");
			return;
		}
		ChimeraAIWorld aiWorld = ChimeraAIWorld.Cast(GetGame().GetAIWorld());
		if (aiWorld)
			aiWorld.EnqueueSpawnRequest(this, total - GetAgentsCount(), 0);
	}

	protected void DCO_SnapDone(string src)
	{
		if (!m_aDCOSnap)
			return;
		m_aDCOApply = m_aDCOSnap;
		m_aDCOSnap = null;
		m_fDCOWokeAt_s = DCO_Util.NowS();
		// -1 alive = "not dormant". Without this a group later wiped out in combat keeps
		// reporting its old dormant count and is never deleted (OnEmpty skips dormant groups).
		// The dead count is kept so vanilla never tops a slot-based group back up with the dead.
		SetDormantCounts(-1, GetDormantDeadCount());
		GetGame().GetCallqueue().CallLater(DCO_ApplySnap, 1500, false);
		SCR_EditableGroupComponent editable = SCR_EditableGroupComponent.Cast(FindComponent(SCR_EditableGroupComponent));
		if (editable)
			editable.OnDormantStateChanged();
		DCO_Perf.Count("dormant:restore_" + src);
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(this), string.Format("dormant_restore grp=%1 members=%2 snap=%3 src=%4 alive_count_now=%5 dormant_now=%6", DCO_Log.IdGroup(this), GetAgentsCount(), m_aDCOApply.Count(), src, GetDormantAliveCount(), DCO_Util.B01(IsDormant())));
	}

	protected void DCO_ApplySnap()
	{
		if (!m_aDCOApply)
			return;
		array<AIAgent> agents = {};
		GetAgents(agents);
		array<bool> used = {};
		foreach (DCO_DormantMember unused : m_aDCOApply)
			used.Insert(false);
		int applied;
		foreach (AIAgent a : agents)
		{
			if (!a)
				continue;
			IEntity ch = a.GetControlledEntity();
			if (!ch || !ch.GetPrefabData())
				continue;
			ResourceName pf = ch.GetPrefabData().GetPrefabName();
			for (int i = 0; i < m_aDCOApply.Count(); i++)
			{
				if (used[i] || m_aDCOApply[i].m_sPrefab != pf)
					continue;
				used[i] = true;
				DCO_Dormant.Restore(a, ch, m_aDCOApply[i]);
				applied++;
				break;
			}
		}
		DCO_Perf.Count("dormant:restore_state");
		// Garrison/defend groups: the garrison activity has to survive the sleep and take the
		// respawned soldiers as new free agents. garrison=0 on a DEFEND/GARRISON group means it did not.
		string task = "-";
		bool garrison;
		DCO_GroupUtilityComponent gu = DCO_GroupUtilityComponent.Cast(FindComponent(DCO_GroupUtilityComponent));
		if (gu)
		{
			task = typename.EnumToString(DCO_EGroupTask, gu.GetTask());
			garrison = gu.GetGarrisonActivity() != null;
		}
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(this), string.Format("dormant_restore_state grp=%1 applied=%2 snap=%3 task=%4 garrison=%5 pos=%6", DCO_Log.IdGroup(this), applied, m_aDCOApply.Count(), task, DCO_Util.B01(garrison), GetOrigin()));
		m_aDCOApply = null;
	}

	// A dormant group that can't come back: nothing recorded (e.g. restored from a save while
	// dormant) and no prefab slots to fall back on. Clear it so it stops reporting phantom
	// soldiers, and delete it if it would have been deleted when empty.
	bool DCO_FixPhantom()
	{
		if (!Replication.IsServer() || !IsDormant() || GetAgentsCount() > 0 || m_aDCOSnap)
			return false;
		if (m_aUnitPrefabSlots && !m_aUnitPrefabSlots.IsEmpty())
			return false;
		int alive = GetDormantAliveCount();
		SetDormantCounts(-1, 0);
		DCO_Perf.Count("dormant:phantom");
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.WARN, DCO_Log.IdGroup(this), string.Format("dormant_phantom grp=%1 alive_was=%2 delete=%3", DCO_Log.IdGroup(this), alive, DCO_Util.B01(m_bDeleteWhenEmpty)));
		if (m_bDeleteWhenEmpty)
			GetGame().GetCallqueue().CallLater(SCR_EntityHelper.DeleteEntityAndChildren, 1, false, this);
		return true;
	}
}
#endif
