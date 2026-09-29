[ComponentEditorProps(category: "GameScripted/Commander", description: "Koordinasi fire mission artillery untuk AI Commander")]
class CMD_ArtillerySupportClass : ScriptComponentClass
{
}

class CMD_ArtillerySupport : ScriptComponent
{
	[Attribute("0.4", UIWidgets.EditBox, "Interval (detik) dispatch tiap shell dari queue", category: "Firing")]
	protected float m_fDispatchInterval;

	[Attribute("90.0", UIWidgets.EditBox, "Detik sebelum request di queue dianggap kadaluarsa", category: "Firing")]
	protected float m_fQueueExpiry;

	[Attribute("25.0", UIWidgets.EditBox, "Radius cek friendly (meter) sebelum fire mission diizinkan", category: "Safety")]
	protected float m_fFriendlySafeRadius;

	[Attribute("0.1", UIWidgets.Range, "Chance request ditolak secara random (0 = tidak pernah, 1 = selalu)", params: "0 1 0.01", category: "Firing")]
	protected float m_fBaseRejectionChance;

	[Attribute("50", UIWidgets.EditBox, "Ketidakpastian posisi (m) sampai segini = HE titik (sheaf rapat)", category: "Mission Tiers")]
	protected float m_fPointUncertainty;

	[Attribute("150", UIWidgets.EditBox, "Ketidakpastian posisi (m) sampai segini = HE area (sheaf selebar ketidakpastian, peluru +50%). Di atasnya gak ada HE: recon (siang) / ILLUMINATION (malam)", category: "Mission Tiers")]
	protected float m_fAreaUncertainty;

	[Attribute("35", UIWidgets.EditBox, "Radius mematikan peluru (m), ditambah ke sebaran nyata buat radius aman kawan", category: "Safety")]
	protected float m_fLethalRadius;

	[Attribute("100", UIWidgets.EditBox, "Danger close: squad peminta yang lagi ditekan boleh sedekat ini (m) dari titik tembak; kawan lain tetap butuh radius aman penuh", category: "Safety")]
	protected float m_fDangerCloseRadius;

	[Attribute("80", UIWidgets.EditBox, "Jarak tembak minimum unit mortir (m)", category: "Range")]
	protected float m_fMinRange;

	[Attribute("3500", UIWidgets.EditBox, "Jarak tembak maksimum unit mortir (m)", category: "Range")]
	protected float m_fMaxRange;

	[Attribute("15.0", UIWidgets.EditBox, "Cooldown base (detik) per shell yang ditembak", category: "Firing")]
	protected float m_fCooldownPerShell;

	[Attribute("30.0", UIWidgets.EditBox, "Minimum cooldown global artillery (detik)", category: "Firing")]
	protected float m_fMinGlobalCooldown;

	[Attribute("300.0", UIWidgets.EditBox, "Maximum cooldown global artillery (detik)", category: "Firing")]
	protected float m_fMaxGlobalCooldown;

	[Attribute("30.0", UIWidgets.EditBox, "Minimum dispersion radius (meter)", category: "Artillery Accuracy")]
	protected float m_fMinDispersion;

	[Attribute("250.0", UIWidgets.EditBox, "Maximum dispersion radius (meter)", category: "Artillery Accuracy")]
	protected float m_fMaxDispersion;

	[Attribute("0.06", UIWidgets.Range, "Dispersion scale per meter jarak unit->target (range * scale = base dispersion)", params: "0.01 0.5 0.01", category: "Artillery Accuracy")]
	protected float m_fDispersionRangeScale;

	[Attribute("0.5", UIWidgets.Range, "Base accuracy sebelum modifier", params: "0.0 1.0 0.01", category: "Artillery Accuracy")]
	protected float m_fBaseAccuracy;

	[Attribute("1.5", UIWidgets.Range, "Global dispersion multiplier — naikan buat lebih spray, turunin buat lebih presisi", params: "0.1 3.0 0.1", category: "Artillery Accuracy")]
	protected float m_fDispersionMultiplier;

	[Attribute("0.1", UIWidgets.Range, "Cluster ratio — 0 = flat random, 1 = sangat clustering di center", params: "0.0 1.0 0.01", category: "Artillery Accuracy")]
	protected float m_fClusterRatio;

