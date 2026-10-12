class CMD_FrontlineReconTrack
{
	DCO_GroupUtilityComponent m_Squad;
	float m_fExpireTime;
}

class DCO_FrontlineSegment
{
	vector m_vStart;
	vector m_vEnd;

	vector m_vFacing;

	float m_fPressure;

	CMD_AICommanderObjectiveComponent m_Owned;
	CMD_AICommanderObjectiveComponent m_Threat;

	vector Center()
	{
		return (m_vStart + m_vEnd) * 0.5;
	}

	vector NearestPointTo(vector p)
	{
		vector ab = m_vEnd - m_vStart;
		float  len2 = ab.LengthSq();

		if (len2 < 0.001)
			return m_vStart;

		vector ap = p - m_vStart;
		float  t  = Math.Clamp(vector.Dot(ap, ab) / len2, 0.0, 1.0);

		return m_vStart + (ab * t);
	}
}

class DCO_CommanderFrontline : Managed
{
	protected ref array<ref DCO_FrontlineSegment> m_aSegments = new array<ref DCO_FrontlineSegment>();
	protected ref array<ref CMD_FrontlineReconTrack> m_aReconTracks = new array<ref CMD_FrontlineReconTrack>();
	protected string m_sReason = "not built yet";

	protected float m_fAutoRadiusShare = 0.4;
	protected float m_fAutoRadiusThreatGap = 100.0;
	protected int m_iEnvelopeMinNodes = 3;
	protected float m_fPressureDistScale = 1000.0;
	protected float m_fReconOverlapPenalty = 0.25;

	protected float m_fDebugLabelHeight = 52.0;
	protected float m_fDebugDotSpacing = 12.0;
	protected float m_fDebugFacingArrow = 35.0;

	array<ref DCO_FrontlineSegment> GetSegments()	{ return m_aSegments; }
	string GetReason()								{ return m_sReason; }

	void Build(AICommander_BaseComponent cmd)
	{
		m_aSegments.Clear();

		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
			return;

		array<CMD_AICommanderObjectiveComponent> own   = {};
		array<CMD_AICommanderObjectiveComponent> hostile = {};

		foreach (CMD_AICommanderObjectiveComponent obj : mgr.m_aObjective)
		{
			if (!obj || !obj.GetOwner())
				continue;

			if (obj.IsCapturedBy(cmd.GetCommanderFactionKey(), cmd.GetCommanderUID()))
				own.Insert(obj);
			else
				hostile.Insert(obj);
		}

		if (hostile.IsEmpty())
		{
			m_sReason = "no hostile objectives";
			return;
		}

		if (own.IsEmpty())
		{
			vector groupCentroid;
			if (!TryGetOwnGroupCentroid(cmd, groupCentroid))
			{
				m_sReason = "no territory and no groups";
				return;
			}

			BuildEnvelopeAt(cmd, groupCentroid, 0.0, null, hostile);

			if (m_aSegments.IsEmpty())
				m_sReason = "envelope failed from group centroid";
			else
				m_sReason = "from group centroid (no objective held yet)";

			return;
		}

		if (own.Count() == 1)
		{
			BuildEnvelopeAt(cmd, own[0].GetOwner().GetOrigin(), own[0].GetRadius(), own[0], hostile);

			if (m_aSegments.IsEmpty())
				m_sReason = "envelope failed";
			else
				m_sReason = "envelope around single held objective";

			return;
		}

		BuildPaired(cmd, own, hostile);

		if (m_aSegments.IsEmpty())
			m_sReason = "paired build produced no segments";
		else
			m_sReason = "paired from held objectives";
	}

