modded class SCR_AIDCO_AttackPush
{
	protected DCO_GlobalAIComponent m_DCOTuneSrc_AP;
	protected int m_iDCOTuneVer_AP = -1;
	protected int m_iDCOTuneAll_AP = -1;
	protected bool m_bDCOTuneFac_AP;
	protected ref map<string, float> m_mDCOTuneOrig_AP;

	protected float DCO_Orig_AP(string key, float cur)
	{
		if (!m_mDCOTuneOrig_AP)
			m_mDCOTuneOrig_AP = new map<string, float>();
		float v;
		if (m_mDCOTuneOrig_AP.Find(key, v))
			return v;
		m_mDCOTuneOrig_AP.Set(key, cur);
		return cur;
	}

	protected float DCO_OrigB_AP(string key, bool cur)
	{
		float c = 0;
		if (cur)
			c = 1;
		return DCO_Orig_AP(key, c);
	}

	void DCO_TuneApply_AP(AIAgent owner)
	{
		bool fac = DCO_GlobalAIComponent.DCO_FactionActive();
		int all = DCO_GlobalAIComponent.DCO_TuneVerAll();
		if (!fac && !m_bDCOTuneFac_AP && all == m_iDCOTuneAll_AP)
			return;
		m_bDCOTuneFac_AP = fac;
		m_iDCOTuneAll_AP = all;
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetFor(owner);
		if (!g || (g == m_DCOTuneSrc_AP && g.DCO_TuneVer() == m_iDCOTuneVer_AP))
			return;
		m_DCOTuneSrc_AP = g;
		m_iDCOTuneVer_AP = g.DCO_TuneVer();
		m_fUpdateInterval_ms = g.DCO_TuneF("ap.UpdateInterval_ms", DCO_Orig_AP("ap.UpdateInterval_ms", m_fUpdateInterval_ms));
		m_bAllowBuildingAssault = g.DCO_TuneB("ap.AllowBuildingAssault", DCO_OrigB_AP("ap.AllowBuildingAssault", m_bAllowBuildingAssault));
		m_fBuildingAssaultDist = g.DCO_TuneF("ap.BuildingAssaultDist", DCO_Orig_AP("ap.BuildingAssaultDist", m_fBuildingAssaultDist));
		m_fFlankChance = g.DCO_TuneF("ap.FlankChance", DCO_Orig_AP("ap.FlankChance", m_fFlankChance));
	}

	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		DCO_TuneApply_AP(owner);
		return super.EOnTaskSimulate(owner, dt);
	}
}