	[Attribute("50.0", UIWidgets.EditBox, "Radius (meter) buat nganggep fire mission baru itu 'nembak tempat yang sama' kayak sebelumnya", category: "Artillery Accuracy")]
	protected float m_fCorrectionRadius;

	[Attribute("300.0", UIWidgets.EditBox, "Detik -- kalau gak ada fire mission baru ke area itu dalem waktu ini, tracking koreksi di-reset (dianggap situasi baru)", category: "Artillery Accuracy")]
	protected float m_fCorrectionExpiry;

	[Attribute("3", UIWidgets.EditBox, "Maximum jumlah koreksi berturut-turut sebelum bonus akurasi plateau (gak nambah lagi)", category: "Artillery Accuracy")]
	protected int m_iMaxCorrections;

	[Attribute("0.08", UIWidgets.Range, "Bonus akurasi PER TINGKAT koreksi (tingkat 1 = panggilan pertama, gak dapet bonus)", params: "0.0 0.2 0.01", category: "Artillery Accuracy")]
	protected float m_fCorrectionBonusPerStep;

	protected ref array<ref CMD_ArtilleryCorrectionTrack> m_aCorrectionTracks = new array<ref CMD_ArtilleryCorrectionTrack>();

	[Attribute("5", UIWidgets.EditBox, "Maximum request di queue sebelum kena penalty cooldown", category: "Cooldown")]
	protected int m_iMaxQueueSize;

	[Attribute("10.0", UIWidgets.EditBox, "Penalty cooldown tambahan per request saat queue penuh (detik)", category: "Cooldown")]
	protected float m_fQueueOverloadPenalty;

	[Attribute("60.0", UIWidgets.EditBox, "Cooldown global artillery setelah fire mission selesai (detik)", category: "Cooldown")]
	protected float m_fGlobalCooldown;

	[Attribute("5.0", UIWidgets.EditBox, "Waktu proses per request di queue (detik) sebelum request berikutnya bisa di-process", category: "Cooldown")]
	protected float m_fQueueProcessTime;

	protected float m_fGlobalCooldownUntil  = 0.0;
	protected float m_fNextProcessTime      = 0.0;
	protected int   m_iTotalShellsFired     = 0;

	protected AICommander_BaseComponent               m_Commander;
	protected ref array<DCO_GroupUtilityComponent>        m_aUnits        = new array<DCO_GroupUtilityComponent>();
	protected ref array<ref CMD_FireMissionRequest>   m_aQueue        = new array<ref CMD_FireMissionRequest>();
	protected CMD_ThreatResponseComponent m_ThreatResponseComponent;
	protected float                                   m_fDispatchTimer = 0.0;

	protected bool   m_bFriendlyFound   = false;
	protected string m_sFriendlyFaction = string.Empty;
	protected Faction m_FriendlyFaction;

	float GetGlobalCooldownUntil()     { return m_fGlobalCooldownUntil; }
	int   GetTotalShellsFired()        { return m_iTotalShellsFired; }

	float GetRejectionChance()               { return m_fBaseRejectionChance; }
	void  SetRejectionChance(float v)        { m_fBaseRejectionChance = Math.Clamp(v, 0, 1); }
	float GetGlobalCooldown()                { return m_fGlobalCooldown; }
	void  SetGlobalCooldown(float v)         { m_fGlobalCooldown = Math.Max(v, 0); }
	float GetCooldownPerShell()              { return m_fCooldownPerShell; }
	void  SetCooldownPerShell(float v)       { m_fCooldownPerShell = Math.Max(v, 0); }
	float GetBaseAccuracy()                  { return m_fBaseAccuracy; }
	void  SetBaseAccuracy(float v)           { m_fBaseAccuracy = Math.Clamp(v, 0, 1); }
	float GetDispersionMultiplier()          { return m_fDispersionMultiplier; }
	void  SetDispersionMultiplier(float v)   { m_fDispersionMultiplier = Math.Max(v, 0.1); }
	float GetFriendlySafeRadius()            { return m_fFriendlySafeRadius; }
	void  SetFriendlySafeRadius(float v)     { m_fFriendlySafeRadius = Math.Max(v, 0); }
	float GetPointUncertainty()              { return m_fPointUncertainty; }
	void  SetPointUncertainty(float v)       { m_fPointUncertainty = Math.Clamp(v, 10, m_fAreaUncertainty); }
	float GetAreaUncertainty()               { return m_fAreaUncertainty; }
	void  SetAreaUncertainty(float v)        { m_fAreaUncertainty = Math.Max(v, m_fPointUncertainty); }

