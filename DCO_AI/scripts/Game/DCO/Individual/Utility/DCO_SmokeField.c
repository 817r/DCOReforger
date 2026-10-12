class DCO_SmokeCloud
{
	int m_iId;
	IEntity m_Ent;
	vector m_vPos;
	float m_fStart_ms;
	float m_fEnd_ms;
	string m_sKind;
	bool m_bActiveLogged;
}

class DCO_SmokeField
{
	protected static ref array<ref DCO_SmokeCloud> s_aClouds;
	protected static bool s_bTicking;
	protected static int s_iNextId;

	static bool Any()
	{
		return s_aClouds && !s_aClouds.IsEmpty();
	}

	static void Add(IEntity ent, vector pos, string kind, float delay_s, float life_s)
	{
		if (!Replication.IsServer())
			return;
		if (!s_aClouds)
			s_aClouds = {};
		float now = GetGame().GetWorld().GetWorldTime();
		foreach (DCO_SmokeCloud old : s_aClouds)
		{
			if (vector.DistanceXZ(old.m_vPos, pos) <= 8.0 && Math.AbsFloat(old.m_fStart_ms - (now + delay_s * 1000.0)) <= 6000.0)
			{
				if (ent && !old.m_Ent)
					old.m_Ent = ent;
				DCO_Perf.Count("smoke:dedupe");
				return;
			}
		}
		DCO_SmokeCloud c = new DCO_SmokeCloud();
		s_iNextId++;
		c.m_iId = s_iNextId;
		c.m_Ent = ent;
		c.m_vPos = pos;
		c.m_fStart_ms = now + delay_s * 1000.0;
		c.m_fEnd_ms = c.m_fStart_ms + life_s * 1000.0;
		c.m_sKind = kind;
		s_aClouds.Insert(c);
		DCO_Perf.Count("smoke:tracked_" + kind);
		if (DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG))
			DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG, "-", string.Format("smoke_tracked id=%1 kind=%2 pos=%3 delay_s=%4 life_s=%5", c.m_iId, kind, DCO_Log.Pos(pos), delay_s, life_s));
		if (!s_bTicking)
		{
			s_bTicking = true;
			GetGame().GetCallqueue().CallLater(Update, 1000, true);
		}
	}

	static void TrackFast(IEntity ent, int ticks)
	{
		if (!s_aClouds || ticks > 150)
			return;
		DCO_SmokeCloud c;
		foreach (DCO_SmokeCloud x : s_aClouds)
		{
			if (x.m_Ent && x.m_Ent == ent)
				c = x;
		}
		if (!c)
		{
			DCO_Perf.Count("smoke:fast_track_end");
			return;
		}
		c.m_vPos = ent.GetOrigin();
		Physics ph = ent.GetPhysics();
		if (ticks > 5 && ph && ph.GetVelocity().Length() < 0.5)
		{
			DCO_Perf.Count("smoke:fast_track_rest");
			if (DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO))
				DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, "-", string.Format("smoke_landed id=%1 kind=%2 pos=%3 flight_s=%4", c.m_iId, c.m_sKind, DCO_Log.Pos(c.m_vPos), (1.0 * ticks * 0.1).ToString(1, 1)));
			return;
		}
		GetGame().GetCallqueue().CallLater(TrackFast, 100, false, ent, ticks + 1);
	}

	static void Update()
	{
		float now = GetGame().GetWorld().GetWorldTime();
		for (int i = s_aClouds.Count() - 1; i >= 0; i--)
		{
			DCO_SmokeCloud c = s_aClouds[i];
			if (c.m_Ent)
				c.m_vPos = c.m_Ent.GetOrigin();
			if (now > c.m_fEnd_ms)
			{
				DCO_Perf.Count("smoke:cloud_end");
				if (DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO))
					DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, "-", string.Format("smoke_cloud_end id=%1 kind=%2 pos=%3", c.m_iId, c.m_sKind, DCO_Log.Pos(c.m_vPos)));
				s_aClouds.Remove(i);
				continue;
			}
			if (!c.m_bActiveLogged && now >= c.m_fStart_ms)
			{
				c.m_bActiveLogged = true;
				DCO_Perf.Count("smoke:cloud");
				if (DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO))
					DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, "-", string.Format("smoke_cloud id=%1 kind=%2 pos=%3 life_s=%4", c.m_iId, c.m_sKind, DCO_Log.Pos(c.m_vPos), Math.Round((c.m_fEnd_ms - now) / 1000.0)));
			}
		}
		if (s_aClouds.IsEmpty())
		{
			GetGame().GetCallqueue().Remove(Update);
			s_bTicking = false;
		}
	}

	static float Radius(DCO_SmokeCloud c, float now)
	{
		float t = (now - c.m_fStart_ms) / 1000.0;
		if (t < 0)
			return 0;
		float r = Math.Min(2.0 + t * 0.8, 8.0);
		float left = (c.m_fEnd_ms - now) / 1000.0;
		if (left < 10.0)
			r = r * Math.Max(left, 0) / 10.0;
		return r;
	}

	static vector Center(DCO_SmokeCloud c)
	{
		return c.m_vPos + vector.Up * 1.5;
	}

	static DCO_SmokeCloud OnLine(vector a, vector b)
	{
		if (!Any())
			return null;
		float now = GetGame().GetWorld().GetWorldTime();
		vector ab = b - a;
		float len2 = ab.LengthSq();
		foreach (DCO_SmokeCloud c : s_aClouds)
		{
			float r = Radius(c, now);
			if (r < 1.5)
				continue;
			vector p = Center(c);
			float t = 0;
			if (len2 > 0.0001)
				t = Math.Clamp(vector.Dot(p - a, ab) / len2, 0.0, 1.0);
			if (vector.Distance(a + ab * t, p) <= r)
				return c;
		}
		return null;
	}

	static DCO_SmokeCloud Inside(vector p, float scale)
	{
		if (!Any())
			return null;
		float now = GetGame().GetWorld().GetWorldTime();
		foreach (DCO_SmokeCloud c : s_aClouds)
		{
			float r = Radius(c, now);
			if (r >= 2.0 && vector.DistanceXZ(p, c.m_vPos) <= r * scale)
				return c;
		}
		return null;
	}

	static void GetActive(notnull array<DCO_SmokeCloud> outClouds, float minRadius)
	{
		outClouds.Clear();
		if (!Any())
			return;
		float now = GetGame().GetWorld().GetWorldTime();
		foreach (DCO_SmokeCloud c : s_aClouds)
		{
			if (Radius(c, now) >= minRadius)
				outClouds.Insert(c);
		}
	}

	static float AimFactor(vector eye, vector aim)
	{
		if (!Any() || !OnLine(eye, aim))
			return 1.0;
		DCO_Perf.Count("smoke:aim_degraded");
		return 4.0;
	}
}
