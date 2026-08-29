modded class SCR_AIInvestigateClusterActivity
{
	static const float INVESTIGATION_ACCURATE_RADIUS_MIN = 10;
	
	override protected void SendInvestigateMessages(AICommunicationComponent comms, vector pos, float radius)
	{
		array<AIAgent> agents = {};
		foreach (SCR_AIGroupFireteamLock ft : m_aFireteamsInvestigate)
		{	
			ft.GetFireteam().GetMembers(agents);
			foreach (AIAgent agent : agents)
			{
				if (!agent)
					continue;
				
				if (SCR_AICompartmentHandling.IsInCompartment(agent))
					continue;
				
				// Duration is large, since soldiers' investigation is tied to this activity
				SCR_AIMessage_Investigate msg = SCR_AIMessage_Investigate.Create(this,pos, radius / 4, true, duration: 2000 * radius);
				msg.SetReceiver(agent);
				comms.RequestBroadcast(msg, agent);
			}
		}
		
		foreach (SCR_AIGroupFireteamLock ft : m_aFireteamsCover)
		{	
			ft.GetFireteam().GetMembers(agents);
			foreach (AIAgent agent : agents)
			{
				if (!agent)
					continue;
				
				if (SCR_AICompartmentHandling.IsInCompartment(agent))
					continue;
				
				SCR_AIMessage_Investigate msg = SCR_AIMessage_Investigate.Create(this,pos, radius, true, duration: 1500 * radius);
				msg.m_RelatedGroupActivity = this;
				msg.SetReceiver(agent);
				comms.RequestBroadcast(msg, agent);
			}
		}
	}
	
	protected void SendAccurateInvestigateMessages(AICommunicationComponent comms, vector pos, float radius)
	{
		array<AIAgent> agents = {};
		foreach (SCR_AIGroupFireteamLock ft : m_aFireteamsInvestigate)
		{	
			ft.GetFireteam().GetMembers(agents);
			foreach (AIAgent agent : agents)
			{
				if (!agent)
					continue;
				
				if (SCR_AICompartmentHandling.IsInCompartment(agent))
					continue;
				
				// Duration is large, since soldiers' investigation is tied to this activity
				SCR_AIMessage_Investigate msg = SCR_AIMessage_Investigate.Create(this,pos, radius, true, duration: 20000);
				msg.SetReceiver(agent);
				comms.RequestBroadcast(msg, agent);
			}
		}
		
		foreach (SCR_AIGroupFireteamLock ft : m_aFireteamsCover)
		{	
			ft.GetFireteam().GetMembers(agents);
			foreach (AIAgent agent : agents)
			{
				if (!agent)
					continue;
				
				if (SCR_AICompartmentHandling.IsInCompartment(agent))
					continue;
				
				SCR_AIMessage_Investigate msg = SCR_AIMessage_Investigate.Create(this,pos, radius * 2, true, duration: 10000);
				msg.m_RelatedGroupActivity = this;
				msg.SetReceiver(agent);
				comms.RequestBroadcast(msg, agent);
			}
		}
	}
	
	static void AccurateCalculateInvestigationArea(notnull SCR_AITargetClusterState s, out vector outCenterPos, out float outRadius)
	{
		const float minRadius = INVESTIGATION_ACCURATE_RADIUS_MIN;
		
		vector center = 0.5*(s.m_vBBMin + s.m_vBBMax);
		float radius = Math.Min(minRadius, vector.DistanceXZ(center, s.m_vBBMax));
		
		outCenterPos = center;
		outRadius = radius;
	}
	
	override void OnActionSelected()
	{
		super.OnActionSelected();
		
		if (!m_bOrdersSent)
		{
			vector investigatePos;
			float investigateRadius;
			
			if (vector.Distance(m_ClusterState.GetCenterPosition(), m_Utility.GetOwner().GetOrigin()))
				AccurateCalculateInvestigationArea(m_ClusterState, investigatePos, investigateRadius);
			else
				CalculateInvestigationArea(m_ClusterState, investigatePos, investigateRadius);
			
			AICommunicationComponent comms = m_Utility.m_Owner.GetCommunicationComponent();
			if (!comms)
				return;
			
			if (vector.Distance(m_ClusterState.GetCenterPosition(), m_Utility.GetOwner().GetOrigin()))
				SendAccurateInvestigateMessages(comms, investigatePos, investigateRadius);
			else
				SendInvestigateMessages(comms, investigatePos, investigateRadius);
			
			
		}
		
		// !! It runs once and then is suspended, we don't need to run the behavior tree for it.
		//SetActionIsSuspended(true);
	}
}