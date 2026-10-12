[ComponentEditorProps(category: "GameScripted/Commander", description: "Handles enemy contact response and reinforcement")]
class CMD_ThreatResponseComponentClass : ScriptComponentClass {}

class CMD_ThreatResponseComponent : ScriptComponent
{
	static bool IsNight()
	{
		ChimeraWorld world = ChimeraWorld.CastFrom(GetGame().GetWorld());
		if (!world)
			return false;
		TimeAndWeatherManagerEntity manager = world.GetTimeAndWeatherManager();
		float sunrise, sunset;
		if (!manager || !manager.GetSunriseHour(sunrise) || !manager.GetSunsetHour(sunset))
			return false;
		float now = manager.GetTimeOfTheDay();
		return now < sunrise || now > sunset;
	}

	[Attribute("30.0", UIWidgets.EditBox, "Minimum score to send a normal reinforcement.", category: "Threat")]
	protected float m_fEngageThreshold;

	[Attribute("70.0", UIWidgets.EditBox, "Minimum score for a priority reinforcement.", category: "Threat")]
	protected float m_fReinforcementThreshold;

	[Attribute("60.0", UIWidgets.EditBox, "Seconds before a threat entry expires and is removed.", category: "Threat")]
	protected float m_fThreatExpiry;

	[Attribute("70.0", UIWidgets.EditBox, "Distance (m) at which two reports count as the same threat (dedup/merge).", category: "Threat")]
	protected float m_fMergeRadius;

	[Attribute("150.0", UIWidgets.EditBox, "Distance (m) for grouping threats into one response cluster. Larger than the merge radius, because this decides how we respond, not report dedup.", category: "Threat")]
	protected float m_fClusterRadius;

	[Attribute("25.0", UIWidgets.EditBox, "Minimum combined cluster score before an active response (reinforcement, flank, artillery) is sent. Below this the threat is ignored.", category: "Threat")]
	protected float m_fClusterMinResponseScore;

	[Attribute("45.0", UIWidgets.EditBox, "Threat list think cycle interval (s).", category: "Threat")]
	protected float m_fThinkInterval;

	[Attribute("2", UIWidgets.EditBox, "Maximum reinforcements sent to one cluster.", category: "Reinforcement")]
	protected int m_iMaxReinforcementSent;

	[Attribute("120.0", UIWidgets.EditBox, "Cooldown (s) between reinforcements sent to the same cluster.", category: "Reinforcement")]
	protected float m_fReinforcementCooldown;

	[Attribute("1", UIWidgets.EditBox, "Score added per detected enemy.", category: "Threat Scoring")]
	protected int m_iEnemyIncrementedScore;

	[Attribute("30.0", UIWidgets.EditBox, "Seconds before intel counts as stale.", category: "Intel Decay")]
	protected float m_fStalenessThreshold;

	[Attribute("120.0", UIWidgets.EditBox, "Counter-flank distance from the threat position (m).", category: "Flanking")]
	protected float m_fFlankDistance;

	[Attribute("0.5", UIWidgets.Range, "Artillery accuracy for manual or direct requests (0-1).", params: "0 1 0.01", category: "Artillery")]
	protected float m_fArtilleryAccuracy;

	[Attribute("90.0", UIWidgets.EditBox, "Artillery cooldown per cluster (s).", category: "Artillery")]
	protected float m_fArtilleryCooldown;

	[Attribute("0", UIWidgets.CheckBox, desc: "Draw the threat system overlay: each contact report, its score and its response cluster. Only visible while Game Master is open.", category: "Threat Debug")]
	protected bool m_bDebugMode;

	[Attribute("0.5", UIWidgets.EditBox, "Interval (s) between threat overlay redraws.", category: "Threat Debug")]
	protected float m_fDebugRefreshInterval;

	protected ref array<ref Shape> m_aDebugShapes = new array<ref Shape>();
	protected ref array<ref DebugTextWorldSpace> m_aDebugTexts = new array<ref DebugTextWorldSpace>();
	protected float m_fDebugTimer = 0.0;

	protected ref array<ref CMD_ThreatCluster> m_aDebugClusters;

	[Attribute("300.0", UIWidgets.EditBox, "Seconds the response record per area (reinforcement count, artillery and flank cooldowns) lasts, even after the threat entry has expired.", category: "Reinforcement")]
	protected float m_fLedgerTTL;

	[Attribute("125.0", UIWidgets.EditBox, "Reinforcements are sent this far (m) BEHIND the friendly group in contact, along the friendly-to-enemy line.", category: "Reinforcement")]
	protected float m_fReinforceBehindDist;

	[Attribute("150.0", UIWidgets.EditBox, "Fallback when no friendly group is in contact: a standoff point this far (m) from the threat toward the commander.", category: "Reinforcement")]
	protected float m_fReinforceStandoffDist;

	[Attribute("180.0", UIWidgets.EditBox, "Maximum seconds a counter-flank group stays tasked before returning to RESERVE.", category: "Flanking")]
	protected float m_fFlankTaskExpiry;

	[Attribute("1.5", UIWidgets.EditBox, "Threat position uncertainty radius grows by this much (m per second of intel age).", category: "Intel Decay")]
	protected float m_fUncertaintyGrowth;

	[Attribute("60.0", UIWidgets.EditBox, "Extrapolate position from the movement direction for at most this long (s).", category: "Intel Decay")]
	protected float m_fExtrapolateMax;

	[Attribute("400", UIWidgets.EditBox, "Radius (m) for the local friendly vs enemy strength ratio around a threat.", category: "Threat Decision")]
	protected float m_fRatioRadius;

	[Attribute("0.6", UIWidgets.EditBox, "Strength ratio (friendly / weighted enemy) below this = outmatched; delay or withdraw become options.", category: "Threat Decision")]
	protected float m_fWithdrawRatio;

	[Attribute("600", UIWidgets.EditBox, "Maximum seconds a response may run without result before it times out (group pulled back, keep watching).", category: "Threat Decision")]
	protected float m_fMissionTimeout;

	[Attribute("120", UIWidgets.EditBox, "Maximum seconds a simultaneous attack release may be held because of a threat on the flank.", category: "Threat Decision")]
	protected float m_fHoldReleaseMax;

	[Attribute("100", UIWidgets.EditBox, "Reserve blocking position: this far (m) outside the objective radius toward the threat.", category: "Threat Decision")]
	protected float m_fBlockingDist;

	[Attribute("1", UIWidgets.CheckBox, "Prepare defenses: garrison mode Hold is raised to Defend.", category: "Threat Decision")]
	protected bool m_bUpgradeGarrisonMode;

	[Attribute("180", UIWidgets.EditBox, "Project the threat's movement this far ahead (s) to find threatened assets.", category: "Threat Decision")]
	protected float m_fProjectTime;

	protected ref array<ref CMD_ThreatMission> m_aMissions = {};
	protected ref map<CMD_AICommanderObjectiveComponent, float> m_mHoldRelease = new map<CMD_AICommanderObjectiveComponent, float>();
	protected static int s_iMissionId;

	protected AICommander_BaseComponent          m_Commander;
	protected CMD_ArtillerySupport				 m_ArtySupport;
	protected ref array<ref CMD_ThreatEntry>     m_aThreats    = new array<ref CMD_ThreatEntry>();
	protected ref array<ref CMD_ResponseLedgerEntry> m_aLedger = {};
	protected ref array<ref CMD_FlankTracker>    m_aFlankers   = {};
	protected ref map<int, string> m_mFlankSkipWhy = new map<int, string>();
	protected ref DCO_ThreatBook m_Book;
	protected float                              m_fThinkTimer = 0.0;
	protected float                              m_fMergeSQ    = 0.0;
	protected float                              m_fClusterSQ  = 0.0;

	void ReceiveContactReport(CMD_ContactReport report, DCO_GroupUtilityComponent grp)
	{
		int pt = DCO_Perf.Begin();
		DoReceiveContactReport(report, grp);
		DCO_Perf.End("cmd_contact_intake", pt);
	}


	protected void DoReceiveContactReport(CMD_ContactReport report, DCO_GroupUtilityComponent grp)
	{
		if (!report)
			return;

		float worldTime = report.m_fInfoTime;

		float reportQuality = 0.7;
		if (grp && grp.GetOwner())
		{
			float spotDistance = vector.Distance(grp.GetOwner().GetOrigin(), report.m_vPosition);
			reportQuality = ComputeReportQuality(spotDistance);
		}

		if (m_Commander && m_Commander.GetDefense())
		{
			m_Commander.GetDefense().OnContactReport(m_Commander, report, grp);
			if (grp && m_Commander.GetDefense().IsOP(grp))
				reportQuality = Math.Min(reportQuality + 0.2, 1.0);
		}

		CMD_ThreatEntry existing = FindNearbyThreat(report.m_vPosition);
		if (!existing && report.m_bLowPriority)
			existing = FindThreatWithin(report.m_vPosition, 75.0);
		if (existing)
		{
			if (worldTime < existing.m_fLastUpdateTime)
				return;
			existing.MergeReport(report);

			existing.m_vPosition            = report.m_vPosition;
			existing.m_iEstimatedEnemyCount = Math.Max(existing.m_iEstimatedEnemyCount, report.m_iEstimatedEnemyCount);
			existing.m_fLastUpdateTime      = worldTime;
			existing.m_bNeedsRecon          = false;
			existing.m_bReconSent           = false;
			existing.m_fReportQuality       = reportQuality;
			existing.m_bArmorSeen           = existing.m_bArmorSeen || report.m_bArmorSeen;
			existing.m_bATSeen              = existing.m_bATSeen || report.m_bATSeen;
			ApplyReportIntel(existing, report, grp);
			DCO_Battle.Intel(m_Commander, existing, false);
			return;
		}

		CMD_ThreatEntry entry = new CMD_ThreatEntry(report.m_vPosition, report.m_iEstimatedEnemyCount, worldTime, grp);
		entry.m_fReportQuality = reportQuality;
		entry.m_bArmorSeen     = report.m_bArmorSeen;
		entry.m_bATSeen        = report.m_bATSeen;
		entry.m_bLowPriority   = true;
		entry.MergeReport(report);
		ApplyReportIntel(entry, report, grp);
		m_aThreats.Insert(entry);
		DCO_Battle.Intel(m_Commander, entry, true);
	}

	protected void ApplyReportIntel(CMD_ThreatEntry e, CMD_ContactReport report, DCO_GroupUtilityComponent grp)
	{
		e.m_fUncertainty         = report.m_fUncertainty;
		e.m_vVelocity            = report.m_vVelocity;
		e.m_vTruePos             = report.m_vTruePos;
		e.m_vBelievedPos         = report.m_vPosition;
		e.m_fBelievedUncertainty = report.m_fUncertainty;
		if (grp)
			e.m_sEngagingGroupName = grp;
	}

	protected void UpdateBeliefs(float worldTime)
	{
		foreach (CMD_ThreatEntry t : m_aThreats)
		{
			if (!t)
				continue;

			float age = Math.Max(worldTime - t.m_fLastUpdateTime, 0);
			t.m_vBelievedPos = t.m_vPosition + t.m_vVelocity * Math.Min(age, m_fExtrapolateMax);
			t.m_vBelievedPos = DCO_Util.Surface(t.m_vBelievedPos);
			t.m_fBelievedUncertainty = t.m_fUncertainty + age * m_fUncertaintyGrowth;
		}
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

		float worldTime = DCO_Util.NowS();

		if (CMD_ArtillerySupport.IsLethal(request))
			m_ArtySupport.ApplyTier(request, m_ArtySupport.ResolveTier(request.m_fUncertainty), request.m_fUncertainty, request.m_sSource);

		if (request.m_sTier == "none")
		{
			CMD_ArtillerySupport.LogDenied(request, "uncertain");
			return;
		}

		if (m_ArtySupport.HasFriendlyNearRequest(request, worldTime))
		{
			CMD_ArtillerySupport.LogDenied(request, "friendly");
			return;
		}

		DispatchArtilleryRequest(request, worldTime, request.m_sSource);
	}

