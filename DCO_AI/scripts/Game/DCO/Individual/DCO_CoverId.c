modded class SCR_AIUtilityComponent
{
	string m_sDCOCidKind = "none";
	string m_sDCOCidEnt = "-";
	string m_sDCOCidMis = "-";
	string m_sDCOCidKey;
	bool m_bDCOCidHard;
	int m_iDCOCidProt;
	int m_iDCOCidArcL;
	int m_iDCOCidArcR;
	float m_fDCOCidThreatDeg = -1;
	float m_fDCOCidAt_ms = -100000;
	float m_fDCOCidCheck_ms;
	float m_fDCOCovLogAt_ms;
	vector m_vDCOCidPos;
}

class DCO_CoverId
{
	static bool On()
	{
		return DCO_Ctx.TuneG("cid.on", 1.0) > 0;
	}

	static void Tick(SCR_AIUtilityComponent u)
	{
		if (!u)
			return;
		float now = GetGame().GetWorld().GetWorldTime();
		if (now < u.m_fDCOCidCheck_ms)
			return;
		u.m_fDCOCidCheck_ms = now + 1000.0;
		if (!u.m_OwnerEntity || !u.m_CombatMoveState || !On())
			return;

		SCR_AICombatMoveState st = u.m_CombatMoveState;
		string src;
		if (st.IsInValidCover())
			src = "lock";
		else if (st.m_bInCover)
			src = "indoor";
		else if (now < st.m_fDCOFieldHiddenUntil_ms)
			src = "conceal";
		else
			return;

		vector pos = u.m_OwnerEntity.GetOrigin();
		string tsrc;
		vector dir = ThreatDir(u, tsrc);
		float deg = Math.Round(Math.Atan2(dir[0], dir[2]) * Math.RAD2DEG);
		if (deg < 0)
			deg = deg + 360.0;
		bool moved = vector.DistanceXZ(pos, u.m_vDCOCidPos) > 1.5;
		float dd = Math.AbsFloat(deg - u.m_fDCOCidThreatDeg);
		if (dd > 180.0)
			dd = 360.0 - dd;
		bool turned = u.m_fDCOCidThreatDeg < 0 || dd > 30.0;
		bool stale = tsrc == "enemy" && now - u.m_fDCOCidAt_ms > DCO_Ctx.TuneForU(u, "cid.stale_s", 10.0) * 1000.0;
		if (!moved && !turned && !stale)
			return;
		Compute(u, pos, dir, deg, src, tsrc, now);
	}

	static vector ThreatDir(SCR_AIUtilityComponent u, out string tsrc)
	{
		vector pos = u.m_OwnerEntity.GetOrigin();
		DCO_EngageInfo i = DCO_Engage.Info(u);
		if (i && i.m_iKnown > 0 && i.m_vNearest != vector.Zero)
		{
			vector d = i.m_vNearest - pos;
			d[1] = 0;
			if (d.LengthSq() > 0.25)
			{
				tsrc = "enemy";
				return d.Normalized();
			}
		}
		tsrc = "facing";
		vector f = u.m_OwnerEntity.GetTransformAxis(2);
		f[1] = 0;
		if (f.LengthSq() < 0.01)
			return vector.Forward;
		return f.Normalized();
	}

	static bool Blocked(vector pos, vector dir, float h, IEntity me, out IEntity hit)
	{
		vector from = pos + Vector(0, h, 0);
		float f = DCO_Util.TraceFrac(from, from + dir * 2.0, me, TraceFlags.WORLD | TraceFlags.ENTS, -1, "cid");
		if (f >= 0.99)
		{
			hit = null;
			return false;
		}
		hit = DCO_Util.LastTraceEnt();
		return true;
	}

	static vector Rot(vector dir, float deg)
	{
		float r = deg * Math.DEG2RAD;
		float c = Math.Cos(r);
		float s = Math.Sin(r);
		return Vector(dir[0] * c - dir[2] * s, 0, dir[0] * s + dir[2] * c);
	}

	static int ArcSide(vector pos, vector dir, float h, IEntity me, float sign, int steps = 3)
	{
		int ext;
		IEntity hit;
		for (int k = 0; k < steps; k++)
		{
			int a = 20;
			if (k == 1)
				a = 45;
			else if (k == 2)
				a = 70;
			if (!Blocked(pos, Rot(dir, sign * a), h, me, hit))
				break;
			ext = a;
		}
		return ext;
	}

