[ComponentEditorProps(category: "GameScripted/DCO", description: "Benchmark logger -- nulis state commander/objective/grup + event ke file sendiri di $profile.")]
class DCO_BenchmarkLoggerComponentClass : ScriptComponentClass
{
}

class DCO_BenchmarkLoggerComponent : ScriptComponent
{
	[Attribute("DCO_Bench", UIWidgets.EditBox, "Folder di dalam $profile.", category: "Benchmark")]
	protected string m_sFolder;

	[Attribute("run", UIWidgets.EditBox, "Prefix nama file log (nama skenario/varian test).", category: "Benchmark")]
	protected string m_sRunLabel;

	[Attribute("10", UIWidgets.EditBox, "Interval (detik) baris TICK: FPS, AI hidup per faction, commander, objective.", category: "Benchmark")]
	protected float m_fTickInterval;

	[Attribute("30", UIWidgets.EditBox, "Interval (detik) dump detail per grup. 0 = matiin.", category: "Benchmark")]
	protected float m_fGroupInterval;

	[Attribute("1", UIWidgets.CheckBox, "Ikut dump baris UNIT per anggota grup (posisi, stance, action, dll) tiap Group Interval.", category: "Benchmark")]
	protected bool m_bLogUnits;

	[Attribute("1", UIWidgets.CheckBox, "Dump baris VEH per kendaraan yang pernah dinaikin AI (damage, crew, speed) tiap Group Interval.", category: "Benchmark")]
	protected bool m_bLogVehicles;

	[Attribute("", UIWidgets.EditBox, "TES: nama grup (pisah koma) yang di-Hold Position terus, kayak GM nahan grup itu.", category: "Benchmark")]
	protected string m_sTestHoldGroups;

	protected static DCO_BenchmarkLoggerComponent s_Instance;

	protected ref map<AIAgent, vector> m_mLastUnitPos = new map<AIAgent, vector>();
	protected ref array<IEntity> m_aVehicles = {};
	protected ref map<IEntity, vector> m_mLastVehPos = new map<IEntity, vector>();
	protected ref map<IEntity, string> m_mVehState = new map<IEntity, string>();
	protected string m_sFilePath;
	protected float m_fStartTime;
	protected float m_fNextGroupDump;
	protected bool m_bSummaryWritten;

	protected ref map<FactionKey, int> m_mInfantryLost = new map<FactionKey, int>();
	protected ref map<FactionKey, int> m_mVehiclesLost = new map<FactionKey, int>();
	protected ref map<FactionKey, int> m_mKills = new map<FactionKey, int>();
	protected ref map<FactionKey, int> m_mPeakAlive = new map<FactionKey, int>();
	protected ref map<CMD_AICommanderObjectiveComponent, FactionKey> m_mObjOwner = new map<CMD_AICommanderObjectiveComponent, FactionKey>();
	protected ref map<AICommander_BaseComponent, int> m_mShellsFired = new map<AICommander_BaseComponent, int>();
	protected ref array<string> m_aBuffer = {};

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

		int y, mo, d, h, mi, s;
		System.GetYearMonthDay(y, mo, d);
		System.GetHourMinuteSecond(h, mi, s);

		string folder = "$profile:" + m_sFolder;
		FileIO.MakeDirectory(folder);
		m_sFilePath = string.Format("%1/%2_%3%4%5_%6%7%8.log", folder, m_sRunLabel,
			y, Pad(mo), Pad(d), Pad(h), Pad(mi), Pad(s));

		m_fStartTime = Now();
		m_fNextGroupDump = m_fGroupInterval;
		s_Instance = this;

		Line(string.Format("START label=%1 date=%2-%3-%4 %5:%6:%7", m_sRunLabel, y, Pad(mo), Pad(d), Pad(h), Pad(mi), Pad(s)));
		Flush();

		SCR_BaseGameMode gm = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (gm)
			gm.GetOnControllableDestroyed().Insert(OnControllableDestroyed);