	protected void DispatchArtilleryRequest(CMD_FireMissionRequest request, float worldTime, string sourceTag)
	{
		if (!m_ArtySupport || !request)
			return;

		if (!m_ArtySupport.HasRegisteredUnits())
		{
			CMD_ArtillerySupport.LogDenied(request, "no_unit");
			return;
		}

		int shellNum = CalculateArtilleryShellCount(request, m_fArtilleryAccuracy, worldTime);

		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("Fire mission (%1) -> %2 shell @ %3",
			sourceTag, shellNum, request.m_vImpactPos.ToString()));

		m_ArtySupport.RequestShellImpact(request, worldTime, shellNum);
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

		CMD_ResponseLedgerEntry ledger = GetLedger(threat.m_vPosition, worldTime);
		if (worldTime - ledger.m_fLastReinforcementTime < m_fReinforcementCooldown)
			return;

		if (ledger.m_iReinforcements >= DCO_QrfMax())
			return;

		float combatFocusMod = Math.Lerp(1.8, 0.4, m_Commander.GetCombatFocus());
		float effectiveThreshold = m_fReinforcementThreshold * combatFocusMod;

		if (threat.m_fPriorityScore < effectiveThreshold)
			return;

		threat.m_sEngagingGroupName = requestingGrp;
		bool emergency = threat.m_eThreatLevel == CMD_EThreatLevel.CRITICAL && IsNearOwnObjective(threat.m_vPosition);
		DispatchReinforcement(threat.m_vPosition, threat.m_eThreatLevel, threat, worldTime, emergency);
	}

	protected CMD_ResponseLedgerEntry GetLedger(vector pos, float worldTime)
	{
		foreach (CMD_ResponseLedgerEntry e : m_aLedger)
		{
			if (vector.DistanceSq(e.m_vPos, pos) <= m_fClusterSQ)
			{
				e.m_fLastTouched = worldTime;
				return e;
			}
		}

		CMD_ResponseLedgerEntry created = new CMD_ResponseLedgerEntry();
		created.m_vPos = pos;
		created.m_fLastTouched = worldTime;
		m_aLedger.Insert(created);
		return created;
	}

	protected void PurgeLedger(float worldTime)
	{
		for (int i = m_aLedger.Count() - 1; i >= 0; i--)
		{
			if (worldTime - m_aLedger[i].m_fLastTouched > m_fLedgerTTL)
				m_aLedger.Remove(i);
		}
	}

	protected bool IsNearOwnObjective(vector pos)
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr || !m_Commander)
			return false;

		foreach (CMD_AICommanderObjectiveComponent obj : mgr.m_aObjective)
		{
			if (!obj || !obj.IsCapturedBy(m_Commander.GetCommanderFactionKey()))
				continue;

			float r = obj.GetRadius() * 1.5;
			if (vector.DistanceSq(pos, obj.GetOwner().GetOrigin()) <= r * r)
				return true;
		}
		return false;
	}

	protected void DCO_Shortfall(CMD_AICommanderObjectiveComponent obj, vector threatPos, float enemy, string path)
	{
		if (!obj || !DCO_DefShortfall.On(m_Commander))
			return;
		float d = vector.DistanceXZ(threatPos, obj.GetOwner().GetOrigin());
		int prio = 1;
		if (d < 500.0)
			prio = 2;
		DCO_Perf.Count("threat:shortfall_" + path);
		DCO_DefShortfall.Report(m_Commander, obj, DCO_DefShortfall.Floor(obj, enemy, false), DCO_DefShortfall.Defenders(m_Commander, obj), "threat", prio, d);
	}

	protected bool DCO_FloorBlocks(DCO_GroupUtilityComponent g, float enemy, string path)
	{
		if (!g || !DCO_DefShortfall.FloorOn(m_Commander))
			return false;
		CMD_AICommanderObjectiveComponent obj = g.GetGroupObjective();
		if (!obj || (g.GetTask() != DCO_EGroupTask.DEFEND && g.GetTask() != DCO_EGroupTask.GARRISON))
			return false;
		int have = DCO_DefShortfall.Defenders(m_Commander, obj);
		int floor = DCO_DefShortfall.Floor(obj, enemy, false);
		if (have - 1 >= floor)
			return false;
		DCO_Perf.Count("def:floor_block");
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("def_floor_block cmd=%1 grp=%2 obj=%3 have=%4 floor=%5 path=%6", m_Commander.GetCommanderUID(), DCO_Log.IdGroup(AIGroup.Cast(g.GetOwner())), obj.GetLogName(), have, floor, path));
		return true;
	}

	protected bool CanUseGroup(DCO_GroupUtilityComponent grp, bool emergency)
	{
		return grp && !TacticOwns(grp, "threat_pick", -1) && !m_Commander.DCO_MedicTeamBlocked(grp, DCO_EGroupTask.REINFORCE, "threat_pick") && (emergency || m_Commander.CanCommitGroup(grp));
	}

	protected bool TacticOwns(DCO_GroupUtilityComponent g, string path, int missionId)
	{
		if (!g || !m_Commander || !m_Commander.GetTactics() || !DCO_TacticsCompat.OwnsGroup(m_Commander.GetTactics(), g))
			return false;
		DCO_Perf.Count("threat_op_skip:tactic_plan");
		if (DCO_Log.On(DCO_ELogChannel.CMD, DCO_ELogLevel.DEBUG))
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.DEBUG, "-", string.Format("threat_support_skip grp=%1 reason=tactic_plan path=%2 mission=%3 task=%4", g.GetOwner().GetName(), path, missionId, typename.EnumToString(DCO_EGroupTask, g.GetTask())));
		return true;
	}

	protected vector ComputeReinforcementPoint(CMD_ThreatEntry holder, vector threatPos)
	{
		vector anchor;
		vector away;
		DCO_GroupUtilityComponent friend = holder.m_sEngagingGroupName;
		if (friend && friend.GetOwner() && friend.GetUnitCount() > 0)
		{
			anchor = friend.GetOwner().GetOrigin();
			away = anchor - threatPos;
			away[1] = 0;
			if (away.LengthSq() > 1)
			{
				away.Normalize();
				anchor = anchor + away * m_fReinforceBehindDist;
				anchor = DCO_Util.Surface(anchor);
				return anchor;
			}
		}

		away = m_Commander.GetOwner().GetOrigin() - threatPos;
		away[1] = 0;
		if (away.LengthSq() < 1)
			away = vector.Forward;
		away.Normalize();
		anchor = threatPos + away * m_fReinforceStandoffDist;
		anchor = DCO_Util.Surface(anchor);
		return anchor;
	}

	protected void UpdateFlankers(float worldTime)
	{
		for (int i = m_aFlankers.Count() - 1; i >= 0; i--)
		{
			CMD_FlankTracker t = m_aFlankers[i];
			if (!t.m_Group)
			{
				m_aFlankers.Remove(i);
				continue;
			}

			bool routeDone = !t.m_Group.IsMoving() && !t.m_Group.IsGroupHaveWaypoint();
			if (!routeDone && worldTime < t.m_fExpiry)
			{
				RetargetFlank(t, worldTime);
				continue;
			}

			DCO_EGroupTask task = t.m_Group.GetTask();
			bool release = task == DCO_EGroupTask.FLANK || (t.m_bArmored && task == DCO_EGroupTask.NONE && t.m_Group.m_iDCOThreatAct == CMD_EThreatAction.COUNTERATTACK);
			if (release)
			{
				t.m_Group.CompleteAllWaypoints();
				if (t.m_bArmored)
					t.m_Group.m_iDCOThreatAct = -1;
				else
					t.m_Group.SetTask(DCO_EGroupTask.NONE, "CommanderThreatResponse:452");
			}
			string why = "expired";
			if (routeDone)
				why = "route_done";
			DCO_Perf.Count("threat:flank_release_" + why);
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(m_Commander), string.Format("threat_flank_release mission=%1 grp=%2 why=%3 armored=%4 released=%5 task=%6 retargets=%7",
				t.m_iMission, t.m_Group.GetOwner().GetName(), why, t.m_bArmored, release, typename.EnumToString(DCO_EGroupTask, task), t.m_iRetargets));
			m_aFlankers.Remove(i);
		}
	}

	protected void Think(float worldTime)
	{
		PurgeExpiredThreats(worldTime);
		PurgeLedger(worldTime);
		UpdateFlankers(worldTime);
		MergeNearbyThreats();
		UpdateBeliefs(worldTime);

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

		if (m_bDebugMode)
			m_aDebugClusters = clusters;
		else
			m_aDebugClusters = null;

		if (!m_Commander)
			return;

		if (!m_Book)
			m_Book = new DCO_ThreatBook(m_Commander);
		float gate = m_fClusterMinResponseScore * Math.Lerp(1.4, 0.6, m_Commander.GetCombatFocus());
		foreach (CMD_ThreatMission ms : m_aMissions)
			ms.m_bSeen = false;

		int gateSkip;
		int gateLow;
		float gateBest = -1000;
		int gateBestN;
		foreach (CMD_ThreatCluster cluster : clusters)
		{
			if (!cluster || cluster.m_aMembers.IsEmpty())
				continue;

			CMD_ThreatMission mission = FindMission(cluster.m_vCenterPos);
			bool lowOnly = IsLowPriorityOnly(cluster, gate);
			if (!mission && (cluster.m_fCombinedScore < gate || lowOnly))
			{
				gateSkip++;
				if (lowOnly)
					gateLow++;
				if (cluster.m_fCombinedScore > gateBest)
				{
					gateBest = cluster.m_fCombinedScore;
					gateBestN = cluster.m_iTotalEstimatedEnemies;
				}
				NotifyReporters(cluster, null);
				continue;
			}

			CMD_ThreatAssessment a = Assess(cluster, worldTime);
			ScoreActions(a, cluster);

			if (!mission)
			{
				mission = new CMD_ThreatMission();
				mission.m_iId = ++s_iMissionId;
				mission.m_fStart = worldTime;
				mission.m_fDeadline = worldTime + m_fMissionTimeout;
				mission.m_iStartEnemies = a.m_iEnemies;
				m_aMissions.Insert(mission);
			}
			mission.m_bSeen = true;
			mission.m_vPos = cluster.m_vCenterPos;
			mission.m_Last = a;

			DCO_ThreatDossier dos = m_Book.Observe(cluster, a, mission, worldTime);
			m_Book.Bias(dos, a, cluster, worldTime);
			DecideAction(mission, a, worldTime);
			ExecuteAction(mission, cluster, a, worldTime);
			m_Book.Sync(dos, mission, worldTime);
			NotifyReporters(cluster, mission);
		}

		if (gateSkip > 0)
		{
			DCO_Perf.CountN("threat:gate_skip", gateSkip);
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("threat_gate_skip cmd=%1 clusters=%2 low_only=%3 best_score=%4 best_enemies=%5 gate=%6 focus=%7 missions=%8", m_Commander.GetCommanderUID(), gateSkip, gateLow, gateBest.ToString(-1, 1), gateBestN, gate.ToString(-1, 1), m_Commander.GetCombatFocus().ToString(-1, 2), m_aMissions.Count()));
		}
		ReviewMissions(worldTime);
		m_Book.Tick(worldTime);
	}

	protected bool IsLowPriorityOnly(CMD_ThreatCluster cluster, float gate)
	{
		foreach (CMD_ThreatEntry e : cluster.m_aMembers)
		{
			if (e && !e.m_bLowPriority)
				return false;
		}
		if (DCO_Ctx.TuneForCmd(m_Commander, "thr.low_cluster", 0.0) <= 0)
			return true;
		string why;
		if (cluster.m_iTotalEstimatedEnemies >= 3)
			why = "enemies";
		else if (cluster.m_fCombinedScore >= gate * 1.5)
			why = "score";
		else
			return true;
		DCO_Perf.Count("threat:low_override_" + why);
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("threat_low_override cmd=%1 why=%2 enemies=%3 score=%4 gate=%5 members=%6", m_Commander.GetCommanderUID(), why, cluster.m_iTotalEstimatedEnemies, cluster.m_fCombinedScore.ToString(-1, 1), gate.ToString(-1, 1), cluster.m_aMembers.Count()));
		return false;
	}

	protected bool HasPriorityMember(CMD_ThreatCluster cluster)
	{
		foreach (CMD_ThreatEntry e : cluster.m_aMembers)
		{
			if (e && e.IsPriorityTarget())
				return true;
		}
		return false;
	}

	protected CMD_ThreatMission FindMission(vector pos)
	{
		float r = m_fClusterRadius * 1.5;
		foreach (CMD_ThreatMission ms : m_aMissions)
		{
			if (vector.DistanceXZ(ms.m_vPos, pos) <= r)
				return ms;
		}
		return null;
	}

	bool IsHoldingRelease(CMD_AICommanderObjectiveComponent obj, float worldTime)
	{
		float until;
		return m_mHoldRelease.Find(obj, until) && worldTime < until;
	}

	protected CMD_ThreatAssessment Assess(CMD_ThreatCluster cluster, float worldTime)
	{
		CMD_ThreatAssessment a = new CMD_ThreatAssessment();
		vector center = cluster.m_vCenterPos;
		a.m_iEnemies = Math.Max(cluster.m_iTotalEstimatedEnemies, 1);
		a.m_fUncertainty = cluster.m_fUncertainty;
		a.m_bStale = worldTime - cluster.m_fFreshestUpdateTime >= m_fStalenessThreshold;

		bool armor;
		vector vel;
		int n;
		foreach (CMD_ThreatEntry e : cluster.m_aMembers)
		{
			if (!e)
				continue;
			armor = armor || e.m_bArmorSeen || e.m_iArmor > 0;
			a.m_bMG = a.m_bMG || e.m_iMG > 0;
			a.m_bAT = a.m_bAT || e.m_iAT > 0 || e.m_bATSeen;
			a.m_bFiringAtUs = a.m_bFiringAtUs || e.m_bFiringAtUs;
			a.m_bSniper = a.m_bSniper || e.m_iSniper > 0;
			vel = vel + e.m_vVelocity;
			n++;
		}
		if (n > 0)
			vel = vel * (1.0 / n);
		vel[1] = 0;
		a.m_vVelocity = vel;

		float speed = vel.Length();
		if (armor)
			a.m_eType = CMD_EThreatType.ARMOR;
		else if (speed > 3)
			a.m_eType = CMD_EThreatType.MOTORIZED;
		else
			a.m_eType = CMD_EThreatType.INFANTRY;

		a.m_bEmergency = cluster.m_eClusterLevel == CMD_EThreatLevel.CRITICAL && IsNearOwnObjective(center);

		FindThreatenedAsset(a, cluster, center, speed);

		float r2 = m_fRatioRadius * m_fRatioRadius;
		foreach (DCO_GroupUtilityComponent g : m_Commander.GetOwnedGroups())
		{
			if (!g || !g.GetOwner() || vector.DistanceSqXZ(g.GetOwner().GetOrigin(), center) > r2)
				continue;
			float w = g.GetUnitCount();
			if (g.IsArmor())
				w *= 3;
			a.m_fFriendly += w;
		}
		float enemyMul = 1;
		if (a.m_eType == CMD_EThreatType.ARMOR)
			enemyMul = 4;
		else if (a.m_eType == CMD_EThreatType.MOTORIZED)
			enemyMul = 1.5;
		a.m_fEnemy = 1.0 * a.m_iEnemies * enemyMul;
		a.m_fRatio = a.m_fFriendly / Math.Max(a.m_fEnemy, 1);

		a.m_bCanMatch = a.m_eType != CMD_EThreatType.ARMOR || HasAntiArmorAsset(center);
		return a;
	}

	protected void FindThreatenedAsset(CMD_ThreatAssessment a, CMD_ThreatCluster cluster, vector center, float speed)
	{
		array<vector> pos = {};
		array<int> kinds = {};
		array<CMD_AICommanderObjectiveComponent> objRefs = {};
		array<DCO_LogiJob> jobRefs = {};

		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		FactionKey fk = m_Commander.GetCommanderFactionKey();
		if (mgr)
		{
			foreach (CMD_AICommanderObjectiveComponent obj : mgr.m_aObjective)
			{
				if (!obj)
					continue;

				if (obj.IsCapturedBy(fk))
				{
					pos.Insert(obj.GetOwner().GetOrigin());
					kinds.Insert(CMD_EAssetKind.OBJECTIVE);
					objRefs.Insert(obj);
					jobRefs.Insert(null);
					continue;
				}

				vector st = vector.Zero;
				if (m_Commander.GetStagingPos(obj, st) && !m_Commander.IsAssaultReleased(obj))
				{
					pos.Insert(st);
					kinds.Insert(CMD_EAssetKind.STAGING);
					objRefs.Insert(obj);
					jobRefs.Insert(null);
				}

				float opR = obj.GetRadius() + m_fRatioRadius;
				if (!a.m_OpObjective && HasOperationGroups(obj) && (vector.DistanceXZ(obj.GetOwner().GetOrigin(), center) <= opR || (st != vector.Zero && vector.DistanceXZ(st, center) <= m_fRatioRadius)))
					a.m_OpObjective = obj;
			}
		}

		DCO_Logistics logi = m_Commander.GetLogistics();
		if (logi)
		{
			pos.Insert(logi.GetHub());
			kinds.Insert(CMD_EAssetKind.HUB);
			objRefs.Insert(null);
				jobRefs.Insert(null);

			foreach (DCO_LogiJob job : logi.GetJobs())
			{
				if (!job.m_Team)
					continue;
				pos.Insert(job.m_Team.GetTeamPos());
				kinds.Insert(CMD_EAssetKind.LOGISTICS);
				objRefs.Insert(null);
				jobRefs.Insert(job);
				pos.Insert(job.m_vLZ);
				kinds.Insert(CMD_EAssetKind.LOGISTICS);
				objRefs.Insert(null);
				jobRefs.Insert(job);
			}
		}

		if (m_ArtySupport)
		{
			array<DCO_GroupUtilityComponent> mortars = {};
			m_ArtySupport.GetRegisteredUnits(mortars);
			foreach (DCO_GroupUtilityComponent m : mortars)
			{
				if (!m || !m.GetOwner())
					continue;
				pos.Insert(m.GetOwner().GetOrigin());
				kinds.Insert(CMD_EAssetKind.MORTAR);
				objRefs.Insert(null);
				jobRefs.Insert(null);
			}
		}

		foreach (CMD_ThreatEntry e : cluster.m_aMembers)
		{
			if (e && e.m_sEngagingGroupName && e.m_sEngagingGroupName.GetOwner())
			{
				pos.Insert(e.m_sEngagingGroupName.GetOwner().GetOrigin());
				kinds.Insert(CMD_EAssetKind.CONTACT_GROUP);
				objRefs.Insert(null);
				jobRefs.Insert(null);
			}
		}

		float bestEta = float.MAX;
		vector dir;
		float len = speed * m_fProjectTime;
		if (speed > 0.5)
			dir = a.m_vVelocity * (1.0 / speed);

		foreach (int i, vector p : pos)
		{
			float eta;
			if (speed > 0.5)
			{
				vector rel = p - center;
				float along = rel[0] * dir[0] + rel[2] * dir[2];
				if (along < -50 || along > len)
					continue;
				vector closest = center + dir * Math.Max(along, 0);
				if (vector.DistanceXZ(closest, p) > 200)
					continue;
				eta = Math.Max(along, 0) / speed;
			}
			else
			{
				float d = vector.DistanceXZ(p, center);
				if (d > m_fRatioRadius)
					continue;
				eta = d / 1.4;
			}

			if (eta < bestEta)
			{
				bestEta = eta;
				a.m_eAsset = kinds[i];
				a.m_vAssetPos = p;
				a.m_fAssetEta = eta;
				a.m_Objective = objRefs[i];
				a.m_Job = jobRefs[i];
			}
		}
	}

	protected bool HasOperationGroups(CMD_AICommanderObjectiveComponent obj)
	{
		foreach (DCO_GroupUtilityComponent g : m_Commander.GetOwnedGroups())
		{
			if (!g || g.GetGroupObjective() != obj)
				continue;
			DCO_EGroupTask t = g.GetTask();
			if (t == DCO_EGroupTask.ATTACK || t == DCO_EGroupTask.FLANK || t == DCO_EGroupTask.SUPPORT_BY_FIRE)
				return true;
		}
		return false;
	}

	protected bool HasAntiArmorAsset(vector center)
	{
		if (m_ArtySupport && m_ArtySupport.HasRegisteredUnits())
			return true;

		float nearSq = m_fRatioRadius * m_fRatioRadius;
		foreach (DCO_GroupUtilityComponent g : m_Commander.GetOwnedGroups())
		{
			if (!g || g.IsPlayerGroup() || (!g.HasAT() && !g.IsArmor()))
				continue;
			if (g.IsAvailableReserve() || vector.DistanceSqXZ(g.GetOwner().GetOrigin(), center) <= nearSq)
				return true;
		}
		return false;
	}

	protected void ScoreActions(CMD_ThreatAssessment a, CMD_ThreatCluster cluster)
	{
		float aggr = m_Commander.DCO_EffAggression();
		float resil = m_Commander.GetResilience();
		float risk = m_Commander.GetRiskTaking();
		float focus = m_Commander.GetCombatFocus();

		float sev = Math.Clamp(cluster.m_fCombinedScore / 75.0, 0, 1.5);
		float unc = Math.Clamp((a.m_fUncertainty - 50) / 150.0, 0, 1);
		float uncPenalty = unc * (1 - risk);

		a.m_aScores.Clear();
		for (int i = 0; i <= CMD_EThreatAction.SUPPORT_OP; i++)
			a.m_aScores.Insert(0);

		a.m_aScores[CMD_EThreatAction.MONITOR] = 0.35 * (1 - Math.Min(sev, 1)) + 0.2 * (1 - focus);

		float recon = 0.7 * unc * (1 - risk * 0.6);
		if (a.m_bStale)
			recon += 0.3;
		a.m_aScores[CMD_EThreatAction.RECON] = recon;

		float hold = 0.15 + 0.3 * sev + 0.15 * Math.Clamp(a.m_fRatio - 0.5, 0, 1) - 0.2 * uncPenalty;
		if (a.m_eAsset != CMD_EAssetKind.NONE)
			hold += 0.2;
		if (a.m_eType == CMD_EThreatType.ARMOR && a.m_bCanMatch)
			hold += 0.25;
		if (!a.m_bCanMatch && !a.m_bEmergency)
			hold *= 0.2;
		a.m_aScores[CMD_EThreatAction.HOLD_DESTROY] = hold;

		float counter = 0;
		if (cluster.m_eClusterLevel >= CMD_EThreatLevel.MEDIUM)
			counter = 0.15 + 0.45 * aggr + 0.2 * Math.Clamp(a.m_fRatio - 1, 0, 1) - 0.3 * uncPenalty;
		if (a.m_fRatio < 1)
			counter *= 0.5;
		if (a.m_bMG && counter > 0)
			counter += 0.15;
		if (!a.m_bCanMatch)
			counter *= 0.1;
		a.m_aScores[CMD_EThreatAction.COUNTERATTACK] = counter;

		float defense = 0;
		if (a.m_eAsset == CMD_EAssetKind.OBJECTIVE && a.m_Objective)
		{
			defense = 0.45 + 0.3 * Math.Clamp(1 - a.m_fAssetEta / 300.0, 0, 1);
			if (!a.m_bCanMatch)
				defense += 0.2;
		}
		a.m_aScores[CMD_EThreatAction.PREPARE_DEFENSE] = defense;

		float avoid = 0;
		if (a.m_eAsset == CMD_EAssetKind.LOGISTICS || a.m_eAsset == CMD_EAssetKind.STAGING)
			avoid = 0.6 + 0.2 * Math.Min(sev, 1);
		a.m_aScores[CMD_EThreatAction.AVOID] = avoid;

		float withdraw = 0;
		if (a.m_fRatio < m_fWithdrawRatio && a.m_fFriendly > 0)
			withdraw = 0.5 + 0.3 * (1 - a.m_fRatio / m_fWithdrawRatio);
		if (!a.m_bCanMatch && a.m_fFriendly > 0)
			withdraw += 0.4;
		withdraw *= 1 - 0.6 * resil;
		if (m_Commander && !DCO_CommanderTactics.IsAllowed(m_Commander, DCO_ETactic.DELAY))
			withdraw = 0;
		a.m_aScores[CMD_EThreatAction.DELAY_WITHDRAW] = withdraw;

		float op = 0;
		if (a.m_OpObjective)
			op = 0.55 + 0.2 * Math.Min(sev, 1);
		a.m_aScores[CMD_EThreatAction.SUPPORT_OP] = op;
	}

	protected void DecideAction(CMD_ThreatMission mission, CMD_ThreatAssessment a, float worldTime)
	{
		if (!mission.m_bTimedOut && worldTime > mission.m_fDeadline)
		{
			mission.m_bTimedOut = true;
			RecallMission(mission, "timeout");
			SwitchAction(mission, CMD_EThreatAction.MONITOR, a, worldTime, "timeout");
			return;
		}
		if (mission.m_bTimedOut)
			return;

		CMD_EThreatAction best = a.Best();
		if (best == mission.m_eAction)
			return;

		float adapt = m_Commander.GetAdaptability();
		bool escalate = a.m_iEnemies >= 1.5 * mission.m_iStartEnemies + 2;
		bool first = mission.m_eAction == CMD_EThreatAction.MONITOR && mission.m_aGroups.IsEmpty() && !mission.m_bExecuted;
		if (!first && !escalate)
		{
			if (a.Score(best) < a.Score(mission.m_eAction) + Math.Lerp(0.25, 0.05, adapt))
				return;
			if (worldTime - mission.m_fLastChange < Math.Lerp(120, 30, adapt))
				return;
		}

		if (escalate)
			mission.m_iStartEnemies = a.m_iEnemies;

		string why = "score";
		if (escalate)
			why = "escalate";
		RecallMission(mission, "switch");
		SwitchAction(mission, best, a, worldTime, why);
	}

	protected void SwitchAction(CMD_ThreatMission mission, CMD_EThreatAction action, CMD_ThreatAssessment a, float worldTime, string why)
	{
		CMD_EThreatAction old = mission.m_eAction;
		mission.m_eAction = action;
		mission.m_fLastChange = worldTime;
		mission.m_bExecuted = false;

		string line = string.Format("threat_action mission=%1 %2->%3 why=%4 type=%5 enemies=%6 ratio=%7 asset=%8 eta=%9",
			mission.m_iId, CMD_ThreatMission.ActionName(old), CMD_ThreatMission.ActionName(action), why,
			typename.EnumToString(CMD_EThreatType, a.m_eType), a.m_iEnemies, a.m_fRatio.ToString(-1, 2),
			typename.EnumToString(CMD_EAssetKind, a.m_eAsset), Math.Round(a.m_fAssetEta));
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", line);
		if (action != CMD_EThreatAction.MONITOR)
			NotifyPlayers(mission, a);
	}

	protected void ExecuteAction(CMD_ThreatMission mission, CMD_ThreatCluster cluster, CMD_ThreatAssessment a, float worldTime)
	{
		switch (mission.m_eAction)
		{
			case CMD_EThreatAction.MONITOR:
			{
				if (a.m_bForceArty)
					TrySendClusterArtillery(cluster, worldTime, true);
				break;
			}
			case CMD_EThreatAction.RECON:
			{
				CMD_ThreatEntry primary = cluster.GetPrimaryMember();
				if (primary && !primary.m_bReconSent)
					TrySendReconForThreat(primary, worldTime);
				TrySendClusterArtillery(cluster, worldTime, a.m_bForceArty);
				break;
			}
			case CMD_EThreatAction.HOLD_DESTROY:
			{
				TrySendClusterReinforcement(cluster, worldTime, a.m_bEmergency, a, mission);
				TrySendClusterArtillery(cluster, worldTime, a.m_bForceArty);
				break;
			}
			case CMD_EThreatAction.COUNTERATTACK:
			{
				TrySendCounterFlank(cluster, worldTime, a.m_bEmergency, a, mission);
				TrySendClusterArtillery(cluster, worldTime, a.m_bForceArty);
				break;
			}
			case CMD_EThreatAction.PREPARE_DEFENSE:
			{
				if (!mission.m_bExecuted)
					PrepareDefense(mission, cluster, a, worldTime);
				TrySendClusterArtillery(cluster, worldTime, a.m_bForceArty);
				break;
			}
			case CMD_EThreatAction.AVOID:
			{
				if (!mission.m_bExecuted)
					AvoidThreat(mission, cluster, a, worldTime);
				break;
			}
			case CMD_EThreatAction.DELAY_WITHDRAW:
			{
				if (!mission.m_bExecuted)
					Withdraw(mission, cluster, a, worldTime);
				TrySendClusterArtillery(cluster, worldTime, a.m_bForceArty);
				break;
			}
			case CMD_EThreatAction.SUPPORT_OP:
			{
				if (!mission.m_bExecuted)
					SupportOperation(mission, cluster, a, worldTime);
				break;
			}
		}
	}

	protected DCO_GroupUtilityComponent FindMatchedGroup(DCO_EGroupTask task, vector pos, CMD_ThreatAssessment a, out bool armored)
	{
		armored = false;
		DCO_GroupUtilityComponent g;
		bool needAT = a && (a.m_eType == CMD_EThreatType.ARMOR || a.m_bNeedAT);
		if (!a || needAT || a.m_eType == CMD_EThreatType.INFANTRY)
		{
			g = FindClosestGroupForTask(task, pos, -1, true);
			if (g)
			{
				armored = true;
				return g;
			}
		}

		int caps = -1;
		if (needAT)
			caps = DCO_EAIGroupCapabilities.ANTI_ARMOR | AICommander_BaseComponent.DefaultCapabilitiesForTask(task);
		g = FindClosestGroupForTask(task, pos, caps);

		if (g && needAT && !a.m_bEmergency && !g.HasAT() && !g.IsArmor())
			return null;
		return g;
	}

	protected void PrepareDefense(CMD_ThreatMission mission, CMD_ThreatCluster cluster, CMD_ThreatAssessment a, float worldTime)
	{
		mission.m_bExecuted = true;
		CMD_AICommanderObjectiveComponent obj = a.m_Objective;
		if (!obj)
			return;

		vector objPos = obj.GetOwner().GetOrigin();
		vector dir = cluster.m_vCenterPos - objPos;
		dir[1] = 0;
		float dist = dir.Length();
		if (dist < 1)
			return;
		dir = dir * (1.0 / dist);

		int faced;
		foreach (DCO_GroupUtilityComponent g : m_Commander.GetOwnedGroups())
		{
			if (!g || g.GetGroupObjective() != obj || g.GetTask() != DCO_EGroupTask.DEFEND)
				continue;
			DCO_AIGarrisonActivity act = g.GetGarrisonActivity();
			if (act && act.IsActive() && act.DCO_Reface(dir, m_bUpgradeGarrisonMode))
				faced++;
		}

		vector blockPos = objPos + dir * Math.Min(obj.GetRadius() + m_fBlockingDist, dist * 0.6);
		blockPos = DCO_Util.Surface(blockPos);

		bool armored;
		DCO_GroupUtilityComponent blocker = FindMatchedGroup(DCO_EGroupTask.REINFORCE, blockPos, a, armored);
		string blockName = "none";
		if (CanUseGroup(blocker, a.m_bEmergency))
		{
			blocker.CompleteAllWaypoints();
			if (m_Commander.SpawnMoveRoute(blocker, blocker.GetOwner().GetOrigin(), blockPos, worldTime))
			{
				if (!armored)
					blocker.SetTask(DCO_EGroupTask.REINFORCE, "CommanderThreatResponse:1024");
				mission.m_aGroups.Insert(blocker);
				blocker.m_iDCOThreatAct = CMD_EThreatAction.PREPARE_DEFENSE;
				blockName = blocker.GetOwner().GetName();
			}
		}

		if (m_Commander.GetDefense())
			m_Commander.GetDefense().RequestApproachAmbush(m_Commander, obj, cluster.m_vCenterPos, a.m_eType != CMD_EThreatType.INFANTRY, worldTime);

		if (blockName == "none")
			DCO_Shortfall(obj, cluster.m_vCenterPos, cluster.m_iTotalEstimatedEnemies, "prepare");
		string line = string.Format("threat_defense mission=%1 obj=%2 garrison_refaced=%3 blocker=%4", mission.m_iId, obj.GetLogName(), faced, blockName);
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", line);
	}

	protected void AvoidThreat(CMD_ThreatMission mission, CMD_ThreatCluster cluster, CMD_ThreatAssessment a, float worldTime)
	{
		mission.m_bExecuted = true;
		string what = "none";
		if (a.m_eAsset == CMD_EAssetKind.LOGISTICS && a.m_Job && a.m_Job.m_Team)
		{
			a.m_Job.m_Team.OnThreatAhead(worldTime);
			what = "logistics_job_" + a.m_Job.m_iId;
		}
		else if (a.m_eAsset == CMD_EAssetKind.STAGING && a.m_Objective)
		{
			if (m_Commander.ShiftStaging(a.m_Objective, cluster.m_vCenterPos, m_fRatioRadius * 0.5, worldTime))
				what = "staging_" + a.m_Objective.GetLogName();
		}

		string line = string.Format("threat_avoid mission=%1 target=%2", mission.m_iId, what);
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", line);
	}

	protected void Withdraw(CMD_ThreatMission mission, CMD_ThreatCluster cluster, CMD_ThreatAssessment a, float worldTime)
	{
		mission.m_bExecuted = true;
		vector c = cluster.m_vCenterPos;
		float r2 = m_fRatioRadius * m_fRatioRadius;
		int n;
		foreach (DCO_GroupUtilityComponent g : m_Commander.GetOwnedGroups())
		{
			if (!g || !g.GetOwner() || g.IsPlayerGroup() || g.IsDedicatedTransport() || g.IsMortar() || g.IsInTransport())
				continue;
			vector gp = g.GetOwner().GetOrigin();
			if (vector.DistanceSqXZ(gp, c) > r2)
				continue;
			if (g.GetTask() == DCO_EGroupTask.DEFEND && a.m_bEmergency)
				continue;
			if (TacticOwns(g, "withdraw", mission.m_iId))
				continue;
			if (DCO_FloorBlocks(g, cluster.m_iTotalEstimatedEnemies, "withdraw"))
				continue;

			vector fall = FindFallback(gp, c);
			m_Commander.DetachFromObjective(g);
			if (!DCO_BreakContact.Start(g, c, fall, "threat_withdraw"))
			{
				ThrowSmoke(g, c);
				g.SetRetreating(true);
				g.CompleteAllWaypoints();
				m_Commander.SpawnMoveRoute(g, gp, fall, worldTime);
			}
			mission.m_aGroups.Insert(g);
			g.m_iDCOThreatAct = CMD_EThreatAction.DELAY_WITHDRAW;
			n++;
		}

		string line = string.Format("threat_withdraw mission=%1 groups=%2 ratio=%3 can_match=%4", mission.m_iId, n, a.m_fRatio.ToString(-1, 2), a.m_bCanMatch);
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", line);
	}

	protected vector FindFallback(vector from, vector threat)
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		float myDist = vector.DistanceXZ(from, threat);
		float best = float.MAX;
		vector pick;
		if (mgr)
		{
			foreach (CMD_AICommanderObjectiveComponent obj : mgr.m_aObjective)
			{
				if (!obj || !obj.IsCapturedBy(m_Commander.GetCommanderFactionKey()) || m_Commander.IsObjectiveContested(obj))
					continue;
				vector op = obj.GetOwner().GetOrigin();
				if (vector.DistanceXZ(op, threat) < myDist + 100)
					continue;
				float d = vector.DistanceXZ(op, from);
				if (d < best)
				{
					best = d;
					pick = op;
				}
			}
		}
		if (best < float.MAX)
			return pick;

		vector away = from - threat;
		away[1] = 0;
		if (away.LengthSq() < 1)
			away = m_Commander.GetOwner().GetOrigin() - threat;
		away[1] = 0;
		away.Normalize();
		vector p = from + away * 300;
		p = DCO_Util.Surface(p);
		return p;
	}

	static void ThrowSmoke(DCO_GroupUtilityComponent g, vector threat)
	{
		SCR_AIGroup grp = SCR_AIGroup.Cast(g.GetOwner());
		if (!grp)
			return;

		array<AIAgent> agents = {};
		grp.GetAgents(agents);
		foreach (AIAgent agent : agents)
		{
			SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(agent);
			if (!ca || !ca.m_InfoComponent || !ca.m_InfoComponent.HasRole(EUnitRole.HAS_SMOKE_GRENADE) || !ca.GetCommunicationComponent() || !ca.GetControlledEntity())
				continue;

			vector from = ca.GetControlledEntity().GetOrigin();
			vector dir = threat - from;
			dir[1] = 0;
			dir.Normalize();
			SCR_AIMessage_ThrowGrenadeTo msg = SCR_AIMessage_ThrowGrenadeTo.Create(from + dir * 15, EWeaponType.WT_SMOKEGRENADE, 0);
			msg.m_fPriorityLevel = SCR_AIActionBase.PRIORITY_BEHAVIOR_THROW_GRENADE;
			msg.SetReceiver(ca);
			ca.GetCommunicationComponent().RequestBroadcast(msg, ca);
			return;
		}
	}

	protected void SupportOperation(CMD_ThreatMission mission, CMD_ThreatCluster cluster, CMD_ThreatAssessment a, float worldTime)
	{
		mission.m_bExecuted = true;
		CMD_AICommanderObjectiveComponent obj = a.m_OpObjective;
		if (!obj)
			return;

		vector c = cluster.m_vCenterPos;
		DCO_GroupUtilityComponent pick;
		float best = float.MAX;
		foreach (DCO_GroupUtilityComponent g : m_Commander.GetOwnedGroups())
		{
			if (!g || g.GetGroupObjective() != obj || g.IsInTransport())
				continue;
			if (g.GetTask() != DCO_EGroupTask.FLANK && g.GetTask() != DCO_EGroupTask.SUPPORT_BY_FIRE)
				continue;
			if (TacticOwns(g, "support_op", mission.m_iId))
				continue;
			if (a.m_eType == CMD_EThreatType.ARMOR && !a.m_bEmergency && !g.HasAT() && !g.IsArmor())
				continue;
			float d = vector.DistanceSqXZ(g.GetOwner().GetOrigin(), c);
			if (d < best)
			{
				best = d;
				pick = g;
			}
		}

		string how;
		if (pick)
		{
			vector flankPos = m_Commander.ComputeFlankPosition(pick.GetOwner().GetOrigin(), c, m_fFlankDistance);
			pick.CompleteAllWaypoints();
			m_Commander.SpawnMoveRoute(pick, pick.GetOwner().GetOrigin(), flankPos, worldTime);
			SCR_AIWaypoint sweep = m_Commander.SpawnMoveWP(c);
			if (sweep)
				pick.MoveTo(sweep, worldTime);
			how = "op_group " + pick.GetOwner().GetName();
		}
		else if (!m_Commander.IsAssaultReleased(obj))
		{
			m_mHoldRelease.Set(obj, worldTime + m_fHoldReleaseMax);
			mission.m_HoldObj = obj;
			how = "hold_release";
		}
		else
		{
			how = "none";
		}

		string line = string.Format("threat_support_op mission=%1 obj=%2 how=%3", mission.m_iId, obj.GetLogName(), how);
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", line);
	}

	protected void ReviewMissions(float worldTime)
	{
		for (int i = m_aMissions.Count() - 1; i >= 0; i--)
		{
			CMD_ThreatMission ms = m_aMissions[i];
			if (ms.m_bSeen)
				continue;

			if (m_Book)
				m_Book.OnGone(ms.m_iDossier, worldTime);
			RecallMission(ms, "threat_gone");
			string line = string.Format("threat_mission_done mission=%1 action=%2 duration=%3s", ms.m_iId, CMD_ThreatMission.ActionName(ms.m_eAction), Math.Round(worldTime - ms.m_fStart));
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", line);
			m_aMissions.Remove(i);
		}
	}

	protected void RecallMission(CMD_ThreatMission ms, string why)
	{
		foreach (DCO_GroupUtilityComponent g : ms.m_aGroups)
		{
			if (!g)
				continue;
			if (g.HasState(DCO_EGroupState.RETREATING))
			{
				g.SetRetreating(false);
				continue;
			}
			DCO_EGroupTask t = g.GetTask();
			if (t == DCO_EGroupTask.REINFORCE || t == DCO_EGroupTask.FLANK)
			{
				g.CompleteAllWaypoints();
				g.SetTask(DCO_EGroupTask.NONE, "CommanderThreatResponse:1240");
			}
			else if (g.IsArmor() && g.m_iDCOThreatAct >= 0)
			{
				g.CompleteAllWaypoints();
				g.m_iDCOThreatAct = -1;
				DCO_Perf.Count("threat:recall_armor");
			}
		}
		ms.m_aGroups.Clear();

		if (ms.m_HoldObj)
		{
			m_mHoldRelease.Remove(ms.m_HoldObj);
			ms.m_HoldObj = null;
		}
	}

	protected void NotifyPlayers(CMD_ThreatMission mission, CMD_ThreatAssessment a)
	{
		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetInstance();
		float radius = 800;
		if (cfg)
			radius = cfg.GetRadioRadius();

		array<int> ids = {};
		DCO_PlayerAwareness.CollectPlayerGroupsNear(m_Commander.GetCommanderFactionKey(), mission.m_vPos, radius, ids);
		if (ids.IsEmpty())
			return;

		string type = "@thr_infantry";
		if (a.m_eType == CMD_EThreatType.ARMOR)
			type = "@thr_armor";
		else if (a.m_eType == CMD_EThreatType.MOTORIZED)
			type = "@thr_motorized";

		string key = "threat_alert";
		array<string> params = DCO_Radio.P("type", type, "count", a.m_iEnemies.ToString(), "grid", DCO_PlayerComms.Grid(mission.m_vPos), "action", CMD_ThreatMission.ActionKey(mission.m_eAction));
		if (a.m_vVelocity.LengthSq() > 0.25)
		{
			key = "threat_alert_moving";
			params.Insert("dir");
			params.Insert(DCO_Radio.Dir(mission.m_vPos, mission.m_vPos + a.m_vVelocity * 100));
		}
		string comp;
		if (a.m_bMG)
			comp = "@comp_mg";
		if (a.m_bAT)
			comp = DCO_Radio.Join(comp, "@comp_at");
		if (a.m_bSniper)
			comp = DCO_Radio.Join(comp, "@comp_sniper");
		if (!comp.IsEmpty())
		{
			key += "_comp";
			params.Insert("composition");
			params.Insert(comp);
		}
		bool critical = a.m_eType == CMD_EThreatType.ARMOR;
		foreach (int id : ids)
			DCO_Radio.Group(id, "THREAT", key, DCO_ERadioKind.WARNING, params, critical);
	}

	protected void NotifyReporters(CMD_ThreatCluster cluster, CMD_ThreatMission mission)
	{
		foreach (CMD_ThreatEntry e : cluster.m_aMembers)
		{
			if (!e || e.m_iReporterPid <= 0 || e.m_bReporterNotified)
				continue;
			e.m_bReporterNotified = true;

			string decision = "@act_noted";
			if (mission)
				decision = CMD_ThreatMission.ActionKey(mission.m_eAction);
			DCO_Radio.Player(e.m_iReporterPid, "COMMANDER", "report_decision", DCO_ERadioKind.REPORT, DCO_Radio.P("grid", DCO_PlayerComms.Grid(e.m_vPosition), "action", decision));
		}
	}

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

		string hvt = DCO_HVT.Kind(m_Commander, threat);
		if (!hvt.IsEmpty())
		{
			score = score * 1.5;
			if (threat.m_sHvt != hvt)
			{
				threat.m_sHvt = hvt;
				DCO_HVT.Log(m_Commander, threat, hvt, "threat_response");
			}
		}

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
		float bestScore    = 0.0;
		float freshest     = 0.0;

		foreach (CMD_ThreatEntry e : cluster.m_aMembers)
		{
			if (!e)
				continue;

			sumPos        += e.m_vBelievedPos;
			bestScore      = Math.Max(bestScore, e.m_fPriorityScore);

			if (e.m_fLastUpdateTime > freshest)
				freshest = e.m_fLastUpdateTime;
		}

		float bestUnc = float.MAX;
		foreach (CMD_ThreatEntry e : cluster.m_aMembers)
		{
			if (e && freshest - e.m_fLastUpdateTime <= 30.0)
				bestUnc = Math.Min(bestUnc, e.m_fBelievedUncertainty);
		}
		if (bestUnc < float.MAX)
			cluster.m_fUncertainty = bestUnc;

		int count = cluster.m_aMembers.Count();
		if (count > 0)
			cluster.m_vCenterPos = sumPos / count;

		float combinedScore = bestScore + Math.Min(Math.Max(count - 1, 0) * 5.0, 15.0);

		int raw;
		totalEnemies = Math.Round(DCO_CommanderOps.DedupEntries(cluster.m_aMembers, DCO_Util.NowS(), false, 0.25, raw));
		if (raw > 0)
			totalEnemies = Math.Max(totalEnemies, 1);

		cluster.m_iTotalEstimatedEnemies = totalEnemies;
		cluster.m_fCombinedScore         = combinedScore;
		cluster.m_fFreshestUpdateTime    = freshest;
		cluster.m_eClusterLevel          = ClassifyScore(combinedScore);
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
				a.m_bArmorSeen               = a.m_bArmorSeen || b.m_bArmorSeen;
				a.m_bATSeen                  = a.m_bATSeen || b.m_bATSeen;
				a.m_iMG = Math.Max(a.m_iMG, b.m_iMG);
				a.m_iAT = Math.Max(a.m_iAT, b.m_iAT);
				a.m_iSniper = Math.Max(a.m_iSniper, b.m_iSniper);
				a.m_iVehicle = Math.Max(a.m_iVehicle, b.m_iVehicle);
				a.m_iArmor = Math.Max(a.m_iArmor, b.m_iArmor);
				a.m_bFiringAtUs = a.m_bFiringAtUs || b.m_bFiringAtUs;
				a.m_bLowPriority = a.m_bLowPriority && b.m_bLowPriority;

				if (b.m_fLastUpdateTime > a.m_fLastUpdateTime)
				{
					a.m_vPosition       = b.m_vPosition;
					a.m_fLastUpdateTime = b.m_fLastUpdateTime;
				}

				if (b.m_iReporterPid > 0 && a.m_iReporterPid <= 0)
				{
					a.m_iReporterPid = b.m_iReporterPid;
					a.m_bReporterNotified = b.m_bReporterNotified;
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

	protected float m_fDCOReconCapLogAt = -1000;

	protected void TrySendReconForThreat(CMD_ThreatEntry threat, float worldTime)
	{
		if (!m_Commander)
			return;

		vector targetPos = threat.m_vBelievedPos;
		int onRecon = m_Commander.DCO_ThreatReconCount();
		if (onRecon >= 2)
		{
			DCO_Perf.Count("recon_skip:cap");
			if (worldTime - m_fDCOReconCapLogAt >= 60.0)
			{
				m_fDCOReconCapLogAt = worldTime;
				DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("recon_skip cmd=%1 grp=- reason=cap on_recon=%2 max=2 action=queued", m_Commander.GetCommanderUID(), onRecon));
			}
			return;
		}
		DCO_GroupUtilityComponent reconGrp = m_Commander.FindBestIdleGroupForTask_Public(DCO_EGroupTask.RECON, targetPos);
		if (!reconGrp)
		{
			DCO_Perf.Count("recon_skip:no_candidate");
			return;
		}
		if (!CanUseGroup(reconGrp, false))
			return;

		vector obsPos = CMD_ReconSpotFinder.FindBestReconSpot(reconGrp.GetOwner().GetOrigin(), targetPos, 150.0, 350.0, 12);
		if (obsPos == vector.Zero)
			return;

		reconGrp.CompleteAllWaypoints();
		if (!m_Commander.SpawnMoveRoute(reconGrp, reconGrp.GetOwner().GetOrigin(), obsPos, worldTime))
			return;

		reconGrp.SetTask(DCO_EGroupTask.RECON, "CommanderThreatResponse:1618");
		threat.m_bReconSent = true;
	}

	protected DCO_GroupUtilityComponent FindClosestGroupForTask(DCO_EGroupTask task, vector pos, int wantedCaps = -1, bool armor = false)
	{
	    if (!m_Commander)
	        return null;

	    return m_Commander.FindBestIdleGroupForTask_Public(task, pos, wantedCaps, armor);
	}

	protected void TrySendCounterFlank(CMD_ThreatCluster cluster, float worldTime, bool emergency, CMD_ThreatAssessment a = null, CMD_ThreatMission mission = null)
	{
		if (!m_Commander)
			return;

		CMD_ThreatEntry primary = cluster.GetPrimaryMember();
		if (!primary)
			return;

		int mid = -1;
		if (mission)
			mid = mission.m_iId;
		CMD_ResponseLedgerEntry ledger = GetLedger(cluster.m_vCenterPos, worldTime);
		if (worldTime - ledger.m_fLastFlankTime < m_fFlankTaskExpiry)
		{
			FlankSkip(mid, "cooldown", null, DCO_ELogLevel.DEBUG);
			return;
		}

		bool armored;
		DCO_GroupUtilityComponent flankGrp = FindMatchedGroup(DCO_EGroupTask.FLANK, cluster.m_vCenterPos, a, armored);
		if (!flankGrp)
			flankGrp = FindMatchedGroup(DCO_EGroupTask.REINFORCE, cluster.m_vCenterPos, a, armored);
		if (!flankGrp)
		{
			FlankSkip(mid, "no_group", null, DCO_ELogLevel.INFO);
			if (a)
				DCO_Shortfall(a.m_Objective, cluster.m_vCenterPos, cluster.m_iTotalEstimatedEnemies, "counter");
			return;
		}
		if (!CanUseGroup(flankGrp, emergency))
		{
			FlankSkip(mid, "cannot_commit", flankGrp, DCO_ELogLevel.INFO);
			if (a)
				DCO_Shortfall(a.m_Objective, cluster.m_vCenterPos, cluster.m_iTotalEstimatedEnemies, "counter");
			return;
		}
		if (m_Book && !m_Book.CapOk(mission, a, flankGrp, emergency))
		{
			FlankSkip(mid, "cap", flankGrp, DCO_ELogLevel.INFO);
			return;
		}

		vector from = flankGrp.GetOwner().GetOrigin();
		vector center = cluster.m_vCenterPos;
		vector flankPos;
		string how = "friend_axis";
		if (!FlankFromFriend(primary, from, center, flankPos))
		{
			how = "own_axis";
			flankPos = m_Commander.ComputeFlankPosition(from, center, m_fFlankDistance);
		}
		if (m_Commander.EvaluateFlankCandidate(flankPos, center) <= -1.0)
		{
			FlankSkip(mid, "no_flank_pos", flankGrp, DCO_ELogLevel.INFO);
			return;
		}

		flankGrp.CompleteAllWaypoints();
		vector detour;
		bool hasDetour = FlankDetour(from, flankPos, center, detour);
		bool routed;
		if (hasDetour)
			routed = m_Commander.SpawnMoveRoute(flankGrp, from, detour, worldTime) && m_Commander.SpawnMoveRoute(flankGrp, detour, flankPos, worldTime);
		else
			routed = m_Commander.SpawnMoveRoute(flankGrp, from, flankPos, worldTime);
		if (!routed)
		{
			flankGrp.CompleteAllWaypoints();
			FlankSkip(mid, "route_failed", flankGrp, DCO_ELogLevel.WARN);
			return;
		}

		SCR_AIWaypoint sweepWp = m_Commander.SpawnMoveWP(center);
		if (sweepWp)
			flankGrp.MoveTo(sweepWp, worldTime);

		if (!armored)
			flankGrp.SetTask(DCO_EGroupTask.FLANK, "CommanderThreatResponse:1661");
		flankGrp.m_iDCOThreatAct = CMD_EThreatAction.COUNTERATTACK;
		primary.m_bFlankSent = true;
		if (mission)
			mission.m_aGroups.Insert(flankGrp);
		ledger.m_fLastFlankTime = worldTime;

		CMD_FlankTracker tracker = new CMD_FlankTracker();
		tracker.m_Group = flankGrp;
		tracker.m_fExpiry = worldTime + m_fFlankTaskExpiry;
		tracker.m_bArmored = armored;
		tracker.m_SweepWp = sweepWp;
		tracker.m_vTarget = center;
		tracker.m_iMission = mid;
		m_aFlankers.Insert(tracker);

		vector toFlank = flankPos - center;
		vector toFrom = from - center;
		float sideDeg = Math.AbsFloat(Math.Atan2(toFlank[2], toFlank[0]) - Math.Atan2(toFrom[2], toFrom[0])) * Math.RAD2DEG;
		if (sideDeg > 180.0)
			sideDeg = 360.0 - sideDeg;
		DCO_Perf.Count("threat:flank_sent");
		DCO_Perf.Count("threat:flank_" + how);
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(m_Commander), string.Format("threat_flank mission=%1 grp=%2 armored=%3 how=%4 from_d=%5 flank_d=%6 side_deg=%7 detour=%8 emergency=%9",
			mid, flankGrp.GetOwner().GetName(), armored, how, Math.Round(vector.DistanceXZ(from, center)), Math.Round(vector.DistanceXZ(flankPos, center)), Math.Round(sideDeg), hasDetour, emergency));
	}

	protected void FlankSkip(int mid, string why, DCO_GroupUtilityComponent g, DCO_ELogLevel lvl)
	{
		DCO_Perf.Count("threat:flank_skip_" + why);
		string last;
		if (m_mFlankSkipWhy.Find(mid, last) && last == why)
			return;
		if (m_mFlankSkipWhy.Count() > 200)
			m_mFlankSkipWhy.Clear();
		m_mFlankSkipWhy.Set(mid, why);
		if (!DCO_Log.On(DCO_ELogChannel.CMD, lvl))
			return;
		string name = "-";
		if (g && g.GetOwner())
			name = g.GetOwner().GetName();
		DCO_Log.Write(DCO_ELogChannel.CMD, lvl, DCO_Log.IdCmd(m_Commander), string.Format("threat_flank_skip mission=%1 why=%2 grp=%3", mid, why, name));
	}

	protected bool FlankFromFriend(CMD_ThreatEntry primary, vector from, vector center, out vector pos)
	{
		if (DCO_Ctx.TuneForCmd(m_Commander, "thr.flank_rel", 1.0) <= 0)
			return false;
		DCO_GroupUtilityComponent fr = primary.m_sEngagingGroupName;
		if (!fr || !fr.GetOwner() || fr.GetUnitCount() <= 0)
		{
			DCO_Perf.Count("threat:flank_rel_no_friend");
			return false;
		}
		vector axis = center - fr.GetOwner().GetOrigin();
		axis[1] = 0;
		if (axis.LengthSq() < 1.0)
			return false;
		axis.Normalize();
		vector perp = Vector(-axis[2], 0, axis[0]);
		vector a = DCO_Util.Surface(center + perp * m_fFlankDistance);
		vector b = DCO_Util.Surface(center - perp * m_fFlankDistance);
		float sa = m_Commander.EvaluateFlankCandidate(a, center);
		float sb = m_Commander.EvaluateFlankCandidate(b, center);
		if (sa <= -1.0 && sb <= -1.0)
		{
			DCO_Perf.Count("threat:flank_rel_no_side");
			return false;
		}
		pos = b;
		if (sb <= -1.0 || (sa > -1.0 && vector.DistanceXZ(from, a) - 40.0 * sa <= vector.DistanceXZ(from, b) - 40.0 * sb))
			pos = a;
		return true;
	}

	protected bool FlankDetour(vector from, vector to, vector center, out vector detour)
	{
		if (DCO_Ctx.TuneForCmd(m_Commander, "thr.flank_route", 0.0) <= 0)
			return false;
		vector seg = to - from;
		seg[1] = 0;
		float len2 = seg.LengthSq();
		if (len2 < 1.0)
			return false;
		vector rel = center - from;
		rel[1] = 0;
		float k = Math.Clamp(vector.Dot(rel, seg) / len2, 0.0, 1.0);
		vector closest = from + seg * k;
		vector push = closest - center;
		push[1] = 0;
		float d = push.Length();
		float standoff = m_fFlankDistance * 0.8;
		if (d >= standoff)
			return false;
		if (d < 1.0)
		{
			push = Vector(-seg[2], 0, seg[0]);
			d = push.Length();
		}
		push = push * (1.0 / d);
		detour = DCO_Util.Surface(center + push * m_fFlankDistance);
		DCO_Perf.Count("threat:flank_detour");
		return true;
	}

	protected void RetargetFlank(CMD_FlankTracker t, float worldTime)
	{
		if (!t.m_SweepWp || DCO_Ctx.TuneForCmd(m_Commander, "thr.flank_retarget", 1.0) <= 0)
			return;
		float minMove = DCO_Ctx.TuneForCmd(m_Commander, "thr.flank_retarget_m", 30.0);
		CMD_ThreatEntry best;
		float bestD = m_fClusterRadius;
		foreach (CMD_ThreatEntry e : m_aThreats)
		{
			if (!e)
				continue;
			float d = vector.DistanceXZ(e.m_vBelievedPos, t.m_vTarget);
			if (d < bestD)
			{
				bestD = d;
				best = e;
			}
		}
		if (!best || bestD < minMove)
			return;
		AIGroup ag = AIGroup.Cast(t.m_Group.GetOwner());
		SCR_AIWaypoint wp = m_Commander.SpawnMoveWP(best.m_vBelievedPos);
		if (!ag || !wp)
			return;
		ag.RemoveWaypoint(t.m_SweepWp);
		SCR_EntityHelper.DeleteEntityAndChildren(t.m_SweepWp);
		t.m_Group.MoveTo(wp, worldTime);
		vector old = t.m_vTarget;
		t.m_SweepWp = wp;
		t.m_vTarget = best.m_vBelievedPos;
		t.m_iRetargets++;
		DCO_Perf.Count("threat:flank_retarget");
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(m_Commander), string.Format("threat_flank_retarget mission=%1 grp=%2 moved_m=%3 old=%4 new=%5 n=%6",
			t.m_iMission, t.m_Group.GetOwner().GetName(), Math.Round(bestD), DCO_Util.XZ(old), DCO_Util.XZ(t.m_vTarget), t.m_iRetargets));
	}

	protected void TrySendClusterReinforcement(CMD_ThreatCluster cluster, float worldTime, bool emergency, CMD_ThreatAssessment a = null, CMD_ThreatMission mission = null)
	{
		if (!m_Commander)
			return;

		CMD_ThreatEntry primary = cluster.GetPrimaryMember();
		if (!primary)
			return;

		CMD_ResponseLedgerEntry ledger = GetLedger(cluster.m_vCenterPos, worldTime);
		if (worldTime - ledger.m_fLastReinforcementTime < m_fReinforcementCooldown)
			return;

		if (ledger.m_iReinforcements >= DCO_QrfMax())
			return;

		if (!primary.m_sEngagingGroupName || primary.m_sEngagingGroupName.GetUnitCount() <= 0)
		{
			foreach (CMD_ThreatEntry m : cluster.m_aMembers)
			{
				if (m && m.m_sEngagingGroupName && m.m_sEngagingGroupName.GetUnitCount() > 0)
				{
					primary.m_sEngagingGroupName = m.m_sEngagingGroupName;
					break;
				}
			}
		}

		DispatchReinforcement(cluster.m_vCenterPos, cluster.m_eClusterLevel, primary, worldTime, emergency, a, mission);
	}

	protected void DispatchReinforcement(vector threatPos, CMD_EThreatLevel level, CMD_ThreatEntry stateHolder, float worldTime, bool emergency = false, CMD_ThreatAssessment a = null, CMD_ThreatMission mission = null)
	{
	    if (!m_Commander)
	        return;

	    CMD_ResponseLedgerEntry ledger = GetLedger(threatPos, worldTime);
	    int slotsLeft = DCO_QrfMax() - ledger.m_iReinforcements;
	    if (slotsLeft <= 0)
	        return;

	    vector targetPos = ComputeReinforcementPoint(stateHolder, threatPos);

	    int sentThisCall = 0;

	    int wantedCaps = -1;
	    bool armorThreat = stateHolder.m_bArmorSeen || (a && (a.m_eType == CMD_EThreatType.ARMOR || a.m_bNeedAT));
	    if (armorThreat)
	        wantedCaps = DCO_EAIGroupCapabilities.ANTI_ARMOR | AICommander_BaseComponent.DefaultCapabilitiesForTask(DCO_EGroupTask.REINFORCE);

	    while (sentThisCall < slotsLeft)
	    {
	        DCO_GroupUtilityComponent reinforcement = null;
	        bool armored = false;

	        if (level >= CMD_EThreatLevel.HIGH || armorThreat)
	        {
	            reinforcement = FindClosestGroupForTask(DCO_EGroupTask.REINFORCE, targetPos, -1, true);
	            if (reinforcement)
	            {
	                armored = true;
	            }
	            else
	            {
	                reinforcement = FindClosestGroupForTask(DCO_EGroupTask.REINFORCE, targetPos, wantedCaps);
	                armored = false;
	            }
	        }
	        else
	        {
	            reinforcement = FindClosestGroupForTask(DCO_EGroupTask.REINFORCE, targetPos, wantedCaps);
	        }

	        if (!reinforcement && emergency && sentThisCall == 0)
	            DCO_GroupJobs.EmergencyStarved(m_Commander, 17, "threat_emergency");

	        if (!CanUseGroup(reinforcement, emergency))
	        {
	            if (sentThisCall == 0 && a)
	                DCO_Shortfall(a.m_Objective, threatPos, stateHolder.m_iEstimatedEnemyCount, "hold");
	            break;
	        }

	        if (armorThreat && !armored && !emergency && !reinforcement.HasAT())
	            break;

	        if (m_Book && !m_Book.CapOk(mission, a, reinforcement, emergency))
	            break;

	        if (mission)
	            mission.m_aGroups.Insert(reinforcement);

	        if (!armored && m_Commander.TryAssignTransport(reinforcement, targetPos, worldTime, DCO_EGroupTask.REINFORCE))
	        {
	            RecordReinforcement(ledger, stateHolder, worldTime);
	            sentThisCall++;

	            continue;
	        }

			reinforcement.CompleteAllWaypoints();

	        if (!m_Commander.SpawnMoveRoute(reinforcement, reinforcement.GetOwner().GetOrigin(), targetPos, worldTime))
	            break;

	        if (!armored)
	            reinforcement.SetTask(DCO_EGroupTask.REINFORCE, "CommanderThreatResponse:1770");
	        else
	            reinforcement.SetVehicleCruiseSpeed(0);
	        reinforcement.m_iDCOThreatAct = CMD_EThreatAction.HOLD_DESTROY;
	        if (mission)
	            reinforcement.m_iDCOThreatAct = mission.m_eAction;

	        RecordReinforcement(ledger, stateHolder, worldTime);
	        sentThisCall++;

	        DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("Reinforcement %1/%2 (foot/motor) sent to %3",
	            ledger.m_iReinforcements,
	            DCO_QrfMax(),
	            targetPos.ToString()));
	    }

	    if (sentThisCall > 0)
	    {
	        DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("Total %1 groups sent, slots filled %2/%3",
	            sentThisCall,
	            ledger.m_iReinforcements,
	            DCO_QrfMax()));
	    }
	}

	protected void RecordReinforcement(CMD_ResponseLedgerEntry ledger, CMD_ThreatEntry stateHolder, float worldTime)
	{
		ledger.m_iReinforcements++;
		ledger.m_fLastReinforcementTime = worldTime;

		stateHolder.m_iReinforcementSentNumber = ledger.m_iReinforcements;
		stateHolder.m_bReinforcementSent       = true;
		stateHolder.m_fLastReinforcementTime   = worldTime;
	}

	protected void TrySendClusterArtillery(CMD_ThreatCluster cluster, float worldTime, bool force = false)
	{
		if (!m_Commander || !m_ArtySupport)
			return;

		CMD_ThreatEntry primary = cluster.GetPrimaryMember();
		if (!primary)
			return;

		bool isFresh = (worldTime - cluster.m_fFreshestUpdateTime) < m_fStalenessThreshold;
		bool defensive = isFresh && IsApproachingHeldObjective(cluster);
		bool dangerous = cluster.m_eClusterLevel == CMD_EThreatLevel.CRITICAL || cluster.m_eClusterLevel == CMD_EThreatLevel.HIGH || HasPriorityMember(cluster);
		if (isFresh && !dangerous && !defensive && !force)
			return;

		string source = "cluster";
		if (defensive && !dangerous)
			source = "defensive";
		if (force && !dangerous && !defensive)
			source = "dossier";

		string tier = m_ArtySupport.ResolveTier(cluster.m_fUncertainty);
		SCR_EAIArtilleryAmmoType shellType = SCR_EAIArtilleryAmmoType.HIGH_EXPLOSIVE;
		if (!isFresh || tier == "none")
		{
			if (!DCO_Util.IsNight())
			{
				if (isFresh && !primary.m_bReconSent)
					TrySendReconForThreat(primary, worldTime);
				return;
			}
			shellType = SCR_EAIArtilleryAmmoType.ILLUMINATION;
			tier = "illum";
		}

		CMD_ResponseLedgerEntry ledger = GetLedger(cluster.m_vCenterPos, worldTime);
		if (worldTime - ledger.m_fLastArtilleryTime < m_fArtilleryCooldown)
		{
			if (ledger.m_fCooldownLoggedAt < ledger.m_fLastArtilleryTime)
			{
				ledger.m_fCooldownLoggedAt = worldTime;
				CMD_FireMissionRequest probe = new CMD_FireMissionRequest(cluster.m_vCenterPos, shellType, worldTime);
				m_ArtySupport.ApplyTier(probe, tier, cluster.m_fUncertainty, source);
				CMD_ArtillerySupport.LogDenied(probe, "cooldown");
			}
			return;
		}

		int baseShellCount = CalculateClusterShellCount(cluster, worldTime);
		if (shellType == SCR_EAIArtilleryAmmoType.ILLUMINATION)
			baseShellCount = 1;

		CMD_FireMissionRequest request = new CMD_FireMissionRequest(cluster.m_vCenterPos, shellType, worldTime, baseShellCount, cluster.m_fFreshestUpdateTime, primary.m_fReportQuality);
		m_ArtySupport.ApplyTier(request, tier, cluster.m_fUncertainty, source);
		if (m_ArtySupport.HasFriendlyNearRequest(request, worldTime))
		{
			CMD_ArtillerySupport.LogDenied(request, "friendly");
			return;
		}

		DispatchArtilleryRequest(request, worldTime, source);

		ledger.m_fLastArtilleryTime  = worldTime;
		primary.m_bArtilleryCalled   = true;
		primary.m_fLastArtilleryTime = worldTime;
	}


	protected bool IsApproachingHeldObjective(CMD_ThreatCluster cluster)
	{
		if (!m_Commander)
			return false;

		vector vel = vector.Zero;
		foreach (CMD_ThreatEntry e : cluster.m_aMembers)
		{
			if (e)
				vel = vel + e.m_vVelocity;
		}

		FactionKey fk = m_Commander.GetCommanderFactionKey();
		foreach (CMD_AICommanderObjectiveComponent obj : m_Commander.DCO_AllObjectives())
		{
			if (!obj || !obj.GetOwner() || obj.GetOwnerFaction() != fk)
				continue;

			vector toObj = obj.GetOwner().GetOrigin() - cluster.m_vCenterPos;
			toObj[1] = 0;
			float dist = toObj.Length();
			if (dist <= obj.GetRadius())
				return true;
			if (dist <= obj.GetRadius() + 250.0 && vector.Dot(vel, toObj) > 0)
				return true;
		}
		return false;
	}

	protected int CalculateClusterShellCount(CMD_ThreatCluster cluster, float worldTime)
	{
		float shells = 2.0 + (0.5 * cluster.m_iTotalEstimatedEnemies);

		float age = worldTime - cluster.m_fFreshestUpdateTime;
		if (age > 30.0)
			shells += 1.0;

		return Math.Clamp(Math.Round(shells), 1, 12);
	}

	CMD_ThreatEntry FindThreatNear(vector pos)
	{
		return FindNearbyThreat(pos);
	}

	protected CMD_ThreatEntry FindThreatWithin(vector pos, float radius)
	{
		float rSq = radius * radius;
		foreach (CMD_ThreatEntry t : m_aThreats)
		{
			if (t && vector.DistanceSqXZ(t.m_vPosition, pos) <= rSq)
				return t;
		}
		return null;
	}

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

	float GetEngageThreshold()                 { return m_fEngageThreshold; }
	void  SetEngageThreshold(float v)          { m_fEngageThreshold = Math.Max(v, 0); }
	float GetReinforcementThreshold()          { return m_fReinforcementThreshold; }
	void  SetReinforcementThreshold(float v)   { m_fReinforcementThreshold = Math.Max(v, 0); }
	float GetClusterMinResponseScore()         { return m_fClusterMinResponseScore; }
	void  SetClusterMinResponseScore(float v)  { m_fClusterMinResponseScore = Math.Max(v, 0); }
	int   GetMaxReinforcementSent()            { return m_iMaxReinforcementSent; }
	protected int DCO_QrfMax()                 { return DCO_Force.Max(m_Commander, "qrf", m_iMaxReinforcementSent); }
	void  SetMaxReinforcementSent(int v)       { m_iMaxReinforcementSent = Math.Max(v, 0); }
	float GetReinforcementCooldown()           { return m_fReinforcementCooldown; }
	void  SetReinforcementCooldown(float v)    { m_fReinforcementCooldown = Math.Max(v, 0); }
	float GetThreatExpiry()                    { return m_fThreatExpiry; }
	void  SetThreatExpiry(float v)             { m_fThreatExpiry = Math.Max(v, 1); }
	float GetThinkInterval()                   { return m_fThinkInterval; }
	void  SetThinkInterval(float v)            { m_fThinkInterval = Math.Max(v, 1); }
	float GetFlankDistance()                   { return m_fFlankDistance; }
	void  SetFlankDistance(float v)            { m_fFlankDistance = Math.Max(v, 0); }
	float GetArtilleryCooldown()               { return m_fArtilleryCooldown; }
	void  SetArtilleryCooldown(float v)        { m_fArtilleryCooldown = Math.Max(v, 0); }

	FactionKey GetFactionKey()
	{
		if (!m_Commander)
			return string.Empty;
		return m_Commander.GetCommanderFactionKey();
	}

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		UpdateThreatDebug(timeSlice);

		if (!Replication.IsServer())
			return;

		m_fThinkTimer += timeSlice;
		if (m_fThinkTimer < m_fThinkInterval)
			return;

		m_fThinkTimer = 0.0;
		float worldTime = DCO_Util.NowS();
		int pt = DCO_Perf.Begin();
		Think(worldTime);
		DCO_Perf.End("cmd_threat", pt);
	}

	protected void UpdateThreatDebug(float timeSlice)
	{
		if (!m_bDebugMode)
		{
			if (!m_aDebugShapes.IsEmpty() || !m_aDebugTexts.IsEmpty())
			{
				m_aDebugShapes.Clear();
				m_aDebugTexts.Clear();
			}
			return;
		}

		m_fDebugTimer += timeSlice;
		if (m_fDebugTimer < m_fDebugRefreshInterval)
			return;

		m_fDebugTimer = 0.0;

		m_aDebugShapes.Clear();
		m_aDebugTexts.Clear();

		if (!DCO_DebugDraw.IsLocalPlayerInGM())
			return;

		int   flags     = DCO_DebugDraw.Flags();
		float worldTime = DCO_Util.NowS();

		DrawThreatHeader(flags, worldTime);
		DrawThreatEntries(flags, worldTime);
		DrawThreatClusters(flags, worldTime);
		DrawMissions(flags, worldTime);
	}

	protected void DrawMissions(int flags, float worldTime)
	{
		foreach (CMD_ThreatMission ms : m_aMissions)
		{
			CMD_ThreatAssessment a = ms.m_Last;
			if (!a)
				continue;

			if (a.m_eAsset != CMD_EAssetKind.NONE)
				m_aDebugShapes.Insert(Shape.CreateArrow(ms.m_vPos + "0 4 0", a.m_vAssetPos + "0 4 0", 3, 0xFFFF33AA, flags));

			string scores;
			foreach (int i, float sc : a.m_aScores)
				scores += string.Format("%1 %2  ", CMD_ThreatMission.ActionName(i).Substring(0, 3), sc.ToString(-1, 2));

			string text = string.Format("RESPONSE #%1  %2\n%3 x%4  unc %5m  ratio %6  match %7\nasset %8 eta %9s",
				ms.m_iId, CMD_ThreatMission.ActionName(ms.m_eAction), typename.EnumToString(CMD_EThreatType, a.m_eType), a.m_iEnemies,
				Math.Round(a.m_fUncertainty), a.m_fRatio.ToString(-1, 2), DCO_DebugDraw.YesNo(a.m_bCanMatch),
				typename.EnumToString(CMD_EAssetKind, a.m_eAsset), Math.Round(a.m_fAssetEta));
			text += string.Format("\n%1\ngroups %2  age %3s  timeout %4", scores, ms.m_aGroups.Count(), Math.Round(worldTime - ms.m_fStart), DCO_DebugDraw.YesNo(ms.m_bTimedOut));
			m_aDebugTexts.Insert(DCO_DebugDraw.SpawnText(ms.m_vPos + "0 46 0", text, 15.0, 0xFFFF33AA));
		}
	}

	protected void DrawThreatHeader(int flags, float worldTime)
	{
		vector p = GetOwner().GetOrigin();

		int clusterCount = 0;
		if (m_aDebugClusters)
			clusterCount = m_aDebugClusters.Count();

		string a = string.Format(
			"THREAT SYSTEM\nthreats %1   clusters %2\nthink every %3s   next in %4s",
			m_aThreats.Count(),
			clusterCount,
			DCO_DebugDraw.F1(m_fThinkInterval),
			DCO_DebugDraw.F1(Math.Max(m_fThinkInterval - m_fThinkTimer, 0.0)));

		string b = string.Format(
			"\nengage >= %1   reinforce >= %2   cluster gate >= %3\nexpiry %4s   merge %5m   cluster %6m",
			DCO_DebugDraw.F1(m_fEngageThreshold),
			DCO_DebugDraw.F1(m_fReinforcementThreshold),
			DCO_DebugDraw.F1(m_fClusterMinResponseScore),
			DCO_DebugDraw.F1(m_fThreatExpiry),
			Math.Round(m_fMergeRadius),
			Math.Round(m_fClusterRadius));

		string c = string.Format(
			"\nmax reinforce %1/cluster   cd %2s\nflank dist %3m   arty cd %4s   stale %5s",
			m_iMaxReinforcementSent,
			DCO_DebugDraw.F1(m_fReinforcementCooldown),
			Math.Round(m_fFlankDistance),
			DCO_DebugDraw.F1(m_fArtilleryCooldown),
			DCO_DebugDraw.F1(m_fStalenessThreshold));

		m_aDebugTexts.Insert(DCO_DebugDraw.SpawnText(
			Vector(p[0], p[1] + 64.0, p[2]), a + b + c, 16.0, 0xFFFF8822));
	}

	protected void DrawThreatEntries(int flags, float worldTime)
	{
		for (int i = 0; i < m_aThreats.Count(); i++)
		{
			CMD_ThreatEntry t = m_aThreats.Get(i);
			if (!t)
				continue;

			int color = DCO_DebugDraw.ThreatLevelColor(t.m_eThreatLevel);

			m_aDebugShapes.Insert(Shape.CreateSphere(color, flags, t.m_vBelievedPos, 1.5));
			if (t.m_fBelievedUncertainty > 0)
				m_aDebugShapes.Insert(Shape.CreateCylinder(color, flags | ShapeFlags.WIREFRAME, t.m_vBelievedPos, t.m_fBelievedUncertainty, 2.0));
			if (t.m_vTruePos != vector.Zero)
			{
				m_aDebugShapes.Insert(Shape.CreateSphere(0xFFFFFFFF, flags, t.m_vTruePos, 1.0));
				m_aDebugShapes.Insert(Shape.CreateArrow(t.m_vTruePos + Vector(0, 1, 0), t.m_vBelievedPos + Vector(0, 1, 0), 1.0, 0xFFFFFFFF, flags));
			}

			float age      = worldTime - t.m_fFirstReportTime;
			float lastSeen = worldTime - t.m_fLastUpdateTime;

			string stale;
			if (lastSeen >= m_fStalenessThreshold)
				stale = "  STALE";
			else
				stale = "";

			string head = string.Format(
				"THREAT %1/%2   %3\nscore %4   enemies %5   quality %6\nage %7s   last seen %8s ago%9",
				i + 1,
				m_aThreats.Count(),
				DCO_DebugDraw.ThreatLevelName(t.m_eThreatLevel),
				DCO_DebugDraw.F1(t.m_fPriorityScore),
				t.m_iEstimatedEnemyCount,
				DCO_DebugDraw.F1(t.m_fReportQuality),
				DCO_DebugDraw.F1(age),
				DCO_DebugDraw.F1(lastSeen),
				stale);

			string engaging = "none";
			if (t.m_sEngagingGroupName && t.m_sEngagingGroupName.GetOwner())
				engaging = t.m_sEngagingGroupName.GetOwner().GetName();

			string flags2 = string.Format(
				"\nengaged %1   reinforced %2/%3\nflank %4   arty %5   recon %6/%7\nengaging: %8",
				DCO_DebugDraw.YesNo(t.m_bEngaged),
				t.m_iReinforcementSentNumber,
				m_iMaxReinforcementSent,
				DCO_DebugDraw.YesNo(t.m_bFlankSent),
				DCO_DebugDraw.YesNo(t.m_bArtilleryCalled),
				DCO_DebugDraw.YesNo(t.m_bNeedsRecon),
				DCO_DebugDraw.YesNo(t.m_bReconSent),
				engaging);

			string intel = string.Format("\nuncertainty %1m   speed %2m/s",
				Math.Round(t.m_fBelievedUncertainty), DCO_DebugDraw.F1(t.m_vVelocity.Length()));

			m_aDebugTexts.Insert(DCO_DebugDraw.SpawnText(
				Vector(t.m_vBelievedPos[0], t.m_vBelievedPos[1] + 14.0, t.m_vBelievedPos[2]),
				head + flags2 + intel, 15.0, color));
		}
	}

	protected void DrawThreatClusters(int flags, float worldTime)
	{
		if (!m_aDebugClusters)
			return;

		for (int c = 0; c < m_aDebugClusters.Count(); c++)
		{
			CMD_ThreatCluster cl = m_aDebugClusters.Get(c);
			if (!cl || cl.m_aMembers.IsEmpty())
				continue;

			int color = DCO_DebugDraw.ThreatLevelColor(cl.m_eClusterLevel);

			m_aDebugShapes.Insert(Shape.CreateSphere(color, flags, cl.m_vCenterPos, 3.0));

			foreach (CMD_ThreatEntry m : cl.m_aMembers)
			{
				if (!m)
					continue;

				m_aDebugShapes.Insert(Shape.CreateArrow(
					Vector(cl.m_vCenterPos[0], cl.m_vCenterPos[1] + 2.0, cl.m_vCenterPos[2]),
					Vector(m.m_vBelievedPos[0], m.m_vBelievedPos[1] + 2.0, m.m_vBelievedPos[2]),
					1.5, color, flags));
			}

			string gate;
			if (cl.m_fCombinedScore < m_fClusterMinResponseScore)
				gate = "BELOW GATE -- no response";
			else if (cl.m_eClusterLevel == CMD_EThreatLevel.HIGH || cl.m_eClusterLevel == CMD_EThreatLevel.CRITICAL)
				gate = "active: flank + reinforce + arty";
			else
				gate = "active: reinforce + arty";

			m_aDebugTexts.Insert(DCO_DebugDraw.SpawnText(
				Vector(cl.m_vCenterPos[0], cl.m_vCenterPos[1] + 30.0, cl.m_vCenterPos[2]),
				string.Format(
					"CLUSTER %1/%2   %3\ncombined %4 vs gate %5\nenemies %6   members %7\n%8",
					c + 1,
					m_aDebugClusters.Count(),
					DCO_DebugDraw.ThreatLevelName(cl.m_eClusterLevel),
					DCO_DebugDraw.F1(cl.m_fCombinedScore),
					DCO_DebugDraw.F1(m_fClusterMinResponseScore),
					cl.m_iTotalEstimatedEnemies,
					cl.m_aMembers.Count(),
					gate),
				16.0, color));
		}
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

class CMD_ResponseLedgerEntry
{
	vector m_vPos;
	int    m_iReinforcements;
	float  m_fLastReinforcementTime = -1000;
	float  m_fLastArtilleryTime     = -1000;
	float  m_fCooldownLoggedAt      = -1000;
	float  m_fLastFlankTime         = -1000;
	float  m_fLastTouched;
}

class CMD_FlankTracker
{
	DCO_GroupUtilityComponent m_Group;
	float m_fExpiry;
	bool m_bArmored;
	SCR_AIWaypoint m_SweepWp;
	vector m_vTarget;
	int m_iMission = -1;
	int m_iRetargets;
}

class CMD_ThreatCluster
{
	vector m_vCenterPos;
	int m_iTotalEstimatedEnemies = 0;
	float m_fCombinedScore = 0.0;
	CMD_EThreatLevel m_eClusterLevel = CMD_EThreatLevel.NEGLIGIBLE;
	float m_fFreshestUpdateTime = 0.0;
	float m_fUncertainty = 0.0;
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