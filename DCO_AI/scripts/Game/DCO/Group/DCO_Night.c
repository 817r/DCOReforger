class DCO_Night
{

	protected static ref array<vector> s_aFlarePos;
	protected static array<vector> GetFlarePos()
	{
		if (!s_aFlarePos)
			s_aFlarePos = {};
		return s_aFlarePos;
	}
	protected static ref array<float> s_aFlareTime;
	protected static array<float> GetFlareTime()
	{
		if (!s_aFlareTime)
			s_aFlareTime = {};
		return s_aFlareTime;
	}
	protected static ref array<IEntity> s_aItems;
	protected static array<IEntity> GetItemsStore()
	{
		if (!s_aItems)
			s_aItems = {};
		return s_aItems;
	}

	static bool IsActive()
	{
		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetInstance();
		return cfg && cfg.GetNightEnabled() && DCO_Util.IsNight();
	}

	static float EngageMultiplier()
	{
		if (!IsActive())
			return 1.0;
		return DCO_GlobalAIComponent.GetInstance().GetNightEngageMul();
	}

	static bool FlareNear(vector pos, float now_ms, float radius = 150.0)
	{
		float radiusSq = radius * radius;
		for (int i = GetFlarePos().Count() - 1; i >= 0; i--)
		{
			if (now_ms - GetFlareTime()[i] > 40000.0 || now_ms < GetFlareTime()[i])
			{
				GetFlarePos().Remove(i);
				GetFlareTime().Remove(i);
				continue;
			}
			if (vector.DistanceSqXZ(GetFlarePos()[i], pos) <= radiusSq)
				return true;
		}
		return false;
	}

	static bool IsFlareItem(IEntity item)
	{
		if (!item || !item.GetPrefabData())
			return false;

		string prefab = item.GetPrefabData().GetPrefabName();
		if (!prefab.Contains("Flare"))
			return false;
		return prefab.Contains("Ammo_Flare_40mm") || prefab.Contains("Flare_RSP30");
	}

	static bool FindFlare(notnull SCR_AIGroup grp, out AIAgent outAgent, out IEntity outItem)
	{
		array<AIAgent> agents = {};
		grp.GetAgents(agents);
		foreach (AIAgent a : agents)
		{
			ChimeraCharacter ch = ChimeraCharacter.Cast(a.GetControlledEntity());
			if (!ch || ch.IsInVehicle())
				continue;
			if (!DCO_Util.IsAlive(ch))
				continue;

			SCR_InventoryStorageManagerComponent inv = SCR_InventoryStorageManagerComponent.Cast(ch.FindComponent(SCR_InventoryStorageManagerComponent));
			if (!inv)
				continue;

			GetItemsStore().Clear();
			GetItemsStore();
			inv.GetItems(s_aItems);
			foreach (IEntity item : GetItemsStore())
			{
				if (IsFlareItem(item))
				{
					outAgent = a;
					outItem = item;
					GetItemsStore().Clear();
					return true;
				}
			}
		}
		GetItemsStore().Clear();
		return false;
	}

	static bool FireFlare(AIAgent shooter, IEntity item, vector clusterPos, float now_ms)
	{
		IEntity ent = shooter.GetControlledEntity();
		if (!ent)
			return false;

		SCR_InventoryStorageManagerComponent inv = SCR_InventoryStorageManagerComponent.Cast(ent.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!inv)
			return false;

		vector dir = clusterPos - ent.GetOrigin();
		dir[1] = 0;
		float dist = dir.Length();
		if (dist > 1)
			dir = dir * (1.0 / dist);

		vector burst = clusterPos + dir * 20.0;
		burst[1] = GetGame().GetWorld().GetSurfaceY(burst[0], burst[2]) + 120.0;

		Resource res = Resource.Load("{A090D9A11955DF54}Prefabs/Weapons/Ammo/FlareEffect_30mm_RSP30_White.et");
		if (!res || !res.IsValid())
			return false;

		string type = "rsp30";
		if (item.GetPrefabData() && item.GetPrefabData().GetPrefabName().Contains("40mm"))
			type = "40mm";
		inv.TryDeleteItem(item);

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		Math3D.MatrixIdentity4(params.Transform);
		params.Transform[3] = burst;
		DCO_Perf.Count("sp:flare");
		GetGame().SpawnEntityPrefab(res, GetGame().GetWorld(), params);

		GetFlarePos().Insert(clusterPos);
		GetFlareTime().Insert(now_ms);

		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("night_flare grp=%1 shooter=%2 type=%3 dist=%4",
			shooter.GetParentGroup(), ent, type, Math.Round(dist)));
		return true;
	}

	static bool IsDark(SCR_AIGroupTargetCluster c)
	{
		int n = 0;
		foreach (SCR_AITargetInfo t : c.m_aTargets)
		{
			if (!t || !t.m_Entity)
				continue;
			if (t.m_eCategory != EAITargetInfoCategory.DETECTED && t.m_eCategory != EAITargetInfoCategory.IDENTIFIED)
				continue;

			PerceivableComponent perc = PerceivableComponent.Cast(t.m_Entity.FindComponent(PerceivableComponent));
			if (perc && perc.GetIlluminationFactor() >= 0.2)
				return false;
			n++;
		}
		return n > 0;
	}

	static void ApplyLights(notnull SCR_AIGroup grp, bool stealth, DCO_CQBClear cqb)
	{
		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetFor(grp);
		if (!cfg || !cfg.GetNightLightDiscipline())
			return;

		array<AIAgent> agents = {};
		grp.GetAgents(agents);
		foreach (AIAgent a : agents)
		{
			IEntity ent = a.GetControlledEntity();
			SCR_GadgetManagerComponent gm = SCR_GadgetManagerComponent.GetGadgetManager(ent);
			if (!gm)
				continue;

			IEntity gadget = gm.GetGadgetByType(EGadgetType.FLASHLIGHT);
			if (!gadget)
				continue;
			SCR_FlashlightComponent fl = SCR_FlashlightComponent.Cast(gadget.FindComponent(SCR_FlashlightComponent));
			if (!fl)
				continue;

			bool want = !stealth && cqb && cqb.IsEntry(a) && SCR_CoverManagerComponent.DCO_GetBuildingAt(ent) != null;
			if (fl.IsToggledOn() == want)
				continue;

			gm.AskToggleGadget(fl, want);
			if (cqb)
			{
				string state = "off";
				if (want)
					state = "on";
				DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("light_%1 grp=%2 unit=%3 cqb=1", state, grp, ent));
			}
		}
	}

	protected static ref map<IEntity, float> s_mHeadlightsOff;
	protected static map<IEntity, float> GetHeadlightsOff()
	{
		if (!s_mHeadlightsOff)
			s_mHeadlightsOff = new map<IEntity, float>();
		return s_mHeadlightsOff;
	}

	static void ApplyHeadlights(notnull SCR_AIGroup grp, SCR_AIGroupPerception perc)
	{
		IEntity veh = DCO_VehicleCombat.GetVehicle(grp.GetLeaderEntity());
		if (!veh || !perc)
			return;

		BaseLightManagerComponent lights = BaseLightManagerComponent.Cast(veh.FindComponent(BaseLightManagerComponent));
		if (!lights || (!lights.GetLightsState(ELightType.Head) && !lights.GetLightsState(ELightType.HiBeam)))
			return;

		float rangeSq = 800.0 * 800.0;
		vector p = veh.GetOrigin();
		bool threat = false;
		foreach (SCR_AITargetInfo t : perc.m_aTargets)
		{
			if (t && (t.m_eCategory == EAITargetInfoCategory.DETECTED || t.m_eCategory == EAITargetInfoCategory.IDENTIFIED)
				&& vector.DistanceSqXZ(t.m_vWorldPos, p) <= rangeSq)
			{
				threat = true;
				break;
			}
		}
		if (!threat)
			return;

		lights.SetLightsState(ELightType.Head, false);
		lights.SetLightsState(ELightType.HiBeam, false);

		float now = GetGame().GetWorld().GetWorldTime();
		if (now - GetHeadlightsOff().Get(veh) > 60000)
		{
			GetHeadlightsOff().Set(veh, now);
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("headlights_off veh=%1 grp=%2", veh, grp));
		}
	}
}
