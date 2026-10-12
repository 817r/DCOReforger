enum DCO_ECtrlOwner
{
	NONE,
	IDLE_DEFEND,
	GARRISON,
	FM_FIX,
	STATIC_GUNNER,
	CQB_SUPPORT,
	FM_ASSAULT,
	CQB_ENTRY
}

enum DCO_ECtrlReq
{
	DANGER,
	ATTACK,
	DODGE,
	PRONE,
	MORALE,
	SMOKE,
	MEDIC,
	RELOC,
	TURRET_IDLE,
	CMD_ORDER,
	CMD_WITHDRAW,
	GETOUT
}

enum DCO_ECtrlAns
{
	ALLOW,
	LIMIT,
	DENY,
	QUEUE
}

class DCO_CtrlLockTable
{
	protected ref array<int> m_aAns = {};
	protected ref array<string> m_aLim = {};

	void DCO_CtrlLockTable()
	{
		int n = 8 * 12;
		for (int i = 0; i < n; i++)
		{
			m_aAns.Insert(DCO_ECtrlAns.ALLOW);
			m_aLim.Insert("");
		}
		int o = DCO_ECtrlOwner.CQB_ENTRY;
		S(o, DCO_ECtrlReq.ATTACK, DCO_ECtrlAns.LIMIT, "fire_only");
		S(o, DCO_ECtrlReq.DODGE, DCO_ECtrlAns.LIMIT, "crouch_only");
		S(o, DCO_ECtrlReq.PRONE, DCO_ECtrlAns.DENY, "");
		S(o, DCO_ECtrlReq.MORALE, DCO_ECtrlAns.LIMIT, "shaken");
		S(o, DCO_ECtrlReq.SMOKE, DCO_ECtrlAns.DENY, "");
		S(o, DCO_ECtrlReq.MEDIC, DCO_ECtrlAns.QUEUE, "room_mark");
		S(o, DCO_ECtrlReq.RELOC, DCO_ECtrlAns.DENY, "");
		S(o, DCO_ECtrlReq.TURRET_IDLE, DCO_ECtrlAns.DENY, "");
		S(o, DCO_ECtrlReq.CMD_ORDER, DCO_ECtrlAns.QUEUE, "room_end");
		o = DCO_ECtrlOwner.CQB_SUPPORT;
		S(o, DCO_ECtrlReq.ATTACK, DCO_ECtrlAns.LIMIT, "fire_only");
		S(o, DCO_ECtrlReq.DODGE, DCO_ECtrlAns.LIMIT, "radius3");
		S(o, DCO_ECtrlReq.MORALE, DCO_ECtrlAns.LIMIT, "shaken");
		S(o, DCO_ECtrlReq.SMOKE, DCO_ECtrlAns.DENY, "");
		S(o, DCO_ECtrlReq.MEDIC, DCO_ECtrlAns.LIMIT, "radius10");
		S(o, DCO_ECtrlReq.RELOC, DCO_ECtrlAns.DENY, "");
		S(o, DCO_ECtrlReq.TURRET_IDLE, DCO_ECtrlAns.DENY, "");
		S(o, DCO_ECtrlReq.CMD_ORDER, DCO_ECtrlAns.QUEUE, "room_end");
		o = DCO_ECtrlOwner.FM_ASSAULT;
		S(o, DCO_ECtrlReq.DODGE, DCO_ECtrlAns.LIMIT, "crouch_only");
		S(o, DCO_ECtrlReq.PRONE, DCO_ECtrlAns.LIMIT, "bound_pause_only");
		S(o, DCO_ECtrlReq.MORALE, DCO_ECtrlAns.LIMIT, "shaken");
		S(o, DCO_ECtrlReq.SMOKE, DCO_ECtrlAns.DENY, "");
		S(o, DCO_ECtrlReq.MEDIC, DCO_ECtrlAns.DENY, "");
		S(o, DCO_ECtrlReq.RELOC, DCO_ECtrlAns.DENY, "");
		S(o, DCO_ECtrlReq.TURRET_IDLE, DCO_ECtrlAns.DENY, "");
		o = DCO_ECtrlOwner.FM_FIX;
		S(o, DCO_ECtrlReq.ATTACK, DCO_ECtrlAns.LIMIT, "fire_only");
		S(o, DCO_ECtrlReq.DODGE, DCO_ECtrlAns.LIMIT, "radius3");
		S(o, DCO_ECtrlReq.MORALE, DCO_ECtrlAns.LIMIT, "shaken");
		S(o, DCO_ECtrlReq.SMOKE, DCO_ECtrlAns.LIMIT, "planned_only");
		S(o, DCO_ECtrlReq.MEDIC, DCO_ECtrlAns.LIMIT, "radius15");
		S(o, DCO_ECtrlReq.GETOUT, DCO_ECtrlAns.LIMIT, "vehicle_fix_crew");
		S(o, DCO_ECtrlReq.RELOC, DCO_ECtrlAns.DENY, "");
		S(o, DCO_ECtrlReq.TURRET_IDLE, DCO_ECtrlAns.DENY, "");
		o = DCO_ECtrlOwner.STATIC_GUNNER;
		S(o, DCO_ECtrlReq.DODGE, DCO_ECtrlAns.DENY, "");
		S(o, DCO_ECtrlReq.PRONE, DCO_ECtrlAns.DENY, "");
		S(o, DCO_ECtrlReq.MORALE, DCO_ECtrlAns.LIMIT, "shaken");
		S(o, DCO_ECtrlReq.SMOKE, DCO_ECtrlAns.DENY, "");
		S(o, DCO_ECtrlReq.MEDIC, DCO_ECtrlAns.DENY, "");
		S(o, DCO_ECtrlReq.RELOC, DCO_ECtrlAns.DENY, "");
		S(o, DCO_ECtrlReq.TURRET_IDLE, DCO_ECtrlAns.DENY, "");
		S(o, DCO_ECtrlReq.CMD_ORDER, DCO_ECtrlAns.QUEUE, "contact_end");
		S(o, DCO_ECtrlReq.GETOUT, DCO_ECtrlAns.LIMIT, "gun_exceptions");
	}