	protected bool TryGetOwnGroupCentroid(AICommander_BaseComponent cmd, out vector centroid)
	{
		vector sum = vector.Zero;
		int    n   = 0;

		foreach (DCO_GroupUtilityComponent grp : cmd.GetOwnedGroups())
		{
			if (!grp || !grp.GetOwner() || grp.IsPlayerGroup() || grp.IsDedicatedTransport())
				continue;

			sum = sum + grp.GetOwner().GetOrigin();
			n   = n + 1;
		}

		if (n <= 0)
			return false;

		centroid    = sum / n;
		centroid = DCO_Util.Surface(centroid);

		return true;
	}

	protected void BuildPaired(
		AICommander_BaseComponent cmd,
		notnull array<CMD_AICommanderObjectiveComponent> own,
		notnull array<CMD_AICommanderObjectiveComponent> hostile)
	{
		vector centroid = vector.Zero;
		foreach (CMD_AICommanderObjectiveComponent o : own)
			centroid = centroid + o.GetOwner().GetOrigin();

		centroid = centroid / own.Count();

		array<ref DCO_FrontlineSegment> nodes = {};

		foreach (CMD_AICommanderObjectiveComponent o : own)
		{
			vector oPos = o.GetOwner().GetOrigin();

			CMD_AICommanderObjectiveComponent nearest = null;
			float nearestSq = float.MAX;

			foreach (CMD_AICommanderObjectiveComponent h : hostile)
			{
				float dSq = vector.DistanceSq(oPos, h.GetOwner().GetOrigin());
				if (dSq < nearestSq)
				{
					nearestSq = dSq;
					nearest   = h;
				}
			}

			if (!nearest)
				continue;

			vector hPos = nearest.GetOwner().GetOrigin();
			vector node = oPos + ((hPos - oPos) * cmd.DCO_GT_FrontlineNodeBias());
			node[1]     = GetGame().GetWorld().GetSurfaceY(node[0], node[2]);

			DCO_FrontlineSegment seg = new DCO_FrontlineSegment();
			seg.m_vStart   = node;
			seg.m_Owned    = o;
			seg.m_Threat   = nearest;
			seg.m_fPressure = SegmentPressure(cmd, nearest, node);

			nodes.Insert(seg);
		}

		if (nodes.Count() < 2)
			return;

		SortNodesByAngle(nodes, centroid);

		for (int i = 0; i < nodes.Count() - 1; i++)
		{
			DCO_FrontlineSegment seg = new DCO_FrontlineSegment();
			seg.m_vStart    = nodes[i].m_vStart;
			seg.m_vEnd      = nodes[i + 1].m_vStart;
			seg.m_Owned     = nodes[i].m_Owned;
			seg.m_Threat    = nodes[i].m_Threat;
			seg.m_fPressure = (nodes[i].m_fPressure + nodes[i + 1].m_fPressure) * 0.5;
			seg.m_vFacing   = FacingAway(seg.Center(), centroid);

			m_aSegments.Insert(seg);
		}
	}