	protected ref array<IEntity> m_aFriendlyExclude = {};

	string ResolveTier(float uncertainty)
	{
		if (uncertainty <= m_fPointUncertainty)
			return "point";
		if (uncertainty <= m_fAreaUncertainty)
			return "area";
		return "none";
	}

	void ApplyTier(notnull CMD_FireMissionRequest req, string tier, float uncertainty, string source)
	{
		req.m_sTier = tier;
		req.m_sSource = source;
		req.m_fUncertainty = uncertainty;
		if (tier == "area")
			req.m_fAreaRadius = uncertainty;
		else
			req.m_fAreaRadius = 0;
	}

	static void LogDenied(CMD_FireMissionRequest req, string reason)
	{
		string src = "manual";
		string tier = "point";
		vector pos;
		if (req)
		{
			src = req.m_sSource;
			tier = req.m_sTier;
			pos = req.m_vImpactPos;
		}
		string line = string.Format("arty_denied reason=%1 src=%2 tier=%3 pos=%4,%5", reason, src, tier, Math.Round(pos[0]), Math.Round(pos[2]));
		Print("[CMD_ArtillerySupport] " + line);
		DCO_BenchmarkLoggerComponent.Event(line);
		if (req)
		{
			req.m_sDeny = reason;
			if (req.m_iPlayerGroup >= 0 && req.m_bQueued)
				DCO_Radio.Group(req.m_iPlayerGroup, "FIRE SUPPORT", "fire_dropped", DCO_ERadioKind.WARNING, DCO_Radio.P("reason", DenyKey(reason)));
		}
	}

	int CountUnitsInRange(vector target)
	{
		int n;
		foreach (DCO_GroupUtilityComponent unit : m_aUnits)
		{
			if (!unit || !unit.GetOwner())
				continue;
			float d = vector.DistanceXZ(unit.GetOwner().GetOrigin(), target);
			if (d >= m_fMinRange && d <= m_fMaxRange)
				n++;
		}
		return n;
	}

	bool IsQueueBusy()
	{
		return m_iMaxQueueSize > 0 && m_aQueue.Count() * 2 > m_iMaxQueueSize;
	}

	bool IsRequesterDangerClose(CMD_FireMissionRequest req, SCR_AIGroup grp)
	{
		if (!grp)
			return false;
		float radius = Math.Max(req.m_fSafeRadius, m_fFriendlySafeRadius);
		array<int> pids = grp.GetPlayerIDs();
		if (!pids)
			return false;
		foreach (int pid : pids)
		{
			IEntity ent = GetGame().GetPlayerManager().GetPlayerControlledEntity(pid);
			if (ent && vector.DistanceXZ(ent.GetOrigin(), req.m_vImpactPos) <= radius)
				return true;
		}
		return false;
	}

	DCO_GroupUtilityComponent FindUnitInRange(vector target, bool idleOnly, out string denyReason)
	{
		denyReason = "no_unit";
		DCO_GroupUtilityComponent best;
		float bestDist = float.MAX;
		foreach (DCO_GroupUtilityComponent unit : m_aUnits)
		{
			if (!unit || !unit.GetOwner())
				continue;
			if (idleOnly && unit.IsMoving())
				continue;

			float d = vector.DistanceXZ(unit.GetOwner().GetOrigin(), target);
			if (d < m_fMinRange || d > m_fMaxRange)
			{
				denyReason = "out_of_range";
				continue;
			}

			if (d < bestDist)
			{
				bestDist = d;
				best = unit;
			}
		}
		return best;
	}

	protected vector FirePosFor(vector target)
	{
		string reason;
		DCO_GroupUtilityComponent unit = FindUnitInRange(target, false, reason);
		if (unit)
			return unit.GetOwner().GetOrigin();
		if (m_Commander)
			return m_Commander.GetOwner().GetOrigin();
		return target;
	}

	void AddGlobalCooldown(float worldTime, float penalty)
	{
	    m_fGlobalCooldownUntil = Math.Clamp(
	        Math.Max(m_fGlobalCooldownUntil, worldTime) + penalty,
	        worldTime,
	        worldTime + m_fMaxGlobalCooldown
	    );
	}

