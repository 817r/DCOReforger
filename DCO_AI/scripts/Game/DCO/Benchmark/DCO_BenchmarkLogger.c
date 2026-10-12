[ComponentEditorProps(category: "GameScripted/DCO", description: "Benchmark logger. Writes commander, objective and group state plus events to DCO_Log (dco.log in the session folder $profile:DCO/Logs).")]
class DCO_BenchmarkLoggerComponentClass : ScriptComponentClass
{
}

class DCO_BenchmarkLoggerComponent : ScriptComponent
{
	[Attribute("run", UIWidgets.EditBox, "Run label (scenario or test variant name), written on the START line.", category: "Benchmark")]
	protected string m_sRunLabel;

	[Attribute("10", UIWidgets.EditBox, "Interval (s) of TICK lines: FPS, live AI per faction, commanders, objectives.", category: "Benchmark")]
	protected float m_fTickInterval;

	[Attribute("30", UIWidgets.EditBox, "Interval (s) of the per-group detail dump. 0 = off.", category: "Benchmark")]
	protected float m_fGroupInterval;

	[Attribute("1", UIWidgets.CheckBox, "Also dump a UNIT line for each group member (position, stance, action, etc.) every Group Interval.", category: "Benchmark")]
	protected bool m_bLogUnits;

	[Attribute("1", UIWidgets.CheckBox, "Dump a VEH line for each vehicle AI has boarded (damage, crew, speed) every Group Interval.", category: "Benchmark")]
	protected bool m_bLogVehicles;

	[Attribute("", UIWidgets.EditBox, "TEST: group names (comma separated) that are kept on Hold Position, as if a GM is holding them.", category: "Benchmark")]
	protected string m_sTestHoldGroups;

	protected static DCO_BenchmarkLoggerComponent s_Instance;

	protected ref map<AIAgent, vector> m_mLastUnitPos = new map<AIAgent, vector>();
	protected ref array<IEntity> m_aVehicles = {};
	protected ref map<IEntity, vector> m_mLastVehPos = new map<IEntity, vector>();
	protected ref map<IEntity, string> m_mVehState = new map<IEntity, string>();
	protected float m_fStartTime;
	protected float m_fNextGroupDump;
	protected bool m_bSummaryWritten;

	protected ref map<FactionKey, int> m_mInfantryLost = new map<FactionKey, int>();
	protected ref map<FactionKey, int> m_mVehiclesLost = new map<FactionKey, int>();
	protected ref map<FactionKey, int> m_mKills = new map<FactionKey, int>();
	protected ref map<FactionKey, int> m_mPeakAlive = new map<FactionKey, int>();
	protected ref map<CMD_AICommanderObjectiveComponent, FactionKey> m_mObjOwner = new map<CMD_AICommanderObjectiveComponent, FactionKey>();
	protected ref map<AICommander_BaseComponent, int> m_mShellsFired = new map<AICommander_BaseComponent, int>();

