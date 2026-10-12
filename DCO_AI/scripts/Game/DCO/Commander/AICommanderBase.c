[ComponentEditorProps(category: "GameScripted/Commander")]
class AICommander_BaseComponentClass : ScriptComponentClass
{
	[Attribute("{35BD6541CBB8AC08}Prefabs/AI/Waypoints/AIWaypoint_Cycle.et", UIWidgets.ResourceNamePicker, desc: "Cycle waypoint for repeating patrol routes.", "et", category: "Commander 00. Waypoint Prefabs")]
	protected ResourceName m_sCycleWaypointPrefab;

	[Attribute("{750A8D1695BD6998}Prefabs/AI/Waypoints/AIWaypoint_Move.et", UIWidgets.ResourceNamePicker, desc: "Move waypoint for all commander move orders (radius 20 and completion All are set in code).", "et", category: "Commander 00. Waypoint Prefabs")]
	protected ResourceName m_sDefaultMoveWaypointPrefab;

	[Attribute("{93291E72AC23930F}Prefabs/AI/Waypoints/AIWaypoint_Defend.et", UIWidgets.ResourceNamePicker, desc: "Defend waypoint for outdoor defense sectors.", "et", category: "Commander 00. Waypoint Prefabs")]
	protected ResourceName m_sDefaultDefendWaypointPrefab;

	[Attribute("{F22E21BBBF46819E}Prefabs/AI/Waypoints/AIWaypoint_Garrison.et", UIWidgets.ResourceNamePicker, desc: "Garrison waypoint for groups that enter buildings. The GarrisonWaypointPrefab JSON preset can override it.", "et", category: "Commander 00. Waypoint Prefabs")]
	protected ResourceName m_sGarrisonWaypointPrefab;

	[Attribute("{712F4795CF8B91C7}Prefabs/AI/Waypoints/AIWaypoint_GetIn.et", UIWidgets.ResourceNamePicker, desc: "Get-in waypoint (transport and group vehicles).", "et", category: "Commander 00. Waypoint Prefabs")]
	protected ResourceName m_sDefaultGetInWaypointPrefab;

	[Attribute("{C40316EE26846CAB}Prefabs/AI/Waypoints/AIWaypoint_GetOut.et", UIWidgets.ResourceNamePicker, desc: "Get-out waypoint.", "et", category: "Commander 00. Waypoint Prefabs")]
	protected ResourceName m_sDefaultGetOutWaypointPrefab;

	[Attribute("{C40316EE26846CAB}Prefabs/AI/Waypoints/AIWaypoint_GetOut.et", UIWidgets.ResourceNamePicker, desc: "Move waypoint for tasking (GetDefaultMoveTaskPrefab).", "et", category: "Commander 00. Waypoint Prefabs")]
	protected ResourceName m_sDefaultTaskPlayerMovePrefab;

	[Attribute("{C524700A27CFECDD}Prefabs/AI/Waypoints/AIWaypoint_ArtillerySupport.et", UIWidgets.ResourceNamePicker, desc: "Fire waypoint for commander mortar and artillery units.", "et", category: "Commander 00. Waypoint Prefabs")]
	protected ResourceName m_sDefaultShootArtilleryPrefab;

	[Attribute("{70FE5AA8B4BCA67A}Prefabs/AI/Waypoints/AIWaypoint_Suppress_Editor.et", UIWidgets.ResourceNamePicker, desc: "Suppress area waypoint (fix group or support by fire).", "et", category: "Commander 00. Waypoint Prefabs")]
	protected ResourceName m_sDefaultSuppressPrefab;

	[Attribute("{5C3A91E7D24B8F06}Prefabs/AI/Waypoints/AIWaypoint_TacticRole.et", UIWidgets.ResourceNamePicker, desc: "Tactic x role waypoint (Fire and Maneuver: Fix and Assault).", "et", category: "Commander 00. Waypoint Prefabs")]
	protected ResourceName m_sTacticRoleWaypointPrefab;

	ResourceName GetCycleWaypointPrefab()
	{
		return m_sCycleWaypointPrefab;
	}

	ResourceName GetDefaultMoveWaypointPrefab()
	{
		return m_sDefaultMoveWaypointPrefab;
	}

	ResourceName GetDefaultDefendWaypointPrefab()
	{
		return m_sDefaultDefendWaypointPrefab;
	}

	ResourceName GetGarrisonWaypointPrefab()
	{
		return m_sGarrisonWaypointPrefab;
	}

	ResourceName GetDefaultGetInWaypointPrefab()
	{
		return m_sDefaultGetInWaypointPrefab;
	}

	ResourceName GetDefaultGetOutWaypointPrefab()
	{
		return m_sDefaultGetOutWaypointPrefab;
	}

	ResourceName GetDefaultMoveTaskPrefab()
	{
		return m_sDefaultTaskPlayerMovePrefab;
	}

	ResourceName GetShootArtilleryWaypointPrefab()
	{
		return m_sDefaultShootArtilleryPrefab;
	}

	ResourceName GetDefaultSuppressPrefab()
	{
		return m_sDefaultSuppressPrefab;
	}

	ResourceName GetTacticRoleWaypointPrefab()
	{
		return m_sTacticRoleWaypointPrefab;
	}
}

class AICommander_BaseComponent : ScriptComponent
{
	protected float m_fThinkScaleSlow = 1.5;
	protected float m_fThinkScaleFast = 0.5;
	protected float m_fRetreatScaleTimid = 2.0;
	protected float m_fRetreatScaleSteady = 0.5;
	protected float m_fStalemateScaleImpatient = 0.4;
	protected float m_fStalemateScalePatient = 2.5;
	protected float m_fPatienceFast = 0.2;
	protected float m_fPatienceNormal = 0.5;
	protected float m_fPatienceSlow = 0.8;
	protected float m_fDecisionFastScore = 1.33;
	protected float m_fDecisionSlowScore = 0.67;
	protected float m_fRandomTraitJitter = 0.5;
	protected float m_fShiftPerAggression = 0.15;
	protected float m_fShiftPerCourage = 0.1;
	protected float m_fShiftMax = 0.4;
	protected float m_fMoraleThresholdSpread = 0.6;

	protected float m_fRoeDistScaleCautious = 0.8;
	protected float m_fRoeDistScaleAggressive = 1.3;
	protected float m_fHoldEngageDistMax = 80.0;
	protected float m_fFireWhenSuppressedResilience = 0.6;

	protected float m_fDefendSharePerReserve = 2.5;
	protected float m_fDefendShareMin = 0.1;
	protected int m_iOpStallMin_s = 60;
	protected float m_fOpIntelPctMin = 10.0;
	protected float m_fOpLossPctMin = 5.0;
	protected float m_fRatioScaleCautious = 1.15;
	protected float m_fRatioScaleAggressive = 0.85;
	protected float m_fEnemyEstimateMargin = 150.0;
	protected float m_fArmorStrengthWeight = 3.0;
	protected float m_fAssumedGroupSize = 6.0;
	protected float m_fReinforceSkipShare = 0.7;
	protected float m_fAttackWatchdogIdle_s = 300.0;
	protected float m_fDoctrineSkipCooldown_s = 120.0;
	protected float m_fPlanNeedMargin = 1.2;
	protected int m_iOffensiveRankExtra = 4;
	protected int m_iLostObjOutnumber = 3;
	protected float m_fCtrlLogEvery_s = 30.0;

	protected float m_fFullSquadSize = 12.0;
	protected float m_fTaskScoreBase = 100.0;
	protected int m_iReconIdealSize = 3;
	protected float m_fReconSizePenalty = 12.0;
	protected float m_fReconHeavyPenalty = 10.0;
	protected float m_fReconCapBonus = 40.0;
	protected float m_fLightInfBonus = 20.0;
	protected float m_fArmedVehicleBonus = 10.0;
	protected int m_iLightInfMin = 2;
	protected int m_iLightInfMax = 4;
	protected float m_fFlankIdealStrength = 50.0;
	protected float m_fMobilityBonus = 25.0;
	protected float m_fDistPenaltyStep = 25.0;
	protected float m_fDistPenaltyMax = 80.0;
	protected float m_fCapabilityMatchBonus = 30.0;

	protected float m_fReconMinStandoff = 100.0;
	protected float m_fReconBand = 200.0;
	protected int m_iReconSpotSamples = 16;

	protected float m_fReserveDistFalloff = 500.0;
	protected float m_fReserveSpreadHalfArc = 90.0;
	protected float m_fReserveSpreadMin = 2.0;
	protected float m_fReserveSpreadMax = 6.0;
	protected float m_fPatrolVehicleReach = 150.0;

	protected float m_fFlankAngleMin = 55.0;
	protected float m_fFlankAngleMax = 80.0;
	protected float m_fFlankHeightScale = 20.0;
	protected float m_fFlankScoreJitter = 0.3;
	protected float m_fFlankStagingAngle = 25.0;
	protected float m_fFlankSwingOut = 220.0;
	protected float m_fFlankSwingAngle = 110.0;
	protected float m_fFlankEdgeOut = 60.0;
	protected float m_fFlankEdgeAngle = 100.0;
	protected float m_fFlankSearchArc = 70.0;
	protected float m_fFlankArcOffset = 125.0;
	protected float m_fApproachRadiusMul = 1.4;
	protected float m_fAssaultSearchArc = 90.0;
	protected float m_fSearchWpSpacing = 50.0;
	protected int m_iMinWpPerRing = 3;
	protected float m_fRingSectorSpacing = 1.5;

	protected float m_fStagingAxisSameDot = 0.996;
	protected int m_iStagingDepthSteps = 3;
	protected float m_fStagingDepthStep = 100.0;
	protected int m_iStagingAngleSteps = 2;
	protected float m_fStagingAngleStep = 20.0;
	protected float m_fCoverEyeHeight = 1.7;
	protected float m_fCoverTargetHeight = 2.0;
	protected float m_fCoverBlockedFrac = 0.85;
	protected float m_fCoverBonus = 2.0;
	protected float m_fCoverAnglePenalty = 0.15;
	protected float m_fCoverDepthPenalty = 0.25;
	protected float m_fStagingPathClearance = 150.0;
	protected int m_iStagingClearTries = 6;
	protected float m_fHostileObjKeepOut = 30.0;
	protected float m_fStagingClearStep = 80.0;
	protected float m_fStagingSlotSpacing = 22.0;
	protected float m_fStagingArriveRadius = 80.0;
	protected float m_fStageContactForward = 400.0;
	protected float m_fStageContactRadius = 150.0;
	protected int m_iReleaseNotifyDelay_ms = 100;

	protected float m_fRouteLegJitter = 8.0;
	protected float m_fDestJitterMin = 3.0;
	protected float m_fDestJitterMax = 8.0;
	protected int m_iNudgeSteps = 4;
	protected float m_fMoveWpRadius = 20.0;
	protected float m_fGarrisonWpMinRadius = 30.0;
	protected float m_fSuppressIntelMatch = 150.0;
	protected float m_fAltPosMinRadius = 5.0;
	protected float m_fAltPosRadiusFloor = 6.0;
	protected float m_fSectorModLow = 0.95;
	protected float m_fSectorModHigh = 1.10;
	protected float m_fStaticsSearchMargin = 50.0;
	protected int m_iMannedStaticWeight = 2;

	protected float m_fIdleArmedSearch = 2000.0;
	protected float m_fVehOwRecheck_s = 20.0;
	protected float m_fVehOwSiteTolerance = 40.0;
	protected float m_fVehOwEdgeKeep = 300.0;
	protected float m_fVehOwBehindAssault = 100.0;
	protected float m_fVehicleEyeHeight = 2.2;
	protected float m_fStagingAimHeight = 1.5;
	protected float m_fVehOwBlockTolerance = 10.0;

