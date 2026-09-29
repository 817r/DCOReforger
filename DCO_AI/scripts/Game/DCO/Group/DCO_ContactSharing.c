class DCO_SharedContact
{
	ref array<IEntity> m_aTargets = {};
	ref array<vector> m_aPositions = {};
	ref array<Faction> m_aFactions = {};
	vector m_vCenter;
	int m_iCount;
	bool m_bVehicle;
	int m_iHop;
	bool m_bRadio;
	vector m_vOffset;
	SCR_AIGroup m_Origin;
	SCR_AIGroup m_Sender;
	SCR_AIGroupUtilityComponent m_Receiver;
	int m_iPlayerID;
	float m_fDeliverAt;

	DCO_SharedContact CopyFor(float deliverAt, bool radio, vector offset)
	{
		DCO_SharedContact c = new DCO_SharedContact();
		c.m_aTargets.Copy(m_aTargets);
		c.m_aPositions.Copy(m_aPositions);
		c.m_aFactions.Copy(m_aFactions);
		c.m_vCenter = m_vCenter;
		c.m_iCount = m_iCount;
		c.m_bVehicle = m_bVehicle;
		c.m_iHop = m_iHop;
		c.m_Origin = m_Origin;
		c.m_Sender = m_Sender;
		c.m_fDeliverAt = deliverAt;
		c.m_bRadio = radio;
		c.m_vOffset = offset;
		return c;
	}
}

class DCO_ShareTrack
{
	vector m_vPos;
	float m_fSentAt;
	float m_fInfoTs;
	float m_fLastMatched;
}

class DCO_ContactSharing
{
	protected static const int TICK_MS = 1000;

	protected static ref DCO_ContactSharing s_Instance;

	protected ref array<SCR_AIGroupUtilityComponent> m_aGroups = {};
	protected ref array<ref DCO_SharedContact> m_aPending = {};
	protected ref map<string, float> m_mPlayerLast = new map<string, float>();
	protected int m_iScanIdx;

	static void Register(SCR_AIGroupUtilityComponent group)
	{
		if (!group || !Replication.IsServer())
			return;

		if (!s_Instance)
		{
			s_Instance = new DCO_ContactSharing();
			GetGame().GetCallqueue().CallLater(s_Instance.Tick, TICK_MS, true);
		}

		if (!s_Instance.m_aGroups.Contains(group))
			s_Instance.m_aGroups.Insert(group);
	}

	protected void Tick()
	{
		int pt = DCO_Perf.Begin();
		DoTick();
		DCO_Perf.End("contact_sharing", pt);
	}

	protected void DoTick()
	{
		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetInstance();
		if (!cfg || !cfg.GetShareEnabled() || !GetGame().GetWorld())
		{
			m_aPending.Clear();
			return;
		}

		PerceptionManager pm = GetGame().GetPerceptionManager();
		if (!pm)
			return;

		float now = GetGame().GetWorld().GetWorldTime() / 1000.0;
		Deliver(cfg, pm, now);

		int n = m_aGroups.Count();
		int budget = Math.Max(4, Math.Ceil(n / 3.0));
		while (budget > 0 && !m_aGroups.IsEmpty())
		{
			if (m_iScanIdx >= m_aGroups.Count())
				m_iScanIdx = 0;

			SCR_AIGroupUtilityComponent g = m_aGroups[m_iScanIdx];
			if (!g)
			{
				m_aGroups.Remove(m_iScanIdx);
				continue;
			}

			m_iScanIdx++;
			budget--;
			ScanSender(cfg, pm, g, now);
		}
	}

