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
	
	//! JANGAN dijadiin field initializer (dulu: AIGroup group = AIGroup.Cast(GetOwner());).
	//! Initializer dievaluasi pas construct, sebelum owner ke-assign, jadi nilainya
	//! dijamin null selamanya dan ShouldReturnToFormation() pasti null-deref.
	protected AIGroup m_Group;
	
	//--------------------------------------------------------------------------------------------
	protected AIGroup GetGroup()
	{
		if (!m_Group)
			m_Group = AIGroup.Cast(GetOwner());
		
		return m_Group;
	}
	
	//--------------------------------------------------------------------------------------------
	//! Jarak efektif dari Squad Leader sebelum anggota nyoba regroup.
	float GetCohesionDistance()
	{
		return m_fCohessionDistance * m_fCohessionDistanceMult;
	}
	
	//--------------------------------------------------------------------------------------------
	bool ShouldReturnToFormation(vector who)
	{
		AIGroup group = GetGroup();
		
		if (!group)
			return false;
		
		IEntity leader = group.GetLeaderEntity();
		
		if (!leader)
			return false;
		
		return vector.Distance(leader.GetOrigin(), who) > GetCohesionDistance();
	}
	
	//--------------------------------------------------------------------------------------------
	bool GroupCapableOf(DCO_EAIGroupCapabilities cap)
	{
		return m_fGroupCapabilities & cap;
	}
	
	//--------------------------------------------------------------------------------------------
	void AddUnitState(DCO_EAIGroupCapabilities state)
	{
		m_fGroupCapabilities = m_fGroupCapabilities | state;
	}

	//--------------------------------------------------------------------------------------------
	void RemoveUnitState(DCO_EAIGroupCapabilities state)
	{
		if (GroupCapableOf(state))
			m_fGroupCapabilities = m_fGroupCapabilities & ~state;
	}
}