modded class SCR_AIThreatSystem
{	
	protected int bulletClose;
	protected int bulletHit;
	
	
	float timeStampBullClose;
	const int bulletCloseFixedFlush = 3000;
	
	float timeStampBullHit;
	const int bulletHitFixedFlush = 5000;

	//------------------------------------------------------------------------------------------------
	//!
	//! Called by utilityComponent each EvaluateBehavior call
	//! \param[in] utility
	//! \param[in] timeSlice
	override void Update(SCR_AIUtilityComponent utility, float timeSlice)
	{
		super.Update(utility, timeSlice);
		timeStampBullClose += timeSlice;
		timeStampBullHit += timeSlice;
		if (timeStampBullClose > bulletCloseFixedFlush)
		{
			bulletClose = 0;
			timeStampBullClose = 0;
		}
		if (timeStampBullHit > bulletHitFixedFlush)
		{
			bulletHit = 0;
			timeStampBullHit = 0;
		}
	}
	
	void ThreatBulletClose(int count)
	{
		bulletClose += count;
	}
	
	void BulletHit(int count)
	{
		bulletHit += count;
	}
	
	int BulletCloseCount()
	{
		Print("Bullet Close " + bulletClose.ToString());
		return bulletClose;
	}
	
	int BulletHitCount()
	{
		Print("Bullet Hit " + bulletHit.ToString());
		return bulletHit;
	}
}