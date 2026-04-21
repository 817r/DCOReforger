[ComponentEditorProps(category: "GameScripted/Commander", description: "Handles enemy contact response and reinforcement")]
class CMD_ThreatResponseComponentClass : ScriptComponentClass {}

class CMD_ThreatResponseComponent : ScriptComponent
{
	[Attribute("30.0", UIWidgets.EditBox, "Score minimum untuk kirim reinforcement biasa", category: "Threat")]
	protected float m_fEngageThreshold;

	[Attribute("70.0", UIWidgets.EditBox, "Score minimum untuk reinforcement diprioritaskan", category: "Threat")]
	protected float m_fReinforcementThreshold;

	[Attribute("60.0", UIWidgets.EditBox, "Detik sebelum threat entry dianggap expired dan dihapus", category: "Threat")]
	protected float m_fThreatExpiry;

	[Attribute("70.0", UIWidgets.EditBox, "Jarak (meter) dua laporan dianggap threat yang sama", category: "Threat")]
	protected float m_fMergeRadius;

	[Attribute("45.0", UIWidgets.EditBox, "Interval think cycle threat list (detik)", category: "Threat")]
	protected float m_fThinkInterval;

	[Attribute("2", UIWidgets.EditBox, "Maksimum reinforcement dikirim ke satu threat", category: "Reinforcement")]
	protected int m_iMaxReinforcementSent;

	[Attribute("120.0", UIWidgets.EditBox, "Cooldown (detik) antara pengiriman reinforcement ke threat yang sama", category: "Reinforcement")]
	protected float m_fReinforcementCooldown;

	[Attribute("1", UIWidgets.EditBox, "Penambahan score per musuh yang terdeteksi", category: "Threat Scoring")]
	protected int m_iEnemyIncrementedScore;

	[Attribute("30.0", UIWidgets.EditBox, "Detik sebelum intel dianggap stale", category: "Intel Decay")]
	protected float m_fStalenessThreshold;

	[Attribute("120.0", UIWidgets.EditBox, "Jarak counter-flank dari posisi threat (meter)", category: "Flanking")]
	protected float m_fFlankDistance;

	[Attribute("0.5", UIWidgets.Range, "Akurasi artillery untuk threat response (0–1)", params: "0 1 0.01", category: "Artillery")]
	protected float m_fArtilleryAccuracy;
	
	[Attribute("1", UIWidgets.EditBox, "Jumlah shell HE per fire mission", category: "Artillery")]
	protected int m_iHEShellCount;
	
	[Attribute("1", UIWidgets.EditBox, "Jumlah shell Smoke per fire mission", category: "Artillery")]
	protected int m_iSmokeShellCount;
	
	[Attribute("1", UIWidgets.EditBox, "Jumlah shell Illum per fire mission", category: "Artillery")]
	protected int m_iIllumShellCount;

	//--------------------------------------------------------------------
	protected AICommander_BaseComponent          m_Commander;
	protected CMD_ArtillerySupport				 m_ArtySupport;
	protected ref array<ref CMD_ThreatEntry>     m_aThreats    = new array<ref CMD_ThreatEntry>();
	protected float                              m_fThinkTimer = 0.0;
	protected float                              m_fMergeSQ    = 0.0;

	//--------------------------------------------------------------------
	void ReceiveContactReport(CMD_ContactReport report, DCO_GroupUtilityComponent grp)
	{
		if (!report)
			return;

		float worldTime = report.m_fReportTime;

		CMD_ThreatEntry existing = FindNearbyThreat(report.m_vPosition);
		if (existing)
		{
			existing.m_vPosition            = report.m_vPosition;
			existing.m_iEstimatedEnemyCount = Math.Max(existing.m_iEstimatedEnemyCount, report.m_iEstimatedEnemyCount);
			existing.m_fLastUpdateTime      = worldTime;
			existing.m_bNeedsRecon          = false;
			existing.m_bReconSent           = false;
			return;
		}

		CMD_ThreatEntry entry = new CMD_ThreatEntry(report.m_vPosition, report.m_iEstimatedEnemyCount, worldTime, grp);
		m_aThreats.Insert(entry);
	}
	
