//------------------------------------------------------------------------------------------------
//! Catatan waktu gerakan cover per AI, dipakai bareng oleh semua danger reaction.
//! Tujuannya: total pergerakan satu AI punya batas, walau pemicunya datang dari reaction berbeda.
//------------------------------------------------------------------------------------------------
class DCO_CoverMoveBudget
{
	protected static const int PRUNE_THRESHOLD = 128;
	protected static const float PRUNE_STALE_AGE_MS = 30000.0;

	protected static ref map<IEntity, float> s_mLastMoveTime = new map<IEntity, float>();

	//------------------------------------------------------------------------------------------------
	//! True kalau jarak waktu sejak gerakan cover terakhir (dari reaction mana pun) sudah melewati minGap_s
	static bool CanMove(IEntity entity, float minGap_s)
	{
		if (!entity)
			return false;

		float lastTime_ms;
		if (!s_mLastMoveTime.Find(entity, lastTime_ms))
			return true;

		return (GetGame().GetWorld().GetWorldTime() - lastTime_ms) > (minGap_s * 1000.0);
	}

	//------------------------------------------------------------------------------------------------
	//! Dipanggil setiap kali sebuah reaction benar-benar mengirim gerakan cover
	static void MarkMove(IEntity entity)
	{
		if (!entity)
			return;

		float now_ms = GetGame().GetWorld().GetWorldTime();
		s_mLastMoveTime.Set(entity, now_ms);

		if (s_mLastMoveTime.Count() > PRUNE_THRESHOLD)
			Prune(now_ms);
	}

	//------------------------------------------------------------------------------------------------
	//! Sisa waktu (detik) sebelum boleh bergerak lagi, buat keperluan log
	static float GetTimeSinceLastMove(IEntity entity)
	{
		if (!entity)
			return -1;

		float lastTime_ms;
		if (!s_mLastMoveTime.Find(entity, lastTime_ms))
			return -1;

		return (GetGame().GetWorld().GetWorldTime() - lastTime_ms) / 1000.0;
	}

	//------------------------------------------------------------------------------------------------
	protected static void Prune(float now_ms)
	{
		array<IEntity> toRemove = {};

		foreach (IEntity ent, float lastTime_ms : s_mLastMoveTime)
		{
			if (!ent || (now_ms - lastTime_ms) > PRUNE_STALE_AGE_MS)
				toRemove.Insert(ent);
		}

		foreach (IEntity entRemove : toRemove)
		{
			s_mLastMoveTime.Remove(entRemove);
		}
	}
}