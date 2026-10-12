modded class DCO_GroupContactReporterComponent
{
	protected DCO_GlobalAIComponent m_DCOTuneSrc_CR;
	protected int m_iDCOTuneVer_CR = -1;
	protected int m_iDCOTuneAll_CR = -1;
	protected bool m_bDCOTuneFac_CR;
	protected ref map<string, float> m_mDCOTuneOrig_CR;

	protected float DCO_Orig_CR(string key, float cur)
	{
		if (!m_mDCOTuneOrig_CR)
			m_mDCOTuneOrig_CR = new map<string, float>();
		float v;
		if (m_mDCOTuneOrig_CR.Find(key, v))
			return v;
		m_mDCOTuneOrig_CR.Set(key, cur);
		return cur;
	}

	protected float DCO_OrigB_CR(string key, bool cur)
	{
		float c = 0;
		if (cur)
			c = 1;
		return DCO_Orig_CR(key, c);
	}

	void DCO_TuneApply_CR()
	{
		bool fac = DCO_GlobalAIComponent.DCO_FactionActive();
		int all = DCO_GlobalAIComponent.DCO_TuneVerAll();
		if (!fac && !m_bDCOTuneFac_CR && all == m_iDCOTuneAll_CR)
			return;
		m_bDCOTuneFac_CR = fac;
		m_iDCOTuneAll_CR = all;
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetFor(GetOwner());
		if (!g || (g == m_DCOTuneSrc_CR && g.DCO_TuneVer() == m_iDCOTuneVer_CR))
			return;
		m_DCOTuneSrc_CR = g;
		m_iDCOTuneVer_CR = g.DCO_TuneVer();
		m_iMinEnemyToReport = g.DCO_TuneI("cr.MinEnemyToReport", DCO_Orig_CR("cr.MinEnemyToReport", m_iMinEnemyToReport));
		m_fContactMaxAge = g.DCO_TuneF("cr.ContactMaxAge", DCO_Orig_CR("cr.ContactMaxAge", m_fContactMaxAge));
		m_fContactClusterRadius = g.DCO_TuneF("cr.ContactClusterRadius", DCO_Orig_CR("cr.ContactClusterRadius", m_fContactClusterRadius));
		m_fReportMinShift = g.DCO_TuneF("cr.ReportMinShift", DCO_Orig_CR("cr.ReportMinShift", m_fReportMinShift));
		m_fMinResendInterval = g.DCO_TuneF("cr.MinResendInterval", DCO_Orig_CR("cr.MinResendInterval", m_fMinResendInterval));
		m_fLowConfidenceThreshold = g.DCO_TuneF("cr.LowConfidenceThreshold", DCO_Orig_CR("cr.LowConfidenceThreshold", m_fLowConfidenceThreshold));
		m_iHighDangerEnemyCount = g.DCO_TuneI("cr.HighDangerEnemyCount", DCO_Orig_CR("cr.HighDangerEnemyCount", m_iHighDangerEnemyCount));
		m_fHighDangerDistance = g.DCO_TuneF("cr.HighDangerDistance", DCO_Orig_CR("cr.HighDangerDistance", m_fHighDangerDistance));
		m_iReinforcementThreshold = g.DCO_TuneI("cr.ReinforcementThreshold", DCO_Orig_CR("cr.ReinforcementThreshold", m_iReinforcementThreshold));
		m_fReinfRequestTimeout = g.DCO_TuneF("cr.ReinfRequestTimeout", DCO_Orig_CR("cr.ReinfRequestTimeout", m_fReinfRequestTimeout));
		m_iArtilleryEnemyThreshold = g.DCO_TuneI("cr.ArtilleryEnemyThreshold", DCO_Orig_CR("cr.ArtilleryEnemyThreshold", m_iArtilleryEnemyThreshold));
		m_fArtilleryRequestCooldown = g.DCO_TuneF("cr.ArtilleryRequestCooldown", DCO_Orig_CR("cr.ArtilleryRequestCooldown", m_fArtilleryRequestCooldown));
		m_fNonFavoredRoleArtyRejectChance = g.DCO_TuneF("cr.NonFavoredRoleArtyRejectChance", DCO_Orig_CR("cr.NonFavoredRoleArtyRejectChance", m_fNonFavoredRoleArtyRejectChance));
		m_fNoisePer100m = g.DCO_TuneF("cr.NoisePer100m", DCO_Orig_CR("cr.NoisePer100m", m_fNoisePer100m));
		m_fNightNoiseMul = g.DCO_TuneF("cr.NightNoiseMul", DCO_Orig_CR("cr.NightNoiseMul", m_fNightNoiseMul));
		m_fDetectedNoiseMul = g.DCO_TuneF("cr.DetectedNoiseMul", DCO_Orig_CR("cr.DetectedNoiseMul", m_fDetectedNoiseMul));
		m_iBucketSquadMin = g.DCO_TuneI("cr.BucketSquadMin", DCO_Orig_CR("cr.BucketSquadMin", m_iBucketSquadMin));
		m_iBucketPlatoonMin = g.DCO_TuneI("cr.BucketPlatoonMin", DCO_Orig_CR("cr.BucketPlatoonMin", m_iBucketPlatoonMin));
		m_fArmorIdentifyDist = g.DCO_TuneF("cr.ArmorIdentifyDist", DCO_Orig_CR("cr.ArmorIdentifyDist", m_fArmorIdentifyDist));
		m_fRadioDelayMin = g.DCO_TuneF("cr.RadioDelayMin", DCO_Orig_CR("cr.RadioDelayMin", m_fRadioDelayMin));
		m_fRadioDelayMax = g.DCO_TuneF("cr.RadioDelayMax", DCO_Orig_CR("cr.RadioDelayMax", m_fRadioDelayMax));
		m_bRequireRadio = g.DCO_TuneB("cr.RequireRadio", DCO_OrigB_CR("cr.RequireRadio", m_bRequireRadio));
		m_fSuppressedDelayMul = g.DCO_TuneF("cr.SuppressedDelayMul", DCO_Orig_CR("cr.SuppressedDelayMul", m_fSuppressedDelayMul));
		m_fSuppressedNoiseMul = g.DCO_TuneF("cr.SuppressedNoiseMul", DCO_Orig_CR("cr.SuppressedNoiseMul", m_fSuppressedNoiseMul));
	}

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		DCO_TuneApply_CR();
		super.EOnFrame(owner, timeSlice);
	}
}

