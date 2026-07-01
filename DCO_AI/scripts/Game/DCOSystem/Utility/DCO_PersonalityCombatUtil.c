// Utility bersama buat nyambungin DCO_EAIPersonality ke keputusan combat yang beneran
// berasa: morale threshold (seberapa gampang panik), stopped-wait-time (seberapa
// sering gerak), sama burst-fire distance (seberapa berani buang peluru dari jarak
// jauh). Dipisah jadi 1 file biar tuning-nya kumpul di satu tempat, konsisten sama
// pola DCO_MoraleCombatUtility.
class DCO_PersonalityCombatUtility
{
	//------------------------------------------------------------------------------------------------
	//! Skala threshold morale (ANXIOUS/MANIAC/BREAK). CAUTIOUS panik lebih cepet
	//! (threshold diturunin), RECKLESS jauh lebih susah panik (threshold dinaikin).
	//! Dijaga max ~1.15 (RECKLESS) biar BREAK_THRESHOLD (3.7) * scale masih di bawah
	//! cap m_fMoraleTotal (4.5) -- RECKLESS tetep BISA break, cuma butuh kondisi ekstrem.
	static float GetMoraleThresholdScale(SCR_AIUtilityComponent utility)
	{
		DCO_EAIPersonality p = GetPersonalitySafe(utility);
		
		switch (p)
		{
			case DCO_EAIPersonality.CAUTIOUS:
				return 0.75;
			case DCO_EAIPersonality.AGGRESSIVE:
				return 1.05;
			case DCO_EAIPersonality.RECKLESS:
				return 1.15;
			default:
				return 1.0; // STANDARD
		}
		return 1;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Skala stopped-wait-time (berapa lama diem sebelum push move request lagi).
	//! AGGRESSIVE/RECKLESS gerak lebih sering (waktu diem dipangkas), CAUTIOUS lebih
	//! betah hunker down di cover (waktu diem diperpanjang).
	static float GetStoppedWaitTimeScale(SCR_AIUtilityComponent utility)
	{
		DCO_EAIPersonality p = GetPersonalitySafe(utility);
		
		switch (p)
		{
			case DCO_EAIPersonality.CAUTIOUS:
				return 1.4;
			case DCO_EAIPersonality.AGGRESSIVE:
				return 0.7;
			case DCO_EAIPersonality.RECKLESS:
				return 0.5;
			default:
				return 1.0; // STANDARD
		}
		return 1;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Skala jarak maksimum buat berani burst-fire (BURST_FIRE_MAX_DISTANCE).
	//! AGGRESSIVE/RECKLESS lebih royal buang peluru burst dari jauh, CAUTIOUS lebih
	//! konservatif (pilih single-fire buat hemat amunisi & kontrol lebih baik).
	static float GetBurstDistanceScale(SCR_AIUtilityComponent utility)
	{
		DCO_EAIPersonality p = GetPersonalitySafe(utility);
		
		switch (p)
		{
			case DCO_EAIPersonality.CAUTIOUS:
				return 0.6;
			case DCO_EAIPersonality.AGGRESSIVE:
				return 1.4;
			case DCO_EAIPersonality.RECKLESS:
				return 1.7;
			default:
				return 1.0; // STANDARD
		}
		return 1;
	}
	
	//------------------------------------------------------------------------------------------------
	protected static DCO_EAIPersonality GetPersonalitySafe(SCR_AIUtilityComponent utility)
	{
		if (!utility || !utility.m_DCOConfig)
			return DCO_EAIPersonality.STANDARD;
		
		return utility.m_DCOConfig.GetPersonality();
	}
	
	//------------------------------------------------------------------------------------------------
	//! Chance (0-1) buat beneran lempar grenade begitu kondisi trigger kepenuhin.
	//! CAUTIOUS lebih hemat/jarang, AGGRESSIVE/RECKLESS hampir selalu ambil kesempatan.
	static float GetGrenadeThrowChance(SCR_AIUtilityComponent utility)
	{
		DCO_EAIPersonality p = GetPersonalitySafe(utility);
		
		switch (p)
		{
			case DCO_EAIPersonality.CAUTIOUS:
				return 0.35;
			case DCO_EAIPersonality.AGGRESSIVE:
				return 0.85;
			case DCO_EAIPersonality.RECKLESS:
				return 0.95;
			default:
				return 0.65; // STANDARD
		}
		return 0.65;
	}
}