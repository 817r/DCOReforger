[ComponentEditorProps(category: "GameScripted/Group")]
class DCO_GroupUtilityComponentClass : ScriptComponentClass
{

}

class DCO_GroupUtilityComponent : ScriptComponent
{
	// === ADDED: Group Debug ===
	// CATATAN ERGONOMI: checkbox ini per-prefab grup. Kalau mau nyalain buat
	// semua grup sekaligus, nyalainnya harus di prefab dasar yang diwarisi --
	// bukan satu-satu di tiap grup di world.
	[Attribute("0", UIWidgets.CheckBox, desc: "Gambar overlay grup ini (role, tugas, status order) sebagai teks 3D. Cuma kelihatan waktu Game Master kebuka.", category: "Group Debug")]
	protected bool m_bDebugMode;

	[Attribute("0.5", UIWidgets.EditBox, "Interval (detik) gambar ulang overlay grup.", category: "Group Debug")]
	protected float m_fDebugRefreshInterval;

	protected ref array<ref Shape> m_aDebugShapes = new array<ref Shape>();
	protected ref array<ref DebugTextWorldSpace> m_aDebugTexts = new array<ref DebugTextWorldSpace>();
	protected float m_fDebugTimer = 0.0;
	// === END ADDED ===

	[Attribute("0", UIWidgets.EditBox, "Radius pencarian kendaraan", category: "Commander")]
	protected bool m_bIsDedicatedTransport;
	
	[Attribute("1", UIWidgets.EditBox, "Can it be used By Commander?", category: "Commander")]
	protected bool m_bIsProcessedByCommander;
	
	[Attribute("1", UIWidgets.EditBox, "Can it Have By Commander?", category: "Commander")]
	protected bool m_bCanHaveCommander;
	
	[Attribute("1", UIWidgets.EditBox, "Can it call Artillery?", category: "Support")]
	protected bool m_bCanCallArtillery;
	
	[Attribute("1", UIWidgets.EditBox, "Can it call Reinforcement?", category: "Support")]
	protected bool m_bCanCallReinforcement;
	
	[Attribute("1", UIWidgets.EditBox, "Can it role be override by Commander?", category: "Group Role")]
	protected bool m_bCanCommanderOverrideRole;
	
	protected SCR_AIGroup m_Group;
	protected SCR_AIGroupUtilityComponent m_UtilityComp;
	ref SCR_AIGroupPerception perc;
	protected AIFormationComponent m_FormationComponent;
	protected AICommander_BaseComponent myCommander;
	protected CMD_ThreatResponseComponent threatComp;
	protected DCO_TransportTeamComponent DedicatedTransport;
	protected DCO_GroupContactReporterComponent contactReportComponent;
	
	// === ADDED: Commander Assignment (GM) -- combat mode external sebelum
	// commander ambil alih; dibalikin waktu release. ===
	protected EAIGroupCombatMode m_eCombatModeBeforeCommander = EAIGroupCombatMode.FIRE_AT_WILL;
	// === END ADDED ===
 
	protected DCOG_EGroupStatus m_eGroupStatus = DCOG_EGroupStatus.IDLE;
	
	[Attribute("0", UIWidgets.SearchComboBox, "", enums: ParamEnumArray.FromEnum(CMD_EGroupRole))]
	protected CMD_EGroupRole m_eGroupRoleExternal;
	
	[Attribute("", UIWidgets.Auto, "Blacklisted Commander to not process this Group", category: "Commander")]
	protected ref array<string> m_sBlacklistedCo;
	
	[Attribute("", UIWidgets.Auto, "Assign This Unit To Commander", category: "Commander")]
	protected string m_sDedicatedCo;
	
	[Attribute("", UIWidgets.Auto, "Usable Mortar For This Group In The Inital", category: "Artillery Group")]
	protected ref array<string> m_sUsableVehicle;
	
	protected CMD_EGroupRole    m_eGroupRole   = CMD_EGroupRole.NONE;
	protected FactionKey fk;
	
	protected CMD_AICommanderObjectiveComponent currentObjective = null;
	
	protected bool IsPlayerGroup = false;
	
	protected vector m_vOrderTarget    = vector.Zero;
	protected float  m_fOrderStartTime = 0.0;
	protected float  m_fOrderTimeout   = 0.0;
	protected bool   m_bOrderActive    = false;
	
