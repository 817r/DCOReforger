class DCO_PersonalityCombatUtility
{
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
				return 1.0;
		}
		return 1.0;
	}

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
				return 1.0;
		}
		return 1.0;
	}
	
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
				return 1.0;
		}
		return 1.0;
	}
	
	//------------------------------------------------------------------------------------------------
	protected static DCO_EAIPersonality GetPersonalitySafe(SCR_AIUtilityComponent utility)
	{
		if (!utility || !utility.m_DCOConfig)
			return DCO_EAIPersonality.STANDARD;
		
		return utility.m_DCOConfig.GetPersonality();
	}

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
				return 0.98;
			default:
				return 0.7;
		}
		return 0.65;
	}

	static float GetEndangeredReturnFireThreshold(SCR_AIUtilityComponent utility)
	{
		DCO_EAIPersonality p = GetPersonalitySafe(utility);
		
		switch (p)
		{
			case DCO_EAIPersonality.CAUTIOUS:
				return 2;
			case DCO_EAIPersonality.AGGRESSIVE:
				return 1.2;
			case DCO_EAIPersonality.RECKLESS:
				return 1;
			default:
				return 1.7; // STANDARD
		}
		return 1.7;
	}

	static float GetTakeCoverChanceScale(SCR_AIUtilityComponent utility)
	{
		DCO_EAIPersonality p = GetPersonalitySafe(utility);
		
		switch (p)
		{
			case DCO_EAIPersonality.CAUTIOUS:
				return 1.3;
			case DCO_EAIPersonality.AGGRESSIVE:
				return 0.6;
			case DCO_EAIPersonality.RECKLESS:
				return 0.3;
			default:
				return 1.0; // STANDARD
		}
		return 1.0;
	}
	
	static float GetInvestigateEagernessScale(SCR_AIUtilityComponent utility)
	{
		DCO_EAIPersonality p = GetPersonalitySafe(utility);
		
		switch (p)
		{
			case DCO_EAIPersonality.CAUTIOUS:
				return 1.6;
			case DCO_EAIPersonality.AGGRESSIVE:
				return 0.6;
			case DCO_EAIPersonality.RECKLESS:
				return 0.4;
			default:
				return 1.0; // STANDARD
		}
		return 1.0;
	}
	
	protected static DCO_AISKILL GetSkillSafe(SCR_AIUtilityComponent utility)
	{
		if (!utility || !utility.m_DCOConfig)
			return DCO_AISKILL.REGULAR;
		
		return utility.m_DCOConfig.GetAISkill();
	}
	
	static float GetDisciplineBreakChanceScale(SCR_AIUtilityComponent utility)
	{
		DCO_EAIPersonality p = GetPersonalitySafe(utility);
		
		switch (p)
		{
			case DCO_EAIPersonality.CAUTIOUS:
				return 0.4;
			case DCO_EAIPersonality.AGGRESSIVE:
				return 1.2;
			case DCO_EAIPersonality.RECKLESS:
				return 1.6;
			default:
				return 1.0; // STANDARD
		}
		return 1.0;
	}
	
	static float GetSkillDisciplineFactor(SCR_AIUtilityComponent utility)
	{
		DCO_AISKILL skill = GetSkillSafe(utility);
		
		switch (skill)
		{
			case DCO_AISKILL.NOOB:        return 1.0;
			case DCO_AISKILL.ROOKIE:      return 0.8;
			case DCO_AISKILL.REGULAR:     return 0.55;
			case DCO_AISKILL.VETERAN:     return 0.35;
			case DCO_AISKILL.EXPERT:      return 0.2;
			case DCO_AISKILL.SPECIAL_OPS: return 0.08;
			case DCO_AISKILL.TERMINATOR:  return 0.0;
			default: return 0.55;
		}
		return 0.55;
	}
	
	static float GetThreatRememberFromPersonalityScale(SCR_AIUtilityComponent utility)
	{
		DCO_EAIPersonality p = GetPersonalitySafe(utility);
		
		switch (p)
		{
			case DCO_EAIPersonality.CAUTIOUS:
				return 5;
			case DCO_EAIPersonality.AGGRESSIVE:
				return 3;
			case DCO_EAIPersonality.RECKLESS:
				return 0.5;
			default:
				return 1;
		}
		return 2.5;
	}
	
	static float GetObserveDurationScale(SCR_AIUtilityComponent utility)
	{
		DCO_EAIPersonality p = GetPersonalitySafe(utility);

		switch (p)
		{
			case DCO_EAIPersonality.CAUTIOUS:
				return 1.5;
			case DCO_EAIPersonality.AGGRESSIVE:
				return 0.7;
			case DCO_EAIPersonality.RECKLESS:
				return 0.5;
			default:
				return 1.0;
		}
		return 1.0;
	}

	//! Skala lama AI nunduk sebelum nekat nembak balik pas ditekan terus-terusan.
	//! >1 = lebih lama nunduk. Dipakai bareng CRITICAL_RETURN_FIRE_BASE_S (5s).
	//! Hasil: CAUTIOUS 8.0s / STANDARD 5.0s / AGGRESSIVE 3.5s / RECKLESS 2.0s
	static float GetReturnFireDelayScale(SCR_AIUtilityComponent utility)
	{
		DCO_EAIPersonality p = GetPersonalitySafe(utility);

		switch (p)
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

	//! Skala lama "lock" reposisi selama musuh masih dalam jarak tembak optimal.
	//! <1 = lebih cepat rela ninggalin posisi buat cari cover.
	//! Sengaja BERLAWANAN arah sama GetStoppedWaitTimeScale: yang itu ngatur
	//! "betah diam di posisi bagus", yang ini ngatur "keras kepala tetap nembak".
	//! Dipakai bareng REPOSITION_LOCK_BASE_S (5s).
	//! Hasil: CAUTIOUS 3.0s / STANDARD 5.0s / AGGRESSIVE 7.0s / RECKLESS 9.0s
	static float GetRepositionLockScale(SCR_AIUtilityComponent utility)
	{
		DCO_EAIPersonality p = GetPersonalitySafe(utility);

		switch (p)
		{
			case DCO_EAIPersonality.CAUTIOUS:
				return 0.6;
			case DCO_EAIPersonality.AGGRESSIVE:
				return 1.4;
			case DCO_EAIPersonality.RECKLESS:
				return 1.8;
			default:
				return 1.0;
		}
		return 1.0;
	}

	//------------------------------------------------------------------------------------------------
	// ARAH GERAK COVER - skala bobot arah per personality
	// Dipakai SCR_AIDangerReaction_WeaponFired & SCR_AIDangerReaction_ProjectileHit.
	// Bobot dasar tetap dari attribute reaksi, personality cuma mengalikan.
	//------------------------------------------------------------------------------------------------

	//! Pengali bobot MUNDUR. RECKLESS 0 = tidak pernah mundur (kecuali semua bobot jadi 0).
	static float GetCoverDirBackwardScale(SCR_AIUtilityComponent utility)
	{
		DCO_EAIPersonality p = GetPersonalitySafe(utility);

		switch (p)
		{
			case DCO_EAIPersonality.CAUTIOUS:
				return 2.0;
			case DCO_EAIPersonality.AGGRESSIVE:
				return 0.3;
			case DCO_EAIPersonality.RECKLESS:
				return 0.0;
			default:
				return 1.0;
		}
		return 1.0;
	}

	//! Pengali bobot KIRI & KANAN (menyamping relatif ke ancaman).
	static float GetCoverDirLateralScale(SCR_AIUtilityComponent utility)
	{
		DCO_EAIPersonality p = GetPersonalitySafe(utility);

		switch (p)
		{
			case DCO_EAIPersonality.CAUTIOUS:
				return 1.0;
			case DCO_EAIPersonality.AGGRESSIVE:
				return 1.5;
			case DCO_EAIPersonality.RECKLESS:
				return 1.5;
			default:
				return 1.0;
		}
		return 1.0;
	}

	//! Pengali bobot BEBAS (ANYWHERE, cover terdekat dari arah mana pun).
	static float GetCoverDirAnywhereScale(SCR_AIUtilityComponent utility)
	{
		DCO_EAIPersonality p = GetPersonalitySafe(utility);

		switch (p)
		{
			case DCO_EAIPersonality.CAUTIOUS:
				return 0.5;
			case DCO_EAIPersonality.AGGRESSIVE:
				return 1.0;
			case DCO_EAIPersonality.RECKLESS:
				return 0.5;
			default:
				return 1.0;
		}
		return 1.0;
	}

	//! Bobot MAJU (absolut, bukan pengali - tidak ada bobot dasar maju di attribute reaksi).
	//! CAUTIOUS & STANDARD 0 = perilaku lama tidak berubah.
	static float GetCoverDirForwardWeight(SCR_AIUtilityComponent utility)
	{
		DCO_EAIPersonality p = GetPersonalitySafe(utility);

		switch (p)
		{
			case DCO_EAIPersonality.AGGRESSIVE:
				return 0.5;
			case DCO_EAIPersonality.RECKLESS:
				return 1.0;
			default:
				return 0.0;
		}
		return 0.0;
	}

	//! Pilih arah lari ke cover dari bobot dasar reaksi x skala personality.
	//! allowForward = false -> arah MAJU tidak pernah dipilih (bangunan, threat tinggi, peluru mendarat).
	//! Semua bobot hasil 0 = mundur (sama dengan konvensi PickCoverDirection lama).
	static SCR_EAICombatMoveDirection PickCoverDirectionForPersonality(SCR_AIUtilityComponent utility, float baseBack, float baseLeft, float baseRight, float baseAnywhere, float forwardWeightScale, bool allowForward)
	{
		float lateralScale = GetCoverDirLateralScale(utility);

		float wBack     = Math.Max(0.0, baseBack)     * GetCoverDirBackwardScale(utility);
		float wLeft     = Math.Max(0.0, baseLeft)     * lateralScale;
		float wRight    = Math.Max(0.0, baseRight)    * lateralScale;
		float wAnywhere = Math.Max(0.0, baseAnywhere) * GetCoverDirAnywhereScale(utility);

		float wForward = 0.0;
		if (allowForward)
			wForward = Math.Max(0.0, GetCoverDirForwardWeight(utility) * forwardWeightScale);

		float total = wBack + wLeft + wRight + wForward + wAnywhere;
		if (total <= 0)
			return SCR_EAICombatMoveDirection.BACKWARD;

		float r = Math.RandomFloat(0, total);

		if (r < wBack)
			return SCR_EAICombatMoveDirection.BACKWARD;
		r -= wBack;

		if (r < wLeft)
			return SCR_EAICombatMoveDirection.LEFT;
		r -= wLeft;

		if (r < wRight)
			return SCR_EAICombatMoveDirection.RIGHT;
		r -= wRight;

		if (r < wForward)
			return SCR_EAICombatMoveDirection.FORWARD;

		return SCR_EAICombatMoveDirection.ANYWHERE;
	}
}