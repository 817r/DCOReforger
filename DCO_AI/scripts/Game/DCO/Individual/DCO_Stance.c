class DCO_StanceReq
{
	int m_iTo;
	string m_sWho;
	float m_fAt_ms;
	int m_iInterrupts;
	int m_iBlocked;
	bool m_bPending;
	bool m_bBraked;
}

class DCO_StanceHold
{
	float m_fUntil_ms;
	int m_iStance;
	int m_iPrio;
	string m_sWho;
}

class DCO_StanceTrack
{
	ref DCO_StanceReq m_Req;
	float m_fStart_ms;
	float m_fPrev_ms;
	float m_fH0;
	float m_fHPrev;
	vector m_vPrev;
	float m_fSpd0;
	float m_fMaxStep;
	float m_fMaxAt_ms;
	float m_fSpdAtMax;
	float m_fHAtMax;
	int m_iStanceAtMax;
	int m_iFrames;
}

class DCO_Stance
{
	protected static ref map<IEntity, ref DCO_StanceReq> s_mReq;
	protected static ref array<ref DCO_StanceTrack> s_aTracks;
	protected static ref map<IEntity, ref DCO_StanceHold> s_mHold;

	static int SourcePrio(string who)
	{
		if (who == "devcmd")
			return 9;
		if (who == "navguard")
			return 8;
		if (who == "blast")
			return 7;
		if (who == "hit" || who == "pin")
			return 6;
		if (who == "fired_at")
			return 5;
		if (who == "longrange")
			return 4;
		if (who == "bt" || who == "idle")
			return 1;
		return 3;
	}

	static float HoldMs(string who)
	{
		if (who == "blast" || who == "hit" || who == "pin")
			return 8000.0;
		if (who == "fired_at")
			return 6000.0;
		return 0;
	}

	static bool Set(CharacterControllerComponent ctrl, ECharacterStance stance, string who)
	{
		return Request(ctrl, SCR_AIStanceHandling.ConvertStanceToStanceChange(stance), who);
	}

	static float Speed(CharacterControllerComponent ctrl)
	{
		vector v = ctrl.GetVelocity();
		return Math.Sqrt(v[0] * v[0] + v[2] * v[2]);
	}

	static bool Request(CharacterControllerComponent ctrl, int change, string who)
	{
		if (!ctrl || change < 1 || change > 3)
			return false;
		IEntity me = ctrl.GetOwner();
		if (!me)
			return false;
		int to = change - 1;
		int cur = ctrl.GetStance();
		bool changing = ctrl.IsChangingStance();
		if (!s_mReq)
			s_mReq = new map<IEntity, ref DCO_StanceReq>();
		DCO_StanceReq last = s_mReq.Get(me);
		float now = GetGame().GetWorld().GetWorldTime();

		int prio = SourcePrio(who);
		if (!s_mHold)
			s_mHold = new map<IEntity, ref DCO_StanceHold>();
		DCO_StanceHold hold = s_mHold.Get(me);
		if (hold && now < hold.m_fUntil_ms && to < hold.m_iStance && prio < hold.m_iPrio)
		{
			DCO_Perf.Count("stance:hold_blocked_" + who);
			if (DCO_Log.On(DCO_ELogChannel.UNIT, DCO_ELogLevel.TRACE))
				DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.TRACE, DCO_Log.IdUnit(me), string.Format("stance_hold_blocked unit=%1 who=%2 to=%3 hold_who=%4 hold=%5 left_ms=%6", DCO_Log.IdUnit(me), who, Name(to), hold.m_sWho, Name(hold.m_iStance), Math.Round(hold.m_fUntil_ms - now)));
			return false;
		}
		float holdMs = HoldMs(who);
		if (holdMs > 0 && to > ECharacterStance.STAND)
		{
			if (!hold)
			{
				hold = new DCO_StanceHold();
				s_mHold.Set(me, hold);
			}
			if (now >= hold.m_fUntil_ms || to >= hold.m_iStance || prio >= hold.m_iPrio)
			{
				hold.m_iStance = to;
				hold.m_iPrio = prio;
				hold.m_sWho = who;
			}
			hold.m_fUntil_ms = Math.Max(hold.m_fUntil_ms, now + holdMs);
			DCO_Perf.Count("stance:hold_set_" + who);
		}

		if (last && to < last.m_iTo && prio <= 3 && now - last.m_fAt_ms < 1500.0)
		{
			DCO_Perf.Count("stance:raise_hysteresis_" + who);
			return false;
		}

