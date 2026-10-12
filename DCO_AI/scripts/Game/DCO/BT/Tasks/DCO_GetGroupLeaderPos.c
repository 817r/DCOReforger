class SCR_AIGetPositionAndRotationLeader: AITaskScripted
{
	protected SCR_AIGroup m_groupOwner;

    static override bool VisibleInPalette() {return true;}

	protected static ref TStringArray s_aVarsOut;
	protected static TStringArray GetVarsOut()
	{
		if (!s_aVarsOut)
			s_aVarsOut = { "Position", "Rotation" };
		return s_aVarsOut;
	}
    override array<string> GetVariablesOut()
    {
        return GetVarsOut();
    }

	override void OnInit(AIAgent owner)
	{
		m_groupOwner = SCR_AIGroup.Cast(owner);
		if (!m_groupOwner)
		{
			m_groupOwner = SCR_AIGroup.Cast(owner.GetParentGroup());
			if (!m_groupOwner)
			{
				SCR_AgentMustBeAIGroup(this, owner);
				return;
			}
		}
	}

    override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
    {
		IEntity entity = m_groupOwner.GetLeaderEntity();

		if (!entity)
			return ENodeResult.FAIL;

		SetVariableOut("Position",entity.GetOrigin());
		SetVariableOut("Rotation",entity.GetAngles());

		return ENodeResult.SUCCESS;
    }
};