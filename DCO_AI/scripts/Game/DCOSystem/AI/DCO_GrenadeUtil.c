class DCO_GrenadeUtility
{
	protected static ref map<IEntity, float> s_mLastGrenadeThrowTime = new map<IEntity, float>();
	
	static const float GRENADE_COOLDOWN_MS = 15000.0; // per-unit, jangan lempar lebih dari 1x per 15 detik
	
	//------------------------------------------------------------------------------------------------
	//! Cek gabungan cooldown + personality chance. Return true = boleh lempar sekarang.
	//! TIDAK otomatis mulai cooldown -- caller yang beneran berhasil queue action harus
	//! manggil NotifyGrenadeThrown() abis ini biar cooldown mulai jalan.
	static bool CanThrowGrenadeNow(SCR_AIUtilityComponent utility)
	{
		if (!utility || !utility.m_OwnerEntity)
			return false;
		
		IEntity myEntity = utility.m_OwnerEntity;
		float worldTime_ms = GetGame().GetWorld().GetWorldTime();
		
		float lastThrow;
		if (s_mLastGrenadeThrowTime.Find(myEntity, lastThrow))
		{
			if ((worldTime_ms - lastThrow) < GRENADE_COOLDOWN_MS)
				return false;
		}
		
		float chance = DCO_PersonalityCombatUtility.GetGrenadeThrowChance(utility);
		if (Math.RandomFloat01() > chance)
			return false;
		
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Panggil abis AddAction(gren) berhasil di-queue, buat mulai cooldown unit ini.
	static void NotifyGrenadeThrown(SCR_AIUtilityComponent utility)
	{
		if (!utility || !utility.m_OwnerEntity)
			return;
		
		s_mLastGrenadeThrowTime.Set(utility.m_OwnerEntity, GetGame().GetWorld().GetWorldTime());
	}
}