	protected void ScanSender(DCO_GlobalAIComponent cfg, PerceptionManager pm, SCR_AIGroupUtilityComponent u, float now)
	{
		SCR_AIGroup grp = u.m_Owner;
		if (!grp || !u.m_Perception || !IsAlive(grp.GetLeaderEntity()) || IsPlayerGroup(grp))
			return;

		if (u.m_mDCOInjected.Count() > 128)
			u.m_mDCOInjected.Clear();

		float pmNow = pm.GetTime();
		array<ref DCO_SharedContact> clusters = {};
		array<float> newest = {};

		foreach (SCR_AITargetInfo t : u.m_Perception.m_aTargets)
		{
			if (!t || !t.m_Entity)
				continue;
			if (t.m_eCategory != EAITargetInfoCategory.DETECTED && t.m_eCategory != EAITargetInfoCategory.IDENTIFIED)
				continue;
			if (pmNow - t.m_fTimestamp > 10.0)
				continue;

			float injected = 0;
			if (u.m_mDCOInjected.Find(t.m_Entity, injected) && t.m_fTimestamp <= injected + 0.01)
				continue;

			int ci = -1;
			foreach (int i, DCO_SharedContact c : clusters)
			{
				if (vector.DistanceXZ(c.m_vCenter, t.m_vWorldPos) <= 75.0)
				{
					ci = i;
					break;
				}
			}
			if (ci < 0)
			{
				DCO_SharedContact nc = new DCO_SharedContact();
				nc.m_vCenter = t.m_vWorldPos;
				clusters.Insert(nc);
				newest.Insert(0);
				ci = clusters.Count() - 1;
			}

			DCO_SharedContact cl = clusters[ci];
			Faction fac = t.m_Faction;
			if (!fac)
				fac = SCR_AIFactionHandling.GetEntityPerceivedFaction(t.m_Entity);
			if (cl.m_aTargets.Count() < 6)
			{
				cl.m_aTargets.Insert(t.m_Entity);
				cl.m_aPositions.Insert(t.m_vWorldPos);
				cl.m_aFactions.Insert(fac);
			}
			cl.m_iCount++;
			cl.m_vCenter = cl.m_vCenter + (t.m_vWorldPos - cl.m_vCenter) * (1.0 / cl.m_iCount);
			if (Vehicle.Cast(t.m_Entity))
				cl.m_bVehicle = true;
			newest[ci] = Math.Max(newest[ci], t.m_fTimestamp);
		}

		for (int i = u.m_aDCOShareTracks.Count() - 1; i >= 0; i--)
		{
			if (now - u.m_aDCOShareTracks[i].m_fLastMatched > 60.0)
				u.m_aDCOShareTracks.Remove(i);
		}

		foreach (int i, DCO_SharedContact c : clusters)
		{
			DCO_ShareTrack track = null;
			foreach (DCO_ShareTrack tr : u.m_aDCOShareTracks)
			{
				if (vector.DistanceXZ(tr.m_vPos, c.m_vCenter) <= 110.0)
				{
					track = tr;
					break;
				}
			}

			bool isNew = !track
				|| vector.DistanceXZ(track.m_vPos, c.m_vCenter) > 50.0
				|| (now - track.m_fSentAt >= 10.0 && newest[i] > track.m_fInfoTs + 0.5);
			if (track)
				track.m_fLastMatched = now;
			if (!isNew)
				continue;

			if (!track)
			{
				track = new DCO_ShareTrack();
				u.m_aDCOShareTracks.Insert(track);
			}
			track.m_vPos = c.m_vCenter;
			track.m_fSentAt = now;
			track.m_fInfoTs = newest[i];
			track.m_fLastMatched = now;

			c.m_iHop = 1;
			c.m_Origin = grp;
			Share(cfg, u, c, now);
		}
	}

