class DCO_AIGetUGLWeapon : AITaskScripted
{
	protected static const string PORT_WEAPON_COMPONENT = "WeaponComponent";

	protected SCR_AIUtilityComponent m_Utility;

	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		if (!m_Utility)
			m_Utility = SCR_AIUtilityComponent.Cast(owner.FindComponent(SCR_AIUtilityComponent));

		if (!m_Utility || !m_Utility.m_CombatComponent || !m_Utility.m_OwnerEntity)
			return ENodeResult.FAIL;

		BaseWeaponComponent weap;
		int selectedMuzzleId;
		m_Utility.m_CombatComponent.GetSelectedWeapon(weap, selectedMuzzleId);

		int uglIdx = DCO_UGLUtility.GetUGLMuzzleIndex(weap);
		if (uglIdx == -1)
			return ENodeResult.FAIL;

		if (!DCO_UGLUtility.HasUGLAmmo(m_Utility.m_OwnerEntity, weap, uglIdx))
			return ENodeResult.FAIL;

		SetVariableOut(PORT_WEAPON_COMPONENT, weap);
		SetVariableOut("MuzzleId", uglIdx);

		return ENodeResult.SUCCESS;
	}

	protected static ref TStringArray s_aVarsOut = { PORT_WEAPON_COMPONENT, "MuzzleId" };
	override TStringArray GetVariablesOut() { return s_aVarsOut; }

	static override bool VisibleInPalette() { return true; }

	static override string GetOnHoverDescription() { return "DCO: cari senjata dengan UGL + index muzzle-nya. FAIL kalau gak ada UGL / amunisi."; }
}