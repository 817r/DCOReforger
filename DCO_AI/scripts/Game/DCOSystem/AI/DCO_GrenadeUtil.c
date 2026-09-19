// === ADDED: mode lemparan hasil ResolveThrowPos (buat debug) ===
enum DCO_EGrenadeThrowMode
{
	NONE,
	DIRECT,		// busur langsung ke target
	ROLL_IN,	// mendarat di depan target lalu menggelinding masuk
	BANK		// dipantulkan ke tembok di dekat target
}

class DCO_GrenadeUtility
{
	protected static ref map<IEntity, float> s_mLastGrenadeThrowTime = new map<IEntity, float>();
	
	// === ADDED: gate roll. ResolveFireTree jalan tiap tick -- tanpa gate ini chance
	// personality di-roll ulang tiap frame sampai tembus, jadi praktis selalu 100%.
	// Waktu roll ditandai SEBELUM roll (cegah spam), sekaligus jadi budget resolver:
	// ResolveThrowPos (yang pakai banyak trace) maksimal jalan 1x per interval per unit.
	protected static ref map<IEntity, float> s_mLastGrenadeRollTime = new map<IEntity, float>();
	static const float GRENADE_ROLL_INTERVAL_MS = 3000.0;
	
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
		
		// === ADDED: gate roll ===
		float lastRoll;
		if (s_mLastGrenadeRollTime.Find(myEntity, lastRoll))
		{
			if ((worldTime_ms - lastRoll) < GRENADE_ROLL_INTERVAL_MS)
				return false;
		}
		
		s_mLastGrenadeRollTime.Set(myEntity, worldTime_ms);
		// === END ADDED ===
		
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
	
	//================================================================================================
	// === ADDED: RESOLVER TITIK LEMPAR (direct -> roll-in -> wall bank) ===
	//
	// Model lintasan diambil dari vanilla. SCR_AIGetAimDistanceCompensation buat granat manggil
	// BallisticTable.GetHeightFromProjectile(jarak3D, out waktu, entityGranat, coef) -> heightOffset,
	// lalu AI membidik ke (target + up * heightOffset) dari tangan. Artinya garis bidik = arah
	// lempar awal, dan lintasannya (tanpa drag) adalah parabola yang lewat tangan & target:
	//
	//   y(u) = startY + (dy + h) * u - h * u^2     u = 0..1 sepanjang jarak horizontal
	//
	// Turunan di u=0 persis kemiringan garis bidik, y(1) = targetY. Gak butuh konstanta
	// kecepatan atau gravitasi -- semua dari ballistic table yang sama dengan yang dipakai vanilla.
	//
	// Titik yang dihasilkan dioper ke SCR_AIThrowGrenadeToBehavior sebagai TargetPosition,
	// vanilla yang ngitung sudutnya. Kita cuma milih TITIK TUMBUKAN PERTAMA.
	//================================================================================================
	
	static const float INIT_SPEED_COEF        = 1.0;   // ThrowGrenadeTo.bt gak nyambungin port ini -> default 1.0
	
	// Roll-in
	static const float ROLL_RATIO_START       = 0.9;   // kandidat di 90% / 80% / 70% jarak
	static const float ROLL_RATIO_STEP        = 0.1;
	static const int   ROLL_RATIO_COUNT       = 3;
	static const float ROLL_LATERAL_OFFSET    = 1.5;   // offset kiri/kanan tiap rasio
	static const float ROLL_MAX_DIST          = 4.0;   // jarak gulir maksimum titik mendarat -> target
	static const float ROLL_MAX_RISE          = 0.5;   // granat gak ngegelinding nanjak
	static const float ROLL_PATH_HEIGHT       = 0.3;   // tinggi trace jalur gulir / jalur pantul
	
	// Wall bank
	static const float BANK_MAX_DIST          = 15.0;  // cuma jarak pendek (CQB / breach)
	static const int   BANK_DIR_COUNT         = 8;
	static const float BANK_WALL_SEARCH       = 3.0;   // radius cari tembok dari target
	static const float BANK_PROBE_HEIGHT      = 1.0;
	static const float BANK_MAX_REBOUND       = 3.0;   // panjang kaki kedua (tembok -> target) maksimum
	static const float BANK_RESTITUTION       = 0.4;   // TUNE IN-GAME: kehilangan energi pas mantul
	static const float BANK_IMPACT_MIN_H      = 0.2;   // tinggi tumbukan ke tembok relatif lantai target
	static const float BANK_IMPACT_MAX_H      = 2.2;
	static const float BANK_APPROACH_RATIO    = 0.9;   // busur dicek sampai 90% jalan ke tembok
	static const float BANK_WALL_OFFSET       = 0.3;
	
