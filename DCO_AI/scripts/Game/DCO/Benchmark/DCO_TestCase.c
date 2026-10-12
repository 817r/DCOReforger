enum DCO_ETestAssertKind
{
	COUNTER_AT_LEAST,
	COUNTER_ZERO,
	EVENT_PRESENT,
	EVENT_ABSENT,
	PERF_MAX_MS
}

[BaseContainerProps(), SCR_BaseContainerCustomTitleField("m_sKey")]
class DCO_TestAssert
{
	[Attribute("0", UIWidgets.ComboBox, "COUNTER_AT_LEAST: DCO_Perf counter >= Value. COUNTER_ZERO: counter must stay 0. EVENT_PRESENT/ABSENT: logger EVENT key (first word). PERF_MAX_MS: perf section max ms <= Value.", "", ParamEnumArray.FromEnum(DCO_ETestAssertKind))]
	DCO_ETestAssertKind m_eKind;

	[Attribute("", UIWidgets.EditBox, "Counter key, event key, or perf section name")]
	string m_sKey;

	[Attribute("1", UIWidgets.EditBox, "Threshold: count for COUNTER_AT_LEAST / EVENT_PRESENT, max ms for PERF_MAX_MS")]
	float m_fValue;

	[Attribute("0", UIWidgets.EditBox, "Must be satisfied within this many seconds of test start. 0 = any time before the end.")]
	float m_fWithin;

	float m_fMetAt = -1;
}

[BaseContainerProps()]
class DCO_TestLogLevel
{
	[Attribute("2", UIWidgets.ComboBox, "Log channel", "", ParamEnumArray.FromEnum(DCO_ELogChannel))]
	DCO_ELogChannel m_eChannel;

	[Attribute("4", UIWidgets.ComboBox, "Level for this channel during the test (Workbench only)", "", ParamEnumArray.FromEnum(DCO_ELogLevel))]
	DCO_ELogLevel m_eLevel;
}

[ComponentEditorProps(category: "DCO/Test", description: "Isolated feature test case: applies setup, checks assertions, writes ITEST PASS/FAIL to the TEST channel of dco.log ($profile:DCO/Logs session folder). Logic runs in Workbench only.")]
class DCO_TestCaseComponentClass : ScriptComponentClass
{
}

class DCO_TestCaseComponent : ScriptComponent
{
	[Attribute("", UIWidgets.EditBox, "Tracker code, e.g. 71", category: "Test Case")]
	protected string m_sCode;

	[Attribute("", UIWidgets.EditBox, "Short feature name", category: "Test Case")]
	protected string m_sName;

	[Attribute("", UIWidgets.EditBoxMultiline, "Intent: what should happen in this test. Shown as a hint and GM message at start, and written to the log.", category: "Test Case")]
	protected string m_sIntent;

	[Attribute("300", UIWidgets.EditBox, "Test length in seconds", category: "Test Case")]
	protected float m_fDuration;

	[Attribute("0", UIWidgets.CheckBox, "Run the whole duration even when every assert is met (for comparing whole fights between runs).", category: "Test Case")]
	protected bool m_bRunFull;

	[Attribute("-1", UIWidgets.EditBox, "Time of day in hours (0-24). -1 = keep world time.", category: "Test Case")]
	protected float m_fTimeOfDay;

	[Attribute("0", UIWidgets.CheckBox, "Keep the normal AI LOD instead of forcing Full Detail on every AI. Only for performance tests that must look like a real server.", category: "Test Case")]
	protected bool m_bVanillaLOD;
	protected float m_fNextLODSweep_s;

	[Attribute("0", UIWidgets.CheckBox, "Every 30 s log the action queue of every AI soldier: total, max per soldier, obsolete entries and the top behavior classes (itest_actions). For performance tests.", category: "Test Case")]
	protected bool m_bActionProbe;
	protected float m_fNextActionProbe_s;

	[Attribute("0 0 0", UIWidgets.Coords, "Play-mode camera is moved here at test start so the test AI is near the player (AI LOD). 0 0 0 = this entity's position; both 0 = camera not moved.", category: "Test Case")]
	protected vector m_vCameraFocus;

	[Attribute("0", UIWidgets.Flags, "DCO behaviors turned OFF for this test (only ones that interfere). Everything else stays at real defaults.", "", ParamEnumArray.FromEnum(DCO_EBehavior), category: "Test Case")]
	protected int m_iDisabledBehaviors;

	[Attribute("", UIWidgets.Object, "Checks evaluated during the test", category: "Test Case")]
	protected ref array<ref DCO_TestAssert> m_aAsserts;

	[Attribute("", UIWidgets.EditBoxMultiline, "Things to check by eye (written to the log)", category: "Test Case")]
	protected string m_sManualNotes;

	[Attribute("", UIWidgets.Object, "Log level per channel for this test world. Applied at test start, Workbench only.", category: "Test Case")]
	protected ref array<ref DCO_TestLogLevel> m_aLogLevels;

	[Attribute("", UIWidgets.Auto, "Group entity names to trace (member behavior switches and group activity switches). Needs UNIT and SQUAD at TRACE. Workbench only.", category: "Test Case")]
	protected ref array<string> m_aTraceGroups;

	[Attribute("0", UIWidgets.CheckBox, "Reset commander learning at test start: clears the learned tactic, terrain/approach (including failed bearings) and defense stores in $profile:DCO/CommanderLearn so this run starts from priors. Workbench only.", category: "Test Case")]
	protected bool m_bResetLearning;

	[Attribute("", UIWidgets.Auto, "Group entity names that get the Task Groups task at test start, for tests without a commander (e.g. a CQB test squad). Workbench only.", category: "Test Case")]
	protected ref array<string> m_aTaskGroups;

	[Attribute("", UIWidgets.EditBox, "Objective entity moved to one random Random Objective Site at test start (isolated tactic tests). Workbench only.", category: "Test Case")]
	protected string m_sRandomObjective;

	[Attribute("", UIWidgets.Auto, "Random Objective Sites, one per entry: 'x y z radius label', e.g. '3020.3 57.7 1763.6 60 CheckPoint'. One is picked at random each run.", category: "Test Case")]
	protected ref array<string> m_aRandomSites;

	[Attribute("", UIWidgets.Auto, "Smoke screens spawned during the test, one per entry: 'time_s x y z', e.g. '40 2960 55 1960'. Each smoke is also registered as a DCO smoke cloud. Workbench only.", category: "Test Case")]
	protected ref array<string> m_aSmokeEvents;
	protected ref array<int> m_aSmokeSpawned = {};

	[Attribute("", UIWidgets.Auto, "Scheduled events, one per entry: 'time_s kill <group[,group..]> <fraction 0-1>' kills that share of the alive members, 'time_s panic <group[,group..]> <fraction> <dur_s>' forces PANIC (morale base 3.2) for that time, 'time_s spawn <group prefab> x y z' spawns a group, 'time_s wound <group[,group..]> <count>' gives the first <count> alive members a leg bleeding wound. Use @event_key instead of time_s to fire when that logger EVENT key first appears, e.g. @obj_capture_pending panic <groups> 1 600. Workbench only.", category: "Test Case")]
	protected ref array<string> m_aWorldEvents;
	protected ref array<int> m_aWorldEventsDone = {};

	[Attribute("", UIWidgets.Auto, "Tune overrides applied at test start to the global and faction DCO settings, one per entry: 'key value', e.g. 'reinf.ratio_attack 0'. Workbench only.", category: "Test Case")]
	protected ref array<string> m_aTune;

