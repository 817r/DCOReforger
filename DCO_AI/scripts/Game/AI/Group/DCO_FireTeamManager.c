modded class SCR_AIGroupFireteamManager : Managed
{
	protected const int FIRETEAM_MIN_SIZE = 2;
	
	protected SCR_AIGroup m_Group;
	protected ref array<ref SCR_AIGroupFireteam> m_aFireteams = {};
	bool m_bRebalanceFireteams = true; // True when fireteams become unbalanced
	
	// Fireteam events
	protected ref ScriptInvokerBase<SCR_AIOnFireteamRemoved> Event_OnFireteamRemoved = new ScriptInvokerBase<SCR_AIOnFireteamRemoved>();
	
	//---------------------------------------------------------------------------------------------------
	void SCR_AIGroupFireteamManager(SCR_AIGroup group)
	{
		m_Group = group;
	}
	
	override protected SCR_AIGroupFireteam CreateFireteam()
	{
		SCR_AIGroupFireteam ft = new SCR_AIGroupFireteam();
		m_aFireteams.Insert(ft);
		return ft;
	}

	override protected static int GetMaxFireteamSize(int groupSize)
	{
		if (groupSize >= 12)
			return 3;
		else if (groupSize > 9)
			return 3;
		else if (groupSize == 4)
			return 2; 
		else
			return groupSize; // When below 4 members, one fireteam or 1-2-3
	}
	
	
	//---------------------------------------------------------------------------------------------------
	override void RebalanceFireteams()
	{
		int groupSize = m_Group.GetAgentsCount();
		
		// Bail if group size is 0
		if (groupSize == 0)
		{
			m_bRebalanceFireteams = false;
			return;
		}
				
		int maxFtSize = GetMaxFireteamSize(groupSize);
		float fMaxFtSize = maxFtSize;
		int agentsCount = m_Group.GetAgentsCount();
		float fAgentsCount = agentsCount;
		
		int desiredFtCount = Math.Ceil(fAgentsCount / fMaxFtSize);
		
		if (m_aFireteams.Count() < desiredFtCount)
		{
			// Create new fireteams
			int newFtCount = desiredFtCount - m_aFireteams.Count();
			for (int i = 0; i < newFtCount; i++)
				CreateFireteam();
		}
		else if (m_aFireteams.Count() > desiredFtCount)
		{
			// Delete fireteams ...
			
			int deleteFtCount = m_aFireteams.Count() - desiredFtCount;
			array<SCR_AIGroupFireteam> fireteamsDelete = {};
			
			// Find smallelst fireteams for deletion
			for (int i = 0; i < deleteFtCount; i++)
			{
				SCR_AIGroupFireteam smallestFt = FindSmallestFireteam(fireteamsDelete);
				fireteamsDelete.Insert(smallestFt);
			}
			
			// Move members from those selected fireteams, and delete them
			foreach (SCR_AIGroupFireteam fireteamDelete : fireteamsDelete)
			{
				SCR_AIGroupFireteam destinationFt = FindSmallestFireteam(fireteamsDelete);
				destinationFt.MoveMembersFrom(fireteamDelete, fireteamDelete.GetMemberCount());
				RemoveFireteam(fireteamDelete);
			}
		}
		
		array<SCR_AIGroupFireteam> fireteamsTooBig = {};
		array<SCR_AIGroupFireteam> fireteamsTooSmall = {};
		CountUnbalancedFireteams(fireteamsTooBig, fireteamsTooSmall);
		
		//array<SCR_AIGroupFireteam> fireteamsExclude = {};
		bool failed = false;
		int nIterations = 0;
		const int maxIterations = 64;
		while ((!fireteamsTooBig.IsEmpty() || !fireteamsTooSmall.IsEmpty()) && !failed && nIterations < maxIterations)
		{
			if (!fireteamsTooBig.IsEmpty())
			{
				// First split big fireteams
				SCR_AIGroupFireteam srcFt = fireteamsTooBig[0];
				
				// Take one big fireteam and move some members to smaller fireteams
				int nExcessMembers = srcFt.GetMemberCount() - maxFtSize;
				for (int i = 0; i < nExcessMembers; i++)
				{
					SCR_AIGroupFireteam dstFt = FindSmallestFireteam(fireteamsTooBig);
					if (!dstFt)
					{
						// It shouldn't be possible
						Print(string.Format("SCR_AIGroupUtilityComponent: failed to reorganize fireteams, all other fireteams are full. %1",
							DiagGetFireteamsData()), LogLevel.ERROR);
						failed = true;
						break;
					}
					else
					{
						dstFt.MoveMembersFrom(srcFt, 1);
					}
				}
			}
			else if (!fireteamsTooSmall.IsEmpty())
			{
				// Second fill up the smallest fireteams
				SCR_AIGroupFireteam dstFt = fireteamsTooSmall[0];
				
				int nLackMembers = FIRETEAM_MIN_SIZE - dstFt.GetMemberCount(); // How many more members we need
				
				for (int i = 0; i < nLackMembers; i++)
				{
					SCR_AIGroupFireteam srcFt = FindBiggestFireteam(fireteamsTooSmall);
					
					if (!srcFt)
					{
						// It shouldn't be possible in general case
						// It could only happen when group size is very small
						//if (agentsCount > maxFtSize)
							Print(string.Format("SCR_AIGroupUtilityComponent: failed to reorganize fireteams, all other fireteams are too small. %1",
								DiagGetFireteamsData()), LogLevel.ERROR);
						failed = true; // For very small group size (1) it might be impossible to make 'balanced' fireteams at all, due to lack of group members
						break;
					}
					else
					{
						dstFt.MoveMembersFrom(srcFt, 1);
					}
				}
				
			}
			
			CountUnbalancedFireteams(fireteamsTooBig, fireteamsTooSmall);
			nIterations++;
		}
		
		if (nIterations == maxIterations)
		{
			Print(string.Format("SCR_AIGroupUtilityComponent: RebalanceFireteams: max amount of iterations has been reached. %1", DiagGetFireteamsData()), LogLevel.ERROR);
		}
		
		m_bRebalanceFireteams = false;
	}
	override protected void CountUnbalancedFireteams(notnull array<SCR_AIGroupFireteam> fireteamsTooBig, notnull array<SCR_AIGroupFireteam> fireteamsTooSmall)
	{
		int maxFtSize = GetMaxFireteamSize(m_Group.GetAgentsCount());
		fireteamsTooBig.Clear();
		fireteamsTooSmall.Clear();
		foreach (SCR_AIGroupFireteam ft : m_aFireteams)
		{
			int size = ft.GetMemberCount();
			if (size == maxFtSize) // Perfect
				continue;
			
			if (size > maxFtSize)
				fireteamsTooBig.Insert(ft);
			else if (size < FIRETEAM_MIN_SIZE)
				fireteamsTooSmall.Insert(ft); // Includes empty or with one member
		}
	}
	
	//---------------------------------------------------------------------------------------------------
	//! Returns string with data about fireteams
	override string DiagGetFireteamsData()
	{
		string s = string.Format("Fireteams: %1: ", m_aFireteams.Count());
		foreach (SCR_AIGroupFireteam ft : m_aFireteams)
		{
			string strLocked = string.Empty;
			if (ft.IsLocked())
				strLocked = "L";
			
			s = s + string.Format("%1%2, ", ft.GetMemberCount(), strLocked);
		}
		return s;
	}
	
	//---------------------------------------------------------------------------------------------------
	override void DiagDrawFireteams()
	{
		array<AIAgent> members = {};
		foreach (int fireteamId, SCR_AIGroupFireteam ft : m_aFireteams)
		{
			ft.GetMembers(members);
			foreach (AIAgent agent : members)
			{
				IEntity e = agent.GetControlledEntity();
				if (!e)
					continue;
				vector textPos = e.GetOrigin() + Vector (0, 0.5, 0);
				string text = string.Format("FT: %1", fireteamId);
				DebugTextWorldSpace.Create(GetGame().GetWorld(), text, DebugTextFlags.ONCE | DebugTextFlags.CENTER | DebugTextFlags.FACE_CAMERA,
					textPos[0], textPos[1], textPos[2], color: Color.GREEN, bgColor: Color.BLACK,
					size: 13.0); 
			}
		}
	}
	
	ref array<ref SCR_AIGroupFireteam> GetFT()
	{
		return m_aFireteams;
	}
}