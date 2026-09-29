class DCO_PlayerContactReports
{
	protected static const float SCAN_S = 10;
	protected static const float NOISE_PER_100M = 12;

	protected static ref DCO_PlayerContactReports s_Instance;

	protected float m_fScanTimer;
	protected ref map<int, float> m_mLastReport = new map<int, float>();
	protected ref map<int, ref array<float>> m_mActive = new map<int, ref array<float>>();
	protected ref map<int, ref CMD_ThreatEntry> m_mMarkerThreat = new map<int, ref CMD_ThreatEntry>();
	protected ref set<int> m_Seen = new set<int>();

	static DCO_PlayerContactReports Get()
	{
		if (!s_Instance)
			s_Instance = new DCO_PlayerContactReports();
		return s_Instance;
	}

	static void Tick(float timeSlice)
	{
		DCO_PlayerContactReports r = Get();
		r.m_fScanTimer += timeSlice;
		if (r.m_fScanTimer < SCAN_S)
			return;
		r.m_fScanTimer = 0;
		r.ScanMarkers();
	}

	static void ReportFromRadial(int playerID, vector pos, int count, bool armor)
	{
		Get().Submit(playerID, pos, count, armor, -1);
	}

	protected void ScanMarkers()
	{
		SCR_MapMarkerManagerComponent mm = SCR_MapMarkerManagerComponent.GetInstance();
		if (!mm || !mm.GetMarkerConfig())
			return;

		SCR_MapMarkerEntryMilitary milCfg = SCR_MapMarkerEntryMilitary.Cast(mm.GetMarkerConfig().GetMarkerEntryConfigByType(SCR_EMapMarkerType.PLACED_MILITARY));
		if (!milCfg)
			return;

		set<int> now = new set<int>();
		foreach (SCR_MapMarkerBase marker : mm.GetStaticMarkers())
		{
			if (!marker || marker.GetType() != SCR_EMapMarkerType.PLACED_MILITARY || marker.GetMarkerOwnerID() <= 0)
				continue;

			int mid = marker.GetMarkerID();
			now.Insert(mid);
			if (m_Seen.Contains(mid))
				continue;
			m_Seen.Insert(mid);

			int pid = marker.GetMarkerOwnerID();
			Faction reporterFaction = SCR_FactionManager.SGetPlayerFaction(pid);
			SCR_MarkerMilitaryFactionEntry entry = milCfg.GetFactionEntry(marker.GetMarkerConfigID() % SCR_MapMarkerEntryMilitary.FACTION_DETERMINATOR);
			if (!reporterFaction || !entry || !IsHostileIdentity(entry.GetFactionIdentity(), reporterFaction))
				continue;

			int wp[2];
			marker.GetWorldPos(wp);
			vector pos = Vector(wp[0], 0, wp[1]);
			pos[1] = GetGame().GetWorld().GetSurfaceY(pos[0], pos[2]);

			int flags = marker.GetFlags();
			bool armor = (flags & (EMilitarySymbolIcon.ARMOR | EMilitarySymbolIcon.TRACKED_TRANSPORT)) != 0;
			int count = ParseCount(marker.GetCustomText());
			if (!armor && (flags & (EMilitarySymbolIcon.MOTORIZED | EMilitarySymbolIcon.WHEELED_TRANSPORT)))
				count = Math.Max(count, 4);

			Submit(pid, pos, count, armor, mid);
		}

		for (int i = m_Seen.Count() - 1; i >= 0; i--)
		{
			int mid = m_Seen[i];
			if (now.Contains(mid))
				continue;

			m_Seen.Remove(i);
			CMD_ThreatEntry t = m_mMarkerThreat.Get(mid);
			if (t)
				t.m_fLastUpdateTime = -100000;
			m_mMarkerThreat.Remove(mid);
		}
	}

	protected void Submit(int pid, vector pos, int count, bool armor, int markerID)
	{
		float now = GetGame().GetWorld().GetWorldTime() / 1000.0;
		float last;
		if (m_mLastReport.Find(pid, last) && now - last < 20.0)
		{
			DCO_Radio.Player(pid, "COMMANDER", "report_wait");
			return;
		}

		array<float> active = m_mActive.Get(pid);
		if (!active)
		{
			active = {};
			m_mActive.Insert(pid, active);
		}
		for (int i = active.Count() - 1; i >= 0; i--)
		{
			if (now - active[i] > 180.0)
				active.Remove(i);
		}
		if (active.Count() >= 3)
		{
			DCO_Radio.Player(pid, "COMMANDER", "report_too_many");
			return;
		}

		AICommander_BaseComponent cmd = FindCommanderForPlayer(pid);
		if (!cmd || !cmd.GetThreatResponseComponent())
		{
			DCO_Radio.Player(pid, "COMMANDER", "no_net", DCO_ERadioKind.WARNING);
			return;
		}

		m_mLastReport.Set(pid, now);
		active.Insert(now);

		IEntity reporter = GetGame().GetPlayerManager().GetPlayerControlledEntity(pid);
		float dist = 200;
		if (reporter)
			dist = vector.DistanceXZ(reporter.GetOrigin(), pos);

		float noise = dist / 100 * NOISE_PER_100M;
		vector noisy = pos + Vector(Math.RandomFloat(-noise, noise), 0, Math.RandomFloat(-noise, noise));
		noisy[1] = GetGame().GetWorld().GetSurfaceY(noisy[0], noisy[2]);

		CMD_ContactReport rep = new CMD_ContactReport(noisy, count, now, "player " + pid);
		rep.m_bArmorSeen = armor;
		rep.m_fUncertainty = noise * 1.5 + 10;
		rep.m_vTruePos = pos;

		float delay = Math.RandomFloat(5.0, 15.0);
		GetGame().GetCallqueue().CallLater(Deliver, delay * 1000, false, cmd, rep, markerID, pid);

		string what = "@rep_squad";
		if (armor)
			what = "@rep_armor";
		else if (count <= 4)
			what = "@rep_team";
		else if (count >= 24)
			what = "@rep_platoon";
		DCO_Radio.Player(pid, "CONTACT REPORT", "report_sent", DCO_ERadioKind.INFO, DCO_Radio.P("what", what, "grid", DCO_PlayerComms.Grid(pos)));

		string line = string.Format("player_contact_report player=%1 pos=%2 count=%3 armor=%4 dist=%5 noise=%6 marker=%7",
			pid, pos, count, armor, Math.Round(dist), Math.Round(noise), markerID);
		Print("[DCO_PlayerReport] " + line);
		DCO_BenchmarkLoggerComponent.Event(line);
	}

	protected void Deliver(AICommander_BaseComponent cmd, CMD_ContactReport rep, int markerID, int pid)
	{
		if (!cmd || !cmd.GetThreatResponseComponent())
			return;

		CMD_ThreatResponseComponent tr = cmd.GetThreatResponseComponent();
		tr.ReceiveContactReport(rep, null);
		CMD_ThreatEntry entry = tr.FindThreatNear(rep.m_vPosition);
		if (markerID >= 0)
			m_mMarkerThreat.Set(markerID, entry);
		if (entry)
		{
			entry.m_iReporterPid = pid;
			entry.m_bReporterNotified = false;
		}

		DCO_Radio.Player(pid, "COMMANDER", "report_received", DCO_ERadioKind.INFO, DCO_Radio.P("grid", DCO_PlayerComms.Grid(rep.m_vPosition)));
	}

	protected static int ParseCount(string text)
	{
		string t = text;
		t.ToLower();
		if (t.Contains("platoon") || t.Contains("peleton"))
			return 24;
		if (t.Contains("team") || t.Contains("tim"))
			return 4;

		int n = t.ToInt();
		if (n > 0)
			return n;
		return 8;
	}

	protected static bool IsHostileIdentity(EMilitarySymbolIdentity id, Faction reporter)
	{
		if (id == EMilitarySymbolIdentity.CIVILIAN || id == EMilitarySymbolIdentity.ASSUMED_CIVILIAN)
			return false;

		EMilitarySymbolIdentity own = EMilitarySymbolIdentity.BLUFOR;
		string key = reporter.GetFactionKey();
		if (key == "USSR")
			own = EMilitarySymbolIdentity.OPFOR;
		else if (key == "FIA")
			own = EMilitarySymbolIdentity.INDFOR;

		switch (own)
		{
			case EMilitarySymbolIdentity.BLUFOR:	return id != EMilitarySymbolIdentity.BLUFOR && id != EMilitarySymbolIdentity.ASSUMED_BLUFOR;
			case EMilitarySymbolIdentity.OPFOR:		return id != EMilitarySymbolIdentity.OPFOR && id != EMilitarySymbolIdentity.ASSUMED_OPFOR;
			case EMilitarySymbolIdentity.INDFOR:	return id != EMilitarySymbolIdentity.INDFOR && id != EMilitarySymbolIdentity.ASSUMED_INDFOR;
		}
		return true;
	}

	static AICommander_BaseComponent FindCommanderForPlayer(int pid)
	{
		SCR_GroupsManagerComponent groups = SCR_GroupsManagerComponent.GetInstance();
		if (groups)
		{
			SCR_AIGroup grp = groups.GetPlayerGroup(pid);
			if (grp)
				return grp.DCO_GetOrdersCommander();
		}
		return FindCommander(SCR_FactionManager.SGetPlayerFaction(pid));
	}

	static AICommander_BaseComponent FindCommander(Faction f)
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr || !f)
			return null;

		foreach (AICommander_BaseComponent cmd : mgr.m_aCommander)
		{
			if (cmd && cmd.GetCommanderFactionKey() == f.GetFactionKey())
				return cmd;
		}
		return null;
	}
}