	void ReceiveArtillerySupport(CMD_FireMissionRequest request, DCO_GroupUtilityComponent grp)
	{
	    if (!m_Commander)
	        return;
	
	    float worldTime = GetGame().GetWorld().GetWorldTime() / 1000.0;
	
	    CMD_ThreatEntry threat = new CMD_ThreatEntry(
	        request.m_vImpactPos,
	        0,
	        worldTime,
	        grp
	    );
		
		float disp = CalculateArtilleryDispersion(GetOwner().GetOrigin(), threat.m_vPosition, request.m_eShellType);
		int shellNum = CalculateArtilleryShellCount(request, m_fArtilleryAccuracy, worldTime);
	    FireMission(threat, request.m_eShellType, shellNum, disp, worldTime);
	}
	
	int CalculateArtilleryShellCount(CMD_FireMissionRequest request, float accuracy, float worldTime)
	{
	    float shells = 0;
	
	    switch (request.m_eShellType)
	    {
	        case SCR_EAIArtilleryAmmoType.HIGH_EXPLOSIVE:
	            shells = 3 * m_iHEShellCount;
	            break;
	        case SCR_EAIArtilleryAmmoType.SMOKE:
	            shells = 5 * m_iSmokeShellCount;
	            break;
	        case SCR_EAIArtilleryAmmoType.ILLUMINATION:
	            shells = 2 * m_iIllumShellCount;
	            break;
	        case SCR_EAIArtilleryAmmoType.PRACTICE:
	            shells = 4;
	            break;
	    }
	
	    // Accuracy rendah = lempar lebih banyak buat kompensasi
	    if (accuracy < 0.3)
	        shells += 3;
	    else if (accuracy < 0.5)
	        shells += 2;
	    else if (accuracy < 0.7)
	        shells += 1;
	    else if (accuracy >= 0.85)
	        shells -= 1;
	
	    // Data stale = coverage lebih luas
	    float dataAge = worldTime - request.m_fRequestedTime;
	    if (dataAge > 60.0)
	        shells += 2;
	    else if (dataAge > 30.0)
	        shells += 1;
	
	    return Math.Clamp(shells, 1, 12);
	}
	
	float CalculateArtilleryDispersion(vector commanderPos, vector impactPos, SCR_EAIArtilleryAmmoType shellType)
	{
	    float rangeToTarget = vector.Distance(commanderPos, impactPos);
	
	    float baseDispersion = Math.Clamp(rangeToTarget * 0.1, 20.0, 150.0);
	
	    switch (shellType)
	    {
	        case SCR_EAIArtilleryAmmoType.HIGH_EXPLOSIVE:
	            baseDispersion *= 1.0;
	            break;
	        case SCR_EAIArtilleryAmmoType.SMOKE:
	            baseDispersion *= 1.2;
	            break;
	        case SCR_EAIArtilleryAmmoType.ILLUMINATION:
	            baseDispersion *= 1.5;
	            break;
	        case SCR_EAIArtilleryAmmoType.PRACTICE:
	            baseDispersion *= 0.8;
	            break;
	    }
	
	    return baseDispersion;
	}
	
	protected void FireMission(CMD_ThreatEntry threat, SCR_EAIArtilleryAmmoType shellType, int shellCount, float baseDispersion, float worldTime)
	{
	    if (!m_Commander)
	        return;
	
	    CMD_ArtillerySupport artComp = CMD_ArtillerySupport.Cast(
	        m_Commander.GetOwner().FindComponent(CMD_ArtillerySupport));
	    if (!artComp)
	    {
	        Print("[DCO_ThreatResponse] FireMission GAGAL — CMD_ArtillerySupport tidak ditemukan");
	        return;
	    }
	
	    // === LOLOS SEMUA CHECK — QUEUE REQUEST ===
	
	    string shellTypeName = "HE";
	    if (shellType == SCR_EAIArtilleryAmmoType.SMOKE)
	        shellTypeName = "SMOKE";
	    else if (shellType == SCR_EAIArtilleryAmmoType.ILLUMINATION)
	        shellTypeName = "ILLUM";
	
	    Print(string.Format("[DCO_ThreatResponse] FireMission → %1 x %2 @ %3",
	        shellCount, shellTypeName, threat.m_vPosition.ToString()));
	
		artComp.RequestShellImpact(threat.m_vPosition, shellType, worldTime, shellCount);
	
	    threat.m_bArtilleryCalled   = true;
	    threat.m_fLastArtilleryTime = worldTime;
	}

