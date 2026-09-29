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
			case DCO_AISKILL.NOOB:        return 3;
			case DCO_AISKILL.ROOKIE:      return 2.2;
			case DCO_AISKILL.REGULAR:     return 1.7;
			case DCO_AISKILL.VETERAN:     return 1.3;
			case DCO_AISKILL.EXPERT:      return 0.65;
			case DCO_AISKILL.SPECIAL_OPS: return 0.2;
			case DCO_AISKILL.TERMINATOR:  return 0;
		}
		return 1.7;
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

	static vector ScatterExplosive(SCR_AIUtilityComponent utility, vector targetPos, bool grenadeLauncher)
	{
		if (!utility || !utility.m_OwnerEntity || !utility.m_DCOConfig)
			return targetPos;

		float ratio = 0.08;
		float accuracy = utility.m_DCOConfig.GetAccuracy();
		if (grenadeLauncher)
		{
			ratio = 0.035;
			accuracy = utility.m_DCOConfig.GetGLAccuracy();
		}

		if (accuracy <= 0)
			return targetPos;

		IEntity self = utility.m_OwnerEntity;
		float sigma = vector.DistanceXZ(self.GetOrigin(), targetPos) * ratio
			* SkillSigma(utility.m_DCOConfig.GetAISkill()) / (1.7 * accuracy);

		if (utility.m_ThreatSystem)
			sigma *= Math.Lerp(1.0, 1.85, Math.Clamp(utility.m_ThreatSystem.GetSuppressionMeasure(), 0, 1));

		if (utility.m_CombatComponent)
			sigma *= Math.Clamp(utility.m_CombatComponent.GetCurrentAimImprovement(), 0.5, 1.0);

		ChimeraCharacter character = ChimeraCharacter.Cast(self);
		if (character && character.GetCharacterController())
		{
			ECharacterStance stance = character.GetCharacterController().GetStance();
			if (stance == ECharacterStance.PRONE)
				sigma *= 0.7;
			else if (stance == ECharacterStance.CROUCH)
				sigma *= 0.85;
		}

		if (sigma <= 0)
			return targetPos;

		vector pos = targetPos;
		pos[0] = pos[0] + Math.RandomGaussFloat(sigma, 0);
		pos[2] = pos[2] + Math.RandomGaussFloat(sigma, 0);		return pos;
	}
}