	static float AVG_MOVE_SPEED_MPS = 2.5;
	static float ORDER_BASE_BUFFER  = 10.0;
	static float ARRIVAL_THRESHOLD  = 3.0;
	
	static float SQUAD_SPREAD_THRESHOLD = 30.0;
	
	protected vector m_vLastCheckPos = vector.Zero;
	protected float m_fLastMoveTime = 0;
	protected const float STUCK_DIST_THRESHOLD = 2.5;
	protected const float STUCK_TIME_THRESHOLD = 15.0;

	protected IEntity m_OwnedVehicle;
	
	IEntity GetOwnedVehicle()
	{
		return m_OwnedVehicle;
	}
	
	void SetOwnedVehicle(IEntity veh)
	{
		m_OwnedVehicle = veh;
	}
	
	bool HasOwnedVehicle()
	{
		return m_OwnedVehicle != null;
	}
	// === END ADDED ===
	
	// === ADDED: GM Commander Attributes -- dipanggil waktu commander di-rename
	// dari GM, biar UID dedicated gak nyangkut ke nama lama. ===
	void SetDedicatedCommanderUID(string uid)
	{
		m_sDedicatedCo = uid;
	}
	// === END ADDED ===
	
	// === ADDED: Manager Auto-Spawn ===
	//! Server. Dipanggil AICommander_BaseComponent.AssignGroup. Kalau EOnInit
	//! ke-skip (group di-spawn sebelum ada manager) atau delayedInit belum jalan
	//! (group baru di-spawn < 5 detik), jalanin sekarang. Gate "gak ada manager =
	//! diem total" di EOnInit tetap dipertahankan.
	void EnsureDormantInit()
	{
		if (!fk.IsEmpty())
			return;
		
		if (!m_bIsProcessedByCommander && !m_bCanHaveCommander)
			return;
		
		if (!contactReportComponent)
			contactReportComponent = DCO_GroupContactReporterComponent.Cast(GetOwner().FindComponent(DCO_GroupContactReporterComponent));
		
		// Batalin delayedInit yang masih nunggu biar gak jalan dua kali.
		GetGame().GetCallqueue().Remove(delayedInit);
		delayedInit(GetOwner());
	}
	// === END ADDED ===
	
	// === ADDED: Commander Assignment (GM) -- dipake GM attribute buat nentuin
	// group ini boleh muncul di dropdown Commander. Pakai kondisi yang sama
	// dengan gate EOnInit. Dua-duanya Attribute prefab, jadi nilainya identik
	// di server & client. ===
	bool IsCommanderEligible()
	{
		return m_bIsProcessedByCommander || m_bCanHaveCommander;
	}
	// === END ADDED ===
	
	bool CanCommanderOverrideRole()
	{
		return m_bCanCommanderOverrideRole;
	}
	
	bool CanCallReinforcement()
	{
		return m_bCanCallReinforcement;
	}
	
	bool CanCallArty()
	{
		return m_bCanCallArtillery;
	}
	
	void CompleteAllWaypoints()
	{
		m_Group.CompleteAllWaypoints();
	}
	
	bool IsCommanderBlacklisted(string cuid)
	{
		return m_sBlacklistedCo.Contains(cuid);
	}
	
	int GetGroupID()
	{
		return m_Group.GetID();
	}
	
	AICommander_BaseComponent GetMyCommander()
	{
		return myCommander;
	}
	
	string DedicatedCommander()
	{
		return m_sDedicatedCo;
	}
	
	bool IsOrderActive()
	{
	    return m_bOrderActive;
	}
	
	bool IsDedicatedTransport()
	{
		return m_bIsDedicatedTransport;
	}
	
	CMD_ThreatResponseComponent GetThreatResponseComponent()
	{
		//Print(threatComp.Type().ToString() + " < THREAT RESPONSE");
		return threatComp;
	}
	
	SCR_AIGroupUtilityComponent GetGroupUtilityComponent()
	{
		return m_UtilityComp;
	}
 
	int GetUnitCount()
	{
		if (!m_Group)
			return 0;
		return m_Group.GetAgentsCount();
	}
 
	void MoveTo(SCR_AIWaypoint wp, float worldTime)
	{
		if (!m_Group || !wp)
			return;
 
		m_Group.AddWaypoint(wp);
		SetGroupStatus(DCOG_EGroupStatus.EXECUTING_COMMAND);
		BeginOrderTracking(wp.GetOrigin(), worldTime);
	}
	
