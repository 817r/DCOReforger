class DCO_SuppressGate
{
	static bool Allowed(SCR_AIUtilityComponent u, vector target, string path, float minFrac = 0.4)
	{
		if (!u || !u.m_OwnerEntity)
			return false;
		IEntity me = u.m_OwnerEntity;
		float eye = 1.5;
		ChimeraCharacter ch = ChimeraCharacter.Cast(me);
		if (ch && ch.GetCharacterController())
		{
			ECharacterStance st = ch.GetCharacterController().GetStance();
			if (st == ECharacterStance.CROUCH || st == ECharacterStance.PRONE)
				eye = 1.0;
		}
		vector from = me.GetOrigin() + Vector(0, eye, 0);
		vector to = target + Vector(0, 1.0, 0);
		vector dir = to - from;
		float dist = dir.Length();
		if (dist > 1.0)
		{
			dir.Normalize();
			from = from + dir * Math.Clamp(DCO_Stress.T(u, "supp.gate_skip_m", 3.0), 0.3, dist * 0.5);
		}
		string bm;
		IEntity bld;
		if (dist > 6.0 && dist <= 400.0)
			bld = DCO_CoverManagerComponent.DCO_GetBuildingAtPos(target, null, bm);
		if (bld)
		{
			to = to - dir * 1.0;
			DCO_Perf.Count("supp_gate:window_front_" + path);
		}
		float frac = DCO_Util.TraceFrac(from, to, me, TraceFlags.WORLD | TraceFlags.ENTS, EPhysicsLayerPresets.Projectile, "t:DCO_SuppressGate");
		IEntity hit = DCO_Util.LastTraceEnt();
		float len = vector.Distance(from, to);
		float shortM = (1.0 - frac) * len;
		if (FracOk(frac, len, minFrac))
		{
			DCO_Perf.Count("supp_gate:ok_" + path);
			return true;
		}
		bool hitTarget = bld && hit && hit.GetRootParent() == bld.GetRootParent();
		if (hitTarget && dist <= 150.0 && shortM <= 8.0)
		{
			DCO_Perf.Count("supp_gate:ok_building_" + path);
			return true;
		}
		DCO_Perf.Count("supp_gate:blocked_" + path);
		if (frac >= minFrac)
			DCO_Perf.Count("supp_gate:blocked_short_" + path);
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("suppress_blocked unit=%1 path=%2 dist=%3 trace_frac=%4 indoor=%5 target_bld=%6 short_m=%7 hit_target=%8",
			DCO_Log.IdUnit(me), path, Math.Round(vector.Distance(me.GetOrigin(), target)), frac.ToString(-1, 2), DCO_Util.B01(DCO_CoverManagerComponent.IsEntityInsideBuilding(me)), DCO_Util.B01(bld != null), Math.Round(shortM), DCO_Util.B01(hitTarget)));
		return false;
	}

	static bool FracOk(float frac, float len, float minFrac)
	{
		if (frac < minFrac)
			return false;
		if (frac >= 0.999 || len > 60.0)
			return true;
		return (1.0 - frac) * len <= 5.0;
	}
}
