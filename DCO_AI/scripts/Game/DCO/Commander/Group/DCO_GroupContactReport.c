[ComponentEditorProps(category: "GameScripted/Group", description: "Detects enemies and reports contact to Commander")]
class DCO_GroupContactReporterComponentClass : ScriptComponentClass {}

class DCO_GroupContactReporterComponent : ScriptComponent
{
	[Attribute("15.0", UIWidgets.EditBox, "Interval scan enemy (detik). Global Contact Report Scan Interval > 0 menimpa nilai ini.", category: "Contact")]
	protected float m_fScanInterval;

	[Attribute("3", UIWidgets.EditBox, "Maksimal laporan kontak per scan. Sisanya dilaporkan scan berikutnya. Global Contact Report Max Per Scan > 0 menimpa nilai ini.", category: "Contact")]
	protected int m_iMaxReportsPerScan;

	[Attribute("0.5", UIWidgets.Range, "Report quality minimum buat dianggep 'confirmed' pas malem -- di bawah ini, minta ILLUMINATION dulu sebelum HE.", params: "0 1 0.01", category: "Contact")]
	protected float m_fLowConfidenceThreshold;

	[Attribute("8", UIWidgets.EditBox, "Jumlah musuh minimum buat dianggep 'danger tinggi' (bareng jarak deket) -- minta SMOKE, bukan HE.", category: "Contact")]
	protected int m_iHighDangerEnemyCount;

	[Attribute("100.0", UIWidgets.EditBox, "Jarak maksimum (meter) spotter-ke-kontak buat dianggep 'danger tinggi'.", category: "Contact")]
	protected float m_fHighDangerDistance;

	[Attribute("1", UIWidgets.EditBox, "Minimum musuh infantri biasa sebelum dilaporkan. AT / MG / sniper / kendaraan / armor selalu dilaporkan. 1-2 infantri biasa = prioritas rendah.", category: "Contact")]
	protected int m_iMinEnemyToReport;

	[Attribute("3", UIWidgets.EditBox, "Minimum Unit Count for reinforcement request", category: "Contact")]
	protected int m_iReinforcementThreshold;

	[Attribute("4", UIWidgets.EditBox, "Minimum enemy visible untuk request artillery support", category: "Artillery")]
	protected int m_iArtilleryEnemyThreshold;

	[Attribute("120.0", UIWidgets.EditBox, "Cooldown antara artillery requests dari group ini (detik)", category: "Artillery")]
	protected float m_fArtilleryRequestCooldown;

	[Attribute("90.0", UIWidgets.EditBox, "Detik sebelum reinforcement request flag di-reset paksa walau grup belum IDLE (nyegah stuck permanen)", category: "Contact")]
	protected float m_fReinfRequestTimeout;

	[Attribute("0.35", UIWidgets.Range, "Chance artillery request DITOLAK kalau role grup bukan RECON/FLANK (0 = gak pernah ditolak, 1 = selalu ditolak)", params: "0 1 0.01", category: "Artillery")]
	protected float m_fNonFavoredRoleArtyRejectChance;

	[Attribute("60.0", UIWidgets.EditBox, "Target yang terakhir terlihat lebih lama dari ini (detik) gak dilaporkan. Target LOST/DESTROYED/DISARMED juga gak.", category: "Contact Realism")]
	protected float m_fContactMaxAge;

	[Attribute("75.0", UIWidgets.EditBox, "Radius (meter) pengelompokan target jadi satu kelompok kontak = satu laporan.", category: "Contact Realism")]
	protected float m_fContactClusterRadius;

	[Attribute("50.0", UIWidgets.EditBox, "Kelompok kontak dianggap info baru kalau posisinya geser lebih dari ini (meter).", category: "Contact Realism")]
	protected float m_fReportMinShift;

	[Attribute("30.0", UIWidgets.EditBox, "Jeda minimum (detik) laporan ulang yang cuma karena info lebih segar (posisi & jumlah sama).", category: "Contact Realism")]
	protected float m_fMinResendInterval;