	void CheckGroupIsHaveOrder()
	{
		if (GetGroupStatus() == DCOG_EGroupStatus.EXECUTING_COMMAND)
		{
			if (IsGroupHaveWaypoint())
				return;
		}
		
		SetGroupStatus(DCOG_EGroupStatus.IDLE);
	}
	
	void ShootMortar(SCR_AIWaypoint wp, float worldTime)
	{
		if (m_eGroupRole != CMD_EGroupRole.ARTILLERY)
			return;
		
		m_Group.AddWaypoint(wp);
		SetGroupStatus(DCOG_EGroupStatus.EXECUTING_COMMAND);
	}
	
	bool IsGroupHaveWaypoint()
	{
		array<AIWaypoint> wp ={};
		m_Group.GetWaypoints(wp);
		
		if (wp.Count() > 0)
			return true;
		else
			return false;
	}
 
	void ForceRetreat(SCR_AIWaypoint rallyPos, float worldTime)
	{
		if (!m_Group)
			return;
 
		SetGroupRole(CMD_EGroupRole.RETREAT);
		SetGroupStatus(DCOG_EGroupStatus.EXECUTING_COMMAND);
		BeginOrderTracking(rallyPos.GetOrigin(), worldTime);
 
		m_Group.CompleteAllWaypoints();
 
		//Print(string.Format("[DCO_Group] %1 FORCE RETREAT → %2",
			//GetOwner().GetName(), rallyPos.ToString()));
	}
	
	void SetGroupObjective(CMD_AICommanderObjectiveComponent obj)
	{
		currentObjective = obj;
		//Print(string.Format("[DCO_Group] %1 SET OBJECTIVE TO → %2",
			//GetOwner().GetName(), currentObjective.GetOwner().GetName()));
	}
	
	CMD_AICommanderObjectiveComponent GetGroupObjective()
	{
		return currentObjective;
	}

	void SetGroupRole(CMD_EGroupRole role)   
	{ 
		m_eGroupRole = role; 
		switch(m_eGroupRole)
		{
			case CMD_EGroupRole.RECON:
			{
				m_FormationComponent.SetFormation("Column");
				m_UtilityComp.SetCombatMode(EAIGroupCombatMode.RETURN_FIRE);
				break;
			}
			case CMD_EGroupRole.ASSAULT:
			{
				m_FormationComponent.SetFormation("Wedge");
				m_UtilityComp.SetCombatMode(EAIGroupCombatMode.RETURN_FIRE);
				break;
			}
			case CMD_EGroupRole.FLANK:
			{
				m_FormationComponent.SetFormation("Line");
				m_UtilityComp.SetCombatMode(EAIGroupCombatMode.RETURN_FIRE);
				break;
			}
			case CMD_EGroupRole.RESERVE:
			{
				m_FormationComponent.SetFormation("StaggeredColumn");
				m_UtilityComp.SetCombatMode(EAIGroupCombatMode.RETURN_FIRE);
				break;
			}
			case CMD_EGroupRole.RETREAT:
			{
				m_FormationComponent.SetFormation("StaggeredColumn");
				m_UtilityComp.SetCombatMode(EAIGroupCombatMode.HOLD_FIRE);
				break;
			}
			case CMD_EGroupRole.TRANSPORT:
			{
				m_FormationComponent.SetFormation("Wedge");
				m_UtilityComp.SetCombatMode(EAIGroupCombatMode.RETURN_FIRE);
				break;
			}
			case CMD_EGroupRole.REINFORNCE:
			{
				m_FormationComponent.SetFormation("Wedge");
				m_UtilityComp.SetCombatMode(EAIGroupCombatMode.RETURN_FIRE);
				break;
			}
			case CMD_EGroupRole.ARTILLERY:
			{
				m_FormationComponent.SetFormation("Wedge");
				m_UtilityComp.SetCombatMode(EAIGroupCombatMode.HOLD_FIRE);
				break;
			}
			case CMD_EGroupRole.SUPPRESS:
			{
				m_FormationComponent.SetFormation("Line");
				m_UtilityComp.SetCombatMode(EAIGroupCombatMode.RETURN_FIRE);
				break;
			}
			case CMD_EGroupRole.NONE:
			{
				m_FormationComponent.SetFormation("Wedge");
				m_UtilityComp.SetCombatMode(EAIGroupCombatMode.RETURN_FIRE);
				break;
			}
		}
	}
	
