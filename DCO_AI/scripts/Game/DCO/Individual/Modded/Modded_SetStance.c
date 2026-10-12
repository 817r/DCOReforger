modded class SCR_AISetStance : AITaskScripted
{
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		if (!m_CharacterController)
			return ENodeResult.FAIL;

		ECharacterStance stance;
		if (!GetVariableIn(STANCE_PORT, stance))
			stance = m_eStance;

		AIGroup group = owner.GetParentGroup();
		if (group)
		{
			SCR_AIGroupInfoComponent groupInfoComp = SCR_AIGroupInfoComponent.Cast(group.FindComponent(SCR_AIGroupInfoComponent));
			if (groupInfoComp)
				stance = groupInfoComp.GetAllowedStance(stance);
		}

		SCR_AICharacterStanceSettingBase stanceSetting = null;
		if (m_SettingsComp)
			stanceSetting = SCR_AICharacterStanceSettingBase.Cast(m_SettingsComp.GetCurrentSetting(SCR_AICharacterStanceSettingBase));
		if (stanceSetting)
			stance = stanceSetting.GetStance(stance);

		if (stance == ECharacterStance.PRONE && !DCO_CQC.ProneRoll(owner.GetControlledEntity()))
			stance = ECharacterStance.CROUCH;

		if (stance != ECharacterStance.STAND && DCO_Nav.Busy(m_CharacterController, "bt_stance"))
			return ENodeResult.SUCCESS;

		DCO_Stance.Set(m_CharacterController, stance, "bt");
		return ENodeResult.SUCCESS;
	}
}
