[ComponentEditorProps(category: "AI/Cover", description: "Static-instance cover manager. Register vector positions, forget on release.")]
class SCR_CoverManagerComponentClass : ScriptComponentClass {}

class SCR_CoverManagerComponent : ScriptComponent
{
	// --------------------------------------------------------
	//  Static singleton instance
	// --------------------------------------------------------
	protected static SCR_CoverManagerComponent s_Instance;

	//! Retrieve the global singleton instance.
	static SCR_CoverManagerComponent GetInstance()
	{
		return s_Instance;
	}

	// --------------------------------------------------------
	//  Editable properties
	// --------------------------------------------------------

	//! Durasi maksimal booking sebelum di-auto-release (detik).
	[Attribute("30", UIWidgets.EditBox, "Max booking duration before auto-release (s)")]
	protected float m_fMaxBookingDuration;

	//! Tampilkan debug sphere di dunia.
	[Attribute("1", UIWidgets.CheckBox, "Show debug visualization")]
	protected bool m_bDebugDraw;

	// --------------------------------------------------------
	//  Internal state
	//
	//  m_mBookings: map dari IEntity (booker) ke vector (posisi)
	//  m_mBookTimes: map dari IEntity ke float (waktu booking)
	//
	//  Saat register  -> masuk ke m_mBookings
	//  Saat release   -> dihapus dari m_mBookings (posisi dilupakan)
	// --------------------------------------------------------
	protected ref map<IEntity, vector> m_mBookings;
	protected ref map<IEntity, float>  m_mBookTimes;

	// --------------------------------------------------------
	//  Lifecycle
	// --------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		m_mBookings  = new map<IEntity, vector>();
		m_mBookTimes = new map<IEntity, float>();

		s_Instance = this;