	// === ADDED: Squad Cohesion Check ===
	// Cek apakah ada member grup yang masih jauh dari leader/group-origin. Dipake buat
	// nunda "order complete" sampe squad beneran ngumpul, bukan cuma leader doang.
	protected bool IsSquadSpreadTooFar(vector leaderPos)
	{
		if (!m_Group)
			return false;
		
		array<AIAgent> agents = {};
		m_Group.GetAgents(agents);
		
		foreach (AIAgent agent : agents)
		{
			IEntity controlled = agent.GetControlledEntity();
			if (!controlled)
				continue;
			
			if (vector.Distance(controlled.GetOrigin(), leaderPos) > SQUAD_SPREAD_THRESHOLD)
				return true;
		}
		
		return false;
	}
	// === END ADDED ===
	
	protected void BeginOrderTracking(vector targetPos, float worldTime)
	{
		m_vOrderTarget    = targetPos;
		m_fOrderStartTime = worldTime;
		m_bOrderActive    = true;
 
		float initialDist = vector.Distance(GetOwner().GetOrigin(), targetPos);
		m_fOrderTimeout   = (initialDist / AVG_MOVE_SPEED_MPS) + ORDER_BASE_BUFFER;
 
		//Print(string.Format("[DCO_Group] %1 order tracking started | dist: %2m | timeout: %3s",
			//GetOwner().GetName(), initialDist.ToString(), m_fOrderTimeout.ToString()));
	}
	
	bool CheckOrderComplete(float worldTime)
	{
		if (!m_bOrderActive)
			return true;
 
		if (!m_Group)
		{
			ResetOrderTracking();
			return true;
		}
		
		vector currentPos = GetOwner().GetOrigin();
		float distToTarget = vector.Distance(currentPos, m_vOrderTarget);
 
		if (distToTarget <= ARRIVAL_THRESHOLD)
		{
			// === ADDED: Squad Cohesion Check ===
			// Sebelumnya, order langsung dianggap selesai begitu GetOwner().GetOrigin()
			// (posisi leader/group-origin) nyampe target -- walau member lain masih jauh
			// di belakang. Commander bisa langsung kasih order baru & leader gerak lagi
			// sebelum squad sempet regroup, bikin gap makin lebar tiap cycle (compounding).
			// Sekarang: kalau ada member yang masih jauh dari leader, order BELUM dianggap
			// selesai -- tunggu squad ngumpul dulu. Jalur stuck-detection & timeout di bawah
			// TETAP jalan normal (gak kena efek ini), jadi kalau ada straggler yang beneran
			// stuck permanen, order tetap bisa selesai lewat jalur itu -- gak bakal deadlock.
			if (IsSquadSpreadTooFar(currentPos))
				return false;
			// === END ADDED ===
			
			SetGroupStatus(DCOG_EGroupStatus.IDLE);
			ResetOrderTracking();
			return true;
		}
 
		if (m_vLastCheckPos == vector.Zero) 
		{
			m_vLastCheckPos = currentPos;
			m_fLastMoveTime = worldTime;
		}
		else
		{
			float distMoved = vector.Distance(currentPos, m_vLastCheckPos);
			
			if (distMoved > STUCK_DIST_THRESHOLD)
			{
				m_vLastCheckPos = currentPos;
				m_fLastMoveTime = worldTime;
			}
			else
			{
				float stuckElapsed = worldTime - m_fLastMoveTime;
				if (stuckElapsed >= STUCK_TIME_THRESHOLD)
				{
					/*Print(string.Format("[DCO_Group] %1 STUCK/IDLE di tempat selama %2s, membatalkan order!",
						GetOwner().GetName(), stuckElapsed.ToString()));*/
					
					SetGroupStatus(DCOG_EGroupStatus.IDLE);
					ResetOrderTracking();
					return true;
				}
			}
		}
 
		float elapsed = worldTime - m_fOrderStartTime;
		if (elapsed >= m_fOrderTimeout)
		{
			SetGroupStatus(DCOG_EGroupStatus.IDLE);
			ResetOrderTracking();
			return true;
		}
 
		return false;
	}
	
	protected void ResetOrderTracking()
	{
		m_vOrderTarget    = vector.Zero;
		m_fOrderStartTime = 0.0;
		m_fOrderTimeout   = 0.0;
		m_bOrderActive    = false;
		m_vLastCheckPos = vector.Zero;
		m_fLastMoveTime = 0;
	}
	
