class DCO_RecoveryJob
{
	IEntity m_Body;
	AIGroup m_Group;
	EWeaponType m_eType;
	SCR_ChimeraAIAgent m_Agent;
	IEntity m_Weapon;
	bool m_bSingleUse;
	float m_fAt;
	float m_fAssignedAt;
	float m_fEquipAt;
	float m_fNextPick;
	IEntity m_DeadWeapon;
	string m_sEnd = "other";
	bool m_bNoCarrierLogged;
}

class DCO_Recovery
{
	protected static ref array<ref DCO_RecoveryJob> s_aJobs;
	protected static int s_iEquipOk;
	protected static int s_iEquipFail;
	protected static ref array<IEntity> s_aFound;
	protected static int s_iCursor;

	static void OnDeath(SCR_ChimeraAIAgent dead)
	{
		if (!dead || !Replication.IsServer() || !DCO_GlobalAIComponent.BehaviorOnFor(dead, DCO_EBehavior.SPECIALIST_RECOVERY))
			return;
		IEntity body = dead.GetControlledEntity();
		AIGroup grp = dead.GetParentGroup();
		if (!body || !grp || !grp.GetLeaderEntity() || SCR_CharacterHelper.IsPlayer(grp.GetLeaderEntity()))
			return;

		int roles;
		if (dead.m_UtilityComponent && dead.m_UtilityComponent.m_fDCORolesAt_ms > 0)
			roles = dead.m_UtilityComponent.m_iDCORoles;
		else
			roles = DCO_Roles.Compute(body, null);
		EWeaponType type;
		bool singleUse = false;
		if (roles & 1)
			type = EWeaponType.WT_MACHINEGUN;
		else if (roles & 2)
			type = EWeaponType.WT_ROCKETLAUNCHER;
		else if (roles & 8)
			type = EWeaponType.WT_GRENADELAUNCHER;
		else if (HasLoadedDisposable(body))
		{
			type = EWeaponType.WT_ROCKETLAUNCHER;
			singleUse = true;
		}
		else
			return;

		DCO_RecoveryJob j = new DCO_RecoveryJob();
		j.m_Body = body;
		j.m_Group = grp;
		j.m_eType = type;
		j.m_bSingleUse = singleUse;
		j.m_fAt = DCO_Util.NowS();
		if (!s_aJobs)
		{
			s_aJobs = {};
			GetGame().GetCallqueue().CallLater(Tick, 1000, true);
		}
		if (s_aJobs.Count() >= 32)
			s_aJobs.Remove(0);
		s_aJobs.Insert(j);
		DCO_Perf.Count("recover:job_" + typename.EnumToString(EWeaponType, type));
		DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, "-", string.Format("recover_job grp=%1 type=%2 single=%3 roles=%4 jobs=%5", DCO_Log.IdGroup(grp), typename.EnumToString(EWeaponType, type), DCO_Util.B01(singleUse), roles, s_aJobs.Count()));
	}

	protected static void Tick()
	{
		int pt = DCO_Perf.Begin();
		float now = DCO_Util.NowS();
		int budget = 8;
		int done = 0;
		while (done < budget && !s_aJobs.IsEmpty() && done < s_aJobs.Count())
		{
			if (s_iCursor >= s_aJobs.Count())
				s_iCursor = 0;
			DCO_RecoveryJob sj = s_aJobs[s_iCursor];
			if (!Step(sj, now))
			{
				DCO_Perf.Count("recover:end_" + sj.m_sEnd);
				DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, "-", string.Format("recover_end grp=%1 type=%2 why=%3 age_s=%4 carrier=%5", DCO_Log.IdGroup(sj.m_Group), typename.EnumToString(EWeaponType, sj.m_eType), sj.m_sEnd, Math.Round(now - sj.m_fAt), DCO_Util.B01(sj.m_Agent != null)));
				s_aJobs.Remove(s_iCursor);
			}
			else
				s_iCursor++;
			done++;
		}
		if (s_aJobs.Count() > budget)
			DCO_Perf.Count("recover:slice_cut");
		DCO_Perf.End("agent_recovery", pt);
	}

	static bool EquipBroken()
	{
		return s_iEquipOk == 0 && s_iEquipFail >= 3;
	}

	protected static bool Step(DCO_RecoveryJob j, float now)
	{
		if (!j.m_Body || !j.m_Group)
		{
			j.m_sEnd = "gone";
			return false;
		}
		if (now - j.m_fAt > 300.0)
		{
			j.m_sEnd = "expired";
			return false;
		}
		if (!DCO_GlobalAIComponent.BehaviorOnFor(j.m_Group, DCO_EBehavior.SPECIALIST_RECOVERY))
		{
			j.m_sEnd = "off";
			return false;
		}

		if (j.m_fEquipAt > 0)
		{
			if (now - j.m_fEquipAt < 3.0)
				return true;
			IEntity me = null;
			if (j.m_Agent)
				me = j.m_Agent.GetControlledEntity();
			bool ok = me && j.m_Weapon && j.m_Weapon.GetRootParent() == me;
			if (ok)
			{
				s_iEquipOk++;
				DCO_Perf.Count("recover:equip_ok");
				if (j.m_Agent.m_UtilityComponent)
					j.m_Agent.m_UtilityComponent.m_fDCORolesAt_ms = 0;
			}
			else
			{
				s_iEquipFail++;
				DCO_Perf.Count("recover:equip_fail");
			}
			DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, "-", string.Format("recover_equip grp=%1 type=%2 ok=%3 total_ok=%4 total_fail=%5", DCO_Log.IdGroup(j.m_Group), typename.EnumToString(EWeaponType, j.m_eType), ok, s_iEquipOk, s_iEquipFail));
			return false;
		}

		array<AIAgent> agents = {};
		j.m_Group.GetAgents(agents);
		int alive = 0;
		foreach (AIAgent a : agents)
		{
			if (a && DCO_Util.IsAlive(a.GetControlledEntity()))
				alive++;
		}
		if (alive <= 2)
		{
			j.m_sEnd = "few_alive";
			return false;
		}

		if (!j.m_Agent || !DCO_Util.IsAlive(j.m_Agent.GetControlledEntity()))
		{
			if (now < j.m_fNextPick)
				return true;
			j.m_Agent = PickCarrier(j, agents);
			j.m_fAssignedAt = now;
			if (!j.m_Agent)
			{
				j.m_fNextPick = now + 5.0;
				DCO_Perf.Count("recover:no_carrier");
				if (!j.m_bNoCarrierLogged)
				{
					j.m_bNoCarrierLogged = true;
					string mix;
					foreach (AIAgent ma : agents)
					{
						SCR_ChimeraAIAgent mc = SCR_ChimeraAIAgent.Cast(ma);
						if (mc && mc.m_UtilityComponent && DCO_Util.IsAlive(mc.GetControlledEntity()))
							mix += DCO_Roles.Get(mc.m_UtilityComponent).ToString() + ",";
					}
					DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, "-", string.Format("recover_no_carrier grp=%1 type=%2 alive=%3 roles_alive=%4 dead_weapon=%5", DCO_Log.IdGroup(j.m_Group), typename.EnumToString(EWeaponType, j.m_eType), alive, mix, DCO_Util.B01(j.m_DeadWeapon != null)));
				}
				if (now - j.m_fAt >= 60.0)
					j.m_sEnd = "no_carrier";
				return now - j.m_fAt < 60.0;
			}
		}

		SCR_AIUtilityComponent u = j.m_Agent.m_UtilityComponent;
		ChimeraCharacter me = ChimeraCharacter.Cast(j.m_Agent.GetControlledEntity());
		if (!u || !me || me.IsInVehicle())
			return true;

		float d = vector.DistanceXZ(me.GetOrigin(), j.m_Body.GetOrigin());
		bool threatened = u.m_ThreatSystem && u.m_ThreatSystem.GetState() == EAIThreatState.THREATENED;
		if (threatened && d > 15.0 && !Critical(j))
			return true;
		if (now - j.m_fAssignedAt > 120.0)
		{
			j.m_Agent = null;
			return true;
		}

		if (d > 2.5)
		{
			if (threatened)
				DCO_RelocateBehavior.Start(u, j.m_Body.GetOrigin(), j.m_Body.GetOrigin(), false, ECharacterStance.CROUCH, 10.0, "recovery");
			else
				DCO_Specialist.MoveIndividually(u, j.m_Body.GetOrigin());
			return true;
		}

		SCR_InventoryStorageManagerComponent inv = SCR_InventoryStorageManagerComponent.Cast(me.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!inv)
			return false;

		int mags = TakeAmmo(inv, j.m_Body, j.m_eType);
		DCO_Perf.Count("recover:ammo");
		if (EquipBroken())
		{
			DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, "-", string.Format("recover_ammo_only grp=%1 type=%2 mags=%3", j.m_Group, typename.EnumToString(EWeaponType, j.m_eType), mags));
			return false;
		}

		j.m_Weapon = FindWeapon(j.m_Body, j.m_eType);
		if (!j.m_Weapon)
		{
			DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, "-", string.Format("recover_no_weapon grp=%1 type=%2 mags=%3", DCO_Log.IdGroup(j.m_Group), typename.EnumToString(EWeaponType, j.m_eType), mags));
			return false;
		}
		bool started = inv.EquipWeapon(j.m_Weapon, null, true);
		j.m_fEquipAt = now;
		if (!started)
			j.m_fEquipAt = now - 3.0;
		return true;
	}

	protected static bool Critical(DCO_RecoveryJob j)
	{
		DCO_GroupUtilityComponent g = DCO_GroupUtilityComponent.Cast(j.m_Group.FindComponent(DCO_GroupUtilityComponent));
		if (!g)
			return false;
		if (j.m_eType == EWeaponType.WT_ROCKETLAUNCHER)
			return !g.HasAT() && g.GetGroupUtilityComponent() && (g.GetGroupUtilityComponent().m_iDCOSharedComp & 16) != 0;
		if (j.m_eType == EWeaponType.WT_MACHINEGUN)
			return !g.HasMG() && g.GetTask() == DCO_EGroupTask.SUPPORT_BY_FIRE;
		return false;
	}

	protected static bool HasLoadedDisposable(IEntity body)
	{
		BaseWeaponManagerComponent wm = BaseWeaponManagerComponent.Cast(body.FindComponent(BaseWeaponManagerComponent));
		if (!wm)
			return false;
		array<IEntity> weapons = {};
		wm.GetWeaponsList(weapons);
		foreach (IEntity w : weapons)
		{
			BaseWeaponComponent wc = BaseWeaponComponent.Cast(w.FindComponent(BaseWeaponComponent));
			if (!wc || wc.GetWeaponType() != EWeaponType.WT_ROCKETLAUNCHER)
				continue;
			array<BaseMuzzleComponent> muzzles = {};
			wc.GetMuzzlesList(muzzles);
			if (!muzzles.IsEmpty() && muzzles[0] && muzzles[0].IsDisposable() && muzzles[0].GetAmmoCount() > 0)
				return true;
		}
		return false;
	}

	protected static SCR_ChimeraAIAgent PickCarrier(DCO_RecoveryJob j, array<AIAgent> agents)
	{
		vector pos = j.m_Body.GetOrigin();
		if (j.m_bSingleUse)
		{
			SCR_ChimeraAIAgent anyone;
			float anyD = 150.0 * 150.0;
			foreach (AIAgent a0 : agents)
			{
				SCR_ChimeraAIAgent c0 = SCR_ChimeraAIAgent.Cast(a0);
				if (!c0 || !c0.m_UtilityComponent || !DCO_Util.IsAlive(c0.GetControlledEntity()))
					continue;
				if (DCO_Roles.Get(c0.m_UtilityComponent) & 2)
					continue;
				float d0 = vector.DistanceSqXZ(c0.GetControlledEntity().GetOrigin(), pos);
				if (d0 < anyD)
				{
					anyD = d0;
					anyone = c0;
				}
			}
			if (anyone)
				DCO_Perf.Count("recover:single_use");
			return anyone;
		}
		BaseWeaponComponent deadWc;
		if (!j.m_DeadWeapon || (j.m_DeadWeapon.GetRootParent() != j.m_Body && j.m_DeadWeapon.GetParent()))
			j.m_DeadWeapon = FindWeapon(j.m_Body, j.m_eType);
		IEntity deadWeapon = j.m_DeadWeapon;
		if (deadWeapon)
			deadWc = BaseWeaponComponent.Cast(deadWeapon.FindComponent(BaseWeaponComponent));
		SCR_ChimeraAIAgent assistant;
		float asD = 150.0 * 150.0;
		if (deadWc)
		{
			foreach (AIAgent a1 : agents)
			{
				SCR_ChimeraAIAgent c1 = SCR_ChimeraAIAgent.Cast(a1);
				if (!c1 || !c1.m_UtilityComponent || !DCO_Util.IsAlive(c1.GetControlledEntity()) || DCO_Util.IsRealMedic(c1.GetControlledEntity()))
					continue;
				if (DCO_Roles.Get(c1.m_UtilityComponent) & (1 | 2 | 8))
					continue;
				SCR_InventoryStorageManagerComponent inv = SCR_InventoryStorageManagerComponent.Cast(c1.GetControlledEntity().FindComponent(SCR_InventoryStorageManagerComponent));
				if (!inv || inv.GetMagazineCountByWeapon(deadWc) <= 0)
					continue;
				float d1 = vector.DistanceSqXZ(c1.GetControlledEntity().GetOrigin(), pos);
				if (d1 < asD)
				{
					asD = d1;
					assistant = c1;
				}
			}
		}
		if (assistant)
		{
			DCO_Perf.Count("recover:assistant");
			return assistant;
		}
		SCR_ChimeraAIAgent rifleman = PickRifleman(agents, pos);
		if (rifleman)
			DCO_Perf.Count("recover:rifleman");
		return rifleman;
	}

	protected static SCR_ChimeraAIAgent PickRifleman(array<AIAgent> agents, vector pos)
	{
		SCR_ChimeraAIAgent best;
		float bestD = 150.0 * 150.0;
		foreach (AIAgent a : agents)
		{
			SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
			if (!ca || !ca.m_UtilityComponent)
				continue;
			IEntity e = ca.GetControlledEntity();
			if (!DCO_Util.IsAlive(e) || DCO_Util.IsRealMedic(e))
				continue;
			if (DCO_Roles.Get(ca.m_UtilityComponent) & (1 | 2 | 4 | 8 | 32))
				continue;
			float dSq = vector.DistanceSqXZ(e.GetOrigin(), pos);
			if (dSq < bestD)
			{
				bestD = dSq;
				best = ca;
			}
		}
		return best;
	}

	protected static IEntity FindWeapon(IEntity body, EWeaponType type)
	{
		BaseWeaponManagerComponent wm = BaseWeaponManagerComponent.Cast(body.FindComponent(BaseWeaponManagerComponent));
		if (wm)
		{
			array<IEntity> weapons = {};
			wm.GetWeaponsList(weapons);
			foreach (IEntity w : weapons)
			{
				if (IsType(w, type))
					return w;
			}
		}

		if (!s_aFound)
			s_aFound = {};
		s_aFound.Clear();
		GetGame().GetWorld().QueryEntitiesBySphere(body.GetOrigin(), 4.0, CollectWeapon);
		foreach (IEntity f : s_aFound)
		{
			if (IsType(f, type) && !f.GetParent())
				return f;
		}
		return null;
	}

	protected static bool CollectWeapon(IEntity e)
	{
		if (e.FindComponent(BaseWeaponComponent))
			s_aFound.Insert(e);
		return true;
	}

	protected static bool IsType(IEntity w, EWeaponType type)
	{
		BaseWeaponComponent wc = BaseWeaponComponent.Cast(w.FindComponent(BaseWeaponComponent));
		if (!wc)
			return false;
		if (type == EWeaponType.WT_GRENADELAUNCHER)
			return DCO_UGLUtility.HasUGL(wc) || wc.GetWeaponType() == EWeaponType.WT_GRENADELAUNCHER;
		return wc.GetWeaponType() == type;
	}

	protected static int TakeAmmo(SCR_InventoryStorageManagerComponent myInv, IEntity body, EWeaponType type)
	{
		SCR_InventoryStorageManagerComponent src = SCR_InventoryStorageManagerComponent.Cast(body.FindComponent(SCR_InventoryStorageManagerComponent));
		IEntity weapon = FindWeapon(body, type);
		if (!src || !weapon)
			return 0;
		BaseWeaponComponent wc = BaseWeaponComponent.Cast(weapon.FindComponent(BaseWeaponComponent));
		array<BaseMuzzleComponent> muzzles = {};
		wc.GetMuzzlesList(muzzles);
		array<typename> wells = {};
		foreach (BaseMuzzleComponent m : muzzles)
		{
			if (m && m.GetMagazineWell())
				wells.Insert(m.GetMagazineWell().Type());
		}

		array<IEntity> items = {};
		array<typename> comps = {};
		comps.Insert(MagazineComponent);
		src.FindItemsWithComponents(items, comps);
		int taken = 0;
		foreach (IEntity it : items)
		{
			MagazineComponent mag = MagazineComponent.Cast(it.FindComponent(MagazineComponent));
			if (!mag || !mag.GetMagazineWell() || !wells.Contains(mag.GetMagazineWell().Type()))
				continue;
			myInv.TrySpawnPrefabToStorage(it.GetPrefabData().GetPrefab().GetResourceName());
			src.TryDeleteItem(it);
			taken++;
			if (taken >= 4)
				break;
		}
		return taken;
	}
}