		GetGame().GetCallqueue().CallLater(Tick, m_fTickInterval * 1000, true);
	}

	override void OnDelete(IEntity owner)
	{
		GetGame().GetCallqueue().Remove(Tick);
		if (s_Instance == this)
			s_Instance = null;

		SCR_BaseGameMode gm = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (gm)
			gm.GetOnControllableDestroyed().Remove(OnControllableDestroyed);

		if (!m_sFilePath.IsEmpty())
		{
			WriteSummary("world_end");
			Flush();
		}

		super.OnDelete(owner);
	}

	protected void Tick()
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();

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

		Line(string.Format("TICK fps=%1 alive%2", fps.ToString(1, 1), aliveStr));

		if (mgr)
		{
			LogCommanders(mgr);
			LogObjectives(mgr);

			float t = Now() - m_fStartTime;
			if (m_fGroupInterval > 0 && t >= m_fNextGroupDump)
			{
				m_fNextGroupDump = t + m_fGroupInterval;
				LogGroups(mgr);
			}
		}

		if (!m_bSummaryWritten)
		{
			foreach (FactionKey fk, int peak : m_mPeakAlive)
			{
				if (peak > 0 && Get(alive, fk) == 0)
				{
					Line(string.Format("EVENT faction_eliminated faction=%1", fk));
					WriteSummary("eliminated_" + fk);
					break;
				}
			}
		}

		Flush();
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
					Line(string.Format("EVENT artillery cmd=%1 shells=%2 total=%3", cmd.GetCommanderUID(), shells - prev, shells));
				m_mShellsFired.Set(cmd, shells);
			}

			Line(string.Format("CMD uid=%1 faction=%2 mode=%3 groups=%4 manpower=%5 reserve=%6 vehicles=%7 shells=%8",
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
				Line(string.Format("EVENT objective_owner obj=%1 from=%2 to=%3", ObjName(obj), NoneIfEmpty(prevOwner), NoneIfEmpty(owner)));
			m_mObjOwner.Set(obj, owner);

			string control;
			obj.GetControlSnapshot(keys, values);
			for (int i = 0; i < keys.Count(); i++)
				control += string.Format(" %1:%2", keys[i], values[i].ToString(1, 0));

			Line(string.Format("OBJ name=%1 owner=%2 control=[%3 ]", ObjName(obj), NoneIfEmpty(owner), control));
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
				Line(string.Format("GRP cmd=%1 name=%2 role=%3 status=%4 units=%5 posture=%6 action=%7 obj=%8 pos=%9",
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
		if (s_Instance)
			s_Instance.Line("EVENT " + text);
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
				Line(string.Format("EVENT veh_state veh=%1 from=%2 to=%3", name, prevState, state));
			m_mVehState.Set(veh, state);

			string armor = "light";
			if (DCO_VehicleCombat.IsArmored(veh))
				armor = "armored";

			Line(string.Format("VEH veh=%1 pos=%2,%3 moved=%4 speed=%5 hp=%6 state=%7 %8 crew=%9",
				name, Math.Round(p[0]), Math.Round(p[2]), moved, Math.Round(speed), hp, state, dmgInfo, VehCrew(veh)) + " armor=" + armor);
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
					if (ctrl.IsWeaponADS())
						ads = "1";
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
					hp = dmg.GetHealthScaled().ToString(1, 2);
			}

			string threat = "-";
			string supp = "-";
			string morale = "-";
			string action = "-";
			string tgt = "-";
			string aim = "-";
			string mag = "-";
			string lean = "-";
			SCR_AIUtilityComponent util = SCR_AIUtilityComponent.Cast(agent.FindComponent(SCR_AIUtilityComponent));
			if (util)
			{
				if (util.m_ThreatSystem)
				{
					threat = typename.EnumToString(EAIThreatState, util.m_ThreatSystem.GetState());
					supp = util.m_ThreatSystem.GetSuppressionMeasure().ToString(1, 2);
				}
				DCO_AIMoraleSystem ms = util.GetMoraleSystem();
				if (ms)
					morale = ms.GetMoraleMeasure().ToString(1, 1);
				AIActionBase act = util.GetCurrentAction();
				if (act)
					action = act.ClassName();
				if (util.m_CombatComponent)
				{
					aim = util.m_CombatComponent.GetCurrentAimImprovement().ToString(1, 3);
					mag = DCO_AimUtility.GetSightMagnification(util.m_CombatComponent).ToString(1, 1);
					lean = util.m_CombatComponent.DCO_GetLean().ToString();
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

			Line(string.Format("UNIT grp=%1 i=%2 %3 pos=%4,%5 moved=%6 stance=%7 veh=%8 hp=%9",
				aiGroup.GetName(), i, role, Math.Round(p[0]), Math.Round(p[2]), moved, stance, veh, hp)
				+ string.Format(" threat=%1 supp=%2 morale=%3 action=%4 tgt=%5 aim=%6 ads=%7 mag=%8", threat, supp, morale, action, tgt, aim, ads, mag)
				+ string.Format(" hold=%1 lean=%2 pers=%3", util && util.m_DCOConfig && util.m_DCOConfig.IsHoldPosition(), lean,
					typename.EnumToString(DCO_EAIPersonality, DCO_PersonalityCombatUtility.GetPersonalitySafe(util))));
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
		Line(string.Format("EVENT killed kind=%1 victim=%2 killer=%3 pos=%4,%5",
			kind, NoneIfEmpty(victimFk), NoneIfEmpty(killerFk), Math.Round(p[0]), Math.Round(p[2])));
	}

	protected void WriteSummary(string reason)
	{
		if (m_bSummaryWritten)
			return;
		m_bSummaryWritten = true;

		float avgFps = 0;
		if (m_iFpsSamples > 0)
			avgFps = m_fFpsSum / m_iFpsSamples;

		Line(string.Format("SUMMARY reason=%1 duration=%2s fps_avg=%3 fps_min=%4",
			reason, Math.Round(Now() - m_fStartTime), avgFps.ToString(1, 1), m_fFpsMin.ToString(1, 1)));

		foreach (FactionKey fk, int peak : m_mPeakAlive)
		{
			Line(string.Format("SUMMARY faction=%1 peak_alive=%2 infantry_lost=%3 vehicles_lost=%4 kills=%5",
				fk, peak, Get(m_mInfantryLost, fk), Get(m_mVehiclesLost, fk), Get(m_mKills, fk)));
		}

		foreach (CMD_AICommanderObjectiveComponent obj, FactionKey owner : m_mObjOwner)
		{
			if (obj)
				Line(string.Format("SUMMARY objective=%1 owner=%2", ObjName(obj), NoneIfEmpty(owner)));
		}
	}

	protected FactionKey EntityFaction(IEntity ent)
	{
		if (!ent)
			return string.Empty;

		FactionAffiliationComponent fac = FactionAffiliationComponent.Cast(ent.FindComponent(FactionAffiliationComponent));
		if (!fac)
			return string.Empty;

		Faction f = fac.GetAffiliatedFaction();
		if (!f)
			f = fac.GetDefaultAffiliatedFaction();
		if (!f)
			return string.Empty;

		return f.GetFactionKey();
	}

	protected string ObjName(CMD_AICommanderObjectiveComponent obj)
	{
		string n = obj.GetObjectiveName();
		if (n.IsEmpty())
			n = obj.GetOwner().GetName();
		return n;
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

	protected float Now()
	{
		return GetGame().GetWorld().GetWorldTime() / 1000.0;
	}

	protected void Line(string s)
	{
		float t = Now() - m_fStartTime;
		m_aBuffer.Insert(string.Format("[T+%1] %2", t.ToString(7, 1), s));
	}

	protected void Flush()
	{
		if (m_aBuffer.IsEmpty() || m_sFilePath.IsEmpty())
			return;

		FileHandle f = FileIO.OpenFile(m_sFilePath, FileMode.APPEND);
		if (!f)
			f = FileIO.OpenFile(m_sFilePath, FileMode.WRITE);
		if (!f)
			return;

		foreach (string s : m_aBuffer)
			f.WriteLine(s);
		f.Close();
		m_aBuffer.Clear();
	}
}
