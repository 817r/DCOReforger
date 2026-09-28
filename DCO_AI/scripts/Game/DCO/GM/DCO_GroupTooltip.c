class DCO_GroupTooltipDetail : SCR_EntityTooltipDetail
{
	protected static const ResourceName TEXT_LAYOUT = "{93AF6A0EA7571B84}UI/layouts/Editor/Tooltips/TooltipPrefabs/TooltipPrefab_Text.layout";

	protected SCR_AIGroup m_Group;
	protected TextWidget m_Text;
	protected string m_sShown;

	void DCO_GroupTooltipDetail()
	{
		m_Layout = TEXT_LAYOUT;
		m_bShowLabel = true;
	}

	override bool NeedUpdate()
	{
		return true;
	}

	override bool InitDetail(SCR_EditableEntityComponent entity, Widget widget)
	{
		SCR_EditableGroupComponent editableGroup = SCR_EditableGroupComponent.Cast(entity);
		if (!editableGroup)
			return false;

		m_Group = editableGroup.GetAIGroupComponent();
		m_Text = TextWidget.Cast(widget);
		if (!m_Group || !m_Text || m_Group.DCO_GetRole() < 0)
			return false;

		m_sShown = string.Empty;
		return true;
	}

	override void UpdateDetail(SCR_EditableEntityComponent entity)
	{
		if (!m_Group || !m_Text)
			return;

		string text = BuildText();
		if (text == m_sShown)
			return;

		m_sShown = text;
		m_Text.SetText(text);
	}

	protected string BuildText()
	{
		return string.Empty;
	}
}

class DCO_GroupRoleTooltipDetail : DCO_GroupTooltipDetail
{
	void DCO_GroupRoleTooltipDetail()
	{
		m_sDisplayName = "Role";
	}

	override protected string BuildText()
	{
		switch (m_Group.DCO_GetRole())
		{
			case DCO_EGroupCapability.MOTORIZED:	return "Motorized";
			case DCO_EGroupCapability.ARMOR:		return "Armor";
			case DCO_EGroupCapability.MORTAR:		return "Mortar team";
			case DCO_EGroupCapability.TRANSPORT:	return "Transport team";
		}
		return "Infantry";
	}
}

class DCO_GroupCapsTooltipDetail : DCO_GroupTooltipDetail
{
	void DCO_GroupCapsTooltipDetail()
	{
		m_sDisplayName = "Capabilities";
	}

	override protected string BuildText()
	{
		int caps = m_Group.DCO_GetCaps();
		array<string> parts = {};
		if (caps & 1)
			parts.Insert("AT");
		if (caps & 2)
			parts.Insert("MG");
		if (caps & 4)
			parts.Insert("Medic");
		if (caps & 8)
			parts.Insert("Radio");
		if (parts.IsEmpty())
			return "Rifles only";
		return SCR_StringHelper.Join(", ", parts);
	}
}

[BaseContainerProps(), SCR_BaseContainerCustomTitleEnum(EEditableEntityType, "m_EntityType")]
modded class SCR_EntityTooltipDetailType
{
	protected bool m_bDCO_Added;

	override bool CreateDetailType(EEditableEntityType type, Widget parent, SCR_EditableEntityComponent entity, out bool showImage = true)
	{
		if (!m_bDCO_Added && m_EntityType == EEditableEntityType.GROUP && m_aDetails)
		{
			m_bDCO_Added = true;
			m_aDetails.Insert(new DCO_GroupRoleTooltipDetail());
			m_aDetails.Insert(new DCO_GroupCapsTooltipDetail());
		}
		return super.CreateDetailType(type, parent, entity, showImage);
	}
}
