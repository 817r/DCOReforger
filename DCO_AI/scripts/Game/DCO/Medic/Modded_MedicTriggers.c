modded class SCR_CharacterDamageManagerComponent
{
	override void OnDamageEffectAdded(notnull SCR_DamageEffect dmgEffect)
	{
		super.OnDamageEffectAdded(dmgEffect);

		if (dmgEffect.GetDamageType() == EDamageType.BLEEDING && !dmgEffect.GetAffectedHitZone().IsProxy() && GetState() != EDamageState.DESTROYED)
			DCO_MedicDispatcher.Notify(GetOwner(), false);
	}
}

modded class SCR_ChimeraAIAgent
{
	override void OnLifeStateChanged(ECharacterLifeState previousLifeState, ECharacterLifeState newLifeState)
	{
		super.OnLifeStateChanged(previousLifeState, newLifeState);

		if (newLifeState == ECharacterLifeState.INCAPACITATED)
			DCO_MedicDispatcher.Notify(GetControlledEntity(), true);
	}
}