	protected void Share(DCO_GlobalAIComponent cfg, SCR_AIGroupUtilityComponent senderUtil, DCO_SharedContact c, float now)
	{
		SCR_AIGroup sender = senderUtil.m_Owner;
		Faction senderFaction = sender.GetFaction();
		IEntity senderLeader = sender.GetLeaderEntity();
		if (!senderFaction || !senderLeader)
			return;

		c.m_Sender = sender;
		vector senderPos = senderLeader.GetOrigin();
		float voice = cfg.GetShareVoiceRange();
		float radio = cfg.GetShareRadioRange();
		bool needRadio = cfg.GetShareRequireRadio();
		bool senderRadio = !needRadio || GroupHasRadio(sender);
		float spotDist = vector.Distance(senderPos, c.m_vCenter);
		float pmNow = GetGame().GetPerceptionManager().GetTime();

		foreach (SCR_AIGroupUtilityComponent r : m_aGroups)
		{
			if (!r || r == senderUtil || !r.m_Owner || r.m_Owner == c.m_Origin || !r.m_Perception)
				continue;
			if (m_aPending.Count() >= 128)
				break;
			if (IsPlayerGroup(r.m_Owner) || !IsAlive(r.m_Owner.GetLeaderEntity()))
				continue;

			Faction rf = r.m_Owner.GetFaction();
			if (!rf || !senderFaction.IsFactionFriendly(rf))
				continue;

			float d = vector.Distance(senderPos, r.m_Owner.GetLeaderEntity().GetOrigin());
			bool useRadio = false;
			if (d > voice)
			{
				if (d > radio || !senderRadio || (needRadio && !GroupHasRadio(r.m_Owner)))
					continue;
				useRadio = true;
			}

			if (AlreadyKnows(r, c))
				continue;

			DCO_SharedContact k = c.CopyFor(now + Delay(useRadio), useRadio, Noise(cfg, spotDist, useRadio, c.m_iHop));
			k.m_Receiver = r;
			m_aPending.Insert(k);
			Reserve(r, c, pmNow);
		}

		if (cfg.GetShareToPlayers())
			SharePlayers(cfg, c, sender, senderFaction, senderPos, spotDist, senderRadio, now);
	}

	protected void SharePlayers(DCO_GlobalAIComponent cfg, DCO_SharedContact c, SCR_AIGroup sender, Faction senderFaction, vector senderPos, float spotDist, bool senderRadio, float now)
	{
		array<int> players = {};
		GetGame().GetPlayerManager().GetPlayers(players);
		array<int> senderPlayers = sender.GetPlayerIDs();

		foreach (int pid : players)
		{
			if (senderPlayers && senderPlayers.Contains(pid))
				continue;

			IEntity ent = GetGame().GetPlayerManager().GetPlayerControlledEntity(pid);
			if (!IsAlive(ent))
				continue;

			Faction pf = SCR_FactionManager.SGetPlayerFaction(pid);
			if (!pf || !senderFaction.IsFactionFriendly(pf))
				continue;

			float d = vector.Distance(senderPos, ent.GetOrigin());
			bool useRadio = false;
			if (d > cfg.GetShareVoiceRange())
			{
				if (d > cfg.GetShareRadioRange() || !senderRadio || (cfg.GetShareRequireRadio() && !HasRadio(ent)))
					continue;
				useRadio = true;
			}

			string key = pid.ToString() + ":" + Math.Floor(c.m_vCenter[0] / 250.0).ToString() + ":" + Math.Floor(c.m_vCenter[2] / 250.0).ToString();
			float last = 0;
			if (m_mPlayerLast.Find(key, last) && now - last < 30.0)
				continue;
			if (m_mPlayerLast.Count() > 1024)
				m_mPlayerLast.Clear();
			m_mPlayerLast.Set(key, now);

			DCO_SharedContact k = c.CopyFor(now + Delay(useRadio), useRadio, Noise(cfg, spotDist, useRadio, c.m_iHop));
			k.m_iPlayerID = pid;
			m_aPending.Insert(k);
		}
	}

