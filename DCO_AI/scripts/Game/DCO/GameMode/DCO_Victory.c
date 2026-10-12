[ComponentEditorProps(category: "GameScripted/DCO", description: "DCO Victory: ends the mission from DCO objectives and commander stock. Put it on the game mode entity.")]
class DCO_VictoryComponentClass : ScriptComponentClass
{
}

class DCO_VictoryComponent : ScriptComponent
{
	[Attribute("1", UIWidgets.CheckBox, "Turn the DCO victory check on.", category: "DCO Victory")]
	protected bool m_bEnabled;

	[Attribute("0", UIWidgets.EditBox, "A faction wins when it holds at least this many DCO objectives, and at least one of them was captured (not owned by it at mission start). 0 = off.", params: "0 100 1", category: "DCO Victory")]
	protected int m_iHoldCount;

	[Attribute("0", UIWidgets.CheckBox, "A faction wins when it holds every objective marked Primary and captured at least one of those primary objectives (it did not own it at mission start). A defender that keeps its own primaries, even after taking a secondary, wins through Enemy Stock Empty, Hold Objective or typed objectives instead. An objective placed later counts its owner at placement as the starting owner.", category: "DCO Victory")]
	protected bool m_bHoldAllPrimary;

	[Attribute("", UIWidgets.EditBox, "Entity name of one objective. A faction wins when it holds it for Hold Minutes in a row. Empty = off.", category: "DCO Victory")]
	protected string m_sHoldObjective;

	[Attribute("5", UIWidgets.EditBox, "Minutes the objective above must be held without interruption.", params: "0.1 240 0.1", category: "DCO Victory")]
	protected float m_fHoldMinutes;

	[Attribute("0", UIWidgets.CheckBox, "A faction wins when every enemy commander has used up its reinforcement stock (soldiers). Unlimited stock never counts.", category: "DCO Victory")]
	protected bool m_bEnemyStockEmpty;

	[Attribute("0", UIWidgets.CheckBox, "Win from objective mission types (DEFEND, HOLD_UNTIL, DESTROY): when every typed objective has finished, the faction that won the most of them wins. DEFEND done = defender, DEFEND failed = attacker, HOLD_UNTIL done = holder, DESTROY done = attacker.", category: "DCO Victory")]
	protected bool m_bTypedObjectives;

	[Attribute("60", UIWidgets.EditBox, "Seconds after mission start before the first check.", params: "0 3600 1", category: "DCO Victory")]
	protected float m_fGraceSeconds;

	[Attribute("5", UIWidgets.EditBox, "Seconds between checks.", params: "1 60 1", category: "DCO Victory")]
	protected float m_fCheckSeconds;

