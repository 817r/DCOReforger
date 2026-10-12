modded class DCO_FindIndoorPosition
{
	protected DCO_GlobalAIComponent m_DCOTuneSrc_FI;
	protected int m_iDCOTuneVer_FI = -1;
	protected int m_iDCOTuneAll_FI = -1;
	protected bool m_bDCOTuneFac_FI;
	protected ref map<string, float> m_mDCOTuneOrig_FI;

	protected float DCO_Orig_FI(string key, float cur)
	{
		if (!m_mDCOTuneOrig_FI)
			m_mDCOTuneOrig_FI = new map<string, float>();
		float v;
		if (m_mDCOTuneOrig_FI.Find(key, v))
			return v;
		m_mDCOTuneOrig_FI.Set(key, cur);
		return cur;
	}

	protected float DCO_OrigB_FI(string key, bool cur)
	{
		float c = 0;
		if (cur)
			c = 1;
		return DCO_Orig_FI(key, c);
	}

	void DCO_TuneApply_FI(AIAgent owner)
	{
		bool fac = DCO_GlobalAIComponent.DCO_FactionActive();
		int all = DCO_GlobalAIComponent.DCO_TuneVerAll();
		if (!fac && !m_bDCOTuneFac_FI && all == m_iDCOTuneAll_FI)
			return;
		m_bDCOTuneFac_FI = fac;
		m_iDCOTuneAll_FI = all;
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetFor(owner);
		if (!g || (g == m_DCOTuneSrc_FI && g.DCO_TuneVer() == m_iDCOTuneVer_FI))
			return;
		m_DCOTuneSrc_FI = g;
		m_iDCOTuneVer_FI = g.DCO_TuneVer();
		m_fRadius = DCO_Orig_FI("fi.RadiusScale", m_fRadius) * g.DCO_TuneF("fi.RadiusScale", 1.0);
		m_iCandidatesPerTick = Math.Round(g.DCO_TuneF("fi.CandidatesPerTickScale", 1.0) * DCO_Orig_FI("fi.CandidatesPerTickScale", m_iCandidatesPerTick));
		m_iMaxCandidates = Math.Round(g.DCO_TuneF("fi.MaxCandidatesScale", 1.0) * DCO_Orig_FI("fi.MaxCandidatesScale", m_iMaxCandidates));
		m_iMaxTileWaitTicks = g.DCO_TuneI("fi.MaxTileWaitTicks", DCO_Orig_FI("fi.MaxTileWaitTicks", m_iMaxTileWaitTicks));
		m_fGoodEnoughScore = g.DCO_TuneF("fi.GoodEnoughScore", DCO_Orig_FI("fi.GoodEnoughScore", m_fGoodEnoughScore));
		m_fResultTTL = g.DCO_TuneF("fi.ResultTTL", DCO_Orig_FI("fi.ResultTTL", m_fResultTTL));
		m_fCellSize = g.DCO_TuneF("fi.CellSize", DCO_Orig_FI("fi.CellSize", m_fCellSize));
		m_fCellHeight = g.DCO_TuneF("fi.CellHeight", DCO_Orig_FI("fi.CellHeight", m_fCellHeight));
		m_fDedupeRadius = g.DCO_TuneF("fi.DedupeRadius", DCO_Orig_FI("fi.DedupeRadius", m_fDedupeRadius));
		m_iTopCandidates = g.DCO_TuneI("fi.TopCandidates", DCO_Orig_FI("fi.TopCandidates", m_iTopCandidates));
		m_fMinHeadroom = g.DCO_TuneF("fi.MinHeadroom", DCO_Orig_FI("fi.MinHeadroom", m_fMinHeadroom));
		m_fOccupancyRadius = g.DCO_TuneF("fi.OccupancyRadius", DCO_Orig_FI("fi.OccupancyRadius", m_fOccupancyRadius));
		m_fDoorBlockRadius = g.DCO_TuneF("fi.DoorBlockRadius", DCO_Orig_FI("fi.DoorBlockRadius", m_fDoorBlockRadius));
		m_fDoorAvoidRadius = g.DCO_TuneF("fi.DoorAvoidRadius", DCO_Orig_FI("fi.DoorAvoidRadius", m_fDoorAvoidRadius));
		m_fDoorAvoidPenalty = g.DCO_TuneF("fi.DoorAvoidPenalty", DCO_Orig_FI("fi.DoorAvoidPenalty", m_fDoorAvoidPenalty));
		m_bRejectChokepoints = g.DCO_TuneB("fi.RejectChokepoints", DCO_OrigB_FI("fi.RejectChokepoints", m_bRejectChokepoints));
		m_fChokepointWidth = g.DCO_TuneF("fi.ChokepointWidth", DCO_Orig_FI("fi.ChokepointWidth", m_fChokepointWidth));
		m_fMinBookedDistance = g.DCO_TuneF("fi.MinBookedDistance", DCO_Orig_FI("fi.MinBookedDistance", m_fMinBookedDistance));
		m_bRequireFireLine = g.DCO_TuneB("fi.RequireFireLine", DCO_OrigB_FI("fi.RequireFireLine", m_bRequireFireLine));
		m_fWeightCover = g.DCO_TuneF("fi.WeightCover", DCO_Orig_FI("fi.WeightCover", m_fWeightCover));
		m_fWeightFireLine = g.DCO_TuneF("fi.WeightFireLine", DCO_Orig_FI("fi.WeightFireLine", m_fWeightFireLine));
		m_fWeightSpread = g.DCO_TuneF("fi.WeightSpread", DCO_Orig_FI("fi.WeightSpread", m_fWeightSpread));
		m_fWeightProximity = g.DCO_TuneF("fi.WeightProximity", DCO_Orig_FI("fi.WeightProximity", m_fWeightProximity));
		m_fInteriorLineValue = g.DCO_TuneF("fi.InteriorLineValue", DCO_Orig_FI("fi.InteriorLineValue", m_fInteriorLineValue));
		m_fStickinessBonus = g.DCO_TuneF("fi.StickinessBonus", DCO_Orig_FI("fi.StickinessBonus", m_fStickinessBonus));
	}

	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		DCO_TuneApply_FI(owner);
		return super.EOnTaskSimulate(owner, dt);
	}
}

