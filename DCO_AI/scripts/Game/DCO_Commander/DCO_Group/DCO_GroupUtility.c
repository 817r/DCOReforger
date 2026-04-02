[ComponentEditorProps(category: "GameScripted/Group")]
class DCO_GroupUtilityComponentClass : ScriptComponentClass
{

}

class DCO_GroupUtilityComponent : ScriptComponent
{
	[Attribute("0", UIWidgets.EditBox, "Radius pencarian kendaraan", category: "Commander")]
	protected bool m_bIsDedicatedTransport;
	
	protected SCR_AIGroup m_Group;
	protected SCR_AIGroupUtilityComponent m_UtilityComp;
	ref SCR_AIGroupPerception perc;
	protected AIFormationComponent m_FormationComponent;
	protected AICommander_BaseComponent myCommander;
	protected CMD_ThreatResponseComponent threatComp;
	protected DCO_TransportTeamComponent DedicatedTransport;
 
	protected DCOG_EGroupStatus m_eGroupStatus = DCOG_EGroupStatus.IDLE;
	
	[Attribute("0", UIWidgets.SearchComboBox, "", enums: ParamEnumArray.FromEnum(CMD_EGroupRole))]
	protected CMD_EGroupRole m_eUnitCapabilities;
	
	protected CMD_EGroupRole    m_eGroupRole   = CMD_EGroupRole.NONE;
	
	protected CMD_AICommanderObjectiveComponent currentObjective = null;
	
	protected bool IsPlayerGroup = false;
	
	protected vector m_vOrderTarget    = vector.Zero;
	protected float  m_fOrderStartTime = 0.0;
	protected float  m_fOrderTimeout   = 0.0;
	protected bool   m_bOrderActive    = false;
	
	static float AVG_MOVE_SPEED_MPS = 2.5;
	static float ORDER_BASE_BUFFER  = 20.0;
	static float ARRIVAL_THRESHOLD  = 5.0;
	
	AICommander_BaseComponent GetMyCommander()
	{
		return myCommander;
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
 
		//Print(string.Format("[DCO_Group] %1 → MoveTo (role: %2)",
			//GetOwner().GetName(), typename.EnumToString(CMD_EGroupRole, m_eGroupRole)));
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
			case CMD_EGroupRole.NONE:
			{
				m_FormationComponent.SetFormation("Wedge");
				m_UtilityComp.SetCombatMode(EAIGroupCombatMode.RETURN_FIRE);
				break;
			}
		}
	}
	
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
 
		float distToTarget = vector.Distance(GetOwner().GetOrigin(), m_vOrderTarget);
 
		if (distToTarget <= ARRIVAL_THRESHOLD)
		{
			//Print(string.Format("[DCO_Group] %1 arrived (dist: %2m)",
				//GetOwner().GetName(), distToTarget.ToString()));
 
			SetGroupStatus(DCOG_EGroupStatus.IDLE);
			ResetOrderTracking();
			return true;
		}
 
		float elapsed = worldTime - m_fOrderStartTime;
 
		if (elapsed >= m_fOrderTimeout)
		{
			/*Print(string.Format("[DCO_Group] %1 TIMEOUT after %2s (remaining dist: %3m | role: %4)",
				GetOwner().GetName(),
				elapsed.ToString(),
				distToTarget.ToString(),
				typename.EnumToString(CMD_EGroupRole, m_eGroupRole)));*/
 
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
		return m_Group.GetFaction().GetFactionKey();
	}
	
	void OnGroupRemoved()
	{
		AICommander_ManagerComponent.GetInstance().UnregisterGroup(this);
	}
	
	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		m_Group = SCR_AIGroup.Cast(owner);
		m_UtilityComp = SCR_AIGroupUtilityComponent.Cast(owner.FindComponent(SCR_AIGroupUtilityComponent));
		m_FormationComponent = AIFormationComponent.Cast(owner.FindComponent(AIFormationComponent));
		SetEventMask(owner, EntityEvent.INIT);
	}
	
	void RegisterCommanderToGroup(AICommander_BaseComponent cmd)
	{
		myCommander = cmd;
		//Print(myCommander.GetOwner().GetName() + " < MY COMMANDER | MY GROUP > " + typename.EnumToString(CMD_EGroupRole, m_eGroupRole));
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
		
		SCR_AIGroup grp = SCR_AIGroup.Cast(owner);
		AICommander_ManagerComponent.GetInstance().RegisterGroup(this);
		if (!m_eUnitCapabilities == CMD_EGroupRole.NONE)
			SetGroupRole(m_eUnitCapabilities);
		else
			SetGroupRole(CMD_EGroupRole.NONE);
		

			
		
		//SetDedicatedTransport(m_bIsDedicatedTransport)
	}
}

