class DCO_ClassAlias
{
	protected static ref map<string, string> s_mOld;

	static string Resolve(string className)
	{
		if (!s_mOld)
		{
			s_mOld = new map<string, string>();
			s_mOld.Set("CNH_CaptureArea", "DCO_CNH_CaptureArea");
			s_mOld.Set("CNH_CaptureAreaClass", "DCO_CNH_CaptureAreaClass");
			s_mOld.Set("CNH_CaptureManager", "DCO_CNH_CaptureManager");
			s_mOld.Set("CNH_CaptureManagerClass", "DCO_CNH_CaptureManagerClass");
			s_mOld.Set("Comamnder_AreaBaseMeshComponent", "DCO_AreaBaseMeshComponent");
			s_mOld.Set("Comamnder_AreaBaseMeshComponentClass", "DCO_AreaBaseMeshComponentClass");
			s_mOld.Set("ResupplyConfig", "DCO_ResupplyConfig");
			s_mOld.Set("SCR_AIDangerEvent_Killzone", "DCO_AIDangerEvent_Killzone");
			s_mOld.Set("SCR_AIDangerReaction_Killzone", "DCO_AIDangerReaction_Killzone");
			s_mOld.Set("SCR_AIFindPositionWithPrefab", "DCO_AIFindPositionWithPrefab");
			s_mOld.Set("SCR_AIGetCurrentWeaponComponent", "DCO_AIGetCurrentWeaponComponent");
			s_mOld.Set("SCR_AIGlobalAIMAttribute", "DCO_AIGlobalAIMAttribute");
			s_mOld.Set("SCR_AIGlobalAIMShakeAttribute", "DCO_AIGlobalAIMShakeAttribute");
			s_mOld.Set("SCR_AIGlobalAIMTimeAttribute", "DCO_AIGlobalAIMTimeAttribute");
			s_mOld.Set("SCR_AIGlobalDisembarkRuleAttribute", "DCO_AIGlobalDisembarkRuleAttribute");
			s_mOld.Set("SCR_AIGlobalDodgeChanceAttribute", "DCO_AIGlobalDodgeChanceAttribute");
			s_mOld.Set("SCR_AIGlobalDodgeCooldownAttribute", "DCO_AIGlobalDodgeCooldownAttribute");
			s_mOld.Set("SCR_AIGlobalDodgeMaxDistAttribute", "DCO_AIGlobalDodgeMaxDistAttribute");
			s_mOld.Set("SCR_AIGlobalDodgeScaleByPersonalityAttribute", "DCO_AIGlobalDodgeScaleByPersonalityAttribute");
			s_mOld.Set("SCR_AIGlobalDodgeSearchDistAttribute", "DCO_AIGlobalDodgeSearchDistAttribute");
			s_mOld.Set("SCR_AIGlobalDodgeShotThresholdAttribute", "DCO_AIGlobalDodgeShotThresholdAttribute");
			s_mOld.Set("SCR_AIGlobalDodgeShotWindowAttribute", "DCO_AIGlobalDodgeShotWindowAttribute");
			s_mOld.Set("SCR_AIGlobalGLAccuracyAttribute", "DCO_AIGlobalGLAccuracyAttribute");
			s_mOld.Set("SCR_AIGlobalGLUsageAttribute", "DCO_AIGlobalGLUsageAttribute");
			s_mOld.Set("SCR_AIGlobalGrenadeUsageAttribute", "DCO_AIGlobalGrenadeUsageAttribute");
			s_mOld.Set("SCR_AIGlobalLODModeAttribute", "DCO_AIGlobalLODModeAttribute");
			s_mOld.Set("SCR_AIGlobalMagicAmmoAttribute", "DCO_AIGlobalMagicAmmoAttribute");
			s_mOld.Set("SCR_AIGlobalPerceptionAttribute", "DCO_AIGlobalPerceptionAttribute");
			s_mOld.Set("SCR_AIGlobalSkillAttribute", "DCO_AIGlobalSkillAttribute");
			s_mOld.Set("SCR_AIGlobalSmokeUsageAttribute", "DCO_AIGlobalSmokeUsageAttribute");
			s_mOld.Set("SCR_AIGlobalSuppressionEffectAttribute", "DCO_AIGlobalSuppressionEffectAttribute");
			s_mOld.Set("SCR_AIGlobalTakeCoverChanceAttribute", "DCO_AIGlobalTakeCoverChanceAttribute");
			s_mOld.Set("SCR_AIGlobalVehicleDismountAttribute", "DCO_AIGlobalVehicleDismountAttribute");
			s_mOld.Set("SCR_AIGroupTacticsComponent", "DCO_GroupTacticsComponent");
			s_mOld.Set("SCR_AIGroupTacticsComponentClass", "DCO_GroupTacticsComponentClass");
			s_mOld.Set("SCR_AIIndividualAIMAttribute", "DCO_AIIndividualAIMAttribute");
			s_mOld.Set("SCR_AIIndividualAIMShakeAttribute", "DCO_AIIndividualAIMShakeAttribute");
			s_mOld.Set("SCR_AIIndividualAIMTimeAttribute", "DCO_AIIndividualAIMTimeAttribute");
			s_mOld.Set("SCR_AIIndividualDodgeChanceAttribute", "DCO_AIIndividualDodgeChanceAttribute");
			s_mOld.Set("SCR_AIIndividualDodgeCooldownAttribute", "DCO_AIIndividualDodgeCooldownAttribute");
			s_mOld.Set("SCR_AIIndividualDodgeMaxDistAttribute", "DCO_AIIndividualDodgeMaxDistAttribute");
			s_mOld.Set("SCR_AIIndividualDodgeScaleByPersonalityAttribute", "DCO_AIIndividualDodgeScaleByPersonalityAttribute");
			s_mOld.Set("SCR_AIIndividualDodgeSearchDistAttribute", "DCO_AIIndividualDodgeSearchDistAttribute");
			s_mOld.Set("SCR_AIIndividualDodgeShotThresholdAttribute", "DCO_AIIndividualDodgeShotThresholdAttribute");
			s_mOld.Set("SCR_AIIndividualDodgeShotWindowAttribute", "DCO_AIIndividualDodgeShotWindowAttribute");
			s_mOld.Set("SCR_AIIndividualGLAccuracyAttribute", "DCO_AIIndividualGLAccuracyAttribute");
			s_mOld.Set("SCR_AIIndividualGLUsageAttribute", "DCO_AIIndividualGLUsageAttribute");
			s_mOld.Set("SCR_AIIndividualGrenadeUsageAttribute", "DCO_AIIndividualGrenadeUsageAttribute");
			s_mOld.Set("SCR_AIIndividualLeadershipAttribute", "DCO_AIIndividualLeadershipAttribute");
			s_mOld.Set("SCR_AIIndividualMagicAmmoAttribute", "DCO_AIIndividualMagicAmmoAttribute");
			s_mOld.Set("SCR_AIIndividualPerceptionAttribute", "DCO_AIIndividualPerceptionAttribute");
			s_mOld.Set("SCR_AIIndividualPersonalityAttribute", "DCO_AIIndividualPersonalityAttribute");
			s_mOld.Set("SCR_AIIndividualSkillAttribute", "DCO_AIIndividualSkillAttribute");
			s_mOld.Set("SCR_AIIndividualSmokeUsageAttribute", "DCO_AIIndividualSmokeUsageAttribute");
			s_mOld.Set("SCR_AIIndividualSuppressionEffectAttribute", "DCO_AIIndividualSuppressionEffectAttribute");
			s_mOld.Set("SCR_AIIndividualTakeCoverChanceAttribute", "DCO_AIIndividualTakeCoverChanceAttribute");
			s_mOld.Set("SCR_AIRemoveItemItemFromInventory_InventoryCallback", "DCO_AIRemoveItemFromInventory_InventoryCallback");
			s_mOld.Set("SCR_AISquadDisembarkRuleAttribute", "DCO_AISquadDisembarkRuleAttribute");
			s_mOld.Set("SCR_AISquadVehicleDismountAttribute", "DCO_AISquadVehicleDismountAttribute");
			s_mOld.Set("SCR_AITakeItemFromInventory", "DCO_AITakeItemFromInventory");
			s_mOld.Set("SCR_AITakeItemFromInventory_InventoryCallback", "DCO_AITakeItemFromInventory_InventoryCallback");
			s_mOld.Set("SCR_CoverManagerComponent", "DCO_CoverManagerComponent");
			s_mOld.Set("SCR_CoverManagerComponentClass", "DCO_CoverManagerComponentClass");
			s_mOld.Set("SCR_PlayerScaledGroupSpawner", "DCO_PlayerScaledGroupSpawner");
			s_mOld.Set("SCR_PlayerScaledGroupSpawnerClass", "DCO_PlayerScaledGroupSpawnerClass");
		}
		string found;
		if (s_mOld.Find(className, found))
		{
			DCO_Perf.Count("class_alias:" + className);
			return found;
		}
		return className;
	}
}
