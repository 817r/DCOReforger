[ComponentEditorProps(category: "GameScripted/Commander")]
class AICommander_ManagerComponentClass : ScriptComponentClass
{
}

class AICommander_ManagerComponent : ScriptComponent
{
	ref array<FactionKey> m_aAvailableFactions = {};
	ref array<DCO_GroupUtilityComponent> m_aGroup = {};
	
	ref array<AICommander_BaseComponent> m_aCommander = {};
	
	ref array<CMD_AICommanderObjectiveComponent> m_aObjective = {};
	
	protected static AICommander_ManagerComponent s_Instance;
	
	bool RegisterGroup(DCO_GroupUtilityComponent grp)
	{
		if (!m_aGroup.Contains(grp))
			m_aGroup.Insert(grp);
		
		AssignGroupToRandomCommander(grp);
		
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
			Print("FACTION AVAILABLE : " + f.GetFactionName());
		}
	}
	
	CMD_AICommanderObjectiveComponent GetHighestPrioObjective(AICommander_BaseComponent cmd)
	{
		CMD_AICommanderObjectiveComponent nearestEntity = null;
		float smallestDistSq = float.MAX;
		
		foreach (CMD_AICommanderObjectiveComponent e : m_aObjective)
		{
			float distSq = vector.DistanceSq(e.GetOwner().GetOrigin(), cmd.GetOwner().GetOrigin());
			if (distSq < smallestDistSq)
			{
				nearestEntity = e;
				smallestDistSq = distSq;
			}
		}
		
		return nearestEntity;
	}
	
	void AssignGroupToRandomCommander(DCO_GroupUtilityComponent grp)
	{
		FactionKey grpFk = grp.GetFactionKey();
		array<AICommander_BaseComponent> aAvailCommander = {};
		
		foreach(AICommander_BaseComponent a : m_aCommander)
		{
			if (a.GetCommanderFactionKey() == grpFk)
				aAvailCommander.Insert(a);
		}
		
		AICommander_BaseComponent rnd = aAvailCommander.GetRandomElement();
		rnd.RegisterGroup(grp);
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