	void RegisterArtilleryGroup(DCO_GroupUtilityComponent grp)
	{
		if (!grp)
			return;

		if (m_aUnits.Contains(grp))
			return;

		m_aUnits.Insert(grp);
	}

	void UnregisterArtilleryGroup(DCO_GroupUtilityComponent grp)
	{
		if (!grp)
			return;

		if (m_aUnits.Contains(grp))
		{
			m_aUnits.RemoveItemOrdered(grp);
		}
	}

	int GetRegisteredUnits(notnull out array<DCO_GroupUtilityComponent> outUnits)
	{
		outUnits.Clear();
		foreach (DCO_GroupUtilityComponent u : m_aUnits)
		{
			if (u)
				outUnits.Insert(u);
		}
		return outUnits.Count();
	}

	bool HasRegisteredUnits()
	{
		return !m_aUnits.IsEmpty();
	}

	int LiftFiresNear(vector pos, float radius)
	{
		int n;
		for (int i = m_aQueue.Count() - 1; i >= 0; i--)
		{
			CMD_FireMissionRequest r = m_aQueue[i];
			if (r && IsLethal(r) && vector.DistanceXZ(r.m_vImpactPos, pos) <= radius)
			{
				m_aQueue.Remove(i);
				n++;
			}
		}

		foreach (DCO_GroupUtilityComponent unit : m_aUnits)
		{
			SCR_AIGroup grp;
			if (unit)
				grp = SCR_AIGroup.Cast(unit.GetOwner());
			if (!grp)
				continue;
			array<AIWaypoint> wps = {};
			grp.GetWaypoints(wps);
			foreach (AIWaypoint wp : wps)
			{
				if (wp && SCR_AIWaypointArtillerySupport.Cast(wp) && vector.DistanceXZ(wp.GetOrigin(), pos) <= radius)
				{
					unit.CompleteAllWaypoints();
					n++;
					break;
				}
			}
		}
		return n;
	}

	bool RequestShellImpact(notnull CMD_FireMissionRequest req, float worldTime, int shellCount)
	{
	    if (!Replication.IsServer())
	        return false;

	    string deny;
	    if (!FindUnitInRange(req.m_vImpactPos, false, deny))
	    {
	        LogDenied(req, deny);
	        return false;
	    }

	    if (req.m_iPlayerGroup < 0 && m_fBaseRejectionChance > 0 && Math.RandomFloat01() < m_fBaseRejectionChance)
	    {
	        LogDenied(req, "random");
	        return false;
	    }

	    if (m_aQueue.Count() >= m_iMaxQueueSize)
	    {
	        LogDenied(req, "queue_full");
	        int overflow       = m_aQueue.Count() - m_iMaxQueueSize + 1;
	        float penalty      = m_fQueueOverloadPenalty * overflow;
	        m_fGlobalCooldownUntil = Math.Clamp(
	            Math.Max(m_fGlobalCooldownUntil, worldTime) + penalty,
	            worldTime,
	            worldTime + m_fMaxGlobalCooldown
	        );
	        return false;
	    }

	    if (req.m_sTier == "area" && req.m_iPlayerGroup < 0)
	        shellCount = Math.Ceil(shellCount * 1.5);

	    req.m_iShellCount    = shellCount;
	    req.m_fRequestedTime = worldTime;
	    if (req.m_fSafeRadius <= 0)
	        req.m_fSafeRadius = ComputeSafeRadius(req, worldTime);
	    int at = m_aQueue.Count();
	    if (req.m_iPlayerGroup >= 0)
	    {
	        at = 0;
	        while (at < m_aQueue.Count() && m_aQueue[at] && m_aQueue[at].m_iPlayerGroup >= 0)
	            at++;
	    }
	    m_aQueue.InsertAt(req, at);
	    req.m_bQueued = true;

	    string line = string.Format("arty_request src=%1 tier=%2 unc=%3 shells=%4 type=%5 safe=%6 pos=%7,%8",
	        req.m_sSource, req.m_sTier, Math.Round(req.m_fUncertainty), shellCount,
	        typename.EnumToString(SCR_EAIArtilleryAmmoType, req.m_eShellType), Math.Round(req.m_fSafeRadius),
	        Math.Round(req.m_vImpactPos[0]), Math.Round(req.m_vImpactPos[2]));
	    Print("[CMD_ArtillerySupport] " + line);
	    DCO_BenchmarkLoggerComponent.Event(line);
	    return true;
	}