	[Attribute("8.0", UIWidgets.EditBox, "Noise posisi laporan (meter) per 100 m jarak pelapor ke kontak.", category: "Contact Realism")]
	protected float m_fNoisePer100m;

	[Attribute("2.0", UIWidgets.EditBox, "Pengali noise waktu malam.", category: "Contact Realism")]
	protected float m_fNightNoiseMul;

	[Attribute("1.8", UIWidgets.EditBox, "Pengali noise kalau kontak cuma DETECTED (belum IDENTIFIED).", category: "Contact Realism")]
	protected float m_fDetectedNoiseMul;

	[Attribute("4", UIWidgets.EditBox, "Jumlah minimum buat dilaporkan sebagai 'regu' (di bawahnya 'beberapa').", category: "Contact Realism")]
	protected int m_iBucketSquadMin;

	[Attribute("10", UIWidgets.EditBox, "Jumlah minimum buat dilaporkan sebagai 'peleton'.", category: "Contact Realism")]
	protected int m_iBucketPlatoonMin;

	[Attribute("400.0", UIWidgets.EditBox, "Armor cuma dilaporkan sebagai armor kalau IDENTIFIED dan dalam jarak ini (meter); selain itu kendaraan biasa.", category: "Contact Realism")]
	protected float m_fArmorIdentifyDist;

	[Attribute("5.0", UIWidgets.EditBox, "Jeda radio minimum (detik) sebelum laporan sampai ke commander.", category: "Contact Realism")]
	protected float m_fRadioDelayMin;

	[Attribute("20.0", UIWidgets.EditBox, "Jeda radio maksimum (detik).", category: "Contact Realism")]
	protected float m_fRadioDelayMax;

	[Attribute("0", UIWidgets.CheckBox, "Wajib radio: laporan cuma bisa dikirim kalau ada anggota grup yang bawa radio.", category: "Contact Realism")]
	protected bool m_bRequireRadio;

	[Attribute("1.5", UIWidgets.EditBox, "Pengali jeda radio kalau grup lagi ditekan (suppressed).", category: "Contact Realism")]
	protected float m_fSuppressedDelayMul;

	[Attribute("1.5", UIWidgets.EditBox, "Pengali noise kalau grup lagi ditekan (suppressed).", category: "Contact Realism")]
	protected float m_fSuppressedNoiseMul;

	protected ref array<ref DCO_ContactTrack>   m_aTracks  = {};
	protected ref array<ref DCO_PendingReport>  m_aPending = {};

	protected DCO_GroupUtilityComponent  m_GroupUtil;
	protected IEntity					 m_MyEntity;
	protected SCR_AIGroup                m_Group;
	protected float                      m_fScanTimer          = 0.0;
	protected bool                       m_bReinfRequested     = false;
	protected float                      m_fLastArtilleryReqAt = -999.0;
	protected float                      m_fLastReinfRequestAt = -999.0;

	protected void ProcessPending(float worldTime)
	{
		for (int i = m_aPending.Count() - 1; i >= 0; i--)
		{
			DCO_PendingReport p = m_aPending[i];
			if (worldTime < p.m_fDeliverAt)
				continue;

			m_aPending.Remove(i);

			CMD_ThreatResponseComponent threatComp = m_GroupUtil.GetThreatResponseComponent();
			if (!threatComp || !CanReport())
				continue;

			if (p.m_bReinforcement)
			{
				threatComp.ReceiveReinforcementRequest(m_GroupUtil, worldTime);
				continue;
			}

			threatComp.ReceiveContactReport(p.m_Report, m_GroupUtil);
			if (p.m_bArtillery)
				RequestArtillerySupport(p.m_Report.m_vPosition, p.m_Report.m_iEstimatedEnemyCount, worldTime, p.m_Report.m_fInfoTime, p.m_Report.m_fUncertainty, p.m_bCallForFire);
		}
	}

	protected void QueueRadio(DCO_PendingReport p, float worldTime)
	{
		float delay = Math.RandomFloatInclusive(m_fRadioDelayMin, Math.Max(m_fRadioDelayMin, m_fRadioDelayMax));
		if (IsSuppressed())
			delay = delay * m_fSuppressedDelayMul;

		p.m_fDeliverAt = worldTime + delay;
		m_aPending.Insert(p);
	}

