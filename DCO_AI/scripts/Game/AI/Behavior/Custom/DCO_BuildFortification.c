class DCO_AIFortification : AITaskScripted
{	
	ref map<string, string> m_mTypenameToResourceName;
	protected vector m_ResultVector;
	
	override void OnInit(AIAgent owner)
	{
		m_mTypenameToResourceName = new map<string, string>;
		m_mTypenameToResourceName.Insert("SandbagBarrier","{0B0C600941B2A6A8}Prefabs/Props/Military/Sandbags/Sandbag_01_short_high_plastic.et");
	}	
	
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{		
		ResourceName resourceName;
		SCR_AIWorld aiWorld = SCR_AIWorld.Cast(GetGame().GetAIWorld());
		if (m_mTypenameToResourceName.Find("SandbagBarrier",resourceName))
		{
			vector direction = owner.GetControlledEntity().GetYawPitchRoll().AnglesToVector();
			vector pos = owner.GetControlledEntity().GetOrigin();
			
			vector forwardVector = pos + ( direction * 1 );
			forwardVector[1] = owner.GetWorld().GetSurfaceY(forwardVector[0], forwardVector[2]);
			m_ResultVector = forwardVector;	
			
			EntitySpawnParams params = EntitySpawnParams();
			params.TransformMode = ETransformMode.LOCAL;
			params.Transform[3] = m_ResultVector;	
			//params.Transform.FromYaw(owner.GetControlledEntity().GetYawPitchRoll().ToYaw());
	
			vector ang = owner.GetControlledEntity().GetYawPitchRoll();
			
			Resource res = Resource.Load(resourceName);
			IEntity composition = GetGame().SpawnEntityPrefab(res, null, params);

			composition.SetYawPitchRoll(ang);
			
			aiWorld.RequestNavmeshRebuildEntity(composition);
			SetVariableOut("YPR", ang.ToString());
			return ENodeResult.SUCCESS;			
		}
		
		
		
		return ENodeResult.FAIL;			
	}	
	
	protected static ref TStringArray s_aVarsOut = {
		"YPR",
	};
	
	override array<string> GetVariablesOut()
    {
        return s_aVarsOut;
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