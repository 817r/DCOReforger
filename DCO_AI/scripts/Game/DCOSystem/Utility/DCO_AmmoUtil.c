// Ammo conservation buat suppressive fire. Sebelumnya fireRate suppressive cuma
// dihitung dari jarak + threat level, gak peduli sisa ammo sama sekali -- AI bisa
// terus-terusan spray suppressive fire sampe kering walau musuh cuma "mungkin ada".
// Personality ngegeser threshold-nya: CAUTIOUS irit dari lebih awal, RECKLESS cuek
// sampe beneran mepet.
class DCO_AmmoUtility
{
	protected static const int AMMO_HARDSTOP_ROUNDS = 2;   // di bawah ini, JANGAN suppressive sama sekali -- simpen buat defense
	protected static const int AMMO_CRITICAL_ROUNDS  = 10; // di bawah ini, fire rate dipangkas abis-abisan
	protected static const int AMMO_LOW_ROUNDS       = 25; // di bawah ini, fire rate dikurangin sedang

	//------------------------------------------------------------------------------------------------
	//! True kalau ammo separah itu sehingga suppressive fire harus DIHINDARI TOTAL
	//! (bukan cuma dikurangi), biar sisa ammo bisa dipake buat defense diri sendiri
	//! kalau beneran diserang.
	static bool ShouldAvoidSuppressiveFire(SCR_AIUtilityComponent utility, BaseWeaponComponent weapon)
	{
		if (!weapon)
			return false;

		if (IsMagicMag(utility))
			return false;

		BaseMagazineComponent mag = weapon.GetCurrentMagazine();
		if (!mag)
			return false;

		int currentAmmo = mag.GetAmmoCount();
		int effectiveHardStop = Math.Round(AMMO_HARDSTOP_ROUNDS * GetPersonalityAmmoBias(utility));

		return currentAmmo <= effectiveHardStop;
	}

	//------------------------------------------------------------------------------------------------
	//! Skala fire rate berdasarkan ammo tersisa + personality. Return 1.0 kalau ammo
	//! cukup, unit magic-resupply, atau info ammo gak ketemu (gak mau asal nebak dan
	//! bikin AI under-shoot padahal ammonya sebenernya cukup).
	static float GetAmmoConservationScale(SCR_AIUtilityComponent utility, BaseWeaponComponent weapon)
	{
		if (!weapon)
			return 1.0;

		if (IsMagicMag(utility))
			return 1.0;

		BaseMagazineComponent mag = weapon.GetCurrentMagazine();
		if (!mag)
			return 1.0;

		int currentAmmo = mag.GetAmmoCount();
		float personalityBias = GetPersonalityAmmoBias(utility);

		int effectiveCritical = Math.Round(AMMO_CRITICAL_ROUNDS * personalityBias);
		int effectiveLow      = Math.Round(AMMO_LOW_ROUNDS * personalityBias);

		if (currentAmmo <= effectiveCritical)
			return 0.25;
		else if (currentAmmo <= effectiveLow)
			return 0.55;

		return 1.0;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool IsMagicMag(SCR_AIUtilityComponent utility)
	{
		if (!utility || !utility.m_DCOConfig)
			return false;

		return utility.m_DCOConfig.GetMagicMag();
	}

	//------------------------------------------------------------------------------------------------
	//! CAUTIOUS: threshold dinaikin (irit dari ammo masih lumayan banyak). RECKLESS:
	//! threshold diturunin jauh (baru mikirin ammo pas beneran hampir kering).
	protected static float GetPersonalityAmmoBias(SCR_AIUtilityComponent utility)
	{
		if (!utility || !utility.m_DCOConfig)
			return 1.0;

		switch (utility.m_DCOConfig.GetPersonality())
		{
			case DCO_EAIPersonality.CAUTIOUS:
				return 1.6;
			case DCO_EAIPersonality.AGGRESSIVE:
				return 0.7;
			case DCO_EAIPersonality.RECKLESS:
				return 0.4;
			default:
				return 1.0; // STANDARD
		}
		return 1.0;
	}
}