	protected void Deliver(DCO_GlobalAIComponent cfg, PerceptionManager pm, float now)
	{
		for (int i = m_aPending.Count() - 1; i >= 0; i--)
		{
			DCO_SharedContact c = m_aPending[i];
			if (now < c.m_fDeliverAt)
				continue;

			m_aPending.Remove(i);

			if (c.m_iPlayerID > 0)
			{
				DeliverPlayer(c);
				continue;
			}

			SCR_AIGroupUtilityComponent r = c.m_Receiver;
			if (!r || !r.m_Owner || !r.m_Perception || KnowsFirstHand(r, c))
				continue;

			float pmNow = pm.GetTime();
			foreach (int t, IEntity ent : c.m_aTargets)
			{
				if (!ent)
					continue;
				vector pos = c.m_aPositions[t] + c.m_vOffset;
				pos[1] = GetGame().GetWorld().GetSurfaceY(pos[0], pos[2]);
				r.m_Perception.AddOrUpdateGunshot(ent, pos, c.m_aFactions[t], pmNow, false);
				r.m_mDCOInjected.Set(ent, pmNow);

				if (cfg.GetShareMapMarkers())
					MarkOnMap(r, ent);
			}

			if (cfg.GetShareDebug())
				Print(string.Format("[DCO_Share] %1 -> %2 hop %3 %4 (%5 musuh, noise %6 m)", DCO_PlayerComms.GetCallsign(c.m_Sender),
					DCO_PlayerComms.GetCallsign(r.m_Owner), c.m_iHop, Channel(c.m_bRadio), c.m_iCount, Math.Round(c.m_vOffset.Length())));

			if (c.m_iHop < cfg.GetShareHops())
			{
				DCO_SharedContact fwd = c.CopyFor(0, false, vector.Zero);
				fwd.m_iHop = c.m_iHop + 1;
				Share(cfg, r, fwd, now);
			}
		}
	}

	protected void DeliverPlayer(DCO_SharedContact c)
	{
		IEntity ent = GetGame().GetPlayerManager().GetPlayerControlledEntity(c.m_iPlayerID);
		SCR_PlayerControllerGroupComponent comp = SCR_PlayerControllerGroupComponent.GetPlayerControllerComponent(c.m_iPlayerID);
		if (!IsAlive(ent) || !comp)
			return;

		vector pos = c.m_vCenter + c.m_vOffset;
		string text = What(c);
		if (c.m_bRadio)
			text = text + ", grid " + DCO_PlayerComms.Grid(pos);
		else
		{
			int dist = Math.Max(50, Math.Round(vector.DistanceXZ(ent.GetOrigin(), pos) / 50) * 50);
			text = string.Format("%1, %2 m %3", text, dist, ShortBearing(ent.GetOrigin(), pos));
		}

		comp.DCO_SendContact(DCO_PlayerComms.GetCallsign(c.m_Sender), text, c.m_bRadio);
	}

	protected static string What(DCO_SharedContact c)
	{
		if (c.m_bVehicle)
			return "enemy vehicle";
		if (c.m_iCount <= 3)
			return "enemy team";
		if (c.m_iCount <= 9)
			return "enemy squad";
		return "enemy platoon";
	}

	protected static string ShortBearing(vector from, vector to)
	{
		vector d = to - from;
		float yaw = Math.Atan2(d[0], d[2]) * Math.RAD2DEG;
		if (yaw < 0)
			yaw += 360;
		int sector = Math.Round(yaw / 45);
		array<string> names = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
		return names[sector % 8];
	}

	protected static string Channel(bool radio)
	{
		if (radio)
			return "radio";
		return "suara";
	}

	protected static float Delay(bool radio)
	{
		if (radio)
			return Math.RandomFloat(2, 5);
		return Math.RandomFloat(1, 2);
	}

	protected static vector Noise(DCO_GlobalAIComponent cfg, float spotDist, bool radio, int hop)
	{
		float sigma = cfg.GetShareNoisePer100m() * spotDist / 100.0;
		float minSigma = 10;
		if (radio)
		{
			sigma = sigma * cfg.GetShareRadioNoise();
			minSigma = 5;
		}
		else
			sigma = sigma * cfg.GetShareVoiceNoise();
		for (int h = 1; h < hop; h++)
			sigma = sigma * 1.5;
		sigma = Math.Max(sigma, minSigma);

		float angle = Math.RandomFloat(0, Math.PI2);
		float r = sigma * Math.Sqrt(Math.RandomFloat01());
		return Vector(Math.Cos(angle) * r, 0, Math.Sin(angle) * r);
	}

