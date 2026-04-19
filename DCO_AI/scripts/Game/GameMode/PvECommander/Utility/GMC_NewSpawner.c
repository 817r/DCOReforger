[EntityEditorProps(category: "GameScripted/Spawner", description: "Player-Scaled Random Group Spawner (With Safe Radius)")]
class SCR_PlayerScaledGroupSpawnerClass : ScriptComponentClass
{
}

class SCR_PlayerScaledGroupSpawner : ScriptComponent
{
    [Attribute("", UIWidgets.ResourceAssignArray, desc: "Daftar Config/Prefab Group yang mau di-spawn", params: "et")]
    ref array<ResourceName> m_aGroupPrefabs;

    [Attribute("10", UIWidgets.EditBox, desc: "Jeda waktu ngecek & spawn (dalam detik)")]
    float m_fSpawnInterval;

    [Attribute("2", UIWidgets.EditBox, desc: "Minimal Group (Batas Bawah)")]
    int m_iMinGroups;

    [Attribute("15", UIWidgets.EditBox, desc: "Maksimal Group (Batas Atas)")]
    int m_iMaxGroups;

    [Attribute("1.0", UIWidgets.EditBox, desc: "Multiplier: Pengali jumlah grup per 1 player")]
    float m_fGroupsPerPlayer;
	
    [Attribute("5.0", UIWidgets.EditBox, desc: "Radius Spawn di dekat point")]
    float m_fSpawnRadius;
	
    [Attribute("300.0", UIWidgets.EditBox, desc: "Safe Radius: Jarak minimal player agar tidak terjadi spawn (meter)")]
    float m_fSafeRadius;

    [Attribute("false", UIWidgets.CheckBox, desc: "Invert Scaling? (Centang = Makin banyak player, makin dikit grup AI)")]
    bool m_bInvertScaling;

    protected ref array<IEntity> m_aSpawnedGroups = {};
	
    override void OnPostInit(IEntity owner)
    {
        super.OnPostInit(owner);
        SetEventMask(owner, EntityEvent.INIT);
    }

    override void EOnInit(IEntity owner)
    {
        GetGame().GetCallqueue().CallLater(CheckAndSpawn, m_fSpawnInterval * 1000, true);
        GetGame().GetCallqueue().CallLater(CheckAndSpawn, 2 * 1000, false);
    }

    void CheckAndSpawn()
    {
        if (!Replication.IsServer())
            return;
		
        CleanUpDeadGroups();

        if (m_fSafeRadius > 0 && IsPlayerNearby())
        {
            return; 
        }

        int playerCount = 1;
        PlayerManager playerManager = GetGame().GetPlayerManager();
        if (playerManager)
        {
            playerCount = playerManager.GetPlayerCount();
        }

        int targetGroups;
        
        if (m_bInvertScaling)
        {
            targetGroups = m_iMaxGroups - Math.Round(playerCount * m_fGroupsPerPlayer);
        }
        else
        {
            targetGroups = m_iMinGroups + Math.Round(playerCount * m_fGroupsPerPlayer);
        }
        
        targetGroups = Math.ClampInt(targetGroups, m_iMinGroups, m_iMaxGroups);

        if (m_aSpawnedGroups.Count() >= targetGroups)
            return;
		
        SpawnRandomGroup(targetGroups);
    }

    bool IsPlayerNearby()
    {
        PlayerManager playerManager = GetGame().GetPlayerManager();
        if (!playerManager) return false;

        array<int> players = {};
        playerManager.GetPlayers(players);
        vector myPos = GetOwner().GetOrigin();

        foreach (int playerId : players)
        {
            IEntity playerEnt = playerManager.GetPlayerControlledEntity(playerId);
            if (playerEnt)
            {
                float dist = vector.Distance(playerEnt.GetOrigin(), myPos);
                
                if (dist <= m_fSafeRadius)
                {
                    return true; 
                }
            }
        }
        
        return false; 
    }

    void SpawnRandomGroup(int currentTarget)
    {
        if (!m_aGroupPrefabs || m_aGroupPrefabs.IsEmpty())
            return;

        int randomIndex = Math.RandomInt(0, m_aGroupPrefabs.Count());
        ResourceName selectedPrefab = m_aGroupPrefabs[randomIndex];

        Resource resource = Resource.Load(selectedPrefab);
        if (!resource || !resource.IsValid())
            return;
            
        RandomGenerator rand = new RandomGenerator();
	    
        EntitySpawnParams params = new EntitySpawnParams();
        params.TransformMode = ETransformMode.WORLD;
        
        vector as = rand.GenerateRandomPointInRadius(0, m_fSpawnRadius, GetOwner().GetOrigin(), false);
        
        // Fix: Pastikan nilai ketinggian daratan dimasukkan ke koordinat Y dari vektor `as`
        as[1] = GetGame().GetWorld().GetSurfaceY(as[0], as[2]); 
        
        params.Transform[3] = as;

        IEntity newGroup = GetGame().SpawnEntityPrefab(resource, null, params);
        if (newGroup)
        {
            m_aSpawnedGroups.Insert(newGroup);
        }
    }

    void CleanUpDeadGroups()
    {
        for (int i = m_aSpawnedGroups.Count() - 1; i >= 0; i--)
        {
            if (!m_aSpawnedGroups[i])
            {
                m_aSpawnedGroups.RemoveOrdered(i);
            }
        }
    }
    
    void ~SCR_PlayerScaledGroupSpawner()
    {
        if (GetGame() && GetGame().GetCallqueue())
            GetGame().GetCallqueue().Remove(CheckAndSpawn);
    }
}