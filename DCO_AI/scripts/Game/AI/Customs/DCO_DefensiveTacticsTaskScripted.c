class DCO_AllocateDefensiveMember : AITaskScripted
{
	static const string PORT_PRIORITY_LEVEL			= "PriorityLevel";
	
	protected SCR_AIGroup m_groupOwner;
	protected float m_fPriorityLevel;
	protected SCR_AIGroupUtilityComponent m_Utility;
	protected SCR_MailboxComponent m_Mailbox;
	protected DCO_DefendActivityTactics m_RelatedActivity;
	
	//------------------------------------------------------------------------------------------------
	override bool VisibleInPalette() {return true;}
	
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
		m_RelatedActivity = DCO_DefendActivityTactics.Cast(m_Utility.GetCurrentAction());
	}
	
	//------------------------------------------------------------------------------------------------
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		if (!m_groupOwner)
			return ENodeResult.FAIL;

		GetVariableIn(PORT_PRIORITY_LEVEL,m_fPriorityLevel);
		
		array<AIAgent> groupMembers = {};		
		m_groupOwner.GetAgents(groupMembers);
		if (groupMembers.IsEmpty())
			return ENodeResult.FAIL;

		int numSAToOccupy;
		
		ref array<BaseCompartmentSlot> compartments = {};
		ref array<AISmartActionComponent> smartActions = {};
		
		m_groupOwner.GetAllocatedCompartments(compartments);
		for (int i = compartments.Count() - 1; i >= 0; i--)
		{
			if (!TurretCompartmentSlot.Cast(compartments[i]))
				compartments.Remove(i);			
		}	
		
		m_groupOwner.GetAllocatedSmartActions(smartActions);	
			
		// Distribution of turrets, actions and sector defends (randomized)
		for (int i = 0, max = groupMembers.Count(); i < max; i++)
		{
			int j = groupMembers.GetRandomIndex();
			if (compartments.Count() > 0)
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
			groupMembers.Remove(j);
		}
		
		return ENodeResult.SUCCESS;
	}	
	
	//Sends order to agent, reserves compartment
	protected bool OccupyTurret(AIAgent who, BaseCompartmentSlot slot)
	{
		IEntity vehicle = slot.GetVehicle();
		if (!vehicle)
			return false;
	
		SCR_AIMessage_GetIn getInMessage = SCR_AIMessage_GetIn.Create(vehicle, null, EAICompartmentType.Turret, false, m_fPriorityLevel, null, m_RelatedActivity);
		m_Mailbox.RequestBroadcast(getInMessage, who);
		m_groupOwner.AddUsableVehicle(vehicle);
		return true;
	}
	
	//Sends order to agent, reserves smart action
	protected bool OccupySA(AIAgent who, AISmartActionComponent smartAction)
	{		
		SCR_AISmartActionComponent smartActionComponent = SCR_AISmartActionComponent.Cast(smartAction);
		if (!smartActionComponent)
			return false;
		smartActionComponent.ReserveAction(who);
		SCR_AIMessage_PerformAction actionMessage = SCR_AIMessage_PerformAction.Create(null, "", smartActionComponent, false, m_fPriorityLevel, null, m_RelatedActivity);
		m_Mailbox.RequestBroadcast(actionMessage, who);
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	protected static ref TStringArray s_aVarsIn = {
		PORT_PRIORITY_LEVEL		
	};
	override TStringArray GetVariablesIn()
	{
		return s_aVarsIn;
	}
	
	//------------------------------------------------------------------------------------------------
	override string GetOnHoverDescription()
	{
		return "AllocateActionsForDefendActivity: Goes over all group members and alocates them either turret, smart action or sector defend.\n Works only inside defend activity under defend waypoint.";
	}
}