enum DCO_EGrenadeThrowMode
{
	NONE,
	DIRECT
}

class DCO_GrenadeUtility
{
	protected static ref map<IEntity, float> s_mLastGrenadeThrowTime;

	protected static ref map<IEntity, float> s_mLastGrenadeRollTime;

	protected static map<IEntity, float> ThrowMap()
	{
		if (!s_mLastGrenadeThrowTime)
			s_mLastGrenadeThrowTime = new map<IEntity, float>();
		return s_mLastGrenadeThrowTime;
	}

	protected static map<IEntity, float> RollMap()
	{
		if (!s_mLastGrenadeRollTime)
			s_mLastGrenadeRollTime = new map<IEntity, float>();
		return s_mLastGrenadeRollTime;
	}

	protected static map<IEntity, IEntity> CachedMap()
	{
		if (!s_mCachedGrenade)
			s_mCachedGrenade = new map<IEntity, IEntity>();
		return s_mCachedGrenade;
	}

	protected static array<IEntity> ItemBuf()
	{
		if (!s_aItemBuffer)
			s_aItemBuffer = {};
		return s_aItemBuffer;
	}




	static bool CanThrowGrenadeNow(SCR_AIUtilityComponent utility)
	{
		if (!utility || !utility.m_OwnerEntity)
			return false;

		IEntity myEntity = utility.m_OwnerEntity;
		float worldTime_ms = GetGame().GetWorld().GetWorldTime();

		float usageScale = DCO_AIConfigComponent.UsageToChanceScale(DCO_AIConfigComponent.GrenadeUsageOf(utility));

		if (usageScale <= 0.0)
			return false;

		float lastThrow;
		if (ThrowMap().Find(myEntity, lastThrow))
		{
			if ((worldTime_ms - lastThrow) < 20000.0)
			{
				DCO_Perf.Count("gren:rej_cooldown");
				return false;
			}
		}

		float lastRoll;
		if (RollMap().Find(myEntity, lastRoll))
		{
			if ((worldTime_ms - lastRoll) < 3000.0)
				return false;
		}

		RollMap().Set(myEntity, worldTime_ms);

		float chance = DCO_PersonalityCombatUtility.GetGrenadeThrowChance(utility);
		chance = Math.Clamp(chance * usageScale, 0.0, 1.0);
		if (Math.RandomFloat01() > chance)
		{
			DCO_Perf.Count("gren:rej_chance");
			return false;
		}

		return true;
	}

	protected static bool IsCooling(IEntity self, float now)
	{
		float t;
		if (ThrowMap().Find(self, t) && now - t < 20000.0)
			return true;
		return RollMap().Find(self, t) && now - t < 3000.0;
	}

	protected static bool ClusterAt(SCR_AIUtilityComponent utility, vector tp, out vector centroid)
	{
		SCR_ChimeraAIAgent agent = SCR_ChimeraAIAgent.Cast(utility.GetOwner());
		if (!agent || !agent.GetParentGroup())
			return false;
		SCR_AIGroupUtilityComponent gu = SCR_AIGroupUtilityComponent.Cast(agent.GetParentGroup().FindComponent(SCR_AIGroupUtilityComponent));
		PerceptionManager pm = GetGame().GetPerceptionManager();
		if (!gu || !gu.m_Perception || !pm)
			return false;
		float pmNow = pm.GetTime();
		vector sum;
		int n;
		foreach (SCR_AITargetInfo t : gu.m_Perception.m_aTargets)
		{
			if (!t || pmNow - t.m_fTimestamp > 5.0)
				continue;
			if (t.m_eCategory != EAITargetInfoCategory.DETECTED && t.m_eCategory != EAITargetInfoCategory.IDENTIFIED)
				continue;
			if (vector.DistanceSqXZ(t.m_vWorldPos, tp) > 25.0)
				continue;
			sum = sum + t.m_vWorldPos;
			n++;
		}
		if (n < 2)
			return false;
		centroid = sum * (1.0 / n);
		float d = vector.DistanceXZ(utility.m_OwnerEntity.GetOrigin(), centroid);
		return d >= 5.0 && d <= 35.0;
	}

