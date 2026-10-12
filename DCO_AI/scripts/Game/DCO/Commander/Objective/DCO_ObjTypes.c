class DCO_ObjTypes
{
	protected static ref DCO_ObjTypes s_Instance;

	protected ref ScriptInvokerBase<DCO_EvObjContestFn> m_OnResult = new ScriptInvokerBase<DCO_EvObjContestFn>();
	protected float m_fAcc;

	static DCO_ObjTypes Get()
	{
		if (!s_Instance)
			s_Instance = new DCO_ObjTypes();
		return s_Instance;
	}

	ScriptInvokerBase<DCO_EvObjContestFn> GetOnObjectiveResult()	{ return m_OnResult; }

	static string TypeName(int t)
	{
		return typename.EnumToString(CMD_EObjectiveType, t);
	}

	void Tick(AICommander_ManagerComponent mgr, float timeSlice)
	{
		m_fAcc += timeSlice;
		if (m_fAcc < 1.0)
			return;
		m_fAcc = 0;
		if (!mgr || !Replication.IsServer())
			return;
		float now_s = GetGame().GetWorld().GetWorldTime() * 0.001;
		foreach (CMD_AICommanderObjectiveComponent obj : mgr.m_aObjective)
		{
			if (obj)
				obj.DCO_UpdateObjType(now_s);
		}
	}

	void Raise(CMD_AICommanderObjectiveComponent obj, bool done, string why, string detail)
	{
		string name = "obj_type_fail";
		if (done)
			name = "obj_type_done";
		int t = obj.GetObjectiveType();
		string line = string.Format("obj=%1 type=%2 why=%3 %4", obj.GetLogName(), TypeName(t), why, detail);
		DCO_Perf.Count("objtype:" + name);
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", name + " " + line);
		m_OnResult.Invoke(obj, done);
		DCO_Events.Get().GetOnAnyEvent().Invoke(name, line);
	}
}

modded class AICommander_ManagerComponent
{
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		super.EOnFrame(owner, timeSlice);
		DCO_ObjTypes.Get().Tick(this, timeSlice);
	}
}

