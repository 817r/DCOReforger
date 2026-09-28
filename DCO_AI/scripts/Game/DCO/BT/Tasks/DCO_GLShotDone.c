class DCO_AIGLShotDone : AITaskScripted
{
	protected SCR_AIUtilityComponent m_Utility;

	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		if (!owner)
			return ENodeResult.SUCCESS;

		if (!m_Utility)
			m_Utility = SCR_AIUtilityComponent.Cast(owner.FindComponent(SCR_AIUtilityComponent));

		BaseWeaponComponent weap;
		int selectedMuzzleId;
		if (m_Utility && m_Utility.m_CombatComponent)
			m_Utility.m_CombatComponent.GetSelectedWeapon(weap, selectedMuzzleId);

		DCO_UGLUtility.NotifyGLShotDone(owner.GetControlledEntity(), weap);

		return ENodeResult.SUCCESS;
	}

	static override bool VisibleInPalette() { return true; }

	static override string GetOnHoverDescription() { return "DCO: satu butir GL selesai. Kurangi jatah volley, lanjut atau tutup volley."; }
}