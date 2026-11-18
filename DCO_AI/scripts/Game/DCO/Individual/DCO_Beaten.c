class DCO_Beaten
{
	protected static ref map<string, float> s_mUntil;
	protected static ref map<IEntity, vector> s_mLastAtk;
	protected static ref map<IEntity, float> s_mLastAtkAt;
	protected static ref array<vector> s_aDeathPos;
	protected static ref array<float> s_aDeathAt;
	protected static ref array<string> s_aDeathFk;
	protected static ref array<int> s_aDeathSec;
	protected static float s_fLogAt_ms;

	static float Now()
	{
		return GetGame().GetWorld().GetWorldTime();
	}

	static float HoldS(SCR_AIUtilityComponent u)
	{
		return DCO_Stress.T(u, "kz.hold_s", 120.0);
	}

	static float HoldSFor(IEntity e)
	{
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetFor(e);
		if (!g)
			return 120.0;
		return g.DCO_TuneF("kz.hold_s", 120.0);
	}

	static int Sector(vector pos, vector threat)
	{
		vector d = threat - pos;
		d[1] = 0;
		if (d.LengthSq() < 1.0)
			return -1;
		int s = Math.Round(Math.Repeat(d.ToYaw(), 360.0) / 45.0);
		return s % 8;
	}

	protected static bool SectorNear(int a, int b)
	{
		if (a < 0 || b < 0)
			return true;
		return a == b || a == (b + 1) % 8 || a == (b + 7) % 8;
	}

	static bool Hostile(IEntity a, IEntity b)
	{
		FactionAffiliationComponent fa = FactionAffiliationComponent.Cast(a.FindComponent(FactionAffiliationComponent));
		FactionAffiliationComponent fb = FactionAffiliationComponent.Cast(b.FindComponent(FactionAffiliationComponent));
		if (!fa || !fb)
			return false;
		Faction x = fa.GetAffiliatedFaction();
		Faction y = fb.GetAffiliatedFaction();
		return x && y && x.IsFactionEnemy(y);
	}

	static void Mark(IEntity victim, vector threat, string why)
	{
		if (!victim || !Replication.IsServer())
			return;
		float holdS = HoldSFor(victim);
		if (holdS <= 0)
			return;
		vector vp = victim.GetOrigin();
		int sector = Sector(vp, threat);
		if (sector < 0)
			return;
		string fk = DCO_Util.FactionKeyOf(victim, true);
		if (fk.IsEmpty())
			return;
		if (!s_mUntil)
			s_mUntil = new map<string, float>();
		float now = Now();
		float hold_ms = holdS * 1000.0 / DCO_Stress.ReachFor(victim);
		string cell = DCO_LearnStore.PosKey(vp, 15.0);
		string key = fk + "|" + sector.ToString() + "|" + cell;
		float until;
		if (s_mUntil.Find(key, until) && until >= now + hold_ms)
		{
			DCO_Perf.Count("beaten:mark_keep");
			return;
		}
		s_mUntil.Set(key, now + hold_ms);
		if (s_mUntil.Count() > 256)
			Prune(now);
		DCO_Perf.Count("beaten:mark_" + why);
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(victim), string.Format("beaten_mark unit=%1 why=%2 cell=%3 sector=%4 hold_s=%5 cells=%6 threat_d=%7", DCO_Log.IdUnit(victim), why, cell, sector, Math.Round(hold_ms / 1000.0), s_mUntil.Count(), Math.Round(vector.DistanceXZ(vp, threat))));
	}

	protected static void Prune(float now)
	{
		array<string> gone = {};
		foreach (string k, float until : s_mUntil)
		{
			if (until <= now)
				gone.Insert(k);
		}
		foreach (string k : gone)
			s_mUntil.Remove(k);
		DCO_Perf.Count("beaten:prune");
	}

	static void OnDamage(IEntity victim, IEntity attacker)
	{
		if (!victim || !attacker || victim == attacker || !Replication.IsServer())
			return;
		if (!Hostile(victim, attacker))
			return;
		if (!s_mLastAtk)
		{
			s_mLastAtk = new map<IEntity, vector>();
			s_mLastAtkAt = new map<IEntity, float>();
		}
		if (s_mLastAtk.Count() > 128)
		{
			s_mLastAtk.Clear();
			s_mLastAtkAt.Clear();
		}
		s_mLastAtk.Set(victim, attacker.GetOrigin());
		s_mLastAtkAt.Set(victim, Now());
		if (EntityUtils.IsPlayer(victim))
			return;
		Mark(victim, attacker.GetOrigin(), "hit");
	}

	static bool HitRecent(IEntity e, float s)
	{
		float at;
		return e && s_mLastAtkAt && s_mLastAtkAt.Find(e, at) && Now() - at < s * 1000.0;
	}

	static vector LastAttackerPos(IEntity e)
	{
		vector p;
		if (e && s_mLastAtk && s_mLastAtk.Find(e, p))
			return p;
		return vector.Zero;
	}

	static void OnDeath(IEntity victim)
	{
		if (!victim || !s_mLastAtk || !Replication.IsServer() || EntityUtils.IsPlayer(victim))
			return;
		vector atk;
		float at;
		if (!s_mLastAtk.Find(victim, atk) || !s_mLastAtkAt.Find(victim, at))
			return;
		s_mLastAtk.Remove(victim);
		s_mLastAtkAt.Remove(victim);
		float now = Now();
		if (now - at > 120000.0)
			return;
		Mark(victim, atk, "death");
		int sector = Sector(victim.GetOrigin(), atk);
		string fk = DCO_Util.FactionKeyOf(victim, true);
		if (sector < 0 || fk.IsEmpty() || HoldSFor(victim) <= 0)
			return;
		if (!s_aDeathAt)
		{
			s_aDeathPos = {};
			s_aDeathAt = {};
			s_aDeathFk = {};
			s_aDeathSec = {};
		}
		if (s_aDeathAt.Count() >= 64)
			RemoveDeath(0);
		s_aDeathPos.Insert(victim.GetOrigin());
		s_aDeathAt.Insert(now);
		s_aDeathFk.Insert(fk);
		s_aDeathSec.Insert(sector);
		DCO_Perf.Count("beaten:death_noted");
	}

	protected static void RemoveDeath(int i)
	{
		s_aDeathPos.RemoveOrdered(i);
		s_aDeathAt.RemoveOrdered(i);
		s_aDeathFk.RemoveOrdered(i);
		s_aDeathSec.RemoveOrdered(i);
	}

	protected static string CellHit(string fk, int sector, vector pos, float now)
	{
		if (!s_mUntil)
			return "";
		string cell = DCO_LearnStore.PosKey(pos, 15.0);
		for (int i = 0; i < 8; i++)
		{
			if (!SectorNear(i, sector))
				continue;
			string k = fk + "|" + i.ToString() + "|" + cell;
			float until;
			if (s_mUntil.Find(k, until) && until > now)
				return k;
		}
		return "";
	}

	protected static string DeathHit(string fk, int sector, vector pos, float now)
	{
		if (!s_aDeathAt)
			return "";
		for (int i = s_aDeathAt.Count() - 1; i >= 0; i--)
		{
			if (now - s_aDeathAt[i] > 300000.0)
			{
				RemoveDeath(i);
				continue;
			}
			if (s_aDeathFk[i] != fk || !SectorNear(s_aDeathSec[i], sector))
				continue;
			if (vector.DistanceXZ(pos, s_aDeathPos[i]) > 6.0)
				continue;
			return "death|" + s_aDeathSec[i].ToString() + "|" + DCO_Util.XZ(s_aDeathPos[i]);
		}
		return "";
	}

	static bool Check(SCR_AIUtilityComponent u, vector pos, vector threat, string src)
	{
		if (!u || !u.m_OwnerEntity || !Replication.IsServer())
			return false;
		if (DCO_Wire.Avoid(u, pos))
		{
			DCO_Perf.Count("wire:avoid_" + src);
			return true;
		}
		bool anyCell = s_mUntil && s_mUntil.Count() > 0;
		bool anyDeath = s_aDeathAt && s_aDeathAt.Count() > 0;
		if (!anyCell && !anyDeath)
			return false;
		if (HoldS(u) <= 0 || DCO_Relentless.Unit(u))
			return false;
		string fk = DCO_Util.FactionKeyOf(u.m_OwnerEntity, true);
		if (fk.IsEmpty())
			return false;
		vector thr = threat;
		if (thr == vector.Zero)
		{
			DCO_EngageInfo ei = DCO_Engage.Info(u);
			if (ei && ei.m_iKnown > 0)
				thr = ei.m_vNearest;
		}
		int sector = Sector(pos, thr);
		float now = Now();
		string hit = CellHit(fk, sector, pos, now);
		if (hit.IsEmpty())
			hit = DeathHit(fk, sector, pos, now);
		if (hit.IsEmpty())
			return false;
		DCO_Perf.Count("beaten:avoid_" + src);
		if (now >= s_fLogAt_ms && DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO))
		{
			s_fLogAt_ms = now + 2000.0;
			DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(u.m_OwnerEntity), string.Format("beaten_avoid unit=%1 src=%2 pos=%3 dist=%4 sector=%5 key=%6", DCO_Log.IdUnit(u.m_OwnerEntity), src, DCO_Util.XZ(pos), Math.Round(vector.DistanceXZ(pos, u.m_OwnerEntity.GetOrigin())), sector, hit));
		}
		return true;
	}
}
