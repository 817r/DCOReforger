[ComponentEditorProps(category: "GameScripted/Commander")]
class AICommander_ManagerComponentClass : ScriptComponentClass
{
}

// === ADDED: Optimasi -- top-level class (bukan nested), konsisten sama pola
// CMD_ThreatCluster/CMD_ContactReport/dll di file lain. Nyimpen hasil precompute
// state objective per Think() cycle, dipake bareng oleh ComputeObjectiveConnectivityCached
// dan IsObjectiveIntelCoveredCached biar gak perlu scan+state-lookup ulang per objective.
class CMD_ObjectiveContextCache
{
	ref array<CMD_AICommanderObjectiveComponent> m_aReconObjs  = new array<CMD_AICommanderObjectiveComponent>();
}
// === END ADDED ===

class AICommander_ManagerComponent : ScriptComponent
{
	// === ADDED: Manager Debug ===
	[Attribute("0", UIWidgets.CheckBox, desc: "Gambar overlay manager (tally faction, commander, objective) di posisi entity ini.", category: "Manager Debug")]
	protected bool m_bDebugMode;

	[Attribute("1.0", UIWidgets.EditBox, "Interval (detik) gambar ulang overlay manager.", category: "Manager Debug")]
	protected float m_fDebugRefreshInterval;

	protected ref array<ref Shape> m_aDebugShapes = new array<ref Shape>();
	protected ref array<ref DebugTextWorldSpace> m_aDebugTexts = new array<ref DebugTextWorldSpace>();
	protected float m_fDebugTimer = 0.0;
	// === END ADDED ===

	[Attribute("0", UIWidgets.EditBox, "Is Use LOD Or No", category: "Simulation")]
	protected bool m_bPreventUseLOD;
	
	ref array<FactionKey> m_aAvailableFactions = {};
	
	ref array<AICommander_BaseComponent> m_aCommander = {};
	
	ref array<CMD_AICommanderObjectiveComponent> m_aObjective = {};
	
	protected static AICommander_ManagerComponent s_Instance;
	
	// === ADDED: Commander Roster ===
	//! Daftar commander yang di-replicate ke client dalam bentuk satu string:
	//! "UID|FactionKey;UID|FactionKey;..." -- udah ke-sort by UID di server.
	//! Server DAN client sama-sama baca daftar lewat roster ini (bukan dari
	//! m_aCommander), jadi urutan index di dropdown GM (client) dan di
	//! ReadVariable/WriteVariable (server) dijamin identik. Client juga gak
	//! bergantung ke entity commander ke-stream atau nggak.
	//!
	//! Butuh RplComponent di entity manager biar ke-replicate. Tanpa itu (SP /
	//! editor) tetap jalan karena server = client.
	[RplProp()]
	protected string m_sCommanderRoster;
	
	protected const string ROSTER_ENTRY_SEP = ";";
	protected const string ROSTER_FIELD_SEP = "|";
	
	//! Cache hasil parse roster. Di-parse ulang lazy kalau string-nya berubah
	//! (m_sParsedRoster != m_sCommanderRoster) -- jadi gak perlu ngandelin
	//! onRplName callback, termasuk buat client yang join belakangan.
	protected ref array<string> m_aRosterUIDs = {};
	protected ref array<FactionKey> m_aRosterFactions = {};
	protected string m_sParsedRoster;
	
	//! Counter buat UID otomatis. Server-only, cuma naik.
	protected int m_iAutoUIDCounter = 0;
	// === END ADDED ===
	
	bool IsPreventLODUsage()
	{
		return m_bPreventUseLOD;
	}
	
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
	
	bool RegisterCommander(AICommander_BaseComponent cmd)
	{
		// === ADDED: Commander Roster -- UID harus unik SEBELUM masuk list,
		// karena dedicated assignment & IsCapturedBy matching-nya pake UID.
		// Commander yang di-spawn GM dari prefab yang sama bakal bawa UID sama. ===
		if (!cmd)
			return false;
		
		if (Replication.IsServer())
			EnsureUniqueCommanderUID(cmd);
		// === END ADDED ===
		
		if (!m_aCommander.Contains(cmd))
			m_aCommander.Insert(cmd);
		
		Print("REGISTERING : " + cmd.GetCommanderUID() + " FACTION : " + cmd.GetCommanderFactionKey());
		
		// === ADDED: Commander Roster ===
		if (Replication.IsServer())
			RebuildCommanderRoster();
		// === END ADDED ===
		
		return true;
	}
	
