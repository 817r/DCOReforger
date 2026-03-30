[ComponentEditorProps(category: "GameScripted/AI/AICommander", description: "Component for AI Commander Objective")]
class CMD_AICommanderObjectiveComponentClass : ScriptComponentClass
{
}

class CMD_AICommanderObjectiveComponent : ScriptComponent
{
	[Attribute("30.0", UIWidgets.Slider, "Radius of the Objective", "1.0 200.0 1.0")]
	protected float m_fRadius;
	
	ref map<FactionKey, CMD_EObjectiveState> m_mObjectiveState = new map<FactionKey, CMD_EObjectiveState>();
	
	IEntity m_OwnerEntity;
	
	float GetRadius()
	{
		return m_fRadius;
	}
	
	protected void InitializeObjective()
	{
		AICommander_ManagerComponent.GetInstance().RegisterObjective(this);
		for(int i = 0; i < AICommander_ManagerComponent.GetInstance().m_aAvailableFactions.Count(); i++)
		{
			m_mObjectiveState.Insert(AICommander_ManagerComponent.GetInstance().m_aAvailableFactions[i], CMD_EObjectiveState.PENDING);
		}
		
		for(int i = 0; i < m_mObjectiveState.Count(); i++)
		{
			Print(m_mObjectiveState.GetKey(i) + " < Fac | State > " + m_mObjectiveState.GetElement(i));
		}
		
		
	}
	
	void SetObjectiveState(FactionKey fk, CMD_EObjectiveState state)
	{
		for(int i = 0; i < m_mObjectiveState.Count(); i++)
		{
			if (fk == m_mObjectiveState.GetKey(i))
			{
				m_mObjectiveState.GetElement(i) = state;
			}
		}
	}
	
	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		m_OwnerEntity = owner;
		SetEventMask(owner, EntityEvent.INIT);
	}
	
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		InitializeObjective();
	}
}