	protected SCR_AIUtilityComponent GetLeaderUtility()
	{
		if (!m_Group || !m_Group.GetLeaderEntity())
			return null;

		SCR_AICombatComponent combat = SCR_AICombatComponent.Cast(m_Group.GetLeaderEntity().FindComponent(SCR_AICombatComponent));
		if (!combat)
			return null;
		return combat.GetUtilityComponent();
	}

	protected bool IsSuppressed()
	{
		SCR_AIUtilityComponent util = GetLeaderUtility();
		return util && util.m_ThreatSystem && util.m_ThreatSystem.GetState() == EAIThreatState.THREATENED;
	}

	protected bool CanReport()
	{
		if (!m_Group)
			return false;

		ChimeraCharacter leader = ChimeraCharacter.Cast(m_Group.GetLeaderEntity());
		if (!leader)
			return false;

		CharacterControllerComponent ctrl = leader.GetCharacterController();
		if (!ctrl || ctrl.GetLifeState() != ECharacterLifeState.ALIVE)
			return false;

		if (!m_bRequireRadio)
			return true;

		array<AIAgent> agents = {};
		m_Group.GetAgents(agents);
		foreach (AIAgent a : agents)
		{
			IEntity member = a.GetControlledEntity();
			if (!member)
				continue;

			SCR_GadgetManagerComponent gadgets = SCR_GadgetManagerComponent.Cast(member.FindComponent(SCR_GadgetManagerComponent));
			if (gadgets && gadgets.GetGadgetByType(EGadgetType.RADIO))
				return true;
		}
		return false;
	}

	protected int BucketCount(int n)
	{
		if (n >= m_iBucketPlatoonMin)
			return BUCKET_PLATOON;
		if (n >= m_iBucketSquadMin)
			return BUCKET_SQUAD;
		return BUCKET_FEW;
	}

	static const int BUCKET_FEW     = 3;
	static const int BUCKET_SQUAD   = 8;
	static const int BUCKET_PLATOON = 25;

	bool CanChangeRole()
	{
		return m_GroupUtil.CanCommanderOverrideRole();
	}

	protected void RequestReinforcement(float worldTime)
	{
		if (m_bReinfRequested)
			return;

		if (!m_GroupUtil)
			return;

		if (!m_GroupUtil.CanCallReinforcement())
			return;

		m_bReinfRequested = true;
		m_fLastReinfRequestAt = worldTime;

		DCO_PendingReport p = new DCO_PendingReport();
		p.m_bReinforcement = true;
		QueueRadio(p, worldTime);
	}

	protected static const float CALL_FOR_FIRE_MIN_DIST = 150.0;

	protected void RequestArtillerySupport(vector contactPos, int enemyCount, float worldTime, float infoTime, float uncertainty, bool callForFire)
	{
	    if (!m_GroupUtil)
	        return;

		if (!m_GroupUtil.CanCallArty())
			return;

	    if (worldTime - m_fLastArtilleryReqAt < m_fArtilleryRequestCooldown)
	        return;

	    DCO_EGroupTask task = m_GroupUtil.GetTask();
	    if (!callForFire && task != DCO_EGroupTask.RECON && task != DCO_EGroupTask.FLANK)
	    {
	        if (Math.RandomFloat01() < m_fNonFavoredRoleArtyRejectChance)
	        {
	            Print(string.Format("[DCO_Reporter] Artillery request DITOLAK -- tugas %1 bukan prioritas buat call-in artillery", typename.EnumToString(DCO_EGroupTask, task)));
	            return;
	        }
	    }

	    CMD_ThreatResponseComponent threatComp = m_GroupUtil.GetThreatResponseComponent();
	    if (!threatComp)
	        return;

	    m_fLastArtilleryReqAt = worldTime;

	    int shellCount = Math.Clamp(Math.Round(enemyCount * 0.5), 2, 8);

	    float spotDistance  = vector.Distance(m_GroupUtil.GetOwner().GetOrigin(), contactPos);
	    float reportQuality = CMD_ThreatResponseComponent.ComputeReportQuality(spotDistance);

	    bool isHighDanger = (enemyCount >= m_iHighDangerEnemyCount && spotDistance < m_fHighDangerDistance);

	    CMD_FireMissionRequest request = new CMD_FireMissionRequest(
	        contactPos,
	        DetermineShellType(worldTime, reportQuality, isHighDanger),
	        worldTime,
	        shellCount,
	        infoTime,
	        reportQuality
	    );
	    request.m_fUncertainty = uncertainty;
	    request.m_sSource = "squad";
	    request.m_Requester = m_GroupUtil;
	    request.m_bDangerClose = callForFire && IsSuppressed();

	    threatComp.ReceiveArtillerySupport(request, m_GroupUtil);

	    Print(string.Format("[DCO_Reporter] %1 requested artillery @ %2 (%3 shell)",
	        GetOwner().GetName(), contactPos.ToString(), shellCount));
	}

