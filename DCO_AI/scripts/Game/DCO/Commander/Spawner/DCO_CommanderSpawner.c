enum DCO_ESpawnerTarget
{
	SPECIFIC_UID,
	WEAKEST_OF_FACTION,
	NEAREST_OF_FACTION
}

enum DCO_ESpawnerObjectiveEvent
{
	NONE,
	LOST,
	CAPTURED
}

enum DCO_ESpawnerRole
{
	AUTO,
	ASSAULT,
	FIRE_TEAM,
	AT,
	RECON,
	MOTORIZED
}

[BaseContainerProps(), SCR_BaseContainerCustomTitleField("m_sPrefab")]
class DCO_SpawnerGroupEntry
{
	[Attribute("", UIWidgets.ResourcePickerThumbnail, "Prefab grup", params: "et")]
	ResourceName m_sPrefab;

	[Attribute("", UIWidgets.ResourcePickerThumbnail, "Prefab kendaraan opsional yang dipakai grup ini (spawn di dekat grup, didaftarkan sebagai kendaraan grup).", params: "et")]
	ResourceName m_sVehicle;

	[Attribute("1", UIWidgets.EditBox, "Bobot pilihan")]
	int m_iWeight;

	[Attribute("-1", UIWidgets.EditBox, "Budget: berapa kali grup ini boleh dikeluarkan. -1 = tanpa batas.")]
	int m_iBudget;

	[Attribute("0", UIWidgets.ComboBox, "Role grup untuk pilihan commander. AUTO = dari ukuran grup / kendaraan.", enums: ParamEnumArray.FromEnum(DCO_ESpawnerRole))]
	DCO_ESpawnerRole m_eRole;

	int m_iUsed;
	int m_iSoldiers = -1;

	bool HasBudget()
	{
		return m_iBudget < 0 || m_iUsed < m_iBudget;
	}

	int Left()
	{
		if (m_iBudget < 0)
			return -1;
		return Math.Max(m_iBudget - m_iUsed, 0);
	}

	DCO_ESpawnerRole ResolveRole()
	{
		if (m_eRole != DCO_ESpawnerRole.AUTO)
			return m_eRole;
		if (!m_sVehicle.IsEmpty())
			return DCO_ESpawnerRole.MOTORIZED;
		if (m_iSoldiers >= 0 && m_iSoldiers <= 4)
			return DCO_ESpawnerRole.FIRE_TEAM;
		return DCO_ESpawnerRole.ASSAULT;
	}

	string Label()
	{
		string name = FilePath.StripExtension(FilePath.StripPath(m_sPrefab));
		if (name.IsEmpty())
			name = "catalog";
		return name;
	}
}

[ComponentEditorProps(category: "GameScripted/Commander", description: "DCO Commander Spawner: spawn grup dan langsung serahkan ke AI Commander")]
class DCO_CommanderSpawnerComponentClass : ScriptComponentClass {}

class DCO_CommanderSpawnerComponent : ScriptComponent
{
	[Attribute("1", UIWidgets.ComboBox, "Commander tujuan", enums: ParamEnumArray.FromEnum(DCO_ESpawnerTarget), category: "Target")]
	protected DCO_ESpawnerTarget m_eTarget;

	[Attribute("", UIWidgets.EditBox, "UID commander (mode SPECIFIC_UID)", category: "Target")]
	protected string m_sCommanderUID;

	[Attribute("", UIWidgets.EditBox, "Faction key (mode WEAKEST / NEAREST). Kosong = semua faction.", category: "Target")]
	protected FactionKey m_sFaction;

	[Attribute("", UIWidgets.Object, "Daftar prefab grup berbobot. Kosong = grup acak dari katalog faction commander.", category: "Content")]
	protected ref array<ref DCO_SpawnerGroupEntry> m_aGroups;

	[Attribute("1", UIWidgets.CheckBox, "Commander memilih jenis grup sesuai kebutuhan (AT, assault, recon, motorized). OFF = acak berbobot.", category: "Content")]
	protected bool m_bCommanderChooses;

	[Attribute("1", UIWidgets.EditBox, "Jumlah grup per spawn", category: "Content")]
	protected int m_iGroupsPerSpawn;

	[Attribute("30", UIWidgets.EditBox, "Radius spawn di sekitar spawner (m)", category: "Content")]
	protected float m_fSpawnRadius;

