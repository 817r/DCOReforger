[ComponentEditorProps(category: "GameScripted/Commander", description: "DCO logistics hub")]
class DCO_LogisticsHubComponentClass : ScriptComponentClass {}

class DCO_LogisticsHubComponent : ScriptComponent
{
	[Attribute("", UIWidgets.EditBox, "Faction pemilik hub. Kosong = dipakai Commander terdekat (faction apa pun).")]
	protected FactionKey m_sFactionKey;

	protected static ref array<DCO_LogisticsHubComponent> s_aHubs = {};

	static array<DCO_LogisticsHubComponent> GetHubs()	{ return s_aHubs; }
	FactionKey GetFactionKey()							{ return m_sFactionKey; }
	void SetFactionKey(FactionKey fk)					{ m_sFactionKey = fk; }

	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if (!s_aHubs.Contains(this))
			s_aHubs.Insert(this);
	}

	override void OnDelete(IEntity owner)
	{
		s_aHubs.RemoveItem(this);
		super.OnDelete(owner);
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_LogisticsHubFactionAttribute : DCO_CommanderFactionAttribute
{
	protected static DCO_LogisticsHubComponent GetHub(Managed item)
	{
		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(item);
		if (!editable || !editable.GetOwner())
			return null;
		return DCO_LogisticsHubComponent.Cast(editable.GetOwner().FindComponent(DCO_LogisticsHubComponent));
	}

	override protected void GetPresetLabels(notnull out array<string> outLabels)
	{
		super.GetPresetLabels(outLabels);
		outLabels.InsertAt("Auto (nearest commander)", 0);
	}

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_LogisticsHubComponent hub = GetHub(item);
		if (!hub)
			return null;

		array<FactionKey> keys = {};
		GetFactionKeys(keys);
		return SCR_BaseEditorAttributeVar.CreateInt(keys.Find(hub.GetFactionKey()) + 1);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		DCO_LogisticsHubComponent hub = GetHub(item);
		if (!hub || !var)
			return;

		array<FactionKey> keys = {};
		GetFactionKeys(keys);
		int index = var.GetInt() - 1;
		if (keys.IsIndexValid(index))
			hub.SetFactionKey(keys[index]);
		else
			hub.SetFactionKey("");
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CommanderLogiBoolAttribute : SCR_BaseEditorAttribute
{
	protected bool Get(AICommander_BaseComponent cmd) { return false; }
	protected void Set(AICommander_BaseComponent cmd, bool value) {}

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		AICommander_BaseComponent cmd = DCO_CommanderBaseAttribute.GetCommander(item);
		if (!cmd)
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(Get(cmd));
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		AICommander_BaseComponent cmd = DCO_CommanderBaseAttribute.GetCommander(item);
		if (cmd && var)
			Set(cmd, var.GetBool());
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CommanderLogiMedevacAttribute : DCO_CommanderLogiBoolAttribute
{
	override protected bool Get(AICommander_BaseComponent cmd) { return cmd.IsLogiMedevac(); }
	override protected void Set(AICommander_BaseComponent cmd, bool value) { cmd.SetLogiMedevac(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CommanderLogiExtractAttribute : DCO_CommanderLogiBoolAttribute
{
	override protected bool Get(AICommander_BaseComponent cmd) { return cmd.IsLogiExtract(); }
	override protected void Set(AICommander_BaseComponent cmd, bool value) { cmd.SetLogiExtract(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CommanderLogiResupplyAttribute : DCO_CommanderLogiBoolAttribute
{
	override protected bool Get(AICommander_BaseComponent cmd) { return cmd.IsLogiResupply(); }
	override protected void Set(AICommander_BaseComponent cmd, bool value) { cmd.SetLogiResupply(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CommanderLogiEscortAttribute : DCO_CommanderLogiBoolAttribute
{
	override protected bool Get(AICommander_BaseComponent cmd) { return cmd.IsLogiEscort(); }
	override protected void Set(AICommander_BaseComponent cmd, bool value) { cmd.SetLogiEscort(value); }
}
