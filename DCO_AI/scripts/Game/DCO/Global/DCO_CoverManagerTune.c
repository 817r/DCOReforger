modded class SCR_CoverManagerComponent
{
	protected DCO_GlobalAIComponent m_DCOTuneSrc_CM;
	protected int m_iDCOTuneVer_CM = -1;
	protected int m_iDCOTuneAll_CM = -1;
	protected bool m_bDCOTuneFac_CM;
	protected ref map<string, float> m_mDCOTuneOrig_CM;

	protected float DCO_Orig_CM(string key, float cur)
	{
		if (!m_mDCOTuneOrig_CM)
			m_mDCOTuneOrig_CM = new map<string, float>();
		float v;
		if (m_mDCOTuneOrig_CM.Find(key, v))
			return v;
		m_mDCOTuneOrig_CM.Set(key, cur);
		return cur;
	}

	protected float DCO_OrigB_CM(string key, bool cur)
	{
		float c = 0;
		if (cur)
			c = 1;
		return DCO_Orig_CM(key, c);
	}

	void DCO_TuneApply_CM()
	{
		bool fac = DCO_GlobalAIComponent.DCO_FactionActive();
		int all = DCO_GlobalAIComponent.DCO_TuneVerAll();
		if (!fac && !m_bDCOTuneFac_CM && all == m_iDCOTuneAll_CM)
			return;
		m_bDCOTuneFac_CM = fac;
		m_iDCOTuneAll_CM = all;
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetInstance();
		if (!g || (g == m_DCOTuneSrc_CM && g.DCO_TuneVer() == m_iDCOTuneVer_CM))
			return;
		m_DCOTuneSrc_CM = g;
		m_iDCOTuneVer_CM = g.DCO_TuneVer();
		m_fMaxBookingDuration = g.DCO_TuneF("cm.MaxBookingDuration", DCO_Orig_CM("cm.MaxBookingDuration", m_fMaxBookingDuration));
	}

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		DCO_TuneApply_CM();
		super.EOnFrame(owner, timeSlice);
	}
}