	[Attribute("0", UIWidgets.CheckBox, "Spawn sekali waktu misi mulai", category: "Modes")]
	protected bool m_bSpawnAtStart;

	[Attribute("300", UIWidgets.EditBox, "Cooldown antar spawn dari trigger (detik)", category: "Modes")]
	protected float m_fCooldown;

	[Attribute("", UIWidgets.EditBox, "Nama entity objective pemicu (kosong = mati)", category: "Modes")]
	protected string m_sTriggerObjective;

	[Attribute("0", UIWidgets.ComboBox, "Event objective pemicu (dilihat dari faction commander tujuan)", enums: ParamEnumArray.FromEnum(DCO_ESpawnerObjectiveEvent), category: "Modes")]
	protected DCO_ESpawnerObjectiveEvent m_eObjectiveEvent;

	[Attribute("0", UIWidgets.EditBox, "Spawn kalau manpower commander tujuan di bawah angka ini (0 = mati)", category: "Modes")]
	protected int m_iManpowerBelow;

	[Attribute("0", UIWidgets.CheckBox, "Commander boleh minta bala bantuan sendiri kalau cadangannya di bawah Reserve Policy", category: "Modes")]
	protected bool m_bCommanderRequest;

	[Attribute("0", UIWidgets.CheckBox, "Pakai stok bala bantuan commander. Tiap prajurit mengurangi stok; stok habis = spawner berhenti.", category: "Manpower")]
	protected bool m_bUseStock;

	[Attribute("300", UIWidgets.EditBox, "Radius aman (m): gak spawn kalau ada pemain atau musuh di dalamnya", category: "Safety")]
	protected float m_fSafeRadius;

	protected static const int TICK_MS = 5000;
	protected static const float START_DELAY_S = 10;

	protected string m_sPending;
	protected float m_fNextAllowed;
	protected bool m_bStartDone;
	protected bool m_bStopped;
	protected int m_iLastObjState = -1;
	protected float m_fStartTime;
	protected string m_sLastDefer;

	protected static ref array<DCO_CommanderSpawnerComponent> s_aAll = {};

	static array<DCO_CommanderSpawnerComponent> GetAll()	{ return s_aAll; }