	static float TimeOfFlight(vector from, vector to)
	{
		return Math.Clamp(10 + vector.DistanceXZ(from, to) / 150, 12, 35);
	}

	static string DenyKey(string reason)
	{
		return "@fr_" + reason;
	}

	static string ShellKey(SCR_EAIArtilleryAmmoType t)
	{
		switch (t)
		{
			case SCR_EAIArtilleryAmmoType.SMOKE:			return "@shell_smoke";
			case SCR_EAIArtilleryAmmoType.ILLUMINATION:	return "@shell_illum";
		}
		return "@shell_he";
	}

	protected void AnnouncePlayerShot(CMD_FireMissionRequest req, DCO_GroupUtilityComponent unit)
	{
		float tof = TimeOfFlight(unit.GetOwner().GetOrigin(), req.m_vImpactPos);
		DCO_Radio.Group(req.m_iPlayerGroup, "FIRE SUPPORT", "fire_shot", DCO_ERadioKind.INFO, DCO_Radio.P("count", req.m_iShellCount.ToString(), "type", ShellKey(req.m_eShellType), "sec", DCO_Radio.N(tof)));
		GetGame().GetCallqueue().CallLater(AnnounceSplash, tof * 1000, false, req.m_iPlayerGroup);
		DCO_BenchmarkLoggerComponent.Event(string.Format("player_fire_shot grp=%1 type=%2 shells=%3 tof=%4", req.m_iPlayerGroup, typename.EnumToString(SCR_EAIArtilleryAmmoType, req.m_eShellType), req.m_iShellCount, Math.Round(tof)));
	}

	protected void AnnounceSplash(int groupID)
	{
		DCO_Radio.Group(groupID, "FIRE SUPPORT", "fire_splash", DCO_ERadioKind.GO);
	}

	int CancelPlayerMissions(int groupID)
	{
		int n;
		for (int i = m_aQueue.Count() - 1; i >= 0; i--)
		{
			CMD_FireMissionRequest req = m_aQueue[i];
			if (req && req.m_iPlayerGroup == groupID)
			{
				m_aQueue.Remove(i);
				n++;
			}
		}
		return n;
	}

	static bool IsLethal(CMD_FireMissionRequest req)
	{
		return req.m_eShellType == SCR_EAIArtilleryAmmoType.HIGH_EXPLOSIVE;
	}

	float ComputeSafeRadius(CMD_FireMissionRequest req, float worldTime)
	{
		if (!req)
			return m_fFriendlySafeRadius;

		vector firePos = FirePosFor(req.m_vImpactPos);
		float dispersion = CalculateArtilleryDispersion(firePos, req.m_vImpactPos, req.m_eShellType);
		float accuracy   = CalculateArtilleryAccuracy(req, firePos, worldTime, false);
		float spread = Math.Max(dispersion * (1.0 - accuracy), req.m_fAreaRadius);
		return Math.Max(m_fFriendlySafeRadius, spread + m_fLethalRadius);
	}

	bool HasFriendlyNearRequest(CMD_FireMissionRequest req, float worldTime)
	{
		req.m_fSafeRadius = ComputeSafeRadius(req, worldTime);

		if (!IsLethal(req))
			return false;

		return HasFriendlyNearFire(req);
	}

	protected bool HasFriendlyNearFire(CMD_FireMissionRequest req)
	{
		float radius = Math.Max(req.m_fSafeRadius, m_fFriendlySafeRadius);
		m_aFriendlyExclude.Clear();

		SCR_AIGroup reqGrp;
		if (req.m_bDangerClose && req.m_Requester)
			reqGrp = SCR_AIGroup.Cast(req.m_Requester.GetOwner());

		if (reqGrp)
		{
			float closeSq = Math.Min(radius, m_fDangerCloseRadius);
			closeSq = closeSq * closeSq;
			array<IEntity> members = {};
			array<AIAgent> agents = {};
			reqGrp.GetAgents(agents);
			foreach (AIAgent a : agents)
				members.Insert(a.GetControlledEntity());
			array<int> pids = reqGrp.GetPlayerIDs();
			if (pids)
			{
				foreach (int pid : pids)
					members.Insert(GetGame().GetPlayerManager().GetPlayerControlledEntity(pid));
			}
			foreach (IEntity ent : members)
			{
				if (!ent)
					continue;
				if (vector.DistanceSqXZ(ent.GetOrigin(), req.m_vImpactPos) < closeSq)
				{
					m_aFriendlyExclude.Clear();
					DCO_PlayerAwareness.NotifyHeld(m_Commander, req, radius);
					return true;
				}
				m_aFriendlyExclude.Insert(ent);
			}
		}

		bool found = HasFriendlyNearPos(req.m_vImpactPos, radius);
		m_aFriendlyExclude.Clear();
		if (found)
			DCO_PlayerAwareness.NotifyHeld(m_Commander, req, radius);
		return found;
	}