	SCR_EAIArtilleryAmmoType DetermineShellType(float worldTime, float reportQuality, bool isHighDanger = false)
	{
	    if (isHighDanger)
	        return SCR_EAIArtilleryAmmoType.SMOKE;

	    ChimeraWorld world = ChimeraWorld.CastFrom(GetOwner().GetWorld());
		if (!world)
			return SCR_EAIArtilleryAmmoType.HIGH_EXPLOSIVE;

		TimeAndWeatherManagerEntity manager = world.GetTimeAndWeatherManager();
		if (!manager)
			return SCR_EAIArtilleryAmmoType.HIGH_EXPLOSIVE;

		float sunriseTime;
		float sunsetTime;

		float currentTime = manager.GetTimeOfTheDay();
		if (!manager.GetSunriseHour(sunriseTime) || !manager.GetSunsetHour(sunsetTime))
			return SCR_EAIArtilleryAmmoType.HIGH_EXPLOSIVE;

		bool isNight = (currentTime < sunriseTime || currentTime > sunsetTime);

		if (isNight && reportQuality < m_fLowConfidenceThreshold)
			return SCR_EAIArtilleryAmmoType.ILLUMINATION;

		return SCR_EAIArtilleryAmmoType.HIGH_EXPLOSIVE;
	}

	protected float GetPersonalityThresholdScale()
	{
		if (!m_Group)
			return 1.0;

		IEntity leaderEntity = m_Group.GetLeaderEntity();
		if (!leaderEntity)
			return 1.0;

		SCR_AICombatComponent leaderCombat = SCR_AICombatComponent.Cast(leaderEntity.FindComponent(SCR_AICombatComponent));
		if (!leaderCombat)
			return 1.0;

		SCR_AIUtilityComponent leaderUtil = leaderCombat.GetUtilityComponent();
		if (!leaderUtil || !leaderUtil.m_DCOConfig)
			return 1.0;

		switch (leaderUtil.m_DCOConfig.GetPersonality())
		{
			case DCO_EAIPersonality.CAUTIOUS:
				return 0.7;
			case DCO_EAIPersonality.AGGRESSIVE:
				return 1.2;
			case DCO_EAIPersonality.RECKLESS:
				return 1.4;
			default:
				return 1.0;
		}

		return 1.0;
	}

	protected void ScanForEnemies(float worldTime)
	{
		int pt = DCO_Perf.Begin();
		DoScanForEnemies(worldTime);
		DCO_Perf.End("grp_contact_scan", pt);
	}

