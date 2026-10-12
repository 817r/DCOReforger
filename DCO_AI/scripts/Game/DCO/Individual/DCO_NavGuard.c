modded class SCR_CharacterControllerComponent
{
	float m_fDCONavHold_ms;
	float m_fDCONavFaceUntil_ms;
	float m_fDCONavNext_ms;
	float m_fDCONavStill_ms;
	vector m_vDCONavPos;
	int m_iDCONavRetry;
	bool m_bDCONavStallLogged;
	int m_iDCONavCmd;
	float m_fDCONavCmdStart_ms;
	vector m_vDCONavCmdFrom;
}

modded class SCR_CharacterCommandHandlerComponent
{
	override void OnCommandActivate(int pCmdId)
	{
		super.OnCommandActivate(pCmdId);
		DCO_Nav.OnCommand(m_OwnerEntity, m_CharacterControllerComp, pCmdId);
	}

	override protected void OnInit(IEntity owner)
	{
		super.OnInit(owner);
		EventHandlerManagerComponent ev = EventHandlerManagerComponent.Cast(owner.FindComponent(EventHandlerManagerComponent));
		if (!ev)
			return;
		ev.RegisterScriptHandler("OnGrenadeThrown", this, DCO_OnGrenadeThrown);
		ev.RegisterScriptHandler("OnProjectileShot", this, DCO_OnProjectileShot);
	}

	void DCO_OnGrenadeThrown(int playerID, BaseWeaponComponent weapon, IEntity entity)
	{
		DCO_Perf.Count("smoke:ev_grenade_thrown");
		if (weapon && entity && weapon.GetWeaponType() == EWeaponType.WT_SMOKEGRENADE)
			DCO_SmokeField.Add(entity, entity.GetOrigin(), "hand", 2.5, 55.0);
	}

	protected static ref array<string> s_aDCOProjSeen;

	void DCO_OnProjectileShot(int playerID, BaseWeaponComponent weapon, IEntity entity)
	{
		if (!entity)
		{
			DCO_Perf.Count("smoke:ev_no_entity");
			return;
		}
		EntityPrefabData pd = entity.GetPrefabData();
		if (!pd)
		{
			DCO_Perf.Count("smoke:ev_no_prefab");
			return;
		}
		string name = pd.GetPrefabName();
		name.ToLower();
		int wt = -1;
		if (weapon)
			wt = weapon.GetWeaponType();
		DCO_Perf.Count("smoke:ev_shot_any");
		if (!s_aDCOProjSeen)
			s_aDCOProjSeen = {};
		if (!s_aDCOProjSeen.Contains(name) && s_aDCOProjSeen.Count() < 40)
		{
			s_aDCOProjSeen.Insert(name);
			DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(m_OwnerEntity), string.Format("proj_prefab_seen unit=%1 weapon_type=%2 prefab=%3 class=%4", DCO_Log.IdUnit(m_OwnerEntity), wt, pd.GetPrefabName(), entity.ClassName()));
		}
		if (!name.Contains("smoke"))
			return;
		DCO_Perf.Count("smoke:ev_projectile");
		if (DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO))
			DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(m_OwnerEntity), string.Format("smoke_projectile unit=%1 player=%2 weapon_type=%3 prefab=%4 pos=%5",
				DCO_Log.IdUnit(m_OwnerEntity), playerID, wt, pd.GetPrefabName(), DCO_Log.Pos(entity.GetOrigin())));
		DCO_SmokeField.Add(entity, entity.GetOrigin(), "gl", 3.0, 40.0);
		DCO_SmokeField.TrackFast(entity, 0);
	}
}

