class DCO_PickupState
{
	float m_fNext_ms;
	float m_fNeedSince_ms = -1;
}

class DCO_WeaponPickup
{
	protected static ref map<IEntity, float> s_mBooked;
	protected static ref array<IEntity> s_aQuery;

	static bool IsLongGun(BaseWeaponComponent wc)
	{
		if (!wc)
			return false;
		EWeaponType t = wc.GetWeaponType();
		return t == EWeaponType.WT_RIFLE || t == EWeaponType.WT_MACHINEGUN || t == EWeaponType.WT_SNIPERRIFLE;
	}

	static typename WellOf(BaseWeaponComponent wc)
	{
		array<BaseMuzzleComponent> muzzles = {};
		wc.GetMuzzlesList(muzzles);
		if (muzzles.IsEmpty() || !muzzles[0] || !muzzles[0].GetMagazineWell())
			return typename.Empty;
		return muzzles[0].GetMagazineWell().Type();
	}

	static int Loaded(BaseWeaponComponent wc)
	{
		array<BaseMuzzleComponent> muzzles = {};
		wc.GetMuzzlesList(muzzles);
		if (muzzles.IsEmpty() || !muzzles[0])
			return 0;
		return muzzles[0].GetAmmoCount();
	}

	static bool ArmedState(IEntity me, SCR_AICombatComponent combat, out int ammo)
	{
		ammo = 0;
		bool hasLong;
		BaseWeaponManagerComponent wm = BaseWeaponManagerComponent.Cast(me.FindComponent(BaseWeaponManagerComponent));
		if (!wm)
			return false;
		array<IEntity> weapons = {};
		wm.GetWeaponsList(weapons);
		foreach (IEntity w : weapons)
		{
			BaseWeaponComponent wc = BaseWeaponComponent.Cast(w.FindComponent(BaseWeaponComponent));
			if (!IsLongGun(wc))
				continue;
			hasLong = true;
			ammo += Loaded(wc);
			typename well = WellOf(wc);
			if (combat && well != typename.Empty)
				ammo += combat.GetMagazineCount(well, false) * 10;
		}
		return hasLong;
	}

	static int MagsFor(IEntity holder, typename well)
	{
		if (!holder || well == typename.Empty)
			return 0;
		SCR_InventoryStorageManagerComponent inv = SCR_InventoryStorageManagerComponent.Cast(holder.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!inv)
			return 0;
		array<IEntity> items = {};
		array<typename> comps = {};
		comps.Insert(MagazineComponent);
		inv.FindItemsWithComponents(items, comps);
		int n;
		foreach (IEntity it : items)
		{
			MagazineComponent mag = MagazineComponent.Cast(it.FindComponent(MagazineComponent));
			if (mag && mag.GetMagazineWell() && mag.GetMagazineWell().Type() == well && mag.GetAmmoCount() > 0 && !(it.GetParent() && it.GetParent().FindComponent(BaseWeaponComponent) && !ChimeraCharacter.Cast(it.GetParent())))
				n++;
		}
		return n;
	}

	protected static bool CollectWeapon(IEntity e)
	{
		if (!e.GetParent() && !ChimeraCharacter.Cast(e) && IsLongGun(BaseWeaponComponent.Cast(e.FindComponent(BaseWeaponComponent))))
			s_aQuery.Insert(e);
		return true;
	}

	static bool Booked(IEntity w, float now)
	{
		if (!s_mBooked || !w)
			return false;
		float until;
		if (!s_mBooked.Find(w, until))
			return false;
		if (until < now)
		{
			s_mBooked.Remove(w);
			return false;
		}
		return true;
	}

	static void Book(IEntity w, float until)
	{
		if (!s_mBooked)
			s_mBooked = new map<IEntity, float>();
		s_mBooked.Set(w, until);
	}

	static void Unbook(IEntity w)
	{
		if (s_mBooked && w)
			s_mBooked.Remove(w);
	}