	protected void BuildEnvelopeAt(
		AICommander_BaseComponent cmd,
		vector cPos,
		float anchorRadius,
		CMD_AICommanderObjectiveComponent anchorObj,
		notnull array<CMD_AICommanderObjectiveComponent> hostile)
	{
		CMD_AICommanderObjectiveComponent nearest = null;
		float nearestSq = float.MAX;

		foreach (CMD_AICommanderObjectiveComponent h : hostile)
		{
			float dSq = vector.DistanceSq(cPos, h.GetOwner().GetOrigin());
			if (dSq < nearestSq)
			{
				nearestSq = dSq;
				nearest   = h;
			}
		}

		if (!nearest)
			return;

		vector facing = nearest.GetOwner().GetOrigin() - cPos;
		facing        = Vector(facing[0], 0.0, facing[2]);

		if (facing.LengthSq() < 1.0)
			return;

		facing = facing.Normalized();

		float baseAngle = Math.Atan2(facing[2], facing[0]) * Math.RAD2DEG;

		float minRadius = cmd.DCO_GT_EnvelopeMinRadius();
		float radius;
		if (anchorRadius > 0.0)
		{
			radius = Math.Max(anchorRadius * cmd.DCO_GT_EnvelopeRadiusMul(), minRadius);
		}
		else
		{
			float toThreat = vector.Distance(cPos, nearest.GetOwner().GetOrigin());
			radius = Math.Clamp(toThreat * m_fAutoRadiusShare, minRadius, Math.Max(toThreat - m_fAutoRadiusThreatGap, minRadius));
		}

		int nodeCount = Math.Max(m_iEnvelopeMinNodes, cmd.DCO_GT_EnvelopeNodes());
		float arcDeg = cmd.DCO_GT_EnvelopeArcDeg();

		array<vector> arc = {};

		for (int i = 0; i < nodeCount; i++)
		{
			float t        = i / (float)(nodeCount - 1);
			float angleDeg = baseAngle - (arcDeg * 0.5) + (arcDeg * t);
			float angleRad = angleDeg * Math.DEG2RAD;

			vector p = cPos + Vector(Math.Cos(angleRad) * radius, 0.0, Math.Sin(angleRad) * radius);
			p[1]     = GetGame().GetWorld().GetSurfaceY(p[0], p[2]);

			arc.Insert(p);
		}

		float pressure = SegmentPressure(cmd, nearest, cPos);

		for (int s = 0; s < arc.Count() - 1; s++)
		{
			DCO_FrontlineSegment seg = new DCO_FrontlineSegment();
			seg.m_vStart    = arc[s];
			seg.m_vEnd      = arc[s + 1];
			seg.m_Owned     = anchorObj;
			seg.m_Threat    = nearest;
			seg.m_fPressure = pressure;
			seg.m_vFacing   = FacingAway(seg.Center(), cPos);

			m_aSegments.Insert(seg);
		}
	}

	protected vector FacingAway(vector segCenter, vector rearRef)
	{
		vector away = segCenter - rearRef;
		away        = Vector(away[0], 0.0, away[2]);

		if (away.LengthSq() < 1.0)
			return Vector(1.0, 0.0, 0.0);

		return away.Normalized();
	}

	protected float SegmentPressure(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent threat, vector node)
	{
		if (!threat || !threat.GetOwner())
			return 0.0;

		int segFriendly;
		int segEnemies;
		threat.CountNearbyUnitsCached(threat.GetRadius(), cmd.GetCommanderFactionKey(), segFriendly, segEnemies);
		float enemies = segEnemies;
		float dist    = Math.Max(vector.Distance(node, threat.GetOwner().GetOrigin()), 1.0);

		return enemies * (m_fPressureDistScale / dist);
	}

	protected void SortNodesByAngle(notnull array<ref DCO_FrontlineSegment> nodes, vector centroid)
	{
		for (int i = 1; i < nodes.Count(); i++)
		{
			ref DCO_FrontlineSegment key = nodes[i];
			float keyAngle = AngleAround(key.m_vStart, centroid);

			int j = i - 1;
			while (j >= 0 && AngleAround(nodes[j].m_vStart, centroid) > keyAngle)
			{
				nodes.Set(j + 1, nodes[j]);
				j = j - 1;
			}

			nodes.Set(j + 1, key);
		}
	}

	protected float AngleAround(vector p, vector centroid)
	{
		return Math.Atan2(p[2] - centroid[2], p[0] - centroid[0]);
	}

	bool TryGetHottestPoint(out vector frontlinePos)
	{
		if (m_aSegments.IsEmpty())
			return false;

		DCO_FrontlineSegment best = null;
		float bestPressure = -1.0;

		foreach (DCO_FrontlineSegment seg : m_aSegments)
		{
			if (seg.m_fPressure > bestPressure)
			{
				bestPressure = seg.m_fPressure;
				best         = seg;
			}
		}

		if (!best)
			return false;

		frontlinePos = best.Center();
		return true;
	}

