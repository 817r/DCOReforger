// === ADDED: helper UGL (grenade launcher bawah laras) ===
// UGL (M203 / GP-25) BUKAN weapon type sendiri -- dia muzzle kedua di senjata WT_RIFLE.
// Terverifikasi in-game: muzzle 0 = MT_BaseMuzzle (laras rifle), muzzle 1 = MT_UGLMuzzle.
// Index TIDAK di-hardcode: senjata mod bisa punya urutan muzzle beda.
//
// Call site:
//   - SCR_AIUpdateTargetAttackData.ResolveFireTree      -> ShouldUseGL()  (FIRE_TREE_GL = 8)
//   - SCR_AIUpdateTargetSuppressionData.ResolveFireTree -> ShouldUseGL()  (FIRE_TREE_GL = 4)
//   - node BT DCO_AIGetUGLWeapon                        -> GetUGLMuzzleIndex() + HasUGLAmmo()
//   - node BT DCO_AIGLShotDone                          -> NotifyGLShotDone()
//
// VOLLEY + COMMIT WINDOW:
// ResolveFireTree jalan TIAP TICK, dan cabang fire tree di Attack_Default / SuppressBehavior
// pakai DecoTestVariable yang nge-abort subtree begitu ID-nya berubah. Jadi setelah roll
// lolos, unit "dikunci" ke GL (commit) dan dapet jatah N butir (volley, sesuai personality).
//
// Tree GL tetap nembak SATU butir lalu selesai. Node DCO_AIGLShotDone di akhir tree
// ngurangin jatah + perpanjang commit. Attack_Default / SuppressBehavior otomatis ngulang
// cabangnya (ada Idle 3-8 detik habis RunBT), dan selama masih commit, ID yang keluar
// tetap GL -> butir berikutnya. Jeda antar-butir = reload + idle, sekitar 5-7 butir/menit.
//
// Volley berhenti kalau: jatah habis / amunisi habis / kawan masuk radius / timeout
// per-butir lewat. Cooldown baru jalan setelah volley selesai.
// Target kelihatan lagi -> cabang GL gak dicek (khusus target gak keliatan), unit
// otomatis balik ke rifle; kalau target hilang lagi sebelum timeout, volley lanjut.
class DCO_UGLUtility
{
	// Jarak
	static const float GL_MIN_DIST          = 35.0;    // di bawah ini ledakan terlalu deket ke diri sendiri
	static const float GL_MAX_DIST          = 300.0;
	
	// Timing (ms)
	static const float GL_ROLL_INTERVAL_MS  = 3000.0;  // roll chance maksimal 1x per interval per unit
	static const float GL_COMMIT_MS         = 10000.0; // timeout butir PERTAMA (switch muzzle + aim + fire)
	static const float GL_SHOT_TIMEOUT_MS   = 15000.0; // timeout butir BERIKUTNYA (reload + idle 3-8 dtk + aim)
	static const float GL_COOLDOWN_MS       = 12000.0; // jeda setelah volley selesai sebelum boleh roll lagi
	
	// Chance dasar per roll, dikali slider GL Usage (DCO_AIConfigComponent.GetGLUsage):
	// usage 0 -> gak pernah, 0.5 -> 0.6 (default), 1 -> 1.0 (clamp)
	static const float GL_FIRE_CHANCE       = 0.6;
	
	// Keamanan
	static const float GL_FRIENDLY_RADIUS   = 12.0;    // kawan dalam radius ini dari titik tembak -> batal
	static const float GL_ORIGIN_HEIGHT     = 1.6;     // tinggi laras perkiraan (berdiri)
	static const float GL_TARGET_HEIGHT     = 0.5;
	
	protected static ref array<BaseMuzzleComponent> s_aMuzzleBuffer = {};
	protected static ref array<IEntity> s_aItemBuffer = {};
	protected static ref array<IEntity> s_aBlastBuffer = {};
	protected static ref array<typename> s_aMagFilter = {MagazineComponent};
	
	protected static ref map<IEntity, float> s_mLastGLRollTime  = new map<IEntity, float>();
	protected static ref map<IEntity, float> s_mGLCommitUntil   = new map<IEntity, float>();
	protected static ref map<IEntity, float> s_mGLCooldownUntil = new map<IEntity, float>();
	protected static ref map<IEntity, int>   s_mGLShotsLeft     = new map<IEntity, int>();
	
