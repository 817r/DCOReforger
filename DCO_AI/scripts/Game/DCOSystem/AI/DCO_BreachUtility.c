// Utility buat "breach grenade" -- lempar frag grenade lebih agresif sebelum AI masuk
// ke building/ruangan yang dicurigai ada musuh (CQB entry), beda dari logic grenade
// yang udah ada di Modded_DCO_UpdateAttackData (itu reaksi ke target yang invisible
// & unseen >2 detik, lebih general -- bukan spesifik context "mau breach masuk ruangan").
class DCO_BreachUtility
{
	protected static ref map<IEntity, float> s_mLastBreachThrowTime = new map<IEntity, float>();

	// --- Tuning ---
	static const float BREACH_COOLDOWN_MS      = 20000.0; // Jangan breach lebih dari 1x per 20 detik per soldier
	static const float BREACH_MAX_DIST         = 15.0;    // Jarak lempar maksimum ke titik masuk/target
	static const float BREACH_MIN_DIST         = 4.0;     // Kelewat deket -> bahaya kena ledakan sendiri, skip
	static const float BREACH_FRIENDLY_CHECK_RADIUS = 5.0; // Radius cek squadmate di sekitar titik lempar

	//------------------------------------------------------------------------------------------------
	//! Coba lempar frag grenade breach sebelum AI masuk building. Dipanggil dari titik
	//! combat move yang mutusin "target ada di dalam building, kita mau masuk".
	//! \param utility Utility milik AI yang mau breach
	//! \param entryPos Posisi target/entry point yang mau di-breach
	//! \return true kalau breach berhasil di-trigger
	static bool TryThrowBreachGrenade(SCR_AIUtilityComponent utility, vector entryPos)
	{
		if (!utility || !utility.m_CombatComponent)
			return false;

		if (!utility.m_CombatComponent.HasWeaponOfType(EWeaponType.WT_FRAGGRENADE))
			return false;

		IEntity myEntity = utility.m_OwnerEntity;
		if (!myEntity)
			return false;

		float distToEntry = vector.Distance(myEntity.GetOrigin(), entryPos);
		if (distToEntry < BREACH_MIN_DIST || distToEntry > BREACH_MAX_DIST)
			return false;

		// Jangan breach kalau ada squadmate deket titik lempar (friendly fire risk)
		if (HasFriendlyNear(myEntity, entryPos))
			return false;

		float worldTime_ms = GetGame().GetWorld().GetWorldTime();
		
		// === ADDED: Personality System ===
		// AGGRESSIVE lebih sering breach (cooldown pendek), CAUTIOUS lebih jarang.
		float personalityCooldownScale = GetPersonalityCooldownScale(utility);
		float effectiveCooldown = BREACH_COOLDOWN_MS * personalityCooldownScale;
		// === END ADDED ===
		
		float lastThrow;
		if (s_mLastBreachThrowTime.Find(myEntity, lastThrow))
		{
			if ((worldTime_ms - lastThrow) < effectiveCooldown)
				return false;
		}

		SCR_AIThrowGrenadeToBehavior breach = new SCR_AIThrowGrenadeToBehavior(
			utility, null, entryPos, EWeaponType.WT_FRAGGRENADE, 1,
			SCR_AIThrowGrenadeToBehavior.PRIORITY_BEHAVIOR_THROW_GRENADE + SCR_AIThrowGrenadeToBehavior.PRIORITY_LEVEL_PLAYER);
		utility.AddAction(breach);

		s_mLastBreachThrowTime.Set(myEntity, worldTime_ms);

		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected static ref array<IEntity> s_aFriendlyCheckResult = {};

	protected static float GetPersonalityCooldownScale(SCR_AIUtilityComponent utility)
	{
		if (!utility || !utility.m_DCOConfig)
			return 1.0;
		
		switch (utility.m_DCOConfig.GetPersonality())
		{
			case DCO_EAIPersonality.CAUTIOUS:
				return 2.0;   // 2x cooldown -- jarang breach
			case DCO_EAIPersonality.AGGRESSIVE:
				return 0.5;   // setengah cooldown -- lebih sering breach
			case DCO_EAIPersonality.RECKLESS:
				return 0.3;   // hampir gak ada jeda
			default:
				return 1.0;   // STANDARD
		}
		
		return 1.0;
	}
	// === END ADDED ===

	protected static bool HasFriendlyNear(IEntity self, vector pos)
	{
		s_aFriendlyCheckResult.Clear();

		FactionAffiliationComponent selfFac = FactionAffiliationComponent.Cast(self.FindComponent(FactionAffiliationComponent));
		if (!selfFac || !selfFac.GetAffiliatedFaction())
			return false;

		string myFactionKey = selfFac.GetAffiliatedFaction().GetFactionKey();

		GetGame().GetWorld().QueryEntitiesBySphere(pos, BREACH_FRIENDLY_CHECK_RADIUS, null, FriendlyQueryCallback, EQueryEntitiesFlags.DYNAMIC);

		foreach (IEntity ent : s_aFriendlyCheckResult)
		{
			if (!ent || ent == self)
				continue;

			FactionAffiliationComponent fac = FactionAffiliationComponent.Cast(ent.FindComponent(FactionAffiliationComponent));
			if (!fac || !fac.GetAffiliatedFaction())
				continue;

			if (fac.GetAffiliatedFaction().GetFactionKey() == myFactionKey)
				return true;
		}

		return false;
	}

	protected static bool FriendlyQueryCallback(IEntity ent)
	{
		if (ent)
			s_aFriendlyCheckResult.Insert(ent);
		return true;
	}
}