		SetEventMask(owner, EntityEvent.FRAME);
	}

	override void OnDelete(IEntity owner)
	{
		if (s_Instance == this)
			s_Instance = null;

		super.OnDelete(owner);
	}

	// --------------------------------------------------------
	//  Frame tick — auto-release booking yang sudah kadaluarsa
	// --------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		array<IEntity> toRelease = new array<IEntity>();

		foreach (IEntity agent, float bookedAt : m_mBookTimes)
		{
			float elapsed = (System.GetTickCount() - bookedAt) * 0.001;

			if (agent != null && elapsed >= m_fMaxBookingDuration)
				toRelease.Insert(agent);
				
		}

		foreach (IEntity agent : toRelease)
		{
			ForgetBooking(agent);
		}
	}

	// --------------------------------------------------------
	//  Public API
	// --------------------------------------------------------

	//! Daftarkan posisi cover untuk agent tertentu.
	//! Jika agent sudah punya booking, posisi lama diganti.
	//! Returns true jika berhasil.
	bool RegisterPosition(IEntity agent, vector position)
	{
		if (!agent)
		{
			return false;
		}

		m_mBookings.Set(agent, position);
		m_mBookTimes.Set(agent, System.GetTickCount());

		return true;
	}

	//! Lepaskan booking milik agent dan lupakan posisinya.
	//! Returns true jika ada booking yang ditemukan dan dihapus.
	bool ReleasePosition(IEntity agent)
	{
		if (!agent)
		{
			return false;
		}

		if (!m_mBookings.Contains(agent))
		{
			return false;
		}

		ForgetBooking(agent);

		return true;
	}
	
	static bool IsEntityInsideBuilding(IEntity entity, float traceDistance = 10.0)
	{
		if (!entity)
		{
			return false;
		}
			

		World world = GetGame().GetWorld();

		if (!world)
		{
			return false;
		}
			

		vector origin   = entity.GetOrigin();
		vector traceEnd = origin + Vector(0, traceDistance, 0);

		TraceParam trace  = new TraceParam();
		trace.Flags		  = TraceFlags.ENTS;
		trace.Start       = origin;
		trace.End         = traceEnd;
		trace.Exclude     = entity;

		float hitFraction = world.TraceMove(trace, null);

		if (hitFraction >= 1.0)
		{
			return false;
		}
			

		if (!trace.TraceEnt)
		{
			return false;
		}
			
		
		IEntity root = trace.TraceEnt.GetRootParent();

		DCO_BuildingPositionComponent buildingComp =
			DCO_BuildingPositionComponent.Cast(root.FindComponent(DCO_BuildingPositionComponent));

		if (!buildingComp)
		{
			return false;
		}

		return true;
	}

	//! Cari posisi cover terdekat yang belum dipakai,
	//! lalu daftarkan untuk agent ini secara otomatis.
	//! candidatePositions: daftar posisi yang tersedia di dunia.
	//! Returns posisi yang berhasil di-booking, atau vector.Zero jika gagal.
	vector BookNearestFreePosition(IEntity agent, array<vector> candidatePositions)
	{
		if (!agent)
			return vector.Zero;

		if (!candidatePositions || candidatePositions.IsEmpty())
			return vector.Zero;

		vector agentOrigin = agent.GetOrigin();
		float  bestDist    = float.MAX;
		vector bestPos     = vector.Zero;
		bool   found       = false;

		foreach (vector pos : candidatePositions)
		{
			if (IsPositionBooked(pos))
				continue;

			float dist = vector.Distance(agentOrigin, pos);

			if (dist < bestDist)
			{
				bestDist = dist;
				bestPos  = pos;
				found    = true;
			}
		}

		if (!found)
		{
			return vector.Zero;
		}

		RegisterPosition(agent, bestPos);
		return bestPos;
	}

	//! Cek apakah sebuah posisi (vector) sudah di-booking oleh siapapun.
	bool IsPositionBooked(vector position)
	{
		foreach (IEntity agent, vector bookedPos : m_mBookings)
		{
			if (bookedPos == position)
				return true;
		}
		return false;
	}

	//! Cek apakah agent tertentu saat ini punya booking aktif.
	bool HasActiveBooking(IEntity agent)
	{
		if (!agent)
			return false;

		return m_mBookings.Contains(agent);
	}

	//! Dapatkan posisi booking milik agent.
	//! Returns vector.Zero jika agent tidak punya booking.
	vector GetBookedPosition(IEntity agent)
	{
		if (!agent)
			return vector.Zero;

		if (!m_mBookings.Contains(agent))
			return vector.Zero;

		return m_mBookings.Get(agent);
	}

	//! Jumlah booking yang sedang aktif.
	int GetActiveBookingCount()
	{
		return m_mBookings.Count();
	}

	//! Lepaskan semua booking sekaligus (misal: saat clear/reset).
	void ReleaseAll()
	{
		m_mBookings.Clear();
		m_mBookTimes.Clear();
	}

	//! Cek jarak terdekat antara posisi A dengan semua posisi yang sedang di-booking,
	//! dihitung hanya pada sumbu XZ (Y diabaikan).
	//! Returns jarak XZ terdekat, atau -1 jika belum ada booking sama sekali.
	float GetNearestBookedDistanceXZ(vector positionA)
	{
		if (m_mBookings.IsEmpty())
			return -1;

		float nearestDist = float.MAX;

		foreach (IEntity agent, vector bookedPos : m_mBookings)
		{
			float dx   = positionA[0] - bookedPos[0];
			float dz   = positionA[2] - bookedPos[2];
			float dist = Math.Sqrt(dx * dx + dz * dz);

			if (dist < nearestDist)
				nearestDist = dist;
		}

		return nearestDist;
	}

	// --------------------------------------------------------
	//  Internal helpers
	// --------------------------------------------------------

	//! Hapus entri booking dari kedua map (posisi terlupakan).
	protected void ForgetBooking(IEntity agent)
	{
		m_mBookings.Remove(agent);
		m_mBookTimes.Remove(agent);
	}
}