//! Tujuan relocate. Nentuin syarat keras dan bobot scoring di DCO_FindIndoorRelocatePosition.
enum DCO_EIndoorRelocateIntent
{
	RELOCATE_DEFEND,	//! Habis kena hit: jangan kelihatan dari ancaman, idealnya masih bisa ngintip. Jangan mendekat ke ancaman.
	FIRE_POSITION		//! Suppression: WAJIB bisa lihat area target dari ketinggian intip. Cover jadi nilai tambah.
}

class DCO_AICombatMoveRequest_IndoorRelocate : SCR_AICombatMoveRequest_Move
{
	//! Gedung tempat unit berada. Weak ref -- gedung bukan milik request.
	IEntity m_Building;

	//! Kandidat lebih deket dari ini ke posisi awal ditolak (harus beneran pindah). 0 = boleh tetap di tempat.
	float m_fMinMoveDist = 2.5;

	//! Posisi unit waktu request mulai dieksekusi. Diisi node pencarian di BT
	//! (tick pertama dia lihat request ini), bukan di state -- state gak pegang entity.
	vector m_vStartPos;

	//! Tujuan relocate. m_vTargetPos = posisi ancaman (DEFEND) atau pusat area target (FIRE_POSITION).
	DCO_EIndoorRelocateIntent m_eIntent = DCO_EIndoorRelocateIntent.RELOCATE_DEFEND;

	//------------------------------------------------------------------------------------------------
	void DCO_AICombatMoveRequest_IndoorRelocate()
	{
		m_eType = SCR_EAICombatMoveRequestType.INDOOR_RELOCATE;
	}
}

// Helper DCO_ApplyIndoorRelocate() ada di Modded_CombatMoveState.c.