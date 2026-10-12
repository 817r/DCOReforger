modded class SCR_AIUtilityComponent
{
	vector m_vDCOOscLegStart;
	vector m_vDCOOscLegDir;
	vector m_vDCOOscWinPos;
	float m_fDCOOscCheck_ms;
	float m_fDCOOscWinStart_ms = -1;
	float m_fDCOOscLogAt_ms;
	int m_iDCOOscRev;
	string m_sDCOOscLastTag;
	string m_sDCOOscTrail;
	SCR_EAICombatMoveDirection m_eDCODangerDir;
	float m_fDCODangerDirAt_ms = -100000;
}

class DCO_Osc
{
	static bool Opposite(SCR_EAICombatMoveDirection a, SCR_EAICombatMoveDirection b)
	{
		if (a == SCR_EAICombatMoveDirection.FORWARD)
			return b == SCR_EAICombatMoveDirection.BACKWARD;
		if (a == SCR_EAICombatMoveDirection.BACKWARD)
			return b == SCR_EAICombatMoveDirection.FORWARD;
		if (a == SCR_EAICombatMoveDirection.LEFT)
			return b == SCR_EAICombatMoveDirection.RIGHT;
		if (a == SCR_EAICombatMoveDirection.RIGHT)
			return b == SCR_EAICombatMoveDirection.LEFT;
		return false;
	}

	static SCR_EAICombatMoveDirection KeepDir(SCR_AIUtilityComponent u, SCR_EAICombatMoveDirection d, bool allowForward, string src)
	{
		if (!u || !u.m_OwnerEntity || DCO_Ctx.TuneForU(u, "osc.fix_dir", 1.0) <= 0)
			return d;
		float now = GetGame().GetWorld().GetWorldTime();
		float age = now - u.m_fDCODangerDirAt_ms;
		SCR_EAICombatMoveDirection last = u.m_eDCODangerDir;
		string why;
		if (last != SCR_EAICombatMoveDirection.FORWARD || allowForward)
		{
			if (age < DCO_Ctx.TuneForU(u, "osc.dir_keep_s", 8.0) * 1000.0)
				why = "reuse";
			else if (age < DCO_Ctx.TuneForU(u, "osc.dir_noreverse_s", 15.0) * 1000.0 && Opposite(last, d))
				why = "no_reverse";
		}
		if (why.IsEmpty())
		{
			u.m_eDCODangerDir = d;
			u.m_fDCODangerDirAt_ms = now;
			DCO_Perf.Count("osc:dir_new");
			return d;
		}
		DCO_Perf.Count("osc:dir_" + why);
		if (last != d && DCO_Log.On(DCO_ELogChannel.UNIT, DCO_ELogLevel.DEBUG))
			DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(u.m_OwnerEntity), string.Format("danger_dir_keep unit=%1 src=%2 picked=%3 kept=%4 why=%5 age_s=%6 allow_fwd=%7",
				DCO_Log.IdUnit(u.m_OwnerEntity), src, typename.EnumToString(SCR_EAICombatMoveDirection, d), typename.EnumToString(SCR_EAICombatMoveDirection, last), why, (age / 1000.0).ToString(-1, 1), DCO_Util.B01(allowForward)));
		return last;
	}

	static void Tick(SCR_AIUtilityComponent u)
	{
		if (!u)
			return;
		float now = GetGame().GetWorld().GetWorldTime();
		if (now < u.m_fDCOOscCheck_ms)
			return;
		u.m_fDCOOscCheck_ms = now + 1000.0;
		if (!u.m_OwnerEntity || DCO_Ctx.TuneForU(u, "osc.on", 1.0) <= 0)
			return;
		vector p = u.m_OwnerEntity.GetOrigin();
		if (u.m_vDCOOscLegStart == vector.Zero)
		{
			u.m_vDCOOscLegStart = p;
			return;
		}
		vector d = p - u.m_vDCOOscLegStart;
		d[1] = 0;
		float len = d.Length();
		if (len < DCO_Ctx.TuneForU(u, "osc.leg_m", 3.0))
			return;
		d = d * (1.0 / len);
		string tag = Tag(u);
		if (u.m_vDCOOscLegDir != vector.Zero && vector.Dot(d, u.m_vDCOOscLegDir) < -0.5)
		{
			DCO_Perf.Count("osc:rev_" + tag);
			if (u.m_fDCOOscWinStart_ms < 0 || now - u.m_fDCOOscWinStart_ms > 30000.0)
			{
				u.m_fDCOOscWinStart_ms = now;
				u.m_vDCOOscWinPos = u.m_vDCOOscLegStart;
				u.m_iDCOOscRev = 0;
				u.m_sDCOOscTrail = string.Empty;
			}
			u.m_iDCOOscRev++;
			if (u.m_sDCOOscTrail.Length() < 400)
				u.m_sDCOOscTrail += u.m_sDCOOscLastTag + ">" + tag + ";";
			if (u.m_iDCOOscRev >= 3)
			{
				float net = vector.DistanceXZ(p, u.m_vDCOOscWinPos);
				float span = (now - u.m_fDCOOscWinStart_ms) / 1000.0;
				if (net < 8.0)
				{
					DCO_Perf.Count("osc:detect");
					if (now >= u.m_fDCOOscLogAt_ms && DCO_Log.On(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO))
					{
						u.m_fDCOOscLogAt_ms = now + 10000.0;
						float supp;
						if (u.m_ThreatSystem)
							supp = u.m_ThreatSystem.GetSuppressionMeasure();
						DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(u.m_OwnerEntity), string.Format("osc_detect unit=%1 rev=%2 span_s=%3 net=%4 leg=%5 supp=%6 pos=%7 trail=%8",
							DCO_Log.IdUnit(u.m_OwnerEntity), u.m_iDCOOscRev, span.ToString(-1, 1), net.ToString(-1, 1), Math.Round(len), supp.ToString(-1, 2), DCO_Log.Pos(p), u.m_sDCOOscTrail));
					}
				}
				u.m_fDCOOscWinStart_ms = -1;
			}
		}
		u.m_vDCOOscLegDir = d;
		u.m_vDCOOscLegStart = p;
		u.m_sDCOOscLastTag = tag;
	}

	static string Tag(SCR_AIUtilityComponent u)
	{
		string tag = "-";
		AIActionBase a = u.GetCurrentAction();
		if (a)
		{
			tag = a.ClassName();
			tag.Replace("SCR_AI", "");
			tag.Replace("Behavior", "");
		}
		DCO_RelocateBehavior rb = DCO_RelocateBehavior.Cast(a);
		if (rb)
			tag = tag + ":" + rb.DCO_Why();
		SCR_AICombatMoveState st = u.m_CombatMoveState;
		if (!st)
			return tag;
		SCR_AICombatMoveRequest_Move rq = SCR_AICombatMoveRequest_Move.Cast(st.GetRequest());
		if (rq)
			tag = tag + "/" + typename.EnumToString(SCR_EAICombatMoveReason, rq.m_eReason) + "_" + typename.EnumToString(SCR_EAICombatMoveDirection, rq.m_eDirection);
		return tag;
	}
}