	protected bool AlreadyKnows(SCR_AIGroupUtilityComponent r, DCO_SharedContact c)
	{
		PerceptionManager pm = GetGame().GetPerceptionManager();
		float pmNow = pm.GetTime();
		foreach (IEntity ent : c.m_aTargets)
		{
			if (!ent)
				continue;
			float injected = 0;
			if (r.m_mDCOInjected.Find(ent, injected) && pmNow - injected < 15.0)
				continue;
			int idx = r.m_Perception.m_aTargetEntities.Find(ent);
			if (idx >= 0 && pmNow - r.m_Perception.m_aTargets[idx].m_fTimestamp < 15.0)
				continue;
			return false;
		}
		return true;
	}

	protected static void Reserve(SCR_AIGroupUtilityComponent r, DCO_SharedContact c, float pmNow)
	{
		foreach (IEntity ent : c.m_aTargets)
		{
			if (ent)
				r.m_mDCOInjected.Set(ent, pmNow);
		}
	}

	protected bool KnowsFirstHand(SCR_AIGroupUtilityComponent r, DCO_SharedContact c)
	{
		float pmNow = GetGame().GetPerceptionManager().GetTime();
		foreach (IEntity ent : c.m_aTargets)
		{
			if (!ent)
				continue;
			int idx = r.m_Perception.m_aTargetEntities.Find(ent);
			if (idx < 0)
				return false;
			float ts = r.m_Perception.m_aTargets[idx].m_fTimestamp;
			if (pmNow - ts >= 15.0)
				return false;
			float injected = 0;
			if (r.m_mDCOInjected.Find(ent, injected) && ts <= injected + 0.01)
				return false;
		}
		return true;
	}

	protected static void MarkOnMap(SCR_AIGroupUtilityComponent r, IEntity ent)
	{
		SCR_AIEnemyMarkingSystem marking = SCR_AIEnemyMarkingSystem.Cast(GetGame().GetWorld().FindSystem(SCR_AIEnemyMarkingSystem));
		int idx = r.m_Perception.m_aTargetEntities.Find(ent);
		if (!marking || idx < 0 || !r.m_Owner.GetLeaderEntity())
			return;

		SCR_AITargetInfo info = r.m_Perception.m_aTargets[idx];
		if (info && info.m_Faction)
			marking.MarkTarget(info, r.m_Owner);
	}

	protected static bool IsPlayerGroup(SCR_AIGroup grp)
	{
		array<int> ids = grp.GetPlayerIDs();
		return ids && !ids.IsEmpty();
	}

	protected static bool IsAlive(IEntity ent)
	{
		ChimeraCharacter ch = ChimeraCharacter.Cast(ent);
		if (!ch)
			return false;
		CharacterControllerComponent ctrl = ch.GetCharacterController();
		return ctrl && ctrl.GetLifeState() == ECharacterLifeState.ALIVE;
	}

	static bool HasRadio(IEntity ent)
	{
		if (!ent)
			return false;
		SCR_GadgetManagerComponent gadgets = SCR_GadgetManagerComponent.Cast(ent.FindComponent(SCR_GadgetManagerComponent));
		return gadgets && gadgets.GetGadgetByType(EGadgetType.RADIO);
	}

	protected static bool GroupHasRadio(SCR_AIGroup grp)
	{
		array<AIAgent> agents = {};
		grp.GetAgents(agents);
		foreach (AIAgent a : agents)
		{
			if (a && HasRadio(a.GetControlledEntity()))
				return true;
		}
		return false;
	}
}