	[Attribute("", UIWidgets.Auto, "Groups that duck out of sight the first time an enemy targets them: their AI is switched off and every member is teleported 10-20 m to a reachable outdoor spot hidden from that enemy. Add :ai to the name (e.g. USSR_FT_1:ai) to keep their AI on so they fight from the hiding spot. For lost-contact tests. Workbench only.", category: "Test Case")]
	protected ref array<string> m_aDuckGroups;
	protected ref array<string> m_aDucked = {};

	[Attribute("", UIWidgets.Auto, "Group features switched off at test start, one per entry: group name and DCO_EGroupFeature mask, e.g. US_Squad_1 1 (1 = CQB). Workbench only.", category: "Test Case")]
	protected ref array<string> m_aGroupFeatureOff;

	[Attribute("", UIWidgets.Auto, "Groups whose 40 mm HEDP rounds are swapped for M713 smoke rounds 8 s after test start (the round already loaded stays). For GL smoke tests. Workbench only.", category: "Test Case")]
	protected ref array<string> m_aGlSmokeGroups;

	[Attribute("1", UIWidgets.ComboBox, "Task given to the Task Groups at test start", "", ParamEnumArray.FromEnum(DCO_EGroupTask), category: "Test Case")]
	protected DCO_EGroupTask m_eTaskGroupsTask;

	[Attribute("", UIWidgets.EditBox, "Objective for a tactic test: entity or display name (e.g. Primary_Objective), scope:<primary name> = the primary plus non-primary objectives within Shaping Radius (shaping objectives may go first), or * / empty = every objective the commander does not hold. The tactic setup is on when Tactic Commander is set: seeded intel, doctrine bypass for these objectives, forced tactic, commander sizing, one re-plan per objective 30 s after a plan ends. Workbench only.", category: "Tactic Test")]
	protected string m_sTacticObjective;

	[Attribute("2", UIWidgets.ComboBox, "Tactic forced on the test objective", "", ParamEnumArray.FromEnum(DCO_ETactic), category: "Tactic Test")]
	protected DCO_ETactic m_eForceTactic;

	[Attribute("", UIWidgets.EditBox, "Commander uid that runs the tactic test (e.g. US_CMD). Empty = tactic setup off.", category: "Tactic Test")]
	protected string m_sTacticCommander;

	[Attribute("600", UIWidgets.EditBox, "Shaping radius (m) for scope:<primary>: non-primary objectives within this distance of the primary may be attacked first when easier or an SBF site for the primary.", category: "Tactic Test")]
	protected float m_fShapingRadius;

	protected static DCO_TestCaseComponent s_Active;
	protected static ref array<AIGroup> s_aTraced;
	protected ref map<string, int> m_mEvents = new map<string, int>();
	protected float m_fStart_s;
	protected bool m_bDone;

	static bool RunOn()
	{
#ifdef WORKBENCH
		return true;
#else
		return System.IsCLIParam("dcoTest");
#endif
	}

	static bool Active()
	{
		return s_Active != null;
	}

	static void OnEvent(string text)
	{
		if (!s_Active || s_Active.m_bDone)
			return;
		int sp = text.IndexOf(" ");
		string key = text;
		if (sp > 0)
			key = text.Substring(0, sp);
		s_Active.m_mEvents.Set(key, s_Active.m_mEvents.Get(key) + 1);
	}