	static void Tick(SCR_AIUtilityComponent u, SCR_AICombatComponent combat, DCO_PickupState st, float now)
	{
		ChimeraCharacter me = ChimeraCharacter.Cast(u.m_OwnerEntity);
		if (!me || me.IsInVehicle() || !DCO_GlobalAIComponent.BehaviorOnFor(me, DCO_EBehavior.REARM))
			return;
		if (now < st.m_fNext_ms || u.FindActionOfType(DCO_AIPickupWeaponBehavior) || u.FindActionOfType(DCO_AIRearmBehavior))
			return;
		int ammo;
		bool hasLong = ArmedState(me, combat, ammo);
		if (hasLong && ammo > 0)
		{
			st.m_fNeedSince_ms = -1;
			return;
		}
		if (st.m_fNeedSince_ms < 0)
		{
			st.m_fNeedSince_ms = now;
			DCO_Perf.Count("pickup:debounce_wait");
			return;
		}
		if (now - st.m_fNeedSince_ms < 1000.0 * DCO_Stress.T(u, "pickup.debounce_s", 2.0))
			return;
		st.m_fNeedSince_ms = -1;
		st.m_fNext_ms = now + 6000.0;
		string reason = "no_weapon";
		if (hasLong)
			reason = "no_ammo";
		bool threatened = u.m_ThreatSystem && u.m_ThreatSystem.GetState() == EAIThreatState.THREATENED;
		float radius = 30.0;
		if (!hasLong)
			radius = 40.0;
		if (threatened)
			radius = 15.0;

		int pt = DCO_Perf.Begin();
		IEntity corpse;
		float d;
		int avail;
		string rej;
		IEntity weapon = Pick(me, radius, now, corpse, d, avail, rej);
		DCO_Perf.End("agent_pickup", pt);
		DCO_Perf.Count("pickup:need_" + reason);
		if (!weapon)
		{
			DCO_Perf.Count("pickup:no_source");
			if (DCO_Log.On(DCO_ELogChannel.UNIT, DCO_ELogLevel.DEBUG))
				DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(me), string.Format("pickup_no_source who=%1 reason=%2 radius=%3 rej=%4", DCO_Log.IdUnit(me), reason, radius, rej));
			return;
		}
		float prio = 96.0;
		if (d <= 15.0)
			prio = 120.0;
		if (hasLong)
			prio = prio - 6.0;
		Book(weapon, now + 70000.0);
		DCO_AIPickupWeaponBehavior b = new DCO_AIPickupWeaponBehavior(u, null, weapon, corpse, reason, prio, avail);
		u.AddAction(b);
	}

	protected static IEntity Pick(IEntity me, float radius, float now, out IEntity corpseOut, out float distOut, out int availOut, out string rej)
	{
		vector mp = me.GetOrigin();
		Faction mine;
		FactionAffiliationComponent mf = FactionAffiliationComponent.Cast(me.FindComponent(FactionAffiliationComponent));
		if (mf)
			mine = mf.GetAffiliatedFaction();
		bool enemyOk = DCO_GlobalAIComponent.BehaviorOnFor(me, DCO_EBehavior.ENEMY_WEAPONS);
		IEntity best;
		float bestScore = float.MAX;
		int rBooked;
		int rEmpty;
		int rFaction;

		if (!s_aQuery)
			s_aQuery = {};
		s_aQuery.Clear();
		DCO_Perf.Count("q:DCO_WeaponPickup");
		GetGame().GetWorld().QueryEntitiesBySphere(mp, radius, CollectWeapon, null, EQueryEntitiesFlags.DYNAMIC);
		foreach (IEntity g : s_aQuery)
		{
			if (Booked(g, now))
			{
				rBooked++;
				continue;
			}
			BaseWeaponComponent gwc = BaseWeaponComponent.Cast(g.FindComponent(BaseWeaponComponent));
			int gAmmo = Loaded(gwc);
			if (gAmmo <= 0)
			{
				rEmpty++;
				continue;
			}
			float gd = vector.DistanceXZ(mp, g.GetOrigin());
			float gs = gd - 1.0 * gAmmo * 0.05;
			if (gs < bestScore)
			{
				bestScore = gs;
				best = g;
				corpseOut = null;
				distOut = gd;
				availOut = gAmmo;
			}
		}
		s_aQuery.Clear();

		array<IEntity> corpses = DCO_Rearm.Corpses();
		if (corpses)
		{
			foreach (IEntity c : corpses)
			{
				if (!c)
					continue;
				float cd = vector.DistanceXZ(mp, c.GetOrigin());
				if (cd > radius)
					continue;
				FactionAffiliationComponent cf = FactionAffiliationComponent.Cast(c.FindComponent(FactionAffiliationComponent));
				if (!enemyOk && mine && cf && cf.GetAffiliatedFaction() && !mine.IsFactionFriendly(cf.GetAffiliatedFaction()))
				{
					rFaction++;
					continue;
				}
				BaseWeaponManagerComponent cwm = BaseWeaponManagerComponent.Cast(c.FindComponent(BaseWeaponManagerComponent));
				if (!cwm)
					continue;
				array<IEntity> cw = {};
				cwm.GetWeaponsList(cw);
				foreach (IEntity w : cw)
				{
					BaseWeaponComponent wc = BaseWeaponComponent.Cast(w.FindComponent(BaseWeaponComponent));
					if (!IsLongGun(wc))
						continue;
					if (Booked(w, now))
					{
						rBooked++;
						continue;
					}
					int a = Loaded(wc) + MagsFor(c, WellOf(wc)) * 20;
					if (a <= 0)
					{
						rEmpty++;
						continue;
					}
					float s = cd - 1.0 * a * 0.05;
					if (s < bestScore)
					{
						bestScore = s;
						best = w;
						corpseOut = c;
						distOut = cd;
						availOut = a;
					}
				}
			}
		}
		rej = string.Format("booked:%1|empty:%2|faction:%3", rBooked, rEmpty, rFaction);
		return best;
	}

	static void Verify(IEntity me, IEntity weapon, string who, string reason, float start_ms, int mags)
	{
		if (DCO_Util.Teardown("pickup_verify") || !me)
			return;
		bool isOwned;
		if (weapon && weapon.GetRootParent() == me)
			isOwned = true;
		bool inHand;
		ChimeraCharacter ch = ChimeraCharacter.Cast(me);
		if (isOwned && ch && ch.GetCharacterController())
		{
			BaseWeaponComponent wc = BaseWeaponComponent.Cast(weapon.FindComponent(BaseWeaponComponent));
			BaseWeaponManagerComponent wm = ch.GetCharacterController().GetWeaponManagerComponent();
			if (wc && wm && wm.GetCurrentWeapon() != wc)
				ch.GetCharacterController().SelectWeapon(wc);
			if (wm && wm.GetCurrentWeapon() == wc)
				inHand = true;
		}
		if (isOwned)
			DCO_Perf.Count("pickup:ok");
		else
			DCO_Perf.Count("pickup:equip_fail");
		Unbook(weapon);
		DCO_ELogLevel lvl = DCO_ELogLevel.INFO;
		if (!isOwned)
			lvl = DCO_ELogLevel.WARN;
		DCO_Log.Write(DCO_ELogChannel.UNIT, lvl, who, string.Format("pickup_equip who=%1 reason=%2 weapon=%3 isOwned=%4 in_hand=%5 mags=%6 ms=%7",
			who, reason, DCO_Util.PrefabShort(weapon), DCO_Util.B01(isOwned), DCO_Util.B01(inHand), mags, Math.Round(GetGame().GetWorld().GetWorldTime() - start_ms)));
	}
}

