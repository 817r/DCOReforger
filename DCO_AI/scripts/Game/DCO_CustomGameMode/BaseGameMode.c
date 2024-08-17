class AOData
{
	ref array<IEntity> prefabSlot = new array<IEntity>;
	missionType type = missionType.UNDEFINED;
	float missionRadius = 20;
	int numberOfGroupSpawned = 1;
}

class MissionModuleComponentClass : ScriptComponentClass
{
	[Attribute(category: "AO Properties", desc: "Size of Area Operation", defvalue: "50.0", params: "0 inf")]
	protected float AORadius;
	
	[Attribute(category: "AO Properties", desc: "Intel Value", defvalue: "10.0", params: "0 inf")]
	protected int AOValue;
	
	[Attribute(missionType.AREA_CAPTURE.ToString(), category: "AO Properties", desc: "Type Of Mission in This AO", uiwidget: UIWidgets.ComboBox, enums: ParamEnumArray.FromEnum(missionType))]
	protected missionType m_eMissionType;
	
	[Attribute(category: "Spawner", desc: "Type Of Group That can be spawned here", uiwidget: UIWidgets.ResourceNamePicker)]
	protected ref array<ResourceName> spawnedGroup;
	
	ResourceName GetRandomSpawnedGroup()
	{
		return spawnedGroup.Get(Math.RandomInt(0, spawnedGroup.Count() - 1));
	}
	
	float getAORadius()
	{
		return AORadius;
	}
}

class MissionModuleComponent : ScriptComponent
{
#ifdef WORKBENCH

	[Attribute("0", desc: "Draw visualisation of bounds", category: "Debug")]
	protected bool m_bShowDebugShape;

	protected ref Shape m_DebugShape;
	protected int m_iDebugShapeColor = Color.BLUE;
#endif
	
	override void EOnInit(IEntity owner)
	{
#ifdef WORKBENCH
		if (m_bShowDebugShape)
			DrawDebugShape();
#endif
		
		MissionModuleComponentClass prefabData = MissionModuleComponentClass.Cast(GetComponentData(GetOwner()));
		if (!prefabData)
			return;
		
	}
	
#ifdef WORKBENCH
	//------------------------------------------------------------------------------------------------
	protected void DrawDebugShape()
	{
		MissionModuleComponentClass prefabData = MissionModuleComponentClass.Cast(GetComponentData(GetOwner()));
		if (!prefabData)
			return;

		vector transform[4];
		IEntity owner = GetOwner();
		owner.GetTransform(transform);

		int shapeFlags = ShapeFlags.WIREFRAME;
		m_DebugShape = Shape.CreateSphere(m_iDebugShapeColor, shapeFlags, owner.GetOrigin(), prefabData.getAORadius());

		m_DebugShape.SetMatrix(transform);
	}

	//------------------------------------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (m_bShowDebugShape && m_DebugShape)
		{
			vector transform[4];
			owner.GetTransform(transform);
			m_DebugShape.SetMatrix(transform);
		}
	}

	//------------------------------------------------------------------------------------------------
	override event void _WB_AfterWorldUpdate(IEntity owner, float timeSlice)
	{
		if (m_bShowDebugShape && m_DebugShape)
		{
			vector transform[4];
			owner.GetTransform(transform);
			m_DebugShape.SetMatrix(transform);
		}
	}
#endif
}