	bool HasFriendlyNearPos(vector pos, float radius)
	{
		if (!m_Commander)
			return false;

		m_bFriendlyFound   = false;
		m_sFriendlyFaction = m_Commander.GetCommanderFactionKey();
		m_FriendlyFaction  = GetGame().GetFactionManager().GetFactionByKey(m_sFriendlyFaction);

		DCO_Perf.Count("q:CommanderArtillerySupport");
		GetGame().GetWorld().QueryEntitiesBySphere(
			pos,
			radius,
			FriendlySafetyCallback,
			null,
			EQueryEntitiesFlags.ALL);

		return m_bFriendlyFound;
	}

	bool HasFriendlyNearPosDefault(vector pos)
	{
		return HasFriendlyNearPos(pos, m_fFriendlySafeRadius);
	}

	bool HasAnyAvailableUnit()
	{
		foreach (DCO_GroupUtilityComponent unit : m_aUnits)
		{
			if (unit && !unit.IsMoving())
				return true;
		}
		return false;
	}

	protected void ProcessQueue(float worldTime)
	{
	    PurgeExpiredRequests(worldTime);
	    PurgeDeadUnits();

	    CleanupExpiredCorrectionTracks(worldTime);

	    if (!m_Commander)
	        return;

	    if (m_aQueue.IsEmpty())
	        return;
	    if (m_aUnits.IsEmpty())
	        return;

	    if (worldTime < m_fGlobalCooldownUntil)
	        return;

	    if (worldTime < m_fNextProcessTime)
	        return;

		int shellsFired = 0;

		int qi = 0;
		while (qi < m_aQueue.Count())
	    {
		    CMD_FireMissionRequest req = m_aQueue[qi];
		    if (!req)
	    	{
		    	m_aQueue.Remove(qi);
		    	continue;
	    	}

		    string deny;
		    DCO_GroupUtilityComponent unit = FindUnitInRange(req.m_vImpactPos, true, deny);
		    if (!unit)
		    {
		        qi++;
		        continue;
		    }

		    req.m_fSafeRadius = ComputeSafeRadius(req, worldTime);
		    if (IsLethal(req) && HasFriendlyNearFire(req))
		    {
		        LogDenied(req, "friendly");
		        m_aQueue.Remove(qi);
		        continue;
		    }

		    DCO_PlayerAwareness.WarnArtillery(m_Commander, req, TimeOfFlight(unit.GetOwner().GetOrigin(), req.m_vImpactPos));

		    array<vector> artyPoint = GenerateArtilleryImpactPoints(req, unit.GetOwner().GetOrigin(), worldTime, req.m_iShellCount);

		    foreach (vector v : artyPoint)
		    {
		        SCR_AIWaypoint wp = m_Commander.SpawnArtilleryWP(v);
		        if (!wp)
		            continue;

		        SCR_AIWaypointArtillerySupport wps = SCR_AIWaypointArtillerySupport.Cast(wp);
		        if (!wps)
		        {
		            SCR_EntityHelper.DeleteEntityAndChildren(wp);
		            continue;
		        }
		        wps.SetAmmoType(req.m_eShellType);
		        wps.SetTargetShotCount(1);
		        wps.SetActive(true);

		        unit.ShootMortar(wps, worldTime);
		        shellsFired++;
		    }

		    if (IsLethal(req))
		    {
		    	DCO_CommanderDefense.BroadcastIndirectFire(m_Commander, unit, req.m_vImpactPos);
		    	DCO_PlayerAwareness.WarnIncoming(m_Commander, unit.GetOwner().GetOrigin(), req.m_vImpactPos);
		    }
		    if (req.m_iPlayerGroup >= 0)
		    	AnnouncePlayerShot(req, unit);
		    m_aQueue.Remove(qi);
		}

	    m_iTotalShellsFired = m_iTotalShellsFired + shellsFired;

	    m_fNextProcessTime = worldTime + m_fQueueProcessTime;

	    float shellCooldown = Math.Clamp(
	        m_fGlobalCooldown + (shellsFired * m_fCooldownPerShell),
	        m_fMinGlobalCooldown,
	        m_fMaxGlobalCooldown
	    );

	    float queueRatio = 0.0;
	    if (m_iMaxQueueSize > 0)
	        queueRatio = Math.Clamp(m_aQueue.Count() / (m_iMaxQueueSize * 1.0), 0.0, 1.0);

	    float finalCooldown = Math.Lerp(shellCooldown, m_fMinGlobalCooldown, queueRatio * 0.5);
	    m_fGlobalCooldownUntil = worldTime + finalCooldown;
	}