	// === ADDED: Commander Roster ===
	//! Dipanggil commander dari OnDelete-nya. Release group/vehicle milik
	//! commander ini BELUM di sini -- itu masuk di step activate/release.
	bool UnregisterCommander(AICommander_BaseComponent cmd)
	{
		if (!cmd)
			return false;
		
		if (m_aCommander.Contains(cmd))
			m_aCommander.RemoveItem(cmd);
		
		Print("UNREGISTERING : " + cmd.GetCommanderUID() + " FACTION : " + cmd.GetCommanderFactionKey());
		
		if (Replication.IsServer())
			RebuildCommanderRoster();
		
		return true;
	}
	
	//! Server-only. Kalau UID kosong, bentrok sama commander lain yang udah
	//! register, atau ngandung separator roster, ganti ke "<FactionKey>-<n>".
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
	
	//! Server-only. Susun ulang roster dari m_aCommander, sort by UID, lalu
	//! replicate.
	protected void RebuildCommanderRoster()
	{
		array<string> entries = {};
		foreach (AICommander_BaseComponent cmd : m_aCommander)
		{
			if (!cmd)
				continue;
			
			entries.Insert(cmd.GetCommanderUID() + ROSTER_FIELD_SEP + cmd.GetCommanderFactionKey());
		}
		
		// UID ada di depan tiap entry, jadi sort string = sort by UID.
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
	
	//! Server & client. UID commander milik faction fk, urut by UID. Ini yang
	//! jadi sumber isi dropdown GM (client) dan resolusi index (server).
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
	
	//! Server-only. Ganti UID commander dari GM. Tolak kalau kosong, bentrok,
	//! atau ngandung separator roster. Roster langsung di-rebuild biar dropdown
	//! assign ikut berubah.
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
	
	//! Server-only (m_aCommander cuma lengkap & otoritatif di server).
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
	// === END ADDED ===
	
	void InitializeCommanderManager()
	{
		// === ADDED: Manager Auto-Spawn -- sekarang dipanggil juga dari
		// objective & GetOrSpawnInstance (urutan EOnInit antar entity gak
		// dijamin), jadi harus idempotent: jangan isi faction dua kali. ===
		if (!m_aAvailableFactions.IsEmpty())
			return;
		
		if (!GetGame().GetFactionManager())
		{
			Print("[CMD_Manager] FactionManager belum ada -- daftar faction kosong", LogLevel.WARNING);
			return;
		}
		// === END ADDED ===
		
		array<Faction> AvailableFactions = {};
		FactionManager fm = GetGame().GetFactionManager();
		
		fm.GetFactionsList(AvailableFactions);
		foreach(Faction f : AvailableFactions)
		{
			m_aAvailableFactions.Insert(f.GetFactionKey());
		}
	}
	
	// === ADDED: Intel Fog System ===
	//! Cek apakah target objective ke-cover intel dari objective RECON manapun yang
	//! lagi aktif (ada grup RECON beneran di situ) dalam radius coverage-nya. Ini
	//! query CROSS-OBJECTIVE, makanya harus lewat manager (objective sendirian gak
	//! punya visibilitas ke objective lain).
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
	// === END ADDED ===
	
	// === REMOVED: Objective Connectivity ===
	// ComputeObjectiveConnectivity/ComputeObjectiveConnectivityCached dicabut --
	// relevance sekarang cuma pake proximity+importance, connectivity (jarak ke
	// objective lain yang in-play) gak dipake lagi. m_aActiveObjs di
	// CMD_ObjectiveContextCache juga udah gak diisi lagi (lihat BuildObjectiveContext
	// di bawah), tapi field-nya dibiarin ada di class declaration -- gak ganggu apapun.
	// === END REMOVED ===
	
	// === ADDED: Optimasi -- Precomputed Objective Context ===
	// IsObjectiveIntelCovered di atas full-scan m_aObjective + manggil
	// IsReconObjectiveActive() (isinya map.Find()) per objective yang di-cek.
	// Dipanggil sekali per objective yang lagi di-assign role (dari
	// AssignRolesToObjective), buat N objective total jadi O(N) state-lookup
	// TERULANG N kali = O(N^2) map lookup, bukan cuma perbandingan vector doang.
	//
	// CMD_ObjectiveContextCache (top-level class, lihat atas file) dibangun SEKALI
	// per Think() cycle (BuildObjectiveContext), nyimpen list objective RECON yang
	// aktif. Versi *Cached di bawah baca dari list itu (udah kefilter), bukan
	// m_aObjective mentah + state-lookup ulang. Fungsi ORIGINAL di atas TETEP ada,
	// gak diubah -- dipake sebagai fallback kalau caller belum di-update buat pake
	// cache (backward compatible, optional parameter di caller).
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
	// === END ADDED ===
	
	// === REMOVED: Dead code cleanup (bagian dari optimasi) ===
	// GetHighestPrioObjective() dan GetTopObjectives() (non-offensive) dihapus --
	// dikonfirmasi gak ada caller aktif di manapun (cuma muncul di komentar sisa
	// optimasi lama). GetHighestPrioObjective() sendiri sebenernya cacat desain dari
	// awal -- namanya "highest PRIORITY" tapi isinya cuma nyari objective TERDEKAT,
	// gak pernah manggil ComputePriorityScore() sama sekali. GetTopObjectives()
	// (versi non-offensive) udah lama digantiin caller-nya pake mgr.m_aObjective
	// langsung (lihat komentar OPTIMIZED di AICommanderBase.c). Kalau nanti butuh
	// versi "top objectives" yang gak exclude captured, tinggal reuse pola
	// GetTopObjectivesOffensive() minus filter IsCapturedBy-nya.
	// === END REMOVED ===
	
	void GetTopObjectivesOffensive(AICommander_BaseComponent forCommander, int count, out array<CMD_AICommanderObjectiveComponent> result)
	{
		result = {};
		if (!forCommander)
			return;
 
		FactionKey fk  = forCommander.GetCommanderFactionKey();
		float worldTime = GetGame().GetWorld().GetWorldTime() / 1000.0;
		
		// === REMOVED: Optimasi -- BuildObjectiveContext() dulu dibangun di sini buat
		// dipass ke ComputePriorityScore, tapi sekarang ComputePriorityScore gak pake
		// contextCache lagi (connectivity yang butuh itu udah dicabut dari relevance).
		// Manggil BuildObjectiveContext() di sini sekarang cuma buang-buang kerjaan
		// (O(N) tanpa konsumen), jadi dicabut.
		// === END REMOVED ===
 
		array<float> scores = {};
		array<CMD_AICommanderObjectiveComponent> sorted = {};
 
		foreach (CMD_AICommanderObjectiveComponent obj : m_aObjective)
		{
			if (!obj)
				continue;
			
			if (obj.IsCommanderBlackListed(forCommander.GetCommanderUID()))
				continue;
			
			if (obj.IsCapturedBy(fk, forCommander.GetCommanderUID()))
				continue;
 
			// === MODIFIED: kirim commander UID biar cache skor di objective bisa di-key
			// per pemanggil. Tanpa ini semua commander sefaction share satu entry cache
			// dan yang nanya belakangan dapet skor punya yang nanya duluan.
			float score = obj.ComputePriorityScore(fk, worldTime, forCommander.GetOwner().GetOrigin(), forCommander.GetCombatFocus(), forCommander.GetCommanderUID());
			// === END MODIFIED ===
 
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
		//Print("GRP FACTION KEY " + grpFk);
 
		array<AICommander_BaseComponent> availCommanders = {};
		foreach (AICommander_BaseComponent cmd : m_aCommander)
		{
			if (cmd && cmd.GetCommanderFactionKey() == grpFk)
				availCommanders.Insert(cmd);
		}
 
		if (availCommanders.IsEmpty())
		{
			//Print(string.Format("[CMD_Manager] WARNING: Tidak ada commander untuk faction '%1'. Group '%2' unassigned.",
				//grpFk, grp.GetOwner().GetName()), LogLevel.WARNING);
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
		
		//Print("CHOSEN " + chosen.GetCommanderUID() + " GRP COUNT " + leastGroups);
 
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
		FactionKey grpFk
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
		DCO_TransportMissionComponent comp = DCO_TransportMissionComponent.Cast(grp.FindComponent(DCO_TransportMissionComponent));
		comp.AssignCommanderOwner(chosen);
		//Print(string.Format("[VEH_Manager] VEH '%1' → Commander '%2'", grp.GetName(), chosen.GetCommanderUID()));
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
	
	// === ADDED: Manager Auto-Spawn ===
	//! Balikin manager yang ada; kalau belum ada dan kita di server, spawn
	//! prefab manager di posisi context. Client gak pernah spawn -- manager
	//! nyampe ke client lewat replikasi.
	//!
	//! Aman dari double-spawn: s_Instance diisi di constructor manager, yang
	//! jalan sinkron di dalam SpawnEntityPrefab. Caller kedua di frame yang sama
	//! udah lihat instance dari caller pertama.
	static AICommander_ManagerComponent GetOrSpawnInstance(ResourceName managerPrefab, IEntity context)
	{
		if (s_Instance)
			return s_Instance;
		
		if (!Replication.IsServer() || !context)
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
		
		// Faction harus siap sekarang juga -- caller (objective) langsung baca
		// m_aAvailableFactions, dan EOnInit manager hasil spawn belum tentu udah
		// jalan di titik ini.
		s_Instance.InitializeCommanderManager();
		
		Print(string.Format("[CMD_Manager] Manager di-spawn otomatis oleh %1", context.GetName()));
		return s_Instance;
	}
	// === END ADDED ===
	
	void AICommander_ManagerComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		if (!s_Instance)
			s_Instance = this;
	}
	
	// === ADDED: BUG FIX -- s_Instance (static) nyimpen reference ke instance ini
	// selamanya, gak pernah di-clear. Begitu entity yang punya component ini
	// di-delete (end-of-mission/world cleanup/hot-reload), engine gak bisa destroy
	// instance-nya karena s_Instance masih megang reference -> "Can't delete instance
	// with non-zero references". Destructor ini clear reference-nya begitu instance
	// ini beneran di-destroy, dan cuma clear kalau MEMANG instance ini yang lagi
	// dipegang s_Instance (defensive, jaga-jaga ada multiple instance somehow).
	void ~AICommander_ManagerComponent()
	{
		if (s_Instance == this)
			s_Instance = null;
	}
	// === END ADDED ===
	
	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);

		// === ADDED: manager butuh tick sendiri buat overlay-nya. ===
		SetEventMask(owner, EntityEvent.FRAME);
	}

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		UpdateManagerDebug(timeSlice);
	}

	//------------------------------------------------------------------------------------------------
	// === ADDED: Manager Debug ===
	//! Overlay milik manager: tally global yang gak dimiliki commander manapun --
	//! daftar faction dari FactionManager, jumlah commander per faction, dan
	//! pembagian kepemilikan objective.
	//!
	//! Digambar di posisi entity manager sendiri. Kalau entity-nya ada di tempat yang
	//! gak kelihatan di map, pindahin aja -- ini murni penanda debug.
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
			DCO_DebugDraw.COLOR_MANAGER, DCO_DebugDraw.Flags(), p, DCO_DebugDraw.MARKER_BIG));

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

		// Per faction: berapa commander dan berapa objective yang dia pegang. Ini
		// jawaban buat "siapa lagi menang" tanpa harus keliling ngecek objective satu
		// per satu.
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
	// === END ADDED ===

	
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		InitializeCommanderManager();
	}
}