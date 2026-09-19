// === ADDED: node BT buat tree GL (pengganti SCR_AIGetWeaponOfType di tree RPG) ===
// SCR_AIGetWeaponOfType gak bisa dipakai buat UGL karena UGL itu WT_RIFLE + MT_UGLMuzzle.
// Output:
//   WeaponComponent -> senjata yang punya UGL (sambung ke SCR_AISwitchWeapon)
//   MuzzleId        -> index muzzle UGL (sambung ke port muzzle SCR_AISwitchWeapon kalau ada)
// FAIL kalau gak ada UGL atau gak ada amunisinya -> tree GL berhenti sebelum switch.
class DCO_AIGetUGLWeapon : AITaskScripted
{
	protected static const string PORT_WEAPON_COMPONENT = "WeaponComponent";
	protected static const string PORT_MUZZLE_ID        = "MuzzleId";
	
	protected SCR_AIUtilityComponent m_Utility;
	
	//------------------------------------------------------------------------------------------------
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		// Lazy getter -- field initializer / OnInit bisa dapet null
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
		SetVariableOut(PORT_MUZZLE_ID, uglIdx);
		
		return ENodeResult.SUCCESS;
	}
	
	//------------------------------------------------------------------------------------------------
	protected static ref TStringArray s_aVarsOut = { PORT_WEAPON_COMPONENT, PORT_MUZZLE_ID };
	override TStringArray GetVariablesOut() { return s_aVarsOut; }
	
	static override bool VisibleInPalette() { return true; }
	
	static override string GetOnHoverDescription() { return "DCO: cari senjata dengan UGL + index muzzle-nya. FAIL kalau gak ada UGL / amunisi."; }
}