modded class CMD_AICommanderObjectiveComponent
{
	[Attribute("0", UIWidgets.EditBox, "Operation phase (server tune obj.phase_on = 1). 0 = always active. Phase N becomes active for a side once that side holds every objective of phases 1 to N-1, then waits DCO Active Delay.", category: "Objective", params: "0 20 1")]
	protected int m_iDCOPhase;

	[Attribute("", UIWidgets.Auto, "Objectives (map name or entity name) that must meet DCO Active Condition before this objective becomes active.", category: "Objective")]
	protected ref array<string> m_aDCOActiveAfter;

	[Attribute("0", UIWidgets.EditBox, "Condition for DCO Active After: 0 = captured by the side that wants to attack, 1 = captured by anyone, 2 = lost by its first owner.", category: "Objective", params: "0 2 1")]
	protected int m_iDCOActiveCond;

	[Attribute("120", UIWidgets.EditBox, "Seconds to wait after the phase / Active After conditions are met (reorganisation pause). 0 = at once.", category: "Objective", params: "0 3600 1")]
	protected float m_fDCOActiveDelay;
	protected float m_fDCOMissLogAt;
	protected ref array<string> m_aDCOAfterSeen;

	protected ref map<FactionKey, float> m_mDCOPhaseMetAt;
	protected ref map<FactionKey, bool> m_mDCOPhaseOn;
	protected string m_sDCOFirstOwner;
	protected bool m_bDCOFirstOwnerSet;

	int DCO_GetPhase()							{ return m_iDCOPhase; }
	void DCO_SetPhase(int v)					{ m_iDCOPhase = Math.ClampInt(v, 0, 20); }
	int DCO_GetActiveCond()						{ return m_iDCOActiveCond; }
	void DCO_SetActiveCond(int v)				{ m_iDCOActiveCond = Math.ClampInt(v, 0, 2); }
	float DCO_GetActiveDelay()					{ return m_fDCOActiveDelay; }
	void DCO_SetActiveDelay(float v)			{ m_fDCOActiveDelay = Math.Clamp(v, 0, 3600); }

	void DCO_GetActiveAfter(notnull array<string> outList)
	{
		outList.Clear();
		if (m_aDCOActiveAfter)
			outList.Copy(m_aDCOActiveAfter);
	}

	void DCO_SetActiveAfter(notnull array<string> list)
	{
		if (!m_aDCOActiveAfter)
			m_aDCOActiveAfter = {};
		m_aDCOActiveAfter.Copy(list);
	}

	bool DCO_HasPhaseRules()
	{
		return m_iDCOPhase > 1 || (m_aDCOActiveAfter && !m_aDCOActiveAfter.IsEmpty());
	}

	string DCO_PhaseFirstOwner()
	{
		if (!m_bDCOFirstOwnerSet)
		{
			FactionKey cur = GetOwnerFaction();
			if (!cur.IsEmpty())
			{
				m_bDCOFirstOwnerSet = true;
				m_sDCOFirstOwner = cur;
			}
		}
		return m_sDCOFirstOwner;
	}

	protected bool DCO_PhaseConditions(FactionKey fk, out string why)
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
			return true;
		if (m_aDCOActiveAfter && m_iDCOActiveCond == 2)
		{
			foreach (string pre : m_aDCOActiveAfter)
			{
				CMD_AICommanderObjectiveComponent pa = DCO_ObjPhase.FindByName(pre);
				if (pa)
					pa.DCO_PhaseFirstOwner();
			}
		}
		if (m_iDCOPhase > 1)
		{
			foreach (CMD_AICommanderObjectiveComponent o : mgr.m_aObjective)
			{
				if (!o || o == this || o.DCO_GetPhase() <= 0 || o.DCO_GetPhase() >= m_iDCOPhase)
					continue;
				if (!o.IsCapturedBy(fk))
				{
					why = "phase_" + o.DCO_GetPhase().ToString() + "_open:" + o.GetLogName();
					return false;
				}
			}
		}
		if (m_aDCOActiveAfter)
		{
			foreach (string n : m_aDCOActiveAfter)
			{
				CMD_AICommanderObjectiveComponent a = DCO_ObjPhase.FindByName(n);
				if (a)
				{
					if (!m_aDCOAfterSeen)
						m_aDCOAfterSeen = {};
					if (!m_aDCOAfterSeen.Contains(n))
						m_aDCOAfterSeen.Insert(n);
				}
				else if (m_aDCOAfterSeen && m_aDCOAfterSeen.Contains(n))
				{
					float nowG = DCO_Util.NowS();
					if (nowG >= m_fDCOMissLogAt)
					{
						m_fDCOMissLogAt = nowG + 300.0;
						DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.WARN, "-", string.Format("obj_phase_dep_gone obj=%1 after=%2 -- dependency existed earlier and was deleted or renamed; treated as met", GetLogName(), n));
					}
					continue;
				}
				if (!a)
				{
					float nowS = DCO_Util.NowS();
					if (nowS >= m_fDCOMissLogAt)
					{
						m_fDCOMissLogAt = nowS + 300.0;
						DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.ERROR, "-", string.Format("obj_phase_missing obj=%1 after=%2 -- dependency not found, treated as NOT met; fix the Active After name", GetLogName(), n));
					}
					why = "missing:" + n;
					return false;
				}
				bool ok;
				if (m_iDCOActiveCond == 1)
					ok = !a.GetOwnerFaction().IsEmpty();
				else if (m_iDCOActiveCond == 2)
				{
					string first = a.DCO_PhaseFirstOwner();
					ok = !first.IsEmpty() && a.GetOwnerFaction() != first;
				}
				else
					ok = a.IsCapturedBy(fk);
				if (!ok)
				{
					why = "after:" + a.GetLogName();
					return false;
				}
			}
		}
		return true;
	}

	bool DCO_PhaseActive(FactionKey fk, out string why)
	{
		if (!DCO_HasPhaseRules() || IsCapturedBy(fk))
			return true;
		if (!m_mDCOPhaseMetAt)
			m_mDCOPhaseMetAt = new map<FactionKey, float>();
		if (!m_mDCOPhaseOn)
			m_mDCOPhaseOn = new map<FactionKey, bool>();
		float now = DCO_Util.NowS();
		bool wasOn;
		m_mDCOPhaseOn.Find(fk, wasOn);
		if (!DCO_PhaseConditions(fk, why))
		{
			m_mDCOPhaseMetAt.Remove(fk);
			if (wasOn)
			{
				m_mDCOPhaseOn.Set(fk, false);
				DCO_Perf.Count("obj_phase:revert");
				string rl = string.Format("obj_phase_revert obj=%1 faction=%2 phase=%3 why=%4", GetLogName(), fk, m_iDCOPhase, why);
				DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", rl);
				DCO_Events.Operation(rl);
			}
			return false;
		}
		float metAt;
		if (!m_mDCOPhaseMetAt.Find(fk, metAt))
		{
			metAt = now;
			m_mDCOPhaseMetAt.Set(fk, now);
		}
		if (now - metAt < m_fDCOActiveDelay)
		{
			why = "delay_" + Math.Round(m_fDCOActiveDelay - (now - metAt)).ToString() + "s";
			return false;
		}
		if (!wasOn)
		{
			m_mDCOPhaseOn.Set(fk, true);
			DCO_Perf.Count("obj_phase:active");
			string al = string.Format("obj_phase_active obj=%1 faction=%2 phase=%3 waited_s=%4", GetLogName(), fk, m_iDCOPhase, Math.Round(now - metAt));
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", al);
			DCO_Events.Operation(al);
		}
		return true;
	}
}

