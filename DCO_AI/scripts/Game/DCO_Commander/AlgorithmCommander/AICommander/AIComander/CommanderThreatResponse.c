[ComponentEditorProps(category: "GameScripted/Commander", description: "Handles enemy contact response and reinforcement")]
class CMD_ThreatResponseComponentClass : ScriptComponentClass {}

// === REWRITE NOTES ===
// Overhaul total dari versi sebelumnya. Perubahan arsitektur utama:
//
// 1. THREAT CLUSTERING -- tiap Think() cycle, m_aThreats di-grouping jadi cluster
//    (flood-fill by m_fClusterRadius, transitif -- bukan cuma pairwise kayak
//    MergeNearbyThreats yang radiusnya lebih kecil buat dedup laporan yang sama).
//    Semua keputusan respons aktif (reinforcement, counter-flank, artillery)
//    dievaluasi di level CLUSTER AGGREGATE, bukan per-entry individual.
// 2. "Kalo ga begitu bahaya ya gak usah dikirim" -- cluster yang combined score-nya
//    di bawah m_fClusterMinResponseScore di-skip total, gak ada respons aktif sama
//    sekali (recon tetep jalan per-entry, karena itu soal ngumpulin info, bukan
//    eskalasi kekuatan).
// 3. Reinforcement & counter-flank sekarang OTOMATIS ke-trigger dari Think() lewat
//    cluster (sebelumnya reinforcement CUMA bisa lewat ReceiveReinforcementRequest
//    eksternal yang gak pernah dipanggil dari manapun -- itu bug utama kenapa
//    "gak jalan").
// 4. Artillery juga otomatis dari cluster (TrySendClusterArtillery), milih shell
//    type berdasarkan kondisi cluster: intel basi -> SMOKE, CRITICAL/HIGH + intel
//    fresh + gak ada friendly deket -> HIGH_EXPLOSIVE, MEDIUM ke bawah -> skip
//    (biar reinforcement aja yang handle).
// 5. Dispersion calculation duplikat (yang sebelumnya dead code, dihitung tapi gak
//    pernah kepake) DIHAPUS dari file ini. CMD_ArtillerySupport sekarang jadi
//    SATU-SATUNYA source of truth buat dispersion/accuracy tembakan beneran.
// 6. Friendly-fire safety check (HasFriendlyNearPosDefault) sekarang BENERAN
//    dipanggil sebelum artillery dikirim (sebelumnya dead, gak pernah di-wire).
// === END REWRITE NOTES ===
class CMD_ThreatResponseComponent : ScriptComponent
{
	[Attribute("30.0", UIWidgets.EditBox, "Score minimum untuk kirim reinforcement biasa", category: "Threat")]
	protected float m_fEngageThreshold;

	[Attribute("70.0", UIWidgets.EditBox, "Score minimum untuk reinforcement diprioritaskan", category: "Threat")]
	protected float m_fReinforcementThreshold;

	[Attribute("60.0", UIWidgets.EditBox, "Detik sebelum threat entry dianggap expired dan dihapus", category: "Threat")]
	protected float m_fThreatExpiry;

	[Attribute("70.0", UIWidgets.EditBox, "Jarak (meter) dua laporan dianggap threat yang sama (dedup/merge)", category: "Threat")]
	protected float m_fMergeRadius;

	// === ADDED: Threat Clustering ===
	[Attribute("150.0", UIWidgets.EditBox, "Jarak (meter) buat grouping threat jadi 1 cluster respons -- lebih gede dari merge radius, karena ini buat 'gimana kita respons', bukan dedup laporan", category: "Threat")]
	protected float m_fClusterRadius;

	[Attribute("25.0", UIWidgets.EditBox, "Combined score minimum cluster sebelum ada respons aktif (reinforcement/flank/artillery) dikirim. Di bawah ini = dianggap gak cukup bahaya, di-skip total", category: "Threat")]
	protected float m_fClusterMinResponseScore;
	// === END ADDED ===

	[Attribute("45.0", UIWidgets.EditBox, "Interval think cycle threat list (detik)", category: "Threat")]
	protected float m_fThinkInterval;