		if (last && last.m_bPending && last.m_iTo == to)
		{
			DCO_Perf.Count("stance:dup_pending");
			return false;
		}
		if (changing && last)
		{
			if (last.m_iTo == to)
			{
				DCO_Perf.Count("stance:dup_in_transition");
				return false;
			}
			if (now - last.m_fAt_ms < 1500)
			{
				DCO_Perf.Count("stance:interrupt_blocked");
				if (DCO_Log.On(DCO_ELogChannel.UNIT, DCO_ELogLevel.DEBUG))
					DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(me), string.Format("stance_interrupt_blocked unit=%1 who=%2 to=%3 running_who=%4 running_to=%5 running_ms=%6",
						DCO_Log.IdUnit(me), who, Name(to), last.m_sWho, Name(last.m_iTo), Math.Round(now - last.m_fAt_ms)));
				return false;
			}
			last.m_iBlocked++;
			DCO_Perf.Count("stance:interrupt");
			if (DCO_Log.On(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO))
				DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("stance_interrupt unit=%1 who=%2 to=%3 running_who=%4 running_to=%5 running_ms=%6",
					DCO_Log.IdUnit(me), who, Name(to), last.m_sWho, Name(last.m_iTo), Math.Round(now - last.m_fAt_ms)));
		}
		if (!changing && cur == to && !(last && last.m_bPending))
			return false;

		if (last && last.m_iTo != to && now - last.m_fAt_ms < 2000)
		{
			DCO_Perf.Count("stance:flip");
			DCO_Perf.Count("stance:flip_" + who);
		}

		if (last && last.m_bBraked)
		{
			ctrl.SetDisableMovementControls(false);
			last.m_bBraked = false;
			DCO_Perf.Count("stance:brake_superseded");
		}

		DCO_StanceReq req = new DCO_StanceReq();
		req.m_iTo = to;
		req.m_sWho = who;
		req.m_fAt_ms = now;
		s_mReq.Set(me, req);

		float spd = Speed(ctrl);
		if (to == ECharacterStance.PRONE && cur != ECharacterStance.PRONE && spd >= 1.2 && !ctrl.GetDisableMovementControls())
		{
			req.m_bPending = true;
			req.m_bBraked = true;
			ctrl.SetDisableMovementControls(true);
			DCO_Perf.Count("stance:brake");
			if (DCO_Log.On(DCO_ELogChannel.UNIT, DCO_ELogLevel.DEBUG))
				DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(me), string.Format("stance_brake unit=%1 who=%2 from=%3 speed=%4", DCO_Log.IdUnit(me), who, Name(cur), spd.ToString(1, 1)));
			GetGame().GetCallqueue().CallLater(Deferred, 50, false, ctrl, req);
			return true;
		}
		Issue(ctrl, req, cur, spd);
		return true;
	}

	static void Deferred(CharacterControllerComponent ctrl, DCO_StanceReq req)
	{
		if (!ctrl || !req || !req.m_bPending || DCO_Util.Teardown("stance_defer"))
			return;
		float now = GetGame().GetWorld().GetWorldTime();
		float waited = now - req.m_fAt_ms;
		float spd = Speed(ctrl);
		if (spd > 0.4 && waited < 600)
		{
			GetGame().GetCallqueue().CallLater(Deferred, 50, false, ctrl, req);
			return;
		}
		req.m_bPending = false;
		DCO_Perf.Count("stance:prone_deferred");
		if (spd > 0.4)
			DCO_Perf.Count("stance:prone_deferred_still_moving");
		if (DCO_Log.On(DCO_ELogChannel.UNIT, DCO_ELogLevel.DEBUG))
			DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(ctrl.GetOwner()), string.Format("stance_prone_deferred unit=%1 who=%2 waited_ms=%3 speed=%4", DCO_Log.IdUnit(ctrl.GetOwner()), req.m_sWho, Math.Round(waited), spd.ToString(1, 1)));
		Issue(ctrl, req, ctrl.GetStance(), spd);
	}

	static void Issue(CharacterControllerComponent ctrl, DCO_StanceReq req, int cur, float spd)
	{
		IEntity me = ctrl.GetOwner();
		float now = GetGame().GetWorld().GetWorldTime();
		ctrl.SetStanceChange(req.m_iTo + 1);
		DCO_Perf.Count("stance:req_" + req.m_sWho);
		if (DCO_Log.On(DCO_ELogChannel.UNIT, DCO_ELogLevel.DEBUG))
			DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(me), string.Format("stance_req unit=%1 who=%2 from=%3 to=%4 speed=%5", DCO_Log.IdUnit(me), req.m_sWho, Name(cur), Name(req.m_iTo), spd.ToString(1, 1)));
		if (req.m_bBraked)
			GetGame().GetCallqueue().CallLater(BrakeRelease, 100, false, ctrl, req, now);

		ChimeraCharacter ch = ChimeraCharacter.Cast(me);
		if (ch && req.m_iTo == ECharacterStance.PRONE && cur == ECharacterStance.STAND)
		{
			DCO_StanceTrack tr = new DCO_StanceTrack();
			tr.m_Req = req;
			tr.m_fStart_ms = now;
			tr.m_fPrev_ms = now;
			tr.m_fH0 = ch.EyePosition()[1] - ch.GetOrigin()[1];
			tr.m_fHPrev = tr.m_fH0;
			tr.m_vPrev = ch.GetOrigin();
			tr.m_fSpd0 = spd;
			if (!s_aTracks)
				s_aTracks = {};
			s_aTracks.Insert(tr);
			GetGame().GetCallqueue().CallLater(Track, 0, false, ch, tr);
		}
	}

	static void Track(ChimeraCharacter ch, DCO_StanceTrack tr)
	{
		if (!tr)
			return;
		if (!ch || DCO_Util.Teardown("stance_track"))
		{
			s_aTracks.RemoveItem(tr);
			return;
		}
		float now = GetGame().GetWorld().GetWorldTime();
		float dt = (now - tr.m_fPrev_ms) / 1000.0;
		float h = ch.EyePosition()[1] - ch.GetOrigin()[1];
		vector p = ch.GetOrigin();
		float spd = 0;
		if (dt > 0.001)
			spd = vector.DistanceXZ(p, tr.m_vPrev) / dt;
		float step = tr.m_fHPrev - h;
		if (step > tr.m_fMaxStep)
		{
			tr.m_fMaxStep = step;
			tr.m_fMaxAt_ms = now - tr.m_fStart_ms;
			tr.m_fSpdAtMax = spd;
			tr.m_fHAtMax = h;
			CharacterControllerComponent ctrl = ch.GetCharacterController();
			if (ctrl)
				tr.m_iStanceAtMax = ctrl.GetStance();
		}
		tr.m_iFrames++;
		tr.m_fHPrev = h;
		tr.m_vPrev = p;
		tr.m_fPrev_ms = now;
		float el = now - tr.m_fStart_ms;
		if (h > 0.75 && el < 3000)
		{
			GetGame().GetCallqueue().CallLater(Track, 0, false, ch, tr);
			return;
		}
		bool snap = tr.m_fMaxStep > 0.15;
		DCO_Perf.Count("stance:drop");
		if (snap)
			DCO_Perf.Count("stance:drop_snap_" + tr.m_Req.m_sWho);
		if (DCO_Log.On(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO))
			DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(ch), string.Format("stance_drop unit=%1 who=%2 dur_ms=%3 frames=%4 h0=%5 h1=%6 snap=%7 interrupts=%8",
				DCO_Log.IdUnit(ch), tr.m_Req.m_sWho, Math.Round(el), tr.m_iFrames, tr.m_fH0.ToString(1, 2), h.ToString(1, 2), snap, tr.m_Req.m_iBlocked)
				+ string.Format(" spd0=%1 max_step=%2 max_at_ms=%3 spd_at_max=%4 h_at_max=%5 stance_at_max=%6", tr.m_fSpd0.ToString(1, 1), tr.m_fMaxStep.ToString(1, 2), Math.Round(tr.m_fMaxAt_ms), tr.m_fSpdAtMax.ToString(1, 1), tr.m_fHAtMax.ToString(1, 2), Name(tr.m_iStanceAtMax)));
		s_aTracks.RemoveItem(tr);
	}

	static void BrakeRelease(CharacterControllerComponent ctrl, DCO_StanceReq req, float start_ms)
	{
		if (!ctrl || !req || !req.m_bBraked || DCO_Util.Teardown("stance_brake"))
			return;
		float held = GetGame().GetWorld().GetWorldTime() - start_ms;
		ChimeraCharacter ch = ChimeraCharacter.Cast(ctrl.GetOwner());
		bool done = ch && !ctrl.IsChangingStance() && ctrl.GetStance() == ECharacterStance.PRONE && ch.EyePosition()[1] - ch.GetOrigin()[1] < 0.8;
		if (!done && held < 1200)
		{
			GetGame().GetCallqueue().CallLater(BrakeRelease, 50, false, ctrl, req, start_ms);
			return;
		}
		ctrl.SetDisableMovementControls(false);
		req.m_bBraked = false;
		DCO_Perf.Count("stance:brake_release");
		if (!done)
			DCO_Perf.Count("stance:brake_timeout");
		if (DCO_Log.On(DCO_ELogChannel.UNIT, DCO_ELogLevel.DEBUG))
			DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(ctrl.GetOwner()), string.Format("stance_brake_release unit=%1 who=%2 held_ms=%3 prone=%4", DCO_Log.IdUnit(ctrl.GetOwner()), req.m_sWho, Math.Round(held), done));
	}

	static string Name(int stance)
	{
		if (stance == ECharacterStance.STAND)
			return "STAND";
		if (stance == ECharacterStance.CROUCH)
			return "CROUCH";
		return "PRONE";
	}
}
