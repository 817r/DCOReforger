class DCO_LogiSupply
{
	protected static ref array<SCR_ResourceComponent> s_aFound;
	protected static ref map<AICommander_BaseComponent, float> s_mLogAt;
	protected static ref map<AICommander_BaseComponent, float> s_mMortarAt;
	protected static ref map<AICommander_BaseComponent, float> s_mRunAt;
	protected static ResourceName s_sQueryPrefab;
	protected static bool s_bQueryHit;

	static bool Paying(AICommander_BaseComponent cmd)
	{
		if (!cmd)
			return false;
		float mode = DCO_Ctx.TuneG("logi.supply_mode", 1.0);
		if (mode <= 0)
			return false;
		if (mode >= 2)
			return true;
		return SCR_ResourceSystemHelper.IsGlobalResourceTypeEnabled();
	}

	protected static bool QueryCb(IEntity e)
	{
		if (!e || Vehicle.Cast(e) || ChimeraCharacter.Cast(e))
			return true;
		SCR_ResourceComponent rc = SCR_ResourceComponent.FindResourceComponent(e);
		if (rc && !s_aFound.Contains(rc))
			s_aFound.Insert(rc);
		return true;
	}

	static array<SCR_ResourceComponent> HubContainers(AICommander_BaseComponent cmd)
	{
		if (!s_aFound)
			s_aFound = {};
		s_aFound.Clear();
		if (!cmd || !cmd.GetOwner())
			return s_aFound;
		string src;
		vector hub = cmd.GetHubFor(cmd.GetOwner().GetOrigin(), src);
		if (hub == vector.Zero)
			return s_aFound;
		GetGame().GetWorld().QueryEntitiesBySphere(hub, DCO_Ctx.TuneG("logi.hub_supply_m", 60.0), QueryCb);
		return s_aFound;
	}

	static float Stored(SCR_ResourceComponent rc)
	{
		SCR_ResourceConsumer c = SCR_ResourceSystemHelper.GetFirstValidConsumer(rc);
		if (!c)
			return 0;
		return c.GetAggregatedResourceValue();
	}

	static float HubStock(AICommander_BaseComponent cmd, out int containers)
	{
		float total;
		containers = 0;
		foreach (SCR_ResourceComponent rc : HubContainers(cmd))
		{
			containers++;
			total += Stored(rc);
		}
		return total;
	}

	static float ItemCost(ResourceName prefab)
	{
		SCR_EntityCatalogManagerComponent mgr = SCR_EntityCatalogManagerComponent.GetInstance();
		if (!mgr || prefab.IsEmpty())
			return 0;
		SCR_EntityCatalogEntry entry = mgr.GetEntryWithPrefabFromCatalog(EEntityCatalogType.ITEM, prefab);
		if (!entry)
			return 0;
		SCR_ArsenalItem data = SCR_ArsenalItem.Cast(entry.GetEntityDataOfType(SCR_ArsenalItem));
		if (!data)
			return 0;
		return data.GetSupplyCost(SCR_EArsenalSupplyCostType.DEFAULT);
	}

	protected static float Take(SCR_ResourceComponent rc, float amount)
	{
		if (!rc || amount <= 0)
			return 0;
		SCR_ResourceConsumer c = SCR_ResourceSystemHelper.GetFirstValidConsumer(rc);
		if (!c)
			return 0;
		float take = Math.Min(amount, c.GetAggregatedResourceValue());
		if (take <= 0)
			return 0;
		if (c.RequestConsumtion(take).GetReason() != EResourceReason.SUFFICIENT)
			return 0;
		return take;
	}

	static bool Pay(AICommander_BaseComponent cmd, float cost, IEntity truck, string what)
	{
		if (!Paying(cmd))
			return true;
		if (cost <= 0)
			return true;
		float left = cost;
		string from = "hub";
		if (truck)
		{
			SCR_ResourceComponent trc = SCR_ResourceComponent.FindResourceComponent(truck);
			if (trc)
			{
				left -= Take(trc, left);
				if (left <= 0.01)
					from = "truck";
			}
		}
		int n;
		float stock = HubStock(cmd, n);
		if (left > 0.01 && stock + 0.01 < left)
		{
			DCO_Perf.Count("logi:supply_denied_" + what);
			if (cmd.GetLogistics())
				cmd.GetLogistics().m_iSupplyDenied++;
			DCO_Log.Write(DCO_ELogChannel.LOGI, DCO_ELogLevel.INFO, DCO_Log.IdCmd(cmd), string.Format("logi_supply_denied cmd=%1 what=%2 cost=%3 hub_stock=%4 containers=%5", DCO_Log.IdCmd(cmd), what, cost.ToString(-1, 1), stock.ToString(-1, 1), n));
			return false;
		}
		foreach (SCR_ResourceComponent rc : HubContainers(cmd))
		{
			if (left <= 0.01)
				break;
			left -= Take(rc, left);
		}
		DCO_Perf.Count("logi:pay_" + what);
		DCO_Log.Write(DCO_ELogChannel.LOGI, DCO_ELogLevel.INFO, DCO_Log.IdCmd(cmd), string.Format("logi_pay cmd=%1 what=%2 cost=%3 from=%4 hub_left=%5", DCO_Log.IdCmd(cmd), what, cost.ToString(-1, 1), from, (stock - (cost - left)).ToString(-1, 1)));
		return true;
	}

	static AICommander_BaseComponent CommanderOf(IEntity soldier)
	{
		if (!soldier)
			return null;
		AIControlComponent ctrl = AIControlComponent.Cast(soldier.FindComponent(AIControlComponent));
		if (!ctrl || !ctrl.GetAIAgent())
			return null;
		SCR_AIGroup grp = SCR_AIGroup.Cast(ctrl.GetAIAgent().GetParentGroup());
		if (!grp)
			return null;
		DCO_GroupUtilityComponent g = DCO_GroupUtilityComponent.Cast(grp.FindComponent(DCO_GroupUtilityComponent));
		if (!g)
			return null;
		return g.GetMyCommander();
	}

	static bool PayMag(IEntity soldier, ResourceName magPrefab, IEntity station)
	{
		AICommander_BaseComponent cmd = CommanderOf(soldier);
		if (!cmd)
			return true;
		float cost = ItemCost(magPrefab) * DCO_Ctx.TuneG("logi.mag_cost_mul", 1.0);
		return Pay(cmd, cost, station, "magazine");
	}

	static bool PayShell(AICommander_BaseComponent cmd)
	{
		return Pay(cmd, DCO_Ctx.TuneG("logi.shell_cost", 20.0), null, "shell");
	}

	static bool HasArsenalFor(IEntity ent, ResourceName prefab)
	{
		if (!ent || prefab.IsEmpty())
			return false;
		SCR_ArsenalInventoryStorageManagerComponent a = SCR_ArsenalInventoryStorageManagerComponent.Cast(ent.FindComponent(SCR_ArsenalInventoryStorageManagerComponent));
		if (a && a.IsPrefabInArsenalStorage(prefab))
			return true;
		IEntity child = ent.GetChildren();
		while (child)
		{
			if (HasArsenalFor(child, prefab))
				return true;
			child = child.GetSibling();
		}
		return false;
	}

	protected static bool ArsenalCb(IEntity e)
	{
		if (s_bQueryHit)
			return false;
		SCR_ArsenalInventoryStorageManagerComponent a = SCR_ArsenalInventoryStorageManagerComponent.Cast(e.FindComponent(SCR_ArsenalInventoryStorageManagerComponent));
		if (a && a.IsPrefabInArsenalStorage(s_sQueryPrefab))
			s_bQueryHit = true;
		return !s_bQueryHit;
	}

	static bool ArsenalNear(vector pos, float radius, ResourceName prefab)
	{
		s_sQueryPrefab = prefab;
		s_bQueryHit = false;
		GetGame().GetWorld().QueryEntitiesBySphere(pos, radius, ArsenalCb);
		return s_bQueryHit;
	}

	static void MortarCheck(AICommander_BaseComponent cmd, DCO_Logistics logi, float now)
	{
		if (!cmd || !logi || !cmd.IsLogiResupply())
			return;
		if (DCO_Ctx.TuneG("logi.mortar_resupply", 0.0) <= 0)
			return;
		if (!s_mMortarAt)
			s_mMortarAt = new map<AICommander_BaseComponent, float>();
		float at;
		if (s_mMortarAt.Find(cmd, at) && now - at < 30.0)
			return;
		s_mMortarAt.Set(cmd, now);
		array<IEntity> mortars = {};
		DCO_GroupUtilityComponent.F_DCO_MortarsOf(cmd, mortars);
		float radius = DCO_Ctx.TuneG("logi.mortar_arsenal_m", 25.0);
		foreach (IEntity m : mortars)
		{
			SCR_AIStaticArtilleryVehicleUsageComponent u = SCR_AIStaticArtilleryVehicleUsageComponent.Cast(m.FindComponent(SCR_AIStaticArtilleryVehicleUsageComponent));
			if (!u || u.GetDamageState() == EDamageState.DESTROYED)
				continue;
			ResourceName ammo = u.GetAmmoResourceName(SCR_EAIArtilleryAmmoType.HIGH_EXPLOSIVE);
			if (ammo.IsEmpty())
				continue;
			vector mp = m.GetOrigin();
			if (ArsenalNear(mp, radius, ammo))
			{
				DCO_Perf.Count("logi:mortar_arsenal_ok");
				continue;
			}
			string src;
			vector toHub = cmd.GetHubFor(mp, src) - mp;
			toHub[1] = 0;
			if (toHub.LengthSq() < 1.0)
				toHub = "0 0 1";
			vector dest = DCO_Util.Surface(mp + toHub.Normalized() * 10.0);
			DCO_Perf.Count("logi:mortar_no_arsenal");
			if (logi.NoTeam(cmd, "mortar ammo"))
				continue;
			logi.QueueMortarResupply(cmd, m, ammo, dest, now);
		}
	}

	static SCR_ResourceContainer EntityContainer(IEntity e)
	{
		if (!e)
			return null;
		SCR_ResourceComponent rc = SCR_ResourceComponent.FindResourceComponent(e);
		if (!rc)
			return null;
		SCR_ResourceContainer c = rc.GetContainer(EResourceType.SUPPLIES);
		if (!c || c.GetMaxResourceValue() <= 0)
			return null;
		return c;
	}

	static SCR_ResourceContainer TruckContainer(IEntity veh)
	{
		if (!veh || !Vehicle.Cast(veh))
			return null;
		return EntityContainer(veh);
	}

	static float ContainerValue(SCR_ResourceContainer c)
	{
		if (!c)
			return 0;
		return c.GetResourceValue();
	}

	static float Transfer(SCR_ResourceContainer from, SCR_ResourceContainer to, float amount)
	{
		if (!from || !to || amount <= 0)
			return 0;
		float take = Math.Min(amount, from.GetResourceValue());
		take = Math.Min(take, to.GetMaxResourceValue() - to.GetResourceValue());
		if (take <= 0.01)
			return 0;
		if (!from.DecreaseResourceValue(take))
			return 0;
		if (!to.IncreaseResourceValue(take))
		{
			from.IncreaseResourceValue(take);
			return 0;
		}
		return take;
	}

	static float UnloadToHub(AICommander_BaseComponent cmd, SCR_ResourceContainer truck, float amount)
	{
		float left = amount;
		foreach (SCR_ResourceComponent rc : HubContainers(cmd))
		{
			if (left <= 0.01)
				break;
			SCR_ResourceContainer hc = rc.GetContainer(EResourceType.SUPPLIES);
			if (!hc)
				continue;
			left -= Transfer(truck, hc, left);
		}
		return amount - left;
	}

	static bool FactionOk(IEntity e, FactionKey mine)
	{
		IEntity cur = e;
		int guard;
		while (cur && guard < 4)
		{
			guard++;
			FactionAffiliationComponent fa = FactionAffiliationComponent.Cast(cur.FindComponent(FactionAffiliationComponent));
			if (fa && fa.GetAffiliatedFaction())
				return fa.GetAffiliatedFaction().GetFactionKey() == mine;
			cur = cur.GetParent();
		}
		return true;
	}

	static IEntity FindDepot(AICommander_BaseComponent cmd, out vector depotPos, out float depotStock)
	{
		depotPos = vector.Zero;
		depotStock = 0;
		if (!cmd || !cmd.GetOwner())
			return null;
		string src;
		vector hub = cmd.GetHubFor(cmd.GetOwner().GetOrigin(), src);
		float hubR = DCO_Ctx.TuneG("logi.hub_supply_m", 60.0);
		float minStock = DCO_Ctx.TuneG("logi.depot_min", 200.0);
		if (!s_aFound)
			s_aFound = {};
		s_aFound.Clear();
		GetGame().GetWorld().QueryEntitiesBySphere(hub, DCO_Ctx.TuneG("logi.depot_search_m", 3000.0), QueryCb);
		array<SCR_ResourceComponent> found = {};
		found.Copy(s_aFound);
		IEntity best;
		float bestD = float.MAX;
		FactionKey mine = cmd.GetCommanderFactionKey();
		foreach (SCR_ResourceComponent rc : found)
		{
			IEntity e = rc.GetOwner();
			if (!e)
				continue;
			float d = vector.DistanceXZ(e.GetOrigin(), hub);
			if (d <= hubR || d >= bestD)
				continue;
			SCR_ResourceContainer c = rc.GetContainer(EResourceType.SUPPLIES);
			if (!c || c.GetResourceValue() < minStock)
				continue;
			if (!FactionOk(e, mine))
				continue;
			best = e;
			bestD = d;
			depotStock = c.GetResourceValue();
		}
		if (best)
			depotPos = best.GetOrigin();
		return best;
	}

	static void SupplyRunCheck(AICommander_BaseComponent cmd, DCO_Logistics logi, float now)
	{
		if (!cmd || !logi || !Paying(cmd))
			return;
		if (DCO_Ctx.TuneG("logi.supply_runs", 1.0) <= 0)
			return;
		if (!s_mRunAt)
			s_mRunAt = new map<AICommander_BaseComponent, float>();
		float at;
		if (s_mRunAt.Find(cmd, at) && now - at < 30.0)
			return;
		s_mRunAt.Set(cmd, now);
		if (logi.HasSupplyRun())
			return;
		int n;
		float stock = HubStock(cmd, n);
		if (n == 0 || stock >= DCO_Ctx.TuneG("logi.hub_low", 300.0))
			return;
		vector depotPos;
		float depotStock;
		IEntity depot = FindDepot(cmd, depotPos, depotStock);
		if (!depot)
		{
			s_mRunAt.Set(cmd, now + 270.0);
			DCO_Perf.Count("logi:supply_run_no_depot");
			DCO_Log.Write(DCO_ELogChannel.LOGI, DCO_ELogLevel.INFO, DCO_Log.IdCmd(cmd), string.Format("supply_run_skip cmd=%1 why=no_depot hub_stock=%2", DCO_Log.IdCmd(cmd), stock.ToString(-1, 1)));
			return;
		}
		if (logi.NoTeam(cmd, "supply run"))
		{
			s_mRunAt.Set(cmd, now + 270.0);
			return;
		}
		string src;
		vector hub = cmd.GetHubFor(cmd.GetOwner().GetOrigin(), src);
		DCO_Log.Write(DCO_ELogChannel.LOGI, DCO_ELogLevel.INFO, DCO_Log.IdCmd(cmd), string.Format("supply_run_need cmd=%1 hub_stock=%2 depot=%3 depot_stock=%4 dist=%5", DCO_Log.IdCmd(cmd), stock.ToString(-1, 1), depot.GetName(), depotStock.ToString(-1, 1), Math.Round(vector.DistanceXZ(depotPos, hub))));
		logi.QueueSupplyRun(cmd, depot, depotPos, hub, now);
	}

	static void Tick(AICommander_BaseComponent cmd, float now)
	{
		if (!cmd)
			return;
		if (!s_mLogAt)
			s_mLogAt = new map<AICommander_BaseComponent, float>();
		float at;
		if (s_mLogAt.Find(cmd, at) && now - at < 60.0)
			return;
		s_mLogAt.Set(cmd, now);
		int n;
		float stock = HubStock(cmd, n);
		string mode = "free";
		if (Paying(cmd))
			mode = "paying";
		DCO_Perf.Count("logi:stock_log");
		DCO_Log.Write(DCO_ELogChannel.LOGI, DCO_ELogLevel.INFO, DCO_Log.IdCmd(cmd), string.Format("logi_supply cmd=%1 hub_stock=%2 containers=%3 mode=%4 radius=%5", DCO_Log.IdCmd(cmd), stock.ToString(-1, 1), n, mode, Math.Round(DCO_Ctx.TuneG("logi.hub_supply_m", 60.0))));
		SupplyState(cmd, stock, n, mode == "paying", now);
	}

	protected static void SupplyState(AICommander_BaseComponent cmd, float stock, int containers, bool paying, float now)
	{
		DCO_Logistics logi = cmd.GetLogistics();
		if (!logi)
			return;
		float low = DCO_Ctx.TuneG("logi.hub_low", 300.0);
		int state;
		if (paying && containers == 0)
			state = 1;
		else if (paying && stock < 1.0)
			state = 3;
		else if (paying && (stock < low || (logi.m_iSupplyState == 2 && stock < low * 1.2)))
			state = 2;
		if (state != logi.m_iSupplyState)
		{
			logi.m_iSupplyState = state;
			string name = "ok";
			string text = string.Format("hub supplies are back to normal (%1).", Math.Round(stock));
			if (state == 1)
			{
				name = "no_container";
				text = string.Format("pays for ammunition but has no supply container within %1 m of its hub. Resupply and mortar shells will be refused.", Math.Round(DCO_Ctx.TuneG("logi.hub_supply_m", 60.0)));
			}
			else if (state >= 2)
			{
				name = "low";
				text = string.Format("hub supplies are low (%1, refill line %2).", Math.Round(stock), Math.Round(low));
				if (state == 3)
				{
					name = "empty";
					text = "hub supplies are used up. Paid resupply and mortar shells are refused.";
				}
				vector depotPos;
				float depotStock;
				if (FindDepot(cmd, depotPos, depotStock))
					text += " A friendly depot is in range for supply runs.";
				else
					text += " No friendly depot with enough supplies is in range, so trucks cannot refill it.";
			}
			DCO_Perf.Count("logi:supply_state_" + name);
			DCO_Log.Write(DCO_ELogChannel.LOGI, DCO_ELogLevel.INFO, DCO_Log.IdCmd(cmd), string.Format("logi_supply_state cmd=%1 state=%2 hub_stock=%3 containers=%4 low=%5", DCO_Log.IdCmd(cmd), name, stock.ToString(-1, 1), containers, Math.Round(low)));
			DCO_PlayerComms.SendToGameMasters("DCO", cmd.GetCommanderUID() + ": " + text);
		}
		if (logi.m_iSupplyDenied > 0 && now - logi.m_fSupplyDeniedToldAt >= 600.0)
		{
			DCO_Log.Write(DCO_ELogChannel.LOGI, DCO_ELogLevel.INFO, DCO_Log.IdCmd(cmd), string.Format("logi_supply_denied_sum cmd=%1 denied=%2 hub_stock=%3", DCO_Log.IdCmd(cmd), logi.m_iSupplyDenied, stock.ToString(-1, 1)));
			DCO_PlayerComms.SendToGameMasters("DCO", string.Format("%1 could not pay for %2 resupplies or mortar shells since the last report (hub supplies %3).", cmd.GetCommanderUID(), logi.m_iSupplyDenied, Math.Round(stock)));
			logi.m_iSupplyDenied = 0;
			logi.m_fSupplyDeniedToldAt = now;
		}
	}
}
