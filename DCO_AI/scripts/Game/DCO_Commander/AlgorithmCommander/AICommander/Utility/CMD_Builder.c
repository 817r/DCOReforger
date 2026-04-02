enum CMD_EFOBType
{
	FORWARD   = 0,   // FOB kecil: spawn point + MG + bunker
	FIREBASE  = 1,   // FOB menengah: + supply depot + mortar
	MAIN_BASE = 2,    // FOB besar: + vehicle depot + medical
	MORTAR_BASE = 3
}

enum CMD_EBuildState
{
	IDLE      = 0,
	BUILDING  = 1,
	COMPLETE  = 2,
	FAILED    = 3
}

// ----------------------------------------------------------------
//  CMD_FOBStructure — satu entry bangunan dalam layout
// ----------------------------------------------------------------

class CMD_FOBStructure
{
	ResourceName m_sPrefab;
	vector       m_vOffset;      // offset dari center FOB
	float        m_fYawDegrees;  // rotasi horizontal
	string       m_sLabel;

	void CMD_FOBStructure(ResourceName prefab, vector offset, float yaw, string label)
	{
		m_sPrefab      = prefab;
		m_vOffset      = offset;
		m_fYawDegrees  = yaw;
		m_sLabel       = label;
	}
}

class CMD_FOBBuildJob
{
	DCO_GroupUtilityComponent       m_BuilderGroup;
	vector                          m_vCenter;
	CMD_EFOBType                    m_eFOBType;
	AICommander_BaseComponent       m_Commander;
	CMD_EBuildState                 m_eState        = CMD_EBuildState.IDLE;
	int                             m_iNextIndex    = 0;
	float                           m_fNextBuildTime = 0.0;
	ref array<IEntity>              m_aBuiltEntities = {};

	static float BUILD_INTERVAL = 15.0;

	void CMD_FOBBuildJob(
		DCO_GroupUtilityComponent   grp,
		vector                      center,
		CMD_EFOBType                fobType,
		AICommander_BaseComponent   commander)
	{
		m_BuilderGroup = grp;
		m_vCenter      = center;
		m_eFOBType     = fobType;
		m_Commander    = commander;
	}

	bool IsComplete() { return m_eState == CMD_EBuildState.COMPLETE || m_eState == CMD_EBuildState.FAILED; }
}

// ----------------------------------------------------------------
//  CMD_FOBBuilderManagerComponent
//  Ditaruh di entitas Commander (sama dengan AICommander_BaseComponent).
//  Tick setiap frame untuk proses build queue.
// ----------------------------------------------------------------

[ComponentEditorProps(category: "GameScripted/Commander", description: "Manages FOB construction jobs")]
class CMD_FOBBuilderManagerComponentClass : ScriptComponentClass {}

class CMD_FOBBuilderManagerComponent : ScriptComponent
{
	protected ref array<CMD_FOBBuildJob> m_aJobs = {};

	// ----------------------------------------------------------------
	//  StartFOB — dipanggil Commander, kick off build job
	// ----------------------------------------------------------------

	void StartFOB(
		DCO_GroupUtilityComponent grp,
		vector                    center,
		CMD_EFOBType              fobType,
		AICommander_BaseComponent commander,
		float                     worldTime)
	{
		if (!grp || !commander)
			return;

		CMD_FOBBuildJob job = new CMD_FOBBuildJob(grp, center, fobType, commander);
		job.m_eState         = CMD_EBuildState.BUILDING;
		job.m_fNextBuildTime = worldTime + 3.0; // grace period setelah group tiba

		m_aJobs.Insert(job);

		// Kirim group ke lokasi FOB
		SCR_AIWaypoint wp = commander.SpawnMoveWP(center);
		if (wp)
			grp.MoveTo(wp, worldTime);

		grp.SetGroupRole(CMD_EGroupRole.NONE);

		Print(string.Format("[CMD_FOB] Build job started at %1 | type: %2 | builder: %3",
			center.ToString(),
			typename.EnumToString(CMD_EFOBType, fobType),
			grp.GetOwner().GetName()));
	}

	// ----------------------------------------------------------------
	//  Tick — proses semua job aktif
	// ----------------------------------------------------------------

	protected void Tick(float worldTime)
	{
		int i = 0;
		while (i < m_aJobs.Count())
		{
			CMD_FOBBuildJob job = m_aJobs[i];

			if (!job || job.IsComplete())
			{
				m_aJobs.Remove(i);
				continue;
			}

			TickJob(job, worldTime);
			i = i + 1;
		}
	}

