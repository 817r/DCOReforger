[ComponentEditorProps(category: "GameScripted/Group")]
class DCO_GroupConfigComponentClass : ScriptComponentClass
{

}

class DCO_GroupConfigComponent : ScriptComponent
{
	[Attribute("0", UIWidgets.Flags, "What is this group Capable of", category: "Capabilities and Role", enums: ParamEnumArray.FromEnum(DCO_EAIGroupCapabilities))]
	protected DCO_EAIGroupCapabilities m_fGroupCapabilities;
	
	[Attribute("50", UIWidgets.Auto, "What is the max distance from SL before attempt to Regroup", category: "Group Cohession")]
	protected float m_fCohessionDistance;
	
	[Attribute("1", UIWidgets.Auto, "What is the Multiplier of Cohession Distance", category: "Group Cohession", params: "0.01 5 0.01")]
	protected float m_fCohessionDistanceMult;
	
	AIGroup group = AIGroup.Cast(GetOwner());
	
	bool ShouldReturnToFormation(vector who)
	{
		return vector.Distance(group.GetLeaderEntity().GetOrigin(), who) > m_fCohessionDistance * m_fCohessionDistanceMult;
	}
	
	bool GroupCapableOf(DCO_EAIGroupCapabilities cap)
	{
		return m_fGroupCapabilities & cap;
	}
	
	void AddUnitState(DCO_EAIGroupCapabilities state)
	{
		m_fGroupCapabilities = m_fGroupCapabilities | state;
	}

	void RemoveUnitState(DCO_EAIGroupCapabilities state)
	{
		if (GroupCapableOf(state))
			m_fGroupCapabilities = m_fGroupCapabilities & ~state;
	}
}