	void SetGroupStatus(DCOG_EGroupStatus st)
	{
		if (m_eGroupStatus == st)
			return;		
		m_eGroupStatus = st;
	}
 
	void ClearAssignment()
	{
		m_eGroupRole = CMD_EGroupRole.NONE;
	}
 
	DCOG_EGroupStatus GetGroupStatus() { return m_eGroupStatus; }
	CMD_EGroupRole GetGroupRole()      { return m_eGroupRole; }
	
	// === ADDED: Fireteam Bounding ===
	//! Bounding cuma aktif pas role ASSAULT -- itu taktik nyerang, gak masuk akal
	//! buat DEFEND/RECON/RESERVE/dll (yang punya pola gerak sendiri-sendiri).
	bool ShouldUseBounding()
	{
		return m_eGroupRole == CMD_EGroupRole.ASSAULT;
	}
 
	FactionKey GetFactionKey()
	{
		//Print(m_Group.GetName() + " Group Faction : " + m_Group.GetFaction());
		return fk;
	}
	
	void OnGroupRemoved()
	{
		// === MODIFIED (Manager Auto-Spawn): null-check -- group bisa mati waktu
		// belum ada manager di world. ===
		//AICommander_ManagerComponent.GetInstance().UnregisterGroup(this);
		if (AICommander_ManagerComponent.GetInstance())
			AICommander_ManagerComponent.GetInstance().UnregisterGroup(this);
		// === END MODIFIED ===
		
		if (m_OwnedVehicle)
		{
			DCO_TransportMissionComponent mission = DCO_TransportMissionComponent.Cast(m_OwnedVehicle.FindComponent(DCO_TransportMissionComponent));
			if (mission)
				mission.ReleaseOwnership();
			m_OwnedVehicle = null;
		}
	}
	
	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		m_Group = SCR_AIGroup.Cast(owner);
		m_UtilityComp = SCR_AIGroupUtilityComponent.Cast(owner.FindComponent(SCR_AIGroupUtilityComponent));
		m_FormationComponent = AIFormationComponent.Cast(owner.FindComponent(AIFormationComponent));
		//
		SetEventMask(owner, EntityEvent.INIT);		
	}

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		UpdateGroupDebug(timeSlice);
	}

	//------------------------------------------------------------------------------------------------
	// === ADDED: Group Debug ===
	//! Overlay milik grup. TANPA bola -- cuma teks yang nempel di posisi grup, plus
	//! panah ke objective yang dia pegang. Semuanya dibaca live dari komponen ini
	//! sendiri, jadi labelnya ngikutin grupnya waktu jalan.
	protected void UpdateGroupDebug(float timeSlice)
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

		vector g     = GetOwner().GetOrigin();
		int    role  = m_eGroupRole;
		int    color = DCO_DebugDraw.RoleColor(role);

		CMD_AICommanderObjectiveComponent obj = GetGroupObjective();

		string taskLine;
		if (obj && obj.GetOwner())
			taskLine = string.Format("task: %1  (%2 away)",
				obj.GetOwner().GetName(),
				DCO_DebugDraw.M(vector.Distance(g, obj.GetOwner().GetOrigin())));
		else
			taskLine = "task: none";

		string cmdLine;
		if (myCommander)
			cmdLine = "commander: " + myCommander.GetCommanderUID();
		else
			cmdLine = "commander: none";

		// strengthPct persis rumus yang dipakai FindBestIdleGroupForRole buat nyekor
		// kandidat -- ini yang nentuin grup mana kepilih duluan.
		int   units       = GetUnitCount();
		float strengthPct = Math.Clamp(units / 12.0 * 100.0, 0.0, 100.0);

		string label = string.Format(
			"%1  x%2  (id %3)\n%4\n%5\n%6",
			DCO_DebugDraw.RoleName(role),
			units,
			GetGroupID(),
			DCO_DebugDraw.GroupStatusName(GetGroupStatus()),
			taskLine,
			cmdLine);

		string flagsLine = string.Format(
			"strength %1%% -- selection score input\nwp %2 | order %3 | override %4 | orderable %5\nvehicle %6 | transport %7 | player %8",
			Math.Round(strengthPct),
			DCO_DebugDraw.YesNo(IsGroupHaveWaypoint()),
			DCO_DebugDraw.YesNo(IsOrderActive()),
			DCO_DebugDraw.YesNo(CanCommanderOverrideRole()),
			DCO_DebugDraw.YesNo(CanItHaveOrder()),
			DCO_DebugDraw.YesNo(HasOwnedVehicle()),
			DCO_DebugDraw.YesNo(IsDedicatedTransport()),
			DCO_DebugDraw.YesNo(IsPlayerGroup()));

		m_aDebugTexts.Insert(DCO_DebugDraw.SpawnText(Vector(g[0], g[1] + 12.0, g[2]), label,     17.0, color));
		m_aDebugTexts.Insert(DCO_DebugDraw.SpawnText(Vector(g[0], g[1] +  4.0, g[2]), flagsLine, 15.0, color));

		if (obj && obj.GetOwner())
		{
			vector t = obj.GetOwner().GetOrigin();
			m_aDebugShapes.Insert(Shape.CreateArrow(
				Vector(g[0], g[1] + 1.5, g[2]),
				Vector(t[0], t[1] + 1.5, t[2]),
				1.2, color, DCO_DebugDraw.Flags()));
		}
	}
	// === END ADDED ===

	
	void RegisterCommanderToGroup(AICommander_BaseComponent cmd)
	{
		myCommander = cmd;
		Print(myCommander.GetCommanderUID() + " < MY COMMANDER | MY GROUP > " + typename.EnumToString(CMD_EGroupRole, m_eGroupRole));
		threatComp = myCommander.GetThreatResponseComponent();
	}
	
	void SetDedicatedTransport(bool tf)
	{
		DedicatedTransport = DCO_TransportTeamComponent.Cast(GetOwner().FindComponent(DCO_TransportTeamComponent));
		if (tf)
			DedicatedTransport.Activate(GetOwner());
		else
			DedicatedTransport.Deactivate(GetOwner());
	}
	
	bool IsPlayerGroup()
	{
		SCR_AIGroup grp = SCR_AIGroup.Cast(GetOwner());
		//8Print(grp.GetTotalAgentCount().ToString() + " < AGENT COUNT | PLAYER COUNT > " + grp.GetTotalPlayerCount().ToString());
		
		if (grp.GetTotalPlayerCount() != 0)
		{
			IsPlayerGroup = true;
		} else
		{
			IsPlayerGroup = false;
		}
		
		return IsPlayerGroup;
	}
	
	void MoveToRoute(notnull array<SCR_AIWaypoint> wps, float worldTime)
	{
		if (!m_Group || wps.IsEmpty())
			return;

		float routeLength = 0.0;
		vector prev  = GetOwner().GetOrigin();
		vector final = prev;
		int added = 0;

		foreach (SCR_AIWaypoint wp : wps)
		{
			if (!wp)
				continue;

			m_Group.AddWaypoint(wp);

			vector wpPos = wp.GetOrigin();
			routeLength += vector.Distance(prev, wpPos);
			prev  = wpPos;
			final = wpPos;
			added++;
		}

		if (added == 0)
			return;

		SetGroupStatus(DCOG_EGroupStatus.EXECUTING_COMMAND);

		m_vOrderTarget    = final;
		m_fOrderStartTime = worldTime;
		m_bOrderActive    = true;
		m_fOrderTimeout   = (routeLength / AVG_MOVE_SPEED_MPS) + ORDER_BASE_BUFFER;
		m_vLastCheckPos   = vector.Zero;
		m_fLastMoveTime   = 0.0;
	}

	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		if (!AICommander_ManagerComponent.GetInstance())
			return;
		
		if (!m_bIsProcessedByCommander && !m_bCanHaveCommander)
			return;
		
		SCR_AIGroup grp = SCR_AIGroup.Cast(owner);
		
		// === MODIFIED (Commander Assignment GM): SetGroupRole dipindah ke
		// ActivateForCommander. SetGroupRole selalu nyetel combat mode
		// RETURN_FIRE, yang bikin group dormant jadi pasif di evaluasi vanilla. ===
		//if (!m_eGroupRoleExternal == CMD_EGroupRole.NONE)
		//	SetGroupRole(m_eGroupRoleExternal);
		//else
		//	SetGroupRole(CMD_EGroupRole.NONE);
		// === END MODIFIED ===
		
		contactReportComponent = DCO_GroupContactReporterComponent.Cast(owner.FindComponent(DCO_GroupContactReporterComponent));
		GetGame().GetCallqueue().CallLater(delayedInit, 5000, false, owner);
		
		//SetDedicatedTransport(m_bIsDedicatedTransport)
	}
	
	bool CanItHaveOrder()
	{
		return m_bIsProcessedByCommander;
	}
	
	protected void delayedInit(IEntity owner)
	{
		SCR_AIGroup grp = SCR_AIGroup.Cast(owner);
		Faction fc = grp.GetFaction();
		if (fc)
			fk = grp.GetFaction().GetFactionKey();
		// === MODIFIED (Commander Assignment GM): gak auto-register lagi. Group
		// dormant sampai GM assign ke commander (AICommander_BaseComponent.
		// AssignGroup -> ActivateForCommander). delayedInit sekarang cuma resolve
		// faction + setup vehicle vanilla. ===
		//if(!AICommander_ManagerComponent.GetInstance().RegisterGroup(this))
		//	return;
		//
		//if (myCommander)
		//{
		//	contactReportComponent.InitializeContactReport();
		//}
		// === END MODIFIED ===
		
		foreach(string s : m_sUsableVehicle)
		{
			IEntity e = GetGame().GetWorld().FindEntityByName(s);
			
			if (e)
			{
				SCR_AIVehicleUsageComponent aiveh = SCR_AIVehicleUsageComponent.Cast(e.FindComponent(SCR_AIVehicleUsageComponent));
				if (aiveh)
				{
					m_UtilityComp.AddUsableVehicle(aiveh);
				}
			}
		}
		
		// === MODIFIED (Commander Assignment GM): dipindah ke ActivateForCommander. ===
		//if (m_eGroupRole == CMD_EGroupRole.ARTILLERY)
		//	GetGame().GetCallqueue().CallLater(CheckGroupIsHaveOrder, 10000, true);
		//
		//SetEventMask(owner, EntityEvent.FRAME);
		// === END MODIFIED ===
	}
	
	// === ADDED: Commander Assignment (GM) ===
	//! Dipanggil AICommander_BaseComponent.AssignGroup SETELAH group masuk
	//! registry commander. Isinya yang dulu jalan otomatis di EOnInit/delayedInit.
	void ActivateForCommander(AICommander_BaseComponent cmd)
	{
		if (!cmd)
			return;
		
		if (m_UtilityComp)
			m_eCombatModeBeforeCommander = m_UtilityComp.DCO_GetCombatModeExternal();
		
		m_sDedicatedCo = cmd.GetCommanderUID();
		RegisterCommanderToGroup(cmd);
		
		if (!m_eGroupRoleExternal == CMD_EGroupRole.NONE)
			SetGroupRole(m_eGroupRoleExternal);
		else
			SetGroupRole(CMD_EGroupRole.NONE);
		
		if (contactReportComponent)
			contactReportComponent.InitializeContactReport();
		
		if (m_eGroupRole == CMD_EGroupRole.ARTILLERY)
			GetGame().GetCallqueue().CallLater(CheckGroupIsHaveOrder, 10000, true);
		
		SetEventMask(GetOwner(), EntityEvent.FRAME);
	}
	
	//! Dipanggil AICommander_BaseComponent.ReleaseGroup SETELAH state di sisi
	//! commander/objective/transport dibersihin. Group balik dormant: waypoint
	//! lama dibuang, role/objective/order direset, combat mode dibalikin.
	void ReleaseFromCommander()
	{
		GetGame().GetCallqueue().Remove(CheckGroupIsHaveOrder);
		
		if (m_Group)
			m_Group.CompleteAllWaypoints();
		
		if (m_OwnedVehicle)
		{
			DCO_TransportMissionComponent mission = DCO_TransportMissionComponent.Cast(m_OwnedVehicle.FindComponent(DCO_TransportMissionComponent));
			if (mission && mission.IsOwnedBy(this))
				mission.ReleaseOwnership();
			m_OwnedVehicle = null;
		}
		
		currentObjective = null;
		ClearAssignment();
		SetGroupStatus(DCOG_EGroupStatus.IDLE);
		ResetOrderTracking();
		
		myCommander = null;
		threatComp = null;
		m_sDedicatedCo = string.Empty;
		
		if (contactReportComponent)
			contactReportComponent.DeactivateContactReport();
		
		if (m_UtilityComp)
			m_UtilityComp.SetCombatMode(m_eCombatModeBeforeCommander);
		
		ClearEventMask(GetOwner(), EntityEvent.FRAME);
		m_aDebugShapes.Clear();
		m_aDebugTexts.Clear();
	}
	// === END ADDED ===
}