	[Attribute("2", UIWidgets.EditBox, "Maksimum reinforcement dikirim ke satu cluster", category: "Reinforcement")]
	protected int m_iMaxReinforcementSent;

	[Attribute("120.0", UIWidgets.EditBox, "Cooldown (detik) antara pengiriman reinforcement ke cluster yang sama", category: "Reinforcement")]
	protected float m_fReinforcementCooldown;

	[Attribute("1", UIWidgets.EditBox, "Penambahan score per musuh yang terdeteksi", category: "Threat Scoring")]
	protected int m_iEnemyIncrementedScore;

	[Attribute("30.0", UIWidgets.EditBox, "Detik sebelum intel dianggap stale", category: "Intel Decay")]
	protected float m_fStalenessThreshold;

	[Attribute("120.0", UIWidgets.EditBox, "Jarak counter-flank dari posisi threat (meter)", category: "Flanking")]
	protected float m_fFlankDistance;

	[Attribute("0.5", UIWidgets.Range, "Akurasi artillery untuk request manual/langsung (0–1)", params: "0 1 0.01", category: "Artillery")]
	protected float m_fArtilleryAccuracy;

	// === ADDED: cooldown khusus artillery per-cluster, terpisah dari reinforcement
	// cooldown -- artillery harusnya lebih jarang/berat daripada kirim reinforcement ===
	[Attribute("90.0", UIWidgets.EditBox, "Cooldown (detik) artillery per-cluster", category: "Artillery")]
	protected float m_fArtilleryCooldown;
	// === END ADDED ===

	//--------------------------------------------------------------------
	protected AICommander_BaseComponent          m_Commander;
	protected CMD_ArtillerySupport				 m_ArtySupport;
	protected ref array<ref CMD_ThreatEntry>     m_aThreats    = new array<ref CMD_ThreatEntry>();
	protected float                              m_fThinkTimer = 0.0;
	protected float                              m_fMergeSQ    = 0.0;
	protected float                              m_fClusterSQ  = 0.0;

	//--------------------------------------------------------------------
	void ReceiveContactReport(CMD_ContactReport report, DCO_GroupUtilityComponent grp)
	{
		if (!report)
			return;

		float worldTime = report.m_fReportTime;

		// === ADDED: Report Quality -- jarak grup pelapor (spotter) ke posisi target
		// pas laporan ini dibuat. Deket = laporan jelas/reliable, jauh = perkiraan
		// kasar. Ini basis "akurasi intel" buat artillery (Bagian 1).
		float reportQuality = 0.7; // default netral kalau grp entah kenapa null
		if (grp && grp.GetOwner())
		{
			float spotDistance = vector.Distance(grp.GetOwner().GetOrigin(), report.m_vPosition);
			reportQuality = ComputeReportQuality(spotDistance);
		}
		// === END ADDED ===

		CMD_ThreatEntry existing = FindNearbyThreat(report.m_vPosition);
		if (existing)
		{
			existing.m_vPosition            = report.m_vPosition;
			existing.m_iEstimatedEnemyCount = Math.Max(existing.m_iEstimatedEnemyCount, report.m_iEstimatedEnemyCount);
			existing.m_fLastUpdateTime      = worldTime;
			existing.m_bNeedsRecon          = false;
			existing.m_bReconSent           = false;
			existing.m_fReportQuality       = reportQuality; // update ke laporan terbaru
			return;
		}

		CMD_ThreatEntry entry = new CMD_ThreatEntry(report.m_vPosition, report.m_iEstimatedEnemyCount, worldTime, grp);
		entry.m_fReportQuality = reportQuality;
		m_aThreats.Insert(entry);
	}
	
	// === ADDED: Report Quality helper ===
	//! Konversi jarak spotter->target jadi skor quality 0.2-1.0. Deket (<=50m) =
	//! quality penuh, jauh (>=400m) = quality minimum, linear di antaranya.
	static float ComputeReportQuality(float spotDistance)
	{
		return Math.Clamp(1.0 - (spotDistance - 50.0) / 350.0, 0.2, 1.0);
	}
	// === END ADDED ===

