class DCO_AIGetUGLWeapon : AITaskScripted
{

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
		{
			DCO_Perf.Count("spec:gl_bt_no_ugl");
			DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(m_Utility.m_OwnerEntity), string.Format("spec_gl_bt unit=%1 step=get_weapon result=no_ugl_on_selected", DCO_Log.IdUnit(m_Utility.m_OwnerEntity)));
			return ENodeResult.FAIL;
		}

		if (!DCO_UGLUtility.HasUGLAmmo(m_Utility.m_OwnerEntity, weap, uglIdx))
		{
			DCO_Perf.Count("spec:gl_bt_no_ammo");
			DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(m_Utility.m_OwnerEntity), string.Format("spec_gl_bt unit=%1 step=get_weapon result=no_ammo", DCO_Log.IdUnit(m_Utility.m_OwnerEntity)));
			return ENodeResult.FAIL;
		}

		SetVariableOut("WeaponComponent", weap);
		SetVariableOut("MuzzleId", uglIdx);

		return ENodeResult.SUCCESS;
	}

	protected static ref TStringArray s_aVarsOut;
	protected static TStringArray GetVarsOut()
	{
		if (!s_aVarsOut)
			s_aVarsOut = { "WeaponComponent", "MuzzleId" };
		return s_aVarsOut;
	}
	override TStringArray GetVariablesOut() { return GetVarsOut(); }

	static override bool VisibleInPalette() { return true; }

	static override string GetOnHoverDescription() { return "DCO: find a weapon with a UGL and its muzzle index. FAIL if there is no UGL or no ammo."; }
}