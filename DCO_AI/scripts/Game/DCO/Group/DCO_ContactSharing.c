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
	int m_iDeferTicks;
	int m_iMG;
	int m_iAT;
	int m_iSniper;
	bool m_bArmor;
	vector m_vVelocity;
	bool m_bAmbush;

	DCO_SharedContact CopyFor(float deliverAt, bool radio, vector offset)
	{
		DCO_SharedContact c = new DCO_SharedContact();
		c.m_aTargets.Copy(m_aTargets);
		c.m_aPositions.Copy(m_aPositions);
		c.m_aFactions.Copy(m_aFactions);
		c.m_vCenter = m_vCenter;
		c.m_iCount = m_iCount;
		c.m_bVehicle = m_bVehicle;
		c.m_iMG = m_iMG;
		c.m_iAT = m_iAT;
		c.m_iSniper = m_iSniper;
		c.m_bArmor = m_bArmor;
		c.m_vVelocity = m_vVelocity;
		c.m_bAmbush = m_bAmbush;
		c.m_iHop = m_iHop;
		c.m_Origin = m_Origin;
		c.m_Sender = m_Sender;
		c.m_fDeliverAt = deliverAt;
		c.m_bRadio = radio;
		c.m_vOffset = offset;
		return c;
	}
}

class DCO_SharedEst
{
	SCR_AIGroup m_Receiver;
	SCR_AIGroup m_Sender;
	vector m_vPos;
	float m_fRadius;
	int m_iCount;
	int m_iHop;
	bool m_bRadio;
	float m_fDeliverAt;
	bool m_bHeard;
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

	protected static ref DCO_ContactSharing s_Instance;

	protected ref array<SCR_AIGroupUtilityComponent> m_aGroups = {};
	protected ref array<ref DCO_SharedContact> m_aPending = {};
	protected ref array<ref DCO_SharedContact> m_aDeferred = {};
	protected ref array<ref DCO_SharedEst> m_aEstPending = {};
	protected ref map<int, float> m_mPlayerLast = new map<int, float>();
	protected int m_iScanIdx;

	static int PendingCount()
	{
		if (!s_Instance)
			return 0;
		return s_Instance.m_aPending.Count() + s_Instance.m_aDeferred.Count();
	}

	static void Register(SCR_AIGroupUtilityComponent group)
	{
		if (!group || !Replication.IsServer())
			return;

		if (!s_Instance)
		{
			s_Instance = new DCO_ContactSharing();
			GetGame().GetCallqueue().CallLater(s_Instance.Tick, 1000, true);
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
			m_aDeferred.Clear();
			return;
		}

		PerceptionManager pm = GetGame().GetPerceptionManager();
		if (!pm)
			return;

		float now = DCO_Util.NowS();
		PromoteDeferred(now);
		Deliver(cfg, pm, now);
		DeliverEst(now);

		int n = m_aGroups.Count();
		int budget = Math.Max(4, Math.Ceil(1.0 * n / 3.0));
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
		if (!grp || !u.m_Perception || !DCO_Util.IsAlive(grp.GetLeaderEntity()) || DCO_Util.IsPlayerGroup(grp))
			return;

		DCO_GroupUtilityComponent dg = DCO_GroupUtilityComponent.Cast(grp.FindComponent(DCO_GroupUtilityComponent));
		float pmNow = pm.GetTime();
		if (u.m_mDCOInjected.Count() > 32)
			PruneInjected(u, pmNow);
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
			if (t.m_eCategory == EAITargetInfoCategory.IDENTIFIED)
				Classify(cl, t.m_Entity);
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
			else if (newest[i] > track.m_fInfoTs + 1.0)
			{
				vector vel = (c.m_vCenter - track.m_vPos) * (1.0 / (newest[i] - track.m_fInfoTs));
				vel[1] = 0;
				if (vel.Length() >= 0.7)
					c.m_vVelocity = vel;
			}
			track.m_vPos = c.m_vCenter;
			track.m_fSentAt = now;
			track.m_fInfoTs = newest[i];
			track.m_fLastMatched = now;

			c.m_iHop = 1;
			c.m_Origin = grp;
			c.m_bAmbush = dg && dg.m_fDCOAmbushAt >= 0 && now - dg.m_fDCOAmbushAt < 60.0;
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

		array<SCR_AIGroupUtilityComponent> recv = {};
		array<float> dists = {};
		foreach (SCR_AIGroupUtilityComponent r : m_aGroups)
		{
			if (!r || r == senderUtil || !r.m_Owner || r.m_Owner == c.m_Origin || !r.m_Perception)
				continue;
			if (DCO_Util.IsPlayerGroup(r.m_Owner) || !DCO_Util.IsAlive(r.m_Owner.GetLeaderEntity()))
				continue;

			Faction rf = r.m_Owner.GetFaction();
			if (!rf || !senderFaction.IsFactionFriendly(rf))
				continue;

			float d = vector.Distance(senderPos, r.m_Owner.GetLeaderEntity().GetOrigin());
			if (d > radio && d > voice)
				continue;

			int at = 0;
			while (at < dists.Count() && dists[at] <= d)
				at++;
			recv.InsertAt(r, at);
			dists.InsertAt(d, at);
		}

		foreach (int ri, SCR_AIGroupUtilityComponent rr : recv)
		{
			bool useRadio = false;
			if (dists[ri] > voice)
			{
				if (!senderRadio || (needRadio && !GroupHasRadio(rr.m_Owner)))
					continue;
				useRadio = true;
			}

			if (AlreadyKnows(rr, c))
				continue;

			DCO_SharedContact k = c.CopyFor(now + Delay(useRadio), useRadio, Noise(cfg, spotDist, useRadio, c.m_iHop));
			k.m_Receiver = rr;
			if (!Enqueue(k))
				break;
			Reserve(rr, c, pmNow);
		}

		if (cfg.GetShareToPlayers())
			SharePlayers(cfg, c, sender, senderFaction, senderPos, spotDist, senderRadio, now);
	}