	protected float m_fFpsSum;
	protected float m_fFpsMin = 9999;
	protected int m_iFpsSamples;

	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);
	}

	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);

		if (!Replication.IsServer() || !GetGame().InPlayMode())
			return;

		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetInstance();
		if (!DCO_TestCaseComponent.RunOn() && (!cfg || !cfg.GetPerfProfiling()))
			return;

		int y, mo, d, h, mi, s;
		System.GetYearMonthDay(y, mo, d);
		System.GetHourMinuteSecond(h, mi, s);

		m_fStartTime = DCO_Util.NowS();
		m_fNextGroupDump = m_fGroupInterval;
		s_Instance = this;

		Line(DCO_ELogChannel.SYS, string.Format("START label=%1 date=%2-%3-%4 %5:%6:%7", m_sRunLabel, y, Pad(mo), Pad(d), Pad(h), Pad(mi), Pad(s)));
		DCO_Log.Flush();

		SCR_BaseGameMode gm = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (gm)
			gm.GetOnControllableDestroyed().Insert(OnControllableDestroyed);

		GetGame().GetCallqueue().CallLater(Tick, m_fTickInterval * 1000, true);
	}

	override void OnDelete(IEntity owner)
	{
		GetGame().GetCallqueue().Remove(Tick);
		bool active = s_Instance == this;
		if (active)
			s_Instance = null;

		SCR_BaseGameMode gm = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (gm)
			gm.GetOnControllableDestroyed().Remove(OnControllableDestroyed);

		if (active)
		{
			WriteSummary("world_end");
			DCO_Log.Flush();
		}

		super.OnDelete(owner);
	}

	protected bool m_bDCOConfigLogged;
	protected ref array<string> m_aFieldName = {"hp", "supp", "morale", "threat_v", "aim", "ads", "mag", "hold", "lean", "shots"};
	protected ref array<float> m_aFieldMin = {};
	protected ref array<float> m_aFieldMax = {};
	protected ref array<int> m_aFieldCount = {};

	protected void LogConfig()
	{
		m_bDCOConfigLogged = true;
		Line(DCO_ELogChannel.SYS, "CONFIG " + DCO_Log.ConfigStamp());
		DCO_Log.Flush();
	}

	protected void TrackField(int idx, float v)
	{
		if (m_aFieldCount.IsEmpty())
		{
			foreach (string n : m_aFieldName)
			{
				m_aFieldMin.Insert(0);
				m_aFieldMax.Insert(0);
				m_aFieldCount.Insert(0);
			}
		}
		if (m_aFieldCount[idx] == 0 || v < m_aFieldMin[idx])
			m_aFieldMin[idx] = v;
		if (m_aFieldCount[idx] == 0 || v > m_aFieldMax[idx])
			m_aFieldMax[idx] = v;
		m_aFieldCount[idx] = m_aFieldCount[idx] + 1;
	}

	protected static ref array<AIGroup> s_aTestGroups;

	static void AddTestGroup(AIGroup g)
	{
		if (!g)
			return;
		if (!s_aTestGroups)
			s_aTestGroups = {};
		if (!s_aTestGroups.Contains(g))
			s_aTestGroups.Insert(g);
	}

	protected void Tick()
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!m_bDCOConfigLogged)
			LogConfig();

		float fps = System.GetFPS();
		m_fFpsSum += fps;
		m_fFpsMin = Math.Min(m_fFpsMin, fps);
		m_iFpsSamples++;

		map<FactionKey, int> alive = new map<FactionKey, int>();
		string aliveStr;

		if (mgr)
		{
			foreach (AICommander_BaseComponent cmd : mgr.m_aCommander)
			{
				if (!cmd)
					continue;

				int n = cmd.GetTotalManpower();
				FactionKey fk = cmd.GetCommanderFactionKey();
				alive.Set(fk, Get(alive, fk) + n);
			}
		}

		foreach (FactionKey fk, int n : alive)
		{
			aliveStr += string.Format(" %1=%2", fk, n);
			if (n > Get(m_mPeakAlive, fk))
				m_mPeakAlive.Set(fk, n);
		}

		Line(DCO_ELogChannel.PERF, string.Format("TICK fps=%1 alive%2", fps.ToString(1, 1), aliveStr));

		float t = DCO_Util.NowS() - m_fStartTime;
		bool groupDue = m_fGroupInterval > 0 && t >= m_fNextGroupDump;
		if (groupDue)
			m_fNextGroupDump = t + m_fGroupInterval;
		if (mgr)
		{
			LogCommanders(mgr);
			LogObjectives(mgr);
			if (groupDue)
				LogGroups(mgr);
		}
		if (groupDue && m_bLogUnits && s_aTestGroups)
		{
			foreach (AIGroup tg : s_aTestGroups)
			{
				if (!tg)
					continue;
				DCO_GroupUtilityComponent tgu = DCO_GroupUtilityComponent.Cast(tg.FindComponent(DCO_GroupUtilityComponent));
				if (mgr && tgu && tgu.GetMyCommander())
					continue;
				LogUnits(tg);
			}
		}
		if (groupDue && m_bLogVehicles && !mgr)
			LogVehicles();

		if (!m_bSummaryWritten)
		{
			foreach (FactionKey fk, int peak : m_mPeakAlive)
			{
				if (peak > 0 && Get(alive, fk) == 0)
				{
					Line(DCO_ELogChannel.CMD, string.Format("faction_eliminated faction=%1", fk));
					WriteSummary("eliminated_" + fk);
					break;
				}
			}
		}

		DCO_Log.Flush();
	}

	protected void LogCommanders(AICommander_ManagerComponent mgr)
	{
		foreach (AICommander_BaseComponent cmd : mgr.m_aCommander)
		{
			if (!cmd)
				continue;

			int shells = 0;
			CMD_ArtillerySupport arty = cmd.GetArtySupport();
			if (arty)
			{
				shells = arty.GetTotalShellsFired();
				int prev;
				m_mShellsFired.Find(cmd, prev);
				if (shells > prev)
					Line(DCO_ELogChannel.ARTY, string.Format("artillery cmd=%1 shells=%2 total=%3", cmd.GetCommanderUID(), shells - prev, shells));
				m_mShellsFired.Set(cmd, shells);
			}

			Line(DCO_ELogChannel.CMD, string.Format("CMD uid=%1 faction=%2 mode=%3 groups=%4 manpower=%5 reserve=%6 vehicles=%7 shells=%8",
				cmd.GetCommanderUID(), cmd.GetCommanderFactionKey(),
				typename.EnumToString(CMD_ECommanderMode, cmd.GetCommanderMode()),
				cmd.GetOwnedGroupCount(), cmd.GetTotalManpower(), cmd.GetReserveManpower(),
				cmd.GetOwnedVehicle(), shells));
		}
	}

	protected void LogObjectives(AICommander_ManagerComponent mgr)
	{
		array<FactionKey> keys = {};
		array<float> values = {};

		foreach (CMD_AICommanderObjectiveComponent obj : mgr.m_aObjective)
		{
			if (!obj)
				continue;

			FactionKey owner = obj.GetOwnerFaction();
			FactionKey prevOwner;
			bool known = m_mObjOwner.Find(obj, prevOwner);
			if (known && prevOwner != owner)
				Line(DCO_ELogChannel.CMD, string.Format("objective_owner obj=%1 from=%2 to=%3", ObjName(obj), NoneIfEmpty(prevOwner), NoneIfEmpty(owner)));
			m_mObjOwner.Set(obj, owner);

			string control;
			obj.GetControlSnapshot(keys, values);
			for (int i = 0; i < keys.Count(); i++)
				control += string.Format(" %1:%2", keys[i], values[i].ToString(1, 0));

			Line(DCO_ELogChannel.CMD, string.Format("OBJ name=%1 owner=%2 control=[%3 ]", ObjName(obj), NoneIfEmpty(owner), control));
		}
	}

	protected void LogGroups(AICommander_ManagerComponent mgr)
	{
		array<DCO_GroupUtilityComponent> groups = {};

		foreach (AICommander_BaseComponent cmd : mgr.m_aCommander)
		{
			if (!cmd)
				continue;

			cmd.GetAllGroups(groups);
			foreach (DCO_GroupUtilityComponent grp : groups)
			{
				string posture = "-";
				string action = "-";
				SCR_AIGroupUtilityComponent util = grp.GetGroupUtilityComponent();
				if (util)
				{
					posture = typename.EnumToString(DCO_GroupTactics, util.DCO_GetPosture());
					AIActionBase act = util.GetCurrentAction();
					if (act)
						action = act.ClassName();
				}

				string objName = "-";
				CMD_AICommanderObjectiveComponent obj = grp.GetGroupObjective();
				if (obj)
					objName = ObjName(obj);

				string wpInfo = "-";
				AIGroup aiGroup = AIGroup.Cast(grp.GetOwner());
				if (aiGroup)
				{
					AIWaypoint wp = aiGroup.GetCurrentWaypoint();
					if (wp)
						wpInfo = string.Format("%1:r%2", wp.ClassName(), wp.GetCompletionRadius());
				}

				vector p = grp.GetOwner().GetOrigin();
				Line(DCO_ELogChannel.SQUAD, string.Format("GRP cmd=%1 name=%2 role=%3 status=%4 units=%5 posture=%6 action=%7 obj=%8 pos=%9",
					cmd.GetCommanderUID(), grp.GetOwner().GetName(),
					typename.EnumToString(DCO_EGroupTask, grp.GetTask()),
					typename.EnumToString(DCO_ETaskPhase, grp.GetPhase()),
					grp.GetUnitCount(), posture, action, objName,
					string.Format("%1,%2", Math.Round(p[0]), Math.Round(p[2]))) + " wp=" + wpInfo + " caps=" + GroupCaps(grp) + " state=" + grp.GetState() + " morale=" + grp.GetGroupMorale().ToString(-1, 2) + " cap=" + typename.EnumToString(DCO_EGroupCapability, grp.GetCapability()));

				if (m_bLogUnits && aiGroup)
					LogUnits(aiGroup);
			}
		}

		if (m_bLogVehicles)
			LogVehicles();
	}

	protected string GroupCaps(DCO_GroupUtilityComponent grp)
	{
		DCO_GroupConfigComponent cfg = DCO_GroupConfigComponent.Cast(grp.GetOwner().FindComponent(DCO_GroupConfigComponent));
		if (!cfg)
			return "-";

		int caps = cfg.GetCapabilities();
		string letters = "LSACRMQ";
		string s;
		for (int i = 0; i < 7; i++)
		{
			if (caps & (1 << i))
				s += letters.Get(i);
		}
		if (s.IsEmpty())
			return "-";
		return s;
	}

	static void Event(string text)
	{
		DCO_TestCaseComponent.OnEvent(text);
		DCO_Log.Legacy(text);
	}

	protected void TrackVehicle(IEntity vehicle)
	{
		if (Vehicle.Cast(vehicle) && !m_aVehicles.Contains(vehicle))
			m_aVehicles.Insert(vehicle);
	}

	protected void LogVehicles()
	{
		for (int i = m_aVehicles.Count() - 1; i >= 0; i--)
		{
			IEntity veh = m_aVehicles[i];
			if (!veh)
			{
				m_aVehicles.Remove(i);
				continue;
			}

			string name = VehName(veh);
			vector p = veh.GetOrigin();
			string moved = "-";
			vector last;
			if (m_mLastVehPos.Find(veh, last))
				moved = Math.Round(vector.Distance(last, p)).ToString();
			m_mLastVehPos.Set(veh, p);

			float speed = 0;
			Physics phys = veh.GetPhysics();
			if (phys)
				speed = phys.GetVelocity().Length() * 3.6;

			string hp = "-";
			string state = "ok";
			string dmgInfo = "-";
			SCR_VehicleDamageManagerComponent dmg = SCR_VehicleDamageManagerComponent.Cast(veh.FindComponent(SCR_VehicleDamageManagerComponent));
			if (dmg)
			{
				hp = dmg.GetHealthScaled().ToString(1, 2);
				dmgInfo = string.Format("engine=%1 move=%2 aim=%3 fire=%4", dmg.GetEngineFunctional(),
					dmg.GetMovementDamage().ToString(1, 2), dmg.GetAimingDamage().ToString(1, 2), dmg.IsOnFire());
				if (dmg.IsDestroyed())
					state = "destroyed";
				else if (dmg.IsOnFire())
					state = "burning";
				else if (!dmg.GetEngineFunctional())
					state = "immobile";
			}

			string prevState;
			if (m_mVehState.Find(veh, prevState) && prevState != state)
				Line(DCO_ELogChannel.VEH, string.Format("veh_state veh=%1 from=%2 to=%3", name, prevState, state));
			m_mVehState.Set(veh, state);

			vector upAxis = veh.GetTransformAxis(1);
			float upY = upAxis[1];
			string armor = "light";
			if (DCO_VehicleCombat.IsArmored(veh))
				armor = "armored";

			Line(DCO_ELogChannel.VEH, string.Format("VEH veh=%1 pos=%2,%3 moved=%4 speed=%5 hp=%6 state=%7 %8 crew=%9",
				name, Math.Round(p[0]), Math.Round(p[2]), moved, Math.Round(speed), hp, state, dmgInfo, VehCrew(veh)) + " armor=" + armor + " up=" + upY.ToString(1, 2));
		}
	}

	protected string VehCrew(IEntity veh)
	{
		SCR_BaseCompartmentManagerComponent cm = SCR_BaseCompartmentManagerComponent.Cast(veh.FindComponent(SCR_BaseCompartmentManagerComponent));
		if (!cm)
			return "-";

		array<BaseCompartmentSlot> slots = {};
		cm.GetCompartments(slots);
		int drivers, gunners, cargo;
		string driverAction = "-";
		string gunnerTgt = "-";
		foreach (BaseCompartmentSlot slot : slots)
		{
			IEntity occ = slot.GetOccupant();
			if (!occ)
				continue;

			SCR_CharacterDamageManagerComponent occDmg = SCR_CharacterDamageManagerComponent.Cast(occ.FindComponent(SCR_CharacterDamageManagerComponent));
			if (occDmg && occDmg.GetState() == EDamageState.DESTROYED)
				continue;

			SCR_AIUtilityComponent util = UtilityOf(occ);
			if (PilotCompartmentSlot.Cast(slot))
			{
				drivers++;
				if (util && util.GetCurrentAction())
					driverAction = util.GetCurrentAction().ClassName();
			}
			else if (TurretCompartmentSlot.Cast(slot))
			{
				gunners++;
				if (util && util.m_CombatComponent)
				{
					BaseTarget t = util.m_CombatComponent.GetCurrentTarget();
					if (t && t.GetTargetEntity())
						gunnerTgt = Math.Round(vector.Distance(veh.GetOrigin(), t.GetTargetEntity().GetOrigin())).ToString();
				}
			}
			else
				cargo++;
		}

		return string.Format("%1:%2:%3 driver=%4 gunner_tgt=%5", drivers, gunners, cargo, driverAction, gunnerTgt);
	}

	protected static SCR_AIUtilityComponent UtilityOf(IEntity character)
	{
		AIControlComponent ctrl = AIControlComponent.Cast(character.FindComponent(AIControlComponent));
		if (!ctrl || !ctrl.GetAIAgent())
			return null;

		return SCR_AIUtilityComponent.Cast(ctrl.GetAIAgent().FindComponent(SCR_AIUtilityComponent));
	}

	protected string VehName(IEntity veh)
	{
		string prefab = "veh";
		if (veh.GetPrefabData())
			prefab = FilePath.StripExtension(FilePath.StripPath(veh.GetPrefabData().GetPrefabName()));

		return string.Format("%1#%2", prefab, m_aVehicles.Find(veh));
	}

	protected void LogUnits(AIGroup aiGroup)
	{
		array<AIAgent> agents = {};
		aiGroup.GetAgents(agents);

		IEntity leaderEnt;
		AIAgent leader = aiGroup.GetLeaderAgent();
		if (leader)
			leaderEnt = leader.GetControlledEntity();

		string gposture = "-";
		SCR_AIGroup sgrp = SCR_AIGroup.Cast(aiGroup);
		if (sgrp && sgrp.GetGroupUtilityComponent())
			gposture = typename.EnumToString(DCO_GroupTactics, sgrp.GetGroupUtilityComponent().DCO_GetPosture());

		bool testHold = !m_sTestHoldGroups.IsEmpty() && ("," + m_sTestHoldGroups + ",").Contains("," + aiGroup.GetName() + ",");

		foreach (int i, AIAgent agent : agents)
		{
			IEntity ent = agent.GetControlledEntity();
			if (!ent)
				continue;

			if (testHold)
			{
				DCO_AIConfigComponent holdCfg = DCO_AIConfigComponent.Cast(agent.FindComponent(DCO_AIConfigComponent));
				if (holdCfg && !holdCfg.IsHoldPosition())
					holdCfg.SetHoldPosition(true);
			}

			vector p = ent.GetOrigin();
			string moved = "-";
			vector last;
			if (m_mLastUnitPos.Find(agent, last))
				moved = Math.Round(vector.Distance(last, p)).ToString();
			m_mLastUnitPos.Set(agent, p);

			string stance = "-";
			string veh = "0";
			string hp = "-";
			string ads = "0";
			ChimeraCharacter ch = ChimeraCharacter.Cast(ent);
			if (ch)
			{
				CharacterControllerComponent ctrl = ch.GetCharacterController();
				if (ctrl)
				{
					stance = typename.EnumToString(ECharacterStance, ctrl.GetStance());
					float adsV = 0;
					if (ctrl.IsWeaponADS())
					{
						ads = "1";
						adsV = 1;
					}
					TrackField(5, adsV);
				}
				if (ch.IsInVehicle())
				{
					veh = "1";
					CompartmentAccessComponent access = ch.GetCompartmentAccessComponent();
					if (access && access.GetCompartment())
						TrackVehicle(access.GetCompartment().GetOwner().GetRootParent());
				}
				SCR_CharacterDamageManagerComponent dmg = SCR_CharacterDamageManagerComponent.Cast(ch.GetDamageManager());
				if (dmg)
				{
					hp = dmg.GetHealthScaled().ToString(1, 2);
					TrackField(0, dmg.GetHealthScaled());
				}
			}

			string threat = "-";
			string supp = "-";
			string morale = "-";
			string action = "-";
			string tgt = "-";
			string aim = "-";
			string mag = "-";
			string lean = "-";
			string threatV = "-";
			string mstate = "-";
			string stam = DCO_AIMoraleSystem.Stamina(ent).ToString(1, 2);
			string exert = "-";
			string fat = "-";
			int spirited;
			int pinned;
			SCR_AIUtilityComponent util = SCR_AIUtilityComponent.Cast(agent.FindComponent(SCR_AIUtilityComponent));
			if (util)
			{
				if (util.m_ThreatSystem)
				{
					threat = typename.EnumToString(EAIThreatState, util.m_ThreatSystem.GetState());
					supp = util.m_ThreatSystem.GetSuppressionMeasure().ToString(1, 2);
					threatV = util.m_ThreatSystem.GetThreatMeasure().ToString(1, 2);
					TrackField(1, util.m_ThreatSystem.GetSuppressionMeasure());
					TrackField(3, util.m_ThreatSystem.GetThreatMeasure());
				}
				DCO_AIMoraleSystem ms = util.GetMoraleSystem();
				if (ms)
				{
					morale = ms.GetMoraleMeasure().ToString(1, 1);
					TrackField(2, ms.GetMoraleMeasure());
					mstate = typename.EnumToString(moraleState, ms.GetState());
					if (ms.IsSpirited())
						spirited = 1;
					exert = ms.GetExertion().ToString(1, 1);
					fat = Math.Round(ms.GetFatigueSeconds()).ToString();
				}
				if (util.m_CombatComponent && util.m_CombatComponent.DCO_IsPinned())
					pinned = 1;
				AIActionBase act = util.GetCurrentAction();
				if (act)
					action = act.ClassName();
				if (util.m_CombatComponent)
				{
					aim = util.m_CombatComponent.GetCurrentAimImprovement().ToString(1, 3);
					mag = DCO_AimUtility.GetSightMagnification(util.m_CombatComponent).ToString(1, 1);
					lean = util.m_CombatComponent.DCO_GetLean().ToString();
					TrackField(4, util.m_CombatComponent.GetCurrentAimImprovement());
					TrackField(6, DCO_AimUtility.GetSightMagnification(util.m_CombatComponent));
					TrackField(8, util.m_CombatComponent.DCO_GetLean());
					if (ch && ch.GetCharacterController())
						lean += "/" + ch.GetCharacterController().GetCurrentLeanAmount().ToString(1, 2);
					BaseTarget target = util.m_CombatComponent.GetCurrentTarget();
					if (target && target.GetTargetEntity())
					{
						string tgtType = typename.EnumToString(EAIUnitType, target.GetUnitType());
						tgtType.Replace("UnitType_", "");
						tgt = Math.Round(vector.Distance(p, target.GetTargetEntity().GetOrigin())).ToString() + ":" + tgtType;
					}
				}
			}

			string role = "M";
			if (ent == leaderEnt)
				role = "L";

			bool hold = util && util.m_DCOConfig && util.m_DCOConfig.IsHoldPosition();
			int shots = DCO_ShotCounter.Since(ent);
			if (ch && ch.IsInVehicle() && ch.GetCompartmentAccessComponent())
			{
				BaseCompartmentSlot seat = ch.GetCompartmentAccessComponent().GetCompartment();
				if (seat && seat.GetType() == ECompartmentType.TURRET && seat.GetOwner())
				{
					IEntity turretEnt = seat.GetOwner();
					int turretShots = DCO_ShotCounter.Since(turretEnt);
					if (turretEnt.GetRootParent() != turretEnt)
						turretShots += DCO_ShotCounter.Since(turretEnt.GetRootParent());
					shots += turretShots;
					DCO_Perf.CountN("veh:turret_round", turretShots);
				}
			}
			float holdV = 0;
			if (hold)
				holdV = 1;
			TrackField(7, holdV);
			TrackField(9, shots);

			Line(DCO_ELogChannel.UNIT, string.Format("UNIT grp=%1 i=%2 %3 pos=%4,%5 moved=%6 stance=%7 veh=%8 hp=%9",
				aiGroup.GetName(), i, role, Math.Round(p[0]), Math.Round(p[2]), moved, stance, veh, hp)
				+ string.Format(" threat=%1 supp=%2 morale=%3 action=%4 tgt=%5 aim=%6 ads=%7 mag=%8", threat, supp, morale, action, tgt, aim, ads, mag)
				+ string.Format(" hold=%1 lean=%2 pers=%3", hold, lean,
					typename.EnumToString(DCO_EAIPersonality, DCO_PersonalityCombatUtility.GetPersonalitySafe(util)))
				+ string.Format(" threat_v=%1 mstate=%2 spirited=%3 pinned=%4 gposture=%5 shots=%6", threatV, mstate, spirited, pinned, gposture, shots)
				+ string.Format(" stam=%1 exert=%2 fat=%3", stam, exert, fat));
		}
	}

	protected void OnControllableDestroyed(notnull SCR_InstigatorContextData data)
	{
		IEntity victim = data.GetVictimEntity();
		if (!victim)
			return;

		FactionKey victimFk = EntityFaction(victim);
		FactionKey killerFk = EntityFaction(data.GetKillerEntity());

		string kind = "infantry";
		if (Vehicle.Cast(victim))
		{
			kind = "vehicle";
			m_mVehiclesLost.Set(victimFk, Get(m_mVehiclesLost, victimFk) + 1);
		}
		else
		{
			m_mInfantryLost.Set(victimFk, Get(m_mInfantryLost, victimFk) + 1);
		}

		if (!killerFk.IsEmpty() && killerFk != victimFk)
			m_mKills.Set(killerFk, Get(m_mKills, killerFk) + 1);

		vector p = victim.GetOrigin();
		IEntity killer = data.GetKillerEntity();
		string dist = "-";
		string weapon = "-";
		if (killer == victim)
		{
			killer = null;
			killerFk = "self_or_env";
			DCO_Perf.Count("battle:killed_no_attacker");
		}
		if (killer)
		{
			dist = Math.Round(vector.Distance(killer.GetOrigin(), p)).ToString();
			BaseWeaponManagerComponent wm = BaseWeaponManagerComponent.Cast(killer.FindComponent(BaseWeaponManagerComponent));
			if (wm && wm.GetCurrentWeapon() && wm.GetCurrentWeapon().GetOwner() && wm.GetCurrentWeapon().GetOwner().GetPrefabData())
				weapon = FilePath.StripExtension(FilePath.StripPath(wm.GetCurrentWeapon().GetOwner().GetPrefabData().GetPrefabName()));
		}

		string vstate = "-";
		string vthreat = "-";
		string vmorale = "-";
		ChimeraCharacter vch = ChimeraCharacter.Cast(victim);
		if (vch)
		{
			array<string> flags = {};
			if (vch.GetCharacterController())
				flags.Insert(typename.EnumToString(ECharacterStance, vch.GetCharacterController().GetStance()));
			SCR_AIUtilityComponent vu = UtilityOf(vch);
			if (vu && vu.m_CombatMoveState && vu.m_CombatMoveState.IsInValidCover())
				flags.Insert("cover");
			if (vch.GetPhysics() && vch.GetPhysics().GetVelocity().Length() > 0.5)
				flags.Insert("moving");
			if (vu && vu.m_CombatComponent && vu.m_CombatComponent.DCO_IsPinned())
				flags.Insert("pinned");
			if (vch.IsInVehicle())
				flags.Insert("veh");
			vstate = "";
			foreach (int fi, string fl : flags)
			{
				if (fi > 0)
					vstate += "|";
				vstate += fl;
			}
			if (vu && vu.m_ThreatSystem)
				vthreat = typename.EnumToString(EAIThreatState, vu.m_ThreatSystem.GetState());
			if (vu && vu.GetMoraleSystem())
				vmorale = typename.EnumToString(moraleState, vu.GetMoraleSystem().GetState());
		}

		string ammo = "-";
		SCR_CharacterDamageManagerComponent vdm = SCR_CharacterDamageManagerComponent.Cast(victim.FindComponent(SCR_CharacterDamageManagerComponent));
		if (vdm)
			ammo = vdm.DCO_LastAmmo();
		Line(DCO_ELogChannel.COMBAT, string.Format("killed kind=%1 victim=%2 killer=%3 pos=%4,%5",
			kind, NoneIfEmpty(victimFk), NoneIfEmpty(killerFk), Math.Round(p[0]), Math.Round(p[2]))
			+ string.Format(" dist=%1 weapon=%2 kgrp=%3 vgrp=%4 vstate=%5 vthreat=%6 vmorale=%7 ammo=%8", dist, weapon, GroupNameOf(killer), GroupNameOf(victim), NoneIfEmpty(vstate), vthreat, vmorale, ammo));
		if (vch)
			DCO_Battle.DeathCtx(victim, killer);
	}

	protected string GroupNameOf(IEntity ent)
	{
		if (!ent)
			return "-";
		ChimeraCharacter ch = ChimeraCharacter.Cast(ent);
		if (!ch)
		{
			Vehicle v = Vehicle.Cast(ent.GetRootParent());
			if (v)
				return "vehicle";
			return "-";
		}
		SCR_AIUtilityComponent u = UtilityOf(ch);
		if (u && u.GetAIAgent() && u.GetAIAgent().GetParentGroup())
			return NoneIfEmpty(u.GetAIAgent().GetParentGroup().GetName());
		if (GetGame().GetPlayerManager() && GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(ent) > 0)
			return "player";
		return "-";
	}

	protected void WriteSummary(string reason)
	{
		if (m_bSummaryWritten)
			return;
		m_bSummaryWritten = true;

		float avgFps = 0;
		if (m_iFpsSamples > 0)
			avgFps = m_fFpsSum / m_iFpsSamples;

		Line(DCO_ELogChannel.SYS, string.Format("SUMMARY reason=%1 duration=%2s fps_avg=%3 fps_min=%4",
			reason, Math.Round(DCO_Util.NowS() - m_fStartTime), avgFps.ToString(1, 1), m_fFpsMin.ToString(1, 1)));

		foreach (FactionKey fk, int peak : m_mPeakAlive)
		{
			Line(DCO_ELogChannel.SYS, string.Format("SUMMARY faction=%1 peak_alive=%2 infantry_lost=%3 vehicles_lost=%4 kills=%5",
				fk, peak, Get(m_mInfantryLost, fk), Get(m_mVehiclesLost, fk), Get(m_mKills, fk)));
		}

		foreach (CMD_AICommanderObjectiveComponent obj, FactionKey owner : m_mObjOwner)
		{
			if (obj)
				Line(DCO_ELogChannel.SYS, string.Format("SUMMARY objective=%1 owner=%2", ObjName(obj), NoneIfEmpty(owner)));
		}

		for (int i = 0; i < m_aFieldCount.Count(); i++)
		{
			if (m_aFieldCount[i] >= 100 && m_aFieldMin[i] == m_aFieldMax[i])
				DCO_Log.Write(DCO_ELogChannel.PERF, DCO_ELogLevel.WARN, "-", string.Format("SUMMARY WARN const_field field=%1 value=%2 samples=%3", m_aFieldName[i], m_aFieldMin[i], m_aFieldCount[i]));
		}
	}

	protected FactionKey EntityFaction(IEntity ent)
	{
		return DCO_Util.FactionKeyOf(ent, true);
	}

	protected string ObjName(CMD_AICommanderObjectiveComponent obj)
	{
		return obj.GetLogName();
	}

	protected string NoneIfEmpty(string s)
	{
		if (s.IsEmpty())
			return "none";
		return s;
	}

	protected int Get(map<FactionKey, int> m, FactionKey k)
	{
		int v;
		m.Find(k, v);
		return v;
	}

	protected string Pad(int v)
	{
		if (v < 10)
			return "0" + v;
		return v.ToString();
	}


	protected void Line(DCO_ELogChannel ch, string s)
	{
		DCO_Log.Write(ch, DCO_ELogLevel.INFO, "-", s);
	}
}

