class DCO_AIBase
{};

class DCO_System
{};

class DCO_World : DCO_System
{
	static BaseWorld m_World;
	
	//------------------------------------------------------------------------------------------------
	static BaseWorld GetWorld()
	{
		if (m_World)
			return m_World;
		
		m_World = GetGame().GetWorld();
		
		return m_World;
	}
	
	//------------------------------------------------------------------------------------------------
	static bool SurfaceIsWater(vector worldPosition)
	{
		
		float surfaceHeight = GetGame().GetWorld().GetSurfaceY(worldPosition[0], worldPosition[2]);
		
		worldPosition[1] = surfaceHeight;
		
		bool isWaterSurfaceSimple = ChimeraWorldUtils.TryGetWaterSurfaceSimple(GetGame().GetWorld(), worldPosition);
		
		return isWaterSurfaceSimple;

	}
};