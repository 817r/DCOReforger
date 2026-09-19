// === ADDED: node BT penutup tree GL (satu butir selesai) ===
// Taruh di AKHIR tree GL, setelah Reload dan sebelum Return State SUCCESS.
// Ngurangin jatah volley di DCO_UGLUtility. Kalau jatah + amunisi masih ada, commit
// diperpanjang -> ResolveFireTree tetap ngembaliin FIRE_TREE_GL -> butir berikutnya.
// Kalau habis, volley ditutup dan cooldown mulai jalan.
// (Menggantikan DCO_AIEndGLCommit dari versi sebelumnya.)
class DCO_AIGLShotDone : AITaskScripted
{
	protected SCR_AIUtilityComponent m_Utility;
	
	//------------------------------------------------------------------------------------------------
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		if (!owner)
			return ENodeResult.SUCCESS;
		
		// Lazy getter -- field initializer / OnInit bisa dapet null
		if (!m_Utility)
			m_Utility = SCR_AIUtilityComponent.Cast(owner.FindComponent(SCR_AIUtilityComponent));
		
		BaseWeaponComponent weap;
		int selectedMuzzleId;
		if (m_Utility && m_Utility.m_CombatComponent)
			m_Utility.m_CombatComponent.GetSelectedWeapon(weap, selectedMuzzleId);
		
		// weap null -> NotifyGLShotDone nutup volley (anggap gak bisa lanjut)
		DCO_UGLUtility.NotifyGLShotDone(owner.GetControlledEntity(), weap);
		
		return ENodeResult.SUCCESS;
	}
	
	//------------------------------------------------------------------------------------------------
	static override bool VisibleInPalette() { return true; }
	
	static override string GetOnHoverDescription() { return "DCO: satu butir GL selesai. Kurangi jatah volley, lanjut atau tutup volley."; }
}