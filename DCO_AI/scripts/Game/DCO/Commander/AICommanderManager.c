[ComponentEditorProps(category: "GameScripted/Commander")]
class AICommander_ManagerComponentClass : ScriptComponentClass
{
}

class CMD_ObjectiveContextCache
{
	ref array<CMD_AICommanderObjectiveComponent> m_aReconObjs  = new array<CMD_AICommanderObjectiveComponent>();
}

class AICommander_ManagerComponent : ScriptComponent
{
	[Attribute("0", UIWidgets.CheckBox, desc: "Gambar overlay manager (tally faction, commander, objective) di posisi entity ini.", category: "Manager Debug")]
	protected bool m_bDebugMode;

	[Attribute("1.0", UIWidgets.EditBox, "Interval (detik) gambar ulang overlay manager.", category: "Manager Debug")]
	protected float m_fDebugRefreshInterval;

	protected ref array<ref Shape> m_aDebugShapes = new array<ref Shape>();
	protected ref array<ref DebugTextWorldSpace> m_aDebugTexts = new array<ref DebugTextWorldSpace>();
	protected float m_fDebugTimer = 0.0;

	[Attribute("0", UIWidgets.CheckBox, "DEPRECATED, gak dipake lagi. LOD AI sekarang diatur Global AI (DCO_GlobalAIComponent > LOD Mode / atribut GM global).", category: "Simulation")]
	protected bool m_bPreventUseLOD;

	ref array<FactionKey> m_aAvailableFactions = {};

	ref array<AICommander_BaseComponent> m_aCommander = {};

	ref array<CMD_AICommanderObjectiveComponent> m_aObjective = {};

	protected static AICommander_ManagerComponent s_Instance;

	[RplProp()]
	protected string m_sCommanderRoster;

	protected const string ROSTER_ENTRY_SEP = ";";
	protected const string ROSTER_FIELD_SEP = "|";

	protected ref array<string> m_aRosterUIDs = {};
	protected ref array<FactionKey> m_aRosterFactions = {};
	protected string m_sParsedRoster;

	protected int m_iAutoUIDCounter = 0;

	bool RegisterGroup(DCO_GroupUtilityComponent grp)
	{
		AssignGroupToCommander(grp);

		return true;
	}

	bool RegisterVehicle(IEntity veh)
	{
		AssignVehicleToCommander(veh);

		return true;
	}

	bool RegisterObjective(CMD_AICommanderObjectiveComponent obj)
	{
		if (!m_aObjective.Contains(obj))
			m_aObjective.Insert(obj);

		return true;
	}

	protected bool m_bDCOAutoSpawned;

	protected void DespawnIfUnused()
	{
		if (!m_bDCOAutoSpawned || !Replication.IsServer() || !GetGame() || !GetGame().GetWorld())
			return;

		if (!m_aCommander.IsEmpty() || !m_aObjective.IsEmpty())
			return;

		GetGame().GetCallqueue().Remove(DespawnNow);
		GetGame().GetCallqueue().CallLater(DespawnNow, 0);
	}

	protected void DespawnNow()
	{
		if (!m_aCommander.IsEmpty() || !m_aObjective.IsEmpty())
			return;

		Print("[CMD_Manager] Commander & objective habis -- manager auto-spawn dihapus");
		SCR_EntityHelper.DeleteEntityAndChildren(GetOwner());
	}

	void UnregisterObjective(CMD_AICommanderObjectiveComponent obj)
	{
		m_aObjective.RemoveItem(obj);
		DespawnIfUnused();
	}

	bool RegisterCommander(AICommander_BaseComponent cmd)
	{
		if (!cmd)
			return false;

		if (Replication.IsServer())
			EnsureUniqueCommanderUID(cmd);

		if (!m_aCommander.Contains(cmd))
			m_aCommander.Insert(cmd);

		Print("REGISTERING : " + cmd.GetCommanderUID() + " FACTION : " + cmd.GetCommanderFactionKey());

		if (Replication.IsServer())
			RebuildCommanderRoster();

		return true;
	}

	bool UnregisterCommander(AICommander_BaseComponent cmd)
	{
		if (!cmd)
			return false;

		if (m_aCommander.Contains(cmd))
			m_aCommander.RemoveItem(cmd);

		Print("UNREGISTERING : " + cmd.GetCommanderUID() + " FACTION : " + cmd.GetCommanderFactionKey());

		if (Replication.IsServer())
			RebuildCommanderRoster();

		DespawnIfUnused();

		return true;
	}

