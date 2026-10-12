void DCO_EvObjOwnerFn(CMD_AICommanderObjectiveComponent objective, FactionKey fromFaction, FactionKey toFaction);
typedef func DCO_EvObjOwnerFn;

void DCO_EvObjContestFn(CMD_AICommanderObjectiveComponent objective, bool contested);
typedef func DCO_EvObjContestFn;

void DCO_EvCmdModeFn(AICommander_BaseComponent commander, CMD_ECommanderMode fromMode, CMD_ECommanderMode toMode);
typedef func DCO_EvCmdModeFn;

void DCO_EvCmdStockFn(AICommander_BaseComponent commander, string stockKind);
typedef func DCO_EvCmdStockFn;

void DCO_EvTextFn(string eventName, string detail);
typedef func DCO_EvTextFn;

void DCO_EvContactFn(SCR_AIGroup group, IEntity enemy, vector enemyPos);
typedef func DCO_EvContactFn;

void DCO_EvUnitStateFn(IEntity soldier, int fromState, int toState);
typedef func DCO_EvUnitStateFn;

void DCO_EvSquadTaskFn(AIGroup group, DCO_EGroupTask fromTask, DCO_EGroupTask toTask, string by);
typedef func DCO_EvSquadTaskFn;

void DCO_EvKilledFn(IEntity victim, IEntity killer, float distance);
typedef func DCO_EvKilledFn;

class DCO_Events
{
	protected static ref DCO_Events s_Instance;

	protected ref ScriptInvokerBase<DCO_EvObjOwnerFn> m_OnObjectiveOwner = new ScriptInvokerBase<DCO_EvObjOwnerFn>();
	protected ref ScriptInvokerBase<DCO_EvObjContestFn> m_OnObjectiveContested = new ScriptInvokerBase<DCO_EvObjContestFn>();
	protected ref ScriptInvokerBase<DCO_EvCmdModeFn> m_OnCommanderMode = new ScriptInvokerBase<DCO_EvCmdModeFn>();
	protected ref ScriptInvokerBase<DCO_EvCmdStockFn> m_OnCommanderStockEmpty = new ScriptInvokerBase<DCO_EvCmdStockFn>();
	protected ref ScriptInvokerBase<DCO_EvTextFn> m_OnOperation = new ScriptInvokerBase<DCO_EvTextFn>();
	protected ref ScriptInvokerBase<DCO_EvTextFn> m_OnSpawner = new ScriptInvokerBase<DCO_EvTextFn>();
	protected ref ScriptInvokerBase<DCO_EvTextFn> m_OnReinforcement = new ScriptInvokerBase<DCO_EvTextFn>();
	protected ref ScriptInvokerBase<DCO_EvTextFn> m_OnFireMission = new ScriptInvokerBase<DCO_EvTextFn>();
	protected ref ScriptInvokerBase<DCO_EvTextFn> m_OnAny = new ScriptInvokerBase<DCO_EvTextFn>();
	protected ref ScriptInvokerBase<DCO_EvTextFn> m_OnVictory = new ScriptInvokerBase<DCO_EvTextFn>();
	protected ref ScriptInvokerBase<DCO_EvContactFn> m_OnContact = new ScriptInvokerBase<DCO_EvContactFn>();
	protected ref ScriptInvokerBase<DCO_EvUnitStateFn> m_OnStress = new ScriptInvokerBase<DCO_EvUnitStateFn>();
	protected ref ScriptInvokerBase<DCO_EvUnitStateFn> m_OnMorale = new ScriptInvokerBase<DCO_EvUnitStateFn>();
	protected ref ScriptInvokerBase<DCO_EvSquadTaskFn> m_OnSquadTask = new ScriptInvokerBase<DCO_EvSquadTaskFn>();
	protected ref ScriptInvokerBase<DCO_EvKilledFn> m_OnKilled = new ScriptInvokerBase<DCO_EvKilledFn>();

	protected ref map<CMD_AICommanderObjectiveComponent, FactionKey> m_mOwner = new map<CMD_AICommanderObjectiveComponent, FactionKey>();
	protected ref map<CMD_AICommanderObjectiveComponent, bool> m_mContested = new map<CMD_AICommanderObjectiveComponent, bool>();
	protected ref map<AICommander_BaseComponent, int> m_mMode = new map<AICommander_BaseComponent, int>();
	protected ref map<AICommander_BaseComponent, int> m_mStock = new map<AICommander_BaseComponent, int>();
	protected ref map<AICommander_BaseComponent, int> m_mPoints = new map<AICommander_BaseComponent, int>();
	protected ref array<FactionKey> m_aKeys = {};
	protected ref array<float> m_aValues = {};
	protected float m_fAcc;
	protected bool m_bLoggedStart;

	static DCO_Events Get()
	{
		if (!s_Instance)
			s_Instance = new DCO_Events();
		return s_Instance;
	}

