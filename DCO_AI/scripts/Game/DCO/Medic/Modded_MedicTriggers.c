modded class SCR_CharacterDamageManagerComponent
{
	override void OnDamageEffectAdded(notnull SCR_DamageEffect dmgEffect)
	{
		super.OnDamageEffectAdded(dmgEffect);

		if (dmgEffect.GetDamageType() == EDamageType.BLEEDING && !dmgEffect.GetAffectedHitZone().IsProxy() && GetState() != EDamageState.DESTROYED)
			DCO_MedicDispatcher.Notify(GetOwner(), false);
	}

	protected override void OnDamageStateChanged(EDamageState newState, EDamageState previousDamageState, bool isJIP)
	{
		super.OnDamageStateChanged(newState, previousDamageState, isJIP);
		if (newState == EDamageState.DESTROYED && previousDamageState != EDamageState.DESTROYED && Replication.IsServer())
			DCO_Rearm.RegisterCorpse(GetOwner());
	}
}

modded class SCR_ChimeraAIAgent
{
	override void OnLifeStateChanged(ECharacterLifeState previousLifeState, ECharacterLifeState newLifeState)
	{
		super.OnLifeStateChanged(previousLifeState, newLifeState);

		if (newLifeState == ECharacterLifeState.INCAPACITATED)
			DCO_MedicDispatcher.Notify(GetControlledEntity(), true);
		else if (newLifeState == ECharacterLifeState.DEAD)
		{
			DCO_Rearm.RegisterCorpse(GetControlledEntity());
			DCO_Awareness.OnDeath(GetControlledEntity());
			DCO_Recovery.OnDeath(this);
			DCO_MoraleEvents.OnDeath(GetControlledEntity());
			DCO_IndLearn.OnDeath(GetControlledEntity());
			DCO_Events.Killed(GetControlledEntity());
		}
	}
}

modded class SCR_AIFindMedic
{
	protected override AIAgent FindAgentMedic(IEntity entToHeal, notnull array<AIAgent> agents, notnull array<AIAgent> agentsToExclude, bool excludeDrivers = false)
	{
		array<AIAgent> healthy = {};
		int skipped;
		foreach (AIAgent a : agents)
		{
			IEntity e = a.GetControlledEntity();
			if (e && e != entToHeal && DCO_Util.IsBleeding(e))
			{
				skipped++;
				continue;
			}
			healthy.Insert(a);
		}
		AIAgent medic = super.FindAgentMedic(entToHeal, healthy, agentsToExclude, excludeDrivers);
		if (skipped > 0)
		{
			DCO_Perf.CountN("medic:skip_bleeding_helper", skipped);
			if (DCO_Log.On(DCO_ELogChannel.MEDIC, DCO_ELogLevel.DEBUG))
				DCO_Log.Write(DCO_ELogChannel.MEDIC, DCO_ELogLevel.DEBUG, "-", string.Format("medic_pick_skip_bleeding patient=%1 skipped=%2 picked=%3", DCO_Log.IdUnit(entToHeal), skipped, DCO_Log.IdUnit(DCO_AgentEntity(medic))));
		}
		return medic;
	}

	protected IEntity DCO_AgentEntity(AIAgent a)
	{
		if (!a)
			return null;
		return a.GetControlledEntity();
	}
}
