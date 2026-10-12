[EntityEditorProps(category: "GameScripted/Spawner", description: "Periodic Vehicle Spawner with Faction Override & Balancer")]
class GMC_VehicleFactionSpawnerClass : ScriptComponentClass {}

class GMC_VehicleFactionSpawner : ScriptComponent
{
    [Attribute("", UIWidgets.ResourceAssignArray, desc: "List of vehicle configs/prefabs to spawn.", params: "et")]
    ref array<ResourceName> m_aVehiclePrefabs;

    [Attribute("3", UIWidgets.EditBox, desc: "Maximum active vehicles on the map.")]
    int m_iMaxVehicles;

    [Attribute("60", UIWidgets.EditBox, desc: "Check and spawn interval (s).")]
    float m_fSpawnInterval;

    [Attribute("15", UIWidgets.EditBox, desc: "Radius Spawn (meter)")]
    float m_fSpawnRadius;

    [Attribute("", UIWidgets.EditBox, desc: "Faction key override (for example US, USSR, FIA). Leave empty to use the prefab default.")]
    string m_sFactionKey;

    [Attribute("300", UIWidgets.EditBox, desc: "Time (s) before a vehicle with no player nearby is deleted (0 = off).")]
    float m_fAbandonedTimeout;

    [Attribute("150", UIWidgets.EditBox, desc: "Minimum player distance to a vehicle for it not to count as stranded (m).")]
    float m_fMinPlayerDistance;

    protected ref array<IEntity> m_aSpawnedVehicles = {};

    protected ref map<int, float> m_mLastPlayerNearTime = new map<int, float>();

    override void OnPostInit(IEntity owner)
    {
        super.OnPostInit(owner);

        if (!Replication.IsServer())
            return;

        GetGame().GetCallqueue().CallLater(CheckAndSpawn, m_fSpawnInterval * 1000, true);
    }

    void CheckAndSpawn()
    {
        if (!Replication.IsServer())
            return;

        CleanUpDeadVehicles();

        if (m_fAbandonedTimeout > 0)
            CleanUpStrandedVehicles();

        if (m_aSpawnedVehicles.Count() >= m_iMaxVehicles)
            return;

        SpawnVehicle();
    }

    protected void CleanUpStrandedVehicles()
    {
        if (m_aSpawnedVehicles.IsEmpty())
            return;

        float currentTime = GetGame().GetWorld().GetWorldTime();

        for (int i = m_aSpawnedVehicles.Count() - 1; i >= 0; i--)
        {
            IEntity veh = m_aSpawnedVehicles[i];
            if (!veh)
                continue;

            bool playerNearby = IsAnyPlayerNear(veh);

            if (playerNearby)
            {
                m_mLastPlayerNearTime.Set(i, currentTime);
            }
            else
            {
                float lastNear;
                if (!m_mLastPlayerNearTime.Find(i, lastNear))
                {
                    m_mLastPlayerNearTime.Set(i, currentTime);
                    continue;
                }

                float elapsedSec = (currentTime - lastNear) / 1000.0;
                if (elapsedSec >= m_fAbandonedTimeout)
                {
                    SCR_EntityHelper.DeleteEntityAndChildren(veh);
                    m_aSpawnedVehicles.RemoveOrdered(i);
                    m_mLastPlayerNearTime.Remove(i);

                    RebuildPlayerNearTimeMap();
                }
            }
        }
    }

    protected bool IsAnyPlayerNear(IEntity veh)
    {
        if (!veh)
            return false;

        vector vehPos = veh.GetOrigin();
        float distSq  = m_fMinPlayerDistance * m_fMinPlayerDistance;

        array<int> playerIds = {};
        GetGame().GetPlayerManager().GetAllPlayers(playerIds);

        foreach (int playerId : playerIds)
        {
            IEntity playerEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
            if (!playerEntity)
                continue;

            float d = vector.DistanceSq(vehPos, playerEntity.GetOrigin());
            if (d <= distSq)
                return true;
        }

        return false;
    }

    protected void RebuildPlayerNearTimeMap()
    {
        map<int, float> newMap = new map<int, float>();
        m_mLastPlayerNearTime.Clear();
    }

    protected void SpawnVehicle()
    {
        if (!Replication.IsServer())
            return;

        if (!m_aVehiclePrefabs || m_aVehiclePrefabs.IsEmpty())
            return;

        int randomIndex = Math.RandomInt(0, m_aVehiclePrefabs.Count());
        ResourceName selectedPrefab = m_aVehiclePrefabs[randomIndex];
        Resource resource = Resource.Load(selectedPrefab);

        if (!resource || !resource.IsValid())
            return;

        EntitySpawnParams params = new EntitySpawnParams();
        params.TransformMode = ETransformMode.WORLD;

        vector centerPos = GetOwner().GetOrigin();
        vector spawnPos  = centerPos;

        if (m_fSpawnRadius > 0)
        {
            float randomAngle = Math.RandomFloat(0, Math.PI2);
            float randomDist  = Math.RandomFloat(0, m_fSpawnRadius);

            spawnPos[0] = centerPos[0] + (Math.Cos(randomAngle) * randomDist);
            spawnPos[2] = centerPos[2] + (Math.Sin(randomAngle) * randomDist);

            BaseWorld world = GetGame().GetWorld();
            if (world)
                spawnPos[1] = world.GetSurfaceY(spawnPos[0], spawnPos[2]);
        }

        params.Transform[3] = spawnPos;

        IEntity newVehicle = GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), params);
        if (!newVehicle)
            return;

        m_aSpawnedVehicles.Insert(newVehicle);

        if (m_sFactionKey != "")
        {
            FactionAffiliationComponent factionComp = FactionAffiliationComponent.Cast(newVehicle.FindComponent(FactionAffiliationComponent));
            if (factionComp)
            {
                FactionManager fm = GetGame().GetFactionManager();
                if (fm)
                {
                    Faction targetFaction = fm.GetFactionByKey(m_sFactionKey);
                    if (targetFaction)
                        factionComp.SetAffiliatedFaction(targetFaction);
                }
            }
        }
    }

    protected void CleanUpDeadVehicles()
    {
        if (m_aSpawnedVehicles.IsEmpty())
            return;

        bool removed = false;

        for (int i = m_aSpawnedVehicles.Count() - 1; i >= 0; i--)
        {
            IEntity veh = m_aSpawnedVehicles[i];

            if (!veh)
            {
                m_aSpawnedVehicles.RemoveOrdered(i);
                removed = true;
                continue;
            }

            DamageManagerComponent dmgComp = DamageManagerComponent.Cast(veh.FindComponent(DamageManagerComponent));
            if (dmgComp && dmgComp.GetState() == EDamageState.DESTROYED)
            {
                m_aSpawnedVehicles.RemoveOrdered(i);
                removed = true;
            }
        }

        if (removed)
            m_mLastPlayerNearTime.Clear();
    }

    override void OnDelete(IEntity owner)
    {
        if (Replication.IsServer())
        {
            if (GetGame() && GetGame().GetCallqueue())
                GetGame().GetCallqueue().Remove(CheckAndSpawn);
        }

        super.OnDelete(owner);
    }
}