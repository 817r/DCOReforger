[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_NameEditorAttribute : SCR_BaseEditorAttribute
{

	protected string m_sPendingName;

	protected bool GetName(IEntity owner, out string outName)
	{
		return false;
	}

	protected static IEntity GetOwnerEntity(Managed item)
	{
		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(item);
		if (!editable)
			return null;

		return editable.GetOwner();
	}

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		string currentName;
		if (!GetName(GetOwnerEntity(item), currentName))
			return null;

		return SCR_BaseEditorAttributeVar.CreateInt(0);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (var && var.GetInt() != 0 && manager)
			manager.DCO_RequestSetName(item, m_sPendingName);
	}

	override bool IsSerializable()
	{
		return false;
	}

	string StartTextEditing(SCR_AttributesManagerEditorComponent manager)
	{
		m_sPendingName = string.Empty;
		if (!manager)
			return m_sPendingName;

		array<Managed> items = {};
		manager.GetEditedItems(items);
		foreach (Managed item : items)
		{
			if (GetName(GetOwnerEntity(item), m_sPendingName))
				break;
		}

		return m_sPendingName;
	}

	void SetPendingName(string pendingName)
	{
		m_sPendingName = pendingName;
	}

	static void ApplyName(IEntity owner, string newName)
	{
		if (!owner || !Replication.IsServer())
			return;

		newName = newName.Trim();
		if (newName.Length() > 32)
			newName = newName.Substring(0, 32);

		AICommander_BaseComponent cmd = AICommander_BaseComponent.Cast(owner.FindComponent(AICommander_BaseComponent));
		if (cmd)
		{
			newName.Replace(";", "-");
			newName.Replace("|", "-");
			cmd.RenameCommanderFromGM(newName);
			return;
		}

		CMD_AICommanderObjectiveComponent obj = CMD_AICommanderObjectiveComponent.Cast(owner.FindComponent(CMD_AICommanderObjectiveComponent));
		if (obj)
			obj.SetObjectiveName(newName);
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CommanderNameAttribute : DCO_NameEditorAttribute
{
	override protected bool GetName(IEntity owner, out string outName)
	{
		if (!owner)
			return false;

		AICommander_BaseComponent cmd = AICommander_BaseComponent.Cast(owner.FindComponent(AICommander_BaseComponent));
		if (!cmd)
			return false;

		outName = cmd.GetCommanderUID();
		return true;
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ObjectiveNameAttribute : DCO_NameEditorAttribute
{
	override protected bool GetName(IEntity owner, out string outName)
	{
		if (!owner)
			return false;

		CMD_AICommanderObjectiveComponent obj = CMD_AICommanderObjectiveComponent.Cast(owner.FindComponent(CMD_AICommanderObjectiveComponent));
		if (!obj)
			return false;

		outName = obj.GetObjectiveName();
		return true;
	}
}

class DCO_TextEditorAttributeUIComponent : SCR_BaseEditorAttributeUIComponent
{
	protected SCR_EditBoxComponent m_EditBox;
	protected string m_sOriginal;

	override void Init(Widget w, SCR_BaseEditorAttribute attribute)
	{
		Widget editWidget = w.FindAnyWidget(m_sUiComponentName);
		if (!editWidget)
			return;

		m_EditBox = SCR_EditBoxComponent.Cast(editWidget.FindHandler(SCR_EditBoxComponent));
		if (!m_EditBox)
			return;

		super.Init(w, attribute);

		DCO_NameEditorAttribute nameAttribute = DCO_NameEditorAttribute.Cast(attribute);
		if (nameAttribute)
			m_sOriginal = nameAttribute.StartTextEditing(m_AttributeManager);

		m_EditBox.SetValue(m_sOriginal);
		m_EditBox.m_OnChanged.Insert(OnEditBoxChanged);
	}

	override void SetFromVar(SCR_BaseEditorAttributeVar var)
	{
		super.SetFromVar(var);

		if (!var || !m_EditBox || var.GetInt() != 0)
			return;

		m_EditBox.SetValue(m_sOriginal);
		DCO_NameEditorAttribute nameAttribute = DCO_NameEditorAttribute.Cast(GetAttribute());
		if (nameAttribute)
			nameAttribute.SetPendingName(m_sOriginal);
	}

	override void SetVariableToDefaultValue(SCR_BaseEditorAttributeVar var)
	{
		if (var)
			var.SetInt(0);

		if (m_EditBox)
			m_EditBox.SetValue(m_sOriginal);
	}

	protected void OnEditBoxChanged(SCR_EditBoxComponent editBox, string text)
	{
		DCO_NameEditorAttribute nameAttribute = DCO_NameEditorAttribute.Cast(GetAttribute());
		if (!nameAttribute)
			return;

		nameAttribute.SetPendingName(text);

		SCR_BaseEditorAttributeVar var = nameAttribute.GetVariable(true);
		int changed = text != m_sOriginal;
		if (var.GetInt() == changed)
			return;

		var.SetInt(changed);
		AttributeValueChanged();
	}

	override void HandlerDeattached(Widget w)
	{
		if (m_EditBox)
			m_EditBox.m_OnChanged.Remove(OnEditBoxChanged);

		super.HandlerDeattached(w);
	}
}

modded class SCR_AttributesManagerEditorComponent
{
	void DCO_RequestSetName(Managed item, string newName)
	{
		int itemId = Replication.FindItemId(item);
		if (itemId != -1)
			Rpc(DCO_RpcSetName, itemId, newName);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void DCO_RpcSetName(int itemId, string newName)
	{
		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(Replication.FindItem(itemId));
		if (editable)
			DCO_NameEditorAttribute.ApplyName(editable.GetOwner(), newName);
	}
}
