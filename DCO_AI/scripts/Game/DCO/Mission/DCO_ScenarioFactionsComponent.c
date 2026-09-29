[ComponentEditorProps(category: "GameScripted/DCO", description: "Scenario: faction mana yang boleh dipilih pemain. Taruh di satu entity di world.")]
class DCO_ScenarioFactionsComponentClass : ScriptComponentClass
{
}

class DCO_ScenarioFactionsComponent : ScriptComponent
{
	[Attribute("US", UIWidgets.Auto, "Faction key yang boleh dipilih pemain. Sisanya gak playable.")]
	protected ref array<string> m_aPlayableFactions;


	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);
	}

	override void EOnInit(IEntity owner)
	{
		if (Replication.IsServer() && GetGame().InPlayMode())
			GetGame().GetCallqueue().CallLater(Apply, 1000, false);
	}

	protected void Apply()
	{
		FactionManager manager = GetGame().GetFactionManager();
		if (!manager)
			return;

		array<Faction> factions = {};
		manager.GetFactionsList(factions);
		foreach (Faction f : factions)
		{
			SCR_Faction faction = SCR_Faction.Cast(f);
			if (faction)
				faction.SetIsPlayable(m_aPlayableFactions.Contains(faction.GetFactionKey()));
		}
	}
}