	//--------------------------------------------------------------------
	//! Entry point buat request artillery LANGSUNG/MANUAL (misal dari group yang
	//! spesifik minta fire support), terpisah dari jalur otomatis cluster di Think().
	//! Dispersion/accuracy tembakan beneran dihitung sepenuhnya oleh CMD_ArtillerySupport
	//! -- di sini cuma nentuin shell count dan jalanin safety check.
	void ReceiveArtillerySupport(CMD_FireMissionRequest request, DCO_GroupUtilityComponent grp)
	{
		if (!m_Commander || !m_ArtySupport || !request)
			return;

		float worldTime = GetGame().GetWorld().GetWorldTime() / 1000.0;

		// === ADDED: safety check yang sebelumnya gak pernah beneran dipanggil ===
		if (m_ArtySupport.HasFriendlyNearPosDefault(request.m_vImpactPos))
		{
			Print("[DCO_ThreatResponse] Fire mission manual DITOLAK -- friendly kedeteksi di area target");
			return;
		}
		// === END ADDED ===

		DispatchArtilleryRequest(request, worldTime, "manual");
	}

	// === ADDED: satu pipeline final buat SEMUA jalur artillery (manual, cluster-auto,
	// simple-request) -- shellCount dihitung sekali di sini lewat CalculateArtilleryShellCount,
	// gak lagi kepisah-pisah tiap caller ngitung sendiri. ===
	protected void DispatchArtilleryRequest(CMD_FireMissionRequest request, float worldTime, string sourceTag)
	{
		if (!m_ArtySupport || !request)
			return;

		int shellNum = CalculateArtilleryShellCount(request, m_fArtilleryAccuracy, worldTime);

		Print(string.Format("[DCO_ThreatResponse] Fire mission (%1) -> %2 shell @ %3",
			sourceTag, shellNum, request.m_vImpactPos.ToString()));

		m_ArtySupport.RequestShellImpact(request.m_vImpactPos, request.m_eShellType, worldTime, shellNum);
	}
	// === END ADDED ===

	// === MODIFIED: unifikasi shellCount -- sebelumnya request.m_iShellCount (yang
	// diisi caller, misal berdasarkan enemyCount yang mereka liat) SAMA SEKALI GAK
	// DIPAKE, di-override total sama hardcoded base per shell-type. Sekarang
	// request.m_iShellCount jadi BASE (context caller dihormati), fungsi ini cuma
	// nambahin modifier universal (accuracy/staleness) di atasnya. Satu pipeline,
	// dipake bareng oleh SEMUA jalur artillery (manual/cluster-auto/simple-request). ===
	int CalculateArtilleryShellCount(CMD_FireMissionRequest request, float accuracy, float worldTime)
	{
	    float shells = Math.Max(1, request.m_iShellCount);

	    if (accuracy < 0.3)
	        shells += 3;
	    else if (accuracy < 0.5)
	        shells += 2;
	    else if (accuracy < 0.7)
	        shells += 1;
	    else if (accuracy >= 0.85)
	        shells -= 1;

	    float dataAge = worldTime - request.m_fRequestedTime;
	    if (dataAge > 60.0)
	        shells += 2;
	    else if (dataAge > 30.0)
	        shells += 1;

	    return Math.Clamp(shells, 1, 12);
	}

	//--------------------------------------------------------------------
	//! Entry point buat reinforcement request LANGSUNG/MANUAL dari 1 group spesifik
	//! (bukan jalur otomatis cluster). Tetap dipertahankan buat kasus dimana ada
	//! kode lain yang mau minta bantuan langsung tanpa nunggu Think() cycle.
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