	static void TraceUnit(SCR_AIUtilityComponent u, SCR_AIActionBase from, SCR_AIActionBase to)
	{
		if (!u || !s_aTraced || s_aTraced.IsEmpty())
			return;
		AIAgent agent = u.GetOwner();
		if (!agent)
			return;
		AIGroup g = agent.GetParentGroup();
		if (!g || !s_aTraced.Contains(g))
			return;
		string fromName = "-";
		float fromPrio = -1;
		if (from)
		{
			fromName = from.ClassName();
			fromPrio = from.GetPriority();
		}
		string toName = "-";
		float toPrio = -1;
		string act = "-";
		if (to)
		{
			toName = to.ClassName();
			toPrio = to.GetPriority();
			if (to.GetRelatedGroupActivity())
				act = to.GetRelatedGroupActivity().ClassName();
		}
		DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.TRACE, "-", string.Format("unit_action unit=%1 grp=%2 from=%3 to=%4 prio=%5 prio_from=%6 activity=%7",
			DCO_Log.IdUnit(agent.GetControlledEntity()), DCO_Log.IdGroup(g), fromName, toName, toPrio.ToString(-1, 1), fromPrio.ToString(-1, 1), act));
	}

	static void TraceGroup(AIGroup g, SCR_AIActionBase from, SCR_AIActionBase to, string wp, string task)
	{
		if (!g || !s_aTraced || !s_aTraced.Contains(g))
			return;
		string fromName = "-";
		if (from)
			fromName = from.ClassName();
		string toName = "-";
		float prio = -1;
		if (to)
		{
			toName = to.ClassName();
			prio = to.GetPriority();
		}
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.TRACE, "-", string.Format("grp_activity grp=%1 from=%2 to=%3 prio=%4 wp=%5 task=%6", DCO_Log.IdGroup(g), fromName, toName, prio.ToString(-1, 1), wp, task));
	}

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if (!GetGame().InPlayMode() || !Replication.IsServer() || !RunOn())
			return;
		s_Active = this;
		GetGame().GetCallqueue().CallLater(Begin, 1000, false);
	}

	override void OnDelete(IEntity owner)
	{
		if (s_Active == this)
		{
			DCO_TacticTest.Clear();
			if (!m_bDone && m_fStart_s > 0)
				Finish("aborted");
			GetGame().GetCallqueue().Remove(Tick);
			GetGame().GetCallqueue().Remove(DCO_TickDuck);
			GetGame().GetCallqueue().Remove(DCO_SwapGlSmoke);
			s_Active = null;
			s_aTraced = null;
		}
		super.OnDelete(owner);
	}

	protected void ApplyRandomObjective()
	{
		if (m_sRandomObjective.IsEmpty() || !m_aRandomSites || m_aRandomSites.IsEmpty())
			return;
		IEntity e = GetGame().GetWorld().FindEntityByName(m_sRandomObjective);
		CMD_AICommanderObjectiveComponent oc;
		if (e)
			oc = CMD_AICommanderObjectiveComponent.Cast(e.FindComponent(CMD_AICommanderObjectiveComponent));
		if (!oc)
		{
			DCO_Log.Write(DCO_ELogChannel.TEST, DCO_ELogLevel.WARN, "-", string.Format("itest_random_obj_missing code=%1 name=%2", m_sCode, m_sRandomObjective));
			return;
		}
		int roll = Math.RandomInt(0, m_aRandomSites.Count());
		array<string> t = {};
		m_aRandomSites[roll].Split(" ", t, true);
		if (t.Count() < 4)
		{
			DCO_Log.Write(DCO_ELogChannel.TEST, DCO_ELogLevel.WARN, "-", string.Format("itest_random_obj_bad_site code=%1 entry=%2", m_sCode, m_aRandomSites[roll]));
			return;
		}
		vector p = Vector(t[0].ToFloat(), t[1].ToFloat(), t[2].ToFloat());
		e.SetOrigin(p);
		e.Update();
		oc.SetRadius(t[3].ToFloat());
		string label = "-";
		if (t.Count() > 4)
			label = t[4];
		m_vCameraFocus = p + Vector(0, 120, 300);
		DCO_Perf.Count("itest_random_obj:" + label);
		DCO_Log.Write(DCO_ELogChannel.TEST, DCO_ELogLevel.INFO, "-", string.Format("itest_random_obj code=%1 obj=%2 site=%3 pos=%4 radius=%5 roll=%6/%7", m_sCode, oc.GetLogName(), label, DCO_Util.XZ(p), t[3], roll + 1, m_aRandomSites.Count()));
	}

	protected void FocusCamera()
	{
		vector p = m_vCameraFocus;
		if (p == vector.Zero && GetOwner())
			p = GetOwner().GetOrigin();
		if (p == vector.Zero && GetGame().GetAIWorld())
		{
			array<AIAgent> agents = {};
			GetGame().GetAIWorld().GetAIAgents(agents);
			int n;
			foreach (AIAgent a : agents)
			{
				if (!a || !a.GetControlledEntity())
					continue;
				p = p + a.GetControlledEntity().GetOrigin();
				n++;
			}
			if (n > 0)
				p = p * (1.0 / n);
		}
		if (p == vector.Zero || !GetGame().GetCameraManager())
			return;
		CameraBase cam = GetGame().GetCameraManager().CurrentCamera();
		if (!cam)
			return;
		vector mat[4];
		Math3D.AnglesToMatrix(Vector(0, -45, 0), mat);
		mat[3] = p + Vector(0, 80, -80);
		cam.SetTransform(mat);
		DCO_Log.Write(DCO_ELogChannel.TEST, DCO_ELogLevel.INFO, "-", string.Format("itest_camera code=%1 focus=%2,%3 cam=%4", m_sCode, Math.Round(p[0]), Math.Round(p[2]), cam.ClassName()));
	}

	protected void Begin()
	{
		m_fStart_s = DCO_Util.NowS();

		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetInstance();
		if (cfg)
		{
			cfg.SetPerfProfiling(true);
			cfg.SetCensus(true);
			for (int bit = 1; bit > 0 && bit <= m_iDisabledBehaviors; bit = bit << 1)
			{
				if (m_iDisabledBehaviors & bit)
					cfg.SetBehaviorFlag(bit, false);
			}
		}

		if (m_fTimeOfDay >= 0)
		{
			ChimeraWorld world = ChimeraWorld.CastFrom(GetGame().GetWorld());
			if (world && world.GetTimeAndWeatherManager())
				world.GetTimeAndWeatherManager().SetTimeOfTheDay(m_fTimeOfDay, true);
		}

		if (m_bResetLearning)
		{
			int cleared = DCO_CmdLearn.ResetAll();
			int clearedDef = DCO_LearnDefense.ResetAll();
			int clearedBld = DCO_BldLearn.ResetAll();
			int clearedInd = DCO_IndLearn.ResetAll();
			DCO_Perf.Count("learn_reset");
			DCO_Log.Write(DCO_ELogChannel.TEST, DCO_ELogLevel.INFO, "-", string.Format("learn_reset code=%1 tactic_terrain=%2 defense=%3 bld=%4 ind=%5 by=test_case", m_sCode, cleared, clearedDef, clearedBld, clearedInd));
		}

		DCO_GlobalAIComponent gai = DCO_GlobalAIComponent.GetInstance();
		if (gai && m_bVanillaLOD)
			DCO_Log.Write(DCO_ELogChannel.TEST, DCO_ELogLevel.INFO, "-", string.Format("itest_lod code=%1 from=%2 to=%2 rule=perf_vanilla", m_sCode, typename.EnumToString(DCO_EAILODMode, gai.GetLODMode())));
		else if (gai)
		{
			int before = gai.GetLODMode();
			gai.SetLODMode(DCO_EAILODMode.FULL_DETAIL);
			int swept = DCO_SweepTestLOD();
			m_fNextLODSweep_s = DCO_Util.NowS() + 5.0;
			DCO_Log.Write(DCO_ELogChannel.TEST, DCO_ELogLevel.INFO, "-", string.Format("itest_lod code=%1 from=%2 to=%3 rule=tests_full_detail all_ai=%4", m_sCode, typename.EnumToString(DCO_EAILODMode, before), typename.EnumToString(DCO_EAILODMode, gai.GetLODMode()), swept));
		}

		ApplyRandomObjective();
		if (!m_sTacticCommander.IsEmpty())
			DCO_TacticTest.Setup(m_sCode, m_sTacticCommander, m_sTacticObjective, m_eForceTactic, m_fShapingRadius);

		FocusCamera();
		GetGame().GetCallqueue().CallLater(FocusCamera, 3000, false);

		string capS;
		if (System.GetCLIParam("dcoTestMax", capS) && capS.ToFloat() > 0 && m_fDuration > capS.ToFloat())
		{
			DCO_Log.Write(DCO_ELogChannel.TEST, DCO_ELogLevel.INFO, "-", string.Format("itest_cap code=%1 from=%2 to=%3", m_sCode, m_fDuration, capS.ToFloat()));
			m_fDuration = capS.ToFloat();
		}
		DCO_Log.Write(DCO_ELogChannel.TEST, DCO_ELogLevel.INFO, "-", string.Format("itest_start code=%1 name=%2 duration=%3 disabled=%4", m_sCode, m_sName, m_fDuration, DisabledNames()));
		ApplyLogLevels();
		ApplyTraceGroups();
		ApplyTaskGroups();
		ApplyFeatureOff();
		ApplyTune();
		if (m_aGlSmokeGroups && !m_aGlSmokeGroups.IsEmpty())
			GetGame().GetCallqueue().CallLater(DCO_SwapGlSmoke, 8000, false);
		Notify(string.Format("ITEST %1 %2", m_sCode, m_sName), string.Format("%1\nDuration %2 s. Disabled: %3", m_sIntent, Math.Round(m_fDuration), DisabledNames()), 20);
		GetGame().GetCallqueue().CallLater(Tick, 1000, true);
		if (m_aDuckGroups && !m_aDuckGroups.IsEmpty())
			GetGame().GetCallqueue().CallLater(DCO_TickDuck, 200, true);
	}

	protected void DCO_ProbeActions(float t)
	{
		AIWorld aiWorld = GetGame().GetAIWorld();
		if (!aiWorld)
			return;
		array<AIAgent> agents = {};
		aiWorld.GetAIAgents(agents);
		array<ref AIActionBase> acts = {};
		map<string, int> byClass = new map<string, int>();
		int soldiers, total, maxQ, obsolete;
		foreach (AIAgent a : agents)
		{
			if (!a || AIGroup.Cast(a))
				continue;
			SCR_AIUtilityComponent u = SCR_AIUtilityComponent.Cast(a.FindComponent(SCR_AIUtilityComponent));
			if (!u)
				continue;
			soldiers++;
			acts.Clear();
			u.FindActionsOfInheritedType(AIActionBase, acts);
			total += acts.Count();
			maxQ = Math.Max(maxQ, acts.Count());
			foreach (AIActionBase act : acts)
			{
				if (!act)
					continue;
				EAIActionState st = act.GetActionState();
				if (st == EAIActionState.COMPLETED || st == EAIActionState.FAILED)
					obsolete++;
				string cn = act.ClassName();
				byClass.Set(cn, byClass.Get(cn) + 1);
			}
		}
		string top;
		for (int k = 0; k < 8 && !byClass.IsEmpty(); k++)
		{
			string best;
			int bestN = -1;
			foreach (string cn2, int n2 : byClass)
			{
				if (n2 > bestN)
				{
					bestN = n2;
					best = cn2;
				}
			}
			top += string.Format(" %1=%2", best, bestN);
			byClass.Remove(best);
		}
		float avg = total;
		if (soldiers > 0)
			avg = avg / soldiers;
		DCO_Log.Write(DCO_ELogChannel.TEST, DCO_ELogLevel.INFO, "-", string.Format("itest_actions t=%1 soldiers=%2 total=%3 avg=%4 max=%5 obsolete=%6 top:%7", Math.Round(t), soldiers, total, avg.ToString(-1, 2), maxQ, obsolete, top));
	}

	protected int DCO_SweepTestLOD()
	{
		AIWorld aiWorld = GetGame().GetAIWorld();
		if (!aiWorld)
			return 0;
		array<AIAgent> agents = {};
		aiWorld.GetAIAgents(agents);
		int n;
		foreach (AIAgent a : agents)
		{
			if (!a || AIGroup.Cast(a))
				continue;
			SCR_AIUtilityComponent u = SCR_AIUtilityComponent.Cast(a.FindComponent(SCR_AIUtilityComponent));
			if (!u || u.m_iDCO_LOD == DCO_EAILODMode.FULL_DETAIL)
				continue;
			AIGroup g = a.GetParentGroup();
			if (g)
			{
				DCO_GroupUtilityComponent gu = DCO_GroupUtilityComponent.Cast(g.FindComponent(DCO_GroupUtilityComponent));
				if (gu && gu.GetLODOverride() != DCO_EGroupLOD.INHERIT)
					continue;
			}
			DCO_GlobalAIComponent.ApplyLOD(a, DCO_EAILODMode.FULL_DETAIL);
			n++;
		}
		return n;
	}

	protected void ApplyLogLevels()
	{
		if (!m_aLogLevels || m_aLogLevels.IsEmpty())
			return;
		string applied;
		foreach (DCO_TestLogLevel ll : m_aLogLevels)
		{
			if (!ll)
				continue;
			DCO_Log.SetLevel(ll.m_eChannel, ll.m_eLevel);
			if (!applied.IsEmpty())
				applied += "|";
			applied += typename.EnumToString(DCO_ELogChannel, ll.m_eChannel) + "=" + typename.EnumToString(DCO_ELogLevel, ll.m_eLevel);
		}
		DCO_Log.Write(DCO_ELogChannel.TEST, DCO_ELogLevel.INFO, "-", string.Format("itest_log_levels code=%1 levels=%2", m_sCode, applied));
	}

	protected void ApplyTraceGroups()
	{
		s_aTraced = {};
		if (!m_aTraceGroups || m_aTraceGroups.IsEmpty())
			return;
		string names;
		foreach (string gn : m_aTraceGroups)
		{
			if (!names.IsEmpty())
				names += "|";
			names += gn;
			AIGroup tg = AIGroup.Cast(GetGame().GetWorld().FindEntityByName(gn));
			if (tg)
			{
				s_aTraced.Insert(tg);
				DCO_BenchmarkLoggerComponent.AddTestGroup(tg);
			}
			else
				DCO_Log.Write(DCO_ELogChannel.TEST, DCO_ELogLevel.WARN, "-", "itest_trace_missing name=" + gn);
		}
		DCO_Log.Write(DCO_ELogChannel.TEST, DCO_ELogLevel.INFO, "-", string.Format("itest_trace_groups code=%1 groups=%2 found=%3", m_sCode, names, s_aTraced.Count()));
	}

	protected void DCO_SwapGlSmoke()
	{
		foreach (string gn : m_aGlSmokeGroups)
		{
			AIGroup g = AIGroup.Cast(GetGame().GetWorld().FindEntityByName(gn));
			if (!g)
				continue;
			array<AIAgent> agents = {};
			g.GetAgents(agents);
			foreach (AIAgent a : agents)
			{
				IEntity e = a.GetControlledEntity();
				if (!e)
					continue;
				SCR_InventoryStorageManagerComponent inv = SCR_InventoryStorageManagerComponent.Cast(e.FindComponent(SCR_InventoryStorageManagerComponent));
				if (!inv)
					continue;
				array<IEntity> items = {};
				inv.GetItems(items);
				int removed;
				foreach (IEntity it : items)
				{
					if (!it || !it.GetPrefabData() || !it.GetPrefabData().GetPrefabName().Contains("Ammo_Grenade_HEDP"))
						continue;
					if (it.GetParent() && it.GetParent().FindComponent(BaseWeaponComponent))
						continue;
					if (inv.TryDeleteItem(it))
						removed++;
				}
				int spawned;
				for (int k = 0; k < removed; k++)
				{
					if (inv.TrySpawnPrefabToStorage("{BDD18BEEC1165353}Prefabs/Weapons/Ammo/Ammo_Grenade_Smoke_M713_Red.et"))
						spawned++;
				}
				if (removed == 0)
					continue;
				DCO_Perf.Count("test:gl_smoke_swap");
				DCO_Log.Write(DCO_ELogChannel.TEST, DCO_ELogLevel.INFO, "-", string.Format("itest_gl_smoke code=%1 grp=%2 unit=%3 removed_hedp=%4 spawned_smoke=%5", m_sCode, gn, DCO_Log.IdUnit(e), removed, spawned));
			}
		}
	}

	protected void ApplyTune()
	{
		if (!m_aTune)
			return;
		array<DCO_GlobalAIComponent> comps = {};
		comps.Insert(DCO_GlobalAIComponent.GetInstance());
		array<string> facs = {"US", "USSR", "FIA"};
		foreach (string fk : facs)
		{
			DCO_GlobalAIComponent fc = DCO_GlobalAIComponent.GetForFaction(fk);
			if (fc && !comps.Contains(fc))
				comps.Insert(fc);
		}
		foreach (string entry : m_aTune)
		{
			array<string> p = {};
			entry.Split(" ", p, true);
			if (p.Count() < 2)
				continue;
			foreach (DCO_GlobalAIComponent c : comps)
			{
				if (!c)
					continue;
				if (p[0] == "aggression")
					c.DCO_SetAggression(p[1].ToFloat());
				else
					c.DCO_TuneSet(p[0], p[1].ToFloat());
			}
			DCO_Log.Write(DCO_ELogChannel.TEST, DCO_ELogLevel.INFO, "-", string.Format("itest_tune code=%1 key=%2 value=%3 comps=%4", m_sCode, p[0], p[1], comps.Count()));
		}
	}

	protected void ApplyFeatureOff()
	{
		if (!m_aGroupFeatureOff)
			return;
		foreach (string entry : m_aGroupFeatureOff)
		{
			array<string> p = {};
			entry.Split(" ", p, true);
			SCR_AIGroup g;
			if (p.Count() >= 2)
				g = SCR_AIGroup.Cast(GetGame().GetWorld().FindEntityByName(p[0]));
			if (!g)
			{
				DCO_Log.Write(DCO_ELogChannel.TEST, DCO_ELogLevel.WARN, "-", "itest_feature_off_missing entry=" + entry);
				continue;
			}
			g.DCO_SetFeatureOff(g.DCO_GetFeatureOff() | p[1].ToInt(), "test_case");
			DCO_Log.Write(DCO_ELogChannel.TEST, DCO_ELogLevel.INFO, "-", string.Format("itest_feature_off code=%1 grp=%2 mask=%3", m_sCode, p[0], p[1]));
		}
	}

	protected void ApplyTaskGroups()
	{
		if (!m_aTaskGroups || m_aTaskGroups.IsEmpty())
			return;
		foreach (string gn : m_aTaskGroups)
		{
			IEntity ge = GetGame().GetWorld().FindEntityByName(gn);
			DCO_GroupUtilityComponent gu;
			if (ge)
				gu = DCO_GroupUtilityComponent.Cast(ge.FindComponent(DCO_GroupUtilityComponent));
			if (!gu)
			{
				DCO_Log.Write(DCO_ELogChannel.TEST, DCO_ELogLevel.WARN, "-", "itest_task_missing name=" + gn);
				continue;
			}
			gu.SetTask(m_eTaskGroupsTask);
			DCO_BenchmarkLoggerComponent.AddTestGroup(AIGroup.Cast(ge));
			DCO_Perf.Count("itest_task");
			DCO_Log.Write(DCO_ELogChannel.TEST, DCO_ELogLevel.INFO, "-", string.Format("itest_task code=%1 grp=%2 task=%3 result=%4", m_sCode, gn, typename.EnumToString(DCO_EGroupTask, m_eTaskGroupsTask), typename.EnumToString(DCO_EGroupTask, gu.GetTask())));
		}
	}

	protected void DCO_OrderSmokeThrow(vector pos, float t)
	{
		SCR_AIUtilityComponent best;
		float bestD = 45.0;
		if (m_aTraceGroups)
		{
			foreach (string gn : m_aTraceGroups)
			{
				AIGroup g = AIGroup.Cast(GetGame().GetWorld().FindEntityByName(gn));
				if (!g)
					continue;
				array<AIAgent> agents = {};
				g.GetAgents(agents);
				foreach (AIAgent a : agents)
				{
					SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
					if (!ca || !ca.m_UtilityComponent || !ca.m_UtilityComponent.m_CombatComponent || !ca.GetControlledEntity())
						continue;
					if (!ca.m_UtilityComponent.m_CombatComponent.HasWeaponOfType(EWeaponType.WT_SMOKEGRENADE) || ca.m_UtilityComponent.m_CombatComponent.GetSelectedWeaponType() == EWeaponType.WT_MACHINEGUN)
						continue;
					float d = vector.Distance(ca.GetControlledEntity().GetOrigin(), pos);
					if (d < bestD)
					{
						bestD = d;
						best = ca.m_UtilityComponent;
					}
				}
			}
		}
		if (best)
			best.AddAction(new SCR_AIThrowGrenadeToBehavior(best, null, pos, EWeaponType.WT_SMOKEGRENADE, 0, SCR_AIActionBase.PRIORITY_BEHAVIOR_ATTACK_HIGH_PRIORITY + 10));
		string who = "none";
		if (best)
			who = DCO_Log.IdUnit(best.m_OwnerEntity);
		DCO_Log.Write(DCO_ELogChannel.TEST, DCO_ELogLevel.INFO, "-", string.Format("itest_smoke_throw code=%1 pos=%2 thrower=%3 dist=%4 t=%5", m_sCode, DCO_Util.XZ(pos), who, Math.Round(bestD), Math.Round(t)));
	}

	protected void DCO_TickSmokes(float t)
	{
		if (!m_aSmokeEvents)
			return;
		for (int i = 0; i < m_aSmokeEvents.Count(); i++)
		{
			if (m_aSmokeSpawned.Contains(i))
				continue;
			array<string> p = {};
			m_aSmokeEvents[i].Split(" ", p, true);
			if (p.Count() < 4 || t < p[0].ToFloat())
				continue;
			m_aSmokeSpawned.Insert(i);
			vector pos = DCO_Util.Surface(Vector(p[1].ToFloat(), p[2].ToFloat(), p[3].ToFloat()));
			if (p.Count() >= 5 && p[4] == "throw")
			{
				DCO_OrderSmokeThrow(pos, t);
				continue;
			}
			EntitySpawnParams sp = new EntitySpawnParams();
			sp.TransformMode = ETransformMode.WORLD;
			sp.Transform[3] = pos;
			IEntity e = GetGame().SpawnEntityPrefab(Resource.Load("{5C61F07CE0B1DBB2}Prefabs/Editor/EffectsModuleEntities/BaseEffectModule_Particle_Smoke.et"), GetGame().GetWorld(), sp);
			DCO_SmokeField.Add(e, pos, "test", 0.5, 60.0);
			DCO_Log.Write(DCO_ELogChannel.TEST, DCO_ELogLevel.INFO, "-", string.Format("itest_smoke code=%1 pos=%2 spawned=%3 t=%4", m_sCode, DCO_Util.XZ(pos), e != null, Math.Round(t)));
		}
	}

	protected void DCO_TickDuck()
	{
		if (m_bDone || !m_aDuckGroups || m_aDucked.Count() >= m_aDuckGroups.Count())
		{
			GetGame().GetCallqueue().Remove(DCO_TickDuck);
			return;
		}
		array<AIAgent> all = {};
		GetGame().GetAIWorld().GetAIAgents(all);
		foreach (string gn : m_aDuckGroups)
		{
			if (m_aDucked.Contains(gn))
				continue;
			string gname = gn;
			bool keepAi;
			int ci = gn.IndexOf(":");
			if (ci > 0)
			{
				gname = gn.Substring(0, ci);
				keepAi = gn.Substring(ci + 1, gn.Length() - ci - 1) == "ai";
			}
			AIGroup g = AIGroup.Cast(GetGame().GetWorld().FindEntityByName(gname));
			if (!g)
				continue;
			array<AIAgent> members = {};
			g.GetAgents(members);
			array<IEntity> ents = {};
			foreach (AIAgent m : members)
			{
				if (m && m.GetControlledEntity())
					ents.Insert(m.GetControlledEntity());
			}
			foreach (AIAgent a : all)
			{
				SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
				if (!ca || !ca.m_UtilityComponent || !ca.m_UtilityComponent.m_CombatComponent || members.Contains(a))
					continue;
				BaseTarget t = ca.m_UtilityComponent.m_CombatComponent.GetCurrentTarget();
				if (!t || !ents.Contains(t.GetTargetEntity()) || t.GetTimeSinceSeen() > 1.0)
					continue;
				m_aDucked.Insert(gn);
				DCO_Duck(gname, members, ca.GetControlledEntity(), keepAi);
				break;
			}
		}
	}

	protected void DCO_Duck(string gn, array<AIAgent> members, IEntity observer, bool keepAi)
	{
		vector oEye = observer.GetOrigin() + vector.Up * 1.6;
		vector from = members[0].GetControlledEntity().GetOrigin();
		float oDist = vector.DistanceXZ(from, observer.GetOrigin());
		vector spot;
		float best = float.MAX;
		for (int k = 0; k < 24; k++)
		{
			float ang = k * 15.0 * Math.DEG2RAD;
			int ring = k % 3;
			float rr = 10.0 + ring * 5.0;
			vector q;
			if (!DCO_FlushDrill.Reachable(from + Vector(Math.Sin(ang) * rr, 0, Math.Cos(ang) * rr), q))
				continue;
			if (vector.DistanceXZ(q, observer.GetOrigin()) < oDist - 5.0)
				continue;
			string bm;
			if (SCR_CoverManagerComponent.DCO_GetBuildingAtPos(q, null, bm))
				continue;
			if (DCO_Util.TraceFrac(oEye, q + vector.Up * 1.0, observer, TraceFlags.WORLD | TraceFlags.ENTS, -1, "") >= 0.98)
				continue;
			if (DCO_Util.TraceFrac(oEye, q + vector.Up * 1.7, observer, TraceFlags.WORLD | TraceFlags.ENTS, -1, "") >= 0.98)
				continue;
			float d = vector.DistanceXZ(q, from);
			if (d < best)
			{
				best = d;
				spot = q;
			}
		}
		int moved;
		if (best < float.MAX)
		{
			foreach (int i, AIAgent m : members)
			{
				BaseGameEntity e = BaseGameEntity.Cast(m.GetControlledEntity());
				if (!e)
					continue;
				SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(m);
				if (!keepAi && ca && ca.GetControlComponent())
					ca.GetControlComponent().DeactivateAI();
				vector tm[4];
				e.GetWorldTransform(tm);
				tm[3] = DCO_Util.Surface(spot + Vector(1.0 * i * 1.2, 0, 0));
				e.Teleport(tm);
				moved++;
			}
		}
		DCO_Perf.Count("test:duck");
		if (moved == 0)
			DCO_Perf.Count("test:duck_no_spot");
		DCO_Log.Write(DCO_ELogChannel.TEST, DCO_ELogLevel.INFO, "-", string.Format("itest_duck code=%1 grp=%2 observer=%3 obs_dist=%4 moved=%5 shift=%6 spot=%7 keep_ai=%8",
			m_sCode, gn, DCO_Log.IdUnit(observer), Math.Round(oDist), moved, Math.Round(best), DCO_Util.XZ(spot), keepAi));
	}

	protected void DCO_TickWorldEvents(float t)
	{
		if (!m_aWorldEvents)
			return;
		for (int i = 0; i < m_aWorldEvents.Count(); i++)
		{
			if (m_aWorldEventsDone.Contains(i))
				continue;
			array<string> p = {};
			m_aWorldEvents[i].Split(" ", p, true);
			if (p.Count() < 3)
				continue;
			if (p[0].StartsWith("@"))
			{
				string trig = p[0].Substring(1, p[0].Length() - 1);
				if (m_mEvents.Get(trig) < 1)
					continue;
				DCO_Perf.Count("test:event_trigger");
				DCO_Log.Write(DCO_ELogChannel.TEST, DCO_ELogLevel.INFO, "-", string.Format("itest_event_trigger code=%1 on=%2 t=%3 entry=%4", m_sCode, trig, Math.Round(t), m_aWorldEvents[i]));
			}
			else if (t < p[0].ToFloat())
				continue;
			m_aWorldEventsDone.Insert(i);
			if (p[1] == "kill" && p.Count() >= 4)
				DCO_TestKill(p[2], p[3].ToFloat(), t);
			else if (p[1] == "panic" && p.Count() >= 5)
				DCO_TestPanic(p[2], p[3].ToFloat(), p[4].ToFloat(), t);
			else if (p[1] == "spawn" && p.Count() >= 6)
				DCO_TestSpawn(p[2], Vector(p[3].ToFloat(), p[4].ToFloat(), p[5].ToFloat()), t);
			else if (p[1] == "ammo" && p.Count() >= 4)
				DCO_TestAmmo(p[2], p[3].ToInt(), t);
			else if (p[1] == "wound" && p.Count() >= 4)
				DCO_TestWound(p[2], p[3].ToInt(), t);
			else if (p[1] == "breakcontact" && p.Count() >= 5)
				DCO_TestBreakContact(p[2], Vector(p[3].ToFloat(), 0, p[4].ToFloat()), t);
			else
				DCO_Log.Write(DCO_ELogChannel.TEST, DCO_ELogLevel.WARN, "-", string.Format("itest_event_bad code=%1 entry=%2", m_sCode, m_aWorldEvents[i]));
		}
	}

	protected void DCO_TestWound(string names, int count, float t)
	{
		array<string> groupNames = {};
		names.Split(",", groupNames, true);
		int total;
		foreach (string gn : groupNames)
		{
			AIGroup g = AIGroup.Cast(GetGame().GetWorld().FindEntityByName(gn));
			if (!g)
			{
				DCO_Log.Write(DCO_ELogChannel.TEST, DCO_ELogLevel.WARN, "-", string.Format("itest_wound_bad code=%1 group=%2 reason=not_found", m_sCode, gn));
				continue;
			}
			array<AIAgent> agents = {};
			g.GetAgents(agents);
			int done;
			foreach (AIAgent a : agents)
			{
				if (done >= count)
					break;
				IEntity e = a.GetControlledEntity();
				if (!e || !DCO_Util.IsAlive(e))
					continue;
				SCR_CharacterDamageManagerComponent dmg = SCR_CharacterDamageManagerComponent.Cast(e.FindComponent(SCR_CharacterDamageManagerComponent));
				if (!dmg)
					continue;
				string hz = DCO_TestLegZone(dmg);
				dmg.AddParticularBleeding(hz, ECharacterDamageState.WOUNDED);
				done++;
				DCO_Log.Write(DCO_ELogChannel.TEST, DCO_ELogLevel.INFO, DCO_Log.IdUnit(e), string.Format("itest_wound code=%1 grp=%2 unit=%3 zone=%4 t=%5", m_sCode, gn, DCO_Log.IdUnit(e), hz, Math.Round(t)));
			}
			total += done;
		}
		DCO_Perf.CountN("test:event_wound", total);
	}

	protected string DCO_TestLegZone(SCR_CharacterDamageManagerComponent dmg)
	{
		array<HitZone> zones = {};
		dmg.GetPhysicalHitZones(zones);
		foreach (HitZone z : zones)
		{
			if (!z)
				continue;
			string n = z.GetName();
			if (n.Contains("Thigh") || n.Contains("Leg") || n.Contains("Calf"))
				return n;
		}
		return "Chest";
	}

	protected void DCO_TestBreakContact(string names, vector threat, float t)
	{
		array<string> groupNames = {};
		names.Split(",", groupNames, true);
		int started;
		foreach (string gn : groupNames)
		{
			AIGroup g = AIGroup.Cast(GetGame().GetWorld().FindEntityByName(gn));
			if (!g)
				continue;
			DCO_GroupUtilityComponent u = DCO_GroupUtilityComponent.Cast(g.FindComponent(DCO_GroupUtilityComponent));
			if (!u)
				continue;
			threat[1] = GetGame().GetWorld().GetSurfaceY(threat[0], threat[2]);
			bool ok = DCO_BreakContact.Start(u, threat, vector.Zero, "def_test");
			if (ok)
				started++;
			DCO_Log.Write(DCO_ELogChannel.TEST, DCO_ELogLevel.INFO, "-", string.Format("itest_breakcontact code=%1 grp=%2 ok=%3 t=%4", m_sCode, gn, ok, Math.Round(t)));
		}
		DCO_Perf.CountN("test:event_breakcontact", started);
	}

	protected void DCO_TestAmmo(string names, int keep, float t)
	{
		array<string> groupNames = {};
		names.Split(",", groupNames, true);
		int total;
		foreach (string gn : groupNames)
		{
			AIGroup g = AIGroup.Cast(GetGame().GetWorld().FindEntityByName(gn));
			if (!g)
			{
				DCO_Log.Write(DCO_ELogChannel.TEST, DCO_ELogLevel.WARN, "-", string.Format("itest_ammo_bad code=%1 group=%2 reason=not_found", m_sCode, gn));
				continue;
			}
			array<AIAgent> agents = {};
			g.GetAgents(agents);
			foreach (AIAgent a : agents)
			{
				IEntity e = a.GetControlledEntity();
				if (!e)
					continue;
				SCR_AICombatComponent combat = SCR_AICombatComponent.Cast(e.FindComponent(SCR_AICombatComponent));
				SCR_InventoryStorageManagerComponent inv = SCR_InventoryStorageManagerComponent.Cast(e.FindComponent(SCR_InventoryStorageManagerComponent));
				typename well;
				int before = DCO_Rearm.SpareMags(combat, e, well);
				if (!inv || before < 0)
					continue;
				array<IEntity> items = {};
				array<typename> comps = {};
				comps.Insert(MagazineComponent);
				inv.FindItemsWithComponents(items, comps);
				int kept;
				int removed;
				foreach (IEntity it : items)
				{
					if (!it)
						continue;
					if (it.GetParent() && it.GetParent().FindComponent(BaseWeaponComponent))
						continue;
					MagazineComponent mag = MagazineComponent.Cast(it.FindComponent(MagazineComponent));
					if (!mag || !mag.GetMagazineWell() || mag.GetMagazineWell().Type() != well)
						continue;
					if (kept < keep)
					{
						kept++;
						continue;
					}
					if (inv.TryDeleteItem(it))
						removed++;
				}
				total += removed;
				DCO_Log.Write(DCO_ELogChannel.TEST, DCO_ELogLevel.INFO, DCO_Log.IdUnit(e), string.Format("itest_ammo code=%1 grp=%2 unit=%3 before=%4 after=%5 removed=%6 t=%7", m_sCode, gn, DCO_Log.IdUnit(e), before, DCO_Rearm.SpareMags(combat, e, well), removed, Math.Round(t)));
			}
		}
		DCO_Perf.CountN("test:ammo_removed", total);
	}

	protected void DCO_TestKill(string names, float fraction, float t)
	{
		array<string> groupNames = {};
		names.Split(",", groupNames, true);
		array<IEntity> alive = {};
		foreach (string gn : groupNames)
		{
			SCR_AIGroup g = SCR_AIGroup.Cast(GetGame().GetWorld().FindEntityByName(gn));
			if (!g)
				continue;
			array<AIAgent> agents = {};
			g.GetAgents(agents);
			foreach (AIAgent a : agents)
			{
				if (a && a.GetControlledEntity() && DCO_Util.IsAlive(a.GetControlledEntity()))
					alive.Insert(a.GetControlledEntity());
			}
		}
		int want = (int)Math.Round(fraction * alive.Count());
		int killed;
		for (int k = alive.Count() - 1; k >= 0 && killed < want; k--)
		{
			SCR_CharacterDamageManagerComponent dmg = SCR_CharacterDamageManagerComponent.Cast(alive[k].FindComponent(SCR_CharacterDamageManagerComponent));
			if (!dmg)
				continue;
			dmg.Kill(Instigator.CreateInstigator(null));
			killed++;
		}
		DCO_Perf.CountN("test:event_kill", killed);
		DCO_Log.Write(DCO_ELogChannel.TEST, DCO_ELogLevel.INFO, "-", string.Format("itest_kill code=%1 groups=%2 alive=%3 fraction=%4 killed=%5 t=%6", m_sCode, names, alive.Count(), fraction, killed, Math.Round(t)));
	}

	protected ref array<ref DCO_AIMoraleSystem> m_aPanicked = {};

	protected void DCO_TestPanic(string names, float fraction, float dur_s, float t)
	{
		array<string> groupNames = {};
		names.Split(",", groupNames, true);
		array<DCO_AIMoraleSystem> pool = {};
		foreach (string gn : groupNames)
		{
			SCR_AIGroup g = SCR_AIGroup.Cast(GetGame().GetWorld().FindEntityByName(gn));
			if (!g)
				continue;
			array<AIAgent> agents = {};
			g.GetAgents(agents);
			foreach (AIAgent a : agents)
			{
				if (!a || !a.GetControlledEntity() || !DCO_Util.IsAlive(a.GetControlledEntity()))
					continue;
				SCR_AIUtilityComponent u = SCR_AIUtilityComponent.Cast(a.FindComponent(SCR_AIUtilityComponent));
				if (u && u.GetMoraleSystem())
					pool.Insert(u.GetMoraleSystem());
			}
		}
		int want = (int)Math.Round(fraction * pool.Count());
		for (int k = 0; k < want && k < pool.Count(); k++)
		{
			pool[k].DCO_SetMoraleLevel(1.0, 3.2);
			m_aPanicked.Insert(pool[k]);
		}
		if (dur_s > 0)
			GetGame().GetCallqueue().CallLater(DCO_TestPanicRelease, (int)(dur_s * 1000.0), false);
		DCO_Perf.CountN("test:event_panic", want);
		DCO_Log.Write(DCO_ELogChannel.TEST, DCO_ELogLevel.INFO, "-", string.Format("itest_panic code=%1 groups=%2 alive=%3 n=%4 dur_s=%5 t=%6", m_sCode, names, pool.Count(), want, Math.Round(dur_s), Math.Round(t)));
	}

	protected void DCO_TestPanicRelease()
	{
		int n;
		foreach (DCO_AIMoraleSystem ms : m_aPanicked)
		{
			if (!ms)
				continue;
			ms.DCO_SetMoraleLevel(1.0, 0);
			n++;
		}
		m_aPanicked.Clear();
		DCO_Log.Write(DCO_ELogChannel.TEST, DCO_ELogLevel.INFO, "-", string.Format("itest_panic_release code=%1 n=%2", m_sCode, n));
	}

	protected void DCO_TestSpawn(string prefab, vector pos, float t)
	{
		EntitySpawnParams sp = new EntitySpawnParams();
		sp.TransformMode = ETransformMode.WORLD;
		sp.Transform[3] = DCO_Util.Surface(pos);
		IEntity e = GetGame().SpawnEntityPrefab(Resource.Load(prefab), GetGame().GetWorld(), sp);
		DCO_Perf.Count("test:event_spawn");
		DCO_Log.Write(DCO_ELogChannel.TEST, DCO_ELogLevel.INFO, "-", string.Format("itest_spawn code=%1 prefab=%2 pos=%3 spawned=%4 t=%5", m_sCode, FilePath.StripPath(prefab), DCO_Util.XZ(pos), e != null, Math.Round(t)));
	}

	protected void Tick()
	{
		DCO_TickSmokes(DCO_Util.NowS() - m_fStart_s);
		DCO_TickWorldEvents(DCO_Util.NowS() - m_fStart_s);
		if (m_bDone)
			return;
		if (!m_bVanillaLOD && DCO_Util.NowS() >= m_fNextLODSweep_s)
		{
			m_fNextLODSweep_s = DCO_Util.NowS() + 5.0;
			int late = DCO_SweepTestLOD();
			if (late > 0)
				DCO_Log.Write(DCO_ELogChannel.TEST, DCO_ELogLevel.INFO, "-", string.Format("itest_lod_sweep code=%1 newly_full=%2", m_sCode, late));
		}
		if (m_bActionProbe && DCO_Util.NowS() >= m_fNextActionProbe_s)
		{
			m_fNextActionProbe_s = DCO_Util.NowS() + 30.0;
			DCO_ProbeActions(DCO_Util.NowS() - m_fStart_s);
		}

		float t = DCO_Util.NowS() - m_fStart_s;
		bool allPositiveMet = true;
		bool hasNegative = false;
		if (m_aAsserts)
		{
			foreach (DCO_TestAssert a : m_aAsserts)
			{
				if (a.m_eKind == DCO_ETestAssertKind.COUNTER_AT_LEAST || a.m_eKind == DCO_ETestAssertKind.EVENT_PRESENT)
				{
					if (a.m_fMetAt < 0 && Actual(a) >= a.m_fValue)
					{
						a.m_fMetAt = t;
						Notify(string.Format("ITEST %1", m_sCode), string.Format("Met: %1 at %2 s", a.m_sKey, Math.Round(t)), 5);
					}
					if (a.m_fMetAt < 0)
						allPositiveMet = false;
				}
				else
				{
					hasNegative = true;
				}
			}
		}

		if (t >= m_fDuration)
		{
			Finish("timeout");
			return;
		}

		if (m_aAsserts && !m_aAsserts.IsEmpty() && allPositiveMet && !hasNegative && !m_bRunFull)
			Finish("all_met");
	}

	protected float Actual(DCO_TestAssert a)
	{
		switch (a.m_eKind)
		{
			case DCO_ETestAssertKind.COUNTER_AT_LEAST:
			case DCO_ETestAssertKind.COUNTER_ZERO:
				return DCO_Perf.TotalCount(a.m_sKey);
			case DCO_ETestAssertKind.EVENT_PRESENT:
			case DCO_ETestAssertKind.EVENT_ABSENT:
				return m_mEvents.Get(a.m_sKey);
			case DCO_ETestAssertKind.PERF_MAX_MS:
				return DCO_Perf.SectionMaxMs(a.m_sKey);
		}
		return 0;
	}

	protected bool Passed(DCO_TestAssert a)
	{
		float v = Actual(a);
		switch (a.m_eKind)
		{
			case DCO_ETestAssertKind.COUNTER_AT_LEAST:
			case DCO_ETestAssertKind.EVENT_PRESENT:
				return a.m_fMetAt >= 0 && (a.m_fWithin <= 0 || a.m_fMetAt <= a.m_fWithin);
			case DCO_ETestAssertKind.COUNTER_ZERO:
			case DCO_ETestAssertKind.EVENT_ABSENT:
				return v == 0;
			case DCO_ETestAssertKind.PERF_MAX_MS:
				return v <= a.m_fValue;
		}
		return false;
	}

	protected void Finish(string why)
	{
		m_bDone = true;
		DCO_Battle.End("itest_" + why);
		GetGame().GetCallqueue().Remove(Tick);

		bool pass = m_aAsserts && !m_aAsserts.IsEmpty();
		array<string> lines = {};
		if (m_aAsserts)
		{
			foreach (DCO_TestAssert a : m_aAsserts)
			{
				bool ok = Passed(a);
				pass = pass && ok;
				string res = "FAIL";
				if (ok)
					res = "PASS";
				lines.Insert(string.Format("  %1 %2 %3 actual=%4 want=%5 within=%6 met_at=%7", res, typename.EnumToString(DCO_ETestAssertKind, a.m_eKind), a.m_sKey, Actual(a), a.m_fValue, a.m_fWithin, a.m_fMetAt.ToString(-1, 1)));
			}
		}

		string verdict = "FAIL";
		if (pass)
			verdict = "PASS";
		float t = DCO_Util.NowS() - m_fStart_s;
		string head = string.Format("ITEST %1 %2 %3 t=%4 end=%5 disabled=%6", m_sCode, m_sName, verdict, Math.Round(t), why, DisabledNames());

		DCO_ELogLevel lvl = DCO_ELogLevel.INFO;
		if (!pass)
			lvl = DCO_ELogLevel.WARN;
		DCO_Log.Write(DCO_ELogChannel.TEST, lvl, "-", head);
		if (!m_sIntent.IsEmpty())
			DCO_Log.Write(DCO_ELogChannel.TEST, lvl, "-", "INTENT " + OneLine(m_sIntent));
		foreach (string l : lines)
		{
			DCO_Log.Write(DCO_ELogChannel.TEST, lvl, "-", "ASSERT " + l.Trim());
		}
		if (!m_sManualNotes.IsEmpty())
			DCO_Log.Write(DCO_ELogChannel.TEST, lvl, "-", "MANUAL " + OneLine(m_sManualNotes));
		DCO_Log.Write(DCO_ELogChannel.TEST, lvl, "-", string.Format("itest_done code=%1 verdict=%2 end=%3", m_sCode, verdict, why));
		DCO_Log.Flush();

		array<string> failed = {};
		if (m_aAsserts)
		{
			foreach (DCO_TestAssert fa : m_aAsserts)
			{
				if (!Passed(fa))
					failed.Insert(fa.m_sKey);
			}
		}
		string summary = "All checks passed.";
		if (!failed.IsEmpty())
			summary = "Failed: " + SCR_StringHelper.Join(", ", failed);
		Notify(string.Format("ITEST %1 %2: %3", m_sCode, m_sName, verdict), summary, 30);
		if (System.IsCLIParam("dcoTestExit"))
		{
			DCO_Perf.Count("itest_exit_req");
			DCO_Log.Write(DCO_ELogChannel.TEST, DCO_ELogLevel.INFO, "-", string.Format("itest_exit code=%1 verdict=%2 delay_ms=3000", m_sCode, verdict));
			DCO_Log.Flush();
			GetGame().GetCallqueue().CallLater(ExitGame, 3000, false);
		}
	}

	static void ExitGame()
	{
		DCO_Log.Flush();
		GetGame().RequestClose();
	}

	protected string OneLine(string s)
	{
		string r = s;
		r.Replace("\r", "");
		r.Replace("\n", " | ");
		return r;
	}

	protected void Notify(string title, string text, float seconds)
	{
		if (DCO_Util.Teardown("itest_notify"))
			return;
		DCO_PlayerComms.SendToGameMasters(title, text);
		SCR_HintManagerComponent.ShowCustomHint(text, title, seconds);
	}

	protected string DisabledNames()
	{
		if (m_iDisabledBehaviors == 0)
			return "none";
		array<string> names = {};
		for (int bit = 1; bit > 0 && bit <= m_iDisabledBehaviors; bit = bit << 1)
		{
			if (m_iDisabledBehaviors & bit)
				names.Insert(typename.EnumToString(DCO_EBehavior, bit));
		}
		return SCR_StringHelper.Join("|", names);
	}
}

modded class SCR_BaseGameMode
{
	override bool PauseGame(bool pause, SCR_EPauseReason reason = SCR_EPauseReason.SYSTEM)
	{
		if (pause && System.IsCLIParam("dcoTest"))
		{
			DCO_Perf.Count("itest_pause_blocked");
			DCO_Log.Write(DCO_ELogChannel.TEST, DCO_ELogLevel.INFO, "-", string.Format("itest_pause_blocked reason=%1", reason));
			return false;
		}
		return super.PauseGame(pause, reason);
	}
}