	// Snap tanah kandidat
	static const float GROUND_SNAP_UP         = 1.0;
	static const float GROUND_SNAP_DOWN       = 4.0;
	
	// Debug: set true di Workbench buat gambar busur kandidat + print mode
	static const bool  DEBUG_THROW            = false;
	
	protected static ref map<IEntity, IEntity> s_mCachedGrenade = new map<IEntity, IEntity>();
	protected static ref array<IEntity> s_aItemBuffer = {};
	
	#ifdef WORKBENCH
	protected static ref array<ref Shape> s_aDbgShapes = {};
	#endif
	
	//------------------------------------------------------------------------------------------------
	//! Gerbang tunggal lempar frag. Return true + throwPos = titik yang harus dioper ke
	//! SCR_AIThrowGrenadeToBehavior. Bisa sama dengan targetPos (direct) atau titik lain
	//! (roll-in / bank). Return false = gak ada lemparan yang aman.
	static bool ResolveThrowPos(SCR_AIUtilityComponent utility, vector targetPos, out vector throwPos)
	{
		throwPos = targetPos;
		
		if (!utility || !utility.m_OwnerEntity)
			return false;
		
		IEntity self    = utility.m_OwnerEntity;
		vector  selfPos = self.GetOrigin();
		
		DbgClear();
		
		// --- Jarak (nol trace) ---
		float dist = vector.DistanceXZ(selfPos, targetPos);
		if (dist < GRENADE_MIN_THROW_DIST || dist > GRENADE_MAX_THROW_DIST)
			return false;
		
		// --- Radius ledak di titik AKHIR (target), berlaku buat semua mode ---
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
		else
		{
			vector altPos;
			if (FindRollInPos(self, grenade, start, targetPos, altPos))
			{
				throwPos = altPos;
				mode     = DCO_EGrenadeThrowMode.ROLL_IN;
			}
			else if (dist <= BANK_MAX_DIST && FindBankPos(self, grenade, start, targetPos, altPos))
			{
				throwPos = altPos;
				mode     = DCO_EGrenadeThrowMode.BANK;
			}
		}
		
		if (DEBUG_THROW)
		{
			Print(string.Format("[DCO_Grenade] %1 dist=%2 mode=%3 realBallistics=%4 throwPos=%5",
				self, dist, typename.EnumToString(DCO_EGrenadeThrowMode, mode), grenade != null, throwPos), LogLevel.NORMAL);
		}
		
		return mode != DCO_EGrenadeThrowMode.NONE;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Koefisien parabola y(u) = startY + a*u + b*u^2 dari tangan ke end.
	//! Pakai ballistic table vanilla kalau entity granat ketemu; kalau gak, fallback ke model
	//! apex lama (APEX_RISE_*) supaya tetap bisa jalan.
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
	
	//------------------------------------------------------------------------------------------------
	protected static vector ArcPointAt(vector start, vector end, float a, float b, float u)
	{
		vector p;
		p[0] = start[0] + (end[0] - start[0]) * u;
		p[1] = start[1] + a * u + b * u * u;
		p[2] = start[2] + (end[2] - start[2]) * u;
		return p;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Sampling lintasan asli jadi ARC_SEGMENTS segmen dari u=0 sampai uEnd, tiap segmen di-trace.
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
	
	//------------------------------------------------------------------------------------------------
	protected static bool IsSegmentClear(IEntity self, vector from, vector to)
	{
		TraceParam param = new TraceParam();
		param.Start     = from;
		param.End       = to;
		param.Exclude   = self;
		param.Flags     = TraceFlags.WORLD | TraceFlags.ENTS;
		param.LayerMask = EPhysicsLayerDefs.Projectile;
		
		return GetGame().GetWorld().TraceMove(param, null) >= 1.0;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Titik lantai di bawah pos, dicari di sekitar ketinggian refY (lantai target).
	protected static bool SnapToGround(IEntity self, vector pos, float refY, out vector ground)
	{
		vector from = pos;
		from[1] = refY + GROUND_SNAP_UP;
		
		vector to = pos;
		to[1] = refY - GROUND_SNAP_DOWN;
		
		TraceParam param = new TraceParam();
		param.Start     = from;
		param.End       = to;
		param.Exclude   = self;
		param.Flags     = TraceFlags.WORLD | TraceFlags.ENTS;
		param.LayerMask = EPhysicsLayerDefs.Projectile;
		
		float frac = GetGame().GetWorld().TraceMove(param, null);
		if (frac >= 1.0 || frac <= 0.0)
			return false;
		
		ground = from + (to - from) * frac;
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	//! A. ROLL-IN: mendarat lebih pendek / geser samping, lalu menggelinding ke target.
	//! Berguna kalau ujung busur ketutup (atap teras, pintu, pohon) tapi jalur tanahnya terbuka.
	protected static bool FindRollInPos(IEntity self, IEntity grenade, vector start, vector target, out vector outPos)
	{
		vector selfPos = self.GetOrigin();
		
		vector toTarget = target - selfPos;
		toTarget[1] = 0;
		
		float d = toTarget.Length();
		if (d < 0.5)
			return false;
		
		vector fwd   = toTarget * (1.0 / d);
		vector right = Vector(fwd[2], 0, -fwd[0]);
		vector pathUp = vector.Up * ROLL_PATH_HEIGHT;
		
		for (int i = 0; i < ROLL_RATIO_COUNT; i++)
		{
			float ratio = ROLL_RATIO_START - i * ROLL_RATIO_STEP;
			
			for (int j = 0; j < 3; j++)
			{
				float lateral = 0;
				if (j == 1)
					lateral = ROLL_LATERAL_OFFSET;
				else if (j == 2)
					lateral = -ROLL_LATERAL_OFFSET;
				
				vector cand = selfPos + fwd * (d * ratio) + right * lateral;
				
				// --- Geometri murah dulu ---
				vector ground;
				if (!SnapToGround(self, cand, target[1], ground))
					continue;
				
				if (vector.DistanceXZ(ground, target) > ROLL_MAX_DIST)
					continue;
				
				if (target[1] - ground[1] > ROLL_MAX_RISE)
					continue;
				
				if (vector.DistanceXZ(selfPos, ground) < GRENADE_MIN_THROW_DIST)
					continue;
				
				// Kalau ternyata gak ngegelinding, dia meledak di titik mendarat
				if (HasFriendlyInBlast(self, ground))
					continue;
				
				// --- Jalur gulir ---
				if (!IsSegmentClear(self, ground + pathUp, target + pathUp))
				{
					DbgLine(ground + pathUp, target + pathUp, Color.RED);
					continue;
				}
				
				// --- Busur ke titik mendarat ---
				if (!IsTrajectoryClear(self, grenade, start, ground, ARC_END_RATIO))
					continue;
				
				DbgLine(ground + pathUp, target + pathUp, Color.YELLOW);
				outPos = ground;
				return true;
			}
		}
		
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	//! B. WALL BANK (metode cermin): target dicerminkan terhadap bidang tembok, lalu kaki kedua
	//! diperpanjang 1/restitusi. Vanilla membidik ke titik virtual di balik tembok itu; lintasannya
	//! nabrak tembok di titik pantul, dan setelah kehilangan energi, pantulannya berhenti dekat target.
	protected static bool FindBankPos(IEntity self, IEntity grenade, vector start, vector target, out vector outPos)
	{
		vector selfPos = self.GetOrigin();
		vector probe   = target + vector.Up * BANK_PROBE_HEIGHT;
		
		for (int k = 0; k < BANK_DIR_COUNT; k++)
		{
			float angRad = (k * 360.0 / BANK_DIR_COUNT) * Math.DEG2RAD;
			vector dir = Vector(Math.Cos(angRad), 0, Math.Sin(angRad));
			
			// --- Cari tembok di sekitar target ---
			TraceParam wallTrace = new TraceParam();
			wallTrace.Start     = probe;
			wallTrace.End       = probe + dir * BANK_WALL_SEARCH;
			wallTrace.Exclude   = self;
			wallTrace.Flags     = TraceFlags.WORLD | TraceFlags.ENTS;
			wallTrace.LayerMask = EPhysicsLayerDefs.Projectile;
			
			float frac = GetGame().GetWorld().TraceMove(wallTrace, null);
			if (frac >= 1.0)
				continue;
			
			vector wallPt = probe + dir * (BANK_WALL_SEARCH * frac);
			
			// Normal horizontal; permukaan yang terlalu miring (lantai/atap) dibuang
			vector n = wallTrace.TraceNorm;
			n[1] = 0;
			float nLen = n.Length();
			if (nLen < 0.5)
				continue;
			n = n * (1.0 / nLen);
			
			// --- Pelempar harus ada di depan muka tembok ---
			vector startToWall = start - wallPt;
			startToWall[1] = 0;
			float startPlaneDist = vector.Dot(startToWall, n);
			if (startPlaneDist <= 0.5)
				continue;
			
			vector targetToWall = target - wallPt;
			targetToWall[1] = 0;
			float targetPlaneDist = vector.Dot(targetToWall, n);
			if (targetPlaneDist <= 0.1)
				continue;
			
			// --- Cermin target & titik pantul ---
			vector mirror = target - n * (2.0 * targetPlaneDist);
			
			vector startToMirror = mirror - start;
			startToMirror[1] = 0;
			float denom = vector.Dot(startToMirror, n);
			if (denom > -0.01)
				continue;
			
			float tHit = startPlaneDist / -denom;
			if (tHit <= 0 || tHit >= 1)
				continue;
			
			vector bank = start + startToMirror * tHit;
			bank[1] = target[1];
			
			float leg2 = vector.DistanceXZ(bank, target);
			if (leg2 < 0.1 || leg2 > BANK_MAX_REBOUND)
				continue;
			
			// --- Titik virtual: kaki kedua diperpanjang 1/restitusi ---
			vector legDir = mirror - bank;
			legDir[1] = 0;
			vector virt = bank + legDir * (1.0 / BANK_RESTITUTION);
			virt[1] = target[1];
			
			if (vector.DistanceXZ(selfPos, virt) > GRENADE_MAX_THROW_DIST)
				continue;
			
			// --- Tinggi tumbukan ke tembok harus masuk akal ---
			float a, b;
			if (!ComputeArcCoeffs(grenade, start, virt, a, b))
				continue;
			
			float virtDist = vector.DistanceXZ(start, virt);
			if (virtDist < 0.5)
				continue;
			
			float uWall = vector.DistanceXZ(start, bank) / virtDist;
			float yWall = start[1] + a * uWall + b * uWall * uWall;
			
			if (yWall < target[1] + BANK_IMPACT_MIN_H || yWall > target[1] + BANK_IMPACT_MAX_H)
				continue;
			
			vector impact = bank;
			impact[1] = yWall;
			
			// --- Tembok beneran ada di titik & ketinggian tumbukan ---
			if (IsSegmentClear(self, impact + n * 0.5, impact - n * 0.3))
				continue;
			
			// --- Busur sampai tembok ---
			if (!IsTrajectoryClear(self, grenade, start, virt, uWall * BANK_APPROACH_RATIO))
				continue;
			
			// --- Jalur pantul ke target ---
			vector rebound = impact + n * BANK_WALL_OFFSET;
			vector targetPath = target + vector.Up * ROLL_PATH_HEIGHT;
			if (!IsSegmentClear(self, rebound, targetPath))
			{
				DbgLine(rebound, targetPath, Color.RED);
				continue;
			}
			
			DbgLine(rebound, targetPath, Color.BLUE);
			outPos = virt;
			return true;
		}
		
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Entity frag di inventory (dipakai ballistic table sebelum AI switch ke granat).
	//! Di-cache per unit; cache dianggap basi kalau granatnya udah gak di badan unit (udah dilempar).
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
	
	//------------------------------------------------------------------------------------------------
	protected static void DbgClear()
	{
		#ifdef WORKBENCH
		if (DEBUG_THROW)
			s_aDbgShapes.Clear();
		#endif
	}
	
	//------------------------------------------------------------------------------------------------
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
	// === END ADDED (resolver) ===
}