	protected bool m_bEnded;
	protected float m_fStart_ms = -1;
	protected FactionKey m_sHoldOwner;
	protected float m_fHoldSince_ms = -1;
	protected string m_sLastState;
	protected ref array<FactionKey> m_aFactions = {};
	protected ref array<CMD_AICommanderObjectiveComponent> m_aStartObj = {};
	protected ref array<FactionKey> m_aStartOwner = {};
	protected string m_sLastSkip;

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if (!Replication.IsServer() || !GetGame().GetWorld())
			return;
		GetGame().GetCallqueue().CallLater(Check, Math.Max(m_fCheckSeconds, 1.0) * 1000.0, true);
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("victory_init enabled=%1 hold_count=%2 all_primary=%3 hold_obj=%4 hold_min=%5 enemy_stock=%6 typed=%7 grace_s=%8",
			DCO_Util.B01(m_bEnabled), m_iHoldCount, DCO_Util.B01(m_bHoldAllPrimary), NoneIf(m_sHoldObjective), m_fHoldMinutes, DCO_Util.B01(m_bEnemyStockEmpty), DCO_Util.B01(m_bTypedObjectives), m_fGraceSeconds));
	}

	override void OnDelete(IEntity owner)
	{
		GetGame().GetCallqueue().Remove(Check);
		super.OnDelete(owner);
	}

	protected static string NoneIf(string s)
	{
		if (s.IsEmpty())
			return "none";
		return s;
	}

	protected void Check()
	{
		if (!m_bEnabled || m_bEnded || !GetGame().GetWorld())
			return;
		float now = GetGame().GetWorld().GetWorldTime();
		if (m_fStart_ms < 0)
			m_fStart_ms = now;
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
			return;
		bool inGrace = now - m_fStart_ms < m_fGraceSeconds * 1000.0;
		RecordStartOwners(mgr, inGrace || now - m_fStart_ms < 20000.0);
		if (inGrace)
			return;

		m_aFactions.Clear();
		foreach (AICommander_BaseComponent cmd : mgr.m_aCommander)
		{
			if (cmd && !cmd.GetCommanderFactionKey().IsEmpty() && !m_aFactions.Contains(cmd.GetCommanderFactionKey()))
				m_aFactions.Insert(cmd.GetCommanderFactionKey());
		}
		foreach (CMD_AICommanderObjectiveComponent o : mgr.m_aObjective)
		{
			if (o && !o.GetOwnerFaction().IsEmpty() && !m_aFactions.Contains(o.GetOwnerFaction()))
				m_aFactions.Insert(o.GetOwnerFaction());
		}

		UpdateHoldTimer(mgr, now);

		if (m_bTypedObjectives)
		{
			FactionKey typedWinner;
			string typedWhy;
			if (TypedWinner(mgr, typedWinner, typedWhy))
			{
				End(typedWinner, typedWhy);
				return;
			}
		}

		string state;
		foreach (FactionKey f : m_aFactions)
		{
			int held, primaries, primHeld;
			foreach (CMD_AICommanderObjectiveComponent obj : mgr.m_aObjective)
			{
				if (!obj)
					continue;
				bool mine = obj.GetOwnerFaction() == f;
				if (mine)
					held++;
				if (obj.DCO_IsPrimary())
				{
					primaries++;
					if (mine)
						primHeld++;
				}
			}
			state += string.Format(" %1:%2/%3 prim=%4/%5", f, held, mgr.m_aObjective.Count(), primHeld, primaries);

			bool holdCountMet = m_iHoldCount > 0 && held >= m_iHoldCount;
			bool allPrimaryMet = m_bHoldAllPrimary && primaries > 0 && primHeld == primaries;
			if (holdCountMet || allPrimaryMet)
			{
				int captured = CapturedCount(mgr, f, false);
				int capturedPrim = CapturedCount(mgr, f, true);
				if (holdCountMet && captured > 0)
				{
					End(f, string.Format("hold_count held=%1 need=%2 captured=%3", held, m_iHoldCount, captured));
					return;
				}
				if (allPrimaryMet && capturedPrim > 0)
				{
					End(f, string.Format("all_primary held=%1 captured=%2 captured_primary=%3", primaries, captured, capturedPrim));
					return;
				}
				string skip = string.Format("%1 hold_count=%2 all_primary=%3", f, DCO_Util.B01(holdCountMet), DCO_Util.B01(allPrimaryMet));
				if (skip != m_sLastSkip)
				{
					m_sLastSkip = skip;
					DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("victory_skip reason=%7 faction=%1 hold_count=%2 all_primary=%3 held=%4 prim=%5/%6", f, DCO_Util.B01(holdCountMet), DCO_Util.B01(allPrimaryMet), held, primHeld, primaries, NoneIfReason(captured)));
				}
			}
			if (!m_sHoldObjective.IsEmpty() && m_sHoldOwner == f && m_fHoldSince_ms >= 0 && now - m_fHoldSince_ms >= m_fHoldMinutes * 60000.0)
			{
				End(f, string.Format("hold_objective obj=%1 minutes=%2", m_sHoldObjective, m_fHoldMinutes));
				return;
			}
			if (m_bEnemyStockEmpty && EnemiesOutOfStock(mgr, f))
			{
				End(f, "enemy_stock_empty");
				return;
			}
		}

		if (state != m_sLastState)
		{
			m_sLastState = state;
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", "victory_check" + state);
		}
	}

	protected static string NoneIfReason(int captured)
	{
		if (captured > 0)
			return "no_captured_primary";
		return "start_owner";
	}

	protected void RecordStartOwners(AICommander_ManagerComponent mgr, bool inGrace)
	{
		foreach (CMD_AICommanderObjectiveComponent obj : mgr.m_aObjective)
		{
			if (!obj)
				continue;
			FactionKey owner = obj.GetOwnerFaction();
			int i = m_aStartObj.Find(obj);
			if (i < 0)
			{
				m_aStartObj.Insert(obj);
				m_aStartOwner.Insert(owner);
				DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("victory_start_owner obj=%1 owner=%2 grace=%3", obj.GetLogName(), NoneIf(owner), DCO_Util.B01(inGrace)));
			}
			else if (inGrace && m_aStartOwner[i].IsEmpty() && !owner.IsEmpty())
			{
				m_aStartOwner[i] = owner;
				DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("victory_start_owner obj=%1 owner=%2 grace=1 late=1", obj.GetLogName(), owner));
			}
		}
	}

	protected int CapturedCount(AICommander_ManagerComponent mgr, FactionKey f, bool primaryOnly)
	{
		int n;
		foreach (CMD_AICommanderObjectiveComponent obj : mgr.m_aObjective)
		{
			if (!obj || obj.GetOwnerFaction() != f)
				continue;
			if (primaryOnly && !obj.DCO_IsPrimary())
				continue;
			int i = m_aStartObj.Find(obj);
			if (i < 0 || m_aStartOwner[i] != f)
				n++;
		}
		return n;
	}

	protected void UpdateHoldTimer(AICommander_ManagerComponent mgr, float now)
	{
		if (m_sHoldObjective.IsEmpty())
			return;
		FactionKey owner;
		bool found;
		foreach (CMD_AICommanderObjectiveComponent obj : mgr.m_aObjective)
		{
			if (obj && obj.GetOwner() && obj.GetOwner().GetName() == m_sHoldObjective)
			{
				owner = obj.GetOwnerFaction();
				found = true;
				break;
			}
		}
		if (!found)
		{
			if (m_sHoldOwner != "?")
				DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.WARN, "-", string.Format("victory_hold_missing obj=%1", m_sHoldObjective));
			m_sHoldOwner = "?";
			m_fHoldSince_ms = -1;
			return;
		}
		if (owner != m_sHoldOwner)
		{
			m_sHoldOwner = owner;
			if (owner.IsEmpty())
				m_fHoldSince_ms = -1;
			else
				m_fHoldSince_ms = now;
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("victory_hold obj=%1 owner=%2 need_min=%3", m_sHoldObjective, NoneIf(owner), m_fHoldMinutes));
		}
	}

	protected FactionKey OtherFaction(FactionKey f)
	{
		foreach (FactionKey k : m_aFactions)
		{
			if (k != f)
				return k;
		}
		return string.Empty;
	}

	protected FactionKey StrongestEnemy(CMD_AICommanderObjectiveComponent obj, FactionKey def)
	{
		array<FactionKey> keys = {};
		array<float> vals = {};
		obj.GetControlSnapshot(keys, vals);
		FactionKey best;
		float bestV = 0;
		for (int i = 0; i < keys.Count(); i++)
		{
			if (keys[i] != def && vals[i] > bestV)
			{
				bestV = vals[i];
				best = keys[i];
			}
		}
		if (best.IsEmpty() && !def.IsEmpty())
			best = OtherFaction(def);
		return best;
	}

	protected bool TypedWinner(AICommander_ManagerComponent mgr, out FactionKey winner, out string why)
	{
		map<FactionKey, int> wins = new map<FactionKey, int>();
		int typed, finished;
		foreach (CMD_AICommanderObjectiveComponent obj : mgr.m_aObjective)
		{
			if (!obj || !obj.DCO_HasMissionType())
				continue;
			typed++;
			int res = obj.DCO_GetTypeResult();
			if (res == 0)
				return false;
			finished++;
			FactionKey def = obj.DCO_GetDefender();
			FactionKey benefit;
			int t = obj.GetObjectiveType();
			if (t == CMD_EObjectiveType.DEFEND)
			{
				if (res == 1)
					benefit = def;
				else
				{
					string defWhy = obj.DCO_GetTypeWhy();
					if (defWhy.StartsWith("captured_by_"))
						benefit = defWhy.Substring(12, defWhy.Length() - 12);
					else
						benefit = StrongestEnemy(obj, def);
				}
			}
			else if (t == CMD_EObjectiveType.HOLD_UNTIL)
			{
				if (res == 1)
					benefit = obj.GetOwnerFaction();
			}
			else if (t == CMD_EObjectiveType.DESTROY)
			{
				if (res == 1)
				{
					benefit = obj.DCO_GetTypeFaction();
					if (benefit.IsEmpty())
						benefit = StrongestEnemy(obj, def);
				}
				else
					benefit = def;
			}
			if (benefit.IsEmpty())
				continue;
			int n;
			wins.Find(benefit, n);
			wins.Set(benefit, n + 1);
		}
		if (typed == 0)
			return false;
		int best = -1;
		bool tie;
		for (int i = 0; i < wins.Count(); i++)
		{
			int v = wins.GetElement(i);
			if (v > best)
			{
				best = v;
				winner = wins.GetKey(i);
				tie = false;
			}
			else if (v == best)
				tie = true;
		}
		if (best <= 0 || tie)
		{
			if (m_sLastState != "typed_tie")
			{
				m_sLastState = "typed_tie";
				DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.WARN, "-", string.Format("victory_typed_no_winner typed=%1 finished=%2 best=%3 tie=%4", typed, finished, best, DCO_Util.B01(tie)));
			}
			return false;
		}
		why = string.Format("typed_objectives wins=%1 of=%2", best, typed);
		return true;
	}

	protected bool EnemiesOutOfStock(AICommander_ManagerComponent mgr, FactionKey f)
	{
		int enemies;
		foreach (AICommander_BaseComponent cmd : mgr.m_aCommander)
		{
			if (!cmd || cmd.GetCommanderFactionKey() == f)
				continue;
			enemies++;
			if (cmd.GetReinforcementStock() != 0 || cmd.GetStockStart() <= 0)
				return false;
		}
		return enemies > 0;
	}

	protected void End(FactionKey winner, string reason)
	{
		if (m_bEnded)
			return;
		m_bEnded = true;
		GetGame().GetCallqueue().Remove(Check);
		string line = string.Format("victory_end winner=%1 reason=%2", winner, reason);
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", line);
		DCO_Events.Victory(winner, line);

		FactionManager fm = GetGame().GetFactionManager();
		SCR_BaseGameMode gm = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (!fm || !gm)
		{
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.ERROR, "-", "victory_end_failed reason=no_gamemode_or_factionmanager");
			return;
		}
		int fid = fm.GetFactionIndex(fm.GetFactionByKey(winner));
		SCR_GameModeEndData endData = SCR_GameModeEndData.CreateSimple(EGameOverTypes.ENDREASON_SCORELIMIT, winnerFactionId: fid);
		gm.EndGameMode(endData);
	}
}