	protected float m_fScopeFixOpenDelay_s = 30.0;
	protected float m_fScopeFixPosture_ms = 120000.0;
	protected float m_fScopeFixArrive = 50.0;
	protected float m_fScopeFixProgress = 30.0;
	protected float m_fScopeFixStall_s = 45.0;
	protected int m_iScopeFixTries = 3;
	protected float m_fScopeFixOwMin = 200.0;
	protected float m_fScopeFixOwMax = 400.0;
	protected float m_fScopeFixOwIdeal = 300.0;
	protected float m_fScopeFixIntelMargin = 50.0;

	protected float m_fSweepGroupSearch = 1500.0;
	protected float m_fSweepMargin = 50.0;
	protected float m_fCaptureBlockLogEvery_s = 30.0;

	protected float m_fGetInBlock_s = 300.0;
	protected float m_fGetInRetryWindow_s = 60.0;
	protected int m_iGetInMaxTries = 2;
	protected float m_fGetInWpRadius = 25.0;
	protected float m_fOwnedVehicleReachMul = 3.0;
	protected float m_fTransportEnemyObjClearance = 300.0;

	protected float m_fBaseThinkInterval;
	protected int m_iBaseRetreatThreshold;
	protected float m_fBaseStalemateResponseCooldown;

	[Attribute("", UIWidgets.ResourceNamePicker, desc: "Entity prefab with AICommander_ManagerComponent. Spawned automatically (server) if this entity initialises before a manager exists in the world. Empty = no auto spawn.", "et", category: "Commander 01. Identity")]
	protected ResourceName m_sManagerPrefab;

	[Attribute("", UIWidgets.Font, desc: "UID of the Commander.", category: "Commander 01. Identity"), RplProp()]
	protected string m_sCommanderUID;

	[Attribute("", UIWidgets.Auto, desc: "Faction Key of the Commander.", category: "Commander 01. Identity")]
	protected FactionKey m_sFactionKey;

	[Attribute("3.0", UIWidgets.Auto, "Number of the Objective Can be processed at the same time", category: "Commander 04. Attack")]
	protected int m_fObjectiveAtTheSameTime;

	[Attribute("1", UIWidgets.CheckBox, "Use Synchronized Attack (groups gather at staging until full or timeout, then attack together). Off = previous behavior (each group goes to the objective as soon as it gets a slot).", category: "Commander 04. Attack")]
	protected bool m_bUseSynchronizedAttack;

	[Attribute("60.0", UIWidgets.EditBox, "Maximum time (s) to wait for the ASSAULT slots to fill before the synchronized attack is released with the groups already gathered.", category: "Commander 04. Attack")]
	protected float m_fSyncAttackMaxWaitTime;

	[Attribute("30.0", UIWidgets.EditBox, "Think cycle interval (s).", category: "Commander 02. Timing")]
	protected float m_fThinkInterval;

	[Attribute("60.0", UIWidgets.EditBox, "Delay Before Start First Iteration", category: "Commander 02. Timing")]
	protected float m_fDelayFirstIteration;

	[Attribute("120.0", UIWidgets.EditBox, "Cooldown (s) before the stalemate response can trigger again per objective.", category: "Commander 02. Timing")]
	protected float m_fStalemateResponseCooldown;

	[Attribute("2", UIWidgets.EditBox, "Minimum group unit count before the group is forced to retreat.", category: "Commander 07. Retreat")]
	protected int m_iRetreatThreshold;

	[Attribute("15.0", UIWidgets.EditBox, "Capture progress check interval (s).", category: "Commander 02. Timing")]
	protected float m_fCaptureCheckInterval;

	[Attribute("250.0", UIWidgets.EditBox, "Patrol radius for recon groups sent to the frontline (not to a specific objective).", category: "Commander 11. Frontline & Recon")]
	protected float m_fFrontlineReconRadius;

	[Attribute("180.0", UIWidgets.EditBox, "How long (s) a group stays on frontline recon before it returns to the pool (RESERVE).", category: "Commander 11. Frontline & Recon")]
	protected float m_fFrontlineReconDuration;

	[Attribute("1", UIWidgets.EditBox, "Defend groups per objective that enter buildings (garrison). The rest form the outdoor sector ring. 0 = no garrison.", category: "Commander 08. Defend & Garrison")]
	protected int m_iGarrisonGroupsPerObjective;

	[Attribute("1", UIWidgets.ComboBox, "Garrison mode used by the commander.", category: "Commander 08. Defend & Garrison", enums: ParamEnumArray.FromEnum(DCO_EGarrisonMode))]
	protected DCO_EGarrisonMode m_eGarrisonMode;

	protected ref map<CMD_AICommanderObjectiveComponent, ref array<DCO_GroupUtilityComponent>> m_mObjGarrison = new map<CMD_AICommanderObjectiveComponent, ref array<DCO_GroupUtilityComponent>>();

	[Attribute("300", UIWidgets.Slider, "DEFEND/GARRISON: trigger line. Free fire when an enemy is this close (m) to the group; MG x2 (max 800 m).", params: "0 1000 10", category: "Commander 08. Defend & Garrison")]
	protected float m_fDefendEngageDist;

	[Attribute("50", UIWidgets.Slider, "DEFEND: also free fire when an enemy enters the group's objective radius plus this margin (m).", params: "0 300 5", category: "Commander 08. Defend & Garrison")]
	protected float m_fDefendObjectiveMargin;

	float GetDefendEngageDist() { return m_fDefendEngageDist; }
	float GetDefendObjectiveMargin() { return m_fDefendObjectiveMargin; }

	[Attribute("1", UIWidgets.CheckBox, "Idle ARMORED groups and groups with their own vehicle patrol mounted (larger radius, slower, on roads).", category: "Commander 10. Patrol")]
	protected bool m_bVehiclePatrol;

	[Attribute("2.5", UIWidgets.EditBox, "Vehicle patrol radius multiplier compared with foot patrols.", category: "Commander 10. Patrol")]
	protected float m_fVehiclePatrolRadiusMul;

	[Attribute("2", UIWidgets.EditBox, "Maximum simultaneous frontline recon groups.", category: "Commander 11. Frontline & Recon")]
	protected int m_iMaxFrontlineRecon;

	[Attribute("500.0", UIWidgets.EditBox, "Maximum distance (m) an idle group may be pulled for a patrol. If the nearest candidate (HQ or captured objective) is farther, it patrols locally where it is.", category: "Commander 10. Patrol")]
	protected float m_fMaxPatrolPullDistance;

	[Attribute("80.0", UIWidgets.EditBox, "How many meters the patrol center moves toward the frontline each think cycle. 0 = center stays at its anchor.", category: "Commander 10. Patrol")]
	protected float m_fPatrolFrontlineDrift;

	[Attribute("400.0", UIWidgets.EditBox, "Brake: the patrol center stops creeping once it is closer than this to the nearest enemy objective (plus the objective radius). Stops reserves wandering into the fight alone.", category: "Commander 10. Patrol")]
	protected float m_fPatrolFrontlineStandoff;

	[Attribute("1", UIWidgets.CheckBox, desc: "Move patrol points to higher ground with better line of sight. Uses raycasts; turn off if it feels heavy.", category: "Commander 10. Patrol")]
	protected bool m_bPatrolTerrainScoring;

	[Attribute("12", UIWidgets.EditBox, "Number of points whose terrain may be scored per think cycle, shared by ALL idle groups. Once used up, the rest use plain geometric points. Keeps raycasts in check when many groups are idle.", category: "Commander 10. Patrol")]
	protected int m_iPatrolSmartBudget;

	[Attribute("150.0", UIWidgets.EditBox, "A new patrol center counts as too similar if it is closer than this to a recently used center.", category: "Commander 10. Patrol")]
	protected float m_fPatrolMemoryRadius;

	[Attribute("5", UIWidgets.EditBox, "How many recent patrol centers are remembered per group.", category: "Commander 10. Patrol")]
	protected int m_iPatrolMemorySize;

	[Attribute("0.5", UIWidgets.Range, "Position of the frontline node along each pair (our objective -> nearest threat). 0.5 = no man's land. Below 0.5 leans toward our territory (defensive), above 0.5 toward the enemy (aggressive).", params: "0.1 0.9 0.05", category: "Commander 11. Frontline & Recon")]
	protected float m_fFrontlineNodeBias;

	[Attribute("220.0", UIWidgets.EditBox, "Arc span (degrees) when we hold only ONE objective. Over 180 so the flanks are covered too; only the rear stays open.", category: "Commander 11. Frontline & Recon")]
	protected float m_fEnvelopeArcDeg;

	[Attribute("2.0", UIWidgets.EditBox, "Screen arc radius, as a multiplier of the objective radius.", category: "Commander 11. Frontline & Recon")]
	protected float m_fEnvelopeRadiusMul;

	[Attribute("250.0", UIWidgets.EditBox, "Minimum radius of the screen arc, for objectives with a small radius.", category: "Commander 11. Frontline & Recon")]
	protected float m_fEnvelopeMinRadius;

	[Attribute("5", UIWidgets.EditBox, "Number of nodes on the screen arc. More = smoother curve.", category: "Commander 11. Frontline & Recon")]
	protected int m_iEnvelopeNodes;

	[Attribute("0.6", UIWidgets.Range, "Patrol radius multiplier for the smallest groups.", params: "0.2 2 0.05", category: "Commander 10. Patrol")]
	protected float m_fPatrolRadiusMulMin;

	[Attribute("1.6", UIWidgets.Range, "Patrol radius multiplier for full groups (12 soldiers).", params: "0.2 3 0.05", category: "Commander 10. Patrol")]
	protected float m_fPatrolRadiusMulMax;

	[Attribute("0.25", UIWidgets.Range, "The rally point is placed (commander-to-objective distance x this value) FROM THE OBJECTIVE, then clamped to the Min/Max below.", params: "0.05 1 0.01", category: "Commander 06. Staging & Routes")]
	protected float m_fStagingLegFraction;

	[Attribute("350.0", UIWidgets.EditBox, "Minimum distance of the rally point from the objective. Raised automatically if smaller than objective radius + margin, because staging INSIDE the radius keeps the objective CONTESTED and capture never starts.", category: "Commander 06. Staging & Routes")]
	protected float m_fStagingMinDistance;

	[Attribute("600.0", UIWidgets.EditBox, "Maximum distance of the rally point from the objective. This cap keeps the last leg short however far away the commander is.", category: "Commander 06. Staging & Routes")]
	protected float m_fStagingMaxDistance;

	[Attribute("60.0", UIWidgets.EditBox, "Margin outside the objective radius for the rally point.", category: "Commander 06. Staging & Routes")]
	protected float m_fStagingMargin;

	[Attribute("200.0", UIWidgets.EditBox, "One intermediate waypoint is created every this many meters of travel, so groups follow a stepped route instead of one long straight line. 0 = off, a single destination waypoint.", category: "Commander 06. Staging & Routes")]
	protected float m_fWaypointLegDistance;

	[Attribute("120.0", UIWidgets.EditBox, "How far an intermediate point may be shifted sideways to find a safe spot (avoiding water and enemy objective radii).", category: "Commander 06. Staging & Routes")]
	protected float m_fWaypointNudgeRadius;

	[Attribute("15", UIWidgets.EditBox, "Maximum sweep waypoints created per group inside an objective. Each waypoint is a spawned entity, so this stops large objectives creating hundreds of entities per attack. Density is reduced proportionally; the sweep shape stays the same.", category: "Commander 06. Staging & Routes")]
	protected int m_iMaxSearchWaypoints;

	[Attribute("90.0", UIWidgets.EditBox, "How long (s) a suppress support group covers the assault force after release before returning to the pool.", category: "Commander 04. Attack")]
	protected float m_fAssaultSuppressDuration;

	[Attribute("45.0", UIWidgets.EditBox, "How long (s) a suppress cover group covers the contact direction after another group retreats.", category: "Commander 07. Retreat")]
	protected float m_fRetreatCoverDuration;

	[Attribute("50.0", UIWidgets.EditBox, "Suppress radius used to cover a retreat.", category: "Commander 07. Retreat")]
	protected float m_fRetreatCoverRadius;

	[Attribute("400.0", UIWidgets.EditBox, "Minimum distance before looking for transport.", category: "Commander 13. Transport")]
	protected float m_fTransportDistanceThreshold;

