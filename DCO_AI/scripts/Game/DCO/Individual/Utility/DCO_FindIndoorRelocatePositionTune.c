modded class DCO_FindIndoorRelocatePosition
{
	protected DCO_GlobalAIComponent m_DCOTuneSrc_IR;
	protected int m_iDCOTuneVer_IR = -1;
	protected int m_iDCOTuneAll_IR = -1;
	protected bool m_bDCOTuneFac_IR;
	protected ref map<string, float> m_mDCOTuneOrig_IR;

	protected float DCO_Orig_IR(string key, float cur)
	{
		if (!m_mDCOTuneOrig_IR)
			m_mDCOTuneOrig_IR = new map<string, float>();
		float v;
		if (m_mDCOTuneOrig_IR.Find(key, v))
			return v;
		m_mDCOTuneOrig_IR.Set(key, cur);
		return cur;
	}

	protected float DCO_OrigB_IR(string key, bool cur)
	{
		float c = 0;
		if (cur)
			c = 1;
		return DCO_Orig_IR(key, c);
	}

	void DCO_TuneApply_IR(AIAgent owner)
	{
		bool fac = DCO_GlobalAIComponent.DCO_FactionActive();
		int all = DCO_GlobalAIComponent.DCO_TuneVerAll();
		if (!fac && !m_bDCOTuneFac_IR && all == m_iDCOTuneAll_IR)
			return;
		m_bDCOTuneFac_IR = fac;
		m_iDCOTuneAll_IR = all;
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetFor(owner);
		if (!g || (g == m_DCOTuneSrc_IR && g.DCO_TuneVer() == m_iDCOTuneVer_IR))
			return;
		m_DCOTuneSrc_IR = g;
		m_iDCOTuneVer_IR = g.DCO_TuneVer();
		m_fTowardThreatTolerance = g.DCO_TuneF("ir.TowardThreatTolerance", DCO_Orig_IR("ir.TowardThreatTolerance", m_fTowardThreatTolerance));
		m_fThreatWeightDefend = g.DCO_TuneF("ir.ThreatWeightDefend", DCO_Orig_IR("ir.ThreatWeightDefend", m_fThreatWeightDefend));
		m_fThreatWeightFire = g.DCO_TuneF("ir.ThreatWeightFire", DCO_Orig_IR("ir.ThreatWeightFire", m_fThreatWeightFire));
		m_fThreatSampleSpread = g.DCO_TuneF("ir.ThreatSampleSpread", DCO_Orig_IR("ir.ThreatSampleSpread", m_fThreatSampleSpread));
	}

	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		DCO_TuneApply_IR(owner);
		return super.EOnTaskSimulate(owner, dt);
	}
}