modded class SCR_AILookAction
{
	override void LookAt(vector pos, float priority, float duration = 0.8)
	{
		if (priority < 99.0 && DCO_Nav.Facing(m_Utility))
		{
			DCO_Perf.Count("navlink:look_blocked");
			return;
		}
		super.LookAt(pos, priority, duration);
	}

	override void LookAt(IEntity ent, float priority, float duration = 0.8)
	{
		if (priority < 99.0 && DCO_Nav.Facing(m_Utility))
		{
			DCO_Perf.Count("navlink:look_blocked");
			return;
		}
		super.LookAt(ent, priority, duration);
	}
}

class DCO_Nav
{
	protected static ref array<vector> s_aPath;

	static string Kind(int cmd)
	{
		if (cmd == ECharacterCommandIDs.LADDER)
			return "ladder";
		return "climb";
	}

	static bool Busy(CharacterControllerComponent ctrl, string what)
	{
		SCR_CharacterControllerComponent s = SCR_CharacterControllerComponent.Cast(ctrl);
		if (!s)
			return false;
		float now = GetGame().GetWorld().GetWorldTime();
		if (s.m_iDCONavCmd == 0 && now >= s.m_fDCONavHold_ms && now >= s.m_fDCONavFaceUntil_ms && !s.IsClimbing())
			return false;
		DCO_Perf.Count("navlink:block_" + what);
		return true;
	}

	static void OnCommand(ChimeraCharacter c, SCR_CharacterControllerComponent ctrl, int cmd)
	{
		if (!c || !ctrl)
			return;
		bool nav = cmd == ECharacterCommandIDs.CLIMB || cmd == ECharacterCommandIDs.LADDER;
		if (!nav && ctrl.m_iDCONavCmd == 0)
			return;
		if (SCR_CharacterHelper.IsPlayer(c))
			return;

		float now = GetGame().GetWorld().GetWorldTime();
		if (nav)
		{
			if (ctrl.m_iDCONavCmd == cmd)
				return;
			ctrl.m_iDCONavCmd = cmd;
			ctrl.m_fDCONavCmdStart_ms = now;
			ctrl.m_vDCONavCmdFrom = c.GetOrigin();
			ctrl.m_iDCONavRetry = 0;
			DCO_Perf.Count("navlink:" + Kind(cmd) + "_start");
			if (DCO_Log.On(DCO_ELogChannel.UNIT, DCO_ELogLevel.DEBUG))
				DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(c), string.Format("navlink_start unit=%1 kind=%2 stance=%3 pos=%4",
					DCO_Log.IdUnit(c), Kind(cmd), typename.EnumToString(ECharacterStance, ctrl.GetStance()), DCO_Log.Pos(c.GetOrigin())));
			return;
		}