	array<vector> GenerateArtilleryImpactPoints(CMD_FireMissionRequest request, vector commanderPos, float worldTime, int shells = 1)
	{
	    float dispersion         = CalculateArtilleryDispersion(commanderPos, request.m_vImpactPos, request.m_eShellType);
	    float accuracy           = CalculateArtilleryAccuracy(request, commanderPos, worldTime);
	    float effectiveDispersion = Math.Max(dispersion * (1.0 - Math.Clamp(accuracy, 0.0, 1.0)), request.m_fAreaRadius);
	    float minRadius          = Math.Max(effectiveDispersion * 0.1, 1.0);

	    array<vector> impactPoints = new array<vector>();
	    RandomGenerator rand       = new RandomGenerator();

	    for (int i = 0; i < shells; i++)
	    {
	        float radius2;

	        if (m_fClusterRatio >= 0.5)
	        {
	            int sampleCount = Math.Round(Math.Lerp(1.0, 4.0, m_fClusterRatio));
	            float sum = 0.0;
	            for (int s = 0; s < sampleCount; s++)
	                sum += rand.RandFloatXY(minRadius, effectiveDispersion);
	            radius2 = sum / sampleCount;
	        }
	        else
	        {
	            radius2 = rand.RandFloatXY(minRadius, effectiveDispersion);
	        }

	        float angleDeg = rand.RandFloatXY(0.0, 360.0);
	        float angleRad = angleDeg * Math.DEG2RAD;

	        float px = request.m_vImpactPos[0] + Math.Cos(angleRad) * radius2;
	        float pz = request.m_vImpactPos[2] + Math.Sin(angleRad) * radius2;
	        float py = GetGame().GetWorld().GetSurfaceY(px, pz);

	        impactPoints.Insert(Vector(px, py, pz));
	    }

	    return impactPoints;
	}

	float CalculateArtilleryDispersion(vector commanderPos, vector impactPos, SCR_EAIArtilleryAmmoType shellType)
	{
	    float rangeToTarget  = vector.Distance(commanderPos, impactPos);
	    float baseDispersion = Math.Clamp(rangeToTarget * m_fDispersionRangeScale, m_fMinDispersion, m_fMaxDispersion);

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

	    return Math.Clamp(baseDispersion * m_fDispersionMultiplier, m_fMinDispersion, m_fMaxDispersion);
	}

	float CalculateArtilleryAccuracy(CMD_FireMissionRequest request, vector commanderPos, float worldTime, bool applyCorrection = true)
	{
	    float accuracy       = m_fBaseAccuracy;
	    float rangeToTarget  = vector.Distance(commanderPos, request.m_vImpactPos);

	    float dataAge = worldTime - request.m_fTargetLastSeenTime;

	    if (dataAge < 30.0)
	        accuracy += 0.10;

	    if (request.m_eShellType == SCR_EAIArtilleryAmmoType.PRACTICE)
	        accuracy += 0.20;

	    if (dataAge > 30.0)
	        accuracy -= Math.Clamp((dataAge - 30.0) * 0.01, 0.0, 0.25);

	    accuracy -= Math.Clamp(rangeToTarget * 0.0001, 0.0, 0.20);

	    if (request.m_eShellType == SCR_EAIArtilleryAmmoType.ILLUMINATION)
	        accuracy -= 0.15;

	    if (request.m_eShellType == SCR_EAIArtilleryAmmoType.SMOKE)
	        accuracy -= 0.10;

	    accuracy += (request.m_fReportQuality - 0.7) * 0.5;

	    if (applyCorrection)
	        accuracy += GetCorrectionBonus(request.m_vImpactPos, worldTime);

	    return Math.Clamp(accuracy, 0.05, 0.95);
	}