	protected void TickJob(CMD_FOBBuildJob job, float worldTime)
	{
		if (worldTime < job.m_fNextBuildTime)
			return;

		if (!job.m_BuilderGroup)
		{
			job.m_eState = CMD_EBuildState.FAILED;
			return;
		}

		array<CMD_FOBStructure> layout = CMD_FOBBuilder.GetLayout(job.m_eFOBType);

		if (job.m_iNextIndex >= layout.Count())
		{
			job.m_eState = CMD_EBuildState.COMPLETE;

			Print(string.Format("[CMD_FOB] FOB complete at %1 — %2 structures built",
				job.m_vCenter.ToString(), job.m_aBuiltEntities.Count()));

			// Group selesai, kembalikan ke IDLE
			job.m_BuilderGroup.SetGroupStatus(DCOG_EGroupStatus.IDLE);
			job.m_BuilderGroup.SetGroupRole(CMD_EGroupRole.NONE);
			return;
		}

		CMD_FOBStructure structure = layout[job.m_iNextIndex];

		// Hitung posisi + rotasi world space
		vector worldPos = job.m_vCenter + structure.m_vOffset;
		worldPos[1]     = GetGame().GetWorld().GetSurfaceY(worldPos[0], worldPos[2]);

		IEntity built = SpawnStructure(structure.m_sPrefab, worldPos, structure.m_fYawDegrees);

		if (built)
		{
			job.m_aBuiltEntities.Insert(built);
			Print(string.Format("[CMD_FOB] Built: %1 at %2 (%3/%4)",
				structure.m_sLabel,
				worldPos.ToString(),
				job.m_iNextIndex + 1,
				layout.Count()));
		}
		else
		{
			Print(string.Format("[CMD_FOB] FAILED to spawn: %1", structure.m_sLabel), LogLevel.WARNING);
		}

		job.m_iNextIndex      = job.m_iNextIndex + 1;
		job.m_fNextBuildTime  = worldTime + CMD_FOBBuildJob.BUILD_INTERVAL;
	}

	// ----------------------------------------------------------------
	//  SpawnStructure — spawn satu prefab dengan orientasi
	// ----------------------------------------------------------------

	protected IEntity SpawnStructure(ResourceName prefab, vector pos, float yawDeg)
	{
		if (prefab == string.Empty)
			return null;

		Resource res = Resource.Load(prefab);
		if (!res || !res.IsValid())
			return null;

		EntitySpawnParams params = EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;

		vector angles = Vector(0.0, yawDeg, 0.0);
		Math3D.AnglesToMatrix(angles, params.Transform);
		params.Transform[3] = pos;

		return GetGame().SpawnEntityPrefab(res, null, params);
	}

	// ----------------------------------------------------------------
	//  Getters
	// ----------------------------------------------------------------

	int GetActiveJobCount() { return m_aJobs.Count(); }

	bool HasActiveJobAt(vector pos, float mergeRadius)
	{
		foreach (CMD_FOBBuildJob job : m_aJobs)
		{
			if (!job || job.IsComplete())
				continue;
			if (vector.Distance(job.m_vCenter, pos) <= mergeRadius)
				return true;
		}
		return false;
	}

	// ----------------------------------------------------------------
	//  FRAME
	// ----------------------------------------------------------------

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (!Replication.IsServer())
			return;

		if (m_aJobs.IsEmpty())
			return;

		float worldTime = GetGame().GetWorld().GetWorldTime() / 1000.0;
		Tick(worldTime);
	}

	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.FRAME);
	}
}

// ----------------------------------------------------------------
//  CMD_FOBBuilder — static utility
//  GetLayout mendefinisikan struktur FOB per tipe.
//
//  PENTING: Ganti ResourceName GUID dengan prefab yang ada
//           di project kamu. Ini placeholder.
// ----------------------------------------------------------------

class CMD_FOBBuilder
{
	// Radius minimum antar FOB — cegah double-build di lokasi sama
	static float FOB_MERGE_RADIUS = 200.0;

	// ----------------------------------------------------------------
	//  BuildFOB — entry point dari Commander
	// ----------------------------------------------------------------

	static void BuildFOB(
		DCO_GroupUtilityComponent   builderGroup,
		vector                      center,
		CMD_EFOBType                fobType,
		AICommander_BaseComponent   commander,
		float                       worldTime)
	{
		if (!Replication.IsServer())
			return;

		if (!builderGroup || !commander)
			return;

		// Cek apakah sudah ada FOB di sini
		CMD_FOBBuilderManagerComponent mgr = CMD_FOBBuilderManagerComponent.Cast(
			commander.GetOwner().FindComponent(CMD_FOBBuilderManagerComponent));

		if (!mgr)
		{
			Print("[CMD_FOB] ERROR: CMD_FOBBuilderManagerComponent tidak ada di Commander entity!", LogLevel.ERROR);
			return;
		}

		if (mgr.HasActiveJobAt(center, FOB_MERGE_RADIUS))
		{
			Print(string.Format("[CMD_FOB] FOB sudah ada di radius %1m dari %2 — skip",
				FOB_MERGE_RADIUS.ToString(), center.ToString()));
			return;
		}

		mgr.StartFOB(builderGroup, center, fobType, commander, worldTime);
	}