	protected void DoScanForEnemies(float worldTime)
	{
		if (!m_Group)
			return;

		if (!m_GroupUtil.perc || !CanReport() || !m_GroupUtil.CanReportContacts())
			return;

		PerceptionManager pm = GetGame().GetPerceptionManager();
		IEntity leader = m_Group.GetLeaderEntity();
		if (!pm || !leader)
			return;

		array<ref DCO_ContactCluster> clusters = {};
		CollectContactClusters(pm.GetTime(), leader.GetOrigin(), clusters);

		PruneTracks(worldTime);

		int effectiveArtyThreshold = Math.Max(1, Math.Round(m_iArtilleryEnemyThreshold * GetPersonalityThresholdScale()));
		float pmNow = pm.GetTime();

		array<DCO_ContactCluster> candidates = {};
		vector leaderPos = leader.GetOrigin();
		foreach (DCO_ContactCluster c : clusters)
		{
			bool dangerous = c.IsDangerous();
			if (c.m_iCount < m_iMinEnemyToReport && !dangerous)
				continue;

			c.m_vTrue = c.GetCenter();
			c.m_fInfoTime = worldTime - (pmNow - c.m_fNewestTimestamp);
			c.m_iBucket = BucketCount(c.m_iCount);
			c.m_Track = FindTrack(c.m_vTrue);
			c.m_vVelocity = vector.Zero;

			DCO_ContactTrack track = c.m_Track;
			if (track && c.m_fInfoTime > track.m_fInfoTime + 1.0)
			{
				vector vel = (c.m_vTrue - track.m_vTruePos) * (1.0 / (c.m_fInfoTime - track.m_fInfoTime));
				vel[1] = 0;
				if (vel.Length() > MAX_REPORTED_SPEED)
					vel = vel.Normalized() * MAX_REPORTED_SPEED;
				c.m_vVelocity = vel;
			}

			if (track)
				track.m_fLastMatched = worldTime;

			bool isNew = !track
				|| vector.DistanceXZ(c.m_vTrue, track.m_vTruePos) > m_fReportMinShift
				|| c.m_iBucket != track.m_iBucket
				|| (c.m_fInfoTime > track.m_fInfoTime + 0.5 && worldTime - track.m_fSentTime >= m_fMinResendInterval);
			if (!isNew)
				continue;

			c.m_fPrio = ReportPriority(c, vector.DistanceXZ(leaderPos, c.m_vTrue));
			candidates.Insert(c);
		}

		int maxReports = MaxReportsPerScan();
		for (int sent = 0; sent < maxReports && !candidates.IsEmpty(); sent++)
		{
			int bestIdx = 0;
			for (int ci = 1; ci < candidates.Count(); ci++)
			{
				if (candidates[ci].m_fPrio < candidates[bestIdx].m_fPrio)
					bestIdx = ci;
			}
			DCO_ContactCluster pick = candidates[bestIdx];
			candidates.Remove(bestIdx);
			SendClusterReport(pick, leaderPos, effectiveArtyThreshold, worldTime);
		}
	}

	protected float ReportPriority(DCO_ContactCluster c, float dist)
	{
		float rank = 3;
		if (c.m_iArmor > 0 || c.m_iVehicle > 0)
			rank = 0;
		else if (c.m_iAT > 0 || c.m_iMG > 0 || c.m_iSniper > 0)
			rank = 1;
		else if (c.m_bFiringAtUs)
			rank = 2;

		float v = rank * 1000000 + dist;
		if (rank == 3)
			v += (100 - Math.Min(c.m_iCount, 99)) * 10000;
		return v;
	}

	protected int MaxReportsPerScan()
	{
		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetInstance();
		if (cfg && cfg.GetContactReportMaxPerScan() > 0)
			return cfg.GetContactReportMaxPerScan();
		return Math.Max(m_iMaxReportsPerScan, 1);
	}

	protected float ScanInterval()
	{
		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetInstance();
		if (cfg && cfg.GetContactReportScanInterval() > 0)
			return cfg.GetContactReportScanInterval();
		return m_fScanInterval;
	}

