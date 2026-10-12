class DCO_SearchClaim
{
	IEntity m_Who;
	vector m_vPos;
	float m_fUntil_ms;
}

class DCO_Search
{
	protected static ref array<ref DCO_SearchClaim> s_aClaims;

	static bool Claimed(vector p, IEntity me, float now)
	{
		if (!s_aClaims)
			return false;
		for (int i = s_aClaims.Count() - 1; i >= 0; i--)
		{
			DCO_SearchClaim c = s_aClaims[i];
			if (!c.m_Who || now > c.m_fUntil_ms)
			{
				s_aClaims.Remove(i);
				continue;
			}
			if (c.m_Who != me && vector.DistanceXZ(c.m_vPos, p) < 6.0)
				return true;
		}
		return false;
	}

	static void Claim(IEntity me, vector p, float now)
	{
		if (!s_aClaims)
			s_aClaims = {};
		Release(me);
		DCO_SearchClaim c = new DCO_SearchClaim();
		c.m_Who = me;
		c.m_vPos = p;
		c.m_fUntil_ms = now + 20000.0;
		s_aClaims.Insert(c);
	}

	static void Release(IEntity me)
	{
		if (!s_aClaims)
			return;
		for (int i = s_aClaims.Count() - 1; i >= 0; i--)
		{
			if (s_aClaims[i].m_Who == me)
				s_aClaims.Remove(i);
		}
	}

	static bool PickSpot(IEntity me, vector pos, vector lkp, float now, out vector spot)
	{
		vector eye = pos + vector.Up * 1.6;
		float best = float.MAX;
		float jitter = Math.RandomFloat(0, 45.0);
		for (int a = 0; a < 8; a++)
		{
			for (int k = 0; k < 2; k++)
			{
				float rr = 7.0 + k * 7.0;
				float ang = (a * 45.0 + jitter + 1.0 * k * 22.5) * Math.DEG2RAD;
				vector q;
				if (!DCO_FlushDrill.Reachable(lkp + Vector(Math.Sin(ang) * rr, 0, Math.Cos(ang) * rr), q))
					continue;
				float dLkp = vector.DistanceXZ(q, lkp);
				float dMe = vector.DistanceXZ(q, pos);
				if (dLkp > 20.0 || dMe < 4.0 || Claimed(q, me, now))
					continue;
				if (DCO_Util.TraceFrac(eye, q + vector.Up * 1.0, me, TraceFlags.WORLD | TraceFlags.ENTS, -1, "trace:search_spot") >= 0.98)
					continue;
				float score = dMe + dLkp * 0.5;
				if (score < best)
				{
					best = score;
					spot = q;
				}
			}
		}
		return best < float.MAX;
	}

	static bool Corner(IEntity me, vector pos, vector focus, out vector look, out float offDeg)
	{
		vector h = focus - pos;
		h[1] = 0;
		float hl = h.Length();
		if (hl < 1.0)
			return false;
		vector eye = pos + vector.Up * 1.6;
		float baseAng = Math.Atan2(h[0], h[2]);
		float prevD = -1;
		vector prevDir;
		float prevOff;
		float best = float.MAX;
		for (int i = 0; i < 9; i++)
		{
			float off = -60.0 + i * 15.0;
			float ang = baseAng + off * Math.DEG2RAD;
			vector dir = Vector(Math.Sin(ang), 0, Math.Cos(ang));
			float d = DCO_Util.TraceFrac(eye, eye + dir * 30.0, me, TraceFlags.WORLD | TraceFlags.ENTS, -1, "trace:search_corner") * 30.0;
			if (prevD >= 0)
			{
				float nearD = Math.Min(d, prevD);
				float farD = Math.Max(d, prevD);
				if (farD - nearD > 4.0 && nearD > 2.0 && nearD < 28.0)
				{
					vector farDir = dir;
					float farOff = off;
					if (prevD > d)
					{
						farDir = prevDir;
						farOff = prevOff;
					}
					float score = Math.AbsFloat(farOff) + nearD * 1.5;
					if (score < best)
					{
						best = score;
						look = eye + farDir * Math.Min(nearD + 1.5, farD);
						offDeg = farOff;
					}
				}
			}
			prevD = d;
			prevDir = dir;
			prevOff = off;
		}
		return best < float.MAX;
	}
}