	protected void S(int owner, int req, int ans, string lim)
	{
		m_aAns[owner * 12 + req] = ans;
		m_aLim[owner * 12 + req] = lim;
	}

	int Get(int owner, int req, out string lim)
	{
		lim = m_aLim[owner * 12 + req];
		return m_aAns[owner * 12 + req];
	}
}

class DCO_CtrlLock
{
	protected static ref DCO_CtrlLockTable s_Table;
	protected static ref map<AIAgent, int> s_mOwner;
	protected static ref map<string, float> s_mLogAt;
	protected static ref array<int> s_aCensus;
	protected static float s_fCensusAt;
	protected static string s_sOwnerWhy;

	protected static void Init()
	{
		if (s_Table)
			return;
		s_Table = new DCO_CtrlLockTable();
		s_mOwner = new map<AIAgent, int>();
		s_mLogAt = new map<string, float>();
		s_aCensus = {};
		for (int i = 0; i < 8 * 3; i++)
			s_aCensus.Insert(0);
	}

	static string OwnerName(int o)
	{
		return typename.EnumToString(DCO_ECtrlOwner, o);
	}

	static string OwnerWhy()
	{
		return s_sOwnerWhy;
	}

	static float GroupLoss(AIGroup grp)
	{
		SCR_AIGroupUtilityComponent gu = DCO_AssaultLock.GroupUtil(grp);
		if (!gu || !gu.DCO_GetUtil())
			return 0;
		DCO_GroupUtilityComponent g = gu.DCO_GetUtil();
		float peak = Math.Max(g.DCO_GetPeakStrength(), 1);
		return 1.0 - 1.0 * g.GetUnitCount() / peak;
	}

