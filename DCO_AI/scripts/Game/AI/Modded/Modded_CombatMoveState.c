modded class SCR_AICombatMoveState
{
	//! Tanpa syarat terlindungi selama ini setelah apply, karena BT butuh 1 tick untuk mulai eksekusi
	protected static const float DCO_COVER_PROTECT_GRACE_S = 0.5;

	protected SCR_AICombatMoveRequestBase m_DCOCoverRq;           // weak ref, state sendiri yang memegang request
	protected float m_fDCOCoverAppliedAt_ms;
	protected float m_fDCOCoverProtectUntil_ms;
	protected bool  m_bDCOApplyingCover;
	protected int   m_iDCOBlockedCount;                           // untuk log, dicetak sekali saat release
	
	protected ref SCR_AICombatMoveRequestBase m_OldRequest;

	//------------------------------------------------------------------------------------------------
	//! Kirim request cover dan lindungi maksimal protect_s detik
	void DCO_ApplyCoverRequest(SCR_AICombatMoveRequestBase rq, float protect_s)
	{
		if (!rq)
			return;

		m_bDCOApplyingCover = true;
		ApplyNewRequest(rq);
		m_bDCOApplyingCover = false;

		float now_ms = GetGame().GetWorld().GetWorldTime();
		m_iDCOBlockedCount         = 0;
		m_DCOCoverRq               = rq;
		m_fDCOCoverAppliedAt_ms    = now_ms;
		m_fDCOCoverProtectUntil_ms = now_ms + Math.Max(protect_s, DCO_COVER_PROTECT_GRACE_S) * 1000.0;
	}

	//------------------------------------------------------------------------------------------------
	//! True selama request cover masih berjalan / menunggu fallback bangunan
	bool DCO_IsCoverProtected()
	{
		if (!m_DCOCoverRq)
			return false;

		float now_ms = GetGame().GetWorld().GetWorldTime();

		// Batas keras
		if (now_ms > m_fDCOCoverProtectUntil_ms)
		{
			DCO_ReleaseCoverProtection("expired");
			return false;
		}

		// Grace: BT belum sempat mulai
		if (now_ms - m_fDCOCoverAppliedAt_ms < DCO_COVER_PROTECT_GRACE_S * 1000.0)
			return true;

		// Bangunan tidak ketemu: tetap dilindungi sampai fallback cover dikirim (atau batas keras)
		if (m_DCOCoverRq.m_eFailReason == SCR_EAICombatMoveRequestFailReason.NO_BUILDING_FOUND)
			return true;

		// Masih lari
		if (IsExecutingRequest() && GetRequest() == m_DCOCoverRq)
			return true;

		DCO_ReleaseCoverProtection("finished");
		return false;
	}

	//------------------------------------------------------------------------------------------------
	void DCO_ReleaseCoverProtection(string reason)
	{
		if (!m_DCOCoverRq)
			return;

		if (SCR_AIDangerReaction_WeaponFired.DCO_IsCoverDebugOn())
		{
			Print(string.Format("[DCO_Cover] t=%1 state=%2 | PROTECT RELEASE (%3) fail=%4 blocked=%5",
				GetGame().GetWorld().GetWorldTime(), this, reason,
				typename.EnumToString(SCR_EAICombatMoveRequestFailReason, m_DCOCoverRq.m_eFailReason), m_iDCOBlockedCount), LogLevel.NORMAL);
		}

		m_DCOCoverRq = null;
		m_fDCOCoverProtectUntil_ms = 0;
	}

	//------------------------------------------------------------------------------------------------
	override void ApplyNewRequest(notnull SCR_AICombatMoveRequestBase request)
	{
		if (!m_bDCOApplyingCover && DCO_IsCoverProtected())
		{
			m_iDCOBlockedCount++;

			// Log hanya blokir pertama per periode proteksi (sistem lain bisa coba tiap tick)
			if (m_iDCOBlockedCount == 1 && SCR_AIDangerReaction_WeaponFired.DCO_IsCoverDebugOn())
			{
				Print(string.Format("[DCO_Cover] t=%1 state=%2 | PROTECT BLOCKED request lain saat lari ke cover (%3)",
					GetGame().GetWorld().GetWorldTime(), this, request), LogLevel.NORMAL);
			}
			return;
		}

		super.ApplyNewRequest(request);
	}
	
	SCR_AICombatMoveRequestBase GetOldRequest()
	{
		return m_OldRequest;
	}
	
	bool IsMovingToBuilding()
	{
	    if (!m_Request)
	        return false;
	
	    if (m_Request.m_eState != SCR_EAICombatMoveRequestState.EXECUTING)
	        return false;
	
	    return m_Request.m_eType == SCR_EAICombatMoveRequestType.BUILDING;
	}
	
	bool IsPositionInBuilding()
	{
		bool isIt = false;
		
		return isIt;
	}
}

modded enum SCR_EAICombatMoveRequestFailReason
{
	NO_BUILDING_FOUND
}

modded class SCR_AICombatMoveRequestBase
{
	SCR_EAICombatMoveRequestType m_eType = SCR_EAICombatMoveRequestType.STOP;
}