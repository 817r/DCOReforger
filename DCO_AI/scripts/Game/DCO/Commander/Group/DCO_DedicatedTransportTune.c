modded class DCO_TransportTeamComponent
{
	protected DCO_GlobalAIComponent m_DCOTuneSrc_TT;
	protected int m_iDCOTuneVer_TT = -1;
	protected int m_iDCOTuneAll_TT = -1;
	protected bool m_bDCOTuneFac_TT;
	protected ref map<string, float> m_mDCOTuneOrig_TT;

	protected float DCO_Orig_TT(string key, float cur)
	{
		if (!m_mDCOTuneOrig_TT)
			m_mDCOTuneOrig_TT = new map<string, float>();
		float v;
		if (m_mDCOTuneOrig_TT.Find(key, v))
			return v;
		m_mDCOTuneOrig_TT.Set(key, cur);
		return cur;
	}

	protected float DCO_OrigB_TT(string key, bool cur)
	{
		float c = 0;
		if (cur)
			c = 1;
		return DCO_Orig_TT(key, c);
	}

	void DCO_TuneApply_TT()
	{
		bool fac = DCO_GlobalAIComponent.DCO_FactionActive();
		int all = DCO_GlobalAIComponent.DCO_TuneVerAll();
		if (!fac && !m_bDCOTuneFac_TT && all == m_iDCOTuneAll_TT)
			return;
		m_bDCOTuneFac_TT = fac;
		m_iDCOTuneAll_TT = all;
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetFor(GetOwner());
		if (!g || (g == m_DCOTuneSrc_TT && g.DCO_TuneVer() == m_iDCOTuneVer_TT))
			return;
		m_DCOTuneSrc_TT = g;
		m_iDCOTuneVer_TT = g.DCO_TuneVer();
		m_fBoardingDist = g.DCO_TuneF("tt.BoardingDist", DCO_Orig_TT("tt.BoardingDist", m_fBoardingDist));
		m_fBoardedRatio = g.DCO_TuneF("tt.BoardedRatio", DCO_Orig_TT("tt.BoardedRatio", m_fBoardedRatio));
		m_fArrivalDist = g.DCO_TuneF("tt.ArrivalDist", DCO_Orig_TT("tt.ArrivalDist", m_fArrivalDist));
		m_fBoardingTimeout = g.DCO_TuneF("tt.BoardingTimeout", DCO_Orig_TT("tt.BoardingTimeout", m_fBoardingTimeout));
		m_fDriveTimeout = g.DCO_TuneF("tt.DriveTimeout", DCO_Orig_TT("tt.DriveTimeout", m_fDriveTimeout));
		m_fDisembarkTimeout = g.DCO_TuneF("tt.DisembarkTimeout", DCO_Orig_TT("tt.DisembarkTimeout", m_fDisembarkTimeout));
		m_bReturnToRallyAfterDrop = g.DCO_TuneB("tt.ReturnToRallyAfterDrop", DCO_OrigB_TT("tt.ReturnToRallyAfterDrop", m_bReturnToRallyAfterDrop));
		m_fLoadTime = g.DCO_TuneF("tt.LoadTime", DCO_Orig_TT("tt.LoadTime", m_fLoadTime));
		m_fResupplyTime = g.DCO_TuneF("tt.ResupplyTime", DCO_Orig_TT("tt.ResupplyTime", m_fResupplyTime));
		m_iResupplyMags = g.DCO_TuneI("tt.ResupplyMags", DCO_Orig_TT("tt.ResupplyMags", m_iResupplyMags));
	}

	override protected void Tick(float worldTime)
	{
		DCO_TuneApply_TT();
		super.Tick(worldTime);
	}
}

