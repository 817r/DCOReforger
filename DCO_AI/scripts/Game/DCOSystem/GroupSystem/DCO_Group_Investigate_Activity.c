modded class SCR_AIInvestigateClusterActivity
{
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
				SCR_AIMessage_Investigate msg = SCR_AIMessage_Investigate.Create(this,pos, radius, true, duration: 10000);
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
}