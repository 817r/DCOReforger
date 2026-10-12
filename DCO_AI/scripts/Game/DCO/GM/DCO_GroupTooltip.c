class DCO_GroupTooltipDetail : SCR_EntityTooltipDetail
{

	protected SCR_AIGroup m_Group;
	protected TextWidget m_Text;
	protected string m_sShown;

	void DCO_GroupTooltipDetail()
	{
		m_Layout = "{93AF6A0EA7571B84}UI/layouts/Editor/Tooltips/TooltipPrefabs/TooltipPrefab_Text.layout";
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
		if (!m_Group || !m_Text)
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
		if (m_Group.DCO_GetRole() < 0)
			return "No commander";
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
		if (m_Group.DCO_GetRole() < 0)
			return "-";
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

class DCO_GroupLODTooltipDetail : DCO_GroupTooltipDetail
{
	void DCO_GroupLODTooltipDetail()
	{
		m_sDisplayName = "LOD";
	}

	override bool InitDetail(SCR_EditableEntityComponent entity, Widget widget)
	{
		SCR_EditableGroupComponent editableGroup = SCR_EditableGroupComponent.Cast(entity);
		if (!editableGroup)
			return false;

		m_Group = editableGroup.GetAIGroupComponent();
		m_Text = TextWidget.Cast(widget);
		if (!m_Group || !m_Text || m_Group.DCO_GetLODInfo() < 0)
			return false;

		m_sShown = string.Empty;
		return true;
	}

	override protected string BuildText()
	{
		int info = m_Group.DCO_GetLODInfo();
		if (info < 0)
			return string.Empty;

		string mode = "Vanilla";
		if (info % 4 == DCO_EAILODMode.PREVENT_MAX_LOD)
			mode = "Prevent max";
		else if (info % 4 == DCO_EAILODMode.FULL_DETAIL)
			mode = "Full detail";

		string source = "default";
		if (info / 4 == 1)
			source = "commander";
		else if (info / 4 == 2)
			source = "override";
		return mode + " (" + source + ")";
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
			m_aDetails.Insert(new DCO_GroupTaskTooltipDetail(false));
			m_aDetails.Insert(new DCO_GroupTaskTooltipDetail(true));
			m_aDetails.Insert(new DCO_GroupLODTooltipDetail());
			m_aDetails.Insert(new DCO_GroupThreatTooltipDetail());
		}
		if (!m_bDCO_Added && m_EntityType == EEditableEntityType.CHARACTER && m_aDetails)
		{
			m_bDCO_Added = true;
			m_aDetails.Insert(new DCO_CharStatsTooltipDetail(false));
			m_aDetails.Insert(new DCO_CharStatsTooltipDetail(true));
		}
		return super.CreateDetailType(type, parent, entity, showImage);
	}
}
