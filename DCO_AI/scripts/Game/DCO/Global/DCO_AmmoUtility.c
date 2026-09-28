class DCO_AmmoUtility
{
	protected static const int AMMO_HARDSTOP_ROUNDS = 2;
	protected static const int AMMO_CRITICAL_ROUNDS  = 10;
	protected static const int AMMO_LOW_ROUNDS       = 25;

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

	protected static bool IsMagicMag(SCR_AIUtilityComponent utility)
	{
		if (!utility || !utility.m_DCOConfig)
			return false;

		return utility.m_DCOConfig.GetMagicMag();
	}

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
				return 1.0;
		}
		return 1.0;
	}
}