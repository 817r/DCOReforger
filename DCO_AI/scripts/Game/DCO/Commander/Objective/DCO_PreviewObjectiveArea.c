[ComponentEditorProps(category: "GameScripted/Area Mesh", description: "")]
class Comamnder_AreaBaseMeshComponentClass : SCR_BaseAreaMeshComponentClass
{
}

class Comamnder_AreaBaseMeshComponent : SCR_BaseAreaMeshComponent
{
	CMD_AICommanderObjectiveComponent m_MilitaryBaseComponent;

	override float GetRadius()
	{
		return m_MilitaryBaseComponent.GetRadius();
	}

	override void EOnInit(IEntity owner)
	{
		m_MilitaryBaseComponent = CMD_AICommanderObjectiveComponent.Cast(GetOwner().FindComponent(CMD_AICommanderObjectiveComponent));

		if (GetGame().InPlayMode() && !owner.FindComponent(SCR_EditableEntityComponent))
			return;

		if (!m_MilitaryBaseComponent)
		{
			Debug.Error2("SCR_MilitaryBaseAreaMeshComponent", "SCR_MilitaryBaseComponent not found on owner entity!");
			return;
		}

		GenerateAreaMesh();
	}
}
