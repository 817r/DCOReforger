modded class SCR_AIUpdateTargetAttackData : AITaskScripted
{	
	protected const int FIRE_TREE_INVALID 		= -1;	// No aiming or firing is allowed at all
	protected const int FIRE_TREE_LOOK			= 0;	// Looking at target without firing
	protected const int FIRE_TREE_BURST			= 1;
	protected const int FIRE_TREE_SINGLE		= 2;
	protected const int FIRE_TREE_SUPPRESSIVE	= 3;
	protected const int FIRE_TREE_MELEE			= 4;
	protected const int FIRE_TREE_LOOK_THREATS	= 5;	// Looking at data from threat system
	protected const int FIRE_TREE_THROW_GRENADE	= 6;
	protected const int FIRE_TREE_RPG			= 7;
	
	
	protected const float BURST_FIRE_MAX_DISTANCE = 70.0;
	
	// === ADDED: HVT urgency tuning ===
	protected const float HVT_FIRERATE_BOOST = 1.5;
	protected const float HVT_CLOSER_THREAT_MARGIN_M = 20.0; // kalau ada threat lain minimal segini lebih deket dari HVT, turunin urgency
	// === END ADDED ===
	
	// === ADDED: Responsive close-range override ===
	protected const float CLOSE_DIRECT_THREAT_DIST = 20.0; // dalam jarak ini + visible, AI harus tetep bisa balas nembak walau lagi combat-move
	// === END ADDED ===
	
	// === ADDED: Grenade throw (visible-obstructed) tuning ===
	protected const float GRENADE_MIN_THROW_DIST = 5.0;  // kelewat deket -> bahaya ledakan sendiri
	protected const float GRENADE_MAX_THROW_DIST = 25.0; // kelewat jauh -> gak akurat/gak nyampe
	// === END ADDED ===

	//-----------------------------------------------------------------------------------------------------
	// Evaluates which fire tree should be used
	override int ResolveFireTree(BaseTarget target, bool visible, bool weaponReady, out float fireRate)
	{
		// Is aiming forbidden by combat move?
		SCR_AIBehaviorBase executedBehavior = SCR_AIBehaviorBase.Cast(m_UtilityComponent.GetExecutedAction());
		if (executedBehavior && executedBehavior.m_bUseCombatMove && !m_UtilityComponent.m_CombatMoveState.m_bAimAtTarget)
		{
			// === ADDED: Responsive close-range override ===
			// Kalau target deket & visible (lagi nembakin kita dari jarak deket), jangan
			// taat buta ke flag combat-move -- AI harus tetep bisa balas nembak. Cuma
			// berlaku kalau visible, biar gak asal tembak ke arah kosong.
			if (!IsCloseDirectThreat(target, visible))
				return FIRE_TREE_LOOK_THREATS;
			// === END ADDED ===
		}
		
		// Is looking at threats activated?
		if (m_bLookAtThreats)
			return FIRE_TREE_LOOK_THREATS;
		
		// Is weapon not ready?
		if (!weaponReady)
			return FIRE_TREE_LOOK;
		
		
		
		BaseWeaponComponent selectedWeaponComp;
		int selectedMuzzleId;
		m_CombatComponent.GetSelectedWeapon(selectedWeaponComp, selectedMuzzleId);
		
		bool directDamage; // FIX: pre-existing syntax bug, titik-koma kelewat di sini (sebelum ada perubahan apapun dari kita)
		float weaponMinDist, weaponMaxDist;
		m_CombatComponent.GetSelectedWeaponProperties(weaponMinDist, weaponMaxDist, directDamage);
		EWeaponType weaponType = selectedWeaponComp.GetWeaponType();
		
		if (!selectedWeaponComp)
			return FIRE_TREE_LOOK;
		
		// Hold fire?
		if (m_CombatComponent.GetCombatMode() == EAIGroupCombatMode.HOLD_FIRE)
			return FIRE_TREE_LOOK;
		
		
		
		float targetDistance = target.GetDistance();
		
		// Use melee?
		if (targetDistance < MELEE_MAX_DISTANCE &&
			!m_CharacterController.CanFire() &&
			m_CharacterController.GetStance() != ECharacterStance.PRONE)
		{
			return FIRE_TREE_MELEE;
		}
		
		// Friendly in aim?
		if (m_PerceptionComponent.GetFriendlyInLineOfFire())
		{
			return FIRE_TREE_LOOK;
		}
	
		/*if (target.GetUnitType() == EAIUnitType.UnitType_Aircraft)
		{
			if (weaponType == EWeaponType.WT_MACHINEGUN)
			{
				if (target.GetDistance() > 50)
					return FIRE_TREE_LOOK;
				else if (target.GetTraceFraction() > 0.8)
				{
					return FIRE_TREE_SUPPRESSIVE;
				}
			} else if (!weaponType == EWeaponType.WT_ROCKETLAUNCHER || weaponType == EWeaponType.WT_MACHINEGUN)
			{
				return FIRE_TREE_LOOK;
			}
		}*/
		
		// Not melee
		float threat = m_UtilityComponent.m_ThreatSystem.GetThreatMeasure();
		if (targetDistance < weaponMinDist || targetDistance > weaponMaxDist)
		{
			// Outside weapon usage range
			// Look at target
			
			if (weaponType == EWeaponType.WT_MACHINEGUN)
			{
				float maxFireRate = Math.Max(1, Math.Map(targetDistance, 0, SCR_AICombatComponent.LONG_RANGE_COMBAT_DISTANCE, 4, 1));
				fireRate = maxFireRate * threat;
				
				// === MODIFIED: High-Value Target prioritization -- pake urgency scale,
				// bukan flat boost, biar turun kalau ada threat lain yang lebih deket ===
				if (IsHighValueTarget(target, visible))
					fireRate *= GetHVTUrgencyScale(targetDistance);
				// === END MODIFIED ===
				
				return FIRE_TREE_SUPPRESSIVE;
			}
			return FIRE_TREE_LOOK;
		}

		// Within weapon usage range
		
		if (visible)
		{
			// === MODIFIED: High-Value Target prioritization -- burst-force cuma kalau
			// urgency masih tinggi (gak ada threat lain yang lebih deket) ===
			bool isHVT = IsHighValueTarget(target, visible) && GetHVTUrgencyScale(targetDistance) > 1.0;
			// === END MODIFIED ===
			
			// === ADDED: Personality System -- AGGRESSIVE/RECKLESS berani burst dari
			// lebih jauh, CAUTIOUS lebih pendek (hemat amunisi, lebih kontrol) ===
			float effectiveBurstMaxDist = BURST_FIRE_MAX_DISTANCE * DCO_PersonalityCombatUtility.GetBurstDistanceScale(m_UtilityComponent);
			// === END ADDED ===
			
			// === ADDED: Grenade throw -- target VISIBLE tapi susah kena (trace fraction
			// jelek = kehalang cover/object walau keliatan). Sebelumnya grenade cuma
			// pernah dipertimbangkan buat target INVISIBLE, jadi skenario paling umum
			// "musuh di belakang sandbag/tembok tapi keliatan" gak pernah ke-cover.
			if (target.GetTraceFraction() < 0.6 && targetDistance > GRENADE_MIN_THROW_DIST && targetDistance < GRENADE_MAX_THROW_DIST
				&& m_CombatComponent.HasWeaponOfType(EWeaponType.WT_FRAGGRENADE)
				&& DCO_GrenadeUtility.CanThrowGrenadeNow(m_UtilityComponent))
			{
				SCR_AIThrowGrenadeToBehavior gren = new SCR_AIThrowGrenadeToBehavior(m_UtilityComponent, null, target.GetLastSeenPosition(), EWeaponType.WT_FRAGGRENADE, 1, SCR_AIThrowGrenadeToBehavior.PRIORITY_BEHAVIOR_THROW_GRENADE +
				SCR_AIThrowGrenadeToBehavior.PRIORITY_LEVEL_PLAYER);
				m_UtilityComponent.AddAction(gren);
				DCO_GrenadeUtility.NotifyGrenadeThrown(m_UtilityComponent);
				return FIRE_TREE_LOOK;
			}
			// === END ADDED ===
			
			// Visible
			// If machinegun, always use burst at any range
			// For regular weapons, use burst at short range if available, otherwise single
			if (weaponType == EWeaponType.WT_MACHINEGUN)
				return FIRE_TREE_BURST;
			// === ADDED: Grenade Launcher -- weapon indirect/arc, bukan direct-fire kayak
			// rifle. Sebelumnya gak ada case sendiri, jatuh ke logic burst/single generic
			// yang gak cocok buat weapon jenis ini. Diperlakukan kayak MG di outside-range
			// branch -- selalu suppressive/area fire. ===
			else if (weaponType == EWeaponType.WT_GRENADELAUNCHER)
				return FIRE_TREE_SUPPRESSIVE;
			// === END ADDED ===
			else if ((targetDistance < effectiveBurstMaxDist || isHVT) && m_bWeaponHasBurstOrAuto) // MODIFIED: "|| isHVT" -- HVT dapet burst walau di luar jarak burst normal
				return FIRE_TREE_BURST;
			else
				return FIRE_TREE_SINGLE;
		}
		else
		{
			// Invisible
			// Use suppressive fire or don't fire at all
			
			// If weapon is configured to deal indirect damage, then use it for invisible target
			// Otherwise regular weapons can also be used to fire at hidden target,
			// except for rocket launchers, their ammo is too valuable
			// Also ensure we don't do suppressive fire into a wall in front of us
			
			float lastSeenThreshold;
			if (weaponType == EWeaponType.WT_MACHINEGUN)
				lastSeenThreshold = SCR_AICombatComponent.TARGET_MAX_LAST_SEEN_INDIRECT_ATTACK_MG;
			else
			{
				if (targetDistance < SCR_AICombatComponent.CLOSE_RANGE_COMBAT_DISTANCE)
					lastSeenThreshold = SCR_AICombatComponent.TARGET_MAX_LAST_SEEN_INDIRECT_ATTACK_CLOSE;
				else
					lastSeenThreshold = SCR_AICombatComponent.TARGET_MAX_LAST_SEEN_INDIRECT_ATTACK;
			}
			
			lastSeenThreshold = Math.Max(SCR_AICombatComponent.TARGET_MIN_LAST_SEEN_INDIRECT_ATTACK, lastSeenThreshold * threat);
			
			if ((!directDamage || weaponType != EWeaponType.WT_ROCKETLAUNCHER) &&
				target.GetTimeSinceSeen() < lastSeenThreshold &&
				target.GetTraceFraction() > 0.5)
			{
				float maxFireRate = Math.Max(1, Math.Map(targetDistance, 0, SCR_AICombatComponent.LONG_RANGE_COMBAT_DISTANCE, 4, 1));
				fireRate = maxFireRate * threat;
				
				// === MODIFIED: High-Value Target prioritization -- urgency scale ===
				if (IsHighValueTarget(target, false))
					fireRate *= GetHVTUrgencyScale(targetDistance);
				// === END MODIFIED ===
								
				return FIRE_TREE_SUPPRESSIVE;
			}
			//else if (target.GetTimeSinceSeen() > 3 && (target.GetDistance() > 50 && m_CombatComponent.HasWeaponOfType(EWeaponType.WT_ROCKETLAUNCHER)))
			//{
			//	return FIRE_TREE_RPG;
			//}
			// === MODIFIED: widen jarak (15->20m) + gate lewat DCO_GrenadeUtility
			// (cooldown per-unit + personality chance), biar gak dead-simple fallback
			// yang jarang ke-hit dan gak spam kalau kondisi kepenuhin banyak tick ===
			else if (target.GetTimeSinceSeen() > 2 && target.GetDistance() < 20 && target.GetTraceFraction() > 0.5
				&& m_CombatComponent.HasWeaponOfType(EWeaponType.WT_FRAGGRENADE)
				&& DCO_GrenadeUtility.CanThrowGrenadeNow(m_UtilityComponent))
			{
				SCR_AIThrowGrenadeToBehavior gren = new SCR_AIThrowGrenadeToBehavior(m_UtilityComponent, null, target.GetLastSeenPosition(), EWeaponType.WT_FRAGGRENADE, 1, SCR_AIThrowGrenadeToBehavior.PRIORITY_BEHAVIOR_THROW_GRENADE + 
				SCR_AIThrowGrenadeToBehavior.PRIORITY_LEVEL_PLAYER);
				m_UtilityComponent.AddAction(gren);
				DCO_GrenadeUtility.NotifyGrenadeThrown(m_UtilityComponent);
				return FIRE_TREE_LOOK;
			}
			// === END MODIFIED ===
			else
				return FIRE_TREE_LOOK;
		}
		
		return FIRE_TREE_LOOK;
	}
	
	//-----------------------------------------------------------------------------------------------------
	override void ResolveAimpointTypes(notnull BaseTarget target, out EAimPointType aimpointType0, out EAimPointType aimpointType1)
	{
		IEntity targetEntity = target.GetTargetEntity();
		if (!targetEntity)
		{
			aimpointType0 = -1;
			aimpointType1 = -1;
			return;
		}
		
		EWeaponType weaponType = m_CombatComponent.GetSelectedWeaponType();
		ChimeraCharacter character = ChimeraCharacter.Cast(targetEntity);
		if (character)
		{
			// Characters
			
			if (character.IsInVehicle())
			{
				// Aim at head
				aimpointType0 = EAimPointType.WEAK;
				aimpointType1 = EAimPointType.NORMAL;
				return;
			}
			
			if (weaponType == EWeaponType.WT_SNIPERRIFLE)
			{
				aimpointType0 = EAimPointType.INCAPACITATE;
				aimpointType1 = EAimPointType.WEAK;
				return;
			}
			
			aimpointType0 = EAimPointType.NORMAL;
			aimpointType1 = EAimPointType.WEAK;
			return;
		}
		else
		{
			if (weaponType == EWeaponType.WT_ROCKETLAUNCHER)
			{
				// Rocket launcher, aim at weak point
				aimpointType0 = EAimPointType.WEAK;
				aimpointType1 = EAimPointType.NORMAL;
				return;
			}
			
			aimpointType0 = EAimPointType.NORMAL;
			aimpointType1 = EAimPointType.WEAK;
			return;
		}
	}
	
	// === ADDED: High-Value Target prioritization ===
	// Cek apakah target ini "bernilai tinggi" (bawa MG/AT/sniper) berdasarkan senjata
	// yang dia pegang saat ini. Dipakai buat bikin respon (burst-fire, suppressive fire
	// rate) lebih agresif -- ini BUKAN target re-selection (SCR_AIWeaponTargetSelector-nya
	// vanilla, gak bisa kita intervensi milih target). Ini cuma "begitu ngunci HVT, abis-
	// abisan", bukan "selalu tembak HVT duluan".
	//
	// Sengaja gak omniscient penuh: kalau target lagi gak visible DAN udah lama gak keliatan
	// (>5 detik), kita anggap "belum tau" loadout-nya, return false.
	protected static bool IsHighValueTarget(BaseTarget target, bool visible)
	{
		if (!target)
			return false;
		
		if (!visible && target.GetTimeSinceSeen() > 5.0)
			return false;
		
		IEntity targetEntity = target.GetTargetEntity();
		if (!targetEntity)
			return false;
		
		BaseWeaponManagerComponent wpnMgr = BaseWeaponManagerComponent.Cast(targetEntity.FindComponent(BaseWeaponManagerComponent));
		if (!wpnMgr)
			return false;
		
		array<BaseWeaponComponent> weapons = {};
		wpnMgr.GetWeapons(weapons);
		
		foreach (BaseWeaponComponent w : weapons)
		{
			if (!w)
				continue;
			
			EWeaponType wt = w.GetWeaponType();
			if (wt == EWeaponType.WT_MACHINEGUN || wt == EWeaponType.WT_ROCKETLAUNCHER || wt == EWeaponType.WT_SNIPERRIFLE)
				return true;
		}
		
		return false;
	}
	// === END ADDED ===
	
	// === ADDED: HVT urgency scaling -- "turunin urgency kalau ada threat lain" ===
	// Cek jarak threat TERDEKAT yang diketahui grup (dari semua target cluster,
	// bukan cuma target HVT yang lagi kita evaluasi). Kalau ada threat lain yang
	// jelas lebih deket dari target HVT ini, jangan boost fire rate/burst -- kasih
	// perlakuan normal aja (scale 1.0), biar AI gak "nempel" ke HVT jauh pas ada
	// yang lebih urgent di deket.
	protected float GetHVTUrgencyScale(float targetDistance)
	{
		if (!m_UtilityComponent)
			return HVT_FIRERATE_BOOST;
		
		AIAgent agent = m_UtilityComponent.GetAIAgent();
		if (!agent)
			return HVT_FIRERATE_BOOST;
		
		AIGroup grp = agent.GetParentGroup();
		if (!grp)
			return HVT_FIRERATE_BOOST;
		
		SCR_AIGroupUtilityComponent groupUtilComp = SCR_AIGroupUtilityComponent.Cast(grp.FindComponent(SCR_AIGroupUtilityComponent));
		if (!groupUtilComp)
			return HVT_FIRERATE_BOOST;
		
		SCR_AIGroupPerception groupPerc = groupUtilComp.GetPercGroupComp();
		if (!groupPerc)
			return HVT_FIRERATE_BOOST;
		
		float nearestKnownDist = float.MAX;
		foreach (SCR_AIGroupTargetCluster c : groupPerc.m_aTargetClusters)
		{
			if (!c || !c.m_State)
				continue;
			
			if (c.m_State.m_fDistMin < nearestKnownDist)
				nearestKnownDist = c.m_State.m_fDistMin;
		}
		
		if (nearestKnownDist == float.MAX)
			return HVT_FIRERATE_BOOST;
		
		if (nearestKnownDist < targetDistance - HVT_CLOSER_THREAT_MARGIN_M)
			return 1.0; // ada threat lain yang jauh lebih deket -- urgency HVT diturunin total
		
		return HVT_FIRERATE_BOOST;
	}
	// === END ADDED ===
	
	// === ADDED: Responsive close-range override ===
	//! Dipake buat nge-bypass "gak boleh aim pas combat-move" kalau AI lagi ditembak
	//! dari jarak deket. Cuma trigger kalau target visible (biar gak asal nembak ke
	//! arah kosong pas invisible).
	protected bool IsCloseDirectThreat(BaseTarget target, bool visible)
	{
		if (!target || !visible)
			return false;
		
		return target.GetDistance() < CLOSE_DIRECT_THREAT_DIST;
	}
	// === END ADDED ===
}