class DCO_ShotCounter : Managed
{
	protected static ref map<IEntity, ref DCO_ShotCounter> s_mAll;

	protected IEntity m_Owner;
	protected int m_iShots;
	protected int m_iLast;
	protected BaseMagazineComponent m_LastMag;
	protected int m_iLastAmmo;
	protected float m_fLastShot_ms = -100000.0;

	static int Total(IEntity ent)
	{
		DCO_ShotCounter c = Track(ent);
		if (!c)
			return 0;
		return c.m_iShots;
	}

	static float LastShotAt(IEntity ent)
	{
		DCO_ShotCounter c = Track(ent);
		if (!c)
			return -100000.0;
		return c.m_fLastShot_ms;
	}

	static DCO_ShotCounter Track(IEntity ent)
	{
		if (!ent)
			return null;
		if (!s_mAll)
			s_mAll = new map<IEntity, ref DCO_ShotCounter>();
		DCO_ShotCounter c = s_mAll.Get(ent);
		if (c)
			return c;
		if (s_mAll.Count() > 2000)
		{
			map<IEntity, ref DCO_ShotCounter> fresh = new map<IEntity, ref DCO_ShotCounter>();
			foreach (IEntity k, DCO_ShotCounter v : s_mAll)
			{
				if (k)
					fresh.Set(k, v);
			}
			s_mAll = fresh;
		}
		c = new DCO_ShotCounter();
		c.m_Owner = ent;
		s_mAll.Set(ent, c);
		EventHandlerManagerComponent ev = EventHandlerManagerComponent.Cast(ent.FindComponent(EventHandlerManagerComponent));
		if (ev)
			ev.RegisterScriptHandler("OnAmmoCountChanged", c, c.OnAmmoCountChanged);
		return c;
	}