	protected void SendClusterReport(DCO_ContactCluster c, vector leaderPos, int effectiveArtyThreshold, float worldTime)
	{
		bool lowPriority = !c.IsDangerous() && c.m_iCount <= 2;
		DCO_ContactTrack track = c.m_Track;
		if (!track)
		{
			track = new DCO_ContactTrack();
			m_aTracks.Insert(track);
		}
		track.m_vTruePos     = c.m_vTrue;
		track.m_iBucket      = c.m_iBucket;
		track.m_fInfoTime    = c.m_fInfoTime;
		track.m_fSentTime    = worldTime;
		track.m_fLastMatched = worldTime;

		vector velocity = c.m_vVelocity;
		CMD_ContactReport report = BuildReport(c, c.m_vTrue, leaderPos, c.m_iBucket, c.m_fInfoTime, worldTime);
		report.m_vVelocity = velocity;
		report.m_bLowPriority = lowPriority;
		report.m_bMoving = velocity.Length() >= MOVING_MIN_SPEED;
		if (report.m_bMoving)
		{
			float hdg = Math.Atan2(velocity[0], velocity[2]) * Math.RAD2DEG;
			if (hdg < 0)
				hdg += 360;
			report.m_fHeading = hdg;
		}

		DCO_PendingReport p = new DCO_PendingReport();
		p.m_Report = report;
		p.m_bCallForFire = !lowPriority && !m_GroupUtil.IsPlayerGroup() && m_GroupUtil.HasState(DCO_EGroupState.IN_CONTACT)
			&& (IsSuppressed() || c.m_bMG || c.m_bInBuilding)
			&& vector.DistanceXZ(leaderPos, c.m_vTrue) > CALL_FOR_FIRE_MIN_DIST;
		p.m_bArtillery = !lowPriority && (c.m_iBucket >= effectiveArtyThreshold || p.m_bCallForFire);
		QueueRadio(p, worldTime);
	}

	protected const float MAX_REPORTED_SPEED = 15.0;
	protected const float MOVING_MIN_SPEED = 0.7;
	protected ref map<IEntity, int> m_mTypeCache = new map<IEntity, int>();

	static const int TYPE_INF = 0;
	static const int TYPE_MG = 1;
	static const int TYPE_AT = 2;
	static const int TYPE_SNIPER = 3;
	static const int TYPE_VEHICLE = 4;
	static const int TYPE_ARMOR = 5;

	protected int ClassifyTarget(IEntity ent, vector reporterPos)
	{
		int cached;
		if (m_mTypeCache.Find(ent, cached))
			return cached;

		int type = TYPE_INF;
		IEntity veh = DCO_VehicleCombat.GetVehicle(ent);
		if (!veh && Vehicle.Cast(ent))
			veh = ent;
		if (veh)
		{
			type = TYPE_VEHICLE;
			if (DCO_VehicleCombat.IsArmored(veh) && vector.Distance(reporterPos, veh.GetOrigin()) <= m_fArmorIdentifyDist)
				type = TYPE_ARMOR;
		}
		else if (DCO_Strength.HasWeaponInHands(ent, EWeaponType.WT_ROCKETLAUNCHER))
			type = TYPE_AT;
		else if (DCO_Strength.HasWeaponInHands(ent, EWeaponType.WT_MACHINEGUN))
			type = TYPE_MG;
		else if (DCO_Strength.HasWeaponInHands(ent, EWeaponType.WT_SNIPERRIFLE))
			type = TYPE_SNIPER;

		m_mTypeCache.Set(ent, type);
		return type;
	}

	protected void CollectContactClusters(float pmNow, vector reporterPos, notnull array<ref DCO_ContactCluster> outClusters)
	{
		float radiusSq = m_fContactClusterRadius * m_fContactClusterRadius;
		m_mTypeCache.Clear();

		foreach (SCR_AITargetInfo t : m_GroupUtil.perc.m_aTargets)
		{
			if (!t)
				continue;

			if (t.m_eCategory != EAITargetInfoCategory.DETECTED && t.m_eCategory != EAITargetInfoCategory.IDENTIFIED)
				continue;

			if (pmNow - t.m_fTimestamp > m_fContactMaxAge)
				continue;

			DCO_ContactCluster cluster = null;
			foreach (DCO_ContactCluster existing : outClusters)
			{
				if (vector.DistanceSqXZ(existing.GetCenter(), t.m_vWorldPos) <= radiusSq)
				{
					cluster = existing;
					break;
				}
			}
			if (!cluster)
			{
				cluster = new DCO_ContactCluster();
				outClusters.Insert(cluster);
			}

			cluster.m_vSum = cluster.m_vSum + t.m_vWorldPos;
			cluster.m_iCount++;
			cluster.m_fNewestTimestamp = Math.Max(cluster.m_fNewestTimestamp, t.m_fTimestamp);
			if (t.m_bEndangering)
				cluster.m_bFiringAtUs = true;

			if (t.m_eCategory != EAITargetInfoCategory.IDENTIFIED || !t.m_Entity)
			{
				cluster.m_iInf++;
				continue;
			}

			cluster.m_bIdentified = true;
			if (SCR_CoverManagerComponent.DCO_GetBuildingAt(t.m_Entity))
				cluster.m_bInBuilding = true;

			switch (ClassifyTarget(t.m_Entity, reporterPos))
			{
				case TYPE_AT:
					cluster.m_iAT++;
					cluster.m_bAT = true;
					break;
				case TYPE_MG:
					cluster.m_iMG++;
					cluster.m_bMG = true;
					break;
				case TYPE_SNIPER:
					cluster.m_iSniper++;
					break;
				case TYPE_VEHICLE:
					cluster.m_iVehicle++;
					break;
				case TYPE_ARMOR:
					cluster.m_iArmor++;
					cluster.m_bArmor = true;
					break;
				default:
					cluster.m_iInf++;
					break;
			}
		}
	}

