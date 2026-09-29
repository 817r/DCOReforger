modded class SCR_AISuppressionVolumeBase
{






	static bool CreateSuppressionBox(vector center, float sizeXZ, float height, out vector bbMin, out vector bbMax)
	{
	    float half = sizeXZ * 0.5;

	    bbMin = center - Vector(half, 0, half);
	    bbMax = center + Vector(half, height, half);
	    return true;
	}
}