modded class CMD_AICommanderObjectiveComponent
{
	[Attribute("1200", UIWidgets.Slider, "DEFEND: seconds from mission start the defending faction must keep this objective. HOLD_UNTIL: seconds the holding faction must own it without a break.", "30 7200 30", category: "Objective Type")]
	protected float m_fDCOTypeTime_s;

	[Attribute("", UIWidgets.EditBox, "DEFEND: defending faction key (empty = owner at mission start). HOLD_UNTIL: faction that must hold it (empty = any faction). DESTROY: faction that must destroy the targets (empty = any faction not owning the objective at start).", category: "Objective Type")]
	protected string m_sDCOTypeFaction;

	[Attribute("0", UIWidgets.Slider, "HOLD_UNTIL and DESTROY: deadline in seconds from mission start; the objective fails when it passes unfinished. 0 = no deadline.", "0 7200 30", category: "Objective Type")]
	protected float m_fDCOTypeDeadline_s;

	[Attribute("0", UIWidgets.EditBox, "DEFEND: fail early when the defenders inside the objective area stay below this share of the attackers for 60 s (0.25 = outnumbered 4 to 1). 0 = off.", category: "Objective Type")]
	protected float m_fDCOTypeDefendRatio;

	[Attribute("1", UIWidgets.CheckBox, "DEFEND: the objective is set to Retain (no doctrine withdraw) while the defense runs, and restored when it ends.", category: "Objective Type")]
	protected bool m_bDCOTypeRetain;

	[Attribute(desc: "DESTROY: entity names of the targets. Done when every target is destroyed or deleted. Empty = DESTROY without target tracking.", category: "Objective Type")]
	protected ref array<string> m_aDCODestroyTargets;

	protected int m_iDCOTypeResult;
	protected string m_sDCOTypeWhy;
	protected float m_fDCOTypeStart_s = -1;
	protected FactionKey m_sDCODefender;
	protected FactionKey m_sDCOHoldBy;
	protected float m_fDCOHoldSince_s = -1;
	protected float m_fDCOLowSince_s = -1;
	protected bool m_bDCORetainSet;
	protected bool m_bDCORetainPrev;
	protected float m_fDCOTypeProgress;
	protected ref array<IEntity> m_aDCOTargetEnts;
	protected ref array<bool> m_aDCOTargetSeen;
	protected float m_fDCONextProgressLog_s;
	protected bool m_bDCOMissingLogged;

	int DCO_GetTypeResult()			{ return m_iDCOTypeResult; }
	string DCO_GetTypeWhy()			{ return m_sDCOTypeWhy; }
	float DCO_GetTypeProgress()		{ return m_fDCOTypeProgress; }
	float DCO_GetTypeTime()			{ return m_fDCOTypeTime_s; }
	string DCO_GetTypeFaction()		{ return m_sDCOTypeFaction; }
	float DCO_GetTypeDeadline()		{ return m_fDCOTypeDeadline_s; }
	FactionKey DCO_GetDefender()	{ return m_sDCODefender; }
	float DCO_GetTypeDefendRatio()	{ return m_fDCOTypeDefendRatio; }
	bool DCO_GetTypeRetain()		{ return m_bDCOTypeRetain; }

	protected void DCO_LogTypeField(string field, string value, string old, string by, int playerID, string applies)
	{
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("obj_fields_set obj=%1 field=%2 value=%3 old=%4 by=%5 player=%6 applies=%7 type=%8 running=%9",
			GetLogName(), field, value, old, by, playerID, applies, DCO_ObjTypes.TypeName(m_eObjectiveType), DCO_Util.B01(m_fDCOTypeStart_s >= 0 && m_iDCOTypeResult == 0)));
	}

	void DCO_SetTypeTime(float v, string by, int playerID)
	{
		v = Math.Clamp(v, 30.0, 7200.0);
		if (Math.AbsFloat(v - m_fDCOTypeTime_s) < 0.01)
			return;
		float old = m_fDCOTypeTime_s;
		m_fDCOTypeTime_s = v;
		DCO_LogTypeField("type_time_s", Math.Round(v).ToString(), Math.Round(old).ToString(), by, playerID, "now");
	}

	void DCO_SetTypeDeadline(float v, string by, int playerID)
	{
		v = Math.Clamp(v, 0.0, 7200.0);
		if (Math.AbsFloat(v - m_fDCOTypeDeadline_s) < 0.01)
			return;
		float old = m_fDCOTypeDeadline_s;
		m_fDCOTypeDeadline_s = v;
		DCO_LogTypeField("type_deadline_s", Math.Round(v).ToString(), Math.Round(old).ToString(), by, playerID, "now");
	}

	void DCO_SetTypeDefendRatio(float v, string by, int playerID)
	{
		v = Math.Clamp(v, 0.0, 1.0);
		if (Math.AbsFloat(v - m_fDCOTypeDefendRatio) < 0.001)
			return;
		float old = m_fDCOTypeDefendRatio;
		m_fDCOTypeDefendRatio = v;
		m_fDCOLowSince_s = -1;
		DCO_LogTypeField("type_defend_ratio", v.ToString(-1, 2), old.ToString(-1, 2), by, playerID, "now");
	}

	void DCO_SetTypeFaction(string v, string by, int playerID)
	{
		if (v == m_sDCOTypeFaction)
			return;
		string old = m_sDCOTypeFaction;
		m_sDCOTypeFaction = v;
		string applies = "now";
		if (m_eObjectiveType == CMD_EObjectiveType.DEFEND && !m_sDCODefender.IsEmpty())
			applies = "ignored_defense_running";
		DCO_LogTypeField("type_faction", DCO_NoneIf(v), DCO_NoneIf(old), by, playerID, applies);
	}

	void DCO_SetTypeRetain(bool v, string by, int playerID)
	{
		if (v == m_bDCOTypeRetain)
			return;
		bool old = m_bDCOTypeRetain;
		m_bDCOTypeRetain = v;
		if (m_eObjectiveType == CMD_EObjectiveType.DEFEND && m_iDCOTypeResult == 0 && !m_sDCODefender.IsEmpty())
		{
			if (v && !m_bDCORetainSet)
			{
				m_bDCORetainPrev = IsRetain();
				m_bDCORetainSet = true;
				SetRetain(true);
			}
			else if (!v && m_bDCORetainSet)
			{
				SetRetain(m_bDCORetainPrev);
				m_bDCORetainSet = false;
			}
		}
		DCO_LogTypeField("type_retain", DCO_Util.B01(v).ToString(), DCO_Util.B01(old).ToString(), by, playerID, "now");
	}

	bool DCO_HasMissionType()
	{
		if (m_eObjectiveType == CMD_EObjectiveType.DEFEND || m_eObjectiveType == CMD_EObjectiveType.HOLD_UNTIL)
			return true;
		return m_eObjectiveType == CMD_EObjectiveType.DESTROY && m_aDCODestroyTargets && !m_aDCODestroyTargets.IsEmpty();
	}

	protected int DCO_SidePresence(FactionKey side, bool friendly)
	{
		array<FactionKey> keys = {};
		GetPresenceFactionKeys(keys);
		int n;
		foreach (FactionKey k : keys)
		{
			if (AreFactionsFriendly(k, side) == friendly)
				n += GetFactionPresence(k);
		}
		return n;
	}

	protected void DCO_EndType(bool done, string why, float now_s)
	{
		m_iDCOTypeResult = 2;
		if (done)
			m_iDCOTypeResult = 1;
		m_sDCOTypeWhy = why;
		if (m_bDCORetainSet)
		{
			SetRetain(m_bDCORetainPrev);
			m_bDCORetainSet = false;
		}
		string detail = string.Format("t=%1 progress=%2 owner=%3 defender=%4 hold_by=%5", Math.Round(now_s - m_fDCOTypeStart_s), m_fDCOTypeProgress.ToString(-1, 2), DCO_NoneIf(GetOwningFaction()), DCO_NoneIf(m_sDCODefender), DCO_NoneIf(m_sDCOHoldBy));
		DCO_ObjTypes.Get().Raise(this, done, why, detail);
	}

	protected static string DCO_NoneIf(string s)
	{
		if (s.IsEmpty())
			return "none";
		return s;
	}

	void DCO_UpdateObjType(float now_s)
	{
		if (m_iDCOTypeResult != 0 || !DCO_HasMissionType())
			return;
		if (m_fDCOTypeStart_s < 0)
		{
			m_fDCOTypeStart_s = now_s;
			m_fDCONextProgressLog_s = now_s + 60.0;
			int targets;
			if (m_aDCODestroyTargets)
				targets = m_aDCODestroyTargets.Count();
			DCO_Perf.Count("objtype:start");
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("obj_type_start obj=%1 type=%2 time_s=%3 faction=%4 deadline_s=%5 defend_ratio=%6 retain=%7 targets=%8 owner=%9", GetLogName(), DCO_ObjTypes.TypeName(m_eObjectiveType), Math.Round(m_fDCOTypeTime_s), DCO_NoneIf(m_sDCOTypeFaction), Math.Round(m_fDCOTypeDeadline_s), m_fDCOTypeDefendRatio.ToString(-1, 2), DCO_Util.B01(m_bDCOTypeRetain), targets, DCO_NoneIf(GetOwningFaction())));
		}
		float elapsed = now_s - m_fDCOTypeStart_s;
		if (m_eObjectiveType == CMD_EObjectiveType.DEFEND)
			DCO_UpdateDefend(now_s, elapsed);
		else if (m_eObjectiveType == CMD_EObjectiveType.HOLD_UNTIL)
			DCO_UpdateHold(now_s, elapsed);
		else
			DCO_UpdateDestroy(now_s, elapsed);
		if (m_iDCOTypeResult == 0 && now_s >= m_fDCONextProgressLog_s)
		{
			m_fDCONextProgressLog_s = now_s + 60.0;
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("obj_type_progress obj=%1 type=%2 progress=%3 t=%4 owner=%5 defender=%6 hold_by=%7", GetLogName(), DCO_ObjTypes.TypeName(m_eObjectiveType), m_fDCOTypeProgress.ToString(-1, 2), Math.Round(elapsed), DCO_NoneIf(GetOwningFaction()), DCO_NoneIf(m_sDCODefender), DCO_NoneIf(m_sDCOHoldBy)));
		}
	}

	protected void DCO_UpdateDefend(float now_s, float elapsed)
	{
		FactionKey owner = GetOwningFaction();
		if (m_sDCODefender.IsEmpty())
		{
			m_sDCODefender = m_sDCOTypeFaction;
			if (m_sDCODefender.IsEmpty())
				m_sDCODefender = owner;
			if (m_sDCODefender.IsEmpty())
			{
				DCO_Perf.Count("objtype:defend_no_defender");
				if (!m_bDCOMissingLogged && elapsed >= 10.0)
				{
					m_bDCOMissingLogged = true;
					DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.WARN, "-", string.Format("obj_type_no_defender obj=%1 hint=set_objective_type_faction_or_initial_owner", GetLogName()));
				}
				return;
			}
			if (m_bDCOTypeRetain)
			{
				m_bDCORetainPrev = IsRetain();
				m_bDCORetainSet = true;
				SetRetain(true);
			}
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("obj_type_defender obj=%1 defender=%2 retain=%3", GetLogName(), m_sDCODefender, DCO_Util.B01(m_bDCORetainSet)));
		}
		m_fDCOTypeProgress = Math.Clamp(elapsed / Math.Max(m_fDCOTypeTime_s, 1.0), 0.0, 1.0);
		if (!owner.IsEmpty() && !AreFactionsFriendly(owner, m_sDCODefender))
		{
			DCO_EndType(false, "captured_by_" + owner, now_s);
			return;
		}
		if (m_fDCOTypeDefendRatio > 0)
		{
			int friends = DCO_SidePresence(m_sDCODefender, true);
			int enemies = DCO_SidePresence(m_sDCODefender, false);
			if (enemies > 0 && friends < m_fDCOTypeDefendRatio * enemies)
			{
				if (m_fDCOLowSince_s < 0)
					m_fDCOLowSince_s = now_s;
				else if (now_s - m_fDCOLowSince_s >= 60.0)
				{
					DCO_EndType(false, string.Format("outnumbered_%1v%2", friends, enemies), now_s);
					return;
				}
			}
			else
				m_fDCOLowSince_s = -1;
		}
		if (elapsed >= m_fDCOTypeTime_s)
			DCO_EndType(true, "held_until_time", now_s);
	}

	protected void DCO_UpdateHold(float now_s, float elapsed)
	{
		FactionKey owner = GetOwningFaction();
		bool counts = !owner.IsEmpty() && (m_sDCOTypeFaction.IsEmpty() || AreFactionsFriendly(owner, m_sDCOTypeFaction));
		if (counts && owner != m_sDCOHoldBy)
		{
			if (!m_sDCOHoldBy.IsEmpty())
				DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("obj_type_hold_break obj=%1 lost_by=%2 held_s=%3 new=%4", GetLogName(), m_sDCOHoldBy, Math.Round(now_s - m_fDCOHoldSince_s), owner));
			m_sDCOHoldBy = owner;
			m_fDCOHoldSince_s = now_s;
			DCO_Perf.Count("objtype:hold_start");
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("obj_type_hold_start obj=%1 by=%2 need_s=%3", GetLogName(), owner, Math.Round(m_fDCOTypeTime_s)));
		}
		else if (!counts && !m_sDCOHoldBy.IsEmpty())
		{
			DCO_Perf.Count("objtype:hold_break");
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("obj_type_hold_break obj=%1 lost_by=%2 held_s=%3 new=%4", GetLogName(), m_sDCOHoldBy, Math.Round(now_s - m_fDCOHoldSince_s), DCO_NoneIf(owner)));
			m_sDCOHoldBy = "";
			m_fDCOHoldSince_s = -1;
		}
		m_fDCOTypeProgress = 0;
		if (!m_sDCOHoldBy.IsEmpty())
		{
			float held = now_s - m_fDCOHoldSince_s;
			m_fDCOTypeProgress = Math.Clamp(held / Math.Max(m_fDCOTypeTime_s, 1.0), 0.0, 1.0);
			if (held >= m_fDCOTypeTime_s)
			{
				DCO_EndType(true, "held_by_" + m_sDCOHoldBy, now_s);
				return;
			}
		}
		if (m_fDCOTypeDeadline_s > 0 && elapsed >= m_fDCOTypeDeadline_s)
			DCO_EndType(false, "deadline", now_s);
	}

	protected bool DCO_TargetDestroyed(IEntity e)
	{
		if (!e)
			return true;
		DamageManagerComponent dm = DamageManagerComponent.Cast(e.FindComponent(DamageManagerComponent));
		return dm && dm.GetState() == EDamageState.DESTROYED;
	}

	protected void DCO_UpdateDestroy(float now_s, float elapsed)
	{
		int n = m_aDCODestroyTargets.Count();
		if (!m_aDCOTargetEnts)
		{
			m_sDCODefender = GetOwningFaction();
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("obj_type_defender obj=%1 defender=%2 attacker=%3 targets=%4", GetLogName(), DCO_NoneIf(m_sDCODefender), DCO_NoneIf(m_sDCOTypeFaction), n));
			m_aDCOTargetEnts = {};
			m_aDCOTargetSeen = {};
			for (int i = 0; i < n; i++)
			{
				m_aDCOTargetEnts.Insert(null);
				m_aDCOTargetSeen.Insert(false);
			}
		}
		int destroyed;
		int missing;
		for (int k = 0; k < n; k++)
		{
			if (!m_aDCOTargetSeen[k])
			{
				IEntity found = GetGame().GetWorld().FindEntityByName(m_aDCODestroyTargets[k]);
				if (!found)
				{
					missing++;
					if (elapsed >= 10.0 && !m_bDCOMissingLogged)
						DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.WARN, "-", string.Format("obj_type_target_missing obj=%1 target=%2", GetLogName(), m_aDCODestroyTargets[k]));
					continue;
				}
				m_aDCOTargetEnts[k] = found;
				DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("obj_type_target obj=%1 target=%2 prefab=%3", GetLogName(), m_aDCODestroyTargets[k], DCO_Util.PrefabShort(found)));
				m_aDCOTargetSeen[k] = true;
			}
			if (DCO_TargetDestroyed(m_aDCOTargetEnts[k]))
				destroyed++;
		}
		if (elapsed >= 10.0)
			m_bDCOMissingLogged = true;
		int counted = n - missing;
		m_fDCOTypeProgress = 0;
		if (counted > 0)
			m_fDCOTypeProgress = destroyed / (counted * 1.0);
		if (counted > 0 && destroyed >= counted && missing == 0)
		{
			DCO_EndType(true, string.Format("targets_destroyed_%1", destroyed), now_s);
			return;
		}
		if (m_fDCOTypeDeadline_s > 0 && elapsed >= m_fDCOTypeDeadline_s)
			DCO_EndType(false, string.Format("deadline_%1of%2", destroyed, n), now_s);
	}
}
