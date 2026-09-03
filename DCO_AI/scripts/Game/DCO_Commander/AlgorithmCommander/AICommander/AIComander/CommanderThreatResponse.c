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

	[Attribute("90.0", UIWidgets.EditBox, "Cooldown (detik) artillery per-cluster", category: "Artillery")]
	protected float m_fArtilleryCooldown;

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

		float reportQuality = 0.7;
		if (grp && grp.GetOwner())
		{
			float spotDistance = vector.Distance(grp.GetOwner().GetOrigin(), report.m_vPosition);
			reportQuality = ComputeReportQuality(spotDistance);
		}

		CMD_ThreatEntry existing = FindNearbyThreat(report.m_vPosition);
		if (existing)
		{
			existing.m_vPosition            = report.m_vPosition;
			existing.m_iEstimatedEnemyCount = Math.Max(existing.m_iEstimatedEnemyCount, report.m_iEstimatedEnemyCount);
			existing.m_fLastUpdateTime      = worldTime;
			existing.m_bNeedsRecon          = false;
			existing.m_bReconSent           = false;
			existing.m_fReportQuality       = reportQuality;
			return;
		}

		CMD_ThreatEntry entry = new CMD_ThreatEntry(report.m_vPosition, report.m_iEstimatedEnemyCount, worldTime, grp);
		entry.m_fReportQuality = reportQuality;
		m_aThreats.Insert(entry);
	}
	
	array<ref CMD_ThreatEntry> GetThreats()
	{
		return m_aThreats;
	}

	static float ComputeReportQuality(float spotDistance)
	{
		float distanceQuality = Math.Clamp(1.0 - (spotDistance - 50.0) / 350.0, 0.2, 1.0);
		float timeOfDayFactor = ComputeTimeOfDayFactor();
		return Math.Clamp(distanceQuality * timeOfDayFactor, 0.2, 1.0);
	}

	protected static float ComputeTimeOfDayFactor()
	{
		ChimeraWorld world = ChimeraWorld.CastFrom(GetGame().GetWorld());
		if (!world)
			return 1.0;
		
		TimeAndWeatherManagerEntity manager = world.GetTimeAndWeatherManager();
		if (!manager)
			return 1.0;
		
		float sunriseTime, sunsetTime;
		if (!manager.GetSunriseHour(sunriseTime) || !manager.GetSunsetHour(sunsetTime))
			return 1.0;
		
		float currentTime = manager.GetTimeOfTheDay();
		bool isDaytime = (currentTime >= sunriseTime && currentTime <= sunsetTime);
		
		if (isDaytime)
		{
			float middayTime     = (sunriseTime + sunsetTime) * 0.5;
			float halfDayLength  = Math.Max((sunsetTime - sunriseTime) * 0.5, 0.01);
			float distFromMidday = Math.AbsFloat(currentTime - middayTime);
			float dayProgress    = Math.Clamp(distFromMidday / halfDayLength, 0.0, 1.0);
			
			return Math.Lerp(1.0, 0.85, dayProgress);
		}
		else
		{
			float nightLength      = Math.Max(24.0 - (sunsetTime - sunriseTime), 0.01);
			float distFromSunset   = currentTime - sunsetTime;
			if (distFromSunset < 0.0)
				distFromSunset += 24.0;
			
			float midnightPoint    = nightLength * 0.5;
			float distFromMidnight = Math.AbsFloat(distFromSunset - midnightPoint);
			float nightProgress    = Math.Clamp(distFromMidnight / Math.Max(midnightPoint, 0.01), 0.0, 1.0);
			
			return Math.Lerp(0.5, 0.85, nightProgress);
		}
	}

	void ReceiveArtillerySupport(CMD_FireMissionRequest request, DCO_GroupUtilityComponent grp)
	{
		if (!m_Commander || !m_ArtySupport || !request)
			return;

		float worldTime = GetGame().GetWorld().GetWorldTime() / 1000.0;

		if (m_ArtySupport.HasFriendlyNearPosDefault(request.m_vImpactPos))
		{
			Print("[DCO_ThreatResponse] Fire mission manual DITOLAK -- friendly kedeteksi di area target");
			return;
		}

		DispatchArtilleryRequest(request, worldTime, "manual");
	}

	protected void DispatchArtilleryRequest(CMD_FireMissionRequest request, float worldTime, string sourceTag)
	{
		if (!m_ArtySupport || !request)
			return;

		if (!m_ArtySupport.HasRegisteredUnits())
		{
			Print(string.Format("[DCO_ThreatResponse] Fire mission (%1) DITOLAK -- gak ada artillery unit yang terdaftar sama sekali", sourceTag));
			return;
		}

		int shellNum = CalculateArtilleryShellCount(request, m_fArtilleryAccuracy, worldTime);

		Print(string.Format("[DCO_ThreatResponse] Fire mission (%1) -> %2 shell @ %3",
			sourceTag, shellNum, request.m_vImpactPos.ToString()));

		m_ArtySupport.RequestShellImpact(request.m_vImpactPos, request.m_eShellType, worldTime, shellNum);
	}

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

		float combatFocusMod = Math.Lerp(1.8, 0.4, m_Commander.GetCombatFocus());
		float effectiveThreshold = m_fReinforcementThreshold * combatFocusMod;

		if (threat.m_fPriorityScore < effectiveThreshold)
			return;

		DispatchReinforcement(threat.m_vPosition, threat.m_eThreatLevel, threat, worldTime);
	}

	//--------------------------------------------------------------------
	protected void Think(float worldTime)
	{
		PurgeExpiredThreats(worldTime);
		MergeNearbyThreats();

		foreach (CMD_ThreatEntry threat : m_aThreats)
		{
			if (!threat)
				continue;

			ScoreThreat(threat, worldTime);
			ClassifyThreat(threat);

			if (threat.m_bNeedsRecon && !threat.m_bReconSent)
				TrySendReconForThreat(threat, worldTime);
		}

		array<ref CMD_ThreatCluster> clusters = BuildThreatClusters();
		foreach (CMD_ThreatCluster cluster : clusters)
		{
			if (!cluster || cluster.m_aMembers.IsEmpty())
				continue;

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

		float combatFocusMod = Math.Lerp(1.8, 0.4, m_Commander.GetCombatFocus());
		float effectiveThreshold = m_fReinforcementThreshold * combatFocusMod;

		if (cluster.m_fCombinedScore < effectiveThreshold)
			return;

		DispatchReinforcement(cluster.m_vCenterPos, cluster.m_eClusterLevel, primary, worldTime);
	}

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

	protected void TrySendClusterArtillery(CMD_ThreatCluster cluster, float worldTime)
	{
		if (!m_Commander || !m_ArtySupport)
			return;

		CMD_ThreatEntry primary = cluster.GetPrimaryMember();
		if (!primary)
			return;

		if (worldTime - primary.m_fLastArtilleryTime < m_fArtilleryCooldown)
			return;

		if (m_ArtySupport.HasFriendlyNearPosDefault(cluster.m_vCenterPos))
			return;

		bool isFresh = (worldTime - cluster.m_fFreshestUpdateTime) < m_fStalenessThreshold;

		SCR_EAIArtilleryAmmoType shellType;
		if (!isFresh)
		{
			shellType = SCR_EAIArtilleryAmmoType.SMOKE;
		}
		else if (cluster.m_eClusterLevel == CMD_EThreatLevel.CRITICAL || cluster.m_eClusterLevel == CMD_EThreatLevel.HIGH)
		{
			shellType = SCR_EAIArtilleryAmmoType.HIGH_EXPLOSIVE;
		}
		else
		{
			return;
		}

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

		int baseShellCount = 3;
		if (target.m_iEstimatedEnemyCount > 3)
			baseShellCount = 5;

		CMD_FireMissionRequest request = new CMD_FireMissionRequest(target.m_vPosition, desiredShellType, worldTime, baseShellCount, target.m_fLastUpdateTime, target.m_fReportQuality);
		DispatchArtilleryRequest(request, worldTime, "simple-request");
		return true;
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

		m_fMergeSQ   = m_fMergeRadius * m_fMergeRadius;
		m_fClusterSQ = m_fClusterRadius * m_fClusterRadius;

		m_fThinkTimer = Math.RandomFloat(0.0, m_fThinkInterval);
	}
}

class CMD_ThreatCluster
{
	vector m_vCenterPos;
	int m_iTotalEstimatedEnemies = 0;
	float m_fCombinedScore = 0.0;
	CMD_EThreatLevel m_eClusterLevel = CMD_EThreatLevel.NEGLIGIBLE;
	float m_fFreshestUpdateTime = 0.0;
	ref array<CMD_ThreatEntry> m_aMembers = new array<CMD_ThreatEntry>();

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