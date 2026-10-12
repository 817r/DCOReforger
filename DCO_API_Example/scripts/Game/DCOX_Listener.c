class DCOX_Listener : Managed
{
	protected const string MOD = "DCO_API_Example";
	protected int m_iContact;
	protected int m_iStress;
	protected int m_iMorale;
	protected int m_iPanic;
	protected int m_iTask;
	protected int m_iKilled;
	protected bool m_bSoldierTested;

	void Start()
	{
		if (!Replication.IsServer())
			return;
		PrintFormat("[DCOX] DCO API version=%1 ready=%2", DCO_API.Version(), DCO_API.IsReady());

		DCO_Events ev = DCO_API.Events();
		ev.GetOnContact().Insert(OnContact);
		ev.GetOnStressChanged().Insert(OnStress);
		ev.GetOnMoraleChanged().Insert(OnMorale);
		ev.GetOnSquadTaskChanged().Insert(OnSquadTask);
		ev.GetOnSoldierKilled().Insert(OnKilled);

		bool ok1 = DCO_API.SetSetting(MOD, "aiAggression", 1.2);
		bool ok2 = DCO_API.SetSetting(MOD, "hear.shot_err", 1.5);
		bool ok3 = DCO_API.SetSetting(MOD, "hear.shot_err_typo", 1.0);
		bool ok4 = DCO_API.SetSetting(MOD, "tune_pc_night_min", 99.0);
		PrintFormat("[DCOX] set aiAggression=%1 (%2) hear.shot_err=%3 (%4) typo_rejected=%5 clamp pc.night_min=%6 (%7) known=%8",
			ok1, DCO_API.GetSetting("aiAggression"), ok2, DCO_API.GetSetting("hear.shot_err"), !ok3, ok4, DCO_API.GetSetting("pc.night_min"), DCO_API.HasSetting("ctx_on"));

		GetGame().GetCallqueue().CallLater(Report, 30000, true);
	}

	void Stop()
	{
		DCO_Events ev = DCO_API.Events();
		ev.GetOnContact().Remove(OnContact);
		ev.GetOnStressChanged().Remove(OnStress);
		ev.GetOnMoraleChanged().Remove(OnMorale);
		ev.GetOnSquadTaskChanged().Remove(OnSquadTask);
		ev.GetOnSoldierKilled().Remove(OnKilled);
		GetGame().GetCallqueue().Remove(Report);
	}

	protected void OnContact(SCR_AIGroup group, IEntity enemy, vector enemyPos)
	{
		m_iContact++;
		if (m_iContact <= 3)
			PrintFormat("[DCOX] contact group=%1 task=%2 alive=%3/%4 morale=%5 state=%6", group, typename.EnumToString(DCO_EGroupTask, DCO_API.SquadTask(group)), DCO_API.SquadAlive(group), DCO_API.SquadPeak(group), DCO_API.SquadMorale(group), DCO_API.SquadState(group));
		if (!m_bSoldierTested && group && group.GetLeaderEntity())
		{
			m_bSoldierTested = true;
			IEntity s = group.GetLeaderEntity();
			bool ok = DCO_API.SetSoldierSetting(MOD, s, "aiPerception", 1.5);
			PrintFormat("[DCOX] soldier set aiPerception ok=%1 now=%2 skill=%3 morale=%4 stress=%5 supp=%6 cover=%7 ammo=%8",
				ok, DCO_API.GetSoldierSetting(s, "aiPerception"), DCO_API.GetSoldierSetting(s, "aiSkill"), DCO_API.SoldierMorale(s), DCO_API.SoldierStress(s), DCO_API.SoldierSuppression(s), DCO_API.SoldierInCover(s), DCO_API.SoldierAmmo(s));
			int flags = DCO_API.GetSquadSetting(group, "vehicleExit");
			bool okSq = DCO_API.SetSquadSetting(MOD, group, "vehicleExit", 16);
			PrintFormat("[DCOX] squad vehicleExit before=%1 set_ok=%2 after=%3", flags, okSq, DCO_API.GetSquadSetting(group, "vehicleExit"));
		}
	}

	protected void OnStress(IEntity soldier, int fromLvl, int toLvl)
	{
		m_iStress++;
	}

	protected void OnMorale(IEntity soldier, int fromState, int toState)
	{
		m_iMorale++;
		if (toState == moraleState.PANIC)
			m_iPanic++;
	}

	protected void OnSquadTask(AIGroup group, DCO_EGroupTask fromTask, DCO_EGroupTask toTask, string by)
	{
		m_iTask++;
		if (m_iTask <= 3)
			PrintFormat("[DCOX] squad task %1 -> %2 by=%3 commander=%4", typename.EnumToString(DCO_EGroupTask, fromTask), typename.EnumToString(DCO_EGroupTask, toTask), by, DCO_API.SquadCommander(group));
	}

	protected void OnKilled(IEntity victim, IEntity killer, float distance)
	{
		m_iKilled++;
		if (m_iKilled <= 3)
			PrintFormat("[DCOX] killed victim=%1 killer=%2 dist=%3", victim, killer, distance);
	}

	protected void Report()
	{
		array<AICommander_BaseComponent> cmds = {};
		array<CMD_AICommanderObjectiveComponent> objs = {};
		PrintFormat("[DCOX] totals contact=%1 stress=%2 morale=%3 panic=%4 task=%5 killed=%6 commanders=%7 objectives=%8",
			m_iContact, m_iStress, m_iMorale, m_iPanic, m_iTask, m_iKilled, DCO_API.Commanders(cmds), DCO_API.Objectives(objs));
	}
}

modded class SCR_BaseGameMode
{
	protected ref DCOX_Listener m_DCOXListener;

	protected override void OnGameStart()
	{
		super.OnGameStart();
		m_DCOXListener = new DCOX_Listener();
		m_DCOXListener.Start();
	}

	override void OnGameEnd()
	{
		if (m_DCOXListener)
			m_DCOXListener.Stop();
		super.OnGameEnd();
	}
}