	ScriptInvokerBase<DCO_EvObjOwnerFn> GetOnObjectiveOwnerChanged()		{ return m_OnObjectiveOwner; }
	ScriptInvokerBase<DCO_EvObjContestFn> GetOnObjectiveContested()		{ return m_OnObjectiveContested; }
	ScriptInvokerBase<DCO_EvCmdModeFn> GetOnCommanderModeChanged()		{ return m_OnCommanderMode; }
	ScriptInvokerBase<DCO_EvCmdStockFn> GetOnCommanderStockEmpty()		{ return m_OnCommanderStockEmpty; }
	ScriptInvokerBase<DCO_EvTextFn> GetOnOperation()						{ return m_OnOperation; }
	ScriptInvokerBase<DCO_EvTextFn> GetOnSpawner()						{ return m_OnSpawner; }
	ScriptInvokerBase<DCO_EvTextFn> GetOnReinforcementRequest()			{ return m_OnReinforcement; }
	ScriptInvokerBase<DCO_EvTextFn> GetOnFireMission()					{ return m_OnFireMission; }
	ScriptInvokerBase<DCO_EvTextFn> GetOnAnyEvent()						{ return m_OnAny; }
	ScriptInvokerBase<DCO_EvTextFn> GetOnVictory()						{ return m_OnVictory; }
	ScriptInvokerBase<DCO_EvContactFn> GetOnContact()					{ return m_OnContact; }
	ScriptInvokerBase<DCO_EvUnitStateFn> GetOnStressChanged()			{ return m_OnStress; }
	ScriptInvokerBase<DCO_EvUnitStateFn> GetOnMoraleChanged()			{ return m_OnMorale; }
	ScriptInvokerBase<DCO_EvSquadTaskFn> GetOnSquadTaskChanged()		{ return m_OnSquadTask; }
	ScriptInvokerBase<DCO_EvKilledFn> GetOnSoldierKilled()				{ return m_OnKilled; }

