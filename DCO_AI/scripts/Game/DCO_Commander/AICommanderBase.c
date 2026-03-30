[ComponentEditorProps(category: "GameScripted/Commander")]
class AICommander_BaseComponentClass : ScriptComponentClass
{
	[Attribute("{35BD6541CBB8AC08}Prefabs/AI/Waypoints/AIWaypoint_Cycle.et", UIWidgets.ResourceNamePicker, desc: "Cycle waypoint to be used for waypoints in hierarchy.", "et", category: "Commander Waypoint Setting")]
	protected ResourceName m_sCycleWaypointPrefab;

	[Attribute("{FFF9518F73279473}PrefabsEditable/Auto/AI/Waypoints/E_AIWaypoint_Move.et", UIWidgets.ResourceNamePicker, desc: "Waypoint to be used Move.", "et", category: "Commander Waypoint Setting")]
	protected ResourceName m_sDefaultMoveWaypointPrefab;
	
	[Attribute("{D9C14ECEC9772CC6}PrefabsEditable/Auto/AI/Waypoints/E_AIWaypoint_Defend.et", UIWidgets.ResourceNamePicker, desc: "Waypoint to be used Defend.", "et", category: "Commander Waypoint Setting")]
	protected ResourceName m_sDefaultDefendWaypointPrefab;

	ResourceName GetCycleWaypointPrefab()
	{
		return m_sCycleWaypointPrefab;
	}

	ResourceName GetDefaultMoveWaypointPrefab()
	{
		return m_sDefaultMoveWaypointPrefab;
	}
	
	ResourceName GetDefaultDefendWaypointPrefab()
	{
		return m_sDefaultDefendWaypointPrefab;
	}
}

class AICommander_BaseComponent : ScriptComponent
{
	[Attribute("", UIWidgets.Font, desc: "UID of the Commander.", category: "Commander General Setting")]
	protected string m_sCommanderUID;
	
	[Attribute("", UIWidgets.Auto, desc: "Faction Key of the Commander.", category: "Commander General Setting")]
	protected FactionKey m_sFactionKey;	
	
	[Attribute("3.0", UIWidgets.Auto, "Number of the Objective Can be processed at the same time", category: "Commander Objective Setting")]
	protected int m_fObjectiveAtTheSameTime;
	
	protected ref array<CMD_AICommanderObjectiveComponent> m_aObjective = {};
	
	protected CMD_ECommanderState m_eCommanderState = CMD_ECommanderState.IDLE;
	
	protected ref array<DCO_GroupUtilityComponent> m_aOwnedGroup = {};
	
	bool send = true;
	
	void SendGroupToPos(DCO_GroupUtilityComponent grp, CMD_AICommanderObjectiveComponent obj)
	{
		AICommander_BaseComponentClass componentData = AICommander_BaseComponentClass.Cast(GetComponentData(GetOwner()));
		if (!componentData)
			return;
		
		Resource res = Resource.Load(componentData.GetDefaultMoveWaypointPrefab());
		
		EntitySpawnParams params = EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		params.Transform[3] = obj.GetOwner().GetOrigin();	
		
		IEntity a = GetGame().SpawnEntityPrefab(res, null, params);
		SCR_AIWaypoint wp = SCR_AIWaypoint.Cast(a);
		
		grp.MoveTo(wp);
		
		Print(m_sCommanderUID + " Sending Group : " + grp.GetOwner().GetName() + " To : " + obj.GetOwner().GetName());
		send = false;
	}
	
	bool RegisterGroup(DCO_GroupUtilityComponent grp)
	{
		if (!m_aOwnedGroup.Contains(grp))
			m_aOwnedGroup.Insert(grp);
		
		Print(m_sCommanderUID + " Group Count : " + m_aOwnedGroup.Count());
		return true;
	}
	
	bool GetObjective()
	{
		m_aObjective.Clear();
		CMD_AICommanderObjectiveComponent obj = AICommander_ManagerComponent.GetInstance().GetHighestPrioObjective(this);
		if (!m_aObjective.Contains(obj))
		{
			m_aObjective.Insert(obj);
			return true;
		}
		return false;
	}
	
	protected void InitializeCommander()
	{
		AICommander_ManagerComponent.GetInstance().RegisterCommander(this);
	}
	
	FactionKey GetCommanderFactionKey()
	{
		return m_sFactionKey;
	}
	
	string GetCommanderUID()
	{
		return m_sCommanderUID;
	}
	
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		
	}
	
	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);
		SetEventMask(owner, EntityEvent.FRAME);
	}
	
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		InitializeCommander();
	}
}