	protected void EnsureUniqueCommanderUID(AICommander_BaseComponent cmd)
	{
		string uid = cmd.GetCommanderUID();

		bool invalid = uid.IsEmpty() || uid.Contains(ROSTER_ENTRY_SEP) || uid.Contains(ROSTER_FIELD_SEP);
		if (!invalid && !IsCommanderUIDTaken(uid, cmd))
			return;

		string prefix = cmd.GetCommanderFactionKey();
		if (prefix.IsEmpty())
			prefix = "CMD";

		string newUid;
		while (true)
		{
			m_iAutoUIDCounter++;
			newUid = prefix + "-" + m_iAutoUIDCounter.ToString();
			if (!IsCommanderUIDTaken(newUid, cmd))
				break;
		}

		Print(string.Format("[CMD_Manager] Commander UID '%1' invalid/duplicate -> '%2'", uid, newUid), LogLevel.WARNING);
		cmd.SetCommanderUID(newUid);
	}

	protected bool IsCommanderUIDTaken(string uid, AICommander_BaseComponent self)
	{
		foreach (AICommander_BaseComponent other : m_aCommander)
		{
			if (!other || other == self)
				continue;

			if (other.GetCommanderUID() == uid)
				return true;
		}

		return false;
	}

	protected void RebuildCommanderRoster()
	{
		array<string> entries = {};
		foreach (AICommander_BaseComponent cmd : m_aCommander)
		{
			if (!cmd)
				continue;

			entries.Insert(cmd.GetCommanderUID() + ROSTER_FIELD_SEP + cmd.GetCommanderFactionKey());
		}

		entries.Sort();

		string roster;
		for (int i = 0; i < entries.Count(); i++)
		{
			if (i > 0)
				roster = roster + ROSTER_ENTRY_SEP;
			roster = roster + entries[i];
		}

		if (roster == m_sCommanderRoster)
			return;

		m_sCommanderRoster = roster;
		Replication.BumpMe();
	}

	protected void ParseCommanderRosterIfChanged()
	{
		if (m_sParsedRoster == m_sCommanderRoster)
			return;

		m_sParsedRoster = m_sCommanderRoster;
		m_aRosterUIDs.Clear();
		m_aRosterFactions.Clear();

		if (m_sCommanderRoster.IsEmpty())
			return;

		array<string> entries = {};
		m_sCommanderRoster.Split(ROSTER_ENTRY_SEP, entries, true);

		foreach (string entry : entries)
		{
			array<string> fields = {};
			entry.Split(ROSTER_FIELD_SEP, fields, false);
			if (fields.IsEmpty() || fields[0].IsEmpty())
				continue;

			m_aRosterUIDs.Insert(fields[0]);
			if (fields.Count() >= 2)
				m_aRosterFactions.Insert(fields[1]);
			else
				m_aRosterFactions.Insert(string.Empty);
		}
	}

	void DCO_GetRoster(notnull array<string> outUIDs, notnull array<FactionKey> outFactions)
	{
		ParseCommanderRosterIfChanged();
		outUIDs.Copy(m_aRosterUIDs);
		outFactions.Copy(m_aRosterFactions);
	}

	string DCO_GetRosterString()	{ return m_sCommanderRoster; }

	int GetRosterUIDsForFaction(FactionKey fk, notnull out array<string> outUIDs)
	{
		outUIDs.Clear();
		ParseCommanderRosterIfChanged();

		for (int i = 0; i < m_aRosterUIDs.Count(); i++)
		{
			if (m_aRosterFactions[i] == fk)
				outUIDs.Insert(m_aRosterUIDs[i]);
		}

		return outUIDs.Count();
	}

	bool RenameCommander(AICommander_BaseComponent cmd, string newUID)
	{
		if (!Replication.IsServer() || !cmd)
			return false;

		if (newUID.IsEmpty() || newUID.Contains(ROSTER_ENTRY_SEP) || newUID.Contains(ROSTER_FIELD_SEP))
		{
			Print(string.Format("[CMD_Manager] Nama commander '%1' gak valid (kosong atau pake ';' / '|')", newUID), LogLevel.WARNING);
			return false;
		}

		if (newUID == cmd.GetCommanderUID())
			return true;

		if (IsCommanderUIDTaken(newUID, cmd))
		{
			Print(string.Format("[CMD_Manager] Nama commander '%1' udah dipake commander lain", newUID), LogLevel.WARNING);
			return false;
		}

		cmd.SetCommanderUID(newUID);
		RebuildCommanderRoster();
		return true;
	}

