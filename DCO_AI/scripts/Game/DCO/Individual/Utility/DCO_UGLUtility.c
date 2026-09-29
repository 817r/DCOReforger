class DCO_UGLUtility
{




	protected static ref array<BaseMuzzleComponent> s_aMuzzleBuffer = {};
	protected static ref array<IEntity> s_aItemBuffer = {};
	protected static ref array<IEntity> s_aBlastBuffer = {};
	protected static ref array<typename> s_aMagFilter = {MagazineComponent};

	protected static ref map<IEntity, float> s_mLastGLRollTime  = new map<IEntity, float>();
	protected static ref map<IEntity, float> s_mGLCommitUntil   = new map<IEntity, float>();
	protected static ref map<IEntity, float> s_mGLCooldownUntil = new map<IEntity, float>();
	protected static ref map<IEntity, int>   s_mGLShotsLeft     = new map<IEntity, int>();

	static int GetUGLMuzzleIndex(BaseWeaponComponent weap)
	{
		if (!weap)
			return -1;

		s_aMuzzleBuffer.Clear();
		weap.GetMuzzlesList(s_aMuzzleBuffer);

		int count = s_aMuzzleBuffer.Count();
		if (count < 2)
		{
			s_aMuzzleBuffer.Clear();
			return -1;
		}

		for (int i = 0; i < count; i++)
		{
			BaseMuzzleComponent muzzle = s_aMuzzleBuffer[i];
			if (muzzle && muzzle.GetMuzzleType() == EMuzzleType.MT_UGLMuzzle)
			{
				s_aMuzzleBuffer.Clear();
				return i;
			}
		}

		s_aMuzzleBuffer.Clear();
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

		s_aMuzzleBuffer.Clear();
		weap.GetMuzzlesList(s_aMuzzleBuffer);

		if (muzzleIdx >= s_aMuzzleBuffer.Count())
		{
			s_aMuzzleBuffer.Clear();
			return false;
		}

		BaseMuzzleComponent muzzle = s_aMuzzleBuffer[muzzleIdx];
		s_aMuzzleBuffer.Clear();

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

		s_aItemBuffer.Clear();
		inv.FindItemsWithComponents(s_aItemBuffer, s_aMagFilter);

		foreach (IEntity item : s_aItemBuffer)
		{
			if (!item)
				continue;

			MagazineComponent mag = MagazineComponent.Cast(item.FindComponent(MagazineComponent));
			if (!mag || !mag.GetMagazineWell())
				continue;

			if (mag.GetMagazineWell().Type() == wellType && mag.GetAmmoCount() > 0)
			{
				s_aItemBuffer.Clear();
				return true;
			}
		}

		s_aItemBuffer.Clear();
		return false;
	}

	static bool ShouldUseGL(SCR_AIUtilityComponent utility, BaseWeaponComponent weap, vector targetPos)
	{
		if (!utility || !utility.m_OwnerEntity)
			return false;

		IEntity self = utility.m_OwnerEntity;

		float usageScale = 1.0;
		if (utility.m_DCOConfig)
			usageScale = DCO_AIConfigComponent.UsageToChanceScale(utility.m_DCOConfig.GetGLUsage());

		if (usageScale <= 0.0)
		{
			if (s_mGLShotsLeft.Contains(self))
				EndVolley(self);
			return false;
		}

		int uglIdx = GetUGLMuzzleIndex(weap);
		if (uglIdx == -1)
			return false;

		float dist = vector.DistanceXZ(self.GetOrigin(), targetPos);
		if (dist < 35.0 || dist > 300.0)
			return false;

		float now = GetGame().GetWorld().GetWorldTime();

		float commitUntil;
		if (s_mGLCommitUntil.Find(self, commitUntil) && now < commitUntil)
		{
			if (HasFriendlyNear(self, targetPos, 15.0))
			{
				EndVolley(self);
				return false;
			}
			return true;
		}

		float cooldownUntil;
		if (s_mGLCooldownUntil.Find(self, cooldownUntil) && now < cooldownUntil)
			return false;

		float lastRoll;
		if (s_mLastGLRollTime.Find(self, lastRoll) && (now - lastRoll) < 3000.0)
			return false;

		s_mLastGLRollTime.Set(self, now);

		float chance = Math.Clamp(0.6 * usageScale, 0.0, 1.0);
		if (Math.RandomFloat01() > chance)
			return false;

		if (!HasUGLAmmo(self, weap, uglIdx))
			return false;

		if (HasFriendlyNear(self, targetPos, 15.0))
			return false;

		if (!IsFirstImpactSafe(self, targetPos))
			return false;

		s_mGLShotsLeft.Set(self, GetVolleySize(utility));
		s_mGLCommitUntil.Set(self, now + 10000.0);
		s_mGLCooldownUntil.Set(self, now + 10000.0 + 12000.0);

		return true;
	}

	protected static int GetVolleySize(SCR_AIUtilityComponent utility)
	{
		DCO_EAIPersonality p = DCO_EAIPersonality.STANDARD;
		if (utility && utility.m_DCOConfig)
			p = utility.m_DCOConfig.GetPersonality();

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

		int shotsLeft;
		if (!s_mGLShotsLeft.Find(self, shotsLeft))
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

		s_mGLShotsLeft.Set(self, shotsLeft);
		s_mGLCommitUntil.Set(self, now + 15000.0);
		s_mGLCooldownUntil.Set(self, now + 15000.0 + 12000.0);
	}

	static void EndVolley(IEntity self)
	{
		if (!self)
			return;

		float now = GetGame().GetWorld().GetWorldTime();

		s_mGLShotsLeft.Remove(self);
		s_mGLCommitUntil.Remove(self);
		s_mGLCooldownUntil.Set(self, now + 12000.0);
	}

	protected static bool IsFirstImpactSafe(IEntity self, vector targetPos)
	{
		vector start = self.GetOrigin();
		start[1] = start[1] + 1.6;

		vector end = targetPos;
		end[1] = end[1] + 0.5;

		TraceParam param = new TraceParam();
		param.Start     = start;
		param.End       = end;
		param.Exclude   = self;
		param.Flags     = TraceFlags.WORLD | TraceFlags.ENTS;
		param.LayerMask = EPhysicsLayerDefs.Projectile;

		DCO_Perf.Count("t:DCO_UGLUtility");
		float frac = GetGame().GetWorld().TraceMove(param, null);
		if (frac >= 1.0)
			return true;

		float impactDist = vector.Distance(start, end) * frac;
		return impactDist >= 35.0;
	}

	protected static bool HasFriendlyNear(IEntity self, vector pos, float radius)
	{
		s_aBlastBuffer.Clear();

		FactionAffiliationComponent selfFac = FactionAffiliationComponent.Cast(self.FindComponent(FactionAffiliationComponent));
		if (!selfFac || !selfFac.GetAffiliatedFaction())
			return false;

		string myFactionKey = selfFac.GetAffiliatedFaction().GetFactionKey();

		DCO_Perf.Count("q:DCO_UGLUtility");
		GetGame().GetWorld().QueryEntitiesBySphere(pos, radius, null, BlastQueryCallback, EQueryEntitiesFlags.DYNAMIC);

		foreach (IEntity ent : s_aBlastBuffer)
		{
			if (!ent)
				continue;

			FactionAffiliationComponent fac = FactionAffiliationComponent.Cast(ent.FindComponent(FactionAffiliationComponent));
			if (!fac || !fac.GetAffiliatedFaction())
				continue;

			if (fac.GetAffiliatedFaction().GetFactionKey() == myFactionKey)
			{
				s_aBlastBuffer.Clear();
				return true;
			}
		}

		s_aBlastBuffer.Clear();
		return false;
	}

	protected static bool BlastQueryCallback(IEntity ent)
	{
		if (ent)
			s_aBlastBuffer.Insert(ent);

		return true;
	}
}
