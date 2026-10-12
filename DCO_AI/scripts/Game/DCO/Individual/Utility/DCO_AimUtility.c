class DCO_AimUtility
{

	static vector ResolveOrientationPos(IEntity myEntity, vector threatPos, float traceFraction, float timeSinceSeen)
	{
		if (!myEntity)
			return threatPos;

		if (traceFraction > 0.5 && timeSinceSeen < 12.0)
			return threatPos;

		vector flat = threatPos;
		flat[1] = myEntity.GetOrigin()[1] + 1.6;
		return flat;
	}


	static float SkillSigma(DCO_AISKILL skill)
	{
		switch (skill)
		{
			case DCO_AISKILL.NOOB:        return 2.2;
			case DCO_AISKILL.ROOKIE:      return 1.5;
			case DCO_AISKILL.REGULAR:     return 1.0;
			case DCO_AISKILL.VETERAN:     return 0.85;
			case DCO_AISKILL.EXPERT:      return 0.7;
			case DCO_AISKILL.SPECIAL_OPS: return 0.6;
			case DCO_AISKILL.TERMINATOR:  return 0.5;
		}
		return 1.0;
	}

	static float SettleMul(DCO_AISKILL skill)
	{
		switch (skill)
		{
			case DCO_AISKILL.NOOB:        return 1.5;
			case DCO_AISKILL.ROOKIE:      return 1.25;
			case DCO_AISKILL.REGULAR:     return 1.0;
			case DCO_AISKILL.VETERAN:     return 0.9;
			case DCO_AISKILL.EXPERT:      return 0.8;
			case DCO_AISKILL.SPECIAL_OPS: return 0.75;
			case DCO_AISKILL.TERMINATOR:  return 0.7;
		}
		return 1.0;
	}

	static float GetSightMagnification(SCR_AICombatComponent combat)
	{
		if (!combat)
			return 1;

		BaseWeaponComponent weapon = combat.GetCurrentWeapon();
		if (!weapon || !weapon.GetSights())
			return 1;

		SCR_SightsZoomFOVInfo zoom = SCR_SightsZoomFOVInfo.Cast(weapon.GetSights().GetFOVInfo());
		if (!zoom)
			return 1;

		return Math.Max(1, zoom.GetCurrentZoom());
	}

	static vector ScatterExplosive(SCR_AIUtilityComponent utility, vector targetPos, bool grenadeLauncher, float extraMul = 1.0, float minSigma = 0)
	{
		if (!utility || !utility.m_OwnerEntity)
			return targetPos;

		float ratio = 0.08;
		float accuracy = DCO_AIConfigComponent.AccuracyOf(utility) * 3.6;
		if (grenadeLauncher)
		{
			ratio = 0.035;
			accuracy = DCO_AIConfigComponent.GLAccuracyOf(utility);
		}

		if (accuracy <= 0)
			return targetPos;

		IEntity self = utility.m_OwnerEntity;
		float sigma = vector.DistanceXZ(self.GetOrigin(), targetPos) * ratio
			* SkillSigma(DCO_AIConfigComponent.SkillOf(utility)) / accuracy;

		if (utility.m_ThreatSystem)
			sigma *= Math.Lerp(1.0, 2.3, Math.Clamp(utility.m_ThreatSystem.GetSuppressionMeasure(), 0, 1));

		if (utility.m_CombatComponent)
			sigma *= Math.Clamp(utility.m_CombatComponent.GetCurrentAimImprovement() / 2.5, 0.5, 1.0);

		ChimeraCharacter character = ChimeraCharacter.Cast(self);
		if (character && character.GetCharacterController())
		{
			ECharacterStance stance = character.GetCharacterController().GetStance();
			if (stance == ECharacterStance.PRONE)
				sigma *= 0.7;
			else if (stance == ECharacterStance.CROUCH)
				sigma *= 0.85;
		}

		sigma = Math.Max(sigma * extraMul, minSigma);
		if (sigma <= 0)
			return targetPos;

		vector pos = targetPos;
		pos[0] = pos[0] + Math.RandomGaussFloat(sigma, 0);
		pos[2] = pos[2] + Math.RandomGaussFloat(sigma, 0);		return pos;
	}
}