	void OnCommanderFactionChanged(AICommander_BaseComponent cmd)
	{
		if (Replication.IsServer())
			RebuildCommanderRoster();
	}

	AICommander_BaseComponent FindCommanderByUID(string uid)
	{
		if (uid.IsEmpty())
			return null;

		foreach (AICommander_BaseComponent cmd : m_aCommander)
		{
			if (cmd && cmd.GetCommanderUID() == uid)
				return cmd;
		}

		return null;
	}

	AICommander_BaseComponent PickCommanderForFaction(FactionKey fk, string dedicatedUID)
	{
		if (!dedicatedUID.IsEmpty())
			return FindCommanderByUID(dedicatedUID);

		array<AICommander_BaseComponent> pool = {};
		foreach (AICommander_BaseComponent cmd : m_aCommander)
		{
			if (cmd && cmd.GetCommanderFactionKey() == fk)
				pool.Insert(cmd);
		}

		if (pool.IsEmpty())
			return null;

		return pool.GetRandomElement();
	}

	void InitializeCommanderManager()
	{
		if (!m_aAvailableFactions.IsEmpty())
			return;

		if (!GetGame().GetFactionManager())
		{
			Print("[CMD_Manager] FactionManager belum ada -- daftar faction kosong", LogLevel.WARNING);
			return;
		}

		array<Faction> AvailableFactions = {};
		FactionManager fm = GetGame().GetFactionManager();

		fm.GetFactionsList(AvailableFactions);
		foreach(Faction f : AvailableFactions)
		{
			m_aAvailableFactions.Insert(f.GetFactionKey());
		}
	}

	bool IsObjectiveIntelCovered(CMD_AICommanderObjectiveComponent target, FactionKey fk)
	{
		if (!target)
			return false;

		vector targetPos = target.GetOwner().GetOrigin();

		foreach (CMD_AICommanderObjectiveComponent obj : m_aObjective)
		{
			if (!obj || obj == target)
				continue;

			if (obj.GetObjectiveType() != CMD_EObjectiveType.RECON)
				continue;

			if (!obj.IsReconObjectiveActive(fk))
				continue;

			float coverRadius = obj.GetIntelCoverageRadius();
			if (vector.DistanceSq(obj.GetOwner().GetOrigin(), targetPos) <= coverRadius * coverRadius)
				return true;
		}

		return false;
	}

	CMD_ObjectiveContextCache BuildObjectiveContext(FactionKey fk)
	{
		CMD_ObjectiveContextCache ctx = new CMD_ObjectiveContextCache();

		foreach (CMD_AICommanderObjectiveComponent obj : m_aObjective)
		{
			if (!obj)
				continue;

			if (obj.GetObjectiveType() == CMD_EObjectiveType.RECON && obj.IsReconObjectiveActive(fk))
				ctx.m_aReconObjs.Insert(obj);
		}

		return ctx;
	}

	bool IsObjectiveIntelCoveredCached(CMD_AICommanderObjectiveComponent target, CMD_ObjectiveContextCache ctx)
	{
		if (!target || !ctx)
			return false;

		vector targetPos = target.GetOwner().GetOrigin();

		foreach (CMD_AICommanderObjectiveComponent obj : ctx.m_aReconObjs)
		{
			if (!obj || obj == target)
				continue;

			float coverRadius = obj.GetIntelCoverageRadius();
			if (vector.DistanceSq(obj.GetOwner().GetOrigin(), targetPos) <= coverRadius * coverRadius)
				return true;
		}

		return false;
	}

