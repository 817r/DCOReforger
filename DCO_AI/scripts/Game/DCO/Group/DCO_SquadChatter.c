class DCO_ChatState
{
	bool m_bBreak;
	bool m_bPinned;
	bool m_bPanic;
	float m_fNext_ms;
}

class DCO_SquadChatter
{
	protected static ref map<AIGroup, ref DCO_ChatState> s_mState;
	protected static ref map<string, float> s_mSaid;
	protected static ref map<AICommander_BaseComponent, CMD_AICommanderObjectiveComponent> s_mMain;

	static bool On()
	{
		return DCO_Ctx.TuneG("radio.squad_chatter", 0.0) > 0;
	}

	protected static bool Ready()
	{
		if (!Replication.IsServer())
			return false;
		if (!s_mState)
		{
			s_mState = new map<AIGroup, ref DCO_ChatState>();
			s_mSaid = new map<string, float>();
			s_mMain = new map<AICommander_BaseComponent, CMD_AICommanderObjectiveComponent>();
		}
		return true;
	}

	protected static bool Say(FactionKey fk, string who, string key, vector pos, float radius, DCO_ERadioKind kind, array<string> params, float cooldown_ms, float now)
	{
		string sk = who + "|" + key;
		float last;
		if (s_mSaid.Find(sk, last) && now - last < cooldown_ms)
			return false;
		array<int> ids = {};
		DCO_PlayerAwareness.CollectPlayerGroupsNear(fk, pos, radius, ids);
		if (ids.IsEmpty())
			return false;
		s_mSaid.Set(sk, now);
		if (s_mSaid.Count() > 300)
			s_mSaid.Clear();
		foreach (int id : ids)
			DCO_Radio.Group(id, "SQUAD", key, kind, params, false);
		DCO_Perf.Count("chatter:" + key);
		if (DCO_Log.On(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO))
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("chatter key=%1 who=%2 grid=%3 players=%4", key, who, DCO_PlayerComms.Grid(pos), ids.Count()));
		return true;
	}

	protected static float Radius()
	{
		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetInstance();
		if (cfg)
			return cfg.GetRadioRadius();
		return 800;
	}

	static void Tick(SCR_AIUtilityComponent u, float now)
	{
		if (!u || !u.m_OwnerEntity || !On() || !Ready())
			return;
		AIAgent ag = u.GetAIAgent();
		if (!ag)
			return;
		SCR_AIGroup grp = SCR_AIGroup.Cast(ag.GetParentGroup());
		if (!grp || grp.GetLeaderEntity() != u.m_OwnerEntity)
			return;
		DCO_ChatState st;
		if (!s_mState.Find(grp, st))
		{
			st = new DCO_ChatState();
			s_mState.Set(grp, st);
			if (s_mState.Count() > 200)
				Prune();
		}
		if (now < st.m_fNext_ms)
			return;
		st.m_fNext_ms = now + 2000.0;
		DCO_GroupUtilityComponent g = DCO_GroupUtilityComponent.Cast(grp.FindComponent(DCO_GroupUtilityComponent));
		if (!g || g.IsPlayerGroup())
			return;
		FactionKey fk = DCO_Util.FactionKeyOf(u.m_OwnerEntity, true);
		vector pos = u.m_OwnerEntity.GetOrigin();
		string who = grp.GetName();
		float r = Radius();

		bool brk = g.IsBreakingContact();
		if (brk && !st.m_bBreak)
			Say(fk, who, "chatter_break_contact", pos, r, DCO_ERadioKind.WARNING, DCO_Radio.P("grid", DCO_PlayerComms.Grid(pos)), 90000.0, now);
		st.m_bBreak = brk;

		array<AIAgent> agents = {};
		grp.GetAgents(agents);
		int alive;
		int pinned;
		int panic;
		foreach (AIAgent a : agents)
		{
			SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
			if (!ca || !ca.m_UtilityComponent || !DCO_Util.IsAlive(ca.GetControlledEntity()))
				continue;
			alive++;
			SCR_AIUtilityComponent mu = ca.m_UtilityComponent;
			if (mu.m_CombatComponent && mu.m_CombatComponent.DCO_IsPinned())
				pinned++;
			DCO_AIMoraleSystem ms = mu.GetMoraleSystem();
			if (ms && (ms.GetState() == moraleState.PANIC || ms.GetState() == moraleState.BREAK))
				panic++;
		}
		bool isPinned = pinned >= 2 && pinned * 2 >= alive;
		if (isPinned && !st.m_bPinned)
			Say(fk, who, "chatter_pinned", pos, r, DCO_ERadioKind.WARNING, DCO_Radio.P("grid", DCO_PlayerComms.Grid(pos), "count", pinned.ToString()), 90000.0, now);
		st.m_bPinned = isPinned;
		bool isPanic = panic >= 2 && panic * 2 >= alive;
		if (isPanic && !st.m_bPanic)
			Say(fk, who, "chatter_panic", pos, r, DCO_ERadioKind.WARNING, DCO_Radio.P("grid", DCO_PlayerComms.Grid(pos)), 120000.0, now);
		st.m_bPanic = isPanic;

		AICommander_BaseComponent cmd = g.GetMyCommander();
		if (cmd && cmd.GetOps())
		{
			CMD_AICommanderObjectiveComponent main = cmd.GetOps().GetMainEffort();
			CMD_AICommanderObjectiveComponent prev;
			bool had = s_mMain.Find(cmd, prev);
			if (main && main.GetOwner() && (!had || prev != main))
			{
				vector mp = main.GetOwner().GetOrigin();
				if (Say(cmd.GetCommanderFactionKey(), cmd.GetCommanderUID(), "chatter_main_effort", mp, 5000.0, DCO_ERadioKind.GO, DCO_Radio.P("obj", main.GetLogName(), "grid", DCO_PlayerComms.Grid(mp)), 120000.0, now))
					s_mMain.Set(cmd, main);
			}
			else if (!main && had)
				s_mMain.Remove(cmd);
		}
	}

	static void NoteSearch(IEntity me, vector lkp)
	{
		if (!me || !On() || !Ready())
			return;
		AIControlComponent ctrl = AIControlComponent.Cast(me.FindComponent(AIControlComponent));
		if (!ctrl || !ctrl.GetAIAgent())
			return;
		SCR_AIGroup grp = SCR_AIGroup.Cast(ctrl.GetAIAgent().GetParentGroup());
		if (!grp)
			return;
		DCO_GroupUtilityComponent g = DCO_GroupUtilityComponent.Cast(grp.FindComponent(DCO_GroupUtilityComponent));
		if (!g || g.IsPlayerGroup())
			return;
		Say(DCO_Util.FactionKeyOf(me, true), grp.GetName(), "chatter_search", me.GetOrigin(), Radius(), DCO_ERadioKind.INFO, DCO_Radio.P("grid", DCO_PlayerComms.Grid(lkp)), 120000.0, GetGame().GetWorld().GetWorldTime());
	}

	protected static void Prune()
	{
		array<AIGroup> drop = {};
		foreach (AIGroup k, DCO_ChatState v : s_mState)
		{
			if (!k)
				drop.Insert(k);
		}
		foreach (AIGroup d : drop)
			s_mState.Remove(d);
	}
}
