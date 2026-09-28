enum DCO_EGrenadeThrowMode
{
	NONE,
	DIRECT
}

class DCO_GrenadeUtility
{
	protected static ref map<IEntity, float> s_mLastGrenadeThrowTime = new map<IEntity, float>();

	protected static ref map<IEntity, float> s_mLastGrenadeRollTime = new map<IEntity, float>();
	static const float GRENADE_ROLL_INTERVAL_MS = 3000.0;

	static const float GRENADE_COOLDOWN_MS = 20000.0;

	static const float GRENADE_MIN_THROW_DIST = 5.0;
	static const float GRENADE_MAX_THROW_DIST = 40.0;

	static const float THROW_ORIGIN_HEIGHT   = 1.6;
	static const float TARGET_CLEAR_HEIGHT   = 0.5;
	static const float ARC_END_RATIO         = 0.85;
	static const int   ARC_SEGMENTS          = 5;
	static const float APEX_RISE_RATIO       = 0.35;
	static const float APEX_RISE_MIN         = 2.0;
	static const float APEX_RISE_MAX         = 8.0;
	static const float MIN_USABLE_HEADROOM   = 1.0;
	static const float FRIENDLY_BLAST_RADIUS = 15.0;

	protected static ref array<IEntity> s_aBlastCheckResult = {};

	static bool CanThrowGrenadeNow(SCR_AIUtilityComponent utility)
	{
		if (!utility || !utility.m_OwnerEntity)
			return false;

		IEntity myEntity = utility.m_OwnerEntity;
		float worldTime_ms = GetGame().GetWorld().GetWorldTime();

		float usageScale = 1.0;
		if (utility.m_DCOConfig)
			usageScale = DCO_AIConfigComponent.UsageToChanceScale(utility.m_DCOConfig.GetGrenadeUsage());

		if (usageScale <= 0.0)
			return false;

		float lastThrow;
		if (s_mLastGrenadeThrowTime.Find(myEntity, lastThrow))
		{
			if ((worldTime_ms - lastThrow) < GRENADE_COOLDOWN_MS)
				return false;
		}

		float lastRoll;
		if (s_mLastGrenadeRollTime.Find(myEntity, lastRoll))
		{
			if ((worldTime_ms - lastRoll) < GRENADE_ROLL_INTERVAL_MS)
				return false;
		}

		s_mLastGrenadeRollTime.Set(myEntity, worldTime_ms);

		float chance = DCO_PersonalityCombatUtility.GetGrenadeThrowChance(utility);
		chance = Math.Clamp(chance * usageScale, 0.0, 1.0);
		if (Math.RandomFloat01() > chance)
			return false;

		return true;
	}

	static void NotifyGrenadeThrown(SCR_AIUtilityComponent utility)
	{
		if (!utility || !utility.m_OwnerEntity)
			return;

		s_mLastGrenadeThrowTime.Set(utility.m_OwnerEntity, GetGame().GetWorld().GetWorldTime());
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

		if (dist < GRENADE_MIN_THROW_DIST)
			return false;

		if (dist > GRENADE_MAX_THROW_DIST)
			return false;

		if (HasFriendlyInBlast(self, targetPos))
			return false;

		vector start = selfPos;
		start[1] = start[1] + THROW_ORIGIN_HEIGHT;

		vector end = targetPos;
		end[1] = end[1] + TARGET_CLEAR_HEIGHT;

		float apexRise = Math.Clamp(dist * APEX_RISE_RATIO, APEX_RISE_MIN, APEX_RISE_MAX);

		float headroom = GetHeadroom(self, start, apexRise);
		if (headroom < MIN_USABLE_HEADROOM)
			return false;

		if (headroom < apexRise)
			apexRise = headroom;

		return ValidateArc(self, start, end, apexRise);
	}

	protected static float GetHeadroom(IEntity self, vector start, float apexRise)
	{
		TraceParam param = new TraceParam();
		param.Start     = start;
		param.End       = start + (vector.Up * apexRise);
		param.Exclude   = self;
		param.LayerMask = EPhysicsLayerDefs.Projectile;

		DCO_Perf.Count("t:DCO_GrenadeUtility");
		float frac = GetGame().GetWorld().TraceMove(param, null);

		return apexRise * frac;
	}