	static int Since(IEntity ent)
	{
		DCO_ShotCounter c = Track(ent);
		if (!c)
			return 0;
		int n = c.m_iShots - c.m_iLast;
		c.m_iLast = c.m_iShots;
		return n;
	}

	void OnAmmoCountChanged(BaseWeaponComponent weapon, BaseMuzzleComponent muzzle, BaseMagazineComponent magazine, int ammoCount, bool isBarrelChambered)
	{
		if (magazine && magazine == m_LastMag && ammoCount < m_iLastAmmo)
		{
			int n = m_iLastAmmo - ammoCount;
			m_iShots += n;
			m_fLastShot_ms = GetGame().GetWorld().GetWorldTime();
			DCO_Perf.CountN("ev:round", n);
		}
		m_LastMag = magazine;
		m_iLastAmmo = ammoCount;
	}
}

modded class SCR_CharacterDamageManagerComponent
{
	protected int m_iDCOLastDmg = -1;
	protected string m_sDCOLastSrc = "-";
	protected int m_iDCOFatalDmg = -1;
	protected string m_sDCOFatalSrc = "-";
	protected float m_fDCOFatalAt = -1;
	protected bool m_bDCOLastBleed;
	protected float m_fDCOHpLast = 1.0;
	protected float m_fDCOHpBefore = 1.0;
	protected string m_sDCOHitZone = "-";
	protected ref array<float> m_aDCOHitAt;
	protected bool m_bDCOWasUncon;
	protected bool m_bDCOWasBleeding;

	override protected void OnDamage(notnull BaseDamageContext damageContext)
	{
		super.OnDamage(damageContext);
		m_iDCOLastDmg = damageContext.damageType;
		m_sDCOLastSrc = "-";
		IEntity src = damageContext.damageSource;
		if (src && src.GetPrefabData())
			m_sDCOLastSrc = FilePath.StripExtension(FilePath.StripPath(src.GetPrefabData().GetPrefabName()));
		if (!Replication.IsServer())
			return;
		float now = DCO_Util.NowS();
		bool finisher = damageContext.damageType == EDamageType.TRUE && !damageContext.damageSource && m_iDCOFatalDmg >= 0;
		if (finisher)
		{
			m_bDCOLastBleed = m_bDCOLastBleed || m_bDCOWasBleeding || m_bDCOWasUncon;
			DCO_Perf.Count("death:finisher_true");
		}
		else
			m_bDCOLastBleed = damageContext.damageType == EDamageType.BLEEDING;
		IEntity attacker = null;
		if (damageContext.instigator)
			attacker = damageContext.instigator.GetInstigatorEntity();
		if (!m_bDCOLastBleed && !finisher)
		{
			m_iDCOFatalDmg = damageContext.damageType;
			m_sDCOFatalSrc = m_sDCOLastSrc;
			m_fDCOFatalAt = now;
			m_fDCOHpBefore = m_fDCOHpLast;
			if (damageContext.struckHitZone)
				m_sDCOHitZone = damageContext.struckHitZone.GetName();
			if (!m_aDCOHitAt)
				m_aDCOHitAt = {};
			m_aDCOHitAt.Insert(now);
			if (m_aDCOHitAt.Count() > 20)
				m_aDCOHitAt.RemoveOrdered(0);
			DCO_Battle.OnHit(GetOwner(), attacker);
			DCO_IndLearn.OnDamage(GetOwner(), attacker);
			DCO_PlyLearn.OnDamage(GetOwner(), attacker);
		}
		m_fDCOHpLast = GetHealthScaled();
		DCO_WoundCheck(attacker, damageContext.damageType);
	}

	protected void DCO_WoundCheck(IEntity attacker, int dmgType)
	{
		ChimeraCharacter ch = ChimeraCharacter.Cast(GetOwner());
		if (!ch)
			return;
		bool uncon = ch.GetCharacterController() && ch.GetCharacterController().GetLifeState() == ECharacterLifeState.INCAPACITATED;
		bool bleeding = IsBleeding();
		string state = "";
		if (uncon && !m_bDCOWasUncon)
			state = "incapacitated";
		else if (bleeding && !m_bDCOWasBleeding)
			state = "bleeding";
		m_bDCOWasUncon = uncon;
		m_bDCOWasBleeding = bleeding;
		if (state.IsEmpty())
			return;
		DCO_Battle.Wounded(ch, attacker, state, typename.EnumToString(EDamageType, dmgType), m_sDCOLastSrc);
	}

	string DCO_LastAmmo()
	{
		if (m_iDCOFatalDmg >= 0)
			return typename.EnumToString(EDamageType, m_iDCOFatalDmg) + ":" + m_sDCOFatalSrc;
		if (m_iDCOLastDmg < 0)
			return "-";
		return typename.EnumToString(EDamageType, m_iDCOLastDmg) + ":" + m_sDCOLastSrc;
	}

	float DCO_HpBefore()		{ return m_fDCOHpBefore; }
	string DCO_LastHitZone()	{ return m_sDCOHitZone; }
	string DCO_FatalSrc()		{ return m_sDCOFatalSrc; }
	float DCO_FatalAt()			{ return m_fDCOFatalAt; }
	bool DCO_LastWasBleed()		{ return m_bDCOLastBleed; }

	string DCO_FatalType()
	{
		if (m_iDCOFatalDmg < 0)
			return "-";
		return typename.EnumToString(EDamageType, m_iDCOFatalDmg);
	}

	int DCO_HitsWithin(float sec)
	{
		if (!m_aDCOHitAt)
			return 0;
		float cut = DCO_Util.NowS() - sec;
		int n = 0;
		foreach (float t : m_aDCOHitAt)
		{
			if (t >= cut)
				n++;
		}
		return n;
	}
}