	static string Kind(IEntity hit, out bool hard, out string ent)
	{
		ent = "-";
		hard = true;
		string base = DCO_Util.HitKind(hit);
		IEntity root;
		if (hit)
			root = hit.GetRootParent();
		if (!root)
			root = hit;
		string path;
		if (root && root.GetPrefabData())
		{
			path = root.GetPrefabData().GetPrefabName();
			ent = FilePath.StripExtension(FilePath.StripPath(path));
			path.ToLower();
		}
		if (base == "terrain" || base == "building" || base == "vehicle")
			return base;
		if (base == "tree")
		{
			hard = !path.Contains("bush") && !path.Contains("shrub");
			if (!hard)
				return "bush";
			return "tree";
		}
		if (root && ChimeraCharacter.Cast(root))
		{
			hard = false;
			return "mate";
		}
		bool soft = path.Contains("wood") || path.Contains("plank") || path.Contains("crate") || path.Contains("pallet");
		if (path.Contains("sandbag"))
			return "sandbag";
		if (path.Contains("fence"))
		{
			hard = path.Contains("stone") || path.Contains("concrete") || path.Contains("brick");
			return "fence";
		}
		if (path.Contains("wall"))
		{
			hard = !soft;
			return "wall";
		}
		if (path.Contains("rock") || path.Contains("stone") || path.Contains("boulder") || path.Contains("cliff"))
			return "rock";
		if (path.Contains("bush") || path.Contains("shrub") || path.Contains("hedge") || path.Contains("plant") || path.Contains("vegetation") || path.Contains("grass"))
		{
			hard = false;
			return "bush";
		}
		hard = !soft;
		return "object";
	}

	static int Prot(vector pos, vector dir, IEntity me, out IEntity hit, out float h)
	{
		IEntity hC, hS, hP;
		h = 0.85;
		if (Blocked(pos, dir, 0.85, me, hC))
		{
			hit = hC;
			if (Blocked(pos, dir, 1.45, me, hS))
				return 3;
			return 2;
		}
		if (Blocked(pos, dir, 0.35, me, hP))
		{
			hit = hP;
			h = 0.35;
			return 1;
		}
		hit = null;
		return 0;
	}

	static string ProtName(int p)
	{
		if (p == 3)
			return "stand";
		if (p == 2)
			return "crouch";
		if (p == 1)
			return "prone";
		return "none";
	}

	protected static void Compute(SCR_AIUtilityComponent u, vector pos, vector dir, float deg, string src, string tsrc, float now)
	{
		IEntity me = u.m_OwnerEntity;
		IEntity hit;
		float h;
		int prot = Prot(pos, dir, me, hit, h);

		string kind = "none";
		string ent = "-";
		bool hard = false;
		int arcL, arcR;
		if (hit || prot > 0)
		{
			kind = Kind(hit, hard, ent);
			arcL = ArcSide(pos, dir, h, me, -1.0, 2);
			arcR = ArcSide(pos, dir, h, me, 1.0, 2);
		}

		ECharacterStance stance = ECharacterStance.STAND;
		ChimeraCharacter ch = ChimeraCharacter.Cast(me);
		if (ch && ch.GetCharacterController())
			stance = ch.GetCharacterController().GetStance();
		float supp;
		if (u.m_ThreatSystem)
			supp = u.m_ThreatSystem.GetSuppressionMeasure();

		string mis = "-";
		if (prot == 0)
			mis = "no_block";
		else if (!hard && supp >= 0.6)
			mis = "soft_under_fire";
		else if (stance == ECharacterStance.STAND && prot < 3)
			mis = "stand_over_low";
		else if (stance == ECharacterStance.CROUCH && prot < 2)
			mis = "crouch_over_low";
		bool exposed = u.m_CombatMoveState && u.m_CombatMoveState.m_bExposedInCover;
		if (exposed && (mis == "stand_over_low" || mis == "crouch_over_low"))
			mis = mis + "_peek";

		u.m_sDCOCidKind = kind;
		u.m_sDCOCidEnt = ent;
		u.m_bDCOCidHard = hard;
		u.m_iDCOCidProt = prot;
		u.m_iDCOCidArcL = arcL;
		u.m_iDCOCidArcR = arcR;
		u.m_sDCOCidMis = mis;
		u.m_fDCOCidThreatDeg = deg;
		u.m_fDCOCidAt_ms = now;
		u.m_vDCOCidPos = pos;

		DCO_Perf.Count("cid:kind_" + kind);
		if (mis != "-")
			DCO_Perf.Count("cid:mis_" + mis);

		string key = string.Format("%1|%2|%3|%4|%5|%6", kind, ent, prot, arcL, arcR, mis);
		if (key == u.m_sDCOCidKey || !DCO_Log.On(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO))
			return;
		u.m_sDCOCidKey = key;
		DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("cover_id unit=%1 kind=%2 ent=%3 hard=%4 protects=%5 arc=-%6..+%7 threat_deg=%8 threat_src=%9", DCO_Log.IdUnit(me), kind, ent, DCO_Util.B01(hard), ProtName(prot), arcL, arcR, deg, tsrc)
			+ string.Format(" stance=%1 supp=%2 mismatch=%3 src=%4 pos=%5 exposed=%6", typename.EnumToString(ECharacterStance, stance), supp.ToString(-1, 2), mis, src, DCO_Util.XZ(pos), DCO_Util.B01(exposed)));
	}

	static string Brief(SCR_AIUtilityComponent u, IEntity e)
	{
		if (!u || !e || GetGame().GetWorld().GetWorldTime() - u.m_fDCOCidAt_ms > 10000.0 || vector.DistanceXZ(e.GetOrigin(), u.m_vDCOCidPos) > 2.0)
			return "-";
		return string.Format("%1/%2/%3/%4", u.m_sDCOCidKind, DCO_Util.B01(u.m_bDCOCidHard), ProtName(u.m_iDCOCidProt), u.m_sDCOCidMis);
	}
}
