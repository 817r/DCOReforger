//------------------------------------------------------------------------------------------------
class DCO_AIFindCover
{
	static ref array<ref vector> m_CoverPositions = {};
	
	//------------------------------------------------------------------------------------------------
	static array<ref vector> GetCoverPositions()
	{
		return m_CoverPositions;
	}
	
	//------------------------------------------------------------------------------------------------
	static void ClearCoverPositions()
	{
		m_CoverPositions.Clear();
	}
};

modded class SCR_AIWorld : AIWorld
{
	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		DCO_World.GetWorld();
		
		super.EOnInit(owner);
	}
};