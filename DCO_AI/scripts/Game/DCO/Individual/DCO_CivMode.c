enum DCO_ECivState
{
	CALM,
	FREEZE,
	FLEE,
	HIDE,
	CAUTIOUS
}

class DCO_CivData
{
	int m_iState;
	float m_fStateAt;
	float m_fStateEnd;
	float m_fLastDanger = -1000000.0;
	float m_fLastExpl = -1000000.0;
	float m_fNextTick;
	float m_fCalmJitter;
	bool m_bRunSet;
	bool m_bWalk;
	vector m_vThreat;
	vector m_vDest;
	vector m_vFrom;
}

class DCO_CivMoveBehavior : SCR_AIMoveIndividuallyBehavior
{
	void DCO_CivMoveBehavior(SCR_AIUtilityComponent utility, SCR_AIActivityBase groupActivity, vector pos, float priority = PRIORITY_BEHAVIOR_MOVE_INDIVIDUALLY, float priorityLevel = PRIORITY_LEVEL_NORMAL, IEntity ent = null, float radius = 1.0)
	{
	}
}

class DCO_CivMode
{
	protected static ref map<SCR_AIUtilityComponent, ref DCO_CivData> s_mData;
	protected static ref array<IEntity> s_aCombatants;
	protected static float s_fHoldLogAt;
	protected static float s_fFrameAt;
	protected static int s_iFrameUsed;
	protected static ref map<int, vector> s_mCellDest;
	protected static ref map<int, float> s_mCellExp;

	protected static bool Budget(int maxPerFrame)
	{
		float now = GetGame().GetWorld().GetWorldTime();
		if (now != s_fFrameAt)
		{
			s_fFrameAt = now;
			s_iFrameUsed = 0;
		}
		if (s_iFrameUsed >= maxPerFrame)
		{
			DCO_Perf.Count("civ:budget_defer");
			return false;
		}
		s_iFrameUsed++;
		return true;
	}

	protected static int CellKey(vector me, vector threat)
	{
		int cx = Math.Floor(me[0] / 25.0);
		int cz = Math.Floor(me[2] / 25.0);
		int tx = Math.Floor(threat[0] / 50.0);
		int tz = Math.Floor(threat[2] / 50.0);
		return ((cx * 7919 + cz) * 7919 + tx) * 7919 + tz;
	}

	static float CautionMul(IEntity shooter)
	{
		if (!shooter)
			return 1.0;
		float c = DCO_Ctx.TuneFor(shooter, "civ.caution", 1.0);
		if (c < 0.5)
			return 0.0;
		if (c > 1.5)
			return 1.5;
		return 1.0;
	}

	static string CautionName(float mul)
	{
		if (mul <= 0)
			return "ignore";
		if (mul > 1.0)
			return "strict";
		return "normal";
	}

	static bool On(IEntity e)
	{
		return e && DCO_Ctx.TuneFor(e, "civ.mode", 1.0) > 0;
	}

	static bool CivGroup(AIGroup g)
	{
		SCR_AIGroup sg = SCR_AIGroup.Cast(g);
		if (!sg || !On(sg))
			return false;
		SCR_Faction f = SCR_Faction.Cast(sg.GetFaction());
		return f && !f.IsMilitary();
	}

	protected static DCO_CivData Data(SCR_AIUtilityComponent u)
	{
		if (!s_mData)
			s_mData = new map<SCR_AIUtilityComponent, ref DCO_CivData>();
		DCO_CivData d = s_mData.Get(u);
		if (d)
			return d;
		if (s_mData.Count() > 1024)
		{
			array<SCR_AIUtilityComponent> dead = {};
			foreach (SCR_AIUtilityComponent k, DCO_CivData v : s_mData)
			{
				if (!k || !k.m_OwnerEntity)
					dead.Insert(k);
			}
			foreach (SCR_AIUtilityComponent dk : dead)
				s_mData.Remove(dk);
		}
		d = new DCO_CivData();
		d.m_fCalmJitter = Math.RandomFloat(-0.25, 0.25);
		s_mData.Set(u, d);
		return d;
	}

