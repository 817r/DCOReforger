[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CommanderPresetSaveAttribute : DCO_NameEditorAttribute
{
	override protected bool GetName(IEntity owner, out string outName)
	{
		if (!owner || !owner.FindComponent(AICommander_BaseComponent))
			return false;

		outName = string.Empty;
		return true;
	}

	override protected int NameKind()
	{
		return 1;
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CommanderPresetLoadAttribute : SCR_BaseFloatValueHolderEditorAttribute
{
	override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
	{
		m_aValues.Clear();

		array<string> names = {};
		string current = "none";
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (mgr)
		{
			mgr.DCO_GetPresetNames(names);
			AICommander_BaseComponent cmd = EditedCommander();
			if (cmd)
			{
				string label = mgr.DCO_ActivePresetLabel(cmd.GetCommanderUID());
				if (!label.IsEmpty())
					current = label;
			}
		}

		SCR_EditorAttributeFloatStringValueHolder keep = new SCR_EditorAttributeFloatStringValueHolder();
		keep.SetName("Keep (current: " + current + ")");
		keep.SetFloatValue(0);
		m_aValues.Insert(keep);

		for (int i = 0; i < names.Count(); i++)
		{
			SCR_EditorAttributeFloatStringValueHolder value = new SCR_EditorAttributeFloatStringValueHolder();
			value.SetName(names[i]);
			value.SetFloatValue(i + 1);
			m_aValues.Insert(value);
		}
		return super.GetEntries(outEntries);
	}

	protected AICommander_BaseComponent EditedCommander()
	{
		SCR_AttributesManagerEditorComponent attributesManager = SCR_AttributesManagerEditorComponent.Cast(SCR_AttributesManagerEditorComponent.GetInstance(SCR_AttributesManagerEditorComponent));
		if (!attributesManager)
			return null;

		array<Managed> items = {};
		attributesManager.GetEditedItems(items);
		foreach (Managed item : items)
		{
			AICommander_BaseComponent cmd = DCO_CommanderBaseAttribute.GetCommander(item);
			if (cmd)
				return cmd;
		}
		return null;
	}

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!DCO_CommanderBaseAttribute.GetCommander(item))
			return null;
		return SCR_BaseEditorAttributeVar.CreateFloat(0);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;

		int index = Math.Round(var.GetFloat());
		AICommander_BaseComponent cmd = DCO_CommanderBaseAttribute.GetCommander(item);
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (index <= 0 || !cmd || !mgr)
			return;

		array<string> names = {};
		mgr.DCO_GetPresetNames(names);
		if (index > names.Count())
			return;

		cmd.DCO_LoadPresetFromGM(names[index - 1]);
	}

	override bool IsSerializable()
	{
		return false;
	}
}