		DispatchReinforcement(threat.m_vPosition, threat.m_eThreatLevel, threat, worldTime);
	}

	//--------------------------------------------------------------------
	protected void Think(float worldTime)
	{
		PurgeExpiredThreats(worldTime);
		MergeNearbyThreats();

		// Pass 1: scoring & recon per-entry individual. Recon soal verifikasi 1
		// laporan spesifik yang basi, jadi tetep per-entry, bukan per-cluster.
		foreach (CMD_ThreatEntry threat : m_aThreats)
		{
			if (!threat)
				continue;

			ScoreThreat(threat, worldTime);
			ClassifyThreat(threat);

			if (threat.m_bNeedsRecon && !threat.m_bReconSent)
				TrySendReconForThreat(threat, worldTime);
		}

		// Pass 2: cluster jadi hot-zone, evaluasi respons AGGREGATE.
		array<ref CMD_ThreatCluster> clusters = BuildThreatClusters();
		foreach (CMD_ThreatCluster cluster : clusters)
		{
			if (!cluster || cluster.m_aMembers.IsEmpty())
				continue;

			// === "Kalo ga begitu bahaya ya gak usah dikirim" ===
			if (cluster.m_fCombinedScore < m_fClusterMinResponseScore)
				continue;

			if (cluster.m_eClusterLevel == CMD_EThreatLevel.HIGH || cluster.m_eClusterLevel == CMD_EThreatLevel.CRITICAL)
				TrySendCounterFlank(cluster, worldTime);

			TrySendClusterReinforcement(cluster, worldTime);
			TrySendClusterArtillery(cluster, worldTime);
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

	// === MODIFIED: logic score->level diekstrak jadi ClassifyScore, dipake bareng
	// buat entry individual (ClassifyThreat) DAN buat combined score cluster
	// (FinalizeCluster) -- biar konsisten satu definisi threshold ===
	protected CMD_EThreatLevel ClassifyScore(float score)
	{
		if (score < 20.0)
			return CMD_EThreatLevel.NEGLIGIBLE;
		else if (score < m_fEngageThreshold)
			return CMD_EThreatLevel.LOW;
		else if (score < 55.0)
			return CMD_EThreatLevel.MEDIUM;
		else if (score < 75.0)
			return CMD_EThreatLevel.HIGH;
		else
			return CMD_EThreatLevel.CRITICAL;
	}

	protected void ClassifyThreat(CMD_ThreatEntry threat)
	{
		threat.m_eThreatLevel = ClassifyScore(threat.m_fPriorityScore);
	}
	// === END MODIFIED ===

	//--------------------------------------------------------------------
	// === ADDED: Threat Clustering ===
	//! Flood-fill/BFS clustering -- transitif, jadi kalau A-B deket dan B-C deket
	//! tapi A-C sendiri di luar radius, ketiganya tetep kegabung jadi 1 cluster
	//! (beda sama MergeNearbyThreats yang cuma pairwise buat dedup laporan sama).
	protected ref array<ref CMD_ThreatCluster> BuildThreatClusters()
	{
		array<ref CMD_ThreatCluster> clusters = new array<ref CMD_ThreatCluster>();

		array<bool> visited = {};
		for (int i = 0; i < m_aThreats.Count(); i++)
			visited.Insert(false);

		for (int i = 0; i < m_aThreats.Count(); i++)
		{
			if (visited[i])
				continue;

			if (!m_aThreats[i])
			{
				visited[i] = true;
				continue;
			}

			CMD_ThreatCluster cluster = new CMD_ThreatCluster();
			array<int> toVisit = {i};
			visited[i] = true;

			while (!toVisit.IsEmpty())
			{
				int idx = toVisit[0];
				toVisit.Remove(0);

				CMD_ThreatEntry current = m_aThreats[idx];
				if (!current)
					continue;

				cluster.m_aMembers.Insert(current);

				for (int j = 0; j < m_aThreats.Count(); j++)
				{
					if (visited[j])
						continue;

					CMD_ThreatEntry candidate = m_aThreats[j];
					if (!candidate)
					{
						visited[j] = true;
						continue;
					}

					if (vector.DistanceSq(current.m_vPosition, candidate.m_vPosition) <= m_fClusterSQ)
					{
						visited[j] = true;
						toVisit.Insert(j);
					}
				}
			}

			FinalizeCluster(cluster);
			clusters.Insert(cluster);
		}

		return clusters;
	}

	protected void FinalizeCluster(CMD_ThreatCluster cluster)
	{
		vector sumPos      = vector.Zero;
		int totalEnemies   = 0;
		float combinedScore = 0.0;
		float freshest     = 0.0;

		foreach (CMD_ThreatEntry e : cluster.m_aMembers)
		{
			if (!e)
				continue;

			sumPos        += e.m_vPosition;
			totalEnemies  += e.m_iEstimatedEnemyCount;
			combinedScore += e.m_fPriorityScore;

			if (e.m_fLastUpdateTime > freshest)
				freshest = e.m_fLastUpdateTime;
		}

		int count = cluster.m_aMembers.Count();
		if (count > 0)
			cluster.m_vCenterPos = sumPos / count;

		cluster.m_iTotalEstimatedEnemies = totalEnemies;
		cluster.m_fCombinedScore         = combinedScore;
		cluster.m_fFreshestUpdateTime    = freshest;
		cluster.m_eClusterLevel          = ClassifyScore(combinedScore);
	}
	// === END ADDED ===

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

		DCO_GroupUtilityComponent reconGrp = m_Commander.FindBestIdleGroupForRole_Public(CMD_EGroupRole.RECON, threat.m_vPosition);
		if (!reconGrp)
			reconGrp = m_Commander.FindBestIdleGroupForRole_Public(CMD_EGroupRole.RESERVE, threat.m_vPosition);
		if (!reconGrp)
			return;

		SCR_AIWaypoint wp = m_Commander.SpawnMoveWP(threat.m_vPosition);
		if (!wp)
			return;

		reconGrp.CompleteAllWaypoints();

		reconGrp.SetGroupRole(CMD_EGroupRole.RECON);
		reconGrp.MoveTo(wp, worldTime);
		threat.m_bReconSent = true;
	}

	protected DCO_GroupUtilityComponent FindClosestGroupForRole(CMD_EGroupRole role, vector pos)
	{
	    if (!m_Commander)
	        return null;

	    return m_Commander.FindBestIdleGroupForRole_Public(role, pos);
	}

	// === MODIFIED: sekarang beroperasi di level cluster (target = centroid cluster,
	// state di-track di primary member cluster) ===
	protected void TrySendCounterFlank(CMD_ThreatCluster cluster, float worldTime)
	{
		if (!m_Commander)
			return;

		CMD_ThreatEntry primary = cluster.GetPrimaryMember();
		if (!primary || primary.m_bFlankSent)
			return;

		DCO_GroupUtilityComponent flankGrp = m_Commander.FindBestIdleGroupForRole_Public(CMD_EGroupRole.FLANK, cluster.m_vCenterPos);
		if (!flankGrp)
			flankGrp = m_Commander.FindBestIdleGroupForRole_Public(CMD_EGroupRole.REINFORNCE, cluster.m_vCenterPos);
		if (!flankGrp)
			return;

		vector commanderPos = m_Commander.GetOwner().GetOrigin();
		vector flankPos     = m_Commander.ComputeFlankPosition(commanderPos, cluster.m_vCenterPos, m_fFlankDistance);
		flankPos[1]         = GetGame().GetWorld().GetSurfaceY(flankPos[0], flankPos[2]);

		SCR_AIWaypoint wp = m_Commander.SpawnMoveWP(flankPos);
		if (!wp)
			return;

		flankGrp.CompleteAllWaypoints();

		flankGrp.SetGroupRole(CMD_EGroupRole.FLANK);
		flankGrp.MoveTo(wp, worldTime);
		primary.m_bFlankSent = true;
	}
	// === END MODIFIED ===

	// === ADDED: reinforcement otomatis dari cluster, dipanggil langsung dari Think() ===
	protected void TrySendClusterReinforcement(CMD_ThreatCluster cluster, float worldTime)
	{
		if (!m_Commander)
			return;

		CMD_ThreatEntry primary = cluster.GetPrimaryMember();
		if (!primary)
			return;

		if (worldTime - primary.m_fLastReinforcementTime < m_fReinforcementCooldown)
			return;

		if (primary.m_iReinforcementSentNumber >= m_iMaxReinforcementSent)
			return;

		if (cluster.m_fCombinedScore < m_fReinforcementThreshold)
			return;

		DispatchReinforcement(cluster.m_vCenterPos, cluster.m_eClusterLevel, primary, worldTime);
	}
	// === END ADDED ===

	// === MODIFIED: sebelumnya "SendReinforcement(threat, worldTime)", sekarang
	// generic -- nerima posisi/level target + entry yang nyimpen cooldown/count
	// state, biar bisa dipake baik dari cluster (otomatis) maupun request manual ===
	protected void DispatchReinforcement(vector targetPos, CMD_EThreatLevel level, CMD_ThreatEntry stateHolder, float worldTime)
	{
	    if (!m_Commander)
	        return;

	    int slotsLeft = m_iMaxReinforcementSent - stateHolder.m_iReinforcementSentNumber;
	    if (slotsLeft <= 0)
	        return;

	    int sentThisCall = 0;

	    while (sentThisCall < slotsLeft)
	    {
	        DCO_GroupUtilityComponent reinforcement = null;
	        bool armored = false;

	        if (level >= CMD_EThreatLevel.HIGH)
	        {
	            reinforcement = FindClosestGroupForRole(CMD_EGroupRole.ARMORED, targetPos);
	            if (reinforcement)
	            {
	                armored = true;
	            }
	            else
	            {
	                reinforcement = FindClosestGroupForRole(CMD_EGroupRole.REINFORNCE, targetPos);
	                armored = false;
	            }
	        }
	        else
	        {
	            reinforcement = FindClosestGroupForRole(CMD_EGroupRole.REINFORNCE, targetPos);
	        }

	        if (!reinforcement)
	            break;

	        if (m_Commander.TryAssignTransport(reinforcement, targetPos, worldTime))
	        {
	            stateHolder.m_iReinforcementSentNumber++;
	            stateHolder.m_bReinforcementSent     = true;
	            stateHolder.m_fLastReinforcementTime = worldTime;
	            sentThisCall++;

	            continue;
	        }

	        SCR_AIWaypoint wp = m_Commander.SpawnMoveWP(targetPos);
	        if (!wp)
	            break;

	        if (!armored)
	            reinforcement.SetGroupRole(CMD_EGroupRole.REINFORNCE);

			reinforcement.CompleteAllWaypoints();

	        reinforcement.MoveTo(wp, worldTime);

	        stateHolder.m_iReinforcementSentNumber++;
	        stateHolder.m_bReinforcementSent     = true;
	        stateHolder.m_fLastReinforcementTime = worldTime;
	        sentThisCall++;

	        Print(string.Format("[DCO_ThreatResponse] Reinforcement %1/%2 (foot/motor) dikirim ke %3",
	            stateHolder.m_iReinforcementSentNumber,
	            m_iMaxReinforcementSent,
	            targetPos.ToString()));
	    }

	    if (sentThisCall > 0)
	    {
	        Print(string.Format("[DCO_ThreatResponse] Total %1 group dikirim, slot terisi %2/%3",
	            sentThisCall,
	            stateHolder.m_iReinforcementSentNumber,
	            m_iMaxReinforcementSent));
	    }
	}
	// === END MODIFIED ===

	// === ADDED: artillery otomatis dari cluster -- milih shell type berdasarkan
	// kondisi cluster (kesegaran intel, level ancaman, ada friendly deket apa
	// enggak). Dispersion/accuracy tembakan beneran dihitung sepenuhnya oleh
	// CMD_ArtillerySupport, di sini cuma nentuin POSISI, JENIS SHELL, dan JUMLAH. ===
	protected void TrySendClusterArtillery(CMD_ThreatCluster cluster, float worldTime)
	{
		if (!m_Commander || !m_ArtySupport)
			return;

		CMD_ThreatEntry primary = cluster.GetPrimaryMember();
		if (!primary)
			return;

		if (worldTime - primary.m_fLastArtilleryTime < m_fArtilleryCooldown)
			return;

		// Safety -- jangan tembak kalau ada friendly deket cluster ini
		if (m_ArtySupport.HasFriendlyNearPosDefault(cluster.m_vCenterPos))
			return;

		bool isFresh = (worldTime - cluster.m_fFreshestUpdateTime) < m_fStalenessThreshold;

		SCR_EAIArtilleryAmmoType shellType;
		if (!isFresh)
		{
			// Intel udah agak basi -- jangan gambling HE ke posisi yang mungkin
			// udah gak akurat, smoke buat disrupt/obscure aja
			shellType = SCR_EAIArtilleryAmmoType.SMOKE;
		}
		else if (cluster.m_eClusterLevel == CMD_EThreatLevel.CRITICAL || cluster.m_eClusterLevel == CMD_EThreatLevel.HIGH)
		{
			shellType = SCR_EAIArtilleryAmmoType.HIGH_EXPLOSIVE;
		}
		else
		{
			// MEDIUM ke bawah gak worth artillery -- biar reinforcement aja yang handle
			return;
		}

		// === MODIFIED: CalculateClusterShellCount sekarang cuma nentuin BASE count
		// (context cluster -- enemy count, staleness), lalu dioper ke pipeline shared
		// DispatchArtilleryRequest yang nambahin modifier accuracy di atasnya. Gak lagi
		// langsung manggil RequestShellImpact di sini. ===
		int baseShellCount = CalculateClusterShellCount(cluster, worldTime);

		CMD_FireMissionRequest request = new CMD_FireMissionRequest(cluster.m_vCenterPos, shellType, worldTime, baseShellCount, cluster.m_fFreshestUpdateTime, primary.m_fReportQuality);
		DispatchArtilleryRequest(request, worldTime, "cluster-auto");

		primary.m_bArtilleryCalled   = true;
		primary.m_fLastArtilleryTime = worldTime;
	}
	// === END MODIFIED ===

	protected int CalculateClusterShellCount(CMD_ThreatCluster cluster, float worldTime)
	{
		float shells = 2.0 + (cluster.m_iTotalEstimatedEnemies * 0.5);

		float age = worldTime - cluster.m_fFreshestUpdateTime;
		if (age > 30.0)
			shells += 1.0;

		return Math.Clamp(Math.Round(shells), 1, 12);
	}
	// === END ADDED ===

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

	// === ADDED: Simple Artillery Request ===
	//! Beda sama FindNearbyThreat (yang radius-nya ketat, buat dedup laporan sama) --
	//! ini nyari threat KNOWN terdekat TANPA batas radius, dipake buat resolve posisi
	//! target request artillery sederhana. maxDist opsional buat nyegah nembak ke
	//! threat yang kejauhan buat dianggap relevan sama group yang minta.
	protected CMD_ThreatEntry FindClosestKnownThreat(vector pos, float maxDist = -1.0)
	{
		CMD_ThreatEntry best = null;
		float bestDistSq = -1.0;
		float maxDistSq = maxDist * maxDist;

		for (int i = 0; i < m_aThreats.Count(); i++)
		{
			CMD_ThreatEntry t = m_aThreats[i];
			if (!t)
				continue;

			float distSq = vector.DistanceSq(t.m_vPosition, pos);

			if (maxDist > 0 && distSq > maxDistSq)
				continue;

			if (bestDistSq < 0.0 || distSq < bestDistSq)
			{
				bestDistSq = distSq;
				best = t;
			}
		}

		return best;
	}

	//! Entry point buat GROUP yang butuh artillery support tapi gak tau/gak perlu
	//! tau posisi spesifik musuh -- cuma bilang jenis efek yang dimau (HE/Smoke/dll),
	//! posisi target otomatis diresolve dari threat TERDEKAT yang udah diketahui
	//! commander (dari laporan kontak sebelumnya lewat ReceiveContactReport).
	//! Return false kalau gak ada threat yang diketahui di sekitar grup (gak ada
	//! yang bisa di-target), atau kalau ada friendly di area target (safety).
	bool RequestArtillerySupportSimple(DCO_GroupUtilityComponent requestingGrp, SCR_EAIArtilleryAmmoType desiredShellType, float worldTime, float maxSearchDist = 300.0)
	{
		if (!requestingGrp || !m_ArtySupport)
			return false;

		vector grpPos = requestingGrp.GetOwner().GetOrigin();

		CMD_ThreatEntry target = FindClosestKnownThreat(grpPos, maxSearchDist);
		if (!target)
		{
			Print("[DCO_ThreatResponse] RequestArtillerySupportSimple GAGAL -- gak ada threat yang diketahui di sekitar grup");
			return false;
		}

		if (m_ArtySupport.HasFriendlyNearPosDefault(target.m_vPosition))
		{
			Print("[DCO_ThreatResponse] RequestArtillerySupportSimple DITOLAK -- friendly kedeteksi di area target");
			return false;
		}

		// === MODIFIED: base shellCount dari enemyCount target, lalu lewat pipeline
		// shared (DispatchArtilleryRequest) biar modifier accuracy/staleness konsisten
		// sama 2 jalur lainnya (manual, cluster-auto) ===
		int baseShellCount = 3;
		if (target.m_iEstimatedEnemyCount > 3)
			baseShellCount = 5;

		CMD_FireMissionRequest request = new CMD_FireMissionRequest(target.m_vPosition, desiredShellType, worldTime, baseShellCount, target.m_fLastUpdateTime, target.m_fReportQuality);
		DispatchArtilleryRequest(request, worldTime, "simple-request");
		return true;
	}
	// === END MODIFIED ===

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

		m_fMergeSQ   = m_fMergeRadius * m_fMergeRadius;
		m_fClusterSQ = m_fClusterRadius * m_fClusterRadius;
		
		// === ADDED: BUG FIX -- m_fThinkTimer sebelumnya mulai dari 0.0 SAMA PERSIS di
		// semua instance (satu per commander/faction), dan m_fThinkInterval-nya fixed
		// (gak di-scale personality apapun kayak AICommander_BaseComponent). Efeknya:
		// SEMUA faction's threat response (BFS clustering + reinforcement + artillery
		// evaluation) nembak bareng di FRAME YANG SAMA, tiap 45 detik, SELAMANYA --
		// gak ada mekanisme desync apapun. Ini kemungkinan besar penyebab periodic
		// freeze yang dilaporkan. Fix: kasih jitter random per-instance ke starting
		// timer, biar tiap commander punya offset sendiri-sendiri.
		m_fThinkTimer = Math.RandomFloat(0.0, m_fThinkInterval);
		// === END ADDED ===
	}
}