	static void ShareEstimate(SCR_AIGroup sender, vector pos, float radius, int count, int hop, bool heard = false)
	{
		if (!s_Instance || !sender)
			return;
		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetInstance();
		if (!cfg || !cfg.GetShareEnabled())
			return;
		if (hop + 1 > cfg.GetShareHops())
		{
			DCO_Perf.Count("fire_est:share_hop_limit");
			return;
		}
		Faction sf = sender.GetFaction();
		IEntity sl = sender.GetLeaderEntity();
		if (!sf || !DCO_Util.IsAlive(sl))
			return;
		vector sp = sl.GetOrigin();
		float voice = cfg.GetShareVoiceRange();
		float radioR = cfg.GetShareRadioRange();
		bool needRadio = cfg.GetShareRequireRadio();
		bool senderRadio = !needRadio || GroupHasRadio(sender);
		float spotDist = vector.Distance(sp, pos);
		float now = DCO_Util.NowS();
		int sent;
		foreach (SCR_AIGroupUtilityComponent r : s_Instance.m_aGroups)
		{
			if (!r || !r.m_Owner || r.m_Owner == sender || DCO_Util.IsPlayerGroup(r.m_Owner) || !DCO_Util.IsAlive(r.m_Owner.GetLeaderEntity()))
				continue;
			Faction rf = r.m_Owner.GetFaction();
			if (!rf || !sf.IsFactionFriendly(rf))
				continue;
			float d = vector.Distance(sp, r.m_Owner.GetLeaderEntity().GetOrigin());
			bool useRadio = false;
			if (d > voice)
			{
				if (d > radioR || !senderRadio || (needRadio && !GroupHasRadio(r.m_Owner)))
					continue;
				useRadio = true;
			}
			if (s_Instance.m_aEstPending.Count() >= 64)
			{
				DCO_Perf.Count("fire_est:share_queue_full");
				break;
			}
			vector off = Noise(cfg, spotDist, useRadio, hop + 1);
			DCO_SharedEst k = new DCO_SharedEst();
			k.m_Receiver = r.m_Owner;
			k.m_Sender = sender;
			k.m_vPos = DCO_Util.Surface(pos + off);
			k.m_fRadius = radius + off.Length();
			k.m_iCount = count;
			k.m_iHop = hop + 1;
			k.m_bRadio = useRadio;
			k.m_fDeliverAt = now + Delay(useRadio);
			k.m_bHeard = heard;
			s_Instance.m_aEstPending.Insert(k);
			sent++;
		}
		DCO_Perf.Count("fire_est:share");
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdGroup(sender), string.Format("fire_est_share grp=%1 est=%2 radius=%3 reports=%4 hop=%5 receivers=%6 sender_radio=%7 heard=%8", DCO_Log.IdGroup(sender), DCO_Log.Pos(pos), Math.Round(radius), count, hop + 1, sent, DCO_Util.B01(senderRadio), DCO_Util.B01(heard)));
	}

	protected void DeliverEst(float now)
	{
		if (m_aEstPending.IsEmpty())
			return;
		float nowMs = GetGame().GetWorld().GetWorldTime();
		for (int i = m_aEstPending.Count() - 1; i >= 0; i--)
		{
			DCO_SharedEst k = m_aEstPending[i];
			if (now < k.m_fDeliverAt)
				continue;
			m_aEstPending.Remove(i);
			if (!k.m_Receiver || !DCO_Util.IsAlive(k.m_Receiver.GetLeaderEntity()))
				continue;
			if (k.m_bHeard)
				DCO_HeardEst.Receive(k.m_Receiver, k.m_vPos, k.m_fRadius, k.m_iHop, k.m_Sender, k.m_bRadio, nowMs);
			else
				DCO_FireEstimate.Receive(k.m_Receiver, k.m_vPos, k.m_fRadius, k.m_iCount, k.m_iHop, k.m_Sender, k.m_bRadio, nowMs);
		}
	}

	protected void SharePlayers(DCO_GlobalAIComponent cfg, DCO_SharedContact c, SCR_AIGroup sender, Faction senderFaction, vector senderPos, float spotDist, bool senderRadio, float now)
	{
		if (m_mPlayerLast.Count() > 64)
			PrunePlayerLast(now);

		array<int> players = {};
		GetGame().GetPlayerManager().GetPlayers(players);
		array<int> senderPlayers = sender.GetPlayerIDs();

		foreach (int pid : players)
		{
			if (senderPlayers && senderPlayers.Contains(pid))
				continue;

			IEntity ent = GetGame().GetPlayerManager().GetPlayerControlledEntity(pid);
			if (!DCO_Util.IsAlive(ent))
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

			int cx = Math.Floor(c.m_vCenter[0] / 250.0);
			int cz = Math.Floor(c.m_vCenter[2] / 250.0);
			int key = pid * 4096 + (cx & 63) * 64 + (cz & 63);
			float last = 0;
			if (m_mPlayerLast.Find(key, last) && now - last < 30.0)
				continue;
			m_mPlayerLast.Set(key, now);

			DCO_SharedContact k = c.CopyFor(now + Delay(useRadio), useRadio, Noise(cfg, spotDist, useRadio, c.m_iHop));
			k.m_iPlayerID = pid;
			if (!Enqueue(k))
				break;
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
				pos = DCO_Util.Surface(pos);
				r.m_Perception.AddOrUpdateGunshot(ent, pos, c.m_aFactions[t], pmNow, false);
				r.m_mDCOInjected.Set(ent, pmNow);

				if (cfg.GetShareMapMarkers())
					MarkOnMap(r, ent);
			}

			int comp = 0;
			if (c.m_iMG > 0)
				comp |= 1;
			if (c.m_iAT > 0)
				comp |= 2;
			if (c.m_iSniper > 0)
				comp |= 4;
			if (c.m_bVehicle)
				comp |= 8;
			if (c.m_bArmor)
				comp |= 16;
			r.m_iDCOSharedComp = comp;
			r.m_vDCOSharedPos = c.m_vCenter + c.m_vOffset;
			r.m_vDCOSharedVel = c.m_vVelocity;
			r.m_fDCOSharedAt = now;

			if (cfg.GetShareDebug())
				DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("%1 -> %2 hop %3 %4 (%5 enemies, comp %6, speed %7, noise %8 m)", DCO_PlayerComms.GetCallsign(c.m_Sender),
					DCO_PlayerComms.GetCallsign(r.m_Owner), c.m_iHop, Channel(c.m_bRadio), c.m_iCount, comp, Math.Round(c.m_vVelocity.Length()), Math.Round(c.m_vOffset.Length())));

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
		if (!DCO_Util.IsAlive(ent) || !comp)
			return;

		vector pos = c.m_vCenter + c.m_vOffset;
		SCR_AIGroup grp = c.m_Sender;
		string extra;
		string list = CompList(c);
		if (!list.IsEmpty())
			extra = " " + DCO_Radio.Text("share_comp", grp, DCO_Radio.P("comp", list));
		if (c.m_bAmbush)
			extra += " " + DCO_Radio.Text("share_ambush", grp, null);
		if (c.m_vVelocity != vector.Zero)
			extra += " " + DCO_Radio.Text("share_moving", grp, DCO_Radio.P("dir", DCO_Radio.Dir(pos, pos + c.m_vVelocity * 100)));

		string text;
		if (c.m_bRadio)
			text = DCO_Radio.Text("share_radio", grp, DCO_Radio.P("what", What(c), "grid", DCO_PlayerComms.Grid(pos), "extra", extra));
		else
		{
			int dist = Math.Max(50, Math.Round(vector.DistanceXZ(ent.GetOrigin(), pos) / 50) * 50);
			text = DCO_Radio.Text("share_voice", grp, DCO_Radio.P("what", What(c), "dist", dist.ToString(), "dir", DCO_Radio.Dir(ent.GetOrigin(), pos), "extra", extra));
		}

		comp.DCO_SendContact(DCO_PlayerComms.GetCallsign(c.m_Sender), text, c.m_bRadio);
	}

	protected static string What(DCO_SharedContact c)
	{
		if (c.m_bArmor)
			return "@share_what_armor";
		if (c.m_bVehicle)
			return "@share_what_vehicle";
		if (c.m_iCount <= 3)
			return "@share_what_team";
		if (c.m_iCount <= 9)
			return "@share_what_squad";
		return "@share_what_platoon";
	}

	protected static string CompList(DCO_SharedContact c)
	{
		string list;
		if (c.m_iMG > 0)
			list = "@comp_mg";
		if (c.m_iAT > 0)
			list = DCO_Radio.Join(list, "@comp_at");
		if (c.m_iSniper > 0)
			list = DCO_Radio.Join(list, "@comp_sniper");
		return list;
	}

	protected static void Classify(DCO_SharedContact cl, IEntity ent)
	{
		IEntity veh = DCO_VehicleCombat.GetVehicle(ent);
		if (!veh && Vehicle.Cast(ent))
			veh = ent;
		if (veh)
		{
			cl.m_bVehicle = true;
			if (DCO_VehicleCombat.IsArmored(veh))
				cl.m_bArmor = true;
		}
		else if (DCO_Strength.HasWeaponInHands(ent, EWeaponType.WT_ROCKETLAUNCHER))
			cl.m_iAT++;
		else if (DCO_Strength.HasWeaponInHands(ent, EWeaponType.WT_MACHINEGUN))
			cl.m_iMG++;
		else if (DCO_Strength.HasWeaponInHands(ent, EWeaponType.WT_SNIPERRIFLE))
			cl.m_iSniper++;
	}

	protected bool Enqueue(DCO_SharedContact k)
	{
		if (m_aPending.Count() < 128)
		{
			m_aPending.Insert(k);
			return true;
		}
		if (m_aDeferred.Count() >= 64)
			return false;
		m_aDeferred.Insert(k);
		return true;
	}

	protected void PromoteDeferred(float now)
	{
		for (int i = 0; i < m_aDeferred.Count(); i++)
		{
			DCO_SharedContact k = m_aDeferred[i];
			if (m_aPending.Count() < 128)
			{
				k.m_fDeliverAt = Math.Max(k.m_fDeliverAt, now);
				m_aPending.Insert(k);
				m_aDeferred.Remove(i);
				i--;
				continue;
			}
			k.m_iDeferTicks++;
			if (k.m_iDeferTicks > 3)
			{
				m_aDeferred.Remove(i);
				i--;
			}
		}
	}

	protected static void PruneInjected(SCR_AIGroupUtilityComponent u, float pmNow)
	{
		array<IEntity> old = {};
		foreach (IEntity e, float ts : u.m_mDCOInjected)
		{
			if (!e || pmNow - ts > 60.0)
				old.Insert(e);
		}
		foreach (IEntity oe : old)
			u.m_mDCOInjected.Remove(oe);
	}

	protected void PrunePlayerLast(float now)
	{
		array<int> old = {};
		foreach (int key, float ts : m_mPlayerLast)
		{
			if (now - ts >= 30.0)
				old.Insert(key);
		}
		foreach (int ok : old)
			m_mPlayerLast.Remove(ok);
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
			if (a && DCO_Util.HasManpackRadio(a.GetControlledEntity()))
				return true;
		}
		return false;
	}
}