	protected float GetCorrectionBonus(vector impactPos, float worldTime)
	{
	    CMD_ArtilleryCorrectionTrack found = null;

	    foreach (CMD_ArtilleryCorrectionTrack t : m_aCorrectionTracks)
	    {
	        if (!t)
	            continue;

	        if ((worldTime - t.m_fLastCallTime) > m_fCorrectionExpiry)
	            continue;

	        if (vector.Distance(t.m_vPosition, impactPos) <= m_fCorrectionRadius)
	        {
	            found = t;
	            break;
	        }
	    }

	    if (!found)
	    {
	        found = new CMD_ArtilleryCorrectionTrack();
	        found.m_vPosition       = impactPos;
	        found.m_iCorrectionStep = 0;
	        m_aCorrectionTracks.Insert(found);
	    }

	    found.m_iCorrectionStep = Math.Min(found.m_iCorrectionStep + 1, m_iMaxCorrections);
	    found.m_fLastCallTime   = worldTime;
	    found.m_vPosition       = impactPos;

	    return (found.m_iCorrectionStep - 1) * m_fCorrectionBonusPerStep;
	}

	protected void CleanupExpiredCorrectionTracks(float worldTime)
	{
	    for (int i = m_aCorrectionTracks.Count() - 1; i >= 0; i--)
	    {
	        CMD_ArtilleryCorrectionTrack t = m_aCorrectionTracks[i];
	        if (!t || (worldTime - t.m_fLastCallTime) > m_fCorrectionExpiry)
	            m_aCorrectionTracks.Remove(i);
	    }
	}

	protected void PurgeExpiredRequests(float worldTime)
	{
		int i = 0;
		while (i < m_aQueue.Count())
		{
			CMD_FireMissionRequest req = m_aQueue[i];
			if (!req || worldTime - req.m_fRequestedTime > m_fQueueExpiry)
			{
				if (req)
					LogDenied(req, "stale");
				m_aQueue.Remove(i);
				continue;
			}
			i++;
		}
	}

	protected void PurgeDeadUnits()
	{
		int i = 0;
		while (i < m_aUnits.Count())
		{
			DCO_GroupUtilityComponent unit = m_aUnits[i];
			if (!unit)
			{
				m_aUnits.Remove(i);
				continue;
			}
			i++;
		}
	}

	protected bool FriendlySafetyCallback(IEntity ent)
	{
		if (!ent || m_aFriendlyExclude.Contains(ent))
			return true;

		FactionAffiliationComponent facComp = FactionAffiliationComponent.Cast(
			ent.FindComponent(FactionAffiliationComponent));

		if (!facComp)
			return true;

		Faction faction = facComp.GetAffiliatedFaction();
		if (!faction)
			return true;

		if (faction.GetFactionKey() == m_sFriendlyFaction || (m_FriendlyFaction && m_FriendlyFaction.IsFactionFriendly(faction)))
		{
			m_bFriendlyFound = true;
			return false;
		}

		return true;
	}

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (!Replication.IsServer())
			return;

		m_fDispatchTimer = m_fDispatchTimer + timeSlice;
		if (m_fDispatchTimer < m_fDispatchInterval)
			return;

		m_fDispatchTimer = 0.0;
		float worldTime  = GetGame().GetWorld().GetWorldTime() / 1000.0;
		ProcessQueue(worldTime);
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
		{
			Print("[CMD_ArtillerySupport] PERINGATAN — AICommander_BaseComponent tidak ditemukan. " +
				"Friendly safety check dan ProcessQueue tidak akan berfungsi (di-guard, gak crash, tapi artillery gak akan pernah nembak).");
		}
		else
		{
			Print("[CMD_ArtillerySupport] Initialized, menunggu unit artillery daftar.");
		}
	}
}

class CMD_ArtilleryCorrectionTrack
{
	vector m_vPosition;
	int    m_iCorrectionStep;
	float  m_fLastCallTime;
}
