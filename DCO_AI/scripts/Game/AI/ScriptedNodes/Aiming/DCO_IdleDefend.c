class SCR_DCOAIAllocateActionsIdleDefendActivity: AITaskScripted
{	
	protected SCR_AIGroup m_groupOwner;
	protected SCR_AIGroupUtilityComponent m_Utility;
	protected SCR_MailboxComponent m_Mailbox;
	
	//------------------------------------------------------------------------------------------------
	static override bool VisibleInPalette() {return true;}
	
	//------------------------------------------------------------------------------------------------
	override void OnInit(AIAgent owner)
	{
		m_groupOwner = SCR_AIGroup.Cast(owner);
		if (!m_groupOwner)
		{
			m_groupOwner = SCR_AIGroup.Cast(owner.GetParentGroup());
			if (!m_groupOwner)
			{
				SCR_AgentMustBeAIGroup(this, owner);
				return;
			}	
		}
		m_Utility = SCR_AIGroupUtilityComponent.Cast(m_groupOwner.FindComponent(SCR_AIGroupUtilityComponent));
		m_Mailbox = m_Utility.m_Mailbox;
	}
	
	//------------------------------------------------------------------------------------------------
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		if (!m_groupOwner)
			return ENodeResult.FAIL;
		IEntity waypointEnt;
		
		if (!m_Mailbox || !m_Utility)
			return ENodeResult.FAIL;
		
		array<AIAgent> groupMembers = {};		
		m_groupOwner.GetAgents(groupMembers);
		if (groupMembers.IsEmpty())
			return ENodeResult.FAIL;

		int numSAToOccupy;
		
		bool useTurrets = true;
		
		ref array<BaseCompartmentSlot> compartments = {};
		ref array<AISmartActionComponent> smartActions = {};
		
		if (useTurrets)
		{
			m_groupOwner.GetAllocatedCompartments(compartments);
			for (int i = compartments.Count() - 1; i >= 0; i--)
			{
				if (!TurretCompartmentSlot.Cast(compartments[i]))
					compartments.Remove(i);			
			}	
		}
		
		m_groupOwner.GetAllocatedSmartActions(smartActions);
		numSAToOccupy = Math.Round(smartActions.Count());	
			
		// Distribution of turrets, actions and sector defends (randomized)
		for (int i = 0, max = groupMembers.Count(); i < max; i++)
		{
			int j = groupMembers.GetRandomIndex();
			if (useTurrets && compartments.Count() > 0)
			{
				OccupyTurret(groupMembers[j], compartments[0]);
				compartments.Remove(0);
				groupMembers.Remove(j);
				continue;
			}
			if (numSAToOccupy > 0)
			{
				OccupySA(groupMembers[j], smartActions[0]);
				smartActions.Remove(0);
				groupMembers.Remove(j);
				numSAToOccupy--;
				continue;
			}
		}
		
		return ENodeResult.SUCCESS;
	}	
	
	//Sends order to agent, reserves compartment
	protected bool OccupyTurret(AIAgent who, BaseCompartmentSlot slot)
	{
		IEntity vehicle = slot.GetVehicle();
		if (!vehicle)
			return false;
		
		SCR_AIVehicleUsageComponent vehicleUsageComp = SCR_AIVehicleUsageComponent.FindOnNearestParent(vehicle, vehicle);
		if (!vehicleUsageComp)
			return false;
	
		SCR_AIMessage_GetIn getInMessage = SCR_AIMessage_GetIn.Create(vehicle, null, EAICompartmentType.Turret, false, 50, null, null, slot);
		m_Mailbox.RequestBroadcast(getInMessage, who);
		m_groupOwner.GetGroupUtilityComponent().AddUsableVehicle(vehicleUsageComp);
		
		return true;
	}
	
	//Sends order to agent, reserves smart action
	protected bool OccupySA(AIAgent who, AISmartActionComponent smartAction)
	{		
		SCR_AISmartActionComponent smartActionComponent = SCR_AISmartActionComponent.Cast(smartAction);
		if (!smartActionComponent)
			return false;
		smartActionComponent.ReserveAction(who);
		SCR_AIMessage_PerformAction actionMessage = SCR_AIMessage_PerformAction.Create(null, "", smartActionComponent, false, 50, null, null);
		m_Mailbox.RequestBroadcast(actionMessage, who);
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	static override string GetOnHoverDescription()
	{
		return "AllocateActionsForDefendActivity: Goes over all group members and alocates them either turret, smart action or sector defend.\n Works only inside defend activity under defend waypoint.";
	}
}