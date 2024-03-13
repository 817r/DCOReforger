modded class SCR_AIInvestigateClusterActivity : SCR_AIFireteamsClusterActivity
{
	protected DCO_AIInfoGroupComponent m_DCO_AIInfoGroupComponent;
	
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
				
				// Duration is large, since soldiers' investigation is tied to this activity
				SCR_AIMessage_Investigate msg = SCR_AIMessage_Investigate.Create(pos, 100, true, duration: 10000);
				msg.m_RelatedGroupActivity = this;
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
				
				SCR_AIMessage_AttackCluster msg = SCR_AIMessage_AttackCluster.Create(m_ClusterState, true);
				msg.m_RelatedGroupActivity = this;
				msg.SetReceiver(agent);
				comms.RequestBroadcast(msg, agent);
			}
		}
	}
}