	static bool HardCoverBetween(IEntity self, vector tp)
	{
		ChimeraCharacter ch = ChimeraCharacter.Cast(self);
		if (!ch)
			return false;
		float f = DCO_Util.TraceFrac(ch.EyePosition(), tp + Vector(0, 1.0, 0), self, TraceFlags.WORLD | TraceFlags.ENTS, EPhysicsLayerDefs.Projectile, "t:DCO_CloseFrag");
		if (f >= 0.95)
			return false;
		IEntity hit = DCO_Util.LastTraceEnt();
		if (!hit)
			return true;
		return !BaseTree.Cast(hit) && !ChimeraCharacter.Cast(hit.GetRootParent());
	}

	static bool InThrowRange(SCR_AIUtilityComponent utility, float dist)
	{
		if (dist <= 40.0 && dist >= 3.0)
			return true;
		if (!utility || !utility.m_OwnerEntity)
			return false;
		float now = GetGame().GetWorld().GetWorldTime();
		if (!IsCooling(utility.m_OwnerEntity, now))
		{
			RollMap().Set(utility.m_OwnerEntity, now);
			if (dist > 40.0)
				DCO_Perf.Count("gren:rej_far");
			else
				DCO_Perf.Count("gren:rej_close");
		}
		return false;
	}

	protected static ref map<AIGroup, float> s_mOppClusterNext;

