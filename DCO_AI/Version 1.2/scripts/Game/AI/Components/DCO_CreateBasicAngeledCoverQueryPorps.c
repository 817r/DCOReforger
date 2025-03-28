class SCR_AICreateBasicAngeledCoverQueryProps : AITaskScripted
{
	// Outputs
	protected const static string PORT_COVER_QUERY_PROPERTIES = "CoverQueryProps";
	protected const static string ME_POS = "My Position";
	
	// Inputs
	protected const static string PORT_POSITION = "Position";
	protected const static string PORT_POSITION_THREAT = "Position Danger Enemy";
	protected const static string PORT_RADIUS = "Radius";
	
	protected ref CoverQueryProperties m_CoverQueryProps = new CoverQueryProperties();
	
	protected const float COVER_QUERY_SECTOR_ANGLE_RAD = 0.3 * Math.PI;
	
	[Attribute("0", UIWidgets.EditBox)]
	protected float m_fRadius;
	
	[Attribute("0", UIWidgets.CheckBox)]
	protected bool m_bSelectHighestScoreCover;
	
	[Attribute("1", UIWidgets.CheckBox)]
	protected bool m_MaintainVisibility;
	
	//---------------------------------------------------------------
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		IEntity ownerEntity = owner.GetControlledEntity();
		if (!ownerEntity)
			return ENodeResult.FAIL;
		
		float radius = 0;
		if (!GetVariableIn(PORT_RADIUS, radius))
			radius = m_fRadius;
		
		vector queryPos = vector.Zero;
		if (!GetVariableIn(PORT_POSITION, queryPos))
			queryPos = ownerEntity.GetOrigin();
		
		vector mePos = ownerEntity.GetOrigin();
		
		vector threatPos;
		GetVariableIn(PORT_POSITION_THREAT, threatPos);
		if (threatPos == vector.Zero)
			return ENodeResult.FAIL;
		
		m_CoverQueryProps.m_vNearestPolyHalfExtend = SCR_AIFindCover.NEAREST_POLY_HALF_EXTEND;
		m_CoverQueryProps.m_fNmAreaCostScale = SCR_AIFindCover.NAVMESH_AREA_COST_SCALE;
		m_CoverQueryProps.m_vSectorPos = queryPos;
		m_CoverQueryProps.m_vSectorPos = queryPos;
		m_CoverQueryProps.m_vSectorDir = threatPos;
		m_CoverQueryProps.m_vThreatPos = threatPos; // Threat pos is not provided here, it's a basic query in radius
		m_CoverQueryProps.m_fQuerySectorAngleCosMin = -1.0;
		m_CoverQueryProps.m_fSectorDistMin = 2;
		m_CoverQueryProps.m_fSectorDistMax = radius;
		m_CoverQueryProps.m_fCoverToThreatAngleCosMin = COVER_QUERY_SECTOR_ANGLE_RAD;
		m_CoverQueryProps.m_fScoreWeightDirection = 0.0;
		m_CoverQueryProps.m_fScoreWeightDistance = 1.0;
		m_CoverQueryProps.m_bCheckVisibility = m_MaintainVisibility;
		m_CoverQueryProps.m_bSelectHighestScore = m_bSelectHighestScoreCover;
		m_CoverQueryProps.m_iMaxCoversToCheck = SCR_AIFindCover.MAX_COVERS_LOW_PRIORITY;
		
		if (m_bSelectHighestScoreCover)
			m_CoverQueryProps.m_fScoreWeightNavmeshRay = 0.2;
		else
			m_CoverQueryProps.m_fScoreWeightNavmeshRay = 5;
		
		SetVariableOut(PORT_COVER_QUERY_PROPERTIES, m_CoverQueryProps);
		SetVariableOut(ME_POS, mePos);
		return ENodeResult.SUCCESS;
	}
	
	
	
	//---------------------------------------------------------------
	override bool VisibleInPalette() { return true; }
	
	protected static ref TStringArray s_aVarsIn = {
		PORT_POSITION,
		PORT_POSITION_THREAT,
		PORT_RADIUS
	};
	override TStringArray GetVariablesIn() { return s_aVarsIn; }
	
	protected static ref TStringArray s_aVarsOut = {
		PORT_COVER_QUERY_PROPERTIES,
		ME_POS
	};
	override TStringArray GetVariablesOut() { return s_aVarsOut; }
	
	override string GetOnHoverDescription() { return "Creates CoverQueryProperties for basic circular query without threat visibility checking"; }
}