	DCO_ESpawnerTarget GetTargetMode()			{ return m_eTarget; }
	string GetCommanderUID()					{ return m_sCommanderUID; }
	void SetTarget(DCO_ESpawnerTarget mode, string uid)
	{
		m_eTarget = mode;
		m_sCommanderUID = uid;
	}
	FactionKey GetFaction()						{ return m_sFaction; }
	void SetFaction(FactionKey fk)				{ m_sFaction = fk; }
	int GetGroupsPerSpawn()						{ return m_iGroupsPerSpawn; }
	void SetGroupsPerSpawn(int n)				{ m_iGroupsPerSpawn = Math.Clamp(n, 1, 20); }
	float GetSpawnRadius()						{ return m_fSpawnRadius; }
	void SetSpawnRadius(float r)				{ m_fSpawnRadius = Math.Clamp(r, 5, 500); }
	bool GetSpawnAtStart()						{ return m_bSpawnAtStart; }
	void SetSpawnAtStart(bool b)				{ m_bSpawnAtStart = b; }
	float GetCooldown()							{ return m_fCooldown; }
	void SetCooldown(float s)					{ m_fCooldown = Math.Clamp(s, 0, 7200); }
	string GetTriggerObjective()				{ return m_sTriggerObjective; }
	void SetTriggerObjective(string name)
	{
		m_sTriggerObjective = name;
		m_iLastObjState = -1;
	}
	DCO_ESpawnerObjectiveEvent GetObjectiveEvent()	{ return m_eObjectiveEvent; }
	void SetObjectiveEvent(DCO_ESpawnerObjectiveEvent e)	{ m_eObjectiveEvent = e; }
	int GetManpowerBelow()						{ return m_iManpowerBelow; }
	void SetManpowerBelow(int n)				{ m_iManpowerBelow = Math.Max(n, 0); }
	bool GetUseStock()							{ return m_bUseStock; }
	void SetUseStock(bool b)
	{
		m_bUseStock = b;
		m_bStopped = false;
	}
	bool GetCommanderChooses()					{ return m_bCommanderChooses; }
	void SetCommanderChooses(bool b)			{ m_bCommanderChooses = b; }
	bool GetCommanderRequest()					{ return m_bCommanderRequest; }
	void SetCommanderRequest(bool b)			{ m_bCommanderRequest = b; }
	float GetSafeRadius()						{ return m_fSafeRadius; }
	void SetSafeRadius(float r)					{ m_fSafeRadius = Math.Clamp(r, 0, 2000); }

	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if (!s_aAll.Contains(this))
			s_aAll.Insert(this);
		if (!Replication.IsServer() || !GetGame().InPlayMode())
			return;
		m_fStartTime = Now();
		GetGame().GetCallqueue().CallLater(Tick, TICK_MS, true);
	}

	override void OnDelete(IEntity owner)
	{
		s_aAll.RemoveItem(this);
		if (GetGame())
			GetGame().GetCallqueue().Remove(Tick);
		super.OnDelete(owner);
	}

	void SpawnNow(int playerID)
	{
		m_bStopped = false;
		m_sPending = "gm";
		m_fNextAllowed = 0;
		Tick();
	}

	protected void Tick()
	{
		float now = Now();
		AICommander_BaseComponent cmd = ResolveCommander();

		if (m_bSpawnAtStart && !m_bStartDone && now - m_fStartTime >= START_DELAY_S)
		{
			m_bStartDone = true;
			Request("start", true);
		}

		if (cmd)
		{
			if (m_iManpowerBelow > 0 && cmd.GetTotalManpower() < m_iManpowerBelow)
				Request("manpower", false);
			if (m_bCommanderRequest && cmd.GetReserveFloor() > 0 && cmd.GetReserveManpower() < cmd.GetReserveFloor())
				Request("commander_request", false);
			CheckObjective(cmd.GetCommanderFactionKey());
		}

		if (!m_sPending.IsEmpty())
			TrySpawn(cmd, now);
	}

	protected void Request(string reason, bool ignoreCooldown)
	{
		if (m_bStopped || !m_sPending.IsEmpty())
			return;
		if (!ignoreCooldown && Now() < m_fNextAllowed)
			return;
		m_sPending = reason;
	}

	protected void CheckObjective(FactionKey fk)
	{
		if (m_sTriggerObjective.IsEmpty() || m_eObjectiveEvent == DCO_ESpawnerObjectiveEvent.NONE)
			return;

		CMD_AICommanderObjectiveComponent obj = FindObjective(m_sTriggerObjective);
		if (!obj)
			return;

		int state = 0;
		if (obj.IsCapturedBy(fk))
			state = 1;

		int last = m_iLastObjState;
		m_iLastObjState = state;
		if (last < 0 || last == state)
			return;

		if (m_eObjectiveEvent == DCO_ESpawnerObjectiveEvent.CAPTURED && state == 1)
			Request("objective_captured", false);
		else if (m_eObjectiveEvent == DCO_ESpawnerObjectiveEvent.LOST && state == 0)
			Request("objective_lost", false);
	}

	static CMD_AICommanderObjectiveComponent FindObjective(string name)
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr || name.IsEmpty())
			return null;
		foreach (CMD_AICommanderObjectiveComponent o : mgr.m_aObjective)
		{
			if (o && o.GetOwner() && o.GetOwner().GetName() == name)
				return o;
		}
		return null;
	}

	AICommander_BaseComponent ResolveCommander()
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
			return null;

		if (m_eTarget == DCO_ESpawnerTarget.SPECIFIC_UID)
			return mgr.FindCommanderByUID(m_sCommanderUID);

		vector origin = GetOwner().GetOrigin();
		AICommander_BaseComponent best;
		float bestScore = float.MAX;
		foreach (AICommander_BaseComponent c : mgr.m_aCommander)
		{
			if (!c || (!m_sFaction.IsEmpty() && c.GetCommanderFactionKey() != m_sFaction))
				continue;

			float score;
			if (m_eTarget == DCO_ESpawnerTarget.NEAREST_OF_FACTION)
				score = vector.DistanceXZ(c.GetOwner().GetOrigin(), origin);
			else
				score = c.GetTotalManpower() / Math.Max(c.GetObjectiveList().Count(), 1.0);

			if (score < bestScore)
			{
				bestScore = score;
				best = c;
			}
		}
		return best;
	}

	protected void TrySpawn(AICommander_BaseComponent cmd, float now)
	{
		if (!cmd)
		{
			Defer("no_commander");
			return;
		}

		FactionManager fm = GetGame().GetFactionManager();
		Faction faction;
		if (fm)
			faction = fm.GetFactionByKey(cmd.GetCommanderFactionKey());
		if (!faction)
		{
			Defer("no_faction");
			return;
		}

		string block = SafetyBlock(faction);
		if (!block.IsEmpty())
		{
			Defer(block);
			return;
		}

		int needs = ReadNeeds(cmd);
		int spawned;
		string deployed;
		for (int i = 0; i < m_iGroupsPerSpawn; i++)
		{
			if (!AnyBudget())
			{
				StopBudget(cmd);
				break;
			}

			string why;
			DCO_SpawnerGroupEntry entry = PickEntry(faction, needs, why);
			if (!entry)
			{
				Defer("no_prefab");
				break;
			}

			Resource res = Resource.Load(entry.m_sPrefab);
			if (!res || !res.IsValid())
			{
				Defer("bad_prefab");
				break;
			}

			int soldiers = PrefabSoldiers(res);
			if (m_bUseStock && !cmd.ConsumeStock(soldiers))
			{
				StopEmpty(cmd);
				break;
			}

			if (!SpawnGroup(cmd, entry, res, soldiers))
				continue;

			spawned++;
			entry.m_iUsed++;
			entry.m_iSoldiers = soldiers;
			DCO_ESpawnerRole role = entry.ResolveRole();
			needs = needs & ~NeedBit(role);
			if (!deployed.IsEmpty())
				deployed += ", ";
			deployed += entry.Label();
			DCO_BenchmarkLoggerComponent.Event(string.Format("spawner_choice spawner=%1 cmd=%2 prefab=%3 role=%4 why=%5 left=%6",
				GetOwner().GetName(), cmd.GetCommanderUID(), entry.Label(), typename.EnumToString(DCO_ESpawnerRole, role), why, entry.Left()));
		}

		if (spawned > 0 && m_aGroups && !m_aGroups.IsEmpty())
			DCO_PlayerComms.SendToGameMasters("DCO", "Spawner " + GetOwner().GetName() + " -> " + cmd.GetCommanderUID() + ": " + deployed + ". Budget: " + BudgetText());

		if (spawned > 0 || m_bStopped)
		{
			m_sPending = string.Empty;
			m_sLastDefer = string.Empty;
			m_fNextAllowed = now + m_fCooldown;
		}
	}

	protected void Defer(string reason)
	{
		if (reason == m_sLastDefer)
			return;
		m_sLastDefer = reason;
		DCO_BenchmarkLoggerComponent.Event(string.Format("spawner_defer spawner=%1 reason=%2 trigger=%3", GetOwner().GetName(), reason, m_sPending));
	}

	protected void StopEmpty(AICommander_BaseComponent cmd)
	{
		m_bStopped = true;
		string line = string.Format("spawner_stock_empty spawner=%1 cmd=%2", GetOwner().GetName(), cmd.GetCommanderUID());
		Print("[DCO_Spawner] " + line, LogLevel.WARNING);
		DCO_BenchmarkLoggerComponent.Event(line);
		DCO_PlayerComms.SendToGameMasters("DCO", "Spawner " + GetOwner().GetName() + ": " + cmd.GetCommanderUID() + " reinforcement stock is empty, spawner stopped.");
	}

	protected string SafetyBlock(Faction faction)
	{
		vector origin = GetOwner().GetOrigin();
		float safeSq = m_fSafeRadius * m_fSafeRadius;

		if (m_fSafeRadius > 0)
		{
			array<int> players = {};
			GetGame().GetPlayerManager().GetPlayers(players);
			foreach (int pid : players)
			{
				IEntity p = GetGame().GetPlayerManager().GetPlayerControlledEntity(pid);
				if (p && vector.DistanceSqXZ(p.GetOrigin(), origin) <= safeSq)
					return "player_near";
			}
		}

		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetInstance();
		int cap = 0;
		if (cfg)
			cap = cfg.GetSpawnerFactionAICap();
		if (m_fSafeRadius <= 0 && cap <= 0)
			return string.Empty;

		AIWorld world = GetGame().GetAIWorld();
		if (!world)
			return string.Empty;

		array<AIAgent> agents = {};
		world.GetAIAgents(agents);
		int count;
		foreach (AIAgent a : agents)
		{
			ChimeraCharacter ch = ChimeraCharacter.Cast(a.GetControlledEntity());
			if (!ch)
				continue;
			FactionAffiliationComponent fac = FactionAffiliationComponent.Cast(ch.FindComponent(FactionAffiliationComponent));
			if (!fac || !fac.GetAffiliatedFaction())
				continue;
			Faction f = fac.GetAffiliatedFaction();
			if (f == faction)
			{
				count++;
				continue;
			}
			if (m_fSafeRadius > 0 && !f.IsFactionFriendly(faction) && vector.DistanceSqXZ(ch.GetOrigin(), origin) <= safeSq)
				return "enemy_near";
		}

		if (cap > 0 && count >= cap)
			return "faction_cap";
		return string.Empty;
	}

	protected static bool Usable(DCO_SpawnerGroupEntry e)
	{
		return e && !e.m_sPrefab.IsEmpty() && e.m_iWeight > 0 && e.HasBudget();
	}

	protected bool AnyBudget()
	{
		if (!m_aGroups || m_aGroups.IsEmpty())
			return true;
		foreach (DCO_SpawnerGroupEntry e : m_aGroups)
		{
			if (Usable(e))
				return true;
		}
		return false;
	}

	protected string BudgetText()
	{
		string s;
		foreach (DCO_SpawnerGroupEntry e : m_aGroups)
		{
			if (!e || e.m_sPrefab.IsEmpty())
				continue;
			if (!s.IsEmpty())
				s += ", ";
			if (e.m_iBudget < 0)
				s += e.Label() + " unlimited";
			else
				s += string.Format("%1 %2/%3", e.Label(), e.Left(), e.m_iBudget);
		}
		return s;
	}

	protected void StopBudget(AICommander_BaseComponent cmd)
	{
		m_bStopped = true;
		string line = string.Format("spawner_budget_empty spawner=%1 cmd=%2", GetOwner().GetName(), cmd.GetCommanderUID());
		Print("[DCO_Spawner] " + line, LogLevel.WARNING);
		DCO_BenchmarkLoggerComponent.Event(line);
		DCO_PlayerComms.SendToGameMasters("DCO", "Spawner " + GetOwner().GetName() + ": every group budget is used up, spawner stopped.");
	}

	protected static const int NEED_AT = 1;
	protected static const int NEED_ASSAULT = 2;
	protected static const int NEED_RECON = 4;
	protected static const int NEED_MOTOR = 8;
	protected static const int NEED_CHEAP = 16;
	protected static const float MOTOR_DIST = 1500;

	protected static int NeedBit(DCO_ESpawnerRole role)
	{
		if (role == DCO_ESpawnerRole.AT)
			return NEED_AT;
		if (role == DCO_ESpawnerRole.ASSAULT)
			return NEED_ASSAULT;
		if (role == DCO_ESpawnerRole.FIRE_TEAM || role == DCO_ESpawnerRole.RECON)
			return NEED_RECON;
		if (role == DCO_ESpawnerRole.MOTORIZED)
			return NEED_MOTOR;
		return 0;
	}

	protected static string NeedName(int bit)
	{
		if (bit == NEED_AT)
			return "enemy_armor";
		if (bit == NEED_ASSAULT)
			return "attack_objective";
		if (bit == NEED_RECON)
			return "unclear_contacts";
		if (bit == NEED_MOTOR)
			return "far_objective";
		return "default";
	}

	protected int ReadNeeds(AICommander_BaseComponent cmd)
	{
		int needs;
		CMD_ThreatResponseComponent tr = cmd.GetThreatResponseComponent();
		if (tr)
		{
			int armor;
			int unclear;
			foreach (CMD_ThreatEntry t : tr.GetThreats())
			{
				if (!t)
					continue;
				if (t.m_iArmor > 0 || t.m_bArmorSeen)
					armor++;
				if (t.m_bNeedsRecon && !t.m_bReconSent)
					unclear++;
			}

			int antiArmor;
			foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
			{
				if (g && !g.IsPlayerGroup() && (g.HasAT() || g.IsArmor()))
					antiArmor++;
			}
			if (armor > antiArmor)
				needs |= NEED_AT;
			if (unclear > 0)
				needs |= NEED_RECON;
		}

		FactionKey fk = cmd.GetCommanderFactionKey();
		vector origin = GetOwner().GetOrigin();
		float nearest = float.MAX;
		int enemyObj;
		foreach (CMD_AICommanderObjectiveComponent o : cmd.GetObjectiveList())
		{
			if (!o || !o.GetOwner() || o.IsCapturedBy(fk))
				continue;
			enemyObj++;
			nearest = Math.Min(nearest, vector.DistanceXZ(o.GetOwner().GetOrigin(), origin));
		}
		if (enemyObj > 0)
			needs |= NEED_ASSAULT;
		if (enemyObj > 0 && nearest > MOTOR_DIST)
			needs |= NEED_MOTOR;

		if (cmd.GetReserveFloor() > 0 && cmd.GetReserveManpower() < cmd.GetReserveFloor() * 0.5)
			needs |= NEED_CHEAP;
		return needs;
	}

	protected static float ChoiceScore(DCO_SpawnerGroupEntry e, int needs, out string why)
	{
		DCO_ESpawnerRole role = e.ResolveRole();
		int bit = NeedBit(role);
		float score = 1;
		why = "default";
		if (role == DCO_ESpawnerRole.ASSAULT)
			score = 1.2;
		if (bit != 0 && (needs & bit) != 0)
		{
			score += 3;
			why = NeedName(bit);
			if (role == DCO_ESpawnerRole.RECON)
				score += 0.5;
		}
		if ((needs & NEED_CHEAP) != 0)
		{
			score += (12 - Math.Max(e.m_iSoldiers, 1)) * 0.25;
			if (why == "default")
				why = "low_manpower";
		}
		return score + e.m_iWeight * 0.25 + Math.RandomFloat(0, 0.3);
	}

	protected DCO_SpawnerGroupEntry PickEntry(Faction faction, int needs, out string why)
	{
		why = "random";
		if (m_aGroups && !m_aGroups.IsEmpty())
		{
			if (m_bCommanderChooses)
			{
				DCO_SpawnerGroupEntry best;
				float bestScore = -1;
				string bestWhy;
				foreach (DCO_SpawnerGroupEntry c : m_aGroups)
				{
					if (!Usable(c))
						continue;
					if (c.m_iSoldiers < 0)
					{
						Resource cres = Resource.Load(c.m_sPrefab);
						if (cres && cres.IsValid())
							c.m_iSoldiers = PrefabSoldiers(cres);
						else
							c.m_iSoldiers = 0;
					}
					string cw;
					float s = ChoiceScore(c, needs, cw);
					if (s > bestScore)
					{
						bestScore = s;
						best = c;
						bestWhy = cw;
					}
				}
				why = bestWhy;
				return best;
			}

			int total;
			foreach (DCO_SpawnerGroupEntry e : m_aGroups)
			{
				if (Usable(e))
					total += e.m_iWeight;
			}
			if (total <= 0)
				return null;

			int roll = Math.RandomInt(0, total);
			foreach (DCO_SpawnerGroupEntry e2 : m_aGroups)
			{
				if (!Usable(e2))
					continue;
				roll -= e2.m_iWeight;
				if (roll < 0)
					return e2;
			}
			return null;
		}

		why = "catalog";

		SCR_Faction scrFaction = SCR_Faction.Cast(faction);
		if (!scrFaction)
			return null;
		SCR_EntityCatalog catalog = scrFaction.GetFactionEntityCatalogOfType(EEntityCatalogType.GROUP, false);
		if (!catalog)
			return null;
		array<SCR_EntityCatalogEntry> list = {};
		catalog.GetEntityList(list);
		if (list.IsEmpty())
			return null;

		DCO_SpawnerGroupEntry pick = new DCO_SpawnerGroupEntry();
		pick.m_sPrefab = list.GetRandomElement().GetPrefab();
		pick.m_iWeight = 1;
		return pick;
	}

	protected static int PrefabSoldiers(Resource res)
	{
		IEntitySource src = res.GetResource().ToEntitySource();
		array<ResourceName> slots = {};
		if (src && src.Get("m_aUnitPrefabSlots", slots) && !slots.IsEmpty())
			return slots.Count();
		return 1;
	}

	protected bool SpawnGroup(AICommander_BaseComponent cmd, DCO_SpawnerGroupEntry entry, Resource res, int soldiers)
	{
		vector origin = GetOwner().GetOrigin();
		vector pos;
		if (!SCR_WorldTools.FindEmptyTerrainPosition(pos, origin, m_fSpawnRadius, 2, 2))
			pos = origin;

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		params.Transform[3] = pos;
		SCR_AIGroup grp = SCR_AIGroup.Cast(GetGame().SpawnEntityPrefab(res, GetOwner().GetWorld(), params));
		if (!grp)
			return false;

		IEntity vehicle;
		if (!entry.m_sVehicle.IsEmpty())
		{
			Resource vres = Resource.Load(entry.m_sVehicle);
			vector vpos;
			if (!SCR_WorldTools.FindEmptyTerrainPosition(vpos, pos, 25, 4, 3))
				vpos = pos + Vector(8, 0, 0);
			if (vres && vres.IsValid())
			{
				EntitySpawnParams vparams = new EntitySpawnParams();
				vparams.TransformMode = ETransformMode.WORLD;
				vparams.Transform[3] = vpos;
				vehicle = GetGame().SpawnEntityPrefab(vres, GetOwner().GetWorld(), vparams);
			}
		}

		DCO_BenchmarkLoggerComponent.Event(string.Format("spawner_spawn spawner=%1 cmd=%2 group=%3 prefab=%4 soldiers=%5 vehicle=%6 trigger=%7 stock=%8",
			GetOwner().GetName(), cmd.GetCommanderUID(), grp, entry.m_sPrefab, soldiers, vehicle != null, m_sPending, cmd.GetReinforcementStock()));
		GetGame().GetCallqueue().CallLater(AssignLater, 500, false, grp, cmd.GetCommanderUID(), vehicle);
		return true;
	}

	protected void AssignLater(SCR_AIGroup grp, string uid, IEntity vehicle)
	{
		if (!grp)
			return;

		DCO_GroupUtilityComponent util = DCO_GroupUtilityComponent.Cast(grp.FindComponent(DCO_GroupUtilityComponent));
		if (!util)
		{
			DCO_BenchmarkLoggerComponent.Event(string.Format("spawner_assign group=%1 cmd=%2 ok=0 reason=no_utility", grp, uid));
			return;
		}

		if (vehicle)
			util.DCO_AddSpawnVehicle(vehicle);
		util.DCO_AssignFromSpawner(uid);
		bool ok = util.GetMyCommander() != null;
		string how = "direct";
		if (!ok)
			how = "retry";
		DCO_BenchmarkLoggerComponent.Event(string.Format("spawner_assign group=%1 cmd=%2 ok=%3 mode=%4", grp, uid, ok, how));
	}

	protected static float Now()
	{
		return GetGame().GetWorld().GetWorldTime() / 1000.0;
	}
}