	static string ReqName(int r)
	{
		return typename.EnumToString(DCO_ECtrlReq, r);
	}

	static int OwnerOf(AIAgent agent)
	{
		s_sOwnerWhy = "derived";
		if (!agent)
			return DCO_ECtrlOwner.NONE;
		SCR_AIGroupUtilityComponent gu = DCO_AssaultLock.GroupUtil(agent.GetParentGroup());
		DCO_CQBActivity cqb;
		if (gu)
			cqb = gu.DCO_GetCQBActivity();
		if (cqb && cqb.IsEntry(agent))
			return DCO_ECtrlOwner.CQB_ENTRY;
		if (cqb)
			return DCO_ECtrlOwner.CQB_SUPPORT;
		if (StaticGunner(agent.GetControlledEntity()))
			return DCO_ECtrlOwner.STATIC_GUNNER;
		s_sOwnerWhy = "derived";
		DCO_GroupUtilityComponent g;
		if (gu)
			g = gu.DCO_GetUtil();
		if (g && g.GetTask() == DCO_EGroupTask.SUPPORT_BY_FIRE)
		{
			s_sOwnerWhy = "scope_fix";
			if (g.IsArmor() || CompartmentAccessComponent.GetVehicleIn(agent.GetControlledEntity()))
				s_sOwnerWhy = "vehicle_sbf_task";
			return DCO_ECtrlOwner.FM_FIX;
		}
		if (gu && gu.DCO_GetGarrisonMode() >= 0)
			return DCO_ECtrlOwner.GARRISON;
		return DCO_ECtrlOwner.NONE;
	}

	static bool StaticGunner(IEntity ch)
	{
		if (!ch)
			return false;
		IEntity veh = CompartmentAccessComponent.GetVehicleIn(ch);
		if (!veh)
			return false;
		SCR_AIVehicleUsageComponent usage = SCR_AIVehicleUsageComponent.Cast(veh.FindComponent(SCR_AIVehicleUsageComponent));
		if (!usage || usage.GetVehicleType() != EAIVehicleType.STATIC_WEAPON)
			return false;
		SCR_DamageManagerComponent dmg = SCR_DamageManagerComponent.GetDamageManager(veh);
		if (dmg && dmg.GetState() == EDamageState.DESTROYED)
			return false;
		AIControlComponent ctrl = AIControlComponent.Cast(ch.FindComponent(AIControlComponent));
		if (!ctrl || !ctrl.GetAIAgent())
			return false;
		SCR_AIGroup grp = SCR_AIGroup.Cast(ctrl.GetAIAgent().GetParentGroup());
		if (!grp)
			return false;
		SCR_AIGroupUtilityComponent gu = grp.GetGroupUtilityComponent();
		DCO_GroupUtilityComponent g = DCO_GroupUtilityComponent.Cast(grp.FindComponent(DCO_GroupUtilityComponent));
		return (g && g.HasState(DCO_EGroupState.IN_CONTACT)) || (gu && gu.m_Perception && !gu.m_Perception.m_aTargetClusters.IsEmpty());
	}

	protected static string s_sEndangerWhy;

	static bool Endangered(AIAgent agent)
	{
		s_sEndangerWhy = "-";
		IEntity e = agent.GetControlledEntity();
		if (!e)
			return false;
		SCR_CharacterDamageManagerComponent dm = SCR_CharacterDamageManagerComponent.Cast(e.FindComponent(SCR_CharacterDamageManagerComponent));
		if (dm && dm.DCO_HitsWithin(6.0) > 0)
		{
			s_sEndangerWhy = "hit";
			return true;
		}
		SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(agent);
		if (ca && ca.m_UtilityComponent && ca.m_UtilityComponent.m_ThreatSystem && ca.m_UtilityComponent.m_ThreatSystem.GetThreatMeasure() >= 1.2)
		{
			s_sEndangerWhy = "threat";
			return true;
		}
		if (ca && ca.m_UtilityComponent && ca.m_UtilityComponent.m_CombatComponent && ca.m_UtilityComponent.m_CombatComponent.DCO_IsCloseFight())
		{
			s_sEndangerWhy = "enemy_close";
			DCO_Perf.Count("reflex:ctrl_survival");
			return true;
		}
		return false;
	}