	void GetTopObjectivesOffensive(AICommander_BaseComponent forCommander, int count, out array<CMD_AICommanderObjectiveComponent> result)
	{
		result = {};
		if (!forCommander)
			return;

		FactionKey fk  = forCommander.GetCommanderFactionKey();
		float worldTime = GetGame().GetWorld().GetWorldTime() / 1000.0;

		array<float> scores = {};
		array<CMD_AICommanderObjectiveComponent> sorted = {};

		foreach (CMD_AICommanderObjectiveComponent obj : m_aObjective)
		{
			if (!obj)
				continue;

			if (obj.IsCommanderBlackListed(forCommander.GetCommanderUID()))
				continue;

			if (forCommander.IsObjectiveOnCooldown(obj))
				continue;

			if (obj.IsCapturedBy(fk, forCommander.GetCommanderUID()))
				continue;

			float score = obj.ComputePriorityScore(fk, worldTime, forCommander.GetOwner().GetOrigin(), forCommander.GetCombatFocus(), forCommander.GetCommanderUID());

			bool inserted = false;
			for (int i = 0; i < sorted.Count(); i++)
			{
				if (score > scores[i])
				{
					sorted.InsertAt(obj, i);
					scores.InsertAt(score, i);
					inserted = true;
					break;
				}
			}

			if (!inserted)
			{
				sorted.Insert(obj);
				scores.Insert(score);
			}
		}

		int take = Math.Min(count, sorted.Count());
		for (int i = 0; i < take; i++)
			result.Insert(sorted[i]);
	}

	void AssignGroupToCommander(DCO_GroupUtilityComponent grp)
	{
		if (!grp)
			return;

		FactionKey grpFk = grp.GetFactionKey();

		array<AICommander_BaseComponent> availCommanders = {};
		foreach (AICommander_BaseComponent cmd : m_aCommander)
		{
			if (cmd && cmd.GetCommanderFactionKey() == grpFk)
				availCommanders.Insert(cmd);
		}

		if (availCommanders.IsEmpty())
		{
			return;
		}

		AICommander_BaseComponent chosen = null;
		int leastGroups = int.MAX;

		foreach (AICommander_BaseComponent cmd : availCommanders)
		{
			if (grp.IsCommanderBlacklisted(cmd.GetCommanderUID()))
				continue;

			if (!grp.DedicatedCommander().IsEmpty())
			{
				if (cmd.GetCommanderUID() == grp.DedicatedCommander())
				{
					chosen = cmd;
					break;
				}
			}

			int groupCount = cmd.GetOwnedGroupCount();
			if (groupCount < leastGroups)
			{
				leastGroups = groupCount;
				chosen = cmd;
			}
		}

		if (!chosen)
			return;

		chosen.RegisterGroup(grp);
		grp.RegisterCommanderToGroup(chosen);
		Print(string.Format("[CMD_Manager] Group '%1' → Commander '%2'",
			grp.GetOwner().GetName(), chosen.GetCommanderUID()));
	}

	void AssignVehicleToCommander(IEntity grp)
	{
		if (!grp)
			return;
 		SCR_VehicleFactionAffiliationComponent fac = SCR_VehicleFactionAffiliationComponent.Cast(grp.FindComponent(SCR_VehicleFactionAffiliationComponent));
		DCO_TransportMissionComponent comp = DCO_TransportMissionComponent.Cast(grp.FindComponent(DCO_TransportMissionComponent));
		if (!fac || !comp)
			return;

		FactionKey grpFk;
		if (fac.GetAffiliatedFaction())
			grpFk = fac.GetAffiliatedFactionKey();
		else
			grpFk = fac.GetDefaultFactionKey();

		array<AICommander_BaseComponent> availCommanders = {};
		foreach (AICommander_BaseComponent cmd : m_aCommander)
		{
			if (cmd && cmd.GetCommanderFactionKey() == grpFk)
				availCommanders.Insert(cmd);
		}

		if (availCommanders.IsEmpty())
		{
			return;
		}

		AICommander_BaseComponent chosen = null;
		int leastGroups = int.MAX;

		foreach (AICommander_BaseComponent cmd : availCommanders)
		{
			int groupCount = cmd.GetOwnedVehicle();
			if (groupCount < leastGroups)
			{
				leastGroups = groupCount;
				chosen = cmd;
			}
		}

		if (!chosen)
			return;

		chosen.RegisterVehicle(grp);
		comp.AssignCommanderOwner(chosen);
	}

	bool UnregisterGroup(DCO_GroupUtilityComponent grp)
	{
		foreach (AICommander_BaseComponent cmd : m_aCommander)
		{
			if (cmd.IsGroupHere(grp))
				cmd.UnregisterGroup(grp);
		}

		return true;
	}

	static AICommander_ManagerComponent GetInstance()
	{
		return s_Instance;
	}