	bool GetNearestPoint(vector from, out vector pos, out vector facing)
	{
		if (m_aSegments.IsEmpty())
			return false;

		float  bestSq = float.MAX;
		bool   found  = false;

		foreach (DCO_FrontlineSegment seg : m_aSegments)
		{
			vector p    = seg.NearestPointTo(from);
			float  dSq  = vector.DistanceSq(from, p);

			if (dSq < bestSq)
			{
				bestSq = dSq;
				pos    = p;
				facing = seg.m_vFacing;
				found  = true;
			}
		}

		return found;
	}

	protected bool PickReconSegment(AICommander_BaseComponent cmd, out vector pos)
	{
		if (m_aSegments.IsEmpty())
			return false;

		float reconRadius = cmd.DCO_GT_FrontlineReconRadius();
		DCO_FrontlineSegment best = null;
		float bestScore = -1.0;

		foreach (DCO_FrontlineSegment seg : m_aSegments)
		{
			if (seg.m_Threat && cmd.DCO_AssignSkip(seg.m_Threat, "recon_front"))
				continue;

			vector c = seg.Center();

			float score = seg.m_fPressure + 1.0;

			foreach (CMD_FrontlineReconTrack t : m_aReconTracks)
			{
				if (!t || !t.m_Squad || !t.m_Squad.GetOwner())
					continue;

				if (vector.DistanceSq(t.m_Squad.GetOwner().GetOrigin(), c) < reconRadius * reconRadius)
					score = score * m_fReconOverlapPenalty;
			}

			if (score > bestScore)
			{
				bestScore = score;
				best      = seg;
			}
		}

		if (!best)
			return false;

		pos = best.Center();
		return true;
	}

