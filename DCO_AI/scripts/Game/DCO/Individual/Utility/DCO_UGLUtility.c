class DCO_UGLUtility
{




	protected static ref array<BaseMuzzleComponent> s_aMuzzleBuffer;
	protected static array<BaseMuzzleComponent> GetMuzzleBuffer()
	{
		if (!s_aMuzzleBuffer)
			s_aMuzzleBuffer = {};
		return s_aMuzzleBuffer;
	}
	protected static ref array<IEntity> s_aItemBuffer;
	protected static array<IEntity> GetItemBuffer()
	{
		if (!s_aItemBuffer)
			s_aItemBuffer = {};
		return s_aItemBuffer;
	}
	protected static ref array<typename> s_aMagFilter;
	protected static array<typename> GetMagFilter()
	{
		if (!s_aMagFilter)
			s_aMagFilter = {MagazineComponent};
		return s_aMagFilter;
	}

	protected static ref map<IEntity, float> s_mLastGLRollTime;
	protected static map<IEntity, float> GetLastGLRollTime()
	{
		if (!s_mLastGLRollTime)
			s_mLastGLRollTime = new map<IEntity, float>();
		return s_mLastGLRollTime;
	}
	protected static ref map<IEntity, float> s_mGLCommitUntil;
	protected static map<IEntity, float> GetGLCommitUntil()
	{
		if (!s_mGLCommitUntil)
			s_mGLCommitUntil = new map<IEntity, float>();
		return s_mGLCommitUntil;
	}
	protected static ref map<IEntity, float> s_mGLCooldownUntil;
	protected static map<IEntity, float> GetGLCooldownUntil()
	{
		if (!s_mGLCooldownUntil)
			s_mGLCooldownUntil = new map<IEntity, float>();
		return s_mGLCooldownUntil;
	}
	protected static ref map<IEntity, int> s_mGLShotsLeft;
	protected static map<IEntity, int> GetGLShotsLeft()
	{
		if (!s_mGLShotsLeft)
			s_mGLShotsLeft = new map<IEntity, int>();
		return s_mGLShotsLeft;
	}
	protected static ref map<IEntity, vector> s_mGLTarget;
	protected static ref map<IEntity, bool> s_mGLSmokeLoaded;

	static bool LoadedIsSmoke(BaseWeaponComponent weap)
	{
		int idx = GetUGLMuzzleIndex(weap);
		if (idx < 0)
			return false;
		GetMuzzleBuffer().Clear();
		GetMuzzleBuffer();
		weap.GetMuzzlesList(s_aMuzzleBuffer);
		BaseMuzzleComponent muzzle;
		if (idx < GetMuzzleBuffer().Count())
			muzzle = GetMuzzleBuffer()[idx];
		GetMuzzleBuffer().Clear();
		if (!muzzle || !muzzle.GetMagazine() || !muzzle.GetMagazine().GetOwner())
			return false;
		EntityPrefabData pd = muzzle.GetMagazine().GetOwner().GetPrefabData();
		if (!pd)
			return false;
		string n = pd.GetPrefabName();
		n.ToLower();
		return n.Contains("smoke");
	}

	protected static void NoteGLTarget(IEntity self, BaseWeaponComponent weap, vector targetPos, bool volleyStart)
	{
		if (!s_mGLTarget)
		{
			s_mGLTarget = new map<IEntity, vector>();
			s_mGLSmokeLoaded = new map<IEntity, bool>();
		}
		s_mGLTarget.Set(self, targetPos);
		if (volleyStart)
			s_mGLSmokeLoaded.Set(self, LoadedIsSmoke(weap));
	}

	static int GetUGLMuzzleIndex(BaseWeaponComponent weap)
	{
		if (!weap)
			return -1;

		GetMuzzleBuffer().Clear();
		GetMuzzleBuffer();
		weap.GetMuzzlesList(s_aMuzzleBuffer);

		int count = GetMuzzleBuffer().Count();
		if (count < 2)
		{
			GetMuzzleBuffer().Clear();
			return -1;
		}

		for (int i = 0; i < count; i++)
		{
			BaseMuzzleComponent muzzle = GetMuzzleBuffer()[i];
			if (muzzle && muzzle.GetMuzzleType() == EMuzzleType.MT_UGLMuzzle)
			{
				GetMuzzleBuffer().Clear();
				return i;
			}
		}

		GetMuzzleBuffer().Clear();
		return -1;
	}

	static bool HasUGL(BaseWeaponComponent weap)
	{
		return GetUGLMuzzleIndex(weap) != -1;
	}

	static bool HasUGLAmmo(IEntity owner, BaseWeaponComponent weap, int muzzleIdx)
	{
		if (!owner || !weap || muzzleIdx < 0)
			return false;

		GetMuzzleBuffer().Clear();
		GetMuzzleBuffer();
		weap.GetMuzzlesList(s_aMuzzleBuffer);

		if (muzzleIdx >= GetMuzzleBuffer().Count())
		{
			GetMuzzleBuffer().Clear();
			return false;
		}

		BaseMuzzleComponent muzzle = GetMuzzleBuffer()[muzzleIdx];
		GetMuzzleBuffer().Clear();

		if (!muzzle)
			return false;

		BaseMagazineComponent loaded = muzzle.GetMagazine();
		if (loaded && loaded.GetAmmoCount() > 0)
			return true;

		if (!muzzle.GetMagazineWell())
			return false;

		typename wellType = muzzle.GetMagazineWell().Type();

		SCR_InventoryStorageManagerComponent inv = SCR_InventoryStorageManagerComponent.Cast(owner.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!inv)
			return false;

		GetItemBuffer().Clear();
		GetItemBuffer();
		GetMagFilter();
		inv.FindItemsWithComponents(s_aItemBuffer, s_aMagFilter);

		foreach (IEntity item : GetItemBuffer())
		{
			if (!item)
				continue;

			MagazineComponent mag = MagazineComponent.Cast(item.FindComponent(MagazineComponent));
			if (!mag || !mag.GetMagazineWell())
				continue;

			if (mag.GetMagazineWell().Type() == wellType && mag.GetAmmoCount() > 0)
			{
				GetItemBuffer().Clear();
				return true;
			}
		}

		GetItemBuffer().Clear();
		return false;
	}

	static bool ShouldUseGL(SCR_AIUtilityComponent utility, BaseWeaponComponent weap, vector targetPos)
	{
		if (!utility || !utility.m_OwnerEntity)
			return false;

		IEntity self = utility.m_OwnerEntity;
		DCO_Perf.Count("gl_query");

		int uglIdx = GetUGLMuzzleIndex(weap);
		if (uglIdx == -1)
		{
			DCO_Perf.Count("gl_rej:no_ugl_muzzle");
			return false;
		}

		float usageScale = DCO_AIConfigComponent.UsageToChanceScale(DCO_AIConfigComponent.GLUsageOf(utility));

		if (usageScale <= 0.0)
		{
			if (GetGLShotsLeft().Contains(self))
				EndVolley(self);
			DCO_Perf.Count("gl_rej:usage_off");
			return false;
		}

		float dist = vector.DistanceXZ(self.GetOrigin(), targetPos);
		if (dist < 35.0 || dist > 300.0)
		{
			DCO_Perf.Count("gl_rej:range");
			return false;
		}

		float now = GetGame().GetWorld().GetWorldTime();

		float commitUntil;
		if (GetGLCommitUntil().Find(self, commitUntil) && now < commitUntil)
		{
			if (DCO_Util.HasFriendlyNear(self, targetPos, 15.0, "q:DCO_UGLUtility") || FriendlyInLane(self, targetPos))
			{
				EndVolley(self);
				return false;
			}
			NoteGLTarget(self, weap, targetPos, false);
			return true;
		}

		float cooldownUntil;
		if (GetGLCooldownUntil().Find(self, cooldownUntil) && now < cooldownUntil)
		{
			DCO_Perf.Count("gl_rej:cooldown");
			return false;
		}

		float lastRoll;
		if (GetLastGLRollTime().Find(self, lastRoll) && (now - lastRoll) < 3000.0)
			return false;

		GetLastGLRollTime().Set(self, now);

		float chance = Math.Clamp(0.6 * usageScale, 0.0, 1.0);
		if (DCO_GlobalAIComponent.BehaviorOnUtility(utility, DCO_EBehavior.LEADER_RTO_GRENADIER) && !IsPreferredGLTarget(utility, targetPos))
			chance = chance * 0.3;
		if (Math.RandomFloat01() > chance)
		{
			DCO_Perf.Count("gl_rej:chance");
			return false;
		}

		if (!HasUGLAmmo(self, weap, uglIdx))
		{
			DCO_Perf.Count("gl_rej:no_ammo");
			return false;
		}

		if (DCO_Util.HasFriendlyNear(self, targetPos, 15.0, "q:DCO_UGLUtility"))
		{
			DCO_Perf.Count("gl_rej:friendly_near");
			return false;
		}

		if (FriendlyInLane(self, targetPos))
		{
			DCO_Perf.Count("gl_rej:friendly_lane");
			return false;
		}

		if (!IsFirstImpactSafe(self, targetPos))
		{
			DCO_Perf.Count("gl_rej:first_impact");
			return false;
		}

		int volley = GetVolleySize(utility);
		GetGLShotsLeft().Set(self, volley);
		GetGLCommitUntil().Set(self, now + 10000.0);
		GetGLCooldownUntil().Set(self, now + 10000.0 + 12000.0);
		NoteGLTarget(self, weap, targetPos, true);
		DCO_Perf.Count("spec:gl_volley");
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(self), string.Format("spec_gl_volley unit=%1 dist=%2 volley=%3 chance=%4 target=%5", DCO_Log.IdUnit(self), Math.Round(dist), volley, chance.ToString(-1, 2), DCO_Log.Pos(targetPos)));

		return true;
	}

	protected static bool FriendlyInLane(IEntity self, vector targetPos)
	{
		vector from = self.GetOrigin();
		for (int i = 1; i <= 3; i++)
		{
			vector p = from + (targetPos - from) * (0.25 * i);
			if (vector.DistanceXZ(p, from) < 10.0)
				continue;
			if (DCO_Util.HasFriendlyNear(self, p, 6.0, "q:DCO_UGLLane"))
			{
				DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(self), string.Format("spec_gl_block unit=%1 why=friendly_in_lane at=%2 dist=%3", DCO_Log.IdUnit(self), DCO_Log.Pos(p), Math.Round(vector.DistanceXZ(p, from))));
				return true;
			}
		}
		return false;
	}

	static bool IsPointTarget(BaseTarget t)
	{
		if (!t)
			return false;
		ChimeraCharacter tc = ChimeraCharacter.Cast(t.GetTargetEntity());
		if (!tc)
			return true;
		if (tc.IsInVehicle() || t.IsEndangering() || t.GetTimeSinceEndangered() < 5.0)
			return true;
		return SCR_CoverManagerComponent.IsEntityInsideBuilding(tc);
	}

	protected static bool IsPreferredGLTarget(SCR_AIUtilityComponent utility, vector targetPos)
	{
		AIAgent glAgent = utility.GetAIAgent();
		if (!utility.m_PerceptionComponent)
			return true;
		array<BaseTarget> targets = {};
		utility.m_PerceptionComponent.GetTargetsList(targets, ETargetCategory.ENEMY);
		int near = 0;
		IEntity closest;
		float closestSq = 25.0;
		foreach (BaseTarget t : targets)
		{
			if (!t || !t.GetTargetEntity() || t.GetTimeSinceSeen() > 15.0)
				continue;
			float dSq = vector.DistanceSq(t.GetLastSeenPosition(), targetPos);
			if (dSq > 25.0)
				continue;
			near++;
			if (dSq < closestSq)
			{
				closestSq = dSq;
				closest = t.GetTargetEntity();
			}
		}
		if (near >= 2)
			return true;
		ChimeraCharacter cc = ChimeraCharacter.Cast(closest);
		if (cc && cc.IsInVehicle())
			return true;
		return closest && SCR_CoverManagerComponent.IsEntityInsideBuilding(closest);
	}

	protected static int GetVolleySize(SCR_AIUtilityComponent utility)
	{
		DCO_EAIPersonality p = DCO_AIConfigComponent.PersonalityOf(utility);

		switch (p)
		{
			case DCO_EAIPersonality.CAUTIOUS:
				return Math.RandomIntInclusive(1, 2);
			case DCO_EAIPersonality.AGGRESSIVE:
				return Math.RandomIntInclusive(3, 4);
			case DCO_EAIPersonality.RECKLESS:
				return Math.RandomIntInclusive(3, 5);
		}

		return Math.RandomIntInclusive(2, 3);
	}

	static void NotifyGLShotDone(IEntity self, BaseWeaponComponent weap)
	{
		if (!self)
			return;

		DCO_Perf.Count("spec:gl_shot_done");
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(self), string.Format("spec_gl_shot_done unit=%1", DCO_Log.IdUnit(self)));
		bool firedSmoke;
		vector glTarget;
		if (s_mGLSmokeLoaded && s_mGLSmokeLoaded.Find(self, firedSmoke) && firedSmoke && s_mGLTarget.Find(self, glTarget))
		{
			DCO_SmokeField.Add(null, glTarget, "ai_gl", 2.0, 40.0);
			DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(self), string.Format("smoke_ai_gl unit=%1 dist=%2 pos=%3", DCO_Log.IdUnit(self), Math.Round(vector.DistanceXZ(self.GetOrigin(), glTarget)), DCO_Log.Pos(glTarget)));
		}
		if (s_mGLSmokeLoaded)
			s_mGLSmokeLoaded.Set(self, LoadedIsSmoke(weap));

		int shotsLeft;
		if (!GetGLShotsLeft().Find(self, shotsLeft))
		{
			EndVolley(self);
			return;
		}

		shotsLeft--;

		if (shotsLeft <= 0)
		{
			EndVolley(self);
			return;
		}

		int uglIdx = GetUGLMuzzleIndex(weap);
		if (uglIdx == -1 || !HasUGLAmmo(self, weap, uglIdx))
		{
			EndVolley(self);
			return;
		}

		float now = GetGame().GetWorld().GetWorldTime();

		GetGLShotsLeft().Set(self, shotsLeft);
		GetGLCommitUntil().Set(self, now + 15000.0);
		GetGLCooldownUntil().Set(self, now + 15000.0 + 12000.0);
	}

	static void EndVolley(IEntity self)
	{
		if (!self)
			return;

		float now = GetGame().GetWorld().GetWorldTime();

		GetGLShotsLeft().Remove(self);
		GetGLCommitUntil().Remove(self);
		GetGLCooldownUntil().Set(self, now + 12000.0);
	}

	protected static bool IsFirstImpactSafe(IEntity self, vector targetPos)
	{
		vector start = self.GetOrigin();
		start[1] = start[1] + 1.6;

		vector end = targetPos;
		end[1] = end[1] + 0.5;

		float frac = DCO_Util.TraceFrac(start, end, self, TraceFlags.WORLD | TraceFlags.ENTS, EPhysicsLayerDefs.Projectile, "t:DCO_UGLUtility");
		if (frac >= 1.0)
			return true;

		float impactDist = vector.Distance(start, end) * frac;
		return impactDist >= 35.0;
	}

}