	protected static bool ValidateArc(IEntity self, vector start, vector end, float apexRise)
	{
		BaseWorld world = GetGame().GetWorld();

		float segs = ARC_SEGMENTS;
		vector prev = start;

		for (int i = 1; i <= ARC_SEGMENTS; i++)
		{
			float t = (i / segs) * ARC_END_RATIO;

			vector point = ArcPoint(start, end, apexRise, t);

			TraceParam param = new TraceParam();
			param.Start     = prev;
			param.End       = point;
			param.Exclude   = self;
			param.LayerMask = EPhysicsLayerDefs.Projectile;

			DCO_Perf.Count("t:DCO_GrenadeUtility");
			if (world.TraceMove(param, null) < 1.0)
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

	protected static bool HasFriendlyInBlast(IEntity self, vector pos)
	{
		s_aBlastCheckResult.Clear();

		FactionAffiliationComponent selfFac = FactionAffiliationComponent.Cast(self.FindComponent(FactionAffiliationComponent));
		if (!selfFac || !selfFac.GetAffiliatedFaction())
			return false;

		string myFactionKey = selfFac.GetAffiliatedFaction().GetFactionKey();

		DCO_Perf.Count("q:DCO_GrenadeUtility");
		GetGame().GetWorld().QueryEntitiesBySphere(pos, FRIENDLY_BLAST_RADIUS, null, BlastQueryCallback, EQueryEntitiesFlags.DYNAMIC);

		foreach (IEntity ent : s_aBlastCheckResult)
		{
			if (!ent)
				continue;

			FactionAffiliationComponent fac = FactionAffiliationComponent.Cast(ent.FindComponent(FactionAffiliationComponent));
			if (!fac || !fac.GetAffiliatedFaction())
				continue;

			if (fac.GetAffiliatedFaction().GetFactionKey() == myFactionKey)
				return true;
		}

		return false;
	}

	protected static bool BlastQueryCallback(IEntity ent)
	{
		if (ent)
			s_aBlastCheckResult.Insert(ent);

		return true;
	}

	static const float INIT_SPEED_COEF        = 1.0;

	static const bool  DEBUG_THROW            = false;

	protected static ref map<IEntity, IEntity> s_mCachedGrenade = new map<IEntity, IEntity>();
	protected static ref array<IEntity> s_aItemBuffer = {};

	#ifdef WORKBENCH
	protected static ref array<ref Shape> s_aDbgShapes = {};
	#endif

	static bool ResolveThrowPos(SCR_AIUtilityComponent utility, vector targetPos, out vector throwPos)
	{
		throwPos = targetPos;

		if (!utility || !utility.m_OwnerEntity)
			return false;

		IEntity self    = utility.m_OwnerEntity;
		vector  selfPos = self.GetOrigin();

		DbgClear();

		float dist = vector.DistanceXZ(selfPos, targetPos);
		if (dist < GRENADE_MIN_THROW_DIST || dist > GRENADE_MAX_THROW_DIST)
			return false;

		if (HasFriendlyInBlast(self, targetPos))
			return false;

		IEntity grenade = FindFragGrenade(self);

		vector start = selfPos;
		start[1] = start[1] + THROW_ORIGIN_HEIGHT;

		DCO_EGrenadeThrowMode mode = DCO_EGrenadeThrowMode.NONE;

		if (IsTrajectoryClear(self, grenade, start, targetPos, ARC_END_RATIO))
		{
			throwPos = targetPos;
			mode     = DCO_EGrenadeThrowMode.DIRECT;
		}

		if (DEBUG_THROW)
		{
			Print(string.Format("[DCO_Grenade] %1 dist=%2 mode=%3 realBallistics=%4 throwPos=%5",
				self, dist, typename.EnumToString(DCO_EGrenadeThrowMode, mode), grenade != null, throwPos), LogLevel.NORMAL);
		}

		return mode != DCO_EGrenadeThrowMode.NONE;
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
			float h = BallisticTable.GetHeightFromProjectile(vector.Distance(start, end), flightTime, grenade, INIT_SPEED_COEF);

			if (flightTime > 0.01 && h > 0)
			{
				a = dy + h;
				b = -h;
				return true;
			}
		}

		float apexRise = Math.Clamp(dxz * APEX_RISE_RATIO, APEX_RISE_MIN, APEX_RISE_MAX);
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

	protected static bool IsTrajectoryClear(IEntity self, IEntity grenade, vector start, vector end, float uEnd)
	{
		float a, b;
		if (!ComputeArcCoeffs(grenade, start, end, a, b))
			return false;

		float segs = ARC_SEGMENTS;
		vector prev = start;

		for (int i = 1; i <= ARC_SEGMENTS; i++)
		{
			vector point = ArcPointAt(start, end, a, b, (i / segs) * uEnd);

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

	protected static bool IsSegmentClear(IEntity self, vector from, vector to)
	{
		TraceParam param = new TraceParam();
		param.Start     = from;
		param.End       = to;
		param.Exclude   = self;
		param.Flags     = TraceFlags.WORLD | TraceFlags.ENTS;
		param.LayerMask = EPhysicsLayerDefs.Projectile;

		DCO_Perf.Count("t:DCO_GrenadeUtility");
		return GetGame().GetWorld().TraceMove(param, null) >= 1.0;
	}

	protected static IEntity FindFragGrenade(IEntity self)
	{
		if (!self)
			return null;

		IEntity cached;
		if (s_mCachedGrenade.Find(self, cached))
		{
			if (cached && cached.GetRootParent() == self)
				return cached;

			s_mCachedGrenade.Remove(self);
		}

		InventoryStorageManagerComponent inv = InventoryStorageManagerComponent.Cast(self.FindComponent(InventoryStorageManagerComponent));
		if (!inv)
			return null;

		s_aItemBuffer.Clear();
		inv.GetItems(s_aItemBuffer);

		foreach (IEntity item : s_aItemBuffer)
		{
			if (!item)
				continue;

			BaseWeaponComponent weap = BaseWeaponComponent.Cast(item.FindComponent(BaseWeaponComponent));
			if (!weap || weap.GetWeaponType() != EWeaponType.WT_FRAGGRENADE)
				continue;

			s_mCachedGrenade.Set(self, item);
			s_aItemBuffer.Clear();
			return item;
		}

		s_aItemBuffer.Clear();
		return null;
	}

	protected static void DbgClear()
	{
		#ifdef WORKBENCH
		if (DEBUG_THROW)
			s_aDbgShapes.Clear();
		#endif
	}

	protected static void DbgLine(vector from, vector to, int color)
	{
		#ifdef WORKBENCH
		if (!DEBUG_THROW)
			return;

		vector pts[2];
		pts[0] = from;
		pts[1] = to;
		s_aDbgShapes.Insert(Shape.CreateLines(color, ShapeFlags.NOZBUFFER, pts, 2));
		#endif
	}
}
