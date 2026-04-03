[ComponentEditorProps(category: "DCO", description: "Maintains group cohesion — pulls stragglers back to leader after delay")]
class DCO_GroupCohesionComponentClass : ScriptComponentClass {}

class DCO_GroupCohesionComponent : ScriptComponent
{
    [Attribute("8.0", UIWidgets.EditBox, "Radius tambahan per anggota (meter)")]
    protected float m_fRadiusPerMember;

    [Attribute("6.0", UIWidgets.EditBox, "Delay sebelum straggler dipanggil balik (detik)")]
    protected float m_fRecallDelay;

    [Attribute("5.0", UIWidgets.EditBox, "Interval cek cohesion (detik)")]
    protected float m_fCheckInterval;

    protected float m_fCheckTimer = 0.0;

    protected ref map<AIAgent, float> m_mOutOfRadiusTime;

    protected AIGroup m_Group;

    //--------------------------------------------------------------------
    override void OnPostInit(IEntity owner)
    {
        super.OnPostInit(owner);

        if (!Replication.IsServer())
            return;

        m_mOutOfRadiusTime = new map<AIAgent, float>();

        m_Group = AIGroup.Cast(owner);
        if (!m_Group)
        {
            Print("[DCO_Cohesion] Owner bukan AIGroup, component tidak aktif");
            return;
        }

        SetEventMask(owner, EntityEvent.FRAME);
    }

    //--------------------------------------------------------------------
    override void EOnFrame(IEntity owner, float timeSlice)
    {
        if (!Replication.IsServer())
            return;

        if (!m_Group)
            return;

        m_fCheckTimer += timeSlice;
        if (m_fCheckTimer < m_fCheckInterval)
            return;

        m_fCheckTimer = 0.0;

        CheckCohesion(m_fCheckInterval);
    }

    //--------------------------------------------------------------------
    protected void CheckCohesion(float dt)
    {
        AIAgent leader = m_Group.GetLeaderAgent();
        if (!leader)
            return;

        IEntity leaderEntity = leader.GetControlledEntity();
        if (!leaderEntity)
            return;

        vector leaderPos = leaderEntity.GetOrigin();

		array<AIAgent> agents = {};
        int memberCount = m_Group.GetAgents(agents);
        float maxRadius = m_fRadiusPerMember * memberCount;

        for (int i = 0; i < memberCount; i++)
        {
            AIAgent agent = agents[i];
            if (!agent || agent == leader)
                continue;

            IEntity agentEntity = agent.GetControlledEntity();
            if (!agentEntity)
                continue;

            float dist = vector.Distance(agentEntity.GetOrigin(), leaderPos);

            if (dist > maxRadius)
            {
                float timeOut = 0.0;
                m_mOutOfRadiusTime.Find(agent, timeOut);
                timeOut += dt;
                m_mOutOfRadiusTime.Set(agent, timeOut);

                if (timeOut >= m_fRecallDelay)
                {
                    TryRecallAgent(agent, leaderPos, maxRadius);
                }
            }
            else
            {
                if (m_mOutOfRadiusTime.Contains(agent))
                    m_mOutOfRadiusTime.Set(agent, 0.0);
            }
        }
    }

    //--------------------------------------------------------------------
	protected void TryRecallAgent(AIAgent agent, vector leaderPos, float radius)
	{
	    IEntity agentEntity = agent.GetControlledEntity();
	    if (!agentEntity)
	        return;
	
	    SCR_CharacterControllerComponent ctrl = SCR_CharacterControllerComponent.Cast(
	        agentEntity.FindComponent(SCR_CharacterControllerComponent));
	
	    if (ctrl)
	    {
	        if (ctrl.IsReloading() || ctrl.IsWeaponRaised())
	            return;
	    }
	
	    SCR_AIUtilityComponent utility = SCR_AIUtilityComponent.Cast(
	        agentEntity.FindComponent(SCR_AIUtilityComponent));
	    if (!utility)
	        return;
	
	    RandomGenerator rand = new RandomGenerator();
	    vector recallPos = rand.GenerateRandomPointInRadius(0, radius * 0.5, leaderPos, false);
	    recallPos[1] = GetGame().GetWorld().GetSurfaceY(recallPos[0], recallPos[2]);
	
	    SCR_AIMoveIndividuallyBehavior moveOrder = new SCR_AIMoveIndividuallyBehavior(
	        utility,
	        null,
	        recallPos,
	        SCR_AIMoveIndividuallyBehavior.PRIORITY_BEHAVIOR_MOVE_INDIVIDUALLY,
	        SCR_AIMoveIndividuallyBehavior.PRIORITY_LEVEL_PLAYER,
	        null,
	        radius * 0.4
	    );
	
	    utility.AddAction(moveOrder);
	
	    PrintFormat("[DCO_Cohesion] Recall issued ke agent, dist: %1m",
	        vector.Distance(agentEntity.GetOrigin(), leaderPos));
	
	    // Reset timer setelah recall di-issue
	    m_mOutOfRadiusTime.Set(agent, 0.0);
	}

    float GetCurrentMaxRadius()
    {
        if (!m_Group)
            return 0.0;

        return m_fRadiusPerMember * m_Group.GetAgentsCount();
    }

    //--------------------------------------------------------------------
    static DCO_GroupCohesionComponent GetFrom(IEntity entity)
    {
        if (!entity)
            return null;

        return DCO_GroupCohesionComponent.Cast(entity.FindComponent(DCO_GroupCohesionComponent));
    }
}