	void ReceiveReinforcementRequest(DCO_GroupUtilityComponent requestingGrp, float worldTime)
	{
		if (!requestingGrp)
			return;

		CMD_ThreatEntry threat = FindNearbyThreat(requestingGrp.GetOwner().GetOrigin());
		if (!threat)
			return;

		if (worldTime - threat.m_fLastReinforcementTime < m_fReinforcementCooldown)
			return;

		if (threat.m_iReinforcementSentNumber >= m_iMaxReinforcementSent)
			return;

		if (threat.m_fPriorityScore < m_fReinforcementThreshold)
			return;

		SendReinforcement(threat, worldTime);
	}

	//--------------------------------------------------------------------
	protected void Think(float worldTime)
	{
		PurgeExpiredThreats(worldTime);
		MergeNearbyThreats();

		int i = 0;
		while (i < m_aThreats.Count())
		{
			CMD_ThreatEntry threat = m_aThreats[i];
			if (!threat)
			{
				i++;
				continue;
			}

			ScoreThreat(threat, worldTime);
			ClassifyThreat(threat);

			if (threat.m_bNeedsRecon && !threat.m_bReconSent)
				TrySendReconForThreat(threat, worldTime);

			if ((threat.m_eThreatLevel == CMD_EThreatLevel.HIGH
				|| threat.m_eThreatLevel == CMD_EThreatLevel.CRITICAL)
				&& !threat.m_bFlankSent)
			{
				TrySendCounterFlank(threat, worldTime);
			}

			i++;
		}
	}

	//--------------------------------------------------------------------
	protected void ScoreThreat(CMD_ThreatEntry threat, float worldTime)
	{
		float score = 0.0;

		float enemyBonus = Math.Clamp(threat.m_iEstimatedEnemyCount * 5.0, 0.0, 50.0);
		score = score + (enemyBonus * m_iEnemyIncrementedScore);

		float age = worldTime - threat.m_fLastUpdateTime;
		float freshnessBonus = Math.Max(0.0, 20.0 - (age * 0.5));
		score = score + freshnessBonus;

		float proximityBonus = ComputeObjectiveProximityBonus(threat.m_vPosition);
		score = score + proximityBonus;

		if (age > m_fStalenessThreshold)
		{
			float staleAge     = age - m_fStalenessThreshold;
			float stalePenalty = Math.Clamp(staleAge * 0.5, 0.0, 30.0);
			score              = Math.Max(0.0, score - stalePenalty);
			threat.m_bNeedsRecon = true;
		}
		else
		{
			threat.m_bNeedsRecon = false;
		}

		if (!m_Commander)
		{
			threat.m_fPriorityScore = score;
			return;
		}

		float distToBase  = vector.Distance(threat.m_vPosition, m_Commander.GetOwner().GetOrigin());
		float distPenalty = Math.Clamp(distToBase * 0.05, 0.0, 25.0);
		score             = Math.Max(0.0, score - distPenalty);

		threat.m_fPriorityScore = score;
	}