	[Attribute("175.0", UIWidgets.EditBox, "Search radius (m) for transport vehicles around the group.", category: "Commander 13. Transport")]
	protected float m_fVehicleSearchRadius;

	float GetVehicleSearchRadius() { return m_fVehicleSearchRadius; }
	void SetVehicleSearchRadius(float r) { m_fVehicleSearchRadius = Math.Clamp(r, 25, 1000); }

	[Attribute("50.0", UIWidgets.EditBox, "Radius Close To Commander", category: "Commander 08. Defend & Garrison")]
	protected float m_fBaseRadius;

	[Attribute("20", UIWidgets.Slider, "Reserve policy: percent of forces always kept in reserve (not committed to attack, recon or flank). Also sets the share of idle groups that defend in BALANCED mode. 0 = no reserve.", params: "0 50 1", category: "Commander 12. Reserve & Reinforcement")]
	protected float m_fReservePercent;

	[Attribute("0", UIWidgets.CheckBox, "All-in: commit ALL forces to the attack, with no reserve and no defend share in BALANCED. Defenders of an objective under attack are still reinforced.", category: "Commander 12. Reserve & Reinforcement")]
	protected bool m_bAllIn;

	[Attribute("0", UIWidgets.CheckBox, "All-in includes defenders of rear objectives (safe, far from the frontline): they are released to join the attack.", category: "Commander 12. Reserve & Reinforcement")]
	protected bool m_bAllInRearDefenders;

	protected bool m_bDCOObjCapGM;
	protected int m_iDCOObjCapInit = -1;
	protected string m_sDCOObjCapLog;
	protected string m_sDCOObjOverLog;
	protected string m_sDCOK2Log;
	protected string m_sDCOFocusRatioLog;
	protected string m_sDCODefOrderLog;

	int GetObjectivesAtOnce()				{ return m_fObjectiveAtTheSameTime; }

