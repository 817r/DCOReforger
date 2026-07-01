[ComponentEditorProps(category: "GameScripted/Group")]
class DCO_GroupUtilityComponentClass : ScriptComponentClass
{

}

class DCO_GroupUtilityComponent : ScriptComponent
{
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
 
	protected DCOG_EGroupStatus m_eGroupStatus = DCOG_EGroupStatus.IDLE;
	
	[Attribute("0", UIWidgets.SearchComboBox, "", enums: ParamEnumArray.FromEnum(CMD_EGroupRole))]
	protected CMD_EGroupRole m_eUnitCapabilities;
	
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
	
	// === ADDED: Squad Cohesion Check ===
	static float SQUAD_SPREAD_THRESHOLD = 30.0; // Meter -- member terjauh dari leader di atas ini dianggap "belum ngumpul"
	// === END ADDED ===
	
	protected vector m_vLastCheckPos = vector.Zero;
	protected float m_fLastMoveTime = 0;
	protected const float STUCK_DIST_THRESHOLD = 2.5;
	protected const float STUCK_TIME_THRESHOLD = 15.0;
	
	// === ADDED: Vehicle Ownership ===
	// Vehicle biasa (bukan dedicated transport) yang udah "dimiliki" grup ini secara
	// permanen. Sekali grup dapet vehicle, dia bakal terus pake vehicle yang sama tiap
	// kali jalan lagi -- gak search vehicle baru dari nol tiap kali mau transport.
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
		Print(threatComp.Type().ToString() + " < THREAT RESPONSE");
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
		/* FORMATION
			Wedge,
			Line,
			Column,
			StaggeredColumn
		*/
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
 
	FactionKey GetFactionKey()
	{
		//Print(m_Group.GetName() + " Group Faction : " + m_Group.GetFaction());
		return fk;
	}
	
	void OnGroupRemoved()
	{
		AICommander_ManagerComponent.GetInstance().UnregisterGroup(this);
		
		// === ADDED: Vehicle Ownership cleanup ===
		// Grup dihapus/disband -- lepas klaim vehicle-nya biar bisa dipake grup lain,
		// jangan sampai vehicle nyangkut "dimiliki" grup yang udah gak eksis.
		if (m_OwnedVehicle)
		{
			DCO_TransportMissionComponent mission = DCO_TransportMissionComponent.Cast(m_OwnedVehicle.FindComponent(DCO_TransportMissionComponent));
			if (mission)
				mission.ReleaseOwnership();
			m_OwnedVehicle = null;
		}
		// === END ADDED ===
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

	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		if (!AICommander_ManagerComponent.GetInstance())
			return;
		
		if (!m_bIsProcessedByCommander && !m_bCanHaveCommander)
			return;
		
		SCR_AIGroup grp = SCR_AIGroup.Cast(owner);
		
		if (!m_eUnitCapabilities == CMD_EGroupRole.NONE)
			SetGroupRole(m_eUnitCapabilities);
		else
			SetGroupRole(CMD_EGroupRole.NONE);
		
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
		AICommander_ManagerComponent.GetInstance().RegisterGroup(this);
		
		if (myCommander)
		{
			contactReportComponent.InitializeContactReport();
		}
		
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
		
		if (m_eGroupRole == CMD_EGroupRole.ARTILLERY)
			GetGame().GetCallqueue().CallLater(CheckGroupIsHaveOrder, 10000, true);
	}
}