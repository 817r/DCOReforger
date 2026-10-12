class DCO_CoverMoveBudget
{

	protected static ref map<IEntity, float> s_mLastMoveTime;
	protected static map<IEntity, float> GetLastMoveTime()
	{
		if (!s_mLastMoveTime)
			s_mLastMoveTime = new map<IEntity, float>();
		return s_mLastMoveTime;
	}

	static bool CanMove(IEntity entity, float minGap_s)
	{
		if (!entity)
			return false;

		float lastTime_ms;
		if (!GetLastMoveTime().Find(entity, lastTime_ms))
			return true;

		return (GetGame().GetWorld().GetWorldTime() - lastTime_ms) > (minGap_s * 1000.0);
	}

	static void MarkMove(IEntity entity)
	{
		if (!entity)
			return;

		float now_ms = GetGame().GetWorld().GetWorldTime();
		GetLastMoveTime().Set(entity, now_ms);

		if (GetLastMoveTime().Count() > 128)
			Prune(now_ms);
	}

	static float GetTimeSinceLastMove(IEntity entity)
	{
		if (!entity)
			return -1;

		float lastTime_ms;
		if (!GetLastMoveTime().Find(entity, lastTime_ms))
			return -1;

		return (GetGame().GetWorld().GetWorldTime() - lastTime_ms) / 1000.0;
	}

	protected static void Prune(float now_ms)
	{
		array<IEntity> toRemove = {};

		foreach (IEntity ent, float lastTime_ms : GetLastMoveTime())
		{
			if (!ent || (now_ms - lastTime_ms) > 30000.0)
				toRemove.Insert(ent);
		}

		foreach (IEntity entRemove : toRemove)
		{
			GetLastMoveTime().Remove(entRemove);
		}
	}
}