modded class SCR_AIMoveAndInvestigateBehavior
{
	protected int m_iDCOSPhase;
	protected float m_fDCOSStart_ms;
	protected float m_fDCOSNext_ms;
	protected float m_fDCOSSpotUntil_ms;
	protected float m_fDCOSHold_ms;
	protected vector m_vDCOSSpot;
	protected vector m_vDCOSLook;
	protected int m_iDCOSSpots;
	protected int m_iDCOSCorners;
	protected int m_iDCOSHeadOn;
	protected bool m_bDCOSEnded;
	protected bool m_bDCOSFound;
	protected int m_iDCOSTick;

	override void OnActionExecuted()
	{
		super.OnActionExecuted();
		float now = GetGame().GetWorld().GetWorldTime();
		if (now < m_fDCOSNext_ms || m_bDCOSEnded)
			return;
		m_fDCOSNext_ms = now + 1000.0;
		int t = DCO_Perf.Begin();
		DCO_SearchTick(now);
		DCO_Perf.End("ag_search", t);
	}

	override void OnActionCompleted()
	{
		if (!DCO_Util.Teardown("search_completed"))
			DCO_SearchEnd("completed");
		super.OnActionCompleted();
	}

	override void OnActionDeselected()
	{
		if (m_iDCOSPhase > 0 && !m_bDCOSEnded && !DCO_Util.Teardown("search_deselect"))
			DCO_SearchFound(m_Utility.m_OwnerEntity, "deselect");
		super.OnActionDeselected();
	}

	override void OnActionFailed()
	{
		if (!DCO_Util.Teardown("search_failed"))
			DCO_SearchEnd("failed");
		super.OnActionFailed();
	}

	protected void DCO_SearchTick(float now)
	{
		IEntity me = m_Utility.m_OwnerEntity;
		if (!me || !m_bIsDangerous.m_Value || !DCO_GlobalAIComponent.BehaviorOnFor(me, DCO_EBehavior.AWARENESS))
			return;
		if (m_Utility.m_AIInfo && m_Utility.m_AIInfo.HasUnitState(EUnitState.IN_VEHICLE))
			return;

		vector pos = me.GetOrigin();
		vector lkp = m_vPosition.m_Value;
		float dl = vector.DistanceXZ(pos, lkp);

		if (DCO_Stress.Level(m_Utility) >= 1 && !DCO_Engage.SearchAllowed(m_Utility))
		{
			DCO_Perf.Count("tm:search_hold_under_fire");
			if (DCO_Log.On(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO))
				DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("search_hold unit=%1 reason=under_fire lkp_dist=%2 phase=%3 zone=field", DCO_Log.IdUnit(me), Math.Round(dl), m_iDCOSPhase));
			DCO_SearchEnd("under_fire");
			Fail();
			return;
		}

		if (m_iDCOSPhase == 0)
		{
			if (dl > 150.0)
				return;
			m_iDCOSPhase = 1;
			m_fDCOSStart_ms = now;
			DCO_Perf.Count("search:start");
			DCO_SquadChatter.NoteSearch(me, lkp);
			if (DCO_Log.On(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO))
				DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("search_start unit=%1 lkp_dist=%2 radius=%3 lkp=%4",
					DCO_Log.IdUnit(me), Math.Round(dl), Math.Round(m_fRadius.m_Value), DCO_Log.Pos(lkp)));
		}

		DCO_SearchHeadCheck(me, now);

		if (DCO_SearchFound(me, "tick"))
			return;

		if (m_iDCOSPhase == 1 && dl <= Math.Min(m_fRadius.m_Value, 8.0) + 4.0)
		{
			m_iDCOSPhase = 2;
			DCO_Perf.Count("search:arrive");
			if (DCO_Log.On(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO))
				DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("search_arrive unit=%1 t_s=%2 corners=%3",
					DCO_Log.IdUnit(me), Math.Round((now - m_fDCOSStart_ms) / 1000.0), m_iDCOSCorners));
		}

		if (m_iDCOSPhase == 2)
			DCO_SearchSpots(me, pos, lkp, now);

		vector focus = lkp;
		if (m_vDCOSSpot != vector.Zero)
			focus = m_vDCOSSpot;
		m_iDCOSTick++;
		if (dl < 60.0 && m_Utility.m_LookAction && (m_iDCOSTick & 1) == 0)
		{
			vector look;
			float off;
			if (DCO_Search.Corner(me, pos, focus, look, off))
			{
				m_Utility.m_LookAction.LookAt(look, 55.0, 2.0);
				if (m_Utility.m_LookAction.m_vPosition != look)
				{
					DCO_Perf.Count("search:look_rejected");
					return;
				}
				m_vDCOSLook = look;
				m_iDCOSCorners++;
				DCO_Perf.Count("search:corner_look");
				if (DCO_Log.On(DCO_ELogChannel.UNIT, DCO_ELogLevel.DEBUG))
					DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(me), string.Format("search_corner unit=%1 phase=%2 off_deg=%3 dist=%4",
						DCO_Log.IdUnit(me), m_iDCOSPhase, Math.Round(off), Math.Round(vector.Distance(pos, look))));
			}
			else
				DCO_Perf.Count("search:corner_none");
		}
	}

	protected bool DCO_SearchFound(IEntity me, string how)
	{
		if (m_bDCOSFound || !m_Utility.m_CombatComponent)
			return m_bDCOSFound;
		BaseTarget t = m_Utility.m_CombatComponent.GetLastSeenEnemy();
		if (!t || !t.GetTargetEntity() || t.GetTimeSinceSeen() > 2.0)
			return false;
		m_bDCOSFound = true;
		string phase = "approach";
		if (m_iDCOSPhase == 2)
			phase = "search";
		else if (m_iDCOSPhase == 3)
			phase = "cleared";
		DCO_Perf.Count("search:found");
		DCO_Perf.Count("search:found_" + phase);
		if (DCO_Log.On(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO))
			DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("search_found unit=%1 enemy=%2 dist=%3 phase=%4 how=%5 spots=%6 corners=%7 t_s=%8",
				DCO_Log.IdUnit(me), DCO_Log.IdUnit(t.GetTargetEntity()), Math.Round(vector.Distance(me.GetOrigin(), t.GetTargetEntity().GetOrigin())), phase, how, m_iDCOSSpots, m_iDCOSCorners, Math.Round((GetGame().GetWorld().GetWorldTime() - m_fDCOSStart_ms) / 1000.0)));
		return true;
	}

	protected void DCO_SearchHeadCheck(IEntity me, float now)
	{
		if (m_vDCOSLook == vector.Zero)
			return;
		ChimeraCharacter c = ChimeraCharacter.Cast(me);
		if (!c || !c.GetHeadAimingComponent())
			return;
		vector want = m_vDCOSLook - (me.GetOrigin() + vector.Up * 1.6);
		want[1] = 0;
		vector have = c.GetHeadAimingComponent().GetAimingDirectionWorld();
		have[1] = 0;
		want.Normalize();
		have.Normalize();
		vector fwd = me.GetTransformAxis(2);
		fwd[1] = 0;
		fwd.Normalize();
		if (DCO_Log.On(DCO_ELogChannel.UNIT, DCO_ELogLevel.DEBUG))
			DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(me), string.Format("search_head unit=%1 head_err=%2 body_err=%3 head_body=%4 look_pos_now=%5 weapon_raised=%6",
				DCO_Log.IdUnit(me), Math.Round(Math.Acos(Math.Clamp(vector.Dot(want, have), -1, 1)) * Math.RAD2DEG), Math.Round(Math.Acos(Math.Clamp(vector.Dot(want, fwd), -1, 1)) * Math.RAD2DEG),
				Math.Round(Math.Acos(Math.Clamp(vector.Dot(have, fwd), -1, 1)) * Math.RAD2DEG), m_Utility.m_LookAction.m_vPosition == m_vDCOSLook, c.GetCharacterController().IsWeaponRaised()));
		if (vector.Dot(want, have) > 0.9 || vector.Dot(want, fwd) > 0.9)
		{
			m_iDCOSHeadOn++;
			DCO_Perf.Count("search:corner_head_on");
		}
		else
			DCO_Perf.Count("search:corner_head_off");
		m_vDCOSLook = vector.Zero;
	}

	protected void DCO_SearchSpots(IEntity me, vector pos, vector lkp, float now)
	{
		if (m_vDCOSSpot != vector.Zero)
		{
			bool reached = vector.DistanceXZ(pos, m_vDCOSSpot) < 2.5;
			if (!reached && now < m_fDCOSSpotUntil_ms)
			{
				if (m_Utility.m_CombatMoveState && !m_Utility.m_CombatMoveState.IsExecutingRequest())
					DCO_SearchMove(m_vDCOSSpot, pos);
				return;
			}
			string res = "timeout";
			if (reached)
				res = "reached";
			DCO_Perf.Count("search:spot_" + res);
			if (DCO_Log.On(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO))
				DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("search_spot_done unit=%1 n=%2 result=%3 left=%4",
					DCO_Log.IdUnit(me), m_iDCOSSpots, res, Math.Round(vector.DistanceXZ(pos, m_vDCOSSpot))));
			m_vDCOSSpot = vector.Zero;
			m_fDCOSHold_ms = now + 2000.0;
			return;
		}
		if (now < m_fDCOSHold_ms)
			return;

		vector spot;
		string why;
		if (m_iDCOSSpots >= 3)
			why = "done";
		else if (!DCO_Search.PickSpot(me, pos, lkp, now, spot))
			why = "no_spot";
		if (!why.IsEmpty())
		{
			m_iDCOSPhase = 3;
			DCO_Search.Release(me);
			DCO_Perf.Count("search:clear");
			DCO_Perf.Count("search:clear_" + why);
			if (DCO_Log.On(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO))
				DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("search_clear unit=%1 why=%2 spots=%3 corners=%4 head_on=%5 dur_s=%6",
					DCO_Log.IdUnit(me), why, m_iDCOSSpots, m_iDCOSCorners, m_iDCOSHeadOn, Math.Round((now - m_fDCOSStart_ms) / 1000.0)));
			return;
		}

		m_vDCOSSpot = spot;
		m_iDCOSSpots++;
		m_fDCOSSpotUntil_ms = now + 15000.0;
		DCO_Search.Claim(me, spot, now);
		bool pushBusy = m_Utility.m_CombatMoveState && m_Utility.m_CombatMoveState.IsExecutingRequest();
		DCO_SearchMove(spot, pos);
		DCO_Perf.Count("search:spot");
		if (pushBusy)
			DCO_Perf.Count("search:spot_override_move");
		if (DCO_Log.On(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO))
			DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("search_spot unit=%1 n=%2 dist=%3 lkp_dist=%4 override=%5 spot=%6",
				DCO_Log.IdUnit(me), m_iDCOSSpots, Math.Round(vector.DistanceXZ(pos, spot)), Math.Round(vector.DistanceXZ(spot, lkp)), pushBusy, DCO_Log.Pos(spot)));
	}

	protected void DCO_SearchMove(vector spot, vector pos)
	{
		SCR_AICombatMoveState st = m_Utility.m_CombatMoveState;
		if (!st)
			return;
		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();
		rq.m_eType = SCR_EAICombatMoveRequestType.MOVE;
		rq.m_eReason = SCR_EAICombatMoveReason.INVESTIGATE;
		rq.m_vTargetPos = spot + vector.Up * 1.0;
		rq.m_vMovePos = spot;
		rq.m_eDirection = SCR_EAICombatMoveDirection.CUSTOM_POS;
		rq.m_bTryFindCover = false;
		rq.m_bFailIfNoCover = false;
		rq.m_eStanceMoving = ECharacterStance.CROUCH;
		rq.m_eStanceEnd = ECharacterStance.CROUCH;
		rq.m_eMovementType = EMovementType.RUN;
		if (vector.DistanceXZ(pos, spot) < 8.0)
			rq.m_eMovementType = EMovementType.WALK;
		rq.m_bAimAtTarget = true;
		rq.m_bAimAtTargetEnd = true;
		rq.m_fMoveDuration_s = 12.0;
		st.ApplyNewRequest(rq);
	}

	protected void DCO_SearchEnd(string reason)
	{
		if (m_iDCOSPhase == 0 || m_bDCOSEnded)
			return;
		m_bDCOSEnded = true;
		IEntity me = m_Utility.m_OwnerEntity;
		DCO_Search.Release(me);
		bool found = DCO_SearchFound(me, "end");
		string phase = "approach";
		if (m_iDCOSPhase == 2)
			phase = "search";
		else if (m_iDCOSPhase == 3)
			phase = "cleared";
		DCO_Perf.Count("search:end_" + reason);
		if (!found && m_iDCOSPhase == 2)
			DCO_Perf.Count("search:cut");
		if (DCO_Log.On(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO))
			DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("search_end unit=%1 reason=%2 phase=%3 found=%4 spots=%5 corners=%6 head_on=%7 dur_s=%8",
				DCO_Log.IdUnit(me), reason, phase, found, m_iDCOSSpots, m_iDCOSCorners, m_iDCOSHeadOn, Math.Round((GetGame().GetWorld().GetWorldTime() - m_fDCOSStart_ms) / 1000.0)));
	}
}
