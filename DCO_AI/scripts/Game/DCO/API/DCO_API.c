class DCO_API
{
	static int Version()
	{
		return 100;
	}

	static bool IsReady()
	{
		return DCO_GlobalAIComponent.GetInstance() != null;
	}

	static DCO_Events Events()
	{
		return DCO_Events.Get();
	}

	static bool HasSetting(string key)
	{
		return IsNamed(key) || !DCO_GlobalAIComponent.DCO_TuneKeyOf(key).IsEmpty();
	}

	static float GetSetting(string key, FactionKey faction = "")
	{
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetForFaction(faction);
		if (!g)
			return 0;
		switch (key)
		{
			case "aiAggression": return g.DCO_AggressionSlider();
			case "vehicleExit": return g.GetVehicleExit();
			case "aiSkillDefault": return g.GetAISkill();
			case "aimAccuracy": return g.GetUnitSkill();
			case "aimSpeed": return g.GetAimSpeed();
			case "aiPerception": return g.GetUnitPerception();
			case "suppressionEffect": return g.GetSuppressionEffect();
			case "takeCoverChance": return g.GetTakeCoverChance();
		}
		string tk = DCO_GlobalAIComponent.DCO_TuneKeyOf(key);
		vector r;
		if (tk.IsEmpty() || !DCO_GlobalAIComponent.DCO_TuneRange(tk, r))
			return 0;
		return g.DCO_TuneGet(tk, r[2]);
	}

	static bool SetSetting(string mod, string key, float value, FactionKey faction = "")
	{
		string scope = "global";
		if (!faction.IsEmpty())
			scope = "faction:" + faction;
		if (!CanWrite(mod, scope, key))
			return false;
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetForFaction(faction);
		if (!g)
			return Reject(mod, scope, key, "no_global_component");
		if (!faction.IsEmpty() && !g.DCO_IsFactionLayer())
			return Reject(mod, scope, key, "no_faction_layer");
		float old = GetSetting(key, faction);
		switch (key)
		{
			case "aiAggression": g.DCO_SetAggression(value); break;
			case "vehicleExit": g.SetVehicleExit(Math.Round(value)); break;
			case "aiSkillDefault": g.SetAISkill(Math.ClampInt(Math.Round(value), DCO_AISKILL.NOOB, DCO_AISKILL.TERMINATOR)); break;
			case "aimAccuracy": g.SetUnitSkill(Math.Clamp(value, 0.1, 10.0)); break;
			case "aimSpeed": g.SetAimSpeed(value); break;
			case "aiPerception": g.SetUnitPerception(Math.Clamp(value, 0.5, 5.0)); break;
			case "suppressionEffect": g.SetSuppressionEffect(Math.Clamp(value, 0.0, 2.0)); break;
			case "takeCoverChance": g.SetTakeCoverChance(Math.Clamp(value, 0.0, 1.0)); break;
			default:
			{
				string tk = DCO_GlobalAIComponent.DCO_TuneKeyOf(key);
				vector r;
				if (tk.IsEmpty() || !DCO_GlobalAIComponent.DCO_TuneRange(tk, r))
					return Reject(mod, scope, key, "unknown_key");
				g.DCO_TuneSet(tk, Math.Clamp(value, r[0], r[1]));
			}
		}
		Applied(mod, scope, key, old, GetSetting(key, faction));
		return true;
	}

	static float GetSquadSetting(AIGroup group, string key)
	{
		DCO_GroupUtilityComponent gu = Squad(group);
		if (!gu)
			return 0;
		if (key == "vehicleExit")
			return gu.DCO_VehicleExit();
		return 0;
	}

	static bool SetSquadSetting(string mod, AIGroup group, string key, float value)
	{
		string scope = "squad:" + DCO_Log.IdGroup(group);
		if (!CanWrite(mod, scope, key))
			return false;
		DCO_GroupUtilityComponent gu = Squad(group);
		if (!gu)
			return Reject(mod, scope, key, "no_dco_squad");
		if (key != "vehicleExit")
			return Reject(mod, scope, key, "unknown_key");
		float old = gu.DCO_VehicleExit();
		gu.DCO_SetVehicleExit(Math.Round(value));
		Applied(mod, scope, key, old, gu.DCO_VehicleExit());
		return true;
	}

	static float GetSoldierSetting(IEntity soldier, string key)
	{
		DCO_AIConfigComponent c = SoldierConfig(soldier);
		if (!c)
			return 0;
		switch (key)
		{
			case "aiSkill": return c.GetAISkill();
			case "aimAccuracy": return c.GetAccuracy();
			case "aimSpeed": return c.GetAimSpeed();
			case "aiPerception": return c.GetPerception();
			case "suppressionEffect": return c.GetSuppressionEffect();
			case "takeCoverChance": return c.GetTakeCoverChance();
			case "personality": return c.GetPersonality();
		}
		return 0;
	}

	static bool SetSoldierSetting(string mod, IEntity soldier, string key, float value)
	{
		string scope = "soldier:" + DCO_Log.IdUnit(soldier);
		if (!CanWrite(mod, scope, key))
			return false;
		DCO_AIConfigComponent c = SoldierConfig(soldier);
		if (!c)
			return Reject(mod, scope, key, "no_dco_soldier");
		float old = GetSoldierSetting(soldier, key);
		switch (key)
		{
			case "aiSkill": c.SetAISkill(Math.ClampInt(Math.Round(value), DCO_AISKILL.NOOB, DCO_AISKILL.TERMINATOR)); break;
			case "aimAccuracy": c.SetAccuracy(Math.Clamp(value, 0.1, 10.0)); break;
			case "aimSpeed": c.SetAimSpeed(value); break;
			case "aiPerception": c.SetPerception(Math.Clamp(value, 0.5, 5.0)); break;
			case "suppressionEffect": c.SetSuppressionEffect(Math.Clamp(value, 0.0, 2.0)); break;
			case "takeCoverChance": c.SetTakeCoverChance(Math.Clamp(value, 0.0, 1.0)); break;
			case "personality": c.SetPersonality(Math.ClampInt(Math.Round(value), DCO_EAIPersonality.CAUTIOUS, DCO_EAIPersonality.RECKLESS)); break;
			default: return Reject(mod, scope, key, "unknown_key");
		}
		Applied(mod, scope, key, old, GetSoldierSetting(soldier, key));
		return true;
	}

	static int SoldierMorale(IEntity soldier)
	{
		SCR_AIUtilityComponent u = DCO_BldLearn.UtilOf(soldier);
		if (!u || !u.GetMoraleSystem())
			return -1;
		return u.GetMoraleSystem().GetState();
	}

	static float SoldierMoraleValue(IEntity soldier)
	{
		SCR_AIUtilityComponent u = DCO_BldLearn.UtilOf(soldier);
		if (!u || !u.GetMoraleSystem())
			return 0;
		return u.GetMoraleSystem().GetMoraleMeasure();
	}

	static int SoldierStress(IEntity soldier)
	{
		SCR_AIUtilityComponent u = DCO_BldLearn.UtilOf(soldier);
		if (!u || !u.m_CombatComponent)
			return 0;
		return u.m_CombatComponent.DCO_StressLevel();
	}

	static float SoldierSuppression(IEntity soldier)
	{
		SCR_AIUtilityComponent u = DCO_BldLearn.UtilOf(soldier);
		if (!u || !u.m_ThreatSystem)
			return 0;
		return u.m_ThreatSystem.GetSuppressionMeasure();
	}

	static bool SoldierInCover(IEntity soldier)
	{
		SCR_AIUtilityComponent u = DCO_BldLearn.UtilOf(soldier);
		if (!u)
			return false;
		if (SCR_CoverManagerComponent.IsEntityInsideBuilding(soldier))
			return true;
		return u.m_CombatMoveState && u.m_CombatMoveState.IsInValidCover();
	}

	static float SoldierAmmo(IEntity soldier)
	{
		SCR_AIUtilityComponent u = DCO_BldLearn.UtilOf(soldier);
		if (!u || !u.m_CombatComponent)
			return 0;
		return DCO_VehicleActive.AmmoFrac(u);
	}

	static int SquadTask(AIGroup group)
	{
		DCO_GroupUtilityComponent gu = Squad(group);
		if (!gu)
			return DCO_EGroupTask.NONE;
		return gu.GetTask();
	}

	static int SquadPhase(AIGroup group)
	{
		DCO_GroupUtilityComponent gu = Squad(group);
		if (!gu)
			return DCO_ETaskPhase.HOLDING;
		return gu.GetPhase();
	}

	static int SquadState(AIGroup group)
	{
		DCO_GroupUtilityComponent gu = Squad(group);
		if (!gu)
			return 0;
		return gu.GetState();
	}

	static float SquadMorale(AIGroup group)
	{
		DCO_GroupUtilityComponent gu = Squad(group);
		if (!gu)
			return 0;
		return gu.GetGroupMorale();
	}

	static int SquadAlive(AIGroup group)
	{
		DCO_GroupUtilityComponent gu = Squad(group);
		if (!gu)
			return 0;
		return gu.GetUnitCount();
	}

	static int SquadPeak(AIGroup group)
	{
		DCO_GroupUtilityComponent gu = Squad(group);
		if (!gu)
			return 0;
		return Math.Max(gu.DCO_GetPeakStrength(), gu.GetUnitCount());
	}

	static CMD_AICommanderObjectiveComponent SquadObjective(AIGroup group)
	{
		DCO_GroupUtilityComponent gu = Squad(group);
		if (!gu)
			return null;
		return gu.GetGroupObjective();
	}

	static AICommander_BaseComponent SquadCommander(AIGroup group)
	{
		DCO_GroupUtilityComponent gu = Squad(group);
		if (!gu)
			return null;
		return gu.GetMyCommander();
	}

	static int Commanders(notnull array<AICommander_BaseComponent> outList)
	{
		outList.Clear();
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (mgr)
		{
			foreach (AICommander_BaseComponent c : mgr.m_aCommander)
			{
				if (c)
					outList.Insert(c);
			}
		}
		return outList.Count();
	}

	static int Objectives(notnull array<CMD_AICommanderObjectiveComponent> outList)
	{
		outList.Clear();
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (mgr)
		{
			foreach (CMD_AICommanderObjectiveComponent o : mgr.m_aObjective)
			{
				if (o)
					outList.Insert(o);
			}
		}
		return outList.Count();
	}

	static DCO_GroupUtilityComponent Squad(AIGroup group)
	{
		if (!group)
			return null;
		return DCO_GroupUtilityComponent.Cast(group.FindComponent(DCO_GroupUtilityComponent));
	}

	protected static DCO_AIConfigComponent SoldierConfig(IEntity soldier)
	{
		return DCO_AIConfigComponent.Of(DCO_BldLearn.UtilOf(soldier));
	}

	protected static bool IsNamed(string key)
	{
		return key == "aiAggression" || key == "vehicleExit" || key == "aiSkillDefault" || key == "aimAccuracy" || key == "aimSpeed" || key == "aiPerception" || key == "suppressionEffect" || key == "takeCoverChance";
	}

	protected static bool CanWrite(string mod, string scope, string key)
	{
		if (mod.IsEmpty())
			return Reject("?", scope, key, "mod_name_empty");
		if (!Replication.IsServer())
			return Reject(mod, scope, key, "not_server");
		return true;
	}

	protected static bool Reject(string mod, string scope, string key, string why)
	{
		DCO_Perf.Count("api:reject_" + why);
		DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.WARN, "-", string.Format("api_reject mod=%1 scope=%2 key=%3 why=%4", mod, scope, key, why));
		return false;
	}

	protected static void Applied(string mod, string scope, string key, float old, float now)
	{
		DCO_Perf.Count("api:set");
		DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", string.Format("api_set mod=%1 scope=%2 key=%3 old=%4 new=%5", mod, scope, key, old.ToString(-1, 3), now.ToString(-1, 3)));
		DCO_Report.Api(mod, scope + " " + key, now.ToString(-1, 3));
	}
}