	// ----------------------------------------------------------------
	//  GetLayout — define semua struktur per tipe FOB
	//
	//  Offset: X = kanan, Y = up (di-snap ke terrain), Z = depan
	//  Ganti GUID dengan prefab project kamu.
	// ----------------------------------------------------------------

	static array<CMD_FOBStructure> GetLayout(CMD_EFOBType fobType)
	{
		array<CMD_FOBStructure> layout = {};

		// ── SHARED: Spawn point selalu ada di semua FOB ──
		layout.Insert(new CMD_FOBStructure(
			"{F3CFD244E6E2B5A1}Prefabs/Structures/Military/SpawnPoint_US.et",
			Vector(0.0, 0.0, 0.0), 0.0, "Spawn Point"));

		// ── FORWARD FOB ──
		if (fobType == CMD_EFOBType.FORWARD || fobType == CMD_EFOBType.FIREBASE || fobType == CMD_EFOBType.MAIN_BASE)
		{
			layout.Insert(new CMD_FOBStructure(
				"{6EE76E2B1F9C87A3}Prefabs/Structures/Military/Fortification/Bunker_Small.et",
				Vector(8.0, 0.0, 8.0), 45.0, "Bunker NE"));

			layout.Insert(new CMD_FOBStructure(
				"{6EE76E2B1F9C87A3}Prefabs/Structures/Military/Fortification/Bunker_Small.et",
				Vector(-8.0, 0.0, 8.0), 315.0, "Bunker NW"));

			layout.Insert(new CMD_FOBStructure(
				"{A1B2C3D4E5F60001}Prefabs/Structures/Military/Fortification/SandbagWall.et",
				Vector(0.0, 0.0, 12.0), 0.0, "Sandbag Wall N"));

			layout.Insert(new CMD_FOBStructure(
				"{A1B2C3D4E5F60002}Prefabs/Weapons/Emplaced/M2HB_Emplacement.et",
				Vector(0.0, 0.0, -10.0), 180.0, "MG Nest"));
		}

		// ── FIREBASE tambahan ──
		if (fobType == CMD_EFOBType.FIREBASE || fobType == CMD_EFOBType.MAIN_BASE)
		{
			layout.Insert(new CMD_FOBStructure(
				"{B2C3D4E5F6A70003}Prefabs/Structures/Military/SupplyDepot.et",
				Vector(-15.0, 0.0, 0.0), 90.0, "Supply Depot"));

			layout.Insert(new CMD_FOBStructure(
				"{A1B2C3D4E5F60003}Prefabs/Structures/Military/Fortification/SandbagWall.et",
				Vector(15.0, 0.0, 0.0), 270.0, "Sandbag Wall E"));

			layout.Insert(new CMD_FOBStructure(
				"{C3D4E5F6A7B80004}Prefabs/Weapons/Emplaced/Mortar_Nest.et",
				Vector(0.0, 0.0, 15.0), 0.0, "Mortar Pit"));
		}

		// ── MAIN BASE tambahan ──
		if (fobType == CMD_EFOBType.MAIN_BASE)
		{
			layout.Insert(new CMD_FOBStructure(
				"{D4E5F6A7B8C90005}Prefabs/Structures/Military/VehicleDepot.et",
				Vector(-25.0, 0.0, 10.0), 0.0, "Vehicle Depot"));

			layout.Insert(new CMD_FOBStructure(
				"{E5F6A7B8C9D00006}Prefabs/Structures/Military/MedicalTent.et",
				Vector(25.0, 0.0, 10.0), 0.0, "Medical Tent"));

			layout.Insert(new CMD_FOBStructure(
				"{A1B2C3D4E5F60001}Prefabs/Structures/Military/Fortification/SandbagWall.et",
				Vector(0.0, 0.0, -20.0), 180.0, "Sandbag Wall S"));

			layout.Insert(new CMD_FOBStructure(
				"{A1B2C3D4E5F60001}Prefabs/Structures/Military/Fortification/SandbagWall.et",
				Vector(-20.0, 0.0, 0.0), 90.0, "Sandbag Wall W"));
		}

		return layout;
	}
}