	void SetObjectivesAtOnce(float value)
	{
		m_fObjectiveAtTheSameTime = Math.Clamp(Math.Round(value), 1, 10);
		m_bDCOObjCapGM = true;
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("obj_cap_set cmd=%1 value=%2 src=gm", m_sCommanderUID, m_fObjectiveAtTheSameTime));
	}

	int DCO_ObjectiveCap()
	{
		int doc = DCO_Force.Max(this, "objectives");
		bool explicit = m_bDCOObjCapGM || (m_iDCOObjCapInit >= 0 && m_fObjectiveAtTheSameTime != m_iDCOObjCapInit);
		int cap = Math.Min(doc, m_fObjectiveAtTheSameTime);
		string src = "doctrine";
		if (explicit)
		{
			cap = m_fObjectiveAtTheSameTime;
			src = "commander_setting";
		}
		else if (m_fObjectiveAtTheSameTime < doc)
			src = "commander_setting";
		string key = cap.ToString() + "|" + src;
		if (key != m_sDCOObjCapLog)
		{
			m_sDCOObjCapLog = key;
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("obj_cap cmd=%1 cap=%2 src=%3 setting=%4 doctrine=%5 explicit=%6", m_sCommanderUID, cap, src, m_fObjectiveAtTheSameTime, doc, DCO_Util.B01(explicit)));
		}
		return cap;
	}

	[Attribute("", UIWidgets.EditBox, "Objective this commander attacks first (name as shown on the map or the entity name). Empty = none. The GM can change it in the DCO Commander: Style & Attack tab.", category: "Commander 04. Attack")]
	protected string m_sDCOFocusObjective;

	[Attribute("0", UIWidgets.CheckBox, "Force focus: attack the focus objective even when the doctrine says wait or the force ratio is too low. Recon still runs (max 2 minutes) and at least 2 squads are still needed.", category: "Commander 04. Attack")]
	protected bool m_bDCOFocusForce;

	protected CMD_AICommanderObjectiveComponent m_DCOFocus;
	protected bool m_bDCOFocusResolved;
	protected string m_sDCOFocusPickLog;
	protected float m_fDCOFocusReconStart = -1;
	protected float m_fDCOFocusSetAt = -1;
	protected ref map<string, float> m_mDCOAssignSkipAt;

	CMD_AICommanderObjectiveComponent DCO_GetFocus()	{ return m_DCOFocus; }
	bool DCO_FocusForced()							{ return m_DCOFocus && m_bDCOFocusForce; }
	bool DCO_IsForcedFocus(CMD_AICommanderObjectiveComponent obj)	{ return obj && obj == m_DCOFocus && m_bDCOFocusForce; }

	bool SetFocusObjective(CMD_AICommanderObjectiveComponent obj, bool force, string by = "script", int playerId = -1)
	{
		if (!obj)
		{
			ClearFocus("set_null", by, playerId);
			return false;
		}
		if (!obj.DCO_CanProcess(m_sCommanderUID) || obj.IsCommanderBlackListed(m_sCommanderUID) || DCO_RoleBlocksAttack(obj, "focus"))
		{
			DCO_Perf.Count("obj_focus:reject");
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.WARN, DCO_Log.IdCmd(this), string.Format("obj_focus_reject cmd=%1 obj=%2 reason=not_allowed by=%3 player=%4", m_sCommanderUID, obj.GetLogName(), by, playerId));
			DCO_PlayerComms.SendToGameMasters("DCO", string.Format("%1 cannot focus %2: it does not handle this objective.", m_sCommanderUID, DCO_ObjAssign.GmName(obj)));
			return false;
		}
		string from = "none";
		if (m_DCOFocus)
			from = m_DCOFocus.GetLogName();
		if (m_DCOFocus && m_DCOFocus != obj)
			DCO_PlayerComms.SendToGameMasters("DCO", string.Format("%1's focus moves from %2 to %3.", m_sCommanderUID, DCO_ObjAssign.GmName(m_DCOFocus), DCO_ObjAssign.GmName(obj)));
		m_DCOFocus = obj;
		m_bDCOFocusForce = force;
		m_bDCOFocusResolved = true;
		m_sDCOFocusObjective = obj.GetLogName();
		m_fDCOFocusReconStart = -1;
		m_fDCOFocusSetAt = DCO_Util.NowS();
		m_sDCOFocusPickLog = string.Empty;
		DCO_Perf.Count("obj_focus:set");
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("obj_focus_set cmd=%1 obj=%2 force=%3 from=%4 by=%5 player=%6", m_sCommanderUID, obj.GetLogName(), DCO_Util.B01(force), from, by, playerId));
		return true;
	}

	void ClearFocus(string why = "clear", string by = "script", int playerId = -1)
	{
		if (!m_DCOFocus && m_sDCOFocusObjective.IsEmpty())
			return;
		string name = m_sDCOFocusObjective;
		if (m_DCOFocus)
			name = m_DCOFocus.GetLogName();
		m_DCOFocus = null;
		m_sDCOFocusObjective = string.Empty;
		m_bDCOFocusForce = false;
		m_bDCOFocusResolved = true;
		m_fDCOFocusReconStart = -1;
		m_sDCOFocusPickLog = string.Empty;
		DCO_Perf.Count("obj_focus:done");
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("obj_focus_done cmd=%1 obj=%2 why=%3 by=%4 player=%5", m_sCommanderUID, name, why, by, playerId));
	}

	void DCO_SetFocusForce(bool force, string by, int playerId)
	{
		if (m_bDCOFocusForce == force)
			return;
		m_bDCOFocusForce = force;
		string name = "none";
		if (m_DCOFocus)
			name = m_DCOFocus.GetLogName();
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("obj_focus_set cmd=%1 obj=%2 force=%3 by=%4 player=%5", m_sCommanderUID, name, DCO_Util.B01(force), by, playerId));
	}

	protected CMD_AICommanderObjectiveComponent DCO_FindObjectiveByName(string name)
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr || name.IsEmpty())
			return null;
		string n2 = name;
		n2.Replace(" ", "_");
		foreach (CMD_AICommanderObjectiveComponent o : mgr.m_aObjective)
		{
			if (!o || !o.GetOwner())
				continue;
			if (o.GetLogName() == n2 || o.GetDisplayName() == name || o.GetOwner().GetName() == name)
				return o;
		}
		return null;
	}

	protected void DCO_FocusTick(float worldTime)
	{
		if (!m_bDCOFocusResolved)
		{
			m_bDCOFocusResolved = true;
			if (!m_sDCOFocusObjective.IsEmpty())
			{
				CMD_AICommanderObjectiveComponent found = DCO_FindObjectiveByName(m_sDCOFocusObjective);
				if (found)
					SetFocusObjective(found, m_bDCOFocusForce, "we");
				else
					DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.WARN, DCO_Log.IdCmd(this), string.Format("obj_focus_reject cmd=%1 obj=%2 reason=not_found by=we", m_sCommanderUID, m_sDCOFocusObjective));
			}
		}
		if (!m_DCOFocus)
		{
			if (!m_sDCOFocusObjective.IsEmpty())
				ClearFocus("deleted", "auto");
			return;
		}
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr || !mgr.m_aObjective.Contains(m_DCOFocus))
		{
			ClearFocus("removed", "auto");
			return;
		}
		if (m_DCOFocus.IsCapturedBy(m_sFactionKey))
		{
			ClearFocus("captured", "auto");
			return;
		}
		if (!m_DCOFocus.DCO_CanProcess(m_sCommanderUID))
		{
			ClearFocus("not_allowed", "auto");
			return;
		}
		if (DCO_RoleBlocksAttack(m_DCOFocus, "focus"))
			ClearFocus("role_blocked", "auto");
	}

	float DCO_FocusSetAt()
	{
		return m_fDCOFocusSetAt;
	}

	bool DCO_MayAttack(CMD_AICommanderObjectiveComponent obj)
	{
		return obj && obj.DCO_CanProcess(m_sCommanderUID) && !DCO_RoleBlocksAttack(obj, "op") && !DCO_PhaseSkip(obj, "op");
	}

	bool DCO_PhaseSkip(CMD_AICommanderObjectiveComponent obj, string path)
	{
		string why;
		if (!obj || DCO_ObjPhase.ActiveFor(obj, m_sFactionKey, why))
			return false;
		if (DCO_RoleOnce(obj.GetLogName() + "|phase|" + why, 300.0))
		{
			DCO_Perf.Count("obj_phase:wait_" + path);
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("obj_gate_wait cmd=%1 obj=%2 path=%3 why=%4", m_sCommanderUID, obj.GetLogName(), path, why));
		}
		return true;
	}

	protected ref array<CMD_AICommanderObjectiveComponent> m_aDCOHandedOver;
	protected ref map<CMD_AICommanderObjectiveComponent, float> m_mDCORoleSince;
	protected ref map<CMD_AICommanderObjectiveComponent, DCO_GroupUtilityComponent> m_mDCORoleKeep;
	protected ref map<CMD_AICommanderObjectiveComponent, float> m_mDCORoleKeepAt;

	int DCO_Role(CMD_AICommanderObjectiveComponent obj)
	{
		if (!obj || DCO_Ctx.TuneForCmd(this, "obj.role_on", 1) <= 0)
			return 0;
		return obj.DCO_RoleOf(m_sCommanderUID);
	}

	protected bool DCO_RoleOnce(string key, float secs = 60.0)
	{
		if (!m_mDCOAssignSkipAt)
			m_mDCOAssignSkipAt = new map<string, float>();
		float now = DCO_Util.NowS();
		float at;
		if (m_mDCOAssignSkipAt.Find(key, at) && now < at)
			return false;
		m_mDCOAssignSkipAt.Set(key, now + secs);
		return true;
	}

	bool DCO_RoleBlocksAttack(CMD_AICommanderObjectiveComponent obj, string path)
	{
		if (DCO_Role(obj) != 2 || obj.CheckIsItLost(m_sFactionKey))
			return false;
		if (DCO_RoleOnce(obj.GetLogName() + "|role|" + path))
		{
			DCO_Perf.Count("obj_role:block_" + path);
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("obj_role_block cmd=%1 obj=%2 role=defend_only path=%3", m_sCommanderUID, obj.GetLogName(), path));
		}
		return true;
	}

	bool DCO_RoleSkipDefend(CMD_AICommanderObjectiveComponent obj)
	{
		if (DCO_Role(obj) != 1)
			return false;
		if (!m_aDCOHandedOver)
			m_aDCOHandedOver = {};
		if (m_aDCOHandedOver.Contains(obj))
		{
			DCO_RoleKeepTick(obj);
			return true;
		}
		if (!m_mDCORoleSince)
			m_mDCORoleSince = new map<CMD_AICommanderObjectiveComponent, float>();
		float now = DCO_Util.NowS();
		float since;
		if (!m_mDCORoleSince.Find(obj, since))
		{
			since = now;
			m_mDCORoleSince.Set(obj, now);
		}
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		AICommander_BaseComponent relief;
		int reliefGroups = 0;
		bool anyRelief = false;
		if (mgr)
		{
			vector pos = obj.GetOwner().GetOrigin();
			float r = obj.GetRadius() + 50.0;
			foreach (AICommander_BaseComponent c : mgr.m_aCommander)
			{
				if (!c || c == this || c.GetCommanderFactionKey() != m_sFactionKey)
					continue;
				if (!obj.DCO_CanProcess(c.GetCommanderUID()) || obj.DCO_RoleOf(c.GetCommanderUID()) == 1)
					continue;
				anyRelief = true;
				int n = 0;
				foreach (DCO_GroupUtilityComponent g : c.GetOwnedGroups())
				{
					if (g && g.GetOwner() && g.GetUnitCount() > 0 && g.GetGroupObjective() == obj && vector.DistanceXZ(g.GetOwner().GetOrigin(), pos) <= r)
						n++;
				}
				if (n > reliefGroups)
				{
					reliefGroups = n;
					relief = c;
				}
			}
		}
		if (!relief && anyRelief && mgr && now - since >= DCO_Ctx.TuneForCmd(this, "obj.role_handover_s", 300.0))
		{
			foreach (AICommander_BaseComponent c2 : mgr.m_aCommander)
			{
				if (!c2 || c2 == this || c2.GetCommanderFactionKey() != m_sFactionKey || c2.GetCommanderMode() == CMD_ECommanderMode.OFFENSIVE)
					continue;
				if (obj.DCO_CanProcess(c2.GetCommanderUID()) && obj.DCO_RoleOf(c2.GetCommanderUID()) != 1)
				{
					relief = c2;
					break;
				}
			}
		}
		if (!relief)
		{
			string why = "no_relief_commander";
			if (anyRelief)
				why = "relief_not_arrived";
			if (DCO_RoleOnce(obj.GetLogName() + "|hold|" + why))
				DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("obj_role_hold cmd=%1 obj=%2 role=attack_only why=%3 wait_s=%4", m_sCommanderUID, obj.GetLogName(), why, Math.Round(now - since)));
			return false;
		}
		m_aDCOHandedOver.Insert(obj);
		int keep = 0;
		string keptName = "-";
		if (reliefGroups == 0)
		{
			DCO_GroupUtilityComponent kept = DCO_RoleReleaseKeepOne(obj);
			if (kept)
			{
				keep = 1;
				keptName = kept.GetOwner().GetName();
				if (!m_mDCORoleKeep)
					m_mDCORoleKeep = new map<CMD_AICommanderObjectiveComponent, DCO_GroupUtilityComponent>();
				if (!m_mDCORoleKeepAt)
					m_mDCORoleKeepAt = new map<CMD_AICommanderObjectiveComponent, float>();
				m_mDCORoleKeep.Set(obj, kept);
				m_mDCORoleKeepAt.Set(obj, now);
			}
			else
				DCO_RoleRelease(obj);
		}
		else
			DCO_RoleRelease(obj);
		DCO_Perf.Count("obj_role:handover");
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("obj_role_handover obj=%1 from=%2 to=%3 relief_groups=%4 wait_s=%5 keep=%6 kept=%7", obj.GetLogName(), m_sCommanderUID, relief.GetCommanderUID(), reliefGroups, Math.Round(now - since), keep, keptName));
		return true;
	}

	protected bool DCO_RoleGroupOn(DCO_GroupUtilityComponent g, CMD_AICommanderObjectiveComponent obj)
	{
		return g && g.GetOwner() && !g.IsPlayerGroup() && g.GetUnitCount() > 0 && g.GetGroupObjective() == obj && g.CanCommanderOverrideRole() && !g.IsDedicatedTransport();
	}

	protected void DCO_RoleFreeGroup(DCO_GroupUtilityComponent g)
	{
		g.CompleteAllWaypoints();
		g.SetGroupObjective(null);
		g.SetTask(DCO_EGroupTask.NONE, "DCO_RoleHandover");
	}

	protected DCO_GroupUtilityComponent DCO_RoleReleaseKeepOne(CMD_AICommanderObjectiveComponent obj)
	{
		DCO_GroupUtilityComponent kept;
		foreach (DCO_GroupUtilityComponent g : m_aOwnedGroup)
		{
			if (DCO_RoleGroupOn(g, obj) && (!kept || g.GetUnitCount() < kept.GetUnitCount()))
				kept = g;
		}
		if (!kept)
			return null;
		foreach (DCO_GroupUtilityComponent g2 : m_aOwnedGroup)
		{
			if (g2 != kept && DCO_RoleGroupOn(g2, obj))
				DCO_RoleFreeGroup(g2);
		}
		return kept;
	}

	protected void DCO_RoleKeepTick(CMD_AICommanderObjectiveComponent obj)
	{
		if (!m_mDCORoleKeep || !m_mDCORoleKeep.Contains(obj))
			return;
		DCO_GroupUtilityComponent kept = m_mDCORoleKeep.Get(obj);
		float at = m_mDCORoleKeepAt.Get(obj);
		float now = DCO_Util.NowS();
		string why;
		if (!DCO_RoleGroupOn(kept, obj))
			why = "gone";
		else if (now - at >= 120.0)
			why = "timeout";
		else
		{
			AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
			vector pos = obj.GetOwner().GetOrigin();
			float r = obj.GetRadius();
			if (mgr)
			{
				foreach (AICommander_BaseComponent c : mgr.m_aCommander)
				{
					if (!c || c == this || c.GetCommanderFactionKey() != m_sFactionKey || !obj.DCO_CanProcess(c.GetCommanderUID()) || obj.DCO_RoleOf(c.GetCommanderUID()) == 1)
						continue;
					foreach (DCO_GroupUtilityComponent rg : c.GetOwnedGroups())
					{
						if (rg && rg.GetOwner() && rg.GetUnitCount() > 0 && vector.DistanceXZ(rg.GetOwner().GetOrigin(), pos) <= r)
						{
							why = "relief_in_r";
							break;
						}
					}
					if (!why.IsEmpty())
						break;
				}
			}
		}
		if (why.IsEmpty())
			return;
		string kname = "-";
		if (kept && kept.GetOwner())
			kname = kept.GetOwner().GetName();
		if (why != "gone")
			DCO_RoleFreeGroup(kept);
		m_mDCORoleKeep.Remove(obj);
		m_mDCORoleKeepAt.Remove(obj);
		DCO_Perf.Count("obj_role:keep_release_" + why);
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("obj_role_keep_release cmd=%1 obj=%2 grp=%3 why=%4 held_s=%5", m_sCommanderUID, obj.GetLogName(), kname, why, Math.Round(now - at)));
	}

	protected void DCO_RoleRelease(CMD_AICommanderObjectiveComponent obj)
	{
	}

	protected void DCO_RoleForget(CMD_AICommanderObjectiveComponent obj)
	{
		if (m_mDCORoleKeep)
			m_mDCORoleKeep.Remove(obj);
		if (m_mDCORoleKeepAt)
			m_mDCORoleKeepAt.Remove(obj);
		if (m_aDCOHandedOver)
			m_aDCOHandedOver.RemoveItem(obj);
		if (m_mDCORoleSince)
			m_mDCORoleSince.Remove(obj);
	}

	bool DCO_AssignSkip(CMD_AICommanderObjectiveComponent obj, string path)
	{
		if (!obj || obj.DCO_CanProcess(m_sCommanderUID))
			return false;
		if (!m_mDCOAssignSkipAt)
			m_mDCOAssignSkipAt = new map<string, float>();
		string key = obj.GetLogName() + "|" + path;
		float now = DCO_Util.NowS();
		float at;
		if (!m_mDCOAssignSkipAt.Find(key, at) || now >= at)
		{
			m_mDCOAssignSkipAt.Set(key, now + 60.0);
			DCO_Perf.Count("obj_assign:skip_" + path);
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("obj_assign_skip cmd=%1 obj=%2 path=%3", m_sCommanderUID, obj.GetLogName(), path));
		}
		return true;
	}
	float GetReservePercent()				{ return m_fReservePercent; }
	void SetReservePercent(float value)		{ m_fReservePercent = Math.Clamp(value, 0, 50); }
	bool IsAllIn()							{ return m_bAllIn; }
	void SetAllIn(bool value)				{ m_bAllIn = value; }
	bool IsAllInRearDefenders()				{ return m_bAllInRearDefenders; }
	void SetAllInRearDefenders(bool value)	{ m_bAllInRearDefenders = value; }

	[Attribute("1", UIWidgets.CheckBox, desc: "If true, defend group positions are evaluated (high ground and facing the enemy) instead of purely random.", category: "Commander 08. Defend & Garrison")]
	protected bool m_bEnableKeyDefendSpotEvaluation;

	[Attribute("2", UIWidgets.EditBox, "Maximum defend groups per objective sent to evaluated 'key positions' (the rest stay random or patrol as usual).", category: "Commander 08. Defend & Garrison")]
	protected int m_iMaxKeyDefendPositions;

	[Attribute("2", UIWidgets.ComboBox, "Commander Mode", "", ParamEnumArray.FromEnum(CMD_ECommanderMode), category: "Commander 03. Personality" )]
	protected CMD_ECommanderMode m_eCommanderModeExternal;

	[Attribute("0.5", UIWidgets.Range, "Aggressiveness/Eagerness: how quickly the commander commits to an assault without waiting for recon.\n0 = wait for recon first | 1 = attack at once without recon", params: "0 1 0.01", category: "Commander 03. Personality")]
	protected float m_fAggression;

	[Attribute("0.5", UIWidgets.Range, "Adaptability: how fast the commander switches modes and reacts.\n0 = slow to react | 1 = very responsive", params: "0 1 0.01", category: "Commander 03. Personality")]
	protected float m_fAdaptability;

	[Attribute("0.5", UIWidgets.Range, "Risk Taking: how willing the commander is to commit forces to a FOGGY objective (not covered by RECON intel).\n0 = refuse until intel is clear | 1 = attack even when blind", params: "0 1 0.01", category: "Commander 03. Personality")]
	protected float m_fRiskTaking;

	[Attribute("0.5", UIWidgets.Range, "Resilience: how long the commander keeps groups fighting before retreating.\n0 = retreats easily (high threshold) | 1 = very tough (low threshold, holds to the end)", params: "0 1 0.01", category: "Commander 03. Personality")]
	protected float m_fResilience;

	[Attribute("0.5", UIWidgets.Range, "Patience: how long the commander waits before moving groups away from a stalemated objective.\n0 = gives up and reallocates quickly | 1 = waits a long time", params: "0 1 0.01", category: "Commander 03. Personality")]
	protected float m_fPatience;

	[Attribute("0.5", UIWidgets.Range, "Combat Focus: chase and reinforce against detected enemies, or stay focused on capturing objectives.\n0 = focus on objectives, ignore small threats | 1 = chase enemies anywhere, reinforcement triggers easily", params: "0 1 0.01", category: "Commander 03. Personality")]
	protected float m_fCombatFocus;

	float GetCombatFocus() { return m_fCombatFocus; }

	[Attribute("", UIWidgets.EditBox, "Doctrine override: empty = the doctrine of this commander's faction (DCO Settings, Faction layer). Otherwise Default (Measured), Defensive, Aggressive, Recon-heavy, a faction doctrine, or the name of a JSON preset in $profile:DCO/Doctrine/.", category: "Commander 03. Personality")]
	protected string m_sDoctrinePreset;

	[Attribute("", UIWidgets.EditBox, "Doctrine row overrides on top of the doctrine above, as index:mask pairs separated by spaces (battle rows 0-4 = Strong, Favorable, Equal, Weaker, Unknown; defense 5-9; support 10-14). Example 2:1900 = Equal row of Aggressive plus Direct. Empty = none. GM changes replace it.", category: "Commander 03. Personality")]
	protected string m_sDoctrineOverride;
	protected bool m_bDocAttrApplied;

	[Attribute("Measured", UIWidgets.EditBox, "Personality preset: Measured, Careful, Bold or Scout. Sets every trait below that is -1. Random personality varies around it.", category: "Commander 03. Personality")]
	protected string m_sPersonalityPreset;

	[Attribute("-1", UIWidgets.Slider, "Intel 0-2: recon chance, OP frequency, intel confidence threshold. -1 = from the doctrine preset (or converted from the old traits).", params: "-1 2 0.1", category: "Commander 03. Personality")]
	protected float m_fTraitIntel;

	[Attribute("-1", UIWidgets.Slider, "Strategy 0-2: threat vs objective weight, terrain objectives, flank instead of direct. -1 = from the doctrine preset.", params: "-1 2 0.1", category: "Commander 03. Personality")]
	protected float m_fTraitStrategy;

	[Attribute("-1", UIWidgets.Slider, "Aggression 0-2: attack vs defend share, counterattack, QRF tempo. -1 = from the doctrine preset.", params: "-1 2 0.1", category: "Commander 03. Personality")]
	protected float m_fTraitAggression;

	[Attribute("-1", UIWidgets.Slider, "Logistics 0-2: logistics job thresholds, proactive resupply, trucks for logistics. -1 = from the doctrine preset.", params: "-1 2 0.1", category: "Commander 03. Personality")]
	protected float m_fTraitLogistics;

	[Attribute("-1", UIWidgets.Slider, "Support 0-2: artillery, armour and prep fire thresholds, accepting support requests. -1 = from the doctrine preset.", params: "-1 2 0.1", category: "Commander 03. Personality")]
	protected float m_fTraitSupport;

	[Attribute("-1", UIWidgets.Slider, "Leadership 0-2: morale recovery. -1 = from the doctrine preset.", params: "-1 2 0.1", category: "Commander 03. Personality")]
	protected float m_fTraitLeadership;

	[Attribute("-1", UIWidgets.EditBox, "Courage: -1 from the doctrine preset, 0 Hesitant, 1 Careful, 2 Calculated, 3 Bold, 4 Reckless.", category: "Commander 03. Personality")]
	protected int m_iCourage;

	[Attribute("-1", UIWidgets.EditBox, "Decision: -1 from the doctrine preset, 0 Fast, 1 Moderate, 2 Slow (think interval, waiting time, staging margin).", category: "Commander 03. Personality")]
	protected int m_iDecision;

	[Attribute("1", UIWidgets.ComboBox, "Pre-battle intel: how accurate the first picture of the enemy is. Not part of the doctrine preset.", enums: ParamEnumArray.FromEnum(DCO_EPreBattleIntel), category: "Commander 03. Personality")]
	protected DCO_EPreBattleIntel m_ePreBattleIntel;

	protected ref DCO_Doctrine m_Doctrine;
	protected ref map<int, int> m_mDocOverride = new map<int, int>();
	protected int m_iDocVer = -1;
	protected string m_sDocSig;
	protected string m_sTraitSource = "prefab";
	protected ref DCO_DoctrineRuntime m_DoctrineRt = new DCO_DoctrineRuntime();

	DCO_DoctrineRuntime GetDoctrineRuntime()	{ return m_DoctrineRt; }

	protected ref DCO_ReinfBoard m_DCOReinf;

	string GetDoctrinePreset()					{ return m_sDoctrinePreset; }
	string GetPersonalityPreset()				{ return m_sPersonalityPreset; }

	[Attribute("300", UIWidgets.EditBox, "Operation: seconds without progress on the phase MAIN before the commander shifts the main effort.", category: "Operation")]
	protected float m_fDCOOpStallS;
	[Attribute("50", UIWidgets.EditBox, "Operation: percent change in an objective's defender estimate that can trigger a re-plan (must hold 60 s and change the ratio band).", category: "Operation")]
	protected float m_fDCOOpIntelPct;
	[Attribute("25", UIWidgets.EditBox, "Operation: percent losses that turn a supporting capture into a fix, or commit the reserve / shift the main effort.", category: "Operation")]
	protected float m_fDCOOpLossPct;
	protected ref DCO_OpPlanner m_OpPlanner = new DCO_OpPlanner();

	int DCO_OpStallS()							{ return Math.Max(m_iOpStallMin_s, Math.Round(m_fDCOOpStallS)); }
	void DCO_SetOpStallS(float v)				{ m_fDCOOpStallS = v; }
	float DCO_OpIntelPct()						{ return Math.Max(m_fOpIntelPctMin, m_fDCOOpIntelPct); }
	void DCO_SetOpIntelPct(float v)				{ m_fDCOOpIntelPct = v; }
	float DCO_OpLossPct()						{ return Math.Max(m_fOpLossPctMin, m_fDCOOpLossPct); }
	void DCO_SetOpLossPct(float v)				{ m_fDCOOpLossPct = v; }
	DCO_OpPlanner DCO_GetOpPlanner()			{ return m_OpPlanner; }

	protected bool m_bDCODebugDraw;
	protected int m_iDCODebugLayers = 127;
	bool DCO_DebugDraw()						{ return m_bDCODebugDraw && DCO_TestCaseComponent.RunOn(); }
	void DCO_SetDebugDraw(bool on)				{ m_bDCODebugDraw = on; }
	int DCO_DebugLayers()						{ return m_iDCODebugLayers; }
	void DCO_SetDebugLayers(int layers)			{ m_iDCODebugLayers = layers; }
	float DCO_ThinkInterval()					{ return m_fThinkInterval; }
	bool GetRandomPersonality()					{ return m_bRandomPersonality; }

	float GetTraitIntel()						{ return m_fTraitIntel; }
	float GetTraitStrategy()					{ return m_fTraitStrategy; }
	float GetTraitAggression()					{ return m_fTraitAggression; }
	float GetTraitLogistics()					{ return m_fTraitLogistics; }
	float GetTraitSupport()						{ return m_fTraitSupport; }
	float GetTraitLeadership()					{ return m_fTraitLeadership; }
	int GetCourage()							{ return m_iCourage; }
	int GetDecision()							{ return m_iDecision; }
	DCO_EPreBattleIntel GetPreBattleIntel()		{ return m_ePreBattleIntel; }

	[Attribute("8191", UIWidgets.Flags, "Tactics the commander may choose. If none fits: Fire and Maneuver (if allowed), otherwise wait.", enums: ParamEnumArray.FromEnum(DCO_ETacticFlag), category: "Commander 05. Tactics & ROE")]
	protected int m_iAllowedTactics;

	int GetAllowedTactics()					{ return m_iAllowedTactics; }
	void SetAllowedTactics(int mask)		{ m_iAllowedTactics = mask & 8191; }

	[Attribute("255", UIWidgets.Flags, "Defense tactics this commander may use to hold an objective (Area, Depth, Reverse slope, Perimeter, Delay) and the defense modules (Spoiling attack, Counterattack, Engagement area).", enums: ParamEnumArray.FromEnum(DCO_EDefTacticFlag), category: "Commander 05. Tactics & ROE")]
	protected int m_iAllowedDefenseTactics;
	protected ref DCO_DefenseTactics m_DefTactics = new DCO_DefenseTactics();

	int GetAllowedDefenseTactics()			{ return m_iAllowedDefenseTactics; }
	void SetAllowedDefenseTactics(int mask)	{ m_iAllowedDefenseTactics = mask & 255; }
	DCO_DefenseTactics GetDefenseTactics()	{ return m_DefTactics; }

	protected ref DCO_CommanderTactics m_Tactics = new DCO_CommanderTactics();
	protected ref DCO_CommanderFrontline m_Frontline = new DCO_CommanderFrontline();
	protected ref DCO_CommanderPatrol m_Patrol = new DCO_CommanderPatrol();
	protected bool m_bTacticAborting;
	DCO_CommanderTactics GetTactics()		{ return m_Tactics; }
	DCO_CommanderFrontline GetFrontline()	{ return m_Frontline; }
	DCO_CommanderPatrol GetPatrol()			{ return m_Patrol; }

	protected ref map<DCO_GroupUtilityComponent, float> m_mDCOCtrlLogAt = new map<DCO_GroupUtilityComponent, float>();

	[Attribute("", UIWidgets.Object, "Base ROE table per task (empty = default). Personality modifies this table; faction doctrine will fill it later.", category: "Commander 05. Tactics & ROE")]
	protected ref array<ref DCO_ROEEntry> m_aROEBase;

	protected ref DCO_ROETable m_ROE;

	protected int m_iDCOROETuneVer = -1;

	DCO_ROETable GetROETable()
	{
		int tv = DCO_GlobalAIComponent.DCO_TuneVerAll();
		if (m_ROE && tv != m_iDCOROETuneVer)
		{
			m_ROE = null;
			DCO_Perf.Count("cmd:roe_rebuild_tune");
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("cmd_roe_rebuild cmd=%1 reason=tune_ver ver=%2 eff_aggr=%3 a=%4 profile=%5", m_sCommanderUID, tv, DCO_EffAggression().ToString(-1, 2), DCO_AggrA().ToString(-1, 2), DCO_AggProfile()));
		}
		m_iDCOROETuneVer = tv;
		if (!m_ROE)
		{
			m_ROE = BuildBaseROETable();
			ApplyPersonalityToROE(m_ROE);
		}
		return m_ROE;
	}

	protected DCO_ROETable BuildBaseROETable()
	{
		DCO_ROETable t = new DCO_ROETable();
		t.Set(DCO_ROEEntry.Create(DCO_EGroupTask.NONE,				DCO_EROEMode.HOLD,	50, 50));
		t.Set(DCO_ROEEntry.Create(DCO_EGroupTask.PATROL,			DCO_EROEMode.TIGHT,	200, 200));
		t.Set(DCO_ROEEntry.Create(DCO_EGroupTask.TRANSPORT,			DCO_EROEMode.HOLD,	50, 50));
		t.Set(DCO_ROEEntry.Create(DCO_EGroupTask.RECON,				DCO_EROEMode.HOLD,	50, 50));
		t.Set(DCO_ROEEntry.Create(DCO_EGroupTask.ATTACK,			DCO_EROEMode.TIGHT,	300, 300));
		t.Set(DCO_ROEEntry.Create(DCO_EGroupTask.REINFORCE,			DCO_EROEMode.TIGHT,	300, 300));
		t.Set(DCO_ROEEntry.Create(DCO_EGroupTask.FLANK,				DCO_EROEMode.TIGHT,	300, 300));
		t.Set(DCO_ROEEntry.Create(DCO_EGroupTask.SUPPORT_BY_FIRE,	DCO_EROEMode.TIGHT,	600, 600));
		t.Set(DCO_ROEEntry.Create(DCO_EGroupTask.DEFEND,			DCO_EROEMode.TIGHT,	m_fDefendEngageDist, m_fDefendEngageDist));
		t.Set(DCO_ROEEntry.Create(DCO_EGroupTask.GARRISON,			DCO_EROEMode.TIGHT,	m_fDefendEngageDist, m_fDefendEngageDist));
		t.Set(DCO_ROEEntry.Create(DCO_EGroupTask.FIRE_MISSION,		DCO_EROEMode.TIGHT,	50, 50));

		if (m_aROEBase)
		{
			foreach (DCO_ROEEntry e : m_aROEBase)
			{
				if (e)
					t.Set(e);
			}
		}
		return t;
	}

	protected void ApplyPersonalityToROE(DCO_ROETable t)
	{
		t.m_fDistScale = Math.Lerp(m_fRoeDistScaleCautious, m_fRoeDistScaleAggressive, DCO_EffAggression());
		t.m_bEngageDetected = m_fRiskTaking >= 0.5;
		t.m_fHoldEngageDist = 0;
		if (m_fCombatFocus > 0.5)
			t.m_fHoldEngageDist = Math.Lerp(0, m_fHoldEngageDistMax, (m_fCombatFocus - 0.5) * 2);
		t.m_bFireWhenSuppressed = m_fResilience >= m_fFireWhenSuppressedResilience;
	}

	[Attribute("30.0", UIWidgets.EditBox, "Interval (s) at which standing recon reveals nearby enemies to the threat response.", category: "Commander 11. Frontline & Recon")]
	protected float m_fReconRevealInterval;

	[Attribute("600.0", UIWidgets.EditBox, "Radius (m) to search for OTHER captured objectives that can be linked into one patrol route.", category: "Commander 10. Patrol")]
	protected float m_fPatrolLinkRadius;

	[Attribute("0.4", UIWidgets.Range, "Chance a defend group gets an Objective-Link Patrol (roaming between objectives) instead of a Perimeter Patrol (circling one objective).", params: "0 1 0.01", category: "Commander 10. Patrol")]
	protected float m_fLinkPatrolChance;

    [Attribute("false", UIWidgets.CheckBox, desc: "Random Personality Every Playthough?", category: "Commander 03. Personality")]
    bool m_bRandomPersonality;

	[Attribute("3", UIWidgets.EditBox, "MINIMUM number of sectors per objective. Lower clamp; applies at small radii (< about 29 m with a 60 m arc).", category: "Commander 08. Defend & Garrison")]
	protected int m_iMinSector;

	[Attribute("8", UIWidgets.EditBox, "MAXIMUM number of sectors per objective. This is effectively the only cap on garrison manpower; raise with care.", category: "Commander 08. Defend & Garrison")]
	protected int m_iMaxSector;

	[Attribute("60.0", UIWidgets.EditBox, "Arc length (m) covered by one sector. Smaller = more sectors. 60 puts the default radius (30 m) at 3 sectors.", category: "Commander 08. Defend & Garrison")]
	protected float m_fArcPerSector;

	[Attribute("1", UIWidgets.CheckBox, desc: "Scale the sector count by commander personality?", category: "Commander 08. Defend & Garrison")]
	protected bool m_bScaleSectorByPersonality;

	[Attribute("0.75", UIWidgets.Range, "Sector multiplier at Eagerness = 1 (aggressive: fewer sectors, manpower kept for offense).", params: "0.25 2.0 0.05", category: "Commander 08. Defend & Garrison")]
	protected float m_fSectorPersonalityMin;

	[Attribute("1.25", UIWidgets.Range, "Sector multiplier at Eagerness = 0 (cautious: more sectors, denser garrison).", params: "0.25 2.0 0.05", category: "Commander 08. Defend & Garrison")]
	protected float m_fSectorPersonalityMax;

	[Attribute("15.0", UIWidgets.EditBox, "Cap on defend waypoint completion radius (m). Effective value = min(radius x 0.25, this).", category: "Commander 08. Defend & Garrison")]
	protected float m_fMaxCompletionRadius;

	[Attribute("1", UIWidgets.CheckBox, desc: "Gate garrison GROWTH on the manpower budget? Replenishment is never gated.", category: "Commander 08. Defend & Garrison")]
	protected bool m_bGateDefendByManpower;

	[Attribute("400.0", UIWidgets.EditBox, "Radius (m) of the contact search used to find an objective's threat direction.", category: "Commander 08. Defend & Garrison")]
	protected float m_fThreatDirectionRadius;

	protected int m_iPhaseBudget = -1;

	[Attribute("0", UIWidgets.CheckBox, desc: "Draw this commander's overlay as 3D shapes. Only visible while Game Master is open. Objectives, managers and groups have their own checkboxes.", category: "Commander 16. Debug")]
	protected bool m_bDebugMode;
	bool IsDebugMode() { return m_bDebugMode; }

	[Attribute("0.5", UIWidgets.EditBox, "Interval (s) between commander overlay redraws.", category: "Commander 16. Debug")]
	protected float m_fDebugRefreshInterval;

	[Attribute("1", UIWidgets.CheckBox, desc: "Allow taking groups that are busy with another task if that task is less important? If false, the commander only pulls idle or reserve groups.", category: "Commander 12. Reserve & Reinforcement")]
	protected bool m_bEnablePreemption;

	[Attribute("1.3", UIWidgets.EditBox, "How many times more valuable a new objective must be than the group's current task before the group may be taken. 1.0 = take whenever higher (groups flip back and forth). Raise if groups look inconsistent.", category: "Commander 12. Reserve & Reinforcement")]
	protected float m_fPreemptionMargin;

	[Attribute("60.0", UIWidgets.EditBox, "Minimum delay (s) before the same group may be taken again. Stops one group bouncing between objectives.", category: "Commander 12. Reserve & Reinforcement")]
	protected float m_fPreemptionCooldown;

	protected ref map<DCO_GroupUtilityComponent, float> m_mLastPreemptTime = new map<DCO_GroupUtilityComponent, float>();

	protected ref array<ref Shape> m_aDebugShapes = new array<ref Shape>();
	protected ref array<ref DebugTextWorldSpace> m_aDebugTexts = new array<ref DebugTextWorldSpace>();
	protected float m_fDebugTimer = 0.0;

	protected float m_fCaptureCheckTimer = 0.0;

	protected float m_fDefensiveTriggerCooldown = 0.0;
	static float DEFENSIVE_COOLDOWN = 180.0;

	protected ref array<CMD_AICommanderObjectiveComponent> m_aObjective = {};

	protected CMD_ECommanderState m_eCommanderState = CMD_ECommanderState.IDLE;
	protected CMD_ECommanderMode m_eCommanderMode = CMD_ECommanderMode.OFFENSIVE;

	protected CMD_ThreatResponseComponent threatComp;
	protected CMD_ArtillerySupport		  artySupport;

	protected ref array<DCO_GroupUtilityComponent> m_aOwnedGroup = {};
	protected ref array<IEntity> m_aVehicle = {};

	protected ref array<DCO_TransportTeamComponent> m_aTransportTeams = {};

	protected IEntity m_MyEnt;

	protected ref map<CMD_AICommanderObjectiveComponent, float> m_mStalemateResponseTime = new map<CMD_AICommanderObjectiveComponent, float>();

	protected ref map<CMD_AICommanderObjectiveComponent, bool>  m_mAssaultReleased    = new map<CMD_AICommanderObjectiveComponent, bool>();
	protected ref map<CMD_AICommanderObjectiveComponent, float> m_mStagingStartTime   = new map<CMD_AICommanderObjectiveComponent, float>();
	protected ref map<CMD_AICommanderObjectiveComponent, float> m_mFirstArrivalTime   = new map<CMD_AICommanderObjectiveComponent, float>();
	protected ref map<CMD_AICommanderObjectiveComponent, float> m_mSyncDeadline       = new map<CMD_AICommanderObjectiveComponent, float>();

	protected ref map<CMD_AICommanderObjectiveComponent, vector> m_mStagingPos = new map<CMD_AICommanderObjectiveComponent, vector>();

	[Attribute("1", UIWidgets.CheckBox, "Player groups of this faction get tasks (map + task list) for objectives the commander is attacking or defending.", category: "Commander 15. Player Tasking")]
	protected bool m_bPlayerTasking;

	[Attribute("1", UIWidgets.CheckBox, "Player groups count fully in the plan (they take objective slots and are waited for in synchronized attacks). OFF = bonus only.", category: "Commander 15. Player Tasking")]
	protected bool m_bPlayerTaskingCounted;

	[Attribute("2", UIWidgets.EditBox, "Maximum player groups counted per objective (the rest are bonus).", category: "Commander 15. Player Tasking")]
	protected int m_iPlayerMaxPerObjective;

	[Attribute("120", UIWidgets.EditBox, "Seconds the player group leader has to answer the offer card (accept or decline). When time runs out the slot goes to AI without penalty.", category: "Commander 15. Player Tasking")]
	protected float m_fPlayerAcceptTime;

	[Attribute("1.5", UIWidgets.EditBox, "Player group ETA multiplier (distance / speed x multiplier + margin).", category: "Commander 15. Player Tasking")]
	protected float m_fPlayerEtaFactor;

	[Attribute("60", UIWidgets.EditBox, "ETA margin (s).", category: "Commander 15. Player Tasking")]
	protected float m_fPlayerEtaMargin;

	[Attribute("8", UIWidgets.Slider, "Maximum travel minutes for a player group (without margin). Beyond this the commander offers transport; if none is available, the role becomes bonus.", params: "2 30 1", category: "Commander 15. Player Tasking")]
	protected float m_fPlayerMaxEtaMin;

	[Attribute("-1", UIWidgets.EditBox, "Reinforcement stock (number of soldiers) for Commander Spawners that use stock. -1 = unlimited. Server JSON commanderStock >= 0 overrides this value.", category: "Commander 12. Reserve & Reinforcement")]
	protected int m_iReinforcementStock;

	[Attribute("-1", UIWidgets.EditBox, "Reinforcement point stock for Commander Spawners that use points. -1 = unlimited. Separate from soldier stock.", category: "Commander 12. Reserve & Reinforcement")]
	protected int m_iPointsStock;

	protected int m_iStockLeft;
	protected bool m_bStockInit;
	protected int m_iStockStart = -2;
	protected int m_iPointsStart = -2;

	[Attribute("6", UIWidgets.Slider, "Maximum groups per attack on one objective. Server JSON maxAttackGroups >= 1 overrides this value.", params: "1 20 1", category: "Commander 04. Attack")]
	protected int m_iMaxAttackGroups;
	protected bool m_bMaxAttackInit;

	int GetMaxAttackGroups()
	{
		if (!m_bMaxAttackInit)
		{
			m_bMaxAttackInit = true;
			DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetForFaction(m_sFactionKey);
			if (cfg && cfg.GetMaxAttackGroupsDefault() >= 1)
				m_iMaxAttackGroups = cfg.GetMaxAttackGroupsDefault();
		}
		return Math.Max(m_iMaxAttackGroups, 1);
	}

	void SetMaxAttackGroups(int value)
	{
		m_bMaxAttackInit = true;
		m_iMaxAttackGroups = Math.ClampInt(value, 1, 20);
	}

	[Attribute("1", UIWidgets.CheckBox, "A player group enters the objective before release -> all groups are released early to support them.", category: "Commander 15. Player Tasking")]
	protected bool m_bPlayerEarlyRelease;

	[Attribute("2", UIWidgets.EditBox, "After this many failed acceptances or missed ETAs in a row, the group becomes bonus only (not waited for).", category: "Commander 15. Player Tasking")]
	protected int m_iPlayerFailLimit;

	[Attribute("10", UIWidgets.EditBox, "Minutes a player group stays bonus only after hitting the failure limit.", category: "Commander 15. Player Tasking")]
	protected float m_fPlayerBonusMinutes;

	protected ref DCO_PlayerTasking m_PlayerTasking = new DCO_PlayerTasking();

	[Attribute("1", UIWidgets.CheckBox, "Automatic MEDEVAC: unconscious casualties (no medic) are carried to the hub and recover as manpower.", category: "Commander 14. Logistics")]
	protected bool m_bLogiMedevac;

	[Attribute("1", UIWidgets.CheckBox, "Automatic EXTRACT: groups that cannot fight or are retreating are picked up and taken to the hub.", category: "Commander 14. Logistics")]
	protected bool m_bLogiExtract;

	[Attribute("1", UIWidgets.CheckBox, "Automatic RESUPPLY using vehicles with a resupply station. Does not run when global Magic Ammo is ON.", category: "Commander 14. Logistics")]
	protected bool m_bLogiResupply;

	[Attribute("1", UIWidgets.CheckBox, "Hot MEDEVAC / EXTRACT runs are escorted by idle armor.", category: "Commander 14. Logistics")]
	protected bool m_bLogiEscort;

	[Attribute("6", UIWidgets.EditBox, "MEDEVAC priority.", category: "Commander 14. Logistics")]
	protected int m_iLogiPrioMedevac;
	[Attribute("5", UIWidgets.EditBox, "EXTRACT priority.", category: "Commander 14. Logistics")]
	protected int m_iLogiPrioExtract;
	[Attribute("4", UIWidgets.EditBox, "REINFORCE priority.", category: "Commander 14. Logistics")]
	protected int m_iLogiPrioReinforce;
	[Attribute("3", UIWidgets.EditBox, "DEPLOY priority (attack, flank, recon, suppress).", category: "Commander 14. Logistics")]
	protected int m_iLogiPrioDeploy;
	[Attribute("2", UIWidgets.EditBox, "REDEPLOY and long patrol priority.", category: "Commander 14. Logistics")]
	protected int m_iLogiPrioRedeploy;
	[Attribute("1", UIWidgets.EditBox, "RESUPPLY priority.", category: "Commander 14. Logistics")]
	protected int m_iLogiPrioResupply;
	[Attribute("5", UIWidgets.EditBox, "Minimum priority for player requests (transport, extract, medevac).", category: "Commander 14. Logistics")]
	protected int m_iLogiPrioPlayer;

	[Attribute("0.3", UIWidgets.Slider, "Transport is only used if its ETA beats walking by at least this much (0.3 = 30%).", params: "0 0.9 0.05", category: "Commander 14. Logistics")]
	protected float m_fLogiEtaMargin;

	[Attribute("150", UIWidgets.EditBox, "Batching: distance (m) between pickup points and destinations for groups or casualties to share one trip.", category: "Commander 14. Logistics")]
	protected float m_fLogiBatchRadius;

	[Attribute("20", UIWidgets.EditBox, "Batching: seconds after a trip starts that other groups may still join.", category: "Commander 14. Logistics")]
	protected float m_fLogiBatchWindow;

	[Attribute("250", UIWidgets.EditBox, "Safe distance (m) of the LZ and route from threats (plus threat position uncertainty).", category: "Commander 14. Logistics")]
	protected float m_fLogiLZSafeDist;

	[Attribute("180", UIWidgets.EditBox, "Seconds casualties are treated at the hub before recovering.", category: "Commander 14. Logistics")]
	protected float m_fLogiTreatTime;

	[Attribute("400", UIWidgets.EditBox, "EXTRACT only if the group is farther than this from the hub (m).", category: "Commander 14. Logistics")]
	protected float m_fLogiExtractMinDist;

	[Attribute("2", UIWidgets.EditBox, "RESUPPLY: spare magazines below this = low (half the group low -> resupply).", category: "Commander 14. Logistics")]
	protected int m_iLogiLowAmmoMags;

	[Attribute("500", UIWidgets.EditBox, "Automatic hub: own objective at least this far from any objective we do not own (m).", category: "Commander 14. Logistics")]
	protected float m_fLogiHubRearDist;

	protected ref DCO_Logistics m_Logistics = new DCO_Logistics();

	[Attribute("2.0", UIWidgets.EditBox, "Minimum attack strength vs estimated enemy ratio for the MOST aggressive commander (Aggression cannot lower it further).", category: "Commander 04. Attack")]
	protected float m_fAttackRatioAggressive;

	[Attribute("3.0", UIWidgets.EditBox, "Minimum attack strength vs estimated enemy ratio for the most cautious commander (Aggression 0).", category: "Commander 04. Attack")]
	protected float m_fAttackRatioCautious;

	[Attribute("300", UIWidgets.EditBox, "When an attack is cancelled for lack of forces at the gather deadline, this objective is skipped for this long (s) and the commander picks another.", category: "Commander 04. Attack")]
	protected float m_fAttackAbortCooldown;

	protected ref map<CMD_AICommanderObjectiveComponent, float> m_mAttackCooldown = new map<CMD_AICommanderObjectiveComponent, float>();

	protected ref DCO_CommanderOps m_Ops = new DCO_CommanderOps();
	DCO_CommanderOps GetOps()						{ return m_Ops; }

	[Attribute("1", UIWidgets.CheckBox, "Ambush on the approach to our own objectives (projected threat direction or screen in front of the foremost objective).", category: "Commander 09. Defense Ops (Ambush, OP, Counter)")]
	protected bool m_bAmbush;

	[Attribute("1", UIWidgets.CheckBox, "MSR interdiction ambush: roads where enemy vehicles are SEEN passing repeatedly (contact reports). Requires a group with AT.", category: "Commander 09. Defense Ops (Ambush, OP, Counter)")]
	protected bool m_bAmbushMSR;

	[Attribute("2", UIWidgets.EditBox, "Maximum simultaneous active ambushes.", category: "Commander 09. Defense Ops (Ambush, OP, Counter)")]
	protected int m_iMaxAmbushes;

	[Attribute("75", UIWidgets.EditBox, "Ambush kill zone radius (m).", category: "Commander 09. Defense Ops (Ambush, OP, Counter)")]
	protected float m_fAmbushKillRadius;

	[Attribute("600", UIWidgets.EditBox, "Ambush with no contact for this long (s) -> cancel and pull back.", category: "Commander 09. Defense Ops (Ambush, OP, Counter)")]
	protected float m_fAmbushMaxWait;

	[Attribute("60", UIWidgets.EditBox, "Ambush breaks contact this long (s) after opening fire.", category: "Commander 09. Defense Ops (Ambush, OP, Counter)")]
	protected float m_fAmbushFireTime;

	[Attribute("1", UIWidgets.CheckBox, "Observation post (OP/LP): a small team stays still on high ground, holds fire and reports contacts (higher report quality).", category: "Commander 09. Defense Ops (Ambush, OP, Counter)")]
	protected bool m_bOP;

	[Attribute("2", UIWidgets.EditBox, "Maximum active OPs.", category: "Commander 09. Defense Ops (Ambush, OP, Counter)")]
	protected int m_iMaxOP;

	[Attribute("150", UIWidgets.EditBox, "OP pulls back if an enemy is closer than this (m).", category: "Commander 09. Defense Ops (Ambush, OP, Counter)")]
	protected float m_fOPWithdrawDist;

	[Attribute("600", UIWidgets.EditBox, "OP is rotated out after this long (s).", category: "Commander 09. Defense Ops (Ambush, OP, Counter)")]
	protected float m_fOPRotateTime;

	[Attribute("1", UIWidgets.CheckBox, "Immediate counterattack when one of our objectives falls, if the nearest reserve has a good enough strength ratio.", category: "Commander 09. Defense Ops (Ambush, OP, Counter)")]
	protected bool m_bCounterattack;

	[Attribute("120", UIWidgets.EditBox, "Counterattack time limit after an objective falls (s), before the enemy finishes consolidating. After this = normal operation planning.", category: "Commander 09. Defense Ops (Ambush, OP, Counter)")]
	protected float m_fCounterattackWindow;

	[Attribute("1", UIWidgets.CheckBox, "Counter-battery: estimate enemy artillery position from impacts (more accurate with each impact), then return fire or send a hunter group.", category: "Commander 09. Defense Ops (Ambush, OP, Counter)")]
	protected bool m_bCounterBattery;

	[Attribute("250", UIWidgets.EditBox, "Error of the enemy artillery position estimate at the first impact (m); shrinks by the square root of the impact count.", category: "Commander 09. Defense Ops (Ambush, OP, Counter)")]
	protected float m_fCBInitialError;

	[Attribute("1", UIWidgets.CheckBox, "Deception: feint attack by a supporting effort and/or smoke on the other side before the main effort is released.", category: "Commander 09. Defense Ops (Ambush, OP, Counter)")]
	protected bool m_bFeint;

	[Attribute("60", UIWidgets.EditBox, "How long (s) the deception runs before the main effort is released.", category: "Commander 09. Defense Ops (Ambush, OP, Counter)")]
	protected float m_fFeintLead;

	protected ref DCO_CommanderDefense m_Defense = new DCO_CommanderDefense();
	DCO_CommanderDefense GetDefense()			{ return m_Defense; }
	bool IsAmbushEnabled()						{ return m_bAmbush; }
	bool IsMSRAmbushEnabled()					{ return m_bAmbushMSR; }
	int GetMaxAmbushes()						{ return m_iMaxAmbushes; }
	float GetAmbushKillRadius()					{ return m_fAmbushKillRadius; }
	float GetAmbushMaxWait()					{ return m_fAmbushMaxWait; }
	float GetAmbushFireTime()					{ return m_fAmbushFireTime; }
	bool IsOPEnabled()							{ return m_bOP; }
	int GetMaxOP()								{ return m_iMaxOP; }
	float GetOPWithdrawDist()					{ return m_fOPWithdrawDist; }
	float GetOPRotateTime()						{ return m_fOPRotateTime; }
	bool IsCounterattackEnabled()				{ return m_bCounterattack; }
	float GetCounterattackWindow()				{ return m_fCounterattackWindow; }
	bool IsCounterBatteryEnabled()				{ return m_bCounterBattery; }
	float GetCBInitialError()					{ return m_fCBInitialError; }
	bool IsFeintEnabled()						{ return m_bFeint; }
	float GetFeintLead()						{ return m_fFeintLead; }
	void SetAmbushEnabled(bool b)				{ m_bAmbush = b; }
	void SetMSRAmbushEnabled(bool b)			{ m_bAmbushMSR = b; }
	void SetOPEnabled(bool b)					{ m_bOP = b; }
	void SetCounterattackEnabled(bool b)		{ m_bCounterattack = b; }
	void SetCounterBatteryEnabled(bool b)		{ m_bCounterBattery = b; }
	void SetFeintEnabled(bool b)				{ m_bFeint = b; }

	protected ref map<CMD_AICommanderObjectiveComponent, float> m_mDCOReleaseStrength = new map<CMD_AICommanderObjectiveComponent, float>();
	protected ref map<CMD_AICommanderObjectiveComponent, float> m_mDCOReinforceAt = new map<CMD_AICommanderObjectiveComponent, float>();

	protected ref map<CMD_AICommanderObjectiveComponent, float> m_mDCOStaleSince = new map<CMD_AICommanderObjectiveComponent, float>();
	protected ref map<CMD_AICommanderObjectiveComponent, float> m_mDCOSkipLogAt = new map<CMD_AICommanderObjectiveComponent, float>();
	protected float m_fDCOLastAttackActive = -1;

	DCO_Logistics GetLogistics()		{ return m_Logistics; }
	bool IsLogiMedevac()				{ return m_bLogiMedevac; }
	bool IsLogiExtract()				{ return m_bLogiExtract; }
	bool IsLogiResupply()				{ return m_bLogiResupply; }
	bool IsLogiEscort()					{ return m_bLogiEscort; }
	void SetLogiMedevac(bool value)		{ m_bLogiMedevac = value; }
	void SetLogiExtract(bool value)		{ m_bLogiExtract = value; }
	void SetLogiResupply(bool value)	{ m_bLogiResupply = value; }
	void SetLogiEscort(bool value)		{ m_bLogiEscort = value; }
	int GetLogiPlayerPriority()			{ return m_iLogiPrioPlayer; }
	float GetLogiEtaMargin()			{ return m_fLogiEtaMargin; }
	float GetLogiBatchRadius()			{ return m_fLogiBatchRadius; }
	float GetLogiBatchWindow()			{ return m_fLogiBatchWindow; }
	float GetLogiLZSafeDist()			{ return m_fLogiLZSafeDist; }
	float GetLogiTreatTime()			{ return m_fLogiTreatTime; }
	float GetLogiExtractMinDist()		{ return m_fLogiExtractMinDist; }
	int GetLogiLowAmmoMags()			{ return m_iLogiLowAmmoMags; }
	array<DCO_TransportTeamComponent> GetTransportTeams()	{ return m_aTransportTeams; }

	[Attribute("1.5", UIWidgets.EditBox, "Group morale (average, 0 fresh to 3.7 broken) above this = no attack or flank tasks. Resilience raises the tolerance (+/-0.6).", category: "Commander 03. Personality")]
	protected float m_fMoraleAttackThreshold;

	array<DCO_GroupUtilityComponent> GetOwnedGroups()	{ return m_aOwnedGroup; }
	bool IsPlayerTaskingEnabled()		{ return m_bPlayerTasking; }
	bool IsPlayerTaskingCounted()		{ return m_bPlayerTaskingCounted; }
	int GetPlayerMaxPerObjective()		{ return m_iPlayerMaxPerObjective; }
	float GetPlayerAcceptTime()			{ return m_fPlayerAcceptTime; }
	void SetPlayerAcceptTime(float value)	{ m_fPlayerAcceptTime = Math.Clamp(value, 30, 300); }
	float GetPlayerEtaFactor()			{ return m_fPlayerEtaFactor; }
	float GetPlayerEtaMargin()			{ return m_fPlayerEtaMargin; }
	float GetPlayerMaxEtaMin()			{ return m_fPlayerMaxEtaMin; }

	void SetPlayerMaxEtaMin(float value)	{ m_fPlayerMaxEtaMin = Math.Clamp(value, 2, 30); }
	bool GetPlayerEarlyRelease()		{ return m_bPlayerEarlyRelease; }
	int GetPlayerFailLimit()			{ return m_iPlayerFailLimit; }
	float GetPlayerBonusMinutes()		{ return m_fPlayerBonusMinutes; }
	DCO_PlayerTasking GetPlayerTasking()	{ return m_PlayerTasking; }

	protected bool m_bDCOReleaseAll;
	protected string m_sDCOAbortExtra;

	protected ref map<CMD_AICommanderObjectiveComponent, float> m_mAssignedTime = new map<CMD_AICommanderObjectiveComponent, float>();

	[Attribute("45.0", UIWidgets.EditBox, "Maximum time (s) to wait for recon confirmation before skipping it and attacking anyway. Recon is optional; this stops the commander waiting forever for recon that never arrives.", category: "Commander 04. Attack")]
	protected float m_fReconWaitTimeout;

	protected ref map<CMD_AICommanderObjectiveComponent, float> m_mLastReconRevealTime = new map<CMD_AICommanderObjectiveComponent, float>();

	protected float m_fThinkTimer = 0 - m_fDelayFirstIteration;

	int GetOwnedGroupCount() { return m_aOwnedGroup.Count(); }
	CMD_ArtillerySupport GetArtySupport() { return artySupport; }

	int GetOwnedVehicle()	{return m_aVehicle.Count(); }

	protected int m_iManpowerTotalCache = 0;

	protected ref map<DCO_GroupUtilityComponent, float> m_mDCOMedicLogAt = new map<DCO_GroupUtilityComponent, float>();

	bool send = true;

	float GetAggression()                         { return m_fAggression; }
	float DCO_AggrMul()
	{
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetForCommander(this);
		if (!g)
			return 1;
		return g.DCO_Aggression();
	}
	bool DCO_AggProfile()
	{
		return DCO_Ctx.TuneForCmd(this, "agg.cmd_profile", 1) > 0 || DCO_K2();
	}
	bool DCO_K2()
	{
		return DCO_Ctx.TuneForCmd(this, "agg.k2", 1) > 0;
	}
	float DCO_K2Slider()
	{
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetForCommander(this);
		if (!g)
			return 1;
		return Math.Clamp(g.DCO_AggressionSlider(), 0.5, 10);
	}
	float DCO_K2Odds(float base01)
	{
		float cap = DCO_Ctx.TuneForCmd(this, "agg.k2_p0max", 0.9);
		float p0 = Math.Clamp(base01, 0.0, cap);
		float s = DCO_K2Slider();
		return p0 * s / (p0 * s + (1.0 - p0));
	}
	float DCO_K2Tempo()
	{
		return Math.Clamp(Math.Pow(DCO_K2Slider(), 0.5), DCO_Ctx.TuneForCmd(this, "agg.k2_tempo_min", 0.7), DCO_Ctx.TuneForCmd(this, "agg.k2_tempo_max", 2.0));
	}
	protected void DCO_K2Log()
	{
		float base01 = Math.Clamp(m_fAggression * 2.5, 0, 1);
		float p = DCO_K2Odds(base01);
		string key = DCO_K2Slider().ToString(-1, 2) + "|" + base01.ToString(-1, 2) + "|" + DCO_Ctx.TuneForCmd(this, "agg.k2_p0max", 0.9).ToString(-1, 2);
		if (key == m_sDCOK2Log)
			return;
		m_sDCOK2Log = key;
		int capped = 0;
		if (base01 > DCO_Ctx.TuneForCmd(this, "agg.k2_p0max", 0.9))
			capped = 1;
		DCO_Perf.Count("agg:k2_eval");
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("cmd_aggr k2=1 cmd=%1 slider=%2 pers=%3 p0=%4 cap=%5 P=%6 trait=%7 S=%8", m_sCommanderUID, DCO_K2Slider().ToString(-1, 2), m_fAggression.ToString(-1, 2), base01.ToString(-1, 2), capped, p.ToString(-1, 2), DCO_EffTraitAggression().ToString(-1, 2), DCO_K2Tempo().ToString(-1, 2)) + " plan=" + DCO_TraitAggressionPlan().ToString(-1, 2));
	}
	float DCO_AggrA()
	{
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetForCommander(this);
		if (!g)
			return 1;
		return Math.Clamp(g.DCO_AggressionSlider(), 0.5, 4);
	}
	float DCO_EffAggression()
	{
		if (DCO_K2())
		{
			DCO_K2Log();
			return DCO_K2Odds(Math.Clamp(m_fAggression * 2.5, 0, 1));
		}
		if (!DCO_AggProfile())
			return Math.Clamp(m_fAggression * DCO_AggrMul(), 0, 1);
		return Math.Clamp(Math.Clamp(m_fAggression * 2.5, 0, 1) * Math.Pow(DCO_AggrA(), 0.5), 0, 1.5);
	}
	float DCO_EffTraitAggression()
	{
		if (DCO_K2())
			return 2.0 * DCO_K2Odds(Math.Clamp(m_fTraitAggression * 2.5, 0, 2) * 0.5);
		if (!DCO_AggProfile())
			return Math.Clamp(m_fTraitAggression * DCO_AggrMul(), 0, 2);
		return Math.Clamp(Math.Clamp(m_fTraitAggression * 2.5, 0, 2) * Math.Pow(DCO_AggrA(), 0.5), 0, 3);
	}
	float DCO_TraitAggressionPlan()
	{
		if (!DCO_K2())
			return DCO_EffTraitAggression();
		float cap = DCO_Ctx.TuneForCmd(this, "agg.k2_p0max", 0.9);
		return 2.0 * Math.Clamp(Math.Clamp(m_fTraitAggression * 2.5, 0, 2) * 0.5, 0.0, cap);
	}
	float GetAdaptability()                        { return m_fAdaptability; }
	float GetRiskTaking()                          { return m_fRiskTaking; }
	float GetResilience()                          { return m_fResilience; }
	float GetPatience()                            { return m_fPatience; }

	bool  GetVehiclePatrol()                    { return m_bVehiclePatrol; }
	void  SetVehiclePatrol(bool value)          { m_bVehiclePatrol = value; }
	float GetVehiclePatrolRadiusMul()           { return m_fVehiclePatrolRadiusMul; }
	void  SetVehiclePatrolRadiusMul(float value){ m_fVehiclePatrolRadiusMul = Math.Clamp(value, 1, 5); }
	int  GetMaxFrontlineRecon()          { return m_iMaxFrontlineRecon; }
	void SetMaxFrontlineRecon(int value) { m_iMaxFrontlineRecon = Math.Max(value, 0); }

	CMD_ThreatResponseComponent GetThreatResponseComponent()
	{
		return threatComp;
	}

	FactionKey GetCommanderFactionKey()
	{
		return m_sFactionKey;
	}

	array<CMD_AICommanderObjectiveComponent> GetObjectiveList()
	{
		return m_aObjective;
	}

	array<CMD_AICommanderObjectiveComponent> DCO_AllObjectives()
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
			return m_aObjective;
		return mgr.m_aObjective;
	}

	string GetCommanderUID()
	{
		return m_sCommanderUID;
	}

	CMD_ECommanderMode GetCommanderMode()      { return m_eCommanderMode; }

	protected ref map<DCO_GroupUtilityComponent, float> m_mDCONoWpSince = new map<DCO_GroupUtilityComponent, float>();

	[Attribute("100.0", UIWidgets.EditBox, "Minimum distance (m) at which 2 friendly groups count as clustered.", category: "Commander 11. Frontline & Recon")]
	protected float m_fMinGroupClusterDistance;

	static bool TaskCountsSlot(DCO_EGroupTask task)
	{
		return task == DCO_EGroupTask.ATTACK || task == DCO_EGroupTask.FLANK;
	}

	protected ref map<DCO_GroupUtilityComponent, float> m_mDCOReconSkipAt = new map<DCO_GroupUtilityComponent, float>();

	bool IsObjectiveContested(CMD_AICommanderObjectiveComponent obj)
	{
		int friendlyNear;
		int enemyNear;
		obj.CountNearbyUnitsCached(obj.GetRadius(), m_sFactionKey, friendlyNear, enemyNear);
		return enemyNear > 0 && enemyNear >= friendlyNear;
	}

	protected ResourceName m_sDCOGarrisonWpOverride;

	protected ref array<DCO_GroupUtilityComponent> m_aDCOVehOw = {};
	protected ref array<CMD_AICommanderObjectiveComponent> m_aDCOVehOwObj = {};
	protected ref map<DCO_GroupUtilityComponent, float> m_mDCOVehOwAt = new map<DCO_GroupUtilityComponent, float>();

	protected ref map<CMD_AICommanderObjectiveComponent, ref array<DCO_GroupUtilityComponent>> m_mDCOScopeFix = new map<CMD_AICommanderObjectiveComponent, ref array<DCO_GroupUtilityComponent>>();
	protected ref map<CMD_AICommanderObjectiveComponent, float> m_mDCOScopeFixFireAt = new map<CMD_AICommanderObjectiveComponent, float>();
	protected ref map<DCO_GroupUtilityComponent, vector> m_mDCOScopeSite = new map<DCO_GroupUtilityComponent, vector>();
	protected ref map<DCO_GroupUtilityComponent, float> m_mDCOScopeBest = new map<DCO_GroupUtilityComponent, float>();
	protected ref map<DCO_GroupUtilityComponent, float> m_mDCOScopeBestAt = new map<DCO_GroupUtilityComponent, float>();
	protected ref map<DCO_GroupUtilityComponent, float> m_mDCOScopeGoAt = new map<DCO_GroupUtilityComponent, float>();

	protected int m_iDCOStageContactFwd;

	protected ref map<DCO_GroupUtilityComponent, CMD_AICommanderObjectiveComponent> m_mDCOStageArrived = new map<DCO_GroupUtilityComponent, CMD_AICommanderObjectiveComponent>();
	protected int m_iDCOStageContactHere;

	protected ref map<CMD_AICommanderObjectiveComponent, float> m_mCaptureBlockLog = new map<CMD_AICommanderObjectiveComponent, float>();

	protected ref map<CMD_AICommanderObjectiveComponent, string> m_mCapturePlan = new map<CMD_AICommanderObjectiveComponent, string>();
	protected ref map<IEntity, DCO_GroupUtilityComponent> m_mClearHelper = new map<IEntity, DCO_GroupUtilityComponent>();

	protected ref map<string, float> m_mDCOGetInBlock = new map<string, float>();
	protected ref map<string, float> m_mDCOGetInFirst = new map<string, float>();
	protected ref map<string, int> m_mDCOGetInTries = new map<string, int>();
}