	void TrySendRecon(AICommander_BaseComponent cmd, float worldTime)
	{
		int activeRecon = 0;
		foreach (CMD_FrontlineReconTrack t : m_aReconTracks)
		{
			if (t && t.m_Squad)
				activeRecon++;
		}
		if (activeRecon >= DCO_Force.Max(cmd, "recon_front", cmd.GetMaxFrontlineRecon()))
			return;

		vector frontlinePos;
		if (!PickReconSegment(cmd, frontlinePos))
			return;

		DCO_GroupUtilityComponent reconGrp = cmd.FindBestIdleGroupForTask_Public(DCO_EGroupTask.RECON, frontlinePos, -1, false, 0);
		if (!reconGrp)
			return;

		if (reconGrp.IsPlayerGroup())
			return;

		if (!cmd.CanCommitGroup(reconGrp))
			return;

		reconGrp.CompleteAllWaypoints();
		reconGrp.SetTask(DCO_EGroupTask.RECON, "AICommanderBase:3661");
		cmd.GetPatrol().GenerateRoute(cmd, reconGrp, frontlinePos, cmd.DCO_GT_FrontlineReconRadius(), worldTime);

		CMD_FrontlineReconTrack track = new CMD_FrontlineReconTrack();
		track.m_Squad = reconGrp;
		track.m_fExpireTime = worldTime + cmd.DCO_GT_FrontlineReconDuration();
		m_aReconTracks.Insert(track);

		if (cmd.IsDebugMode())
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(cmd), string.Format("cmd=%1 Frontline recon: %2 -> scouting near %3",
				cmd.GetCommanderUID(), reconGrp.GetOwner().GetName(), frontlinePos.ToString()));
	}

	void UpdateReconTracks(AICommander_BaseComponent cmd, float worldTime)
	{
		if (!Replication.IsServer())
			return;

		for (int i = m_aReconTracks.Count() - 1; i >= 0; i--)
		{
			CMD_FrontlineReconTrack track = m_aReconTracks[i];
			if (!track || !track.m_Squad)
			{
				m_aReconTracks.Remove(i);
				continue;
			}

			if (worldTime >= track.m_fExpireTime)
			{
				if (track.m_Squad.GetTask() == DCO_EGroupTask.RECON)
				{
					track.m_Squad.CompleteAllWaypoints();
					track.m_Squad.SetTask(DCO_EGroupTask.NONE, "AICommanderBase:3693");
					if (cmd.IsDebugMode())
						DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(cmd), string.Format("cmd=%1 Frontline recon done -- %2 released back to RESERVE",
							cmd.GetCommanderUID(), track.m_Squad.GetOwner().GetName()));
				}
				m_aReconTracks.Remove(i);
			}
		}
	}

	bool IsReconGroup(DCO_GroupUtilityComponent grp)
	{
		if (!grp)
			return false;

		foreach (CMD_FrontlineReconTrack t : m_aReconTracks)
		{
			if (t && t.m_Squad == grp)
				return true;
		}

		return false;
	}

	void ForgetGroup(DCO_GroupUtilityComponent grp)
	{
		for (int i = m_aReconTracks.Count() - 1; i >= 0; i--)
		{
			CMD_FrontlineReconTrack track = m_aReconTracks[i];
			if (track && track.m_Squad == grp)
				m_aReconTracks.Remove(i);
		}
	}

	void DrawDebug(notnull array<ref Shape> shapes, notnull array<ref DebugTextWorldSpace> texts, vector cmdPos, int flags)
	{
		vector labelPos = Vector(cmdPos[0], cmdPos[1] + m_fDebugLabelHeight, cmdPos[2]);
		if (m_aSegments.IsEmpty())
		{
			texts.Insert(DCO_DebugDraw.SpawnText(labelPos, "FRONTLINE: none\n" + m_sReason, 17.0, DCO_DebugDraw.COLOR_FRONTLINE()));
			return;
		}

		texts.Insert(DCO_DebugDraw.SpawnText(labelPos, string.Format("FRONTLINE: %1 segments\n%2", m_aSegments.Count(), m_sReason), 17.0, DCO_DebugDraw.COLOR_FRONTLINE()));

		for (int i = 0; i < m_aSegments.Count(); i++)
		{
			DCO_FrontlineSegment seg = m_aSegments.Get(i);
			if (!seg)
				continue;

			float segLen = vector.Distance(seg.m_vStart, seg.m_vEnd);
			int   dots   = Math.Max(2, (int)Math.Round(segLen / m_fDebugDotSpacing));

			for (int d = 0; d <= dots; d++)
			{
				float t = d / (float)dots;
				vector p = seg.m_vStart + ((seg.m_vEnd - seg.m_vStart) * t);
				p[1]     = GetGame().GetWorld().GetSurfaceY(p[0], p[2]) + 1.0;

				shapes.Insert(Shape.CreateSphere(DCO_DebugDraw.COLOR_FRONTLINE(), flags, p, 0.6));
			}

			vector c = seg.Center();
			c[1]     = GetGame().GetWorld().GetSurfaceY(c[0], c[2]) + 2.0;

			shapes.Insert(Shape.CreateArrow(c, c + (seg.m_vFacing * m_fDebugFacingArrow), 3.0, DCO_DebugDraw.COLOR_FRONTLINE(), flags));

			string own  = "?";
			string threat = "?";

			if (seg.m_Owned && seg.m_Owned.GetOwner())
				own = seg.m_Owned.GetDisplayName();

			if (seg.m_Threat && seg.m_Threat.GetOwner())
				threat = seg.m_Threat.GetDisplayName();

			texts.Insert(DCO_DebugDraw.SpawnText(
				Vector(c[0], c[1] + 8.0, c[2]),
				string.Format("FRONTLINE %1/%2\npressure %3\n%4 -> %5",
					i + 1,
					m_aSegments.Count(),
					DCO_DebugDraw.F1(seg.m_fPressure),
					own,
					threat),
				15.0, DCO_DebugDraw.COLOR_FRONTLINE()));
		}
	}
}
