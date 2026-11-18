class DCO_PlayerProx
{
	protected static ref array<vector> s_aPos;
	protected static float s_fAt_ms;
	protected static bool s_bFresh;

	static void Refresh(float now)
	{
		if (!s_aPos)
			s_aPos = {};
		if (s_bFresh && now - s_fAt_ms < 2000.0 && now >= s_fAt_ms)
			return;
		s_bFresh = true;
		s_fAt_ms = now;
		s_aPos.Clear();
		PlayerManager pm = GetGame().GetPlayerManager();
		if (!pm)
			return;
		array<int> pids = {};
		pm.GetPlayers(pids);
		foreach (int pid : pids)
		{
			IEntity e = pm.GetPlayerControlledEntity(pid);
			if (e)
				s_aPos.Insert(e.GetOrigin());
		}
	}

	static int Count()
	{
		Refresh(GetGame().GetWorld().GetWorldTime());
		return s_aPos.Count();
	}

	static bool Within(vector pos, float m)
	{
		if (DCO_Dev.TestActive())
			return true;
		Refresh(GetGame().GetWorld().GetWorldTime());
		if (s_aPos.IsEmpty())
			return true;
		float m2 = m * m;
		foreach (vector p : s_aPos)
		{
			if (vector.DistanceSqXZ(p, pos) <= m2)
				return true;
		}
		return false;
	}
}