	protected CMD_ContactReport BuildReport(DCO_ContactCluster c, vector truePos, vector reporterPos, int bucket, float infoTime, float worldTime)
	{
		float sigma = m_fNoisePer100m * vector.Distance(reporterPos, truePos) / 100.0;
		if (CMD_ThreatResponseComponent.IsNight())
			sigma = sigma * m_fNightNoiseMul;
		if (!c.m_bIdentified)
			sigma = sigma * m_fDetectedNoiseMul;
		if (IsSuppressed())
			sigma = sigma * m_fSuppressedNoiseMul;
		sigma = Math.Max(sigma, MIN_UNCERTAINTY);

		float angle = Math.RandomFloat(0, Math.PI2);
		float r     = sigma * Math.Sqrt(Math.RandomFloat01());
		vector noisy = truePos + Vector(Math.Cos(angle) * r, 0, Math.Sin(angle) * r);
		noisy[1] = GetGame().GetWorld().GetSurfaceY(noisy[0], noisy[2]);

		CMD_ContactReport report = new CMD_ContactReport(noisy, bucket, worldTime, GetOwner().GetName());
		report.m_bArmorSeen   = c.m_bArmor;
		report.m_bATSeen      = c.m_bAT;
		report.m_iInf         = c.m_iInf;
		report.m_iMG          = c.m_iMG;
		report.m_iAT          = c.m_iAT;
		report.m_iSniper      = c.m_iSniper;
		report.m_iVehicle     = c.m_iVehicle;
		report.m_iArmor       = c.m_iArmor;
		report.m_bFiringAtUs  = c.m_bFiringAtUs;
		report.m_fUncertainty = sigma;
		report.m_fInfoTime    = infoTime;
		report.m_vTruePos     = truePos;
		return report;
	}

	protected const float MIN_UNCERTAINTY = 5.0;

	protected DCO_ContactTrack FindTrack(vector truePos)
	{
		float matchSq = (m_fContactClusterRadius * 1.5) * (m_fContactClusterRadius * 1.5);
		foreach (DCO_ContactTrack t : m_aTracks)
		{
			if (vector.DistanceSqXZ(t.m_vTruePos, truePos) <= matchSq)
				return t;
		}
		return null;
	}

	protected void PruneTracks(float worldTime)
	{
		for (int i = m_aTracks.Count() - 1; i >= 0; i--)
		{
			if (worldTime - m_aTracks[i].m_fLastMatched > m_fContactMaxAge)
				m_aTracks.Remove(i);
		}
	}

	protected int CountFreshTargets()
	{
		PerceptionManager pm = GetGame().GetPerceptionManager();
		if (!pm || !m_GroupUtil.perc)
			return 0;

		float pmNow = pm.GetTime();
		int n = 0;
		foreach (SCR_AITargetInfo t : m_GroupUtil.perc.m_aTargets)
		{
			if (t && (t.m_eCategory == EAITargetInfoCategory.DETECTED || t.m_eCategory == EAITargetInfoCategory.IDENTIFIED) && pmNow - t.m_fTimestamp <= m_fContactMaxAge)
				n++;
		}
		return n;
	}

