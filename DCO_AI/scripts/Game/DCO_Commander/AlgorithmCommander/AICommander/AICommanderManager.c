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
	[Attribute("0", UIWidgets.EditBox, "Is Use LOD Or No", category: "Simulation")]
	protected bool m_bPreventUseLOD;
	
	ref array<FactionKey> m_aAvailableFactions = {};
	
	ref array<AICommander_BaseComponent> m_aCommander = {};
	
	ref array<CMD_AICommanderObjectiveComponent> m_aObjective = {};
	
	protected static AICommander_ManagerComponent s_Instance;
	
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
		if (!m_aCommander.Contains(cmd))
			m_aCommander.Insert(cmd);
		
		Print("REGISTERING : " + cmd.GetCommanderUID() + " FACTION : " + cmd.GetCommanderFactionKey());
		return true;
	}
	
	void InitializeCommanderManager()
	{
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
 
			float score = obj.ComputePriorityScore(fk, worldTime, forCommander.GetOwner().GetOrigin(), forCommander.GetCombatFocus());
 
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
	}
	
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		InitializeCommanderManager();
	}
}