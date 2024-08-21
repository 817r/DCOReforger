class DCO_AIFortification : AITaskScripted
{	
	ref map<string, string> m_mTypenameToResourceName;
	
	static const string PORT_BUILDING_TYPE = "Building Name";
	
	[Attribute("", UIWidgets.EditBox, "Name of magazine well" )]
	protected string m_BuildingType;
	
	protected vector m_ResultVector;
	
	private string m_Building;
	
	protected static ref TStringArray s_aVarsIn = {
		PORT_BUILDING_TYPE
	};
	override TStringArray GetVariablesIn()
    {
        return s_aVarsIn;
    }
	
	override void OnInit(AIAgent owner)
	{
		m_mTypenameToResourceName = new map<string, string>;
		m_mTypenameToResourceName.Insert("SandbagBarrier","{C1A9D4CA4756B0A6}Prefabs/Compositions/Slotted/SlotFlatSmall/SandbagForiticationCustom.et");
	}	
	
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{			
		if (!GetVariableIn(PORT_BUILDING_TYPE,m_Building))
			return ENodeResult.FAIL;	
		
		ResourceName resourceName;
		SCR_AIWorld aiWorld = SCR_AIWorld.Cast(GetGame().GetAIWorld());
		//if (m_mTypenameToResourceName.Find("SandbagBarrier",resourceName))
		if (m_mTypenameToResourceName.Find(m_Building,resourceName))
		{
			vector direction = owner.GetControlledEntity().GetYawPitchRoll().AnglesToVector();
			vector pos = owner.GetControlledEntity().GetOrigin();
			
			vector forwardVector = pos + ( direction * 1 );
			forwardVector[1] = owner.GetWorld().GetSurfaceY(forwardVector[0], forwardVector[2]) + 0.15;
			m_ResultVector = forwardVector;	
			
			EntitySpawnParams params = EntitySpawnParams();
			params.TransformMode = ETransformMode.LOCAL;
			params.Transform[3] = m_ResultVector;
	
			vector ang = owner.GetControlledEntity().GetYawPitchRoll();
			Math3D.AnglesToMatrix(ang, params.Transform);
			
			Resource res = Resource.Load(resourceName);
			IEntity composition = GetGame().SpawnEntityPrefab(res, null, params);

			//composition.SetYawPitchRoll(ang);
			
			aiWorld.RequestNavmeshRebuildEntity(composition);
			return ENodeResult.SUCCESS;			
		}
		
		return ENodeResult.FAIL;			
	}	
		
	protected override bool VisibleInPalette()
	{
		return true;
	}
	
	protected override string GetOnHoverDescription()
	{
		return "AI task that spawn the Fortification";
	}	
};