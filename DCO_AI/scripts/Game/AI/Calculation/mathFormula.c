class DCO_Math
{
	static vector GetPositionFromYaw(float degree, vector ownerOrigin, vector enemyOrigin, float distance)
	{
		float toYaw = (enemyOrigin - ownerOrigin).Normalized().ToYaw();
		
		toYaw += degree;
		
		vector fromYaw = vector.FromYaw(toYaw);
		
		vector position = ownerOrigin + (fromYaw * distance);
		
		return position;
	}
}