	protected float ComputeObjectiveProximityBonus(vector threatPos)
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr || !m_Commander)
			return 0.0;

		float bestBonus = 0.0;

		foreach (CMD_AICommanderObjectiveComponent obj : mgr.m_aObjective)
		{
			if (!obj)
				continue;

			float dist = vector.Distance(threatPos, obj.GetOwner().GetOrigin());
			if (dist > obj.GetRadius() * 1.3)
				continue;

			float bonus = Math.Max(0.0, 25.0 - (dist * 0.08));
			if (bonus > bestBonus)
				bestBonus = bonus;
		}

		return bestBonus;
	}

	protected void ClassifyThreat(CMD_ThreatEntry threat)
	{
		float score = threat.m_fPriorityScore;

		if (score < 20.0)
			threat.m_eThreatLevel = CMD_EThreatLevel.NEGLIGIBLE;
		else if (score < m_fEngageThreshold)
			threat.m_eThreatLevel = CMD_EThreatLevel.LOW;
		else if (score < 55.0)
			threat.m_eThreatLevel = CMD_EThreatLevel.MEDIUM;
		else if (score < 75.0)
			threat.m_eThreatLevel = CMD_EThreatLevel.HIGH;
		else
			threat.m_eThreatLevel = CMD_EThreatLevel.CRITICAL;
	}

	protected void MergeNearbyThreats()
	{
		int i = 0;
		while (i < m_aThreats.Count())
		{
			CMD_ThreatEntry a = m_aThreats[i];
			if (!a)
			{
				i++;
				continue;
			}

			int j = i + 1;
			while (j < m_aThreats.Count())
			{
				CMD_ThreatEntry b = m_aThreats[j];
				if (!b)
				{
					j++;
					continue;
				}

				if (vector.DistanceSq(a.m_vPosition, b.m_vPosition) > m_fMergeSQ)
				{
					j++;
					continue;
				}

				a.m_iEstimatedEnemyCount     = Math.Max(a.m_iEstimatedEnemyCount, b.m_iEstimatedEnemyCount);
				a.m_iReinforcementSentNumber = Math.Max(a.m_iReinforcementSentNumber, b.m_iReinforcementSentNumber);
				a.m_fLastReinforcementTime   = Math.Max(a.m_fLastReinforcementTime, b.m_fLastReinforcementTime);
				a.m_fLastArtilleryTime       = Math.Max(a.m_fLastArtilleryTime, b.m_fLastArtilleryTime);
				a.m_bFlankSent               = a.m_bFlankSent || b.m_bFlankSent;
				a.m_bReinforcementSent       = a.m_bReinforcementSent || b.m_bReinforcementSent;
				a.m_bArtilleryCalled         = a.m_bArtilleryCalled || b.m_bArtilleryCalled;

				if (b.m_fLastUpdateTime > a.m_fLastUpdateTime)
				{
					a.m_vPosition       = b.m_vPosition;
					a.m_fLastUpdateTime = b.m_fLastUpdateTime;
				}

				if (!a.m_bNeedsRecon && b.m_bNeedsRecon)
				{
					a.m_bNeedsRecon = true;
					a.m_bReconSent  = b.m_bReconSent;
				}

				m_aThreats.Remove(j);
			}

			i++;
		}
	}

	protected void TrySendReconForThreat(CMD_ThreatEntry threat, float worldTime)
	{
		if (!m_Commander)
			return;

		DCO_GroupUtilityComponent reconGrp = m_Commander.FindBestIdleGroupForRole_Public(CMD_EGroupRole.RECON);
		if (!reconGrp)
			reconGrp = m_Commander.FindBestIdleGroupForRole_Public(CMD_EGroupRole.RESERVE);
		if (!reconGrp)
			return;

		SCR_AIWaypoint wp = m_Commander.SpawnMoveWP(threat.m_vPosition);
		if (!wp)
			return;

		reconGrp.SetGroupRole(CMD_EGroupRole.RECON);
		reconGrp.MoveTo(wp, worldTime);
		threat.m_bReconSent = true;
	}
	
	protected DCO_GroupUtilityComponent FindClosestGroupForRole(CMD_EGroupRole role, vector pos)
	{
	    if (!m_Commander)
	        return null;
	
	    return m_Commander.FindClosestIdleGroupForRole_Public(role, pos);
	}

	protected void TrySendCounterFlank(CMD_ThreatEntry threat, float worldTime)
	{
		if (!m_Commander)
			return;

		DCO_GroupUtilityComponent flankGrp = m_Commander.FindBestIdleGroupForRole_Public(CMD_EGroupRole.FLANK);
		if (!flankGrp)
			flankGrp = m_Commander.FindBestIdleGroupForRole_Public(CMD_EGroupRole.REINFORNCE);
		if (!flankGrp)
			return;

		vector commanderPos = m_Commander.GetOwner().GetOrigin();
		vector flankPos     = m_Commander.ComputeFlankPosition(commanderPos, threat.m_vPosition, m_fFlankDistance);
		flankPos[1]         = GetGame().GetWorld().GetSurfaceY(flankPos[0], flankPos[2]);

		SCR_AIWaypoint wp = m_Commander.SpawnMoveWP(flankPos);
		if (!wp)
			return;

		flankGrp.SetGroupRole(CMD_EGroupRole.FLANK);
		flankGrp.MoveTo(wp, worldTime);
		threat.m_bFlankSent = true;
	}

	protected void SendReinforcement(CMD_ThreatEntry threat, float worldTime)
	{
	    if (!m_Commander)
	        return;
	
	    // Hitung slot yang masih kosong untuk threat ini
	    int slotsLeft = m_iMaxReinforcementSent - threat.m_iReinforcementSentNumber;
	    if (slotsLeft <= 0)
	        return;
	
	    int sentThisCall = 0;
	
	    while (sentThisCall < slotsLeft)
	    {
	        DCO_GroupUtilityComponent reinforcement = null;
	        bool armored = false;
	
	        // HIGH/CRITICAL → prioritaskan ARMORED dulu, fallback ke REINFORNCE
	        if (threat.m_eThreatLevel >= CMD_EThreatLevel.HIGH)
	        {
	            reinforcement = FindClosestGroupForRole(CMD_EGroupRole.ARMORED, threat.m_vPosition);
	            if (reinforcement)
	            {
	                armored = true;
	            }
	            else
	            {
	                reinforcement = FindClosestGroupForRole(CMD_EGroupRole.REINFORNCE, threat.m_vPosition);
	                armored = false;
	            }
	        }
	        else
	        {
	            reinforcement = FindClosestGroupForRole(CMD_EGroupRole.REINFORNCE, threat.m_vPosition);
	        }
	
	        // Tidak ada lagi group idle yang bisa dikirim — stop loop
	        if (!reinforcement)
	            break;
	
	        // Coba pakai transport dulu
	        if (m_Commander.TryAssignTransport(reinforcement, threat.m_vPosition, worldTime))
	        {
	            threat.m_iReinforcementSentNumber++;
	            threat.m_bReinforcementSent     = true;
	            threat.m_fLastReinforcementTime = worldTime;
	            sentThisCall++;
	
	            Print(string.Format("[DCO_ThreatResponse] Reinforcement %1/%2 (transport) dikirim ke %3",
	                threat.m_iReinforcementSentNumber,
	                m_iMaxReinforcementSent,
	                threat.m_vPosition.ToString()));
	
	            continue;
	        }
	
	        // Transport tidak tersedia → jalan kaki/kendaraan sendiri
	        SCR_AIWaypoint wp = m_Commander.SpawnMoveWP(threat.m_vPosition);
	        if (!wp)
	            break;
	
	        if (!armored)
	            reinforcement.SetGroupRole(CMD_EGroupRole.REINFORNCE);
	
	        reinforcement.MoveTo(wp, worldTime);
	
	        threat.m_iReinforcementSentNumber++;
	        threat.m_bReinforcementSent     = true;
	        threat.m_fLastReinforcementTime = worldTime;
	        sentThisCall++;
	
	        Print(string.Format("[DCO_ThreatResponse] Reinforcement %1/%2 (foot/motor) dikirim ke %3",
	            threat.m_iReinforcementSentNumber,
	            m_iMaxReinforcementSent,
	            threat.m_vPosition.ToString()));
	    }
	
	    if (sentThisCall > 0)
	    {
	        Print(string.Format("[DCO_ThreatResponse] Total %1 group dikirim, slot terisi %2/%3",
	            sentThisCall,
	            threat.m_iReinforcementSentNumber,
	            m_iMaxReinforcementSent));
	    }
	}

	//--------------------------------------------------------------------
	protected CMD_ThreatEntry FindNearbyThreat(vector pos)
	{
		for (int i = 0; i < m_aThreats.Count(); i++)
		{
			CMD_ThreatEntry t = m_aThreats[i];
			if (!t)
				continue;
			if (vector.DistanceSq(t.m_vPosition, pos) <= m_fMergeSQ)
				return t;
		}
		return null;
	}

	protected void PurgeExpiredThreats(float worldTime)
	{
		int i = 0;
		while (i < m_aThreats.Count())
		{
			CMD_ThreatEntry t = m_aThreats[i];
			if (!t)
			{
				m_aThreats.Remove(i);
				continue;
			}

			if (worldTime - t.m_fLastUpdateTime > m_fThreatExpiry)
			{
				m_aThreats.Remove(i);
				continue;
			}

			i++;
		}
	}

	int GetActiveThreatCount() { return m_aThreats.Count(); }

	FactionKey GetFactionKey()
	{
		if (!m_Commander)
			return string.Empty;
		return m_Commander.GetCommanderFactionKey();
	}

	//--------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (!Replication.IsServer())
			return;

		m_fThinkTimer += timeSlice;
		if (m_fThinkTimer < m_fThinkInterval)
			return;

		m_fThinkTimer = 0.0;
		float worldTime = GetGame().GetWorld().GetWorldTime() / 1000.0;
		Think(worldTime);
	}

	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);
		SetEventMask(owner, EntityEvent.FRAME);
	}

	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		if (!Replication.IsServer())
			return;

		m_Commander = AICommander_BaseComponent.Cast(owner.FindComponent(AICommander_BaseComponent));
		if (!m_Commander)
			return;
		
		m_ArtySupport = CMD_ArtillerySupport.Cast(owner.FindComponent(CMD_ArtillerySupport));

		m_fMergeSQ = m_fMergeRadius * m_fMergeRadius;
	}
}