class DCO_FindPosInBuilding : Managed
{
	protected IEntity m_Building;
	
	protected vector m_vLocalMins, m_vLocalMaxs;
	
	void DCO_FindPosInBuilding(IEntity building)
	{
		m_Building = building;
		m_Building.GetBounds(m_vLocalMins, m_vLocalMaxs);
	}
	
	protected vector GetRandomPosInBounds()
	{
		vector localPos;
		for (int i = 0; i < 3; i++)
		{
			localPos[i] = Math.RandomFloatInclusive(m_vLocalMins[i], m_vLocalMaxs[i]);
		};
		
		return m_Building.CoordToParent(localPos);
	}
}