	static bool IsCiv(SCR_AIUtilityComponent u)
	{
		if (!u || !u.m_OwnerEntity)
			return false;
		float now = GetGame().GetWorld().GetWorldTime();
		if (u.m_iDCOCiv >= 0 && now < u.m_fDCOCivAt)
			return u.m_iDCOCiv == 1;
		u.m_fDCOCivAt = now + 5000.0 + Math.RandomFloat(0, 1000.0);
		bool civ = !u.IsMilitary() && On(u.m_OwnerEntity) && !DCO_Util.HoldsWeapon(u.m_OwnerEntity);
		if (u.m_iDCOCiv == 1 && !civ)
		{
			EndMove(u);
			Data(u).m_iState = DCO_ECivState.CALM;
			DCO_Perf.Count("civ:armed_left_mode");
			DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(u.m_OwnerEntity), string.Format("civ_armed unit=%1 action=left_civilian_mode", DCO_Log.IdUnit(u.m_OwnerEntity)));
		}
		u.m_iDCOCiv = 0;
		if (civ)
			u.m_iDCOCiv = 1;
		return civ;
	}

	static bool OnShot(SCR_AIUtilityComponent u, AIDangerEvent ev)
	{
		AIDangerEventWeaponFire wf = AIDangerEventWeaponFire.Cast(ev);
		if (!wf)
			return true;
		vector src = wf.GetPosition();
		vector me = u.m_OwnerEntity.GetOrigin();
		vector dir = wf.GetDirection();
		vector rel = me - src;
		float along = vector.Dot(rel, dir);
		bool close = false;
		if (along > 0)
			close = (rel - dir * along).Length() < 10.0;
		Danger(u, src, "shot", vector.Distance(me, src), close);
		return true;
	}

	static bool OnExplosion(SCR_AIUtilityComponent u, AIDangerEvent ev)
	{
		vector src = ev.GetPosition();
		float d = vector.Distance(u.m_OwnerEntity.GetOrigin(), src);
		Danger(u, src, "explosion", d, d < 50.0);
		return true;
	}

	static bool OnHit(SCR_AIUtilityComponent u, AIDangerEvent ev)
	{
		IEntity shooter = ev.GetObject();
		vector src = u.m_OwnerEntity.GetOrigin();
		if (shooter)
			src = shooter.GetOrigin();
		Danger(u, src, "hit", vector.Distance(u.m_OwnerEntity.GetOrigin(), src), true);
		return true;
	}

	protected static void Danger(SCR_AIUtilityComponent u, vector src, string kind, float dist, bool close)
	{
		if (dist > 500.0 && !close)
			return;
		DCO_CivData c = Data(u);
		float now = GetGame().GetWorld().GetWorldTime();
		c.m_fLastDanger = now;
		c.m_vThreat = src;
		if (kind == "explosion")
			c.m_fLastExpl = now;
		int st = c.m_iState;
		if (st == DCO_ECivState.FLEE || st == DCO_ECivState.HIDE || st == DCO_ECivState.FREEZE)
		{
			DCO_Perf.Count("civ:danger_refresh");
			return;
		}
		IEntity me = u.m_OwnerEntity;
		if (dist > 300.0 && !close)
		{
			if (st == DCO_ECivState.CALM)
			{
				c.m_bWalk = true;
				c.m_fStateEnd = now + Math.RandomFloat(1000.0, 3000.0);
				SetState(u, c, DCO_ECivState.FREEZE, now, kind + "_far", dist);
			}
			return;
		}
		if (DCO_CoverManagerComponent.IsEntityInsideBuilding(me))
		{
			SetStance(u, ECharacterStance.CROUCH);
			SetState(u, c, DCO_ECivState.HIDE, now, kind, dist);
			return;
		}
		c.m_bWalk = false;
		float freezeMs = Math.RandomFloat(500.0, 2000.0);
		if (!close)
			freezeMs = freezeMs + Math.RandomFloat(1000.0, 4000.0);
		else
		{
			vector cov;
			if (!DCO_Stress.FindCover(me, src, 30.0, cov))
			{
				SetStance(u, ECharacterStance.PRONE);
				freezeMs = Math.RandomFloat(3000.0, 6000.0);
				DCO_Perf.Count("civ:prone");
			}
			else
				freezeMs = Math.RandomFloat(300.0, 800.0);
		}
		c.m_fStateEnd = now + freezeMs;
		SetState(u, c, DCO_ECivState.FREEZE, now, kind, dist);
	}

	static void Tick(SCR_AIUtilityComponent u, float now)
	{
		DCO_CivData c = Data(u);
		if (c.m_iState == DCO_ECivState.CALM || now < c.m_fNextTick || !Budget(25))
			return;
		c.m_fNextTick = now + 500.0;
		int pt = DCO_Perf.Begin();
		IEntity me = u.m_OwnerEntity;
		float quiet = now - c.m_fLastDanger;
		float hideMs = DCO_Ctx.TuneFor(me, "civ.hide_s", 180.0) * 1000.0;
		if (now - c.m_fLastExpl < hideMs + 120000.0)
			hideMs = hideMs * 5.0 / 3.0;
		float cautMs = DCO_Ctx.TuneFor(me, "civ.cautious_s", 240.0) * 1000.0;
		if (c.m_iState == DCO_ECivState.FREEZE)
		{
			if (now >= c.m_fStateEnd)
			{
				if (DCO_CoverManagerComponent.IsEntityInsideBuilding(me))
				{
					SetStance(u, ECharacterStance.CROUCH);
					SetState(u, c, DCO_ECivState.HIDE, now, "indoor", 0);
				}
				else
				{
					SetStance(u, ECharacterStance.STAND);
					StartFlee(u, c, now, "startle_end", vector.Distance(me.GetOrigin(), c.m_vThreat));
					SetState(u, c, DCO_ECivState.FLEE, now, "startle_end", 0);
				}
			}
		}
		else if (c.m_iState == DCO_ECivState.FLEE)
		{
			float fromD = vector.DistanceXZ(me.GetOrigin(), c.m_vFrom);
			if (!c.m_bRunSet && fromD > 30.0)
			{
				c.m_bRunSet = true;
				SetMoveType(u, EMovementType.RUN);
			}
			DCO_CivMoveBehavior mv = DCO_CivMoveBehavior.Cast(u.FindActionOfType(DCO_CivMoveBehavior));
			bool arrived = vector.DistanceXZ(me.GetOrigin(), c.m_vDest) < 4.0;
			if (!mv || arrived || now - c.m_fStateAt > 90000.0)
			{
				EndMove(u);
				if (DCO_CoverManagerComponent.IsEntityInsideBuilding(me))
					SetStance(u, ECharacterStance.CROUCH);
				string why = "arrived";
				if (!arrived)
					why = "move_end";
				SetState(u, c, DCO_ECivState.HIDE, now, why, fromD);
			}
		}
		else if (c.m_iState == DCO_ECivState.HIDE)
		{
			if (quiet > hideMs && now - c.m_fStateAt > Math.Min(hideMs, 15000.0))
			{
				SetStance(u, ECharacterStance.STAND);
				SetState(u, c, DCO_ECivState.CAUTIOUS, now, "quiet", quiet / 1000.0);
			}
		}
		else if (c.m_iState == DCO_ECivState.CAUTIOUS)
		{
			bool calmDue = quiet > hideMs && now - c.m_fStateAt > cautMs * (1.0 + c.m_fCalmJitter);
			if (calmDue && SoldierNear(me, 50.0))
			{
				c.m_fNextTick = now + 5000.0;
				DCO_Perf.Count("civ:calm_wait_soldier");
			}
			else if (calmDue)
			{
				EndMove(u);
				SetStance(u, ECharacterStance.STAND);
				SetState(u, c, DCO_ECivState.CALM, now, "recovered", quiet / 1000.0);
			}
		}
		DCO_Perf.End("civ_tick", pt);
	}

	protected static void SetState(SCR_AIUtilityComponent u, DCO_CivData c, int to, float now, string reason, float dist)
	{
		int from = c.m_iState;
		c.m_iState = to;
		c.m_fStateAt = now;
		string toName = typename.EnumToString(DCO_ECivState, to);
		DCO_Perf.Count("civ:to_" + toName);
		DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(u.m_OwnerEntity), string.Format("civ_state unit=%1 from=%2 to=%3 reason=%4 d=%5 threat=%6", DCO_Log.IdUnit(u.m_OwnerEntity), typename.EnumToString(DCO_ECivState, from), toName, reason, Math.Round(dist), DCO_Util.XZ(c.m_vThreat)));
	}

	protected static void StartFlee(SCR_AIUtilityComponent u, DCO_CivData c, float now, string why, float dist)
	{
		IEntity me = u.m_OwnerEntity;
		string src = "away";
		vector dest;
		int cell = CellKey(me.GetOrigin(), c.m_vThreat);
		vector cached;
		float exp;
		if (HerdDest(u, dest))
			src = "herd";
		else if (s_mCellDest && s_mCellDest.Find(cell, cached) && s_mCellExp.Find(cell, exp) && now < exp)
		{
			dest = cached + Vector(Math.RandomFloat(-4.0, 4.0), 0, Math.RandomFloat(-4.0, 4.0));
			src = "cell";
		}
		else
		{
			if (BuildingDest(me, c.m_vThreat, dest))
				src = "building";
			else
				dest = DCO_Stress.AwayPoint(me, c.m_vThreat, Math.RandomFloat(80.0, 150.0), 40.0);
			vector snapped;
			if (DCO_FlushDrill.Reachable(dest, snapped))
				dest = snapped;
			if (!s_mCellDest)
			{
				s_mCellDest = new map<int, vector>();
				s_mCellExp = new map<int, float>();
			}
			if (s_mCellDest.Count() > 512)
			{
				s_mCellDest.Clear();
				s_mCellExp.Clear();
			}
			s_mCellDest.Set(cell, dest);
			s_mCellExp.Set(cell, now + 30000.0);
		}
		c.m_vDest = dest;
		c.m_vFrom = me.GetOrigin();
		c.m_bRunSet = false;
		EndMove(u);
		u.AddAction(new DCO_CivMoveBehavior(u, null, dest, 85.0, radius: 3.0));
		if (c.m_bWalk)
			SetMoveType(u, EMovementType.WALK);
		else
			SetMoveType(u, EMovementType.SPRINT);
		DCO_Perf.Count("civ:flee_" + src);
		DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("civ_flee unit=%1 dest=%2 src=%3 dist=%4 threat_d=%5 walk=%6 why=%7", DCO_Log.IdUnit(me), DCO_Util.XZ(dest), src, Math.Round(vector.DistanceXZ(me.GetOrigin(), dest)), Math.Round(dist), DCO_Util.B01(c.m_bWalk), why));
	}

	protected static bool HerdDest(SCR_AIUtilityComponent u, out vector dest)
	{
		AIAgent ag = u.GetAIAgent();
		if (!ag || !ag.GetParentGroup())
			return false;
		array<AIAgent> agents = {};
		ag.GetParentGroup().GetAgents(agents);
		vector me = u.m_OwnerEntity.GetOrigin();
		foreach (AIAgent a : agents)
		{
			SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
			if (!ca || ca == ag || !ca.m_UtilityComponent || !ca.GetControlledEntity())
				continue;
			if (!s_mData || vector.DistanceXZ(ca.GetControlledEntity().GetOrigin(), me) > 10.0)
				continue;
			DCO_CivData o = s_mData.Get(ca.m_UtilityComponent);
			if (o && o.m_iState == DCO_ECivState.FLEE && o.m_vDest != vector.Zero)
			{
				dest = o.m_vDest + Vector(Math.RandomFloat(-3.0, 3.0), 0, Math.RandomFloat(-3.0, 3.0));
				return true;
			}
		}
		return false;
	}

	protected static bool BuildingDest(IEntity me, vector threat, out vector dest)
	{
		vector p = me.GetOrigin();
		vector away = p - threat;
		away[1] = 0;
		if (away.Length() < 0.5)
			return false;
		away.Normalize();
		IEntity b = DCO_IndoorNav.NearestBuilding(p + away * 30.0, 35.0);
		if (!b)
			return false;
		vector to = b.GetOrigin() - p;
		to[1] = 0;
		if (to.Length() > 1.0 && vector.Dot(to.Normalized(), away) < 0)
			return false;
		if (vector.DistanceXZ(b.GetOrigin(), threat) < 30.0 || SoldierNear(b, 12.0))
			return false;
		dest = b.GetOrigin();
		return true;
	}

	protected static bool SoldierNear(IEntity at, float radius)
	{
		if (!s_aCombatants)
			s_aCombatants = {};
		s_aCombatants.Clear();
		DCO_Perf.Count("q:DCO_CivMode");
		GetGame().GetWorld().QueryEntitiesBySphere(at.GetOrigin(), radius, OnCombatant, null, EQueryEntitiesFlags.DYNAMIC);
		bool found = !s_aCombatants.IsEmpty();
		s_aCombatants.Clear();
		return found;
	}

	protected static bool OnCombatant(IEntity e)
	{
		if (!ChimeraCharacter.Cast(e) || !DCO_Util.IsAlive(e))
			return true;
		FactionAffiliationComponent fa = FactionAffiliationComponent.Cast(e.FindComponent(FactionAffiliationComponent));
		if (!fa)
			return true;
		SCR_Faction f = SCR_Faction.Cast(fa.GetAffiliatedFaction());
		if (f && f.IsMilitary())
		{
			s_aCombatants.Insert(e);
			return false;
		}
		return true;
	}

	protected static void EndMove(SCR_AIUtilityComponent u)
	{
		DCO_CivMoveBehavior mv = DCO_CivMoveBehavior.Cast(u.FindActionOfType(DCO_CivMoveBehavior));
		if (mv)
			mv.Complete();
	}

	protected static void SetStance(SCR_AIUtilityComponent u, ECharacterStance stance)
	{
		CharacterControllerComponent ctrl = CharacterControllerComponent.Cast(u.m_OwnerEntity.FindComponent(CharacterControllerComponent));
		if (ctrl)
			DCO_Stance.Set(ctrl, stance, "civ");
	}

	protected static void SetMoveType(SCR_AIUtilityComponent u, EMovementType t)
	{
		AIAgent ag = u.GetAIAgent();
		if (!ag)
			return;
		AICharacterMovementComponent move = AICharacterMovementComponent.Cast(ag.GetMovementComponent());
		if (move)
			move.SetMovementTypeWanted(t);
	}

	static BaseTarget TargetGate(SCR_AIUtilityComponent u, BaseTarget t)
	{
		if (!t || !u || !u.m_OwnerEntity || !t.GetTargetEntity())
			return t;
		IEntity te = t.GetTargetEntity();
		if (!DCO_Util.IsCivilianEnt(te) || !On(u.m_OwnerEntity))
			return t;
		float now = GetGame().GetWorld().GetWorldTime();
		if (now - DCO_ShotCounter.LastShotAt(te) < 120000.0)
		{
			DCO_Perf.Count("civ:armed_hostile");
			return t;
		}
		bool armed = DCO_Util.HoldsWeapon(te);
		if (armed)
			DCO_Perf.Count("civ:armed_watch");
		else
			DCO_Perf.Count("civ:target_hold");
		if (now - s_fHoldLogAt > 2000.0)
		{
			s_fHoldLogAt = now;
			string ev = "fire_hold_civ";
			if (armed)
				ev = "civ_armed";
			DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(u.m_OwnerEntity), string.Format("%1 unit=%2 civ=%3 armed=%4 dist=%5 action=hold_no_hostile_act", ev, DCO_Log.IdUnit(u.m_OwnerEntity), DCO_Log.IdUnit(te), DCO_Util.B01(armed), Math.Round(vector.Distance(u.m_OwnerEntity.GetOrigin(), te.GetOrigin()))));
		}
		return null;
	}
}