	static AIAgent AgentOf(IEntity ent)
	{
		if (!ent)
			return null;
		AIControlComponent ctrl = AIControlComponent.Cast(ent.FindComponent(AIControlComponent));
		if (!ctrl)
			return null;
		return ctrl.GetAIAgent();
	}

	static int Ask(AIAgent agent, int req, out string lim)
	{
		lim = "";
		if (!agent)
			return DCO_ECtrlAns.ALLOW;
		Init();
		int owner = OwnerOf(agent);
		NoteOwner(agent, owner);
		if (owner == DCO_ECtrlOwner.NONE)
			return DCO_ECtrlAns.ALLOW;
		int ans = s_Table.Get(owner, req, lim);
		if (ans != DCO_ECtrlAns.ALLOW && (req == DCO_ECtrlReq.DODGE || req == DCO_ECtrlReq.PRONE || req == DCO_ECtrlReq.RELOC)
			&& owner != DCO_ECtrlOwner.CQB_ENTRY && owner != DCO_ECtrlOwner.STATIC_GUNNER && Endangered(agent))
		{
			ans = DCO_ECtrlAns.ALLOW;
			lim = "";
			DCO_Perf.Count("ctrl:survival_" + OwnerName(owner) + "_" + ReqName(req));
			if (DCO_Log.On(DCO_ELogChannel.SQUAD, DCO_ELogLevel.DEBUG))
				DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(agent.GetControlledEntity()), string.Format("ctrl_survival unit=%1 owner=%2 requester=%3 why=%4", DCO_Log.IdUnit(agent.GetControlledEntity()), OwnerName(owner), ReqName(req), s_sEndangerWhy));
		}
		Census(owner, ans);
		if (ans == DCO_ECtrlAns.ALLOW)
			return ans;
		string tag = "deny";
		if (ans == DCO_ECtrlAns.LIMIT)
			tag = "limit";
		else if (ans == DCO_ECtrlAns.QUEUE)
			tag = "queue";
		DCO_Perf.Count("ctrl:" + tag + "_" + OwnerName(owner) + "_" + ReqName(req));
		if (DCO_Log.On(DCO_ELogChannel.SQUAD, DCO_ELogLevel.DEBUG))
		{
			float now = GetGame().GetWorld().GetWorldTime();
			string key = agent.ToString() + ":" + req.ToString();
			float at = -1;
			if (!s_mLogAt.Find(key, at) || now - at >= 5000.0)
			{
				s_mLogAt.Set(key, now);
				if (s_mLogAt.Count() > 4000)
					s_mLogAt.Clear();
				DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(agent.GetControlledEntity()), string.Format("ctrl_%1 unit=%2 owner=%3 requester=%4 limit=%5 src=%6", tag, DCO_Log.IdUnit(agent.GetControlledEntity()), OwnerName(owner), ReqName(req), lim, s_sOwnerWhy));
			}
		}
		return ans;
	}

	static int AskUtil(SCR_AIUtilityComponent u, int req, out string lim)
	{
		lim = "";
		if (!u)
			return DCO_ECtrlAns.ALLOW;
		return Ask(u.GetAIAgent(), req, lim);
	}

	protected static void NoteOwner(AIAgent agent, int owner)
	{
		int was = DCO_ECtrlOwner.NONE;
		bool known = s_mOwner.Find(agent, was);
		if (known && was == owner)
			return;
		if (!known && owner == DCO_ECtrlOwner.NONE)
			return;
		if (s_mOwner.Count() > 2000)
			s_mOwner.Clear();
		s_mOwner.Set(agent, owner);
		DCO_Perf.Count("ctrl:owner_" + OwnerName(owner));
		IEntity e = agent.GetControlledEntity();
		if (was != DCO_ECtrlOwner.NONE && owner != DCO_ECtrlOwner.NONE)
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdUnit(e), string.Format("ctrl_taken unit=%1 from=%2 to=%3 reason=%4", DCO_Log.IdUnit(e), OwnerName(was), OwnerName(owner), s_sOwnerWhy));
		else if (DCO_Log.On(DCO_ELogChannel.SQUAD, DCO_ELogLevel.DEBUG))
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(e), string.Format("ctrl_owner unit=%1 from=%2 to=%3 reason=%4", DCO_Log.IdUnit(e), OwnerName(was), OwnerName(owner), s_sOwnerWhy));
	}

	protected static void Census(int owner, int ans)
	{
		int col = 0;
		if (ans == DCO_ECtrlAns.LIMIT)
			col = 1;
		else if (ans == DCO_ECtrlAns.DENY || ans == DCO_ECtrlAns.QUEUE)
			col = 2;
		s_aCensus[owner * 3 + col] = s_aCensus[owner * 3 + col] + 1;
		float now = GetGame().GetWorld().GetWorldTime();
		if (now - s_fCensusAt < 30000.0)
			return;
		float span = Math.Max(1.0, (now - s_fCensusAt) / 1000.0);
		s_fCensusAt = now;
		string line = "ctrl_census";
		for (int o = 1; o < 8; o++)
		{
			int a = s_aCensus[o * 3];
			int l = s_aCensus[o * 3 + 1];
			int d = s_aCensus[o * 3 + 2];
			if (a + l + d == 0)
				continue;
			float rate = (a + l + d) / span;
			line += string.Format(" %1=%2/%3/%4(%5/s)", OwnerName(o), a, l, d, rate.ToString(-1, 2));
		}
		for (int k = 0; k < s_aCensus.Count(); k++)
			s_aCensus[k] = 0;
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", line);
	}

	static bool GroupOwned(AIGroup g, out string src)
	{
		src = "";
		if (!g)
			return false;
		array<AIAgent> agents = {};
		g.GetAgents(agents);
		foreach (AIAgent a : agents)
		{
			int o = OwnerOf(a);
			if (o == DCO_ECtrlOwner.CQB_ENTRY || o == DCO_ECtrlOwner.CQB_SUPPORT || o == DCO_ECtrlOwner.FM_ASSAULT || o == DCO_ECtrlOwner.FM_FIX)
			{
				src = OwnerName(o) + ":" + s_sOwnerWhy;
				return true;
			}
		}
		return false;
	}

	static int AskGroup(AIGroup g, int req)
	{
		if (!g)
			return DCO_ECtrlAns.ALLOW;
		array<AIAgent> agents = {};
		g.GetAgents(agents);
		int worst = DCO_ECtrlAns.ALLOW;
		foreach (AIAgent a : agents)
		{
			string lim;
			int ans = Ask(a, req, lim);
			if (ans == DCO_ECtrlAns.DENY)
				return ans;
			if (ans == DCO_ECtrlAns.QUEUE || (ans == DCO_ECtrlAns.LIMIT && worst == DCO_ECtrlAns.ALLOW))
				worst = ans;
		}
		return worst;
	}

	static void Withdraw(AIGroup g, string why)
	{
		SCR_AIGroupUtilityComponent gu = DCO_AssaultLock.GroupUtil(g);
		if (!gu || !gu.DCO_GetCQBActivity())
			return;
		gu.DCO_GetCQBActivity().Cancel("cmd_withdraw");
		DCO_Perf.Count("ctrl:owner_NONE");
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(g), string.Format("ctrl_owner grp=%1 from=CQB to=NONE reason=cmd_withdraw why=%2", DCO_Log.IdGroup(g), why));
	}

	static bool AllowMedic(SCR_ChimeraAIAgent medic, vector casualtyPos)
	{
		if (!medic)
			return true;
		string lim;
		int ans = Ask(medic, DCO_ECtrlReq.MEDIC, lim);
		if (ans == DCO_ECtrlAns.ALLOW)
			return true;
		if (ans == DCO_ECtrlAns.DENY)
			return false;
		if (ans == DCO_ECtrlAns.QUEUE)
		{
			SCR_AIGroupUtilityComponent gu = DCO_AssaultLock.GroupUtil(medic.GetParentGroup());
			return gu && gu.DCO_GetCQBActivity() && gu.DCO_GetCQBActivity().RoomSecured();
		}
		float r = 10.0;
		if (lim == "radius15")
			r = 15.0;
		IEntity me = medic.GetControlledEntity();
		return me && vector.DistanceXZ(me.GetOrigin(), casualtyPos) <= r;
	}

	protected static ref map<IEntity, float> s_mDisembarkLogAt;

	static bool DisembarkRuleAllows(SCR_AIUtilityComponent u, bool ordered)
	{
		IEntity inVeh = CompartmentAccessComponent.GetVehicleIn(u.m_OwnerEntity);
		if (!inVeh)
			return true;
		int flags = DCO_AIConfigComponent.VehicleExitOf(u);
		if (flags >= 31)
			return true;
		string kind = ExitVehKind(inVeh);
		if (kind == "static")
			return true;
		string seat = ExitSeat(u.m_OwnerEntity);
		string why;
		bool allow;
		if (DCO_TacticsCompat.VehBailAllowed(inVeh, why))
			allow = true;
		else if (ordered && (flags & DCO_EVehicleExit.ORDERED) != 0)
		{
			allow = true;
			why = "order";
		}
		else
		{
			if ((flags & DCO_EVehicleExit.DAMAGED) != 0)
			{
				SCR_VehicleDamageManagerComponent dmg = SCR_VehicleDamageManagerComponent.Cast(inVeh.FindComponent(SCR_VehicleDamageManagerComponent));
				if (dmg && dmg.GetHealthScaled() < 0.5)
				{
					allow = true;
					why = "damaged";
				}
			}
			if (!allow)
			{
				int need = DCO_EVehicleExit.COMBAT_CREW;
				if (seat == "cargo")
					need = DCO_EVehicleExit.PASSENGERS;
				else if (kind == "unarmed")
					need = DCO_EVehicleExit.UNARMED_DRIVERS;
				if ((flags & need) != 0)
				{
					allow = true;
					why = "own_" + seat;
				}
			}
		}
		if (allow)
		{
			DCO_Perf.Count("disembark:allow_" + why);
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdUnit(u.m_OwnerEntity), string.Format("disembark_allow unit=%1 flags=%2 seat=%3 veh_kind=%4 why=%5 ordered=%6 veh=%7", DCO_Log.IdUnit(u.m_OwnerEntity), flags, seat, kind, why, DCO_Util.B01(ordered), DCO_Log.IdVeh(inVeh)));
			return true;
		}
		DCO_Perf.Count("disembark:deny_" + seat);
		if (!s_mDisembarkLogAt)
			s_mDisembarkLogAt = new map<IEntity, float>();
		float nowS = DCO_Util.NowS();
		if (nowS < s_mDisembarkLogAt.Get(u.m_OwnerEntity))
			return false;
		s_mDisembarkLogAt.Set(u.m_OwnerEntity, nowS + 10.0);
		if (s_mDisembarkLogAt.Count() > 256)
			s_mDisembarkLogAt.Clear();
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdUnit(u.m_OwnerEntity), string.Format("disembark_deny unit=%1 flags=%2 seat=%3 veh_kind=%4 ordered=%5 veh=%6", DCO_Log.IdUnit(u.m_OwnerEntity), flags, seat, kind, DCO_Util.B01(ordered), DCO_Log.IdVeh(inVeh)));
		return false;
	}

	protected static string ExitSeat(IEntity ent)
	{
		ChimeraCharacter ch = ChimeraCharacter.Cast(ent);
		if (!ch || !ch.GetCompartmentAccessComponent())
			return "other";
		BaseCompartmentSlot slot = ch.GetCompartmentAccessComponent().GetCompartment();
		if (!slot)
			return "other";
		if (slot.GetType() == ECompartmentType.CARGO)
			return "cargo";
		if (slot.GetType() == ECompartmentType.PILOT)
			return "driver";
		return "gunner";
	}

	protected static string ExitVehKind(IEntity veh)
	{
		SCR_AIVehicleUsageComponent usage = SCR_AIVehicleUsageComponent.Cast(veh.FindComponent(SCR_AIVehicleUsageComponent));
		if (usage && usage.GetVehicleType() == EAIVehicleType.STATIC_WEAPON)
			return "static";
		if (DCO_VehicleCombat.HasTurret(veh))
			return "armed";
		return "unarmed";
	}

	static bool MayGetOut(SCR_AIUtilityComponent u, IEntity veh, bool ordered = false)
	{
		if (!u || !u.m_OwnerEntity)
			return true;
		if (!DisembarkRuleAllows(u, ordered))
			return false;
		string lim;
		int ans = AskUtil(u, DCO_ECtrlReq.GETOUT, lim);
		if (ans == DCO_ECtrlAns.ALLOW)
			return true;
		if (lim == "gun_exceptions")
		{
			string why;
			return GunnerMayLeave(u.m_OwnerEntity, why);
		}
		if (lim != "vehicle_fix_crew")
			return ans != DCO_ECtrlAns.DENY && ans != DCO_ECtrlAns.QUEUE;
		ChimeraCharacter ch = ChimeraCharacter.Cast(u.m_OwnerEntity);
		IEntity inVeh = CompartmentAccessComponent.GetVehicleIn(u.m_OwnerEntity);
		if (!inVeh || !ch || !ch.GetCompartmentAccessComponent() || ch.GetCompartmentAccessComponent().IsGettingIn())
			return true;
		BaseCompartmentSlot seat = ch.GetCompartmentAccessComponent().GetCompartment();
		if (!seat || seat.GetType() == ECompartmentType.CARGO)
			return true;
		string bailWhy;
		if (DCO_TacticsCompat.VehBailAllowed(inVeh, bailWhy))
			return true;
		SCR_VehicleDamageManagerComponent dmg = SCR_VehicleDamageManagerComponent.Cast(inVeh.FindComponent(SCR_VehicleDamageManagerComponent));
		float hp = -1;
		if (dmg)
			hp = dmg.GetHealthScaled();
		DCO_Perf.Count("ctrl:deny_FM_FIX_GETOUT_vehicle");
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdUnit(u.m_OwnerEntity), string.Format("ctrl_deny unit=%1 owner=FM_FIX requester=GETOUT reason=vehicle_fix_crew veh=%2 hp=%3", DCO_Log.IdUnit(u.m_OwnerEntity), DCO_Log.IdVeh(inVeh), hp.ToString(-1, 2)));
		return false;
	}

	static bool GunnerMayLeave(IEntity ch, out string reason)
	{
		reason = "";
		AIAgent agent = AgentOf(ch);
		string lim;
		int ans = Ask(agent, DCO_ECtrlReq.GETOUT, lim);
		if (ans == DCO_ECtrlAns.ALLOW)
			return true;
		if (ans != DCO_ECtrlAns.LIMIT || !agent)
			return false;
		bool more = DCO_Ctx.TuneFor(ch, "ctrl.gun_exceptions", 0.0) > 0;
		IEntity gun = CompartmentAccessComponent.GetVehicleIn(ch);
		if (gun && more)
		{
			SCR_DamageManagerComponent dm = SCR_DamageManagerComponent.Cast(gun.FindComponent(SCR_DamageManagerComponent));
			if (dm && dm.GetState() == EDamageState.DESTROYED)
			{
				reason = "gun_destroyed";
				GunLeaveLog(ch, gun, reason, -1);
				return true;
			}
			if (GunEmptyLong(ch, gun))
			{
				reason = "gun_no_ammo";
				GunLeaveLog(ch, gun, reason, -1);
				return true;
			}
		}
		SCR_AIGroupUtilityComponent gu = DCO_AssaultLock.GroupUtil(agent.GetParentGroup());
		if (!gu || !gu.m_Perception)
			return false;
		TurretComponent turret;
		if (gun)
			turret = TurretComponent.Cast(gun.FindComponent(TurretComponent));
		SCR_AIUtilityComponent u = SCR_AIUtilityComponent.Cast(agent.FindComponent(SCR_AIUtilityComponent));
		bool threatened = u && u.m_ThreatSystem && u.m_ThreatSystem.GetState() >= EAIThreatState.THREATENED;
		PerceptionManager pm = GetGame().GetPerceptionManager();
		foreach (SCR_AITargetInfo tgt : gu.m_Perception.m_aTargets)
		{
			if (!tgt || !tgt.m_Entity || !DCO_Util.IsNotDead(tgt.m_Entity))
				continue;
			float d = vector.Distance(tgt.m_vWorldPos, ch.GetOrigin());
			if (d < 15.0)
			{
				reason = "enemy_close";
				return true;
			}
			if (!more || !threatened || !turret || !pm || d > 150.0 || pm.GetTime() - tgt.m_fTimestamp > 10.0)
				continue;
			vector excess = turret.GetAimingAngleExcess(tgt.m_vWorldPos);
			if (Math.AbsFloat(excess[0]) >= 15.0)
			{
				reason = "enemy_outside_arc";
				GunLeaveLog(ch, gun, reason, Math.AbsFloat(excess[0]));
				return true;
			}
		}
		return false;
	}

	protected static ref map<IEntity, float> s_mGunEmptySince;

	protected static bool GunEmptyLong(IEntity ch, IEntity gun)
	{
		TurretControllerComponent tc = TurretControllerComponent.Cast(gun.FindComponent(TurretControllerComponent));
		BaseWeaponManagerComponent wm;
		if (tc)
			wm = tc.GetWeaponManager();
		if (!wm || !wm.GetCurrentWeapon())
			return false;
		BaseMagazineComponent mag = wm.GetCurrentWeapon().GetCurrentMagazine();
		if (!s_mGunEmptySince)
			s_mGunEmptySince = new map<IEntity, float>();
		if (s_mGunEmptySince.Count() > 200)
			s_mGunEmptySince.Clear();
		float now = GetGame().GetWorld().GetWorldTime();
		if (mag && mag.GetAmmoCount() > 0)
		{
			s_mGunEmptySince.Remove(ch);
			return false;
		}
		float since;
		if (!s_mGunEmptySince.Find(ch, since))
		{
			s_mGunEmptySince.Set(ch, now);
			return false;
		}
		return now - since >= 8000.0;
	}

	protected static void GunLeaveLog(IEntity ch, IEntity gun, string reason, float excessDeg)
	{
		DCO_Perf.Count("ctrl:gunner_leave_" + reason);
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdUnit(ch), string.Format("ctrl_gunner_leave unit=%1 gun=%2 reason=%3 arc_excess_deg=%4", DCO_Log.IdUnit(ch), DCO_Log.IdVeh(gun), reason, Math.Round(excessDeg)));
	}
}

modded class SCR_AIGetOutVehicle
{
	protected bool m_bDCOCtrlChecked;

	override float CustomEvaluate()
	{
		if (!m_bDCOCtrlChecked)
		{
			m_bDCOCtrlChecked = true;
			SCR_AIGetOutActivity outAct = SCR_AIGetOutActivity.Cast(GetRelatedGroupActivity());
			bool ordered = outAct && outAct.m_RelatedWaypoint;
			if (m_Utility && !DCO_CtrlLock.MayGetOut(m_Utility, m_Vehicle.m_Value, ordered))
			{
				Fail();
				return 0;
			}
		}
		return super.CustomEvaluate();
	}
}