	protected void CheckReinforcementNeed(float worldTime)
	{
		if (!m_GroupUtil)
			return;

		if (m_GroupUtil.GetTask() != DCO_EGroupTask.RECON
			&& m_GroupUtil.GetTask() != DCO_EGroupTask.FLANK)
			return;

		if (!CanReport())
			return;

		int effectiveReinfThreshold = Math.Max(1, Math.Round(m_iReinforcementThreshold * GetPersonalityThresholdScale()));
		if (CountFreshTargets() >= effectiveReinfThreshold)
			RequestReinforcement(worldTime);
	}

	protected void CheckResetFlags(float worldTime)
	{
		if (!m_GroupUtil)
			return;

		if (!m_GroupUtil.IsMoving())
		{
			m_bReinfRequested = false;
			return;
		}

		if (m_bReinfRequested && (worldTime - m_fLastReinfRequestAt) > m_fReinfRequestTimeout)
			m_bReinfRequested = false;
	}

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (!Replication.IsServer())
			return;

		if (!m_GroupUtil)
			return;

		float worldTime = GetGame().GetWorld().GetWorldTime() / 1000.0;
		if (!m_aPending.IsEmpty())
			ProcessPending(worldTime);

		m_fScanTimer += timeSlice;
		if (m_fScanTimer <= ScanInterval() || !m_GroupUtil.perc)
			return;

		m_fScanTimer = 0.0;

		ScanForEnemies(worldTime);
		CheckReinforcementNeed(worldTime);
		CheckResetFlags(worldTime);
	}

	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);
	}

	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		if (!AICommander_ManagerComponent.GetInstance())
			return;

		m_MyEntity = owner;
		m_Group     = SCR_AIGroup.Cast(owner);
		m_GroupUtil = DCO_GroupUtilityComponent.Cast(owner.FindComponent(DCO_GroupUtilityComponent));
	}

	void InitializeContactReport()
	{
		if (!m_MyEntity)
		{
			m_MyEntity  = GetOwner();
			m_Group     = SCR_AIGroup.Cast(m_MyEntity);
			m_GroupUtil = DCO_GroupUtilityComponent.Cast(m_MyEntity.FindComponent(DCO_GroupUtilityComponent));
		}

		SetEventMask(m_MyEntity, EntityEvent.FRAME);
	}

	void DeactivateContactReport()
	{
		if (!m_MyEntity)
			return;

		ClearEventMask(m_MyEntity, EntityEvent.FRAME);
		m_fScanTimer = 0.0;
		m_aPending.Clear();
		m_aTracks.Clear();
	}
}

class DCO_ContactCluster
{
	vector m_vSum;
	int    m_iCount;
	bool   m_bIdentified;
	bool   m_bArmor;
	bool   m_bAT;
	bool   m_bMG;
	bool   m_bInBuilding;
	float  m_fNewestTimestamp;
	int    m_iInf;
	int    m_iMG;
	int    m_iAT;
	int    m_iSniper;
	int    m_iVehicle;
	int    m_iArmor;
	bool   m_bFiringAtUs;
	vector m_vTrue;
	float  m_fInfoTime;
	int    m_iBucket;
	vector m_vVelocity;
	float  m_fPrio;
	DCO_ContactTrack m_Track;

	bool IsDangerous()
	{
		return m_iMG > 0 || m_iAT > 0 || m_iSniper > 0 || m_iVehicle > 0 || m_iArmor > 0;
	}

	vector GetCenter()
	{
		return m_vSum * (1.0 / Math.Max(m_iCount, 1));
	}
}

class DCO_ContactTrack
{
	vector m_vTruePos;
	int    m_iBucket;
	float  m_fInfoTime;
	float  m_fSentTime;
	float  m_fLastMatched;
}

class DCO_PendingReport
{
	ref CMD_ContactReport m_Report;
	bool  m_bArtillery;
	bool  m_bCallForFire;
	bool  m_bReinforcement;
	float m_fDeliverAt;
}