	static void Contact(SCR_AIGroup grp, SCR_AITargetInfo t)
	{
		if (!grp || !t || !Replication.IsServer())
			return;
		DCO_Perf.Count("evt:contact");
		float d = -1;
		IEntity leader = grp.GetLeaderEntity();
		if (leader)
			d = vector.Distance(leader.GetOrigin(), t.m_vWorldPos);
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(grp), string.Format("event name=contact grp=%1 enemy=%2 dist=%3 kind=%4", DCO_Log.IdGroup(grp), DCO_Log.IdUnit(t.m_Entity), Math.Round(d), typename.EnumToString(EAITargetInfoCategory, t.m_eCategory)));
		Get().m_OnContact.Invoke(grp, t.m_Entity, t.m_vWorldPos);
	}

	static void Stress(IEntity soldier, int fromLvl, int toLvl)
	{
		if (!soldier || !Replication.IsServer())
			return;
		DCO_Perf.Count("evt:stress");
		Get().m_OnStress.Invoke(soldier, fromLvl, toLvl);
	}

	static void Morale(IEntity soldier, int fromState, int toState)
	{
		if (!soldier || !Replication.IsServer())
			return;
		DCO_Perf.Count("evt:morale");
		Get().m_OnMorale.Invoke(soldier, fromState, toState);
	}

	static void SquadTask(AIGroup grp, DCO_EGroupTask fromTask, DCO_EGroupTask toTask, string by)
	{
		if (!grp || !Replication.IsServer())
			return;
		DCO_Perf.Count("evt:squad_task");
		Get().m_OnSquadTask.Invoke(grp, fromTask, toTask, by);
	}

	static void Killed(IEntity victim)
	{
		if (!victim || !Replication.IsServer())
			return;
		IEntity killer;
		DamageManagerComponent dm = DamageManagerComponent.Cast(victim.FindComponent(DamageManagerComponent));
		if (dm && dm.GetInstigator())
			killer = dm.GetInstigator().GetInstigatorEntity();
		if (killer == victim)
			killer = null;
		float d = -1;
		if (killer)
			d = vector.Distance(victim.GetOrigin(), killer.GetOrigin());
		DCO_Perf.Count("evt:soldier_killed");
		DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(victim), string.Format("event name=soldier_killed unit=%1 killer=%2 dist=%3", DCO_Log.IdUnit(victim), DCO_Log.IdUnit(killer), Math.Round(d)));
		Get().m_OnKilled.Invoke(victim, killer, d);
	}

	static string FirstWord(string s)
	{
		int sp = s.IndexOf(" ");
		if (sp < 0)
			return s;
		return s.Substring(0, sp);
	}

	protected void Note(string name, string detail)
	{
		DCO_Perf.Count("evt:" + name);
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("event name=%1 %2", name, detail));
		m_OnAny.Invoke(name, detail);
	}

	static void Operation(string line)
	{
		if (!Replication.IsServer())
			return;
		DCO_Events e = Get();
		string name = FirstWord(line);
		e.m_OnOperation.Invoke(name, line);
		e.m_OnAny.Invoke(name, line);
		DCO_Perf.Count("evt:" + name);
	}

	static void Spawner(string name, string detail)
	{
		if (!Replication.IsServer())
			return;
		DCO_Events e = Get();
		e.m_OnSpawner.Invoke(name, detail);
		e.Note(name, detail);
	}

	static void Reinforcement(string detail)
	{
		if (!Replication.IsServer())
			return;
		DCO_Events e = Get();
		e.m_OnReinforcement.Invoke("reinf_request", detail);
		e.Note("reinf_request", detail);
	}

	static void FireMission(string detail)
	{
		if (!Replication.IsServer())
			return;
		DCO_Events e = Get();
		e.m_OnFireMission.Invoke("fire_mission", detail);
		e.Note("fire_mission", detail);
	}

	static void Victory(FactionKey winner, string detail)
	{
		if (!Replication.IsServer())
			return;
		DCO_Events e = Get();
		e.m_OnVictory.Invoke("victory_end", detail);
		e.m_OnAny.Invoke("victory_end", detail);
		DCO_Perf.Count("evt:victory_end");
	}

	void Tick(AICommander_ManagerComponent mgr, float timeSlice)
	{
		m_fAcc += timeSlice;
		if (m_fAcc < 1.0)
			return;
		m_fAcc = 0;
		if (!mgr || !Replication.IsServer())
			return;
		if (!m_bLoggedStart)
		{
			m_bLoggedStart = true;
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("event_api_ready objectives=%1 commanders=%2", mgr.m_aObjective.Count(), mgr.m_aCommander.Count()));
		}
		PollObjectives(mgr);
		PollCommanders(mgr);
	}

	protected void PollObjectives(AICommander_ManagerComponent mgr)
	{
		foreach (CMD_AICommanderObjectiveComponent obj : mgr.m_aObjective)
		{
			if (!obj)
				continue;
			FactionKey owner = obj.GetOwnerFaction();
			FactionKey prev;
			if (m_mOwner.Find(obj, prev) && prev != owner)
			{
				m_OnObjectiveOwner.Invoke(obj, prev, owner);
				Note("objective_owner", string.Format("obj=%1 from=%2 to=%3", obj.GetLogName(), NoneIf(prev), NoneIf(owner)));
			}
			m_mOwner.Set(obj, owner);

			obj.GetControlSnapshot(m_aKeys, m_aValues);
			int present;
			foreach (float v : m_aValues)
			{
				if (v > 0)
					present++;
			}
			bool contested = present >= 2;
			bool was;
			bool known = m_mContested.Find(obj, was);
			if (known && was != contested)
			{
				m_OnObjectiveContested.Invoke(obj, contested);
				Note("objective_contested", string.Format("obj=%1 contested=%2 owner=%3", obj.GetLogName(), DCO_Util.B01(contested), NoneIf(owner)));
			}
			m_mContested.Set(obj, contested);
		}
	}

	protected void PollCommanders(AICommander_ManagerComponent mgr)
	{
		foreach (AICommander_BaseComponent cmd : mgr.m_aCommander)
		{
			if (!cmd)
				continue;
			int mode = cmd.GetCommanderMode();
			int prevMode;
			if (m_mMode.Find(cmd, prevMode) && prevMode != mode)
			{
				m_OnCommanderMode.Invoke(cmd, prevMode, mode);
				Note("commander_mode", string.Format("cmd=%1 from=%2 to=%3", cmd.GetCommanderUID(), typename.EnumToString(CMD_ECommanderMode, prevMode), typename.EnumToString(CMD_ECommanderMode, mode)));
			}
			m_mMode.Set(cmd, mode);

			int stock = cmd.GetReinforcementStock();
			int prevStock;
			if (m_mStock.Find(cmd, prevStock) && prevStock > 0 && stock == 0)
			{
				m_OnCommanderStockEmpty.Invoke(cmd, "soldiers");
				Note("commander_stock_empty", string.Format("cmd=%1 kind=soldiers start=%2", cmd.GetCommanderUID(), cmd.GetStockStart()));
			}
			m_mStock.Set(cmd, stock);

			int pts = cmd.GetPointsStock();
			int prevPts;
			if (m_mPoints.Find(cmd, prevPts) && prevPts > 0 && pts == 0)
			{
				m_OnCommanderStockEmpty.Invoke(cmd, "points");
				Note("commander_stock_empty", string.Format("cmd=%1 kind=points", cmd.GetCommanderUID()));
			}
			m_mPoints.Set(cmd, pts);
		}
	}

	protected static string NoneIf(string s)
	{
		if (s.IsEmpty())
			return "none";
		return s;
	}
}

modded class SCR_AIGroupPerception
{
	protected bool m_bDCOContact;

	override void Update()
	{
		super.Update();
		bool now = !m_aTargets.IsEmpty();
		if (now && !m_bDCOContact)
			DCO_Events.Contact(m_Group, m_aTargets[m_aTargets.Count() - 1]);
		m_bDCOContact = now;
	}
}

modded class AICommander_ManagerComponent
{
	DCO_Events GetEvents()
	{
		return DCO_Events.Get();
	}

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		super.EOnFrame(owner, timeSlice);
		DCO_Events.Get().Tick(this, timeSlice);
	}
}