	//------------------------------------------------------------------------------------------------
	//! Index muzzle UGL di senjata ini, atau -1 kalau gak ada.
	//! Sekaligus jawab "punya GL gak" (!= -1) dan "switch ke muzzle mana".
	static int GetUGLMuzzleIndex(BaseWeaponComponent weap)
	{
		if (!weap)
			return -1;
		
		s_aMuzzleBuffer.Clear();
		weap.GetMuzzlesList(s_aMuzzleBuffer);
		
		// Early-out murah: senjata satu muzzle pasti gak punya UGL
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
	
	//------------------------------------------------------------------------------------------------
	//! Shortcut buat gate: senjata ini punya UGL atau gak.
	static bool HasUGL(BaseWeaponComponent weap)
	{
		return GetUGLMuzzleIndex(weap) != -1;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Ada amunisi UGL: peluru yang udah ke-load di muzzle, ATAU magazine cadangan di inventory
	//! yang magazine well-nya sama dengan muzzle UGL. Scan inventory -- JANGAN dipanggil tiap tick,
	//! cuma setelah gate murah lolos.
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
		
		// --- 1. Udah ke-load ---
		BaseMagazineComponent loaded = muzzle.GetMagazine();
		if (loaded && loaded.GetAmmoCount() > 0)
			return true;
		
		// --- 2. Cadangan di inventory, dicocokin lewat magazine well ---
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
	
	//------------------------------------------------------------------------------------------------
	//! Gate utama GL. True = ResolveFireTree boleh return FIRE_TREE_GL tick ini.
	//! Urutan cek: murah dulu, scan inventory + query + trace cuma setelah roll lolos.
	static bool ShouldUseGL(SCR_AIUtilityComponent utility, BaseWeaponComponent weap, vector targetPos)
	{
		if (!utility || !utility.m_OwnerEntity)
			return false;
		
		IEntity self = utility.m_OwnerEntity;
		
		// --- 0. Slider GL Usage = 0 -> gak pernah (termasuk nutup volley yang lagi jalan) ---
		float usageScale = 1.0;
		if (utility.m_DCOConfig)
			usageScale = DCO_AIConfigComponent.UsageToChanceScale(utility.m_DCOConfig.GetGLUsage());
		
		if (usageScale <= 0.0)
		{
			if (s_mGLShotsLeft.Contains(self))
				EndVolley(self);
			return false;
		}
		
		// --- 1. Punya UGL (loop 2 muzzle, murah) ---
		int uglIdx = GetUGLMuzzleIndex(weap);
		if (uglIdx == -1)
			return false;
		
		// --- 2. Jarak (nol trace) ---
		float dist = vector.DistanceXZ(self.GetOrigin(), targetPos);
		if (dist < GL_MIN_DIST || dist > GL_MAX_DIST)
			return false;
		
		float now = GetGame().GetWorld().GetWorldTime();
		
		// --- 3. Lagi volley: tetap GL, tapi kawan dicek ulang (bisa aja jalan masuk ke area tembak) ---
		float commitUntil;
		if (s_mGLCommitUntil.Find(self, commitUntil) && now < commitUntil)
		{
			if (HasFriendlyNear(self, targetPos, GL_FRIENDLY_RADIUS))
			{
				EndVolley(self);
				return false;
			}
			return true;
		}
		
		// --- 4. Cooldown ---
		float cooldownUntil;
		if (s_mGLCooldownUntil.Find(self, cooldownUntil) && now < cooldownUntil)
			return false;
		
		// --- 5. Gate roll. Waktu roll ditandai SEBELUM roll (cegah re-roll tiap tick) ---
		float lastRoll;
		if (s_mLastGLRollTime.Find(self, lastRoll) && (now - lastRoll) < GL_ROLL_INTERVAL_MS)
			return false;
		
		s_mLastGLRollTime.Set(self, now);
		
		float chance = Math.Clamp(GL_FIRE_CHANCE * usageScale, 0.0, 1.0);
		if (Math.RandomFloat01() > chance)
			return false;
		
		// --- 6. Cek mahal ---
		if (!HasUGLAmmo(self, weap, uglIdx))
			return false;
		
		if (HasFriendlyNear(self, targetPos, GL_FRIENDLY_RADIUS))
			return false;
		
		if (!IsFirstImpactSafe(self, targetPos))
			return false;
		
		// --- Lolos: mulai volley. Cooldown dipasang di belakang timeout sebagai backstop
		// (kalau butir pertama gak pernah keluar, unit tetap kena cooldown) ---
		s_mGLShotsLeft.Set(self, GetVolleySize(utility));
		s_mGLCommitUntil.Set(self, now + GL_COMMIT_MS);
		s_mGLCooldownUntil.Set(self, now + GL_COMMIT_MS + GL_COOLDOWN_MS);
		
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Jumlah butir per volley berdasarkan personality. Slider GL Usage cuma ngatur
	//! seberapa sering volley dimulai, bukan jumlah butirnya.
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
	
	//------------------------------------------------------------------------------------------------
	//! Dipanggil node DCO_AIGLShotDone di akhir tree GL (satu butir udah keluar).
	//! Kurangi jatah; kalau masih ada jatah + amunisi, perpanjang commit buat butir berikutnya.
	static void NotifyGLShotDone(IEntity self, BaseWeaponComponent weap)
	{
		if (!self)
			return;
		
		int shotsLeft;
		if (!s_mGLShotsLeft.Find(self, shotsLeft))
		{
			// Tree GL jalan tanpa volley aktif (mis. commit udah timeout) -- tutup aja
			EndVolley(self);
			return;
		}
		
		shotsLeft--;
		
		if (shotsLeft <= 0)
		{
			EndVolley(self);
			return;
		}
		
		// Scan inventory -- sekali per butir, bukan tiap tick
		int uglIdx = GetUGLMuzzleIndex(weap);
		if (uglIdx == -1 || !HasUGLAmmo(self, weap, uglIdx))
		{
			EndVolley(self);
			return;
		}
		
		float now = GetGame().GetWorld().GetWorldTime();
		
		s_mGLShotsLeft.Set(self, shotsLeft);
		s_mGLCommitUntil.Set(self, now + GL_SHOT_TIMEOUT_MS);
		s_mGLCooldownUntil.Set(self, now + GL_SHOT_TIMEOUT_MS + GL_COOLDOWN_MS);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Tutup volley (jatah habis / amunisi habis / kawan masuk radius).
	//! Cooldown dihitung ulang dari SEKARANG, jadi selesai cepat = cooldown mulai cepat.
	static void EndVolley(IEntity self)
	{
		if (!self)
			return;
		
		float now = GetGame().GetWorld().GetWorldTime();
		
		s_mGLShotsLeft.Remove(self);
		s_mGLCommitUntil.Remove(self);
		s_mGLCooldownUntil.Set(self, now + GL_COOLDOWN_MS);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Titik tumbukan PERTAMA di garis laras -> target harus minimal GL_MIN_DIST dari diri sendiri.
	//! Target di balik tembok 60m -> tumbukan di tembok itu, masih aman (efek area tetap kena).
	//! Tembok 3m di depan muka -> tolak. Garis lurus lebih konservatif dari busur GL buat
	//! halangan rendah (busur lewat di atas garis), jadi ini gak nolak lemparan yang aman.
	protected static bool IsFirstImpactSafe(IEntity self, vector targetPos)
	{
		vector start = self.GetOrigin();
		start[1] = start[1] + GL_ORIGIN_HEIGHT;
		
		vector end = targetPos;
		end[1] = end[1] + GL_TARGET_HEIGHT;
		
		TraceParam param = new TraceParam();
		param.Start     = start;
		param.End       = end;
		param.Exclude   = self;
		param.Flags     = TraceFlags.WORLD | TraceFlags.ENTS;
		param.LayerMask = EPhysicsLayerDefs.Projectile;
		
		float frac = GetGame().GetWorld().TraceMove(param, null);
		if (frac >= 1.0)
			return true;
		
		float impactDist = vector.Distance(start, end) * frac;
		return impactDist >= GL_MIN_DIST;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Ada kawan (termasuk diri sendiri) dalam radius dari pos.
	protected static bool HasFriendlyNear(IEntity self, vector pos, float radius)
	{
		s_aBlastBuffer.Clear();
		
		FactionAffiliationComponent selfFac = FactionAffiliationComponent.Cast(self.FindComponent(FactionAffiliationComponent));
		if (!selfFac || !selfFac.GetAffiliatedFaction())
			return false;
		
		string myFactionKey = selfFac.GetAffiliatedFaction().GetFactionKey();
		
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
	
	//------------------------------------------------------------------------------------------------
	protected static bool BlastQueryCallback(IEntity ent)
	{
		if (ent)
			s_aBlastBuffer.Insert(ent);
		
		return true;
	}
}