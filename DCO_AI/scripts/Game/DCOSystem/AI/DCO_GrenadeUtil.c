class DCO_GrenadeUtility
{
	protected static ref map<IEntity, float> s_mLastGrenadeThrowTime = new map<IEntity, float>();
	
	static const float GRENADE_COOLDOWN_MS = 20000.0; // per-unit, jangan lempar lebih dari 1x per 15 detik
	
	// === ADDED: batas jarak lempar.
	// Dua angka ini SEBELUMNYA dideklarasi di Modded_DCO_UpdateAttackData.c
	// (GRENADE_MIN_THROW_DIST / GRENADE_MAX_THROW_DIST) tapi GAK PERNAH dipakai --
	// gate aslinya hardcoded "< 20" di jalur attack dan "< 30" di jalur suppress,
	// dua-duanya TANPA batas bawah. Batas bawah itu justru yang nyegah unit
	// ngelempar ke kaki sendiri. Dipindah ke sini biar dua jalur pakai angka sama.
	static const float GRENADE_MIN_THROW_DIST = 5.0;
	static const float GRENADE_MAX_THROW_DIST = 40.0;
	
	// Geometri busur lemparan
	static const float THROW_ORIGIN_HEIGHT   = 1.6;   // tinggi tangan pas ngelempar
	static const float TARGET_CLEAR_HEIGHT   = 0.5;   // titik akhir busur dipatok segini di atas tanah
	static const float ARC_END_RATIO         = 0.85;  // busur dicek sampai 85% jarak aja (lihat catatan di ValidateArc)
	static const int   ARC_SEGMENTS          = 5;
	static const float APEX_RISE_RATIO       = 0.35;  // tinggi puncak busur relatif ke jarak
	static const float APEX_RISE_MIN         = 2.0;
	static const float APEX_RISE_MAX         = 8.0;
	static const float MIN_USABLE_HEADROOM   = 1.0;   // di bawah ini, ruangannya kependekan buat nge-lob
	static const float FRIENDLY_BLAST_RADIUS = 8.0;
	
	protected static ref array<IEntity> s_aBlastCheckResult = {};
	
	//------------------------------------------------------------------------------------------------
	//! Cek gabungan cooldown + personality chance. Return true = boleh lempar sekarang.
	//! TIDAK otomatis mulai cooldown -- caller yang beneran berhasil queue action harus
	//! manggil NotifyGrenadeThrown() abis ini biar cooldown mulai jalan.
	static bool CanThrowGrenadeNow(SCR_AIUtilityComponent utility)
	{
		if (!utility || !utility.m_OwnerEntity)
			return false;
		
		IEntity myEntity = utility.m_OwnerEntity;
		float worldTime_ms = GetGame().GetWorld().GetWorldTime();
		
		float lastThrow;
		if (s_mLastGrenadeThrowTime.Find(myEntity, lastThrow))
		{
			if ((worldTime_ms - lastThrow) < GRENADE_COOLDOWN_MS)
				return false;
		}
		
		float chance = DCO_PersonalityCombatUtility.GetGrenadeThrowChance(utility);
		if (Math.RandomFloat01() > chance)
			return false;
		
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Panggil abis AddAction(gren) berhasil di-queue, buat mulai cooldown unit ini.
	static void NotifyGrenadeThrown(SCR_AIUtilityComponent utility)
	{
		if (!utility || !utility.m_OwnerEntity)
			return;
		
		s_mLastGrenadeThrowTime.Set(utility.m_OwnerEntity, GetGame().GetWorld().GetWorldTime());
	}
	
	// === ADDED: validasi lintasan ===
	//------------------------------------------------------------------------------------------------
	//! Gerbang tunggal buat "granat ini bakal nyampe sana, atau mantul balik ke muka gue".
	//!
	//! KENAPA BUKAN CEK LOS BIASA: sebelumnya jalur attack pakai
	//! target.GetTraceFraction() > 0.5 sebagai izin lempar. Itu trace PELURU dari mata
	//! ke target, dan nilai > 0.5 justru berarti ADA yang ngalangin separuh jalan --
	//! alias ada tembok. Dipakai kebalik. Granat itu proyektil lob: keluar dari tangan,
	//! melengkung, mantul kalau kena apa-apa. Yang bener dicek adalah busurnya, bukan
	//! garis lurus mata-ke-target.
	//!
	//! Urutan cek disusun dari yang paling murah ke paling mahal, biar mayoritas
	//! panggilan berhenti sebelum nyentuh trace sama sekali.
	static bool IsThrowSafe(SCR_AIUtilityComponent utility, vector targetPos)
	{
		if (!utility || !utility.m_OwnerEntity)
			return false;
		
		IEntity self    = utility.m_OwnerEntity;
		vector  selfPos = self.GetOrigin();
		
		// --- 1. Jarak (nol trace) ---
		vector flatDelta = targetPos - selfPos;
		flatDelta[1] = 0.0;
		float dist = flatDelta.Length();
		
		if (dist < GRENADE_MIN_THROW_DIST)
			return false;
		
		if (dist > GRENADE_MAX_THROW_DIST)
			return false;
		
		// --- 2. Kawan di radius ledak titik jatuh (1 query, nol trace) ---
		if (HasFriendlyInBlast(self, targetPos))
			return false;
		
		vector start = selfPos;
		start[1] = start[1] + THROW_ORIGIN_HEIGHT;
		
		vector end = targetPos;
		end[1] = end[1] + TARGET_CLEAR_HEIGHT;
		
		float apexRise = Math.Clamp(dist * APEX_RISE_RATIO, APEX_RISE_MIN, APEX_RISE_MAX);
		
		// --- 3. Langit-langit (1 trace) ---
		// Bukan tolak-langsung: kalau ada plafon, busurnya diratain sesuai ruang yang
		// tersisa. Baru kalau ruangnya di bawah MIN_USABLE_HEADROOM kita nyerah --
		// itu kondisi "di dalem ruangan sempit", di mana nge-lob emang bunuh diri.
		float headroom = GetHeadroom(self, start, apexRise);
		if (headroom < MIN_USABLE_HEADROOM)
			return false;
		
		if (headroom < apexRise)
			apexRise = headroom;
		
		// --- 4. Sampling busur (ARC_SEGMENTS trace) ---
		return ValidateArc(self, start, end, apexRise);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Berapa meter ruang kosong lurus ke atas dari titik lempar. Return apexRise penuh
	//! kalau langit terbuka.
	protected static float GetHeadroom(IEntity self, vector start, float apexRise)
	{
		TraceParam param = new TraceParam();
		param.Start     = start;
		param.End       = start + (vector.Up * apexRise);
		param.Exclude   = self;
		param.LayerMask = EPhysicsLayerDefs.Projectile;
		
		float frac = GetGame().GetWorld().TraceMove(param, null);
		
		return apexRise * frac;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Sampling busur parabola jadi beberapa segmen lurus, tiap segmen di-trace.
	//!
	//! Busurnya cuma dicek sampai ARC_END_RATIO (85%) dari jarak, BUKAN sampai titik
	//! target. Alasannya: segmen terakhir pasti berujung deket tanah/target, jadi
	//! hampir selalu kena sesuatu -- entah tanahnya sendiri atau badan musuhnya. Kalau
	//! itu dihitung sebagai halangan, gak akan ada lemparan yang pernah lolos. Sisa 15%
	//! terakhir (~3m di jarak maksimum) udah cukup deket buat mantul pun tetap kena.
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
			
			if (world.TraceMove(param, null) < 1.0)
				return false;
			
			prev = point;
		}
		
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Titik di busur pada t (0 = tangan, 1 = target). Parabola simetris: suku
	//! 4*t*(1-t) puncaknya 1.0 tepat di t = 0.5, jadi apexRise itu tinggi puncak
	//! di atas garis lurus start-end.
	protected static vector ArcPoint(vector start, vector end, float apexRise, float t)
	{
		vector p;
		p[0] = start[0] + (end[0] - start[0]) * t;
		p[1] = start[1] + (end[1] - start[1]) * t + (apexRise * 4.0 * t * (1.0 - t));
		p[2] = start[2] + (end[2] - start[2]) * t;
		
		return p;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Ada kawan dalam radius ledak di TITIK JATUH.
	//!
	//! Ini beda sama GetFriendlyInLineOfFire() yang dipakai buat nembak: itu tes GARIS
	//! buat peluru. Granat mledak dalam radius, jadi yang relevan kawan di sekitar titik
	//! mendarat -- bukan kawan yang kebetulan ada di jalur lempar.
	protected static bool HasFriendlyInBlast(IEntity self, vector pos)
	{
		s_aBlastCheckResult.Clear();
		
		FactionAffiliationComponent selfFac = FactionAffiliationComponent.Cast(self.FindComponent(FactionAffiliationComponent));
		if (!selfFac || !selfFac.GetAffiliatedFaction())
			return false;
		
		string myFactionKey = selfFac.GetAffiliatedFaction().GetFactionKey();
		
		GetGame().GetWorld().QueryEntitiesBySphere(pos, FRIENDLY_BLAST_RADIUS, null, BlastQueryCallback, EQueryEntitiesFlags.DYNAMIC);
		
		foreach (IEntity ent : s_aBlastCheckResult)
		{
			if (!ent)
				continue;
			
			// Diri sendiri SENGAJA gak di-skip: kalau kita ada di radius ledak titik
			// jatuh, itu justru kasus yang paling pengen dicegah.
			FactionAffiliationComponent fac = FactionAffiliationComponent.Cast(ent.FindComponent(FactionAffiliationComponent));
			if (!fac || !fac.GetAffiliatedFaction())
				continue;
			
			if (fac.GetAffiliatedFaction().GetFactionKey() == myFactionKey)
				return true;
		}
		
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	protected static bool BlastQueryCallback(IEntity ent)
	{
		if (ent)
			s_aBlastCheckResult.Insert(ent);
		
		return true;
	}
	// === END ADDED ===
}