	static AICommander_ManagerComponent GetOrSpawnInstance(ResourceName managerPrefab, IEntity context)
	{
		if (s_Instance)
			return s_Instance;

		if (!Replication.IsServer() || !context || !GetGame().InPlayMode())
			return null;

		if (managerPrefab.IsEmpty())
		{
			Print(string.Format("[CMD_Manager] %1 init tanpa manager dan Manager Prefab kosong -- gak di-spawn", context.GetName()), LogLevel.WARNING);
			return null;
		}

		Resource res = Resource.Load(managerPrefab);
		if (!res || !res.IsValid())
		{
			Print(string.Format("[CMD_Manager] Manager Prefab gak valid: %1", managerPrefab), LogLevel.ERROR);
			return null;
		}

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		params.Transform[3] = context.GetOrigin();

		IEntity ent = GetGame().SpawnEntityPrefab(res, context.GetWorld(), params);
		if (!ent)
		{
			Print(string.Format("[CMD_Manager] Gagal spawn manager: %1", managerPrefab), LogLevel.ERROR);
			return null;
		}

		if (!s_Instance)
		{
			Print(string.Format("[CMD_Manager] Prefab %1 gak punya AICommander_ManagerComponent", managerPrefab), LogLevel.ERROR);
			return null;
		}

		s_Instance.InitializeCommanderManager();
		s_Instance.m_bDCOAutoSpawned = true;

		Print(string.Format("[CMD_Manager] Manager di-spawn otomatis oleh %1", context.GetName()));
		return s_Instance;
	}

	void AICommander_ManagerComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		if (!s_Instance)
			s_Instance = this;
	}

	void ~AICommander_ManagerComponent()
	{
		if (s_Instance == this)
			s_Instance = null;
	}

	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);

		SetEventMask(owner, EntityEvent.FRAME);
	}

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		UpdateManagerDebug(timeSlice);

		if (Replication.IsServer())
		{
			DCO_PlayerAwareness.Tick(timeSlice);
			DCO_PlayerContactReports.Tick(timeSlice);
			DCO_PlayerRequests.Tick(timeSlice);
			DCO_TerrainCache.Tick(timeSlice);
		}
	}

	protected void UpdateManagerDebug(float timeSlice)
	{
		if (!m_bDebugMode)
		{
			if (!m_aDebugShapes.IsEmpty() || !m_aDebugTexts.IsEmpty())
			{
				m_aDebugShapes.Clear();
				m_aDebugTexts.Clear();
			}
			return;
		}

		m_fDebugTimer += timeSlice;
		if (m_fDebugTimer < m_fDebugRefreshInterval)
			return;

		m_fDebugTimer = 0.0;

		m_aDebugShapes.Clear();
		m_aDebugTexts.Clear();

		if (!DCO_DebugDraw.IsLocalPlayerInGM())
			return;

		vector p = GetOwner().GetOrigin();

		m_aDebugShapes.Insert(Shape.CreateSphere(
			DCO_DebugDraw.COLOR_MANAGER, DCO_DebugDraw.Flags(), p, 2.0));

		m_aDebugTexts.Insert(DCO_DebugDraw.SpawnText(
			Vector(p[0], p[1] + 20.0, p[2]), BuildManagerDebugText(), 18.0, DCO_DebugDraw.COLOR_MANAGER));
	}

	protected string BuildManagerDebugText()
	{
		string body = string.Format(
			"COMMANDER MANAGER\nfactions %1   commanders %2   objectives %3",
			m_aAvailableFactions.Count(),
			m_aCommander.Count(),
			m_aObjective.Count());

		foreach (FactionKey fk : m_aAvailableFactions)
		{
			int cmdCount = 0;
			foreach (AICommander_BaseComponent c : m_aCommander)
			{
				if (c && c.GetCommanderFactionKey() == fk)
					cmdCount = cmdCount + 1;
			}

			int held = 0;
			foreach (CMD_AICommanderObjectiveComponent o : m_aObjective)
			{
				if (o && o.GetOwningFaction() == fk)
					held = held + 1;
			}

			body = body + string.Format("\n  %1 : %2 cmd, %3 objectives", fk, cmdCount, held);
		}

		int neutral = 0;
		foreach (CMD_AICommanderObjectiveComponent n : m_aObjective)
		{
			if (n && n.GetOwningFaction().IsEmpty())
				neutral = neutral + 1;
		}

		return body + string.Format("\n  Neutral : %1 objectives", neutral);
	}

	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		InitializeCommanderManager();
	}
}