	static bool TryOppCluster(SCR_AIUtilityComponent utility)
	{
		if (!utility || !utility.m_OwnerEntity || !utility.m_CombatComponent || !utility.m_CombatComponent.HasWeaponOfType(EWeaponType.WT_FRAGGRENADE))
			return false;
		if (IsCooling(utility.m_OwnerEntity, GetGame().GetWorld().GetWorldTime()))
			return false;
		SCR_ChimeraAIAgent agent = SCR_ChimeraAIAgent.Cast(utility.GetOwner());
		if (!agent || !agent.GetParentGroup())
			return false;
		SCR_AIGroupUtilityComponent gu = SCR_AIGroupUtilityComponent.Cast(agent.GetParentGroup().FindComponent(SCR_AIGroupUtilityComponent));
		PerceptionManager pm = GetGame().GetPerceptionManager();
		if (!gu || !gu.m_Perception || !pm)
			return false;
		float pmNow = pm.GetTime();
		vector me = utility.m_OwnerEntity.GetOrigin();
		vector centroid;
		bool found;
		foreach (SCR_AITargetInfo t : gu.m_Perception.m_aTargets)
		{
			if (!t || pmNow - t.m_fTimestamp > 5.0)
				continue;
			float dt = vector.DistanceXZ(me, t.m_vWorldPos);
			if (dt < 10.0 || dt > 25.0)
				continue;
			if (ClusterAt(utility, t.m_vWorldPos, centroid))
			{
				found = true;
				break;
			}
		}
		if (!found)
			return false;
		AIGroup ocGrp = agent.GetParentGroup();
		float ocNow = GetGame().GetWorld().GetWorldTime();
		if (!s_mOppClusterNext)
			s_mOppClusterNext = new map<AIGroup, float>();
		float ocNext;
		if (s_mOppClusterNext.Find(ocGrp, ocNext) && ocNow < ocNext)
		{
			DCO_Perf.Count("gren:opp_cluster_skip_squad_cd");
			return false;
		}
		if (FriendlyInBlast(utility.m_OwnerEntity, centroid, 15.0))
		{
			DCO_Perf.Count("gren:opp_cluster_skip_friend15");
			return false;
		}
		if (!CanThrowGrenadeNow(utility))
			return false;
		s_mOppClusterNext.Set(ocGrp, ocNow + 30000.0);
		vector throwPos;
		if (!ResolveThrowPos(utility, centroid, throwPos))
		{
			DCO_Perf.Count("gren:opp_cluster_no_throw");
			return false;
		}
		SCR_AIThrowGrenadeToBehavior gren = new SCR_AIThrowGrenadeToBehavior(utility, null, throwPos, EWeaponType.WT_FRAGGRENADE, 1,
			SCR_AIThrowGrenadeToBehavior.PRIORITY_BEHAVIOR_THROW_GRENADE + SCR_AIThrowGrenadeToBehavior.PRIORITY_LEVEL_PLAYER);
		utility.AddAction(gren);
		NotifyGrenadeThrown(utility, centroid);
		DCO_Perf.Count("gren:opp_cluster");
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(utility.m_OwnerEntity), string.Format("opp_cluster_frag unit=%1 dist=%2 centroid=%3",
			DCO_Log.IdUnit(utility.m_OwnerEntity), Math.Round(vector.DistanceXZ(me, centroid)), DCO_Util.XZ(centroid)));
		return true;
	}

	static bool TryCloseFrag(SCR_AIUtilityComponent utility, BaseTarget target, float dist, bool visible)
	{
		if (!utility || !target || !utility.m_OwnerEntity || dist > 35.0 || dist < 3.0)
			return false;
		if (IsCooling(utility.m_OwnerEntity, GetGame().GetWorld().GetWorldTime()))
			return false;

		vector tp = target.GetLastSeenPosition();
		vector aim = tp;
		int reason;
		vector centroid;
		if (ClusterAt(utility, tp, centroid))
		{
			reason = 2;
			aim = centroid;
		}
		else if (!visible && dist >= 5.0 && dist <= 30.0 && target.GetTimeSinceSeen() > 1.0 && HardCoverBetween(utility.m_OwnerEntity, tp))
			reason = 1;
		if (reason == 0)
			return false;

		if (!CanThrowGrenadeNow(utility))
			return false;
		if (reason == 1)
			DCO_Perf.Count("gren:close_cover");
		else
			DCO_Perf.Count("gren:close_cluster");

		vector throwPos;
		if (!ResolveThrowPos(utility, aim, throwPos))
			return false;

		SCR_AIThrowGrenadeToBehavior gren = new SCR_AIThrowGrenadeToBehavior(utility, null, throwPos, EWeaponType.WT_FRAGGRENADE, 1,
			SCR_AIThrowGrenadeToBehavior.PRIORITY_BEHAVIOR_THROW_GRENADE + SCR_AIThrowGrenadeToBehavior.PRIORITY_LEVEL_PLAYER);
		utility.AddAction(gren);
		NotifyGrenadeThrown(utility, aim);
		return true;
	}

	protected static ref array<IEntity> s_aBlast;
	protected static float s_fFfLogAt;

	protected static bool CollectBlast(IEntity e)
	{
		if (ChimeraCharacter.Cast(e))
			s_aBlast.Insert(e);
		return true;
	}

	protected static bool InBounds(vector p, vector mins, vector maxs)
	{
		return p[0] >= mins[0] && p[0] <= maxs[0] && p[1] >= mins[1] - 1.0 && p[1] <= maxs[1] && p[2] >= mins[2] && p[2] <= maxs[2];
	}

	protected static bool OuterWallBetween(vector pos, IEntity friend)
	{
		float f = DCO_Util.TraceFrac(pos + Vector(0, 0.5, 0), friend.GetOrigin() + Vector(0, 1.2, 0), friend, TraceFlags.WORLD | TraceFlags.ENTS, -1, "t:gren_wall");
		if (f >= 0.99)
			return false;
		IEntity hit = DCO_Util.LastTraceEnt();
		if (!hit)
			return true;
		IEntity building = hit.GetRootParent();
		if (!building || ChimeraCharacter.Cast(building) || BaseTree.Cast(building))
			return false;
		vector mins, maxs;
		building.GetWorldBounds(mins, maxs);
		return !(InBounds(pos, mins, maxs) && InBounds(friend.GetOrigin(), mins, maxs));
	}

	static bool FriendlyInBlast(IEntity self, vector pos, float radius)
	{
		if (!self)
			return false;
		FactionAffiliationComponent selfFac = FactionAffiliationComponent.Cast(self.FindComponent(FactionAffiliationComponent));
		if (!selfFac || !selfFac.GetAffiliatedFaction())
			return false;
		Faction mine = selfFac.GetAffiliatedFaction();
		if (!s_aBlast)
			s_aBlast = {};
		s_aBlast.Clear();
		bool predict = radius > 3.0 && DCO_Ctx.TuneFor(self, "gren.ff_predict", 1.0) > 0;
		float extra = 0;
		float qr = radius;
		if (predict)
		{
			extra = 0.1 * vector.Distance(self.GetOrigin(), pos);
			qr = radius + extra + 21.0;
		}
		DCO_Perf.Count("q:DCO_GrenadeUtility");
		GetGame().GetWorld().QueryEntitiesBySphere(pos, qr, CollectBlast, null, EQueryEntitiesFlags.DYNAMIC);
		bool found = false;
		foreach (IEntity e : s_aBlast)
		{
			if (!e || e == self || !DCO_Util.IsAlive(e))
				continue;
			FactionAffiliationComponent fac = FactionAffiliationComponent.Cast(e.FindComponent(FactionAffiliationComponent));
			if (!fac || fac.GetAffiliatedFaction() != mine)
				continue;
			bool pred = false;
			float fd = vector.Distance(e.GetOrigin(), pos);
			if (predict && fd > radius + extra)
			{
				vector v;
				Physics ph = e.GetPhysics();
				if (ph)
					v = ph.GetVelocity();
				v[1] = 0;
				if (vector.Distance(e.GetOrigin() + v * 3.5, pos) > radius + extra)
					continue;
				pred = true;
			}
			if (OuterWallBetween(pos, e))
			{
				DCO_Perf.Count("gren:wall_exception_used");
				continue;
			}
			found = true;
			if (predict)
			{
				if (pred)
					DCO_Perf.Count("gren:ff_block_pred");
				else
					DCO_Perf.Count("gren:ff_block_now");
				float nowS = DCO_Util.NowS();
				if (nowS - s_fFfLogAt >= 5.0)
				{
					s_fFfLogAt = nowS;
					DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(self), string.Format("gren_ff_block thrower=%1 friend=%2 friend_dist=%3 predicted=%4 radius=%5 scatter=%6", DCO_Log.IdUnit(self), DCO_Log.IdUnit(e), Math.Round(fd), DCO_Util.B01(pred), Math.Round(radius), Math.Round(extra)));
				}
			}
			break;
		}
		s_aBlast.Clear();
		return found;
	}

	static float LastThrow(IEntity e)
	{
		float t;
		if (!e || !ThrowMap().Find(e, t))
			return -1;
		return t;
	}

	static void NotifyGrenadeThrown(SCR_AIUtilityComponent utility, vector target = vector.Zero)
	{
		if (!utility || !utility.m_OwnerEntity)
			return;

		ThrowMap().Set(utility.m_OwnerEntity, GetGame().GetWorld().GetWorldTime());
		vector blast = target;
		if (blast == vector.Zero && utility.m_CombatComponent && utility.m_CombatComponent.GetCurrentTarget())
			blast = utility.m_CombatComponent.GetCurrentTarget().GetLastSeenPosition();
		if (DCO_MoveBrain.On(utility))
			DCO_MoveBrain.NoteBlast(utility, blast);
	}

	static bool IsThrowSafe(SCR_AIUtilityComponent utility, vector targetPos)
	{
		if (!utility || !utility.m_OwnerEntity)
			return false;

		IEntity self    = utility.m_OwnerEntity;
		vector  selfPos = self.GetOrigin();

		vector flatDelta = targetPos - selfPos;
		flatDelta[1] = 0.0;
		float dist = flatDelta.Length();

		if (dist < 5.0)
			return false;

		if (dist > 40.0)
			return false;

		if (DCO_Util.HasFriendlyNear(self, targetPos, 15.0, "q:DCO_GrenadeUtility"))
			return false;

		vector start = selfPos;
		start[1] = start[1] + 1.6;

		vector end = targetPos;
		end[1] = end[1] + 0.5;

		float apexRise = Math.Clamp(dist * 0.35, 2.0, 8.0);

		float headroom = GetHeadroom(self, start, apexRise);
		if (headroom < 1.0)
			return false;

		if (headroom < apexRise)
			apexRise = headroom;

		return ValidateArc(self, start, end, apexRise);
	}

	protected static float GetHeadroom(IEntity self, vector start, float apexRise)
	{
		float frac = DCO_Util.TraceFrac(start, start + (vector.Up * apexRise), self, -1, EPhysicsLayerDefs.Projectile, "t:DCO_GrenadeUtility");

		return apexRise * frac;
	}

	protected static bool ValidateArc(IEntity self, vector start, vector end, float apexRise)
	{

		float segs = 5;
		vector prev = start;

		for (int i = 1; i <= 5; i++)
		{
			float t = (1.0 * i / segs) * 0.85;

			vector point = ArcPoint(start, end, apexRise, t);

			if (DCO_Util.TraceFrac(prev, point, self, -1, EPhysicsLayerDefs.Projectile, "t:DCO_GrenadeUtility") < 1.0)
				return false;

			prev = point;
		}

		return true;
	}

	protected static vector ArcPoint(vector start, vector end, float apexRise, float t)
	{
		vector p;
		p[0] = start[0] + (end[0] - start[0]) * t;
		p[1] = start[1] + (end[1] - start[1]) * t + (apexRise * 4.0 * t * (1.0 - t));
		p[2] = start[2] + (end[2] - start[2]) * t;

		return p;
	}




	protected static ref map<IEntity, IEntity> s_mCachedGrenade;
	protected static ref array<IEntity> s_aItemBuffer;

	#ifdef WORKBENCH
	protected static ref array<ref Shape> s_aDbgShapes;
	#endif

	static bool IsIndoorThrow(IEntity self, vector targetPos)
	{
		if (SCR_CoverManagerComponent.IsEntityInsideBuilding(self))
			return true;
		return DCO_Util.TraceFrac(targetPos + Vector(0, 0.5, 0), targetPos + Vector(0, 12.0, 0), self, TraceFlags.WORLD, -1, "t:DCO_GrenadeUtility") < 1.0;
	}

	static float ScatterMul(SCR_AIUtilityComponent utility)
	{
		float mul = 1.0;
		ChimeraCharacter ch = ChimeraCharacter.Cast(utility.m_OwnerEntity);
		if (ch && ch.GetPhysics() && ch.GetPhysics().GetVelocity().Length() > 1.0)
			mul *= 1.3;
		SCR_CharacterDamageManagerComponent dmg = SCR_CharacterDamageManagerComponent.Cast(utility.m_OwnerEntity.FindComponent(SCR_CharacterDamageManagerComponent));
		if (dmg && dmg.IsBleeding())
			mul *= 1.25;
		DCO_AIMoraleSystem morale = utility.GetMoraleSystem();
		if (morale && (morale.GetState() == moraleState.SHAKEN || morale.GetState() == moraleState.PANIC || morale.GetState() == moraleState.BREAK))
			mul *= 1.2;
		if (utility.m_DCOConfig)
		{
			switch (utility.m_DCOConfig.GetPersonality())
			{
				case DCO_EAIPersonality.CAUTIOUS:	mul *= 0.9; break;
				case DCO_EAIPersonality.AGGRESSIVE:	mul *= 1.1; break;
				case DCO_EAIPersonality.RECKLESS:	mul *= 1.25; break;
			}
		}
		return mul;
	}

	static string s_sDCORej;
	static string s_sDCOMode;

	static bool ResolveThrowPos(SCR_AIUtilityComponent utility, vector targetPos, out vector throwPos, bool breach = false)
	{
		throwPos = targetPos;
		s_sDCORej = "none";
		s_sDCOMode = "-";

		if (!utility || !utility.m_OwnerEntity)
			return false;

		IEntity self    = utility.m_OwnerEntity;
		vector  selfPos = self.GetOrigin();

		DbgClear();
		DCO_Perf.Count("gren:try");

		float dist = vector.DistanceXZ(selfPos, targetPos);
		if (dist > 40.0)
		{
			DCO_Perf.Count("gren:rej_far");
			s_sDCORej = "far";
			return false;
		}
		float minDist = 5.0;
		if (IsIndoorThrow(self, targetPos))
			minDist = 3.0;
		if (breach)
			minDist = 2.0;
		if (dist < minDist)
		{
			DCO_Perf.Count("gren:rej_close");
			s_sDCORej = "close";
			return false;
		}

		if (!breach && dist < 15.0 && !HardCoverBetween(self, targetPos))
		{
			DCO_Perf.Count("gren:rej_close_nocover");
			s_sDCORej = "close_nocover";
			return false;
		}

		float blastR = 15.0;
		if (breach)
			blastR = 3.0;
		if (FriendlyInBlast(self, targetPos, blastR))
		{
			DCO_Perf.Count("gren:rej_friendly");
			s_sDCORej = "friendly_blast";
			return false;
		}

		IEntity grenade = FindFragGrenade(self);

		vector start = selfPos;
		start[1] = start[1] + 1.6;

		DCO_EGrenadeThrowMode mode = DCO_EGrenadeThrowMode.NONE;

		if (breach && IsFlatClear(self, start, targetPos, 0.6))
		{
			throwPos = targetPos;
			mode     = DCO_EGrenadeThrowMode.DIRECT;
			s_sDCOMode = "flat";
		}
		else if (IsTrajectoryClear(self, grenade, start, targetPos, LobClearance(breach)))
		{
			throwPos = targetPos;
			mode     = DCO_EGrenadeThrowMode.DIRECT;
			s_sDCOMode = "lob";
		}
		else if (breach && dist <= 6.0 && IsSegmentClear(self, start, targetPos + Vector(0, 0.3, 0)))
		{
			throwPos = targetPos;
			mode     = DCO_EGrenadeThrowMode.DIRECT;
			s_sDCOMode = "straight";
		}

		if (false)
		{
			Print(string.Format("[DCO_Grenade] %1 dist=%2 mode=%3 realBallistics=%4 throwPos=%5",
				self, dist, typename.EnumToString(DCO_EGrenadeThrowMode, mode), grenade != null, throwPos), LogLevel.NORMAL);
		}

		if (mode == DCO_EGrenadeThrowMode.NONE && !breach && dist <= 25.0 && IsIndoorThrow(self, targetPos))
		{
			vector win;
			if (FindOpening(self, grenade, start, targetPos, win))
			{
				DCO_Perf.Count("gren:window_ok");
				if (DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO))
					DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(self), string.Format("gren_window unit=%1 dist=%2 aim_off=%3 target=%4 aim=%5 %6",
						DCO_Log.IdUnit(self), Math.Round(dist), Math.Round(vector.DistanceXZ(win, targetPos) * 10) / 10, DCO_Log.Pos(targetPos), DCO_Log.Pos(win), s_sDCOWinDiag));
				targetPos = win;
				mode = DCO_EGrenadeThrowMode.DIRECT;
				s_sDCOMode = "window";
			}
			else
			{
				DCO_Perf.Count("gren:window_none");
				if (DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO))
					DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(self), string.Format("gren_window_none unit=%1 dist=%2 %3", DCO_Log.IdUnit(self), Math.Round(dist), s_sDCOWinDiag));
			}
		}

		if (mode == DCO_EGrenadeThrowMode.NONE)
		{
			DCO_Perf.Count("gren:rej_arc");
			s_sDCORej = "arc";
			return false;
		}

		DCO_Perf.Count("gren:ok");
		float scatter = ScatterMul(utility);
		if (s_sDCOMode == "window")
			scatter *= 0.4;
		throwPos = DCO_AimUtility.ScatterExplosive(utility, targetPos, false, scatter, 0.3);
		return true;
	}

	protected static bool ComputeArcCoeffs(IEntity grenade, vector start, vector end, out float a, out float b)
	{
		float dxz = vector.DistanceXZ(start, end);
		if (dxz < 0.5)
			return false;

		float dy = end[1] - start[1];

		if (grenade)
		{
			float flightTime;
			float h = BallisticTable.GetHeightFromProjectile(vector.Distance(start, end), flightTime, grenade, 1.0);

			if (flightTime > 0.01 && h > 0)
			{
				a = dy + h;
				b = -h;
				return true;
			}
		}

		float apexRise = Math.Clamp(dxz * 0.35, 2.0, 8.0);
		a = dy + 4.0 * apexRise;
		b = -4.0 * apexRise;
		return true;
	}

	protected static vector ArcPointAt(vector start, vector end, float a, float b, float u)
	{
		vector p;
		p[0] = start[0] + (end[0] - start[0]) * u;
		p[1] = start[1] + a * u + b * u * u;
		p[2] = start[2] + (end[2] - start[2]) * u;
		return p;
	}

	protected static float LobClearance(bool breach)
	{
		if (breach)
			return 0.97;
		return 0.85;
	}

	protected static bool IsTrajectoryClear(IEntity self, IEntity grenade, vector start, vector end, float uEnd)
	{
		float a, b;
		if (!ComputeArcCoeffs(grenade, start, end, a, b))
			return false;

		float segs = 5;
		vector prev = start;

		for (int i = 1; i <= 5; i++)
		{
			vector point = ArcPointAt(start, end, a, b, (1.0 * i / segs) * uEnd);

			if (!IsSegmentClear(self, prev, point))
			{
				DbgLine(prev, point, Color.RED);
				return false;
			}

			DbgLine(prev, point, Color.GREEN);
			prev = point;
		}

		return true;
	}

	static string s_sDCOWinDiag;
	protected static ref map<IEntity, vector> s_mWinFail;

	protected static string HitName(IEntity h)
	{
		if (!h)
			return "none";
		IEntity root = h.GetRootParent();
		string n = root.ClassName();
		if (root.GetPrefabData())
			n = FilePath.StripPath(root.GetPrefabData().GetPrefabName());
		IEntity e = h;
		while (e && e != root)
		{
			if (e.GetPrefabData() && !e.GetPrefabData().GetPrefabName().IsEmpty())
				return n + ">" + FilePath.StripPath(e.GetPrefabData().GetPrefabName());
			e = e.GetParent();
		}
		if (h != root)
		{
			n = n + ">" + h.ClassName();
			if (h.GetVObject())
				n = n + ":" + FilePath.StripPath(h.GetVObject().GetResourceName());
		}
		return n;
	}

	protected static bool IsWindowPart(IEntity h)
	{
		IEntity e = h;
		while (e)
		{
			if (e.GetPrefabData() && e.GetPrefabData().GetPrefabName().Contains("/Windows/"))
				return true;
			if (e != e.GetRootParent() && e.GetVObject())
			{
				string res = e.GetVObject().GetResourceName();
				res.ToLower();
				if (res.Contains("/windows/") || res.Contains("win_") || res.Contains("glass"))
					return true;
			}
			e = e.GetParent();
		}
		return false;
	}

	protected static bool FindOpening(IEntity self, IEntity grenade, vector start, vector tp, out vector win)
	{
		vector flat = tp - start;
		flat[1] = 0;
		float dist = flat.Length();
		if (dist < 3.0)
			return false;
		float baseAng = Math.Atan2(flat[0], flat[2]);
		float best = float.MAX;
		int nRay;
		int nPen;
		int nRoof;
		int nArc;
		int nGlass;
		bool glassBest;
		string rayHit = "-";
		string arcHit = "-";
		float now = GetGame().GetWorld().GetWorldTime();
		if (!s_mWinFail)
			s_mWinFail = new map<IEntity, vector>();
		vector lastFail;
		if (s_mWinFail.Find(self, lastFail) && now - lastFail[1] < 10000.0 && Math.AbsFloat(lastFail[0] - dist) < 2.0)
		{
			s_sDCOWinDiag = "cached_fail";
			DCO_Perf.Count("gren:window_cached");
			return false;
		}
		for (int hi = 0; hi < 3; hi++)
		{
			float aimY = tp[1] + 0.8 + 1.0 * hi * 0.5;
			for (int k = -12; k <= 12; k++)
			{
				float ang = baseAng + k * 2.0 * Math.DEG2RAD;
				vector dir = Vector(Math.Sin(ang) * dist, aimY - start[1], Math.Cos(ang) * dist);
				float len = dir.Length() + 4.0;
				dir.Normalize();
				nRay++;
				float f = DCO_Util.TraceFrac(start, start + dir * len, self, TraceFlags.WORLD | TraceFlags.ENTS, EPhysicsLayerDefs.Projectile, "t:gren_window");
				float reach = f * len;
				bool glass = false;
				vector winPt = vector.Zero;
				if (reach < dist - 2.5)
				{
					IEntity rh = DCO_Util.LastTraceEnt();
					if (rayHit == "-")
						rayHit = HitName(rh);
					if (dist > 22.0 || !IsWindowPart(rh))
						continue;
					winPt = start + dir * reach;
					vector past = start + dir * (reach + 0.3);
					float f2 = DCO_Util.TraceFrac(past, start + dir * len, self, TraceFlags.WORLD | TraceFlags.ENTS, EPhysicsLayerDefs.Projectile, "t:gren_window");
					float reach2 = reach + 0.3 + f2 * (len - reach - 0.3);
					if (reach2 < dist - 2.5)
						continue;
					glass = true;
					nGlass++;
					reach = reach2;
				}
				nPen++;
				vector p = start + dir * Math.Min(reach - 0.5, dist + 1.5);
				p[1] = tp[1];
				if (vector.DistanceXZ(p, tp) > 4.0)
					continue;
				if (DCO_Util.TraceFrac(p + Vector(0, 0.6, 0), p + Vector(0, 12.0, 0), self, TraceFlags.WORLD | TraceFlags.ENTS, -1, "t:gren_window") >= 1.0)
				{
					nRoof++;
					continue;
				}
				vector land = p + Vector(0, 0.3, 0);
				if (glass)
				{
					float ca, cb;
					float uw = vector.DistanceXZ(start, winPt) / Math.Max(vector.DistanceXZ(start, land), 0.5);
					if (!ComputeArcCoeffs(grenade, start, land, ca, cb) || Math.AbsFloat(start[1] + ca * uw + cb * uw * uw - winPt[1]) > 0.45)
					{
						nArc++;
						if (arcHit == "-")
							arcHit = "glass_height";
						continue;
					}
				}
				else if (!IsTrajectoryClear(self, grenade, start, land, 0.97) && !(dist <= 15.0 && IsFlatClear(self, start, land, 0.6)))
				{
					nArc++;
					if (arcHit == "-")
						arcHit = HitName(DCO_Util.LastTraceEnt());
					continue;
				}
				float d = vector.DistanceXZ(p, tp);
				if (glass)
					d += 1.0;
				if (d < best)
				{
					best = d;
					win = p;
					glassBest = glass;
				}
			}
		}
		if (best == float.MAX)
			s_mWinFail.Set(self, Vector(dist, now, 0));
		s_sDCOWinDiag = string.Format("rays=%1 penetrate=%2 glass=%3 roof_fail=%4 arc_fail=%5 ray_hit=%6 arc_hit=%7 best_glass=%8", nRay, nPen, nGlass, nRoof, nArc, rayHit, arcHit, glassBest);
		return best < float.MAX;
	}

	protected static bool IsFlatClear(IEntity self, vector start, vector end, float apex)
	{
		vector prev = start;
		for (int i = 1; i <= 4; i++)
		{
			float u = 1.0 * i / 4.0;
			vector point = start + (end - start) * u;
			point[1] = point[1] + 4.0 * apex * u * (1.0 - u);
			if (!IsSegmentClear(self, prev, point))
				return false;
			prev = point;
		}
		return true;
	}

	protected static bool IsSegmentClear(IEntity self, vector from, vector to)
	{
		return DCO_Util.TraceClear(from, to, self, 1.0, TraceFlags.WORLD | TraceFlags.ENTS, EPhysicsLayerDefs.Projectile, null, "t:DCO_GrenadeUtility");
	}

	protected static IEntity FindFragGrenade(IEntity self)
	{
		if (!self)
			return null;

		IEntity cached;
		if (CachedMap().Find(self, cached))
		{
			if (cached && cached.GetRootParent() == self)
				return cached;

			CachedMap().Remove(self);
		}

		InventoryStorageManagerComponent inv = InventoryStorageManagerComponent.Cast(self.FindComponent(InventoryStorageManagerComponent));
		if (!inv)
			return null;

		ItemBuf().Clear();
		array<IEntity> itemBuf = ItemBuf();
		inv.GetItems(itemBuf);

		foreach (IEntity item : ItemBuf())
		{
			if (!item)
				continue;

			BaseWeaponComponent weap = BaseWeaponComponent.Cast(item.FindComponent(BaseWeaponComponent));
			if (!weap || weap.GetWeaponType() != EWeaponType.WT_FRAGGRENADE)
				continue;

			CachedMap().Set(self, item);
			ItemBuf().Clear();
			return item;
		}

		ItemBuf().Clear();
		return null;
	}

	protected static void DbgClear()
	{
		#ifdef WORKBENCH
		if (false && s_aDbgShapes)
			s_aDbgShapes.Clear();
		#endif
	}

	protected static void DbgLine(vector from, vector to, int color)
	{
		#ifdef WORKBENCH
		if (!false)
			return;

		vector pts[2];
		pts[0] = from;
		pts[1] = to;
		if (!s_aDbgShapes)
			s_aDbgShapes = {};
		s_aDbgShapes.Insert(Shape.CreateLines(color, ShapeFlags.NOZBUFFER, pts, 2));
		#endif
	}
}