[BaseContainerProps(), SCR_BaseContainerCustomTitleUIInfo("m_Info")]
class DCO_SpawnerSpawnNowContextAction : SCR_SelectedEntitiesContextAction
{
	override int GetParam()
	{
		return GetGame().GetPlayerController().GetPlayerId();
	}

	override bool CanBeShown(SCR_EditableEntityComponent selectedEntity, vector cursorWorldPosition, int flags)
	{
		return selectedEntity && selectedEntity.GetOwner() && selectedEntity.GetOwner().FindComponent(DCO_CommanderSpawnerComponent) != null;
	}

	override bool CanBePerformed(SCR_EditableEntityComponent selectedEntity, vector cursorWorldPosition, int flags)
	{
		return CanBeShown(selectedEntity, cursorWorldPosition, flags);
	}

	override void Perform(SCR_EditableEntityComponent hoveredEntity, notnull set<SCR_EditableEntityComponent> selectedEntities, vector cursorWorldPosition, int flags, int param = -1)
	{
		foreach (SCR_EditableEntityComponent entity : selectedEntities)
		{
			if (!entity || !entity.GetOwner())
				continue;
			DCO_CommanderSpawnerComponent spawner = DCO_CommanderSpawnerComponent.Cast(entity.GetOwner().FindComponent(DCO_CommanderSpawnerComponent));
			if (spawner)
				spawner.SpawnNow(param);
		}
	}
}
