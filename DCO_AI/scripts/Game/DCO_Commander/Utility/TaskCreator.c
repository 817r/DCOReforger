// CMD_TaskNotifier.c
// Kirim notifikasi task ke player:
//   1. SCR_PopUpNotification  — popup di layar
//   2. SCR_MapMarkerEntity    — marker visual di map

enum CMD_ETaskType
{
	MOVE    = 0,
	CAPTURE = 1,
	DEFEND  = 2,
	DESTROY = 3,
	RECON   = 4
}

class CMD_TaskNotifier
{
	// ----------------------------------------------------------------
	//  Notify — entry point
	//
	//  targetGroup  — group yang dapat notifikasi
	//  title        — teks singkat di popup dan marker
	//  position     — posisi marker di map
	//  taskType     — tipe untuk icon marker
	// ----------------------------------------------------------------

	static void Notify(
		IEntity       targetGroup,
		string        title,
		vector        position,
		CMD_ETaskType taskType)
	{
		if (!Replication.IsServer())
			return;

		if (!targetGroup)
			return;

		// Spawn marker di map (server side, auto-replicated)
		SpawnMapMarker(position, title, taskType);

		// Kirim popup ke semua player di group
		array<int> playerIds = GetPlayersInGroup(targetGroup);
		foreach (int playerId : playerIds)
			SendPopup(playerId, title, taskType);
	}

	// ----------------------------------------------------------------
	//  SpawnMapMarker
	//
	//  Spawn SCR_MapMarkerEntity di world position.
	//  Entity ini auto-replicated ke semua client via RplComponent.
	// ----------------------------------------------------------------

	protected static void SpawnMapMarker(vector position, string label, CMD_ETaskType taskType)
	{
		Resource res = Resource.Load("{EC95FBEA75AE409B}Prefabs/Markers/MapMarkerDotCircle.et");
		if (!res || !res.IsValid())
		{
			Print("[CMD_TaskNotifier] MapMarker prefab tidak ditemukan", LogLevel.WARNING);
			return;
		}

		EntitySpawnParams params = EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		Math3D.MatrixIdentity4(params.Transform);
		params.Transform[3] = position;

		IEntity markerEnt = GetGame().SpawnEntityPrefab(res, null, params);
		if (!markerEnt)
			return;

		SCR_MapMarkerDotCircle marker = SCR_MapMarkerDotCircle.Cast(markerEnt);
		if (!marker)
			return;

		marker.SetText(label);
		marker.SetType(GetMarkerTypeForTask(taskType));
		marker.m_fRadius = 5;
		
	}

	// ----------------------------------------------------------------
	//  SendPopup
	//
	//  Kirim SCR_PopUpNotification ke satu player via
	//  SCR_NotificationsComponent di GameMode.
	// ----------------------------------------------------------------

	protected static void SendPopup(int playerId, string title, CMD_ETaskType taskType)
	{
		SCR_NotificationsComponent notifComp = SCR_NotificationsComponent.Cast(
			GetGame().GetGameMode().FindComponent(SCR_NotificationsComponent));

		if (!notifComp)
			return;

		notifComp.SendToPlayer(playerId, ENotification.GROUP_TASK_CREATED);
	}

	// ----------------------------------------------------------------
	//  GetPlayersInGroup — ambil semua player ID yang ada di group
	// ----------------------------------------------------------------

	protected static array<int> GetPlayersInGroup(IEntity groupEntity)
	{
		array<int> result = {};

		SCR_AIGroup grp = SCR_AIGroup.Cast(groupEntity);
		if (!grp)
			return result;

		array<AIAgent> agents = {};
		grp.GetAgents(agents);

		array<int> allPlayerIds = {};
		GetGame().GetPlayerManager().GetAllPlayers(allPlayerIds);

		foreach (AIAgent agent : agents)
		{
			IEntity controlled = agent.GetControlledEntity();
			if (!controlled)
				continue;

			foreach (int playerId : allPlayerIds)
			{
				if (GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId) == controlled)
				{
					result.Insert(playerId);
					break;
				}
			}
		}

		return result;
	}

	// ----------------------------------------------------------------
	//  Helpers
	// ----------------------------------------------------------------

	protected static SCR_EMapMarkerType GetMarkerTypeForTask(CMD_ETaskType t)
	{
		switch (t)
		{
			case CMD_ETaskType.CAPTURE: return SCR_EMapMarkerType.DOT_CIRCLE;
			case CMD_ETaskType.DEFEND:  return SCR_EMapMarkerType.DOT_CIRCLE;
			case CMD_ETaskType.DESTROY: return SCR_EMapMarkerType.DOT_CIRCLE;
			case CMD_ETaskType.RECON:   return SCR_EMapMarkerType.DOT_CIRCLE;
			default:                    return SCR_EMapMarkerType.DOT_CIRCLE;
		}
		return SCR_EMapMarkerType.DOT_CIRCLE;
	}
}