		string kind = Kind(ctrl.m_iDCONavCmd);
		float dur = now - ctrl.m_fDCONavCmdStart_ms;
		float moved = vector.Distance(c.GetOrigin(), ctrl.m_vDCONavCmdFrom);
		ctrl.m_iDCONavCmd = 0;
		ctrl.m_fDCONavHold_ms = Math.Max(ctrl.m_fDCONavHold_ms, now + 1000.0);
		DCO_Perf.Count("navlink:" + kind + "_done");
		DCO_ELogLevel lvl = DCO_ELogLevel.DEBUG;
		string result = "ok";
		if (moved < 0.5)
		{
			result = "no_move";
			lvl = DCO_ELogLevel.INFO;
			DCO_Perf.Count("navlink:" + kind + "_no_move");
		}
		if (DCO_Log.On(DCO_ELogChannel.UNIT, lvl))
			DCO_Log.Write(DCO_ELogChannel.UNIT, lvl, DCO_Log.IdUnit(c), string.Format("navlink_end unit=%1 kind=%2 result=%3 dur_ms=%4 moved=%5 next_cmd=%6 stance=%7",
				DCO_Log.IdUnit(c), kind, result, Math.Round(dur), Math.Round(moved * 10) / 10, cmd, typename.EnumToString(ECharacterStance, ctrl.GetStance())));
	}

	static void Tick(SCR_AIUtilityComponent u, float now)
	{
		SCR_CharacterControllerComponent ctrl = u.GetCharacterController();
		if (!ctrl || now < ctrl.m_fDCONavNext_ms)
			return;
		ctrl.m_fDCONavNext_ms = now + 500.0;

		IEntity me = u.m_OwnerEntity;
		vector pos = me.GetOrigin();
		AICharacterMovementComponent move = AICharacterMovementComponent.Cast(u.GetAIAgent().GetMovementComponent());
		bool moving = move && !move.HasCompletedRequest(false);
		bool inVeh = u.m_AIInfo && u.m_AIInfo.HasUnitState(EUnitState.IN_VEHICLE);
		if (!moving || inVeh || ctrl.m_iDCONavCmd != 0 || ctrl.IsClimbing())
		{
			ctrl.m_vDCONavPos = pos;
			ctrl.m_fDCONavStill_ms = now;
			ctrl.m_bDCONavStallLogged = false;
			if (!moving)
				ctrl.m_iDCONavRetry = 0;
			return;
		}

		bool stillNow = vector.Distance(pos, ctrl.m_vDCONavPos) <= 0.35;
		float still = now - ctrl.m_fDCONavStill_ms;

		if (!s_aPath)
			s_aPath = {};
		s_aPath.Clear();
		move.GetCurrentPath(s_aPath);
		vector next = vector.Zero;
		vector next2 = vector.Zero;
		float best = float.MAX;
		int near = -1;
		for (int i = 0; i < s_aPath.Count(); i++)
		{
			float d = vector.DistanceXZ(s_aPath[i], pos);
			if (d < best)
			{
				best = d;
				near = i;
			}
		}
		if (near >= 0)
		{
			for (int j = near; j < s_aPath.Count(); j++)
			{
				if (next == vector.Zero)
				{
					if (vector.DistanceXZ(s_aPath[j], pos) > 0.8)
						next = s_aPath[j];
				}
				else if (vector.DistanceXZ(s_aPath[j], next) > 0.5)
				{
					next2 = s_aPath[j];
					break;
				}
			}
		}

		bool ladder;
		bool blocked;
		float dy;
		float dist = -1;
		vector dir;
		if (next != vector.Zero)
		{
			dy = next[1] - pos[1];
			dist = vector.DistanceXZ(next, pos);
			ladder = Math.AbsFloat(dy) > 1.2 && dist < 1.5;
			if (!ladder && next2 != vector.Zero && dist < 4.0)
				ladder = Math.AbsFloat(next2[1] - next[1]) > 1.2 && vector.DistanceXZ(next2, next) < 1.5;
			dir = next - pos;
			dir[1] = 0;
			dir.Normalize();
			if (dist < 3.0 || (stillNow && still >= 2000.0))
			{
				vector knee = pos + vector.Up * 0.45;
				blocked = DCO_Util.TraceFrac(knee, knee + dir * Math.Min(dist + 1.5, 4.0), me, -1, -1, "trace:navlink") < 1.0;
			}
		}

		if ((blocked || ladder) && next != vector.Zero)
			Face(u, ctrl, next, dir, ladder, blocked, now);

		if (!stillNow)
		{
			ctrl.m_vDCONavPos = pos;
			ctrl.m_fDCONavStill_ms = now;
			ctrl.m_bDCONavStallLogged = false;
			return;
		}

		if (still < 2000.0 || s_aPath.IsEmpty())
			return;

		ECharacterStance stance = ctrl.GetStance();
		if (stance != ECharacterStance.STAND && (blocked || ladder) && ctrl.m_iDCONavRetry < 2)
		{
			ctrl.m_iDCONavRetry++;
			ctrl.m_fDCONavHold_ms = now + 5000.0;
			ctrl.m_fDCONavStill_ms = now;
			DCO_Stance.Set(ctrl, ECharacterStance.STAND, "navguard");
			DCO_Perf.Count("navlink:stand_retry");
			if (DCO_Log.On(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO))
				DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("navlink_stand_retry unit=%1 from=%2 try=%3 blocked=%4 ladder=%5 dy=%6 next_dist=%7 beh=%8",
					DCO_Log.IdUnit(me), typename.EnumToString(ECharacterStance, stance), ctrl.m_iDCONavRetry, blocked, ladder, Math.Round(dy * 10) / 10, Math.Round(dist * 10) / 10, BehName(u)));
			return;
		}

		if (still < 6000.0 || ctrl.m_bDCONavStallLogged)
			return;
		ctrl.m_bDCONavStallLogged = true;
		DCO_Perf.Count("navlink:stall");
		if (blocked || ladder)
			DCO_Perf.Count("navlink:stall_at_obstacle");
		DCO_ELogLevel stallLvl = DCO_ELogLevel.INFO;
		if (!blocked && !ladder)
			stallLvl = DCO_ELogLevel.DEBUG;
		if (!DCO_Log.On(DCO_ELogChannel.UNIT, stallLvl))
			return;
		string threat = "?";
		if (u.m_ThreatSystem)
			threat = typename.EnumToString(EAIThreatState, u.m_ThreatSystem.GetState());
		DCO_Log.Write(DCO_ELogChannel.UNIT, stallLvl, DCO_Log.IdUnit(me), string.Format("navlink_stall unit=%1 still_s=%2 stance=%3 blocked=%4 ladder=%5 dy=%6 next_dist=%7 path_pts=%8 retries=%9",
			DCO_Log.IdUnit(me), Math.Round(still / 1000.0), typename.EnumToString(ECharacterStance, stance), blocked, ladder, Math.Round(dy * 10) / 10, Math.Round(dist * 10) / 10, s_aPath.Count(), ctrl.m_iDCONavRetry)
			+ string.Format(" beh=%1 threat=%2 pos=%3", BehName(u), threat, DCO_Log.Pos(pos)));
	}

	static void Face(SCR_AIUtilityComponent u, SCR_CharacterControllerComponent ctrl, vector next, vector dir, bool ladder, bool blocked, float now)
	{
		if (!u.m_LookAction)
			return;
		bool fresh = now >= ctrl.m_fDCONavFaceUntil_ms;
		ctrl.m_fDCONavFaceUntil_ms = now + 1500.0;
		vector look = next + dir * 2.0;
		look[1] = Math.Max(next[1], u.m_OwnerEntity.GetOrigin()[1]) + 1.5;
		u.m_LookAction.LookAt(look, 99.0, 1.0);
		DCO_Perf.Count("navlink:face");
		if (!fresh)
			return;
		DCO_Perf.Count("navlink:face_start");
		if (DCO_Log.On(DCO_ELogChannel.UNIT, DCO_ELogLevel.DEBUG))
			DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(u.m_OwnerEntity), string.Format("navlink_face unit=%1 blocked=%2 ladder=%3 next_dist=%4 stance=%5 beh=%6",
				DCO_Log.IdUnit(u.m_OwnerEntity), blocked, ladder, Math.Round(vector.DistanceXZ(next, u.m_OwnerEntity.GetOrigin()) * 10) / 10, typename.EnumToString(ECharacterStance, ctrl.GetStance()), BehName(u)));
	}

	static bool Facing(SCR_AIUtilityComponent u)
	{
		if (!u)
			return false;
		SCR_CharacterControllerComponent ctrl = u.GetCharacterController();
		if (!ctrl)
			return false;
		return ctrl.m_iDCONavCmd != 0 || GetGame().GetWorld().GetWorldTime() < ctrl.m_fDCONavFaceUntil_ms || ctrl.IsClimbing();
	}

	static string BehName(SCR_AIUtilityComponent u)
	{
		SCR_AIBehaviorBase b = u.GetCurrentBehavior();
		if (!b)
			return "none";
		return b.ClassName();
	}
}
