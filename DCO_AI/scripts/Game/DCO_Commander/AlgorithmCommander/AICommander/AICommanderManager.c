[ComponentEditorProps(category: "GameScripted/Commander")]
class AICommander_ManagerComponentClass : ScriptComponentClass
{
}

class AICommander_ManagerComponent : ScriptComponent
{
	ref array<FactionKey> m_aAvailableFactions = {};
	
	ref array<AICommander_BaseComponent> m_aCommander = {};
	
	ref array<CMD_AICommanderObjectiveComponent> m_aObjective = {};
	
	protected static AICommander_ManagerComponent s_Instance;
	
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
		
		//Print("REGISTERING : " + cmd.GetCommanderUID() + " FACTION : " + cmd.GetCommanderFactionKey());
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
			//Print("FACTION AVAILABLE : " + f.GetFactionName());
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
	
	void GetTopObjectives(AICommander_BaseComponent forCommander, int count, out array<CMD_AICommanderObjectiveComponent> result)
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
 
			float score = obj.ComputePriorityScore(fk, worldTime, forCommander.GetOwner().GetOrigin());
 
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
		//Print(string.Format("[CMD_Manager] Group '%1' → Commander '%2'",
			//grp.GetOwner().GetName(), chosen.GetCommanderUID()));
	}
	
	void AssignVehicleToCommander(IEntity grp)
	{
		if (!grp)
			return;
 		SCR_VehicleFactionAffiliationComponent fac = SCR_VehicleFactionAffiliationComponent.Cast(grp.FindComponent(SCR_VehicleFactionAffiliationComponent));
		FactionKey grpFk = fac.GetDefaultFactionKey();
 
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
		Print(string.Format("[VEH_Manager] VEH '%1' → Commander '%2'", grp.GetName(), chosen.GetCommanderUID()));
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
