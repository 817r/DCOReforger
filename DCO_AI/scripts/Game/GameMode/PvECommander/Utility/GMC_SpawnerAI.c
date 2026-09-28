[ComponentEditorProps(category: "GMC_MyMod/Spawner", description: "Spawns and respawns a configurable group of units")]
class GMC_RespawnerComponentClass : ScriptComponentClass {}

class GMC_RespawnerComponent : ScriptComponent
{
	[Attribute("", UIWidgets.Auto, "Daftar konfigurasi group (dipilih random tiap spawn)")]
	protected ref array<ref GMC_RespawnGroupConfig> m_aGroupConfigs;

	[Attribute("30.0", UIWidgets.EditBox, "Delay sebelum respawn (detik)")]
	protected float m_fRespawnDelay;

	[Attribute("5.0", UIWidgets.EditBox, "Radius spawn di sekitar entity ini (meter)")]
	protected float m_fSpawnRadius;

	[Attribute("1", UIWidgets.CheckBox, "Auto-respawn setelah semua unit mati?")]
	protected bool m_bAutoRespawn;

	[Attribute("0", UIWidgets.CheckBox, "Debug log?")]
	protected bool m_bDebugLog;

	protected SCR_AIGroup m_ActiveGroup;

	protected bool  m_bGroupWasPopulated = false;

	protected float m_fRespawnTimer   = 0.0;
	protected bool  m_bWaitingRespawn = false;

	protected ref ScriptInvoker m_OnGroupSpawned;
	protected ref ScriptInvoker m_OnGroupWiped;

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if (!Replication.IsServer()) return;

		if (!m_aGroupConfigs || m_aGroupConfigs.IsEmpty())
		{
			PrintFormat("[GMC_Respawner] WARNING: Tidak ada GroupConfig pada '%1'", owner.GetName());
			return;
		}

		SetEventMask(owner, EntityEvent.FRAME);

		GMC_SpawnBalancerComponent balancer = GMC_SpawnBalancerComponent.GetFrom(owner);
		if (balancer)
			balancer.GetOnBalanceChanged().Insert(OnBalanceChanged);

		SpawnGroup();
	}

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (!Replication.IsServer()) return;

		if (!m_bWaitingRespawn && m_ActiveGroup)
		{
			int agentCount = m_ActiveGroup.GetAgentsCount();

			if (!m_bGroupWasPopulated && agentCount > 0)
			{
				m_bGroupWasPopulated = true;
				DebugLog("Group populated: " + agentCount.ToString() + " agents");
			}

			if (m_bGroupWasPopulated && agentCount == 0)
			{
				m_ActiveGroup        = null;
				m_bGroupWasPopulated = false;

				if (m_OnGroupWiped) m_OnGroupWiped.Invoke();
				DebugLog("Semua unit mati.");

				if (m_bAutoRespawn)
				{
					m_fRespawnTimer   = m_fRespawnDelay;
					m_bWaitingRespawn = true;
					DebugLog(string.Format("Respawn dalam %.1f detik...", m_fRespawnDelay));
				}
			}
		}

		if (!m_bWaitingRespawn) return;

		m_fRespawnTimer -= timeSlice;
		if (m_fRespawnTimer <= 0.0)
		{
			m_bWaitingRespawn = false;
			SpawnGroup();
		}
	}

	override void OnDelete(IEntity owner)
	{
		super.OnDelete(owner);

		GMC_SpawnBalancerComponent balancer = GMC_SpawnBalancerComponent.GetFrom(owner);
		if (balancer)
			balancer.GetOnBalanceChanged().Remove(OnBalanceChanged);

		m_ActiveGroup = null;
	}

	void SpawnGroup()
	{
		if (!Replication.IsServer()) return;
		if (!m_aGroupConfigs || m_aGroupConfigs.IsEmpty()) return;

		GMC_RespawnGroupConfig cfg = m_aGroupConfigs[Math.RandomInt(0, m_aGroupConfigs.Count())];
		if (!cfg || cfg.m_sGroupPrefab == string.Empty)
		{
			Print("[GMC_Respawner] ERROR: Config atau prefab kosong.");
			return;
		}

		GMC_SpawnBalancerComponent balancer = GMC_SpawnBalancerComponent.GetFrom(GetOwner());
		if (balancer)
		{
			int budget = balancer.GetBalancedCount();
			DebugLog("Budget=" + budget.ToString() + " | " + balancer.GetStatusString());
			if (budget <= 0)
			{
				DebugLog("Budget 0, skip spawn.");
				return;
			}
		}

		float  angle    = Math.RandomFloat(0.0, 360.0);
		float  dist     = Math.RandomFloat(0.0, m_fSpawnRadius);
		vector spawnPos = GetOwner().GetOrigin() + vector.FromYaw(angle) * dist;

		EntitySpawnParams p = new EntitySpawnParams();
		p.TransformMode = ETransformMode.WORLD;
		Math3D.AnglesToMatrix(Vector(0, Math.RandomFloat(0.0, 360.0), 0), p.Transform);
		p.Transform[3] = spawnPos;

		Resource res = Resource.Load(cfg.m_sGroupPrefab);
		if (!res.IsValid())
		{
			PrintFormat("[GMC_Respawner] ERROR: Prefab tidak valid: %1", cfg.m_sGroupPrefab);
			return;
		}

		IEntity spawned = GetGame().SpawnEntityPrefab(res, null, p);
		if (!spawned)
		{
			PrintFormat("[GMC_Respawner] ERROR: Spawn gagal: %1", cfg.m_sGroupPrefab);
			return;
		}

		m_ActiveGroup        = SCR_AIGroup.Cast(spawned);
		m_bGroupWasPopulated = false;

		if (!m_ActiveGroup)
			Print("[GMC_Respawner] WARNING: Prefab bukan SCR_AIGroup, wipe detection tidak aktif.");

		DebugLog("SpawnGroup OK: " + cfg.m_sGroupPrefab);
		if (m_OnGroupSpawned) m_OnGroupSpawned.Invoke(0);
	}

	void ForceRespawn()
	{
		if (!Replication.IsServer()) return;
		m_bWaitingRespawn    = false;
		m_ActiveGroup        = null;
		m_bGroupWasPopulated = false;
		SpawnGroup();
	}

	int GetAliveCount()
	{
		if (!m_ActiveGroup) return 0;
		return m_ActiveGroup.GetAgentsCount();
	}

	ScriptInvoker GetOnGroupSpawned()
	{
		if (!m_OnGroupSpawned) m_OnGroupSpawned = new ScriptInvoker();
		return m_OnGroupSpawned;
	}

	ScriptInvoker GetOnGroupWiped()
	{
		if (!m_OnGroupWiped) m_OnGroupWiped = new ScriptInvoker();
		return m_OnGroupWiped;
	}

	static GMC_RespawnerComponent GetFrom(IEntity entity)
	{
		if (!entity) return null;
		return GMC_RespawnerComponent.Cast(entity.FindComponent(GMC_RespawnerComponent));
	}

	protected void OnBalanceChanged(int newCount, int playerCount)
	{
		DebugLog(string.Format("Balance berubah: count=%1 players=%2 → respawn ulang", newCount, playerCount));
		m_bWaitingRespawn    = false;
		m_ActiveGroup        = null;
		m_bGroupWasPopulated = false;
		SpawnGroup();
	}

	protected void DebugLog(string msg)
	{
		if (m_bDebugLog) PrintFormat("[GMC_Respawner] %1", msg);
	}
}