class DCO_ObjPhase
{
	static bool On()
	{
		return DCO_Ctx.TuneG("obj.phase_on", 1) > 0;
	}

	static bool ActiveFor(CMD_AICommanderObjectiveComponent obj, FactionKey fk, out string why)
	{
		if (!obj || !On())
			return true;
		return obj.DCO_PhaseActive(fk, why);
	}

	static CMD_AICommanderObjectiveComponent FindByName(string name)
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr || name.IsEmpty())
			return null;
		string n2 = name;
		n2.Replace(" ", "_");
		foreach (CMD_AICommanderObjectiveComponent o : mgr.m_aObjective)
		{
			if (o && o.GetOwner() && (o.GetOwner().GetName() == name || o.GetLogName() == n2 || o.GetDisplayName() == name))
				return o;
		}
		return null;
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ObjPhaseAttribute : SCR_BaseFloatValueHolderEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		CMD_AICommanderObjectiveComponent obj = DCO_ObjAssign.Obj(item);
		if (!obj)
			return null;
		return SCR_BaseEditorAttributeVar.CreateInt(Math.ClampInt(obj.DCO_GetPhase(), 0, 20));
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		CMD_AICommanderObjectiveComponent obj = DCO_ObjAssign.Obj(item);
		if (!obj || !var)
			return;
		obj.DCO_SetPhase(var.GetInt());
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("obj_fields_set obj=%1 field=phase value=%2 player=%3", obj.GetLogName(), obj.DCO_GetPhase(), playerID));
	}

	override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
	{
		m_aValues.Clear();
		for (int i = 0; i <= 20; i++)
		{
			SCR_EditorAttributeFloatStringValueHolder v = new SCR_EditorAttributeFloatStringValueHolder();
			if (i == 0)
				v.SetName("Always active");
			else
				v.SetName("Phase " + i.ToString());
			v.SetFloatValue(i);
			m_aValues.Insert(v);
		}
		return super.GetEntries(outEntries);
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ObjActiveCondAttribute : SCR_BaseFloatValueHolderEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		CMD_AICommanderObjectiveComponent obj = DCO_ObjAssign.Obj(item);
		if (!obj)
			return null;
		return SCR_BaseEditorAttributeVar.CreateInt(obj.DCO_GetActiveCond());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		CMD_AICommanderObjectiveComponent obj = DCO_ObjAssign.Obj(item);
		if (!obj || !var)
			return;
		obj.DCO_SetActiveCond(var.GetInt());
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("obj_fields_set obj=%1 field=active_cond value=%2 player=%3", obj.GetLogName(), obj.DCO_GetActiveCond(), playerID));
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ObjActiveDelayAttribute : SCR_BaseFloatValueHolderEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		CMD_AICommanderObjectiveComponent obj = DCO_ObjAssign.Obj(item);
		if (!obj)
			return null;
		return SCR_BaseEditorAttributeVar.CreateFloat(obj.DCO_GetActiveDelay());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		CMD_AICommanderObjectiveComponent obj = DCO_ObjAssign.Obj(item);
		if (!obj || !var)
			return;
		obj.DCO_SetActiveDelay(var.GetFloat());
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("obj_fields_set obj=%1 field=active_delay value=%2 player=%3", obj.GetLogName(), Math.Round(obj.DCO_GetActiveDelay()), playerID));
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ObjActiveAfterAttribute : SCR_BaseMultiSelectPresetsEditorAttribute
{
	protected string m_sReadKey;

	override protected void CreatePresets()
	{
		DCO_ObjAssign.FillObjectiveEntries(m_aValues, false);
	}

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		CMD_AICommanderObjectiveComponent obj = DCO_ObjAssign.Obj(item);
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!obj || !mgr)
			return null;
		m_sReadKey = DCO_ObjAssign.RosterKey();
		array<string> list = {};
		obj.DCO_GetActiveAfter(list);
		array<string> names = {};
		DCO_ObjAssign.Objectives(names);
		int mask = 0;
		foreach (int i, string n : names)
		{
			CMD_AICommanderObjectiveComponent o = mgr.DCO_ObjectiveAt(i);
			if (!o || !o.GetOwner())
				continue;
			if (list.Contains(o.GetLogName()) || list.Contains(o.GetOwner().GetName()) || list.Contains(o.GetDisplayName()))
				mask |= DCO_ObjAssign.Bit(i);
		}
		return SCR_BaseEditorAttributeVar.CreateInt(mask);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		CMD_AICommanderObjectiveComponent obj = DCO_ObjAssign.Obj(item);
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!obj || !mgr || !var)
			return;
		if (DCO_ObjAssign.Stale(m_sReadKey, "active_after"))
			return;
		int mask = var.GetInt();
		array<string> names = {};
		DCO_ObjAssign.Objectives(names);
		array<string> list = {};
		foreach (int i, string n : names)
		{
			CMD_AICommanderObjectiveComponent o = mgr.DCO_ObjectiveAt(i);
			if (o && o != obj && (mask & DCO_ObjAssign.Bit(i)))
			{
				string key = o.GetOwner().GetName();
				if (key.IsEmpty())
					key = o.GetLogName();
				list.Insert(key);
			}
		}
		obj.DCO_SetActiveAfter(list);
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("obj_fields_set obj=%1 field=active_after value=%2 player=%3", obj.GetLogName(), CMD_AICommanderObjectiveComponent.DCO_JoinList(list), playerID));
	}
}