class DCO_AIPickupWeaponBehavior : DCO_TakeBehavior
{
	protected IEntity m_DCOWeapon;
	protected IEntity m_DCOCorpse;
	protected string m_sDCOReason;
	protected float m_fDCOPrio;
	protected float m_fDCOStart_ms;
	protected int m_iDCORemoves;
	protected int m_iDCOMags;
	protected bool m_bDCOLogged;
	protected string m_sDCOResult = "";
	protected ref DCO_RearmMoveBehavior m_DCOMove;

	void DCO_AIPickupWeaponBehavior(SCR_AIUtilityComponent utility, SCR_AIActivityBase groupActivity, IEntity weapon, IEntity corpse, string reason, float prio, int avail)
	{
		m_sBehaviorTree = "AI/BehaviorTrees/Chimera/Soldier/DCO_Rearm.bt";
		SetPriority(prio);
		SetIsUniqueInActionQueue(true);
		m_DCOWeapon = weapon;
		m_DCOCorpse = corpse;
		m_sDCOReason = reason;
		m_fDCOPrio = prio;
		m_fDCOStart_ms = GetGame().GetWorld().GetWorldTime();
		if (!utility || !weapon)
			return;
		DCO_StartMove(utility);
		DCO_Perf.Count("pickup:start");
		IEntity me = utility.m_OwnerEntity;
		string src = "ground";
		if (corpse)
			src = "corpse";
		DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("pickup_start who=%1 reason=%2 weapon=%3 src=%4 dist=%5 ammo=%6 prio=%7",
			DCO_Log.IdUnit(me), reason, DCO_Util.PrefabShort(weapon), src, Math.Round(vector.DistanceXZ(utility.GetOrigin(), weapon.GetOrigin())), avail, prio));
	}

	protected void DCO_StartMove(SCR_AIUtilityComponent utility)
	{
		m_DCOMove = new DCO_RearmMoveBehavior(utility, null, m_DCOWeapon.GetOrigin(), m_fDCOPrio + 0.1, PRIORITY_LEVEL_NORMAL, null, 1.0);
		m_DCOMove.DCO_SetParent(this);
		utility.AddAction(m_DCOMove);
	}

	override float DCO_CurrentPrio()
	{
		return m_fDCOPrio;
	}

	override int GetCause()
	{
		return SCR_EAIBehaviorCause.SELF_AID;
	}

	protected void DCO_Finish(string result, bool ok)
	{
		if (m_sDCOResult.IsEmpty())
			m_sDCOResult = result;
		if (ok)
			Complete();
		else
			Fail();
	}

	override float CustomEvaluate()
	{
		EAIActionState st = GetActionState();
		if (st == EAIActionState.COMPLETED || st == EAIActionState.FAILED)
			return 0;
		IEntity me = m_Utility.m_OwnerEntity;
		if (!me || !m_DCOWeapon)
		{
			DCO_Finish("weapon_gone", false);
			return 0;
		}
		IEntity root = m_DCOWeapon.GetRootParent();
		if (root != m_DCOWeapon && root != m_DCOCorpse)
		{
			DCO_Finish("taken_by_other", false);
			return 0;
		}
		float now = GetGame().GetWorld().GetWorldTime();
		if (now - m_fDCOStart_ms > 60000.0)
		{
			DCO_Finish("timeout", false);
			return 0;
		}
		int ammo;
		if (DCO_WeaponPickup.ArmedState(me, m_Utility.m_CombatComponent, ammo) && ammo > 0)
		{
			DCO_Finish("armed_already", false);
			return 0;
		}
		float d = vector.DistanceXZ(me.GetOrigin(), m_DCOWeapon.GetOrigin());
		if (m_Utility.m_ThreatSystem && m_Utility.m_ThreatSystem.GetState() == EAIThreatState.THREATENED && d > 15.0)
			return 0;
		if (d > 3.0 && (!m_DCOMove || m_DCOMove.GetActionState() == EAIActionState.COMPLETED || m_DCOMove.GetActionState() == EAIActionState.FAILED))
		{
			if (m_iDCORemoves >= 3)
			{
				DCO_Finish("unreachable", false);
				return 0;
			}
			m_iDCORemoves++;
			DCO_StartMove(m_Utility);
			DCO_Perf.Count("pickup:remove");
			return 0;
		}
		return m_fDCOPrio;
	}

	override int DCO_DoTake()
	{
		IEntity me = m_Utility.m_OwnerEntity;
		if (!me || !m_DCOWeapon)
		{
			m_sDCOResult = "weapon_gone";
			return 0;
		}
		if (vector.DistanceXZ(me.GetOrigin(), m_DCOWeapon.GetOrigin()) > 3.5)
		{
			m_sDCOResult = "unreachable";
			return 0;
		}
		SCR_InventoryStorageManagerComponent inv = SCR_InventoryStorageManagerComponent.Cast(me.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!inv)
		{
			m_sDCOResult = "no_inventory";
			return 0;
		}
		BaseWeaponComponent wc = BaseWeaponComponent.Cast(m_DCOWeapon.FindComponent(BaseWeaponComponent));
		typename well = DCO_WeaponPickup.WellOf(wc);
		if (!inv.EquipWeapon(m_DCOWeapon, null, true))
		{
			m_sDCOResult = "equip_refused";
			return 0;
		}
		m_iDCOMags = DCO_TakeMags(inv, me, well);
		m_sDCOResult = "taken";
		GetGame().GetCallqueue().CallLater(DCO_WeaponPickup.Verify, 1500, false, me, m_DCOWeapon, DCO_Log.IdUnit(me), m_sDCOReason, m_fDCOStart_ms, m_iDCOMags);
		return 1;
	}

	protected int DCO_TakeMags(SCR_InventoryStorageManagerComponent myInv, IEntity me, typename well)
	{
		if (well == typename.Empty)
			return 0;
		array<IEntity> items = {};
		if (m_DCOCorpse)
		{
			SCR_InventoryStorageManagerComponent src = SCR_InventoryStorageManagerComponent.Cast(m_DCOCorpse.FindComponent(SCR_InventoryStorageManagerComponent));
			if (src)
			{
				array<typename> comps = {};
				comps.Insert(MagazineComponent);
				src.FindItemsWithComponents(items, comps);
			}
		}
		else
		{
			GetMagQuery().Clear();
			GetGame().GetWorld().QueryEntitiesBySphere(m_DCOWeapon.GetOrigin(), 4.0, DCO_CollectMag, null, EQueryEntitiesFlags.DYNAMIC);
			items.Copy(GetMagQuery());
			GetMagQuery().Clear();
		}
		int taken;
		foreach (IEntity it : items)
		{
			if (taken >= 4)
				break;
			MagazineComponent mag = MagazineComponent.Cast(it.FindComponent(MagazineComponent));
			if (!mag || !mag.GetMagazineWell() || mag.GetMagazineWell().Type() != well || mag.GetAmmoCount() <= 0)
				continue;
			IEntity p = it.GetParent();
			if (p && p.FindComponent(BaseWeaponComponent) && !ChimeraCharacter.Cast(p))
				continue;
			BaseInventoryStorageComponent storage = myInv.FindStorageForItem(it, EStoragePurpose.PURPOSE_DEPOSIT);
			if ((storage && myInv.TryMoveItemToStorage(it, storage)) || myInv.TryInsertItem(it, EStoragePurpose.PURPOSE_DEPOSIT))
				taken++;
			else
				DCO_Perf.Count("pickup:mag_move_failed");
		}
		return taken;
	}

	protected static ref array<IEntity> s_aMagQuery;
	protected static array<IEntity> GetMagQuery()
	{
		if (!s_aMagQuery)
			s_aMagQuery = {};
		return s_aMagQuery;
	}

	protected static bool DCO_CollectMag(IEntity e)
	{
		if (!e.GetParent() && e.FindComponent(MagazineComponent))
			GetMagQuery().Insert(e);
		return true;
	}

	protected void DCO_LogEnd(string fallback)
	{
		if (m_bDCOLogged)
			return;
		m_bDCOLogged = true;
		string result = m_sDCOResult;
		if (result.IsEmpty())
			result = fallback;
		if (result == "equip_refused" || result == "unreachable")
		{
			float cool_s = 120.0;
			if (m_Utility)
				cool_s = DCO_Stress.T(m_Utility, "pickup.fail_cooldown_s", 120.0);
			DCO_WeaponPickup.Book(m_DCOWeapon, GetGame().GetWorld().GetWorldTime() + 1000.0 * cool_s);
			DCO_Perf.Count("pickup:fail_" + result);
		}
		else if (result != "taken")
		{
			DCO_WeaponPickup.Unbook(m_DCOWeapon);
			DCO_Perf.Count("pickup:fail_" + result);
		}
		IEntity me;
		if (m_Utility)
			me = m_Utility.m_OwnerEntity;
		DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("pickup_end who=%1 result=%2 weapon=%3 mags=%4 removes=%5 ms=%6",
			DCO_Log.IdUnit(me), result, DCO_Util.PrefabShort(m_DCOWeapon), m_iDCOMags, m_iDCORemoves, Math.Round(GetGame().GetWorld().GetWorldTime() - m_fDCOStart_ms)));
	}

	override void OnActionCompleted()
	{
		super.OnActionCompleted();
		if (DCO_Util.Teardown("pickup_completed"))
			return;
		DCO_LogEnd("taken");
		if (m_DCOMove)
			m_DCOMove.Complete();
	}

	override void OnActionFailed()
	{
		super.OnActionFailed();
		if (DCO_Util.Teardown("pickup_failed"))
			return;
		DCO_LogEnd("cancel");
		if (m_DCOMove)
			m_DCOMove.Fail();
	}

	override string GetActionDebugInfo()
	{
		return string.Format("DCO pickup weapon=%1 reason=%2 prio=%3", DCO_Util.PrefabShort(m_DCOWeapon), m_sDCOReason, m_fDCOPrio);
	}
}
