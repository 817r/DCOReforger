modded class SCR_AIGroupFireteam : Managed
{
	protected ref array<ref SCR_AIGroupFireteam> m_aFireteams = {};
	override void AddMember(AIAgent agent, SCR_AIInfoComponent infoComponent)
	{
		super.AddMember(agent, infoComponent);
		
		SCR_ChimeraAIAgent chimeraAIAgent = SCR_ChimeraAIAgent.Cast(agent);

		if (chimeraAIAgent)
		{
			chimeraAIAgent.SetFireteam(this);
			
			chimeraAIAgent.AddFireteam(this);
		}
	}

	AIAgent GetLeader()
	{
		if (m_aAgents.IsEmpty())
			return null;
		
		return m_aAgents[0];
	}

	array<AIAgent> GetMembers()
	{
		if (m_aAgents.IsEmpty())
			return null;
		
		return m_aAgents;
	}

	ref array<ref SCR_AIGroupFireteam> GetFireteams()
	{
		if (m_aFireteams.IsEmpty())
			return null;
		
		foreach (int index, SCR_AIGroupFireteam fireteam : m_aFireteams)
		{
			if (fireteam == null)
			{
				m_aFireteams.Remove(index);
			}
		}
		
		return m_aFireteams;
	}
};