// === ADDED: Threat Clustering ===
//! Grouping sementara dari beberapa CMD_ThreatEntry yang saling berdekatan --
//! DIBANGUN ULANG tiap Think() cycle (gak persisten), state cooldown/sent tetep
//! disimpen di CMD_ThreatEntry masing-masing (lewat GetPrimaryMember()), jadi
//! gak butuh identity cluster yang stabil antar cycle.
class CMD_ThreatCluster
{
	vector m_vCenterPos;
	int m_iTotalEstimatedEnemies = 0;
	float m_fCombinedScore = 0.0;
	CMD_EThreatLevel m_eClusterLevel = CMD_EThreatLevel.NEGLIGIBLE;
	float m_fFreshestUpdateTime = 0.0;
	ref array<CMD_ThreatEntry> m_aMembers = new array<CMD_ThreatEntry>();

	//! Member dengan priority score individu tertinggi -- carrier buat cooldown/
	//! sent-state seluruh cluster ini (biar gak butuh persistent cluster identity).
	CMD_ThreatEntry GetPrimaryMember()
	{
		CMD_ThreatEntry best = null;
		float bestScore = -1.0;

		foreach (CMD_ThreatEntry e : m_aMembers)
		{
			if (!e)
				continue;

			if (e.m_fPriorityScore > bestScore)
			{
				bestScore = e.m_fPriorityScore;
				best = e;
			}
		}

		return best;
	}
}
// === END ADDED ===