modded class AICommander_BaseComponent
{
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		 m_fThinkTimer += timeSlice;
		 m_fCaptureCheckTimer += timeSlice;

		UpdateCommanderDebug(timeSlice);

		if (Replication.IsServer())
		{
			int pt = DCO_Perf.Begin();
			m_Logistics.Update(this, timeSlice);
			DCO_Perf.End("cmd_logistics", pt);
			pt = DCO_Perf.Begin();
			m_Ops.Tick(this, timeSlice);
			DCO_Perf.End("cmd_ops", pt);
			pt = DCO_Perf.Begin();
			m_Defense.Tick(this, timeSlice);
			DCO_Perf.End("cmd_defense", pt);
			pt = DCO_Perf.Begin();
			m_Tactics.Tick(this, timeSlice);
			DCO_Perf.End("cmd_tactics", pt);
			pt = DCO_Perf.Begin();
			m_DefTactics.Tick(this, timeSlice);
			DCO_Perf.End("cmd_def_tactics", pt);
			pt = DCO_Perf.Begin();
			m_DoctrineRt.Tick(this, timeSlice);
			DCO_Perf.End("cmd_doctrine", pt);
			pt = DCO_Perf.Begin();
			DCO_Reinf().Tick(this);
			DCO_Perf.End("cmd_reinf", pt);
		}

		if (m_fThinkTimer >= m_fThinkInterval)
		{
			m_fThinkTimer = 0.0;
			float worldTime = DCO_Util.NowS();
			int tt = DCO_Perf.Begin();
			Think(worldTime);
			DCO_Perf.End("cmd_think", tt);
		}

		if (m_fCaptureCheckTimer >= m_fCaptureCheckInterval)
		{
		    m_fCaptureCheckTimer = 0.0;
		    ThinkCaptureProgress(DCO_Util.NowS());

		    m_Frontline.UpdateReconTracks(this, DCO_Util.NowS());

		    if (m_bPlayerTasking && Replication.IsServer())
		        m_PlayerTasking.CheckCompletion(this);
		}
	}

	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);
		SetEventMask(owner, EntityEvent.FRAME);
	}

	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		m_MyEnt = owner;
		if (!GetGame().GetWorld())
			return;
		InitializeCommander();
	}

	override void OnDelete(IEntity owner)
	{
		if (Replication.IsServer() && GetGame() && GetGame().GetWorld() && !DCO_Util.Teardown("cmd_release"))
			ReleaseEverything();

		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (mgr)
			mgr.UnregisterCommander(this);

		super.OnDelete(owner);
	}
}
