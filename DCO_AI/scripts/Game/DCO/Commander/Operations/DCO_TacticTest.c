class DCO_TacticTest
{
	protected static bool s_bOn;
	protected static bool s_bAny;
	protected static bool s_bScope;
	protected static float s_fScopeR;
	protected static string s_sScopeLog;
	protected static string s_sHoldLog;
	protected static string s_sCode;
	protected static string s_sCmd;
	protected static string s_sObj;
	protected static int s_iTactic;
	protected static bool s_bSetupLogged;
	protected static float s_fSeedAt;
	protected static ref map<CMD_AICommanderObjectiveComponent, int> s_mEnds;
	protected static ref map<CMD_AICommanderObjectiveComponent, float> s_mReplanAt;
	protected static ref map<CMD_AICommanderObjectiveComponent, float> s_mDefMax;
	protected static ref map<CMD_AICommanderObjectiveComponent, float> s_mReplanFrom;
	protected static ref map<CMD_AICommanderObjectiveComponent, float> s_mReplanLogAt;
	protected static ref map<string, bool> s_mBypassLogged;
	protected static ref map<CMD_AICommanderObjectiveComponent, string> s_mHandOff;
	protected static CMD_AICommanderObjectiveComponent s_Pick;
	protected static string s_sKeepLog;
	protected static string s_sPickWhy;
	protected static CMD_AICommanderObjectiveComponent s_Main;
	protected static string s_sSplitLog;
	protected static string s_sMainLog;
	protected static string s_sRolesLog;
	protected static int s_iCaptured;

	static void Setup(string code, string cmdUid, string objName, int tactic, float scopeR = 600.0)
	{
		s_bOn = !cmdUid.IsEmpty();
		s_bScope = objName.IndexOf("scope:") == 0;
		if (s_bScope)
			objName = objName.Substring(6, objName.Length() - 6);
		s_fScopeR = scopeR;
		s_sScopeLog = "";
		s_sHoldLog = "";
		s_bAny = !s_bScope && (objName.IsEmpty() || objName == "*");
		s_sCode = code;
		s_sCmd = cmdUid;
		s_sObj = objName;
		s_iTactic = tactic;
		s_bSetupLogged = false;
		s_fSeedAt = 0;
		s_mEnds = new map<CMD_AICommanderObjectiveComponent, int>();
		s_mReplanAt = new map<CMD_AICommanderObjectiveComponent, float>();
		s_mDefMax = new map<CMD_AICommanderObjectiveComponent, float>();
		s_mReplanFrom = new map<CMD_AICommanderObjectiveComponent, float>();
		s_mReplanLogAt = new map<CMD_AICommanderObjectiveComponent, float>();
		s_mBypassLogged = new map<string, bool>();
		s_mHandOff = new map<CMD_AICommanderObjectiveComponent, string>();
		s_Pick = null;
		s_sKeepLog = "";
		s_Main = null;
		s_sSplitLog = "";
		s_sMainLog = "";
		s_sRolesLog = "";
		s_iCaptured = 0;
		DCO_Log.Write(DCO_ELogChannel.TEST, DCO_ELogLevel.INFO, "-", string.Format("itest_tactic_armed code=%1 obj=%2 tactic=%3 cmd=%4 on=%5 scope_r=%6", code, ObjText(), DCO_CommanderTactics.Name(tactic), cmdUid, s_bOn, Math.Round(scopeR)));
	}

	protected static string ObjText()
	{
		if (s_bAny)
			return "*";
		if (s_bScope)
			return "scope:" + s_sObj;
		return s_sObj;
	}

	protected static bool NameMatch(CMD_AICommanderObjectiveComponent o)
	{
		return o && o.GetOwner() && (o.GetOwner().GetName() == s_sObj || o.GetLogName() == s_sObj);
	}

	static CMD_AICommanderObjectiveComponent Primary(AICommander_BaseComponent cmd)
	{
		if (!ForCommander(cmd) || s_bAny)
			return null;
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (mgr)
		{
			foreach (CMD_AICommanderObjectiveComponent mo : mgr.m_aObjective)
			{
				if (NameMatch(mo))
					return mo;
			}
		}
		foreach (CMD_AICommanderObjectiveComponent o : cmd.GetObjectiveList())
		{
			if (NameMatch(o))
				return o;
		}
		return null;
	}

	static CMD_AICommanderObjectiveComponent Main(AICommander_BaseComponent cmd)
	{
		if (!ForCommander(cmd))
			return null;
		DCO_OpPlanner opp = cmd.DCO_GetOpPlanner();
		if (opp && opp.Main())
			return opp.Main();
		if (!s_Main || !Forces(cmd, s_Main))
			return null;
		return s_Main;
	}

	protected static void NoteMain(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent main, string why)
	{
		s_Main = main;
		string line = "op_main cmd=" + cmd.GetCommanderUID() + " obj=" + main.GetLogName() + " why=" + why;
		if (line == s_sMainLog)
			return;
		s_sMainLog = line;
		DCO_Perf.Count("op_main:" + why);
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(cmd), line);
	}

	static int ReserveGroups(AICommander_BaseComponent cmd, int maneuver)
	{
		return DCO_Force.Reserve(cmd, maneuver);
	}

	protected static int AvailableGroups(AICommander_BaseComponent cmd)
	{
		int n = 0;
		foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
		{
			if (!g || g.IsPlayerGroup() || g.IsMortar() || g.IsDedicatedTransport() || g.IsMedicTeam() || g.GetUnitCount() <= 0 || g.m_DCOSupportOf)
				continue;
			DCO_EGroupTask t = g.GetTask();
			bool idle = t == DCO_EGroupTask.NONE || t == DCO_EGroupTask.PATROL;
			bool onScope = g.GetGroupObjective() && Forces(cmd, g.GetGroupObjective());
			if (idle || onScope)
				n++;
		}
		return n;
	}

	protected static float Eff(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent o)
	{
		DCO_DoctrineRuntime rt = cmd.GetDoctrineRuntime();
		if (!rt)
			return 0;
		DCO_ForceEval e = rt.GetEval(o);
		if (!e)
			return 0;
		return e.m_fEff;
	}

	protected static bool Split(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent prim, CMD_AICommanderObjectiveComponent shaping)
	{
		int avail = AvailableGroups(cmd);
		int reserve = ReserveGroups(cmd, avail);
		int needP = cmd.GetRequiredAttackGroups(prim);
		int needS = cmd.GetRequiredAttackGroups(shaping);
		bool concurrent = needP + needS <= avail - reserve;
		float effP = Eff(cmd, prim);
		float effS = Eff(cmd, shaping);
		CMD_AICommanderObjectiveComponent main = prim;
		CMD_AICommanderObjectiveComponent support = shaping;
		string mwhy = "value";
		if (effP < 1.5 && effS >= 2.5)
		{
			main = shaping;
			support = prim;
			mwhy = "ratio";
		}
		if (!concurrent)
		{
			main = shaping;
			support = prim;
			mwhy = "sequential_shaping_first";
		}
		NoteMain(cmd, main, mwhy);
		string result = "sequential";
		string why = "need_exceeds_available";
		if (concurrent)
		{
			result = "concurrent";
			why = "need_fits";
		}
		int needMain = needP;
		int needSup = needS;
		if (main != prim)
		{
			needMain = needS;
			needSup = needP;
		}
		string line = string.Format("op_split cmd=%1 main=%2:%3 support=%4:%5 available=%6 reserve=%7 result=%8 why=%9", cmd.GetCommanderUID(), main.GetLogName(), needMain, support.GetLogName(), needSup, avail, reserve, result, why)
			+ string.Format(" support_mode=capture eff_primary=%1 eff_shaping=%2", effP.ToString(-1, 2), effS.ToString(-1, 2));
		string splitKey = main.GetLogName() + "|" + support.GetLogName() + "|" + result + "|" + needMain.ToString() + "|" + needSup.ToString() + "|" + avail.ToString() + "|" + reserve.ToString();
		if (splitKey != s_sSplitLog)
		{
			s_sSplitLog = splitKey;
			DCO_Perf.Count("op_split:" + result);
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(cmd), line);
		}
		return concurrent;
	}

	protected static array<CMD_AICommanderObjectiveComponent> ScopeList(AICommander_BaseComponent cmd)
	{
		array<CMD_AICommanderObjectiveComponent> outL = {};
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
			return outL;
		foreach (CMD_AICommanderObjectiveComponent o : mgr.m_aObjective)
		{
			if (o && Forces(cmd, o))
				outL.Insert(o);
		}
		return outL;
	}

	protected static CMD_AICommanderObjectiveComponent BestRatio(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent skip)
	{
		CMD_AICommanderObjectiveComponent best;
		float bestEff = -1;
		foreach (CMD_AICommanderObjectiveComponent o : ScopeList(cmd))
		{
			if (o == skip)
				continue;
			float e = Eff(cmd, o);
			if (e > bestEff)
			{
				bestEff = e;
				best = o;
			}
		}
		return best;
	}

	protected static int AssignRoles(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent prim, CMD_AICommanderObjectiveComponent pick, notnull array<CMD_AICommanderObjectiveComponent> ordered)
	{
		array<CMD_AICommanderObjectiveComponent> scope = ScopeList(cmd);
		if (scope.IsEmpty())
			return 1;
		CMD_AICommanderObjectiveComponent main = Main(cmd);
		if (!main && pick)
			main = pick;
		if (!main)
			main = prim;
		int avail = AvailableGroups(cmd);
		int reserve = ReserveGroups(cmd, avail);
		int remaining = avail - reserve - Math.Ceil(cmd.GetRequiredAttackGroups(main) * 1.2);
		string capText = "";
		string fixText = "";
		foreach (CMD_AICommanderObjectiveComponent o : scope)
		{
			if (o == main)
				continue;
			int needO = Math.Ceil(cmd.GetRequiredAttackGroups(o) * 1.2);
			if (ordered.Contains(o) || needO <= remaining)
			{
				if (!ordered.Contains(o))
					ordered.Insert(o);
				remaining -= needO;
				if (!capText.IsEmpty())
					capText += ",";
				capText += o.GetLogName();
				continue;
			}
			int want = 1;
			if (ScopeDefenders(cmd, o) > 15.0)
				want = 2;
			string grps = cmd.DCO_ScopeFix(o, want);
			if (!fixText.IsEmpty())
				fixText += ",";
			fixText += o.GetLogName() + ":" + grps;
		}
		if (capText.IsEmpty())
			capText = "-";
		if (fixText.IsEmpty())
			fixText = "-";
		string line = string.Format("op_roles cmd=%1 main=%2 capture=%3 fix=%4 available=%5 reserve=%6", cmd.GetCommanderUID(), main.GetLogName(), capText, fixText, avail, reserve);
		if (line != s_sRolesLog)
		{
			s_sRolesLog = line;
			DCO_Perf.Count("op_roles");
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(cmd), line);
		}
		return scope.Count();
	}

	static bool Taken(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent o)
	{
		if (!o || !o.IsCapturedBy(cmd.GetCommanderFactionKey()))
			return false;
		return true;
	}

	static bool ScopeActive(AICommander_BaseComponent cmd)
	{
		return ForCommander(cmd) && s_bScope;
	}

	static float ScopeRadius()
	{
		return s_fScopeR;
	}

	static CMD_AICommanderObjectiveComponent Pick(AICommander_BaseComponent cmd)
	{
		if (!ForCommander(cmd) || !s_Pick || !Forces(cmd, s_Pick))
			return null;
		return s_Pick;
	}

	static string PickWhy()
	{
		if (s_sPickWhy.IsEmpty())
			return "-";
		return s_sPickWhy;
	}

	static void NoteBypass(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, string gate)
	{
		if (!s_mBypassLogged || !obj)
			return;
		string key = obj.GetLogName() + "|" + gate;
		if (s_mBypassLogged.Contains(key))
			return;
		s_mBypassLogged.Set(key, true);
		DCO_Perf.Count("itest_bypass:" + gate);
		DCO_Log.Write(DCO_ELogChannel.TEST, DCO_ELogLevel.INFO, "-", string.Format("itest_bypass code=%1 obj=%2 gate=%3 cmd=%4", s_sCode, obj.GetLogName(), gate, s_sCmd));
	}

	static void Clear()
	{
		s_bOn = false;
		s_bAny = false;
		s_sCmd = "";
		s_sObj = "";
		s_mEnds = null;
		s_mReplanAt = null;
		s_mDefMax = null;
		s_mReplanFrom = null;
		s_mReplanLogAt = null;
		s_mBypassLogged = null;
		s_mHandOff = null;
		s_Pick = null;
	}

	static void NoteHandOff(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, string why)
	{
		if (!s_mHandOff || !obj || !Forces(cmd, obj) || s_mHandOff.Contains(obj))
			return;
		s_mHandOff.Set(obj, why);
		DCO_Log.Write(DCO_ELogChannel.TEST, DCO_ELogLevel.INFO, "-", string.Format("itest_tactic_handoff code=%1 obj=%2 from=%3 why=%4 next=commander_choice mask=open", s_sCode, obj.GetLogName(), DCO_CommanderTactics.Name(s_iTactic), why));
	}

	static bool ForceSelect(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj)
	{
		if (!Forces(cmd, obj) || (s_mHandOff && s_mHandOff.Contains(obj)))
			return false;
		if (DCO_Ctx.TuneForCmd(cmd, "itest.free_select", 0) > 0)
		{
			NoteBypass(cmd, obj, "free_select");
			return false;
		}
		return true;
	}

	static bool OpenMask(AICommander_BaseComponent cmd)
	{
		return ForCommander(cmd) && s_mHandOff && !s_mHandOff.IsEmpty();
	}

	static bool IsOn()
	{
		return s_bOn;
	}

	static int Tactic()
	{
		return s_iTactic;
	}

	static bool ForCommander(AICommander_BaseComponent cmd)
	{
		return s_bOn && cmd && cmd.GetCommanderUID() == s_sCmd;
	}

	static bool Forces(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj)
	{
		if (!ForCommander(cmd) || !obj || !obj.GetOwner())
			return false;
		if (s_bAny)
			return !Taken(cmd, obj);
		if (NameMatch(obj))
			return true;
		if (!s_bScope || Taken(cmd, obj))
			return false;
		DCO_OpPlanner opp = cmd.DCO_GetOpPlanner();
		if (opp && opp.HasOp())
			return opp.Contains(obj);
		CMD_AICommanderObjectiveComponent prim = Primary(cmd);
		return prim && vector.DistanceXZ(prim.GetOwner().GetOrigin(), obj.GetOwner().GetOrigin()) <= s_fScopeR;
	}

	static void ApplyScope(AICommander_BaseComponent cmd, notnull array<CMD_AICommanderObjectiveComponent> list)
	{
		if (!ForCommander(cmd) || s_bAny)
			return;
		CMD_AICommanderObjectiveComponent prim = Primary(cmd);
		if (!prim)
			return;
		FactionKey fk = cmd.GetCommanderFactionKey();
		int cap = Math.Max(1, list.Count());
		array<CMD_AICommanderObjectiveComponent> ordered = {};
		foreach (CMD_AICommanderObjectiveComponent c : list)
		{
			if (c && Forces(cmd, c) && c.GetObjectiveState(fk) == CMD_EObjectiveState.ASSIGNED)
				ordered.Insert(c);
		}
		CMD_AICommanderObjectiveComponent pick = prim;
		string why = "primary_direct";
		string shaping;
		string primDefText = "-";
		if (s_bScope && !Taken(cmd, prim))
		{
			float dP = ScopeDefenders(cmd, prim);
			vector primPos = prim.GetOwner().GetOrigin();
			vector centroid = cmd.DCO_AttackerCentroid();
			float bestDef = float.MAX;
			CMD_AICommanderObjectiveComponent sbf;
			foreach (CMD_AICommanderObjectiveComponent o : cmd.GetObjectiveList())
			{
				if (!o || o == prim || !Forces(cmd, o))
					continue;
				float dS = ScopeDefenders(cmd, o);
				if (!shaping.IsEmpty())
					shaping += ",";
				shaping += o.GetLogName() + ":" + dS.ToString(-1, 1);
				if (dS < dP && dS * 2 <= dP && dS < bestDef)
				{
					bestDef = dS;
					pick = o;
					why = "easier";
				}
				vector toO = DCO_Specialist.Flat(o.GetOwner().GetOrigin() - primPos);
				vector toC = DCO_Specialist.Flat(centroid - primPos);
				if (!sbf && vector.DistanceXZ(o.GetOwner().GetOrigin(), primPos) <= 450.0 && vector.Dot(toO, toC) >= 0)
					sbf = o;
			}
			if (why == "primary_direct" && sbf)
			{
				pick = sbf;
				why = "sbf_site";
			}
			primDefText = dP.ToString(-1, 1);
			if (s_Pick && s_Pick != prim && Forces(cmd, s_Pick) && s_Pick != pick)
			{
				string pickWas = "primary";
				if (pick)
					pickWas = pick.GetLogName();
				string keepLine = "op_scope_keep pick=" + s_Pick.GetLogName() + " reason=committed would_pick=" + pickWas + " would_why=" + why;
				if (keepLine != s_sKeepLog)
				{
					s_sKeepLog = keepLine;
					DCO_Perf.Count("op_scope_keep");
					DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(cmd), keepLine);
				}
				pick = s_Pick;
				why = "kept";
			}
			else if (pick && pick != prim)
			{
				if (s_iCaptured > 0)
				{
					CMD_AICommanderObjectiveComponent bestEff = BestRatio(cmd, prim);
					if (bestEff)
					{
						pick = bestEff;
						why = "best_ratio";
					}
				}
				s_Pick = pick;
			}
		}
		if (Taken(cmd, prim))
		{
			pick = null;
			pick = BestRatio(cmd, null);
			if (pick)
				why = "primary_taken";
		}
		if (pick && !ordered.Contains(pick))
			ordered.Insert(pick);
		bool shapingActive = pick && pick != prim && !Taken(cmd, prim);
		bool concurrent = false;
		if (shapingActive)
		{
			concurrent = Split(cmd, prim, pick);
			if (concurrent)
				cap = Math.Max(cap, 2);
		}
		else
			NoteMain(cmd, prim, "value");
		if (!Taken(cmd, prim) && !ordered.Contains(prim))
		{
			if (shapingActive && !concurrent)
			{
				string holdLine = "op_scope_hold obj=" + prim.GetLogName() + " reason=shaping_active shaping_pick=" + pick.GetLogName();
				if (holdLine != s_sHoldLog)
				{
					s_sHoldLog = holdLine;
					DCO_Perf.Count("op_scope_hold");
					DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(cmd), holdLine);
				}
			}
			else
				ordered.Insert(prim);
		}
		cap = Math.Max(cap, AssignRoles(cmd, prim, pick, ordered));
		list.Clear();
		for (int i = 0; i < ordered.Count() && i < cap; i++)
			list.Insert(ordered[i]);
		string pickName = "none";
		if (pick)
			pickName = pick.GetLogName();
		if (shaping.IsEmpty())
			shaping = "-";
		s_sPickWhy = why;
		string line = string.Format("op_scope primary=%1 primary_def=%2 shaping=%3 pick=%4 why=%5 r=%6", prim.GetLogName(), primDefText, shaping, pickName, why, Math.Round(s_fScopeR));
		if (line != s_sScopeLog)
		{
			s_sScopeLog = line;
			DCO_Perf.Count("op_scope:" + why);
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(cmd), line);
		}
	}

	static CMD_AICommanderObjectiveComponent ObjectiveFor(AICommander_BaseComponent cmd)
	{
		if (!ForCommander(cmd) || s_bAny || s_bScope)
			return null;
		return Primary(cmd);
	}

	static void OnPlanEnd(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, string result, string reason)
	{
		if (!Forces(cmd, obj) || !s_mEnds)
			return;
		if (obj == s_Pick && result != "captured")
			s_Pick = null;
		if (result == "captured")
			s_iCaptured++;
		if (result == "captured" && cmd.GetTactics())
		{
			AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
			if (mgr)
			{
				foreach (CMD_AICommanderObjectiveComponent other : mgr.m_aObjective)
				{
					if (!other || other == obj || !Forces(cmd, other))
						continue;
					DCO_TacticPlan op = cmd.GetTactics().GetPlan(other);
					if (!op)
						continue;
					NoteMain(cmd, other, "reinforce_success");
					bool sent = cmd.DCO_ReinforceAssault(other, DCO_Util.NowS(), true);
					DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(cmd), string.Format("op_reinforce_success cmd=%1 from=%2 to=%3 sent=%4", cmd.GetCommanderUID(), obj.GetLogName(), other.GetLogName(), DCO_Util.B01(sent)));
					break;
				}
			}
		}
		int ends = s_mEnds.Get(obj) + 1;
		s_mEnds.Set(obj, ends);
		int replanIn = 0;
		if (result != "captured" && ends < 2 && !(s_mHandOff && s_mHandOff.Contains(obj)))
		{
			replanIn = 30;
			s_mReplanAt.Set(obj, DCO_Util.NowS() + 30.0);
		}
		DCO_Log.Write(DCO_ELogChannel.TEST, DCO_ELogLevel.INFO, "-", string.Format("itest_tactic_end code=%1 obj=%2 result=%3 reason=%4 attempt=%5 replan_in_s=%6", s_sCode, obj.GetLogName(), result, reason, ends, replanIn));
	}

	static void Tick(AICommander_BaseComponent cmd, float now)
	{
		if (!ForCommander(cmd))
			return;
		int capSet = Math.Round(DCO_Ctx.TuneFor(cmd.GetOwner(), "itest.obj_cap", 0));
		if (capSet > 0 && cmd.GetObjectivesAtOnce() != capSet)
		{
			DCO_Log.Write(DCO_ELogChannel.TEST, DCO_ELogLevel.INFO, "-", string.Format("itest_obj_cap code=%1 cmd=%2 from=%3 to=%4 via=gm_setter", s_sCode, cmd.GetCommanderUID(), cmd.GetObjectivesAtOnce(), capSet));
			cmd.SetObjectivesAtOnce(capSet);
		}
		if (now >= s_fSeedAt)
		{
			s_fSeedAt = now + 15.0;
			int objs = 0;
			int defTotal = 0;
			array<CMD_AICommanderObjectiveComponent> all = {};
			AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
			if (mgr)
				all.InsertAll(mgr.m_aObjective);
			else
				all.InsertAll(cmd.GetObjectiveList());
			foreach (CMD_AICommanderObjectiveComponent o : all)
			{
				if (!Forces(cmd, o))
					continue;
				objs++;
				defTotal += SeedIntel(cmd, o, now);
			}
			if (!s_bSetupLogged && objs > 0)
			{
				s_bSetupLogged = true;
				int groups = 0;
				foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
				{
					if (g && !g.IsPlayerGroup() && !g.IsMortar() && !g.IsDedicatedTransport() && !g.IsMedicTeam() && g.IsInfantry())
						groups++;
				}
				string objText = ObjText();
				DCO_Log.Write(DCO_ELogChannel.TEST, DCO_ELogLevel.INFO, "-", string.Format("itest_tactic_setup code=%1 obj=%2 tactic=%3 cmd=%4 intel_def=%5 groups=%6 all_in=0 sizing=commander objectives=%7", s_sCode, objText, DCO_CommanderTactics.Name(s_iTactic), s_sCmd, defTotal, groups, objs));
			}
		}
		if (!s_mReplanAt)
			return;
		array<CMD_AICommanderObjectiveComponent> due = {};
		foreach (CMD_AICommanderObjectiveComponent ro, float at : s_mReplanAt)
		{
			if (ro && now >= at)
				due.Insert(ro);
		}
		int usable = UsableGroups(cmd);
		foreach (CMD_AICommanderObjectiveComponent d : due)
		{
			float from;
			if (!s_mReplanFrom || !s_mReplanFrom.Find(d, from))
			{
				from = now;
				if (s_mReplanFrom)
					s_mReplanFrom.Set(d, now);
			}
			float waited = now - from;
			if (usable < 2 && waited < 60.0)
			{
				float logAt;
				if (!s_mReplanLogAt || !s_mReplanLogAt.Find(d, logAt) || now >= logAt)
				{
					if (s_mReplanLogAt)
						s_mReplanLogAt.Set(d, now + 15.0);
					DCO_Log.Write(DCO_ELogChannel.TEST, DCO_ELogLevel.INFO, "-", string.Format("itest_replan_wait code=%1 obj=%2 usable=%3 waited_s=%4 max_s=60", s_sCode, d.GetLogName(), usable, Math.Round(waited)));
				}
				continue;
			}
			s_mReplanAt.Remove(d);
			if (s_mReplanFrom)
				s_mReplanFrom.Remove(d);
			cmd.DCO_TestReplan(d);
			DCO_Log.Write(DCO_ELogChannel.TEST, DCO_ELogLevel.INFO, "-", string.Format("itest_tactic_replan code=%1 obj=%2 tactic=%3 attempt=%4 usable=%5 waited_s=%6", s_sCode, d.GetLogName(), DCO_CommanderTactics.Name(s_iTactic), s_mEnds.Get(d) + 1, usable, Math.Round(waited)));
		}
	}

	protected static int UsableGroups(AICommander_BaseComponent cmd)
	{
		int n = 0;
		foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
		{
			if (!g || g.IsPlayerGroup() || g.IsMortar() || g.IsDedicatedTransport() || g.IsMedicTeam() || g.GetUnitCount() <= 0 || !g.CanItHaveOrder())
				continue;
			DCO_EGroupTask t = g.GetTask();
			if (t == DCO_EGroupTask.NONE || t == DCO_EGroupTask.PATROL)
				n++;
		}
		return n;
	}

	static float ScopeDefenders(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj)
	{
		float v = 0;
		DCO_DoctrineRuntime rt = cmd.GetDoctrineRuntime();
		if (rt)
		{
			DCO_ForceEval e = rt.GetEval(obj);
			if (e)
				v = e.m_fEnemy;
		}
		if (cmd.GetOps())
			v = Math.Max(v, cmd.GetOps().EstimateDefenders(obj));
		if (!s_mDefMax)
			return v;
		float best = s_mDefMax.Get(obj);
		if (v > best)
		{
			s_mDefMax.Set(obj, v);
			best = v;
		}
		return best;
	}

	protected static int SeedIntel(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, float now)
	{
		DCO_CommanderOps ops = cmd.GetOps();
		if (!ops)
			return 0;
		int friendly;
		int enemy;
		obj.CountNearbyUnitsCached(obj.GetRadius() + 150.0, cmd.GetCommanderFactionKey(), friendly, enemy);
		DCO_ObjectiveIntel intel = ops.GetIntel(obj);
		bool wasSeen = intel.m_bEverSeen;
		float before = intel.m_fDefenders;
		float keep = 0;
		if (s_mDefMax)
			keep = s_mDefMax.Get(obj);
		float val = Math.Max(Math.Max(before, keep), enemy);
		string src = "count";
		if (val <= 0)
		{
			val = 8.0;
			src = "default_unseen";
		}
		else if (val > enemy)
			src = "kept_max";
		intel.m_bEverSeen = true;
		intel.m_fConfidence = Math.Max(intel.m_fConfidence, 0.8);
		intel.m_fDefenders = val;
		if (enemy > intel.m_iRaw)
			intel.m_iRaw = enemy;
		intel.m_fUpdated = now;
		if (s_mDefMax && val > keep && src != "default_unseen")
			s_mDefMax.Set(obj, val);
		if (!wasSeen || Math.AbsFloat(before - val) >= 1.0)
			DCO_Log.Write(DCO_ELogChannel.TEST, DCO_ELogLevel.INFO, "-", string.Format("itest_intel_seed obj=%1 defenders=%2 before=%3 counted=%4 src=%5 conf=%6", obj.GetLogName(), val.ToString(-1, 1), before.ToString(-1, 1), enemy, src, intel.m_fConfidence.ToString(-1, 2)));
		return Math.Round(intel.m_fDefenders);
	}
}
