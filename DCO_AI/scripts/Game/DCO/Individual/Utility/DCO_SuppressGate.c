class DCO_SuppressGate
{
	static bool Allowed(SCR_AIUtilityComponent u, vector target, string path)
	{
		if (!u || !u.m_OwnerEntity)
			return false;
		IEntity me = u.m_OwnerEntity;
		float eye = 1.5;
		ChimeraCharacter ch = ChimeraCharacter.Cast(me);
		if (ch && ch.GetCharacterController())
		{
			ECharacterStance st = ch.GetCharacterController().GetStance();
			if (st == ECharacterStance.CROUCH)
				eye = 1.0;
			else if (st == ECharacterStance.PRONE)
				eye = 0.4;
		}
		vector from = me.GetOrigin() + Vector(0, eye, 0);
		vector to = target + Vector(0, 1.0, 0);
		vector dir = to - from;
		float dist = dir.Length();
		if (dist > 6.0)
		{
			dir.Normalize();
			from = from + dir * 3.0;
		}
		float frac = DCO_Util.TraceFrac(from, to, me, TraceFlags.WORLD | TraceFlags.ENTS, EPhysicsLayerPresets.Projectile, "t:DCO_SuppressGate");
		if (frac >= 0.4)
		{
			DCO_Perf.Count("supp_gate:ok_" + path);
			return true;
		}
		if (dist <= 150.0)
		{
			string bm;
			if (SCR_CoverManagerComponent.DCO_GetBuildingAtPos(target, null, bm))
			{
				DCO_Perf.Count("supp_gate:ok_building_" + path);
				return true;
			}
		}
		DCO_Perf.Count("supp_gate:blocked_" + path);
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("suppress_blocked unit=%1 path=%2 dist=%3 trace_frac=%4 indoor=%5",
			DCO_Log.IdUnit(me), path, Math.Round(vector.Distance(me.GetOrigin(), target)), frac.ToString(-1, 2), DCO_Util.B01(SCR_CoverManagerComponent.IsEntityInsideBuilding(me))));
		return false;
	}
}
