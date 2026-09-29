enum DCO_EGarrisonPhase
{
	SCAN,
	WAIT,
	HOLD,
	FALLBACK,
	RELEASED
}

class DCO_GarrisonAssignment
{
	AIAgent m_Agent;
	DCO_GarrisonBuilding m_Building;
	int m_iSlot;
	int m_iRank;
	float m_fBestDist = float.MAX;
	float m_fProgress_ms;
	bool m_bFlanking;
}

class DCO_GarrisonVacancy
{
	DCO_GarrisonBuilding m_Building;
	int m_iSlot;
	int m_iRank;
	float m_fReadyAt_ms;
}

class DCO_AIGarrisonActivity : SCR_AIActivityBase
{

	protected DCO_GarrisonWaypoint m_Waypoint;
	protected DCO_EGarrisonPhase m_ePhase = DCO_EGarrisonPhase.SCAN;
	protected float m_fNextUpdate_ms;
	protected float m_fWaitStart_ms;
	protected vector m_vThreatDir;

	protected ref array<DCO_GarrisonBuilding> m_aCandidates = {};
	protected ref array<DCO_GarrisonBuilding> m_aClaimed = {};
	protected ref array<DCO_GarrisonBuilding> m_aOrderBuilding = {};
	protected ref array<int> m_aOrderSlot = {};
	protected ref array<ref DCO_GarrisonAssignment> m_aAssigned = {};
	protected ref array<ref DCO_GarrisonVacancy> m_aVacancies = {};


	protected int m_iCasualties;
	protected float m_fFlankUntil_ms;
	protected float m_fFlankCooldown_ms;
	protected float m_fReleasedAt_ms;
	protected float m_fLastThreat_ms;
	protected float m_fSuppressedSince_ms = -1;

	void DCO_AIGarrisonActivity(SCR_AIGroupUtilityComponent utility, AIWaypoint relatedWaypoint)
	{
		DCO_GarrisonWaypoint waypoint = DCO_GarrisonWaypoint.Cast(relatedWaypoint);
		m_Waypoint = waypoint;
		m_sBehaviorTree = "AI/BehaviorTrees/Chimera/Group/DCO_ActivityGarrison.bt";
		SetPriority(PRIORITY_ACTIVITY_DEFEND);
		if (waypoint)
			m_fPriorityLevel.Init(this, waypoint.GetPriorityLevel());

		utility.DCO_SetGarrisonActivity(this);
	}

	bool IsActive()
	{
		return IsRunning() && m_ePhase != DCO_EGarrisonPhase.RELEASED;
	}

	protected bool IsRunning()
	{
		EAIActionState s = GetActionState();
		return s != EAIActionState.FAILED && s != EAIActionState.COMPLETED;
	}

	bool DCO_Reface(vector dir, bool upgradeMode)
	{
		if (!m_Waypoint || dir.LengthSq() < 0.01)
			return false;

		dir[1] = 0;
		dir.Normalize();
		bool bigTurn = vector.Dot(dir, m_vThreatDir) < 0.7;

		vector mat[4];
		Math3D.AnglesToMatrix(Vector(dir.ToYaw(), 0, 0), mat);
		mat[3] = m_Waypoint.GetOrigin();
		m_Waypoint.SetWorldTransform(mat);

		if (upgradeMode && m_Waypoint.GetMode() == DCO_EGarrisonMode.HOLD)
			m_Waypoint.SetMode(DCO_EGarrisonMode.DEFEND);

		m_vThreatDir = dir;
		if (!bigTurn || m_ePhase != DCO_EGarrisonPhase.HOLD || HasFreshThreat())
			return false;

		Release("reface", GetGame().GetWorld().GetWorldTime());
		SetPriority(PRIORITY_ACTIVITY_DEFEND);
		m_ePhase = DCO_EGarrisonPhase.SCAN;
		return true;
	}

	void ForceRelease()
	{
		if (IsActive())
			Release("manual", GetGame().GetWorld().GetWorldTime());
	}

	DCO_EGarrisonMode GetMode()
	{
		if (!m_Waypoint)
			return DCO_EGarrisonMode.HOLD;

		return m_Waypoint.GetMode();
	}

	void GetClaimedBuildings(notnull array<DCO_GarrisonBuilding> outBuildings)
	{
		outBuildings.Copy(m_aClaimed);
	}

	void Update(float now_ms)
	{
		if (!IsRunning() || now_ms < m_fNextUpdate_ms)
			return;

		m_fNextUpdate_ms = now_ms + 1000.0;

		DCO_GarrisonRegistry registry = DCO_GarrisonRegistry.GetInstance();
		if (!registry || !m_Waypoint)
			return;

		if (m_Waypoint.GetDebugDraw())
			DCO_GarrisonRegistry.s_bDebugDraw = true;

		switch (m_ePhase)
		{
			case DCO_EGarrisonPhase.SCAN: PhaseScan(registry, now_ms); break;
			case DCO_EGarrisonPhase.WAIT: PhaseWait(registry, now_ms); break;
			case DCO_EGarrisonPhase.HOLD: PhaseHold(registry, now_ms); PhaseMode(now_ms); break;
			case DCO_EGarrisonPhase.RELEASED: PhaseReleased(now_ms); break;
		}
	}

	protected void PhaseMode(float now_ms)
	{
		if (HasFreshThreat())
			m_fLastThreat_ms = now_ms;

		DCO_EGarrisonMode mode = GetMode();
		if (mode != DCO_EGarrisonMode.DEFEND && m_fFlankUntil_ms > 0)
			ReturnFlank(now_ms);

		if (mode == DCO_EGarrisonMode.RELEASE && ShouldRelease(now_ms))
			Release("trigger", now_ms);
		else if (mode == DCO_EGarrisonMode.DEFEND)
			UpdateFlank(now_ms);
	}

	protected void PhaseReleased(float now_ms)
	{
		if (HasFreshThreat())
			m_fLastThreat_ms = now_ms;

		if (!m_Waypoint.GetReturnAfterRelease())
			return;

		if (now_ms - m_fReleasedAt_ms < 60000.0 || now_ms - m_fLastThreat_ms < 60000.0)
			return;

		SetPriority(PRIORITY_ACTIVITY_DEFEND);
		m_ePhase = DCO_EGarrisonPhase.SCAN;
		DCO_BenchmarkLoggerComponent.Event(string.Format("garrison_return grp=%1", m_Utility.m_Owner));
	}

	protected bool ShouldRelease(float now_ms)
	{
		float pct = m_Waypoint.GetReleaseCasualtyPct();
		int total = m_iCasualties + m_aAssigned.Count();
		if (pct > 0 && total > 0 && m_iCasualties * 100.0 / total >= pct)
			return true;

		if (IsSuppressedFor(now_ms, m_Waypoint.GetReleaseSuppressedTime()))
			return true;

		float dist = m_Waypoint.GetReleaseEnemyDist();
		return dist > 0 && EnemyWithin(dist);
	}

	protected bool IsSuppressedFor(float now_ms, float time_s)
	{
		if (time_s <= 0 || m_aAssigned.IsEmpty())
		{
			m_fSuppressedSince_ms = -1;
			return false;
		}

		int threatened = 0;
		foreach (DCO_GarrisonAssignment a : m_aAssigned)
		{
			SCR_AIUtilityComponent utility;
			if (a.m_Agent)
				utility = SCR_AIUtilityComponent.Cast(a.m_Agent.FindComponent(SCR_AIUtilityComponent));
			if (utility && utility.m_ThreatSystem && utility.m_ThreatSystem.GetState() == EAIThreatState.THREATENED)
				threatened++;
		}

		if (threatened * 2 <= m_aAssigned.Count())
		{
			m_fSuppressedSince_ms = -1;
			return false;
		}

		if (m_fSuppressedSince_ms < 0)
			m_fSuppressedSince_ms = now_ms;

		return now_ms - m_fSuppressedSince_ms >= time_s * 1000;
	}

	protected void Release(string reason, float now_ms)
	{
		foreach (DCO_GarrisonAssignment a : m_aAssigned)
			SetAgentLeash(a.m_Agent, null);

		DCO_GarrisonRegistry registry = DCO_GarrisonRegistry.GetInstance();
		if (registry)
		{
			foreach (DCO_GarrisonBuilding b : m_aClaimed)
				registry.Release(b, m_Utility.m_Owner);
		}

		m_aClaimed.Clear();
		m_aAssigned.Clear();
		m_aVacancies.Clear();
		m_fFlankUntil_ms = 0;

		SendCancelMessagesToAllAgents();
		SetPriority(1);
		m_ePhase = DCO_EGarrisonPhase.RELEASED;
		m_fReleasedAt_ms = now_ms;
		DCO_BenchmarkLoggerComponent.Event(string.Format("garrison_release grp=%1 reason=%2 casualties=%3", m_Utility.m_Owner, reason, m_iCasualties));
	}

	protected void UpdateFlank(float now_ms)
	{
		vector threatPos;
		bool threat = FindThreatInLeash(threatPos);

		if (m_fFlankUntil_ms > 0)
		{
			if (!threat || now_ms > m_fFlankUntil_ms)
				ReturnFlank(now_ms);
			return;
		}

		int size = m_Waypoint.GetFlankTeamSize();
		if (!threat || size <= 0 || now_ms < m_fFlankCooldown_ms || IsThreatVisibleInLeash())
			return;

		size = Math.Min(size, m_aAssigned.Count() - 2);
		if (size <= 0)
			return;

		array<DCO_GarrisonAssignment> team = {};
		for (int pick = 0; pick < size; pick++)
		{
			DCO_GarrisonAssignment worst;
			foreach (DCO_GarrisonAssignment a : m_aAssigned)
			{
				if (!a.m_bFlanking && !team.Contains(a) && (!worst || a.m_iRank > worst.m_iRank))
					worst = a;
			}
			if (worst)
				team.Insert(worst);
		}

		vector center = m_Waypoint.GetOrigin();
		float leashR = m_Waypoint.GetCompletionRadius() + m_Waypoint.GetDefendLeashExtra();
		vector flankPos = ComputeFlankPoint(center, threatPos, leashR);

		foreach (DCO_GarrisonAssignment member : team)
		{
			SCR_AIUtilityComponent utility = SCR_AIUtilityComponent.Cast(member.m_Agent.FindComponent(SCR_AIUtilityComponent));
			if (!utility)
				continue;

			DCO_GarrisonHoldBehavior hold = DCO_GarrisonHoldBehavior.Find(utility, this);
			if (hold)
				hold.Fail();

			DCO_AIConfigComponent cfg = DCO_AIConfigComponent.Cast(member.m_Agent.FindComponent(DCO_AIConfigComponent));
			if (cfg)
				cfg.SetLeashRadius(center, leashR);

			member.m_bFlanking = true;
			utility.AddAction(new SCR_AIMoveIndividuallyBehavior(utility, this, flankPos, priorityLevel: m_fPriorityLevel.m_Value, radius: 8));
		}

		m_fFlankUntil_ms = now_ms + 60000.0;
		DCO_BenchmarkLoggerComponent.Event(string.Format("garrison_flank grp=%1 size=%2", m_Utility.m_Owner, team.Count()));
	}

	protected void ReturnFlank(float now_ms)
	{
		foreach (DCO_GarrisonAssignment a : m_aAssigned)
		{
			if (!a.m_bFlanking)
				continue;

			a.m_bFlanking = false;
			if (IsAgentUsable(a.m_Agent))
				SendSlot(a);
		}

		m_fFlankUntil_ms = 0;
		m_fFlankCooldown_ms = now_ms + 60000.0;
	}

	protected vector ComputeFlankPoint(vector center, vector threatPos, float leashR)
	{
		vector dir = threatPos - center;
		dir[1] = 0;
		float len = dir.Length();
		if (len < 1)
			dir = m_vThreatDir;
		else
			dir = dir * (1.0 / len);

		vector side = Vector(-dir[2], 0, dir[0]);
		if (Math.RandomFloat01() < 0.5)
			side = side * -1;

		vector p = threatPos + side * 40.0 - dir * 15.0;
		vector off = p - center;
		off[1] = 0;
		if (off.Length() > leashR * 0.9)
			p = center + off.Normalized() * leashR * 0.9;

		p[1] = GetGame().GetWorld().GetSurfaceY(p[0], p[2]);
		return p;
	}


	protected bool HasFreshThreat()
	{
		vector unused;
		return FindThreatCluster(-1, unused);
	}

	protected bool FindThreatInLeash(out vector threatPos)
	{
		return FindThreatCluster(m_Waypoint.GetCompletionRadius() + m_Waypoint.GetDefendLeashExtra(), threatPos);
	}

	protected bool FindThreatCluster(float radius, out vector threatPos)
	{
		SCR_AIGroupPerception perc = m_Utility.GetPercGroupComp();
		if (!perc)
			return false;

		vector center = m_Waypoint.GetOrigin();
		foreach (SCR_AIGroupTargetCluster c : perc.m_aTargetClusters)
		{
			if (!c.m_State || c.m_State.m_iCountAlive <= 0 || c.m_State.GetTimeSinceLastNewInformation() > 15.0)
				continue;

			vector pos = c.m_State.GetCenterPosition();
			if (radius >= 0 && vector.DistanceXZ(pos, center) > radius)
				continue;

			threatPos = pos;
			return true;
		}
		return false;
	}

	protected bool IsThreatVisibleInLeash()
	{
		SCR_AIGroupPerception perc = m_Utility.GetPercGroupComp();
		PerceptionManager pm = GetGame().GetPerceptionManager();
		if (!perc || !pm)
			return false;

		float pmNow = pm.GetTime();
		vector center = m_Waypoint.GetOrigin();
		float leashR = m_Waypoint.GetCompletionRadius() + m_Waypoint.GetDefendLeashExtra();
		foreach (SCR_AITargetInfo t : perc.m_aTargets)
		{
			if (!t || pmNow - t.m_fTimestamp > 3.0)
				continue;
			if (t.m_eCategory != EAITargetInfoCategory.DETECTED && t.m_eCategory != EAITargetInfoCategory.IDENTIFIED)
				continue;
			if (vector.DistanceXZ(t.m_vWorldPos, center) <= leashR)
				return true;
		}
		return false;
	}

	protected bool EnemyWithin(float dist)
	{
		SCR_AIGroupPerception perc = m_Utility.GetPercGroupComp();
		PerceptionManager pm = GetGame().GetPerceptionManager();
		if (!perc || !pm)
			return false;

		float pmNow = pm.GetTime();
		float distSq = dist * dist;
		foreach (SCR_AITargetInfo t : perc.m_aTargets)
		{
			if (!t || pmNow - t.m_fTimestamp > 10)
				continue;
			if (t.m_eCategory != EAITargetInfoCategory.DETECTED && t.m_eCategory != EAITargetInfoCategory.IDENTIFIED)
				continue;

			foreach (DCO_GarrisonAssignment a : m_aAssigned)
			{
				IEntity ent;
				if (a.m_Agent)
					ent = a.m_Agent.GetControlledEntity();
				if (ent && vector.DistanceSqXZ(ent.GetOrigin(), t.m_vWorldPos) <= distSq)
					return true;
			}
		}
		return false;
	}

	protected void PhaseScan(DCO_GarrisonRegistry registry, float now_ms)
	{
		m_vThreatDir = m_Waypoint.GetThreatDirection();
		vector center = m_Waypoint.GetOrigin();
		float radius = Math.Max(m_Waypoint.GetCompletionRadius(), 30);

		array<IEntity> found = {};
		registry.FindBuildings(center, radius, found);

		array<float> scores = {};
		array<IEntity> ordered = {};
		foreach (IEntity candidate : found)
		{
			float score = ScoreBuilding(candidate, center, radius);
			int at = ordered.Count();
			for (int i = 0; i < scores.Count(); i++)
			{
				if (score > scores[i])
				{
					at = i;
					break;
				}
			}
			ordered.InsertAt(candidate, at);
			scores.InsertAt(score, at);
		}

		m_aCandidates.Clear();
		foreach (IEntity e : ordered)
		{
			if (m_aCandidates.Count() >= 8)
				break;

			DCO_GarrisonBuilding b = registry.Request(e, 1);
			if (b && !registry.IsOwnedByOther(b, m_Utility.m_Owner))
				m_aCandidates.Insert(b);
		}

		m_fWaitStart_ms = now_ms;
		m_ePhase = DCO_EGarrisonPhase.WAIT;
	}

	protected float ScoreBuilding(IEntity e, vector center, float radius)
	{
		vector mins, maxs;
		e.GetBounds(mins, maxs);
		vector size = maxs - mins;

		float sizeScore = Math.Clamp(size[0] * size[2] / 300.0, 0, 1);
		float floorScore = Math.Clamp((size[1] / 3.0 - 1) / 2.0, 0, 1);

		vector to = e.CoordToParent((mins + maxs) * 0.5) - center;
		to[1] = 0;
		float dist = to.Length();
		float distScore = 1 - Math.Clamp(dist / radius, 0, 1);

		float threatScore = 0.5;
		if (dist > 1)
			threatScore = (vector.Dot(to.Normalized(), m_vThreatDir) + 1) * 0.5;

		return 0.3 * sizeScore + 0.15 * floorScore + 0.25 * threatScore + 0.3 * distScore;
	}

	protected void PhaseWait(DCO_GarrisonRegistry registry, float now_ms)
	{
		bool allReady = true;
		foreach (DCO_GarrisonBuilding b : m_aCandidates)
		{
			if (b && !b.m_bReady)
			{
				allReady = false;
				break;
			}
		}

		if (!allReady && now_ms - m_fWaitStart_ms < 30000.0)
			return;

		Allocate(registry, now_ms);
	}

	protected void Allocate(DCO_GarrisonRegistry registry, float now_ms)
	{
		array<AIAgent> agents = {};
		GetAvailableAgents(agents);

		int maxBuildings = m_Waypoint.GetMaxBuildings();
		if (maxBuildings <= 0)
			maxBuildings = 6;

		int capacity = 0;
		foreach (DCO_GarrisonBuilding b : m_aCandidates)
		{
			if (capacity >= agents.Count() || m_aClaimed.Count() >= maxBuildings)
				break;

			if (!b || !b.m_bReady || b.m_aSlots.IsEmpty())
				continue;

			if (!registry.Claim(b, m_Utility.m_Owner))
				continue;

			m_aClaimed.Insert(b);
			capacity += BuildingCap(b);
		}

		if (m_aClaimed.IsEmpty())
		{
			m_ePhase = DCO_EGarrisonPhase.FALLBACK;
			SetPriority(1);
			if (m_Waypoint.GetCurrentDefendPreset())
				m_Utility.AddAction(new SCR_AIDefendActivity(m_Utility, m_Waypoint, vector.Zero, priorityLevel: m_fPriorityLevel.m_Value));
			DCO_BenchmarkLoggerComponent.Event(string.Format("garrison_fallback grp=%1 reason=no_building", m_Utility.m_Owner));
			return;
		}

		BuildOrder();

		foreach (int rank, DCO_GarrisonBuilding orderB : m_aOrderBuilding)
		{
			if (agents.IsEmpty())
				break;

			int slotIdx = m_aOrderSlot[rank];
			AIAgent nearest = TakeNearest(agents, orderB.m_aSlots[slotIdx].m_vWorldPos);
			Assign(registry, nearest, orderB, slotIdx, rank);
		}

		m_ePhase = DCO_EGarrisonPhase.HOLD;
		DCO_BenchmarkLoggerComponent.Event(string.Format("garrison_start grp=%1 buildings=%2 slots=%3 assigned=%4 leftover=%5",
			m_Utility.m_Owner, m_aClaimed.Count(), m_aOrderBuilding.Count(), m_aAssigned.Count(), agents.Count()));
	}

	protected int BuildingCap(DCO_GarrisonBuilding b)
	{
		int usable = 0;
		foreach (DCO_GarrisonSlot s : b.m_aSlots)
		{
			if (!s.m_bUnreachable && (s.m_eType != DCO_EGarrisonSlotType.ROOF || m_Waypoint.GetAllowRoof()))
				usable++;
		}
		return Math.Max(1, Math.Ceil(usable * m_Waypoint.GetFillRatio()));
	}

	protected void BuildOrder()
	{
		m_aOrderBuilding.Clear();
		m_aOrderSlot.Clear();

		if (m_Waypoint.GetFillOrder() == DCO_EGarrisonFillOrder.WINDOWS_FIRST)
		{
			for (int t = DCO_EGarrisonSlotType.WINDOW; t <= DCO_EGarrisonSlotType.INTERIOR; t++)
			{
				foreach (DCO_GarrisonBuilding b : m_aClaimed)
					AppendType(b, t, BuildingCap(b));
			}
			return;
		}

		foreach (DCO_GarrisonBuilding claimed : m_aClaimed)
		{
			int cap = BuildingCap(claimed);
			for (int t2 = DCO_EGarrisonSlotType.WINDOW; t2 <= DCO_EGarrisonSlotType.INTERIOR; t2++)
				AppendType(claimed, t2, cap);
		}
	}

	protected void AppendType(DCO_GarrisonBuilding b, int t, int cap)
	{
		if (t == DCO_EGarrisonSlotType.ROOF && !m_Waypoint.GetAllowRoof())
			return;

		array<int> pool = {};
		foreach (int i, DCO_GarrisonSlot s : b.m_aSlots)
		{
			if (s.m_eType == t && !s.m_bUnreachable)
				pool.Insert(i);
		}

		float w = m_Waypoint.GetThreatWeight();
		array<vector> chosenDirs = {};
		foreach (int rank, DCO_GarrisonBuilding ob : m_aOrderBuilding)
		{
			if (ob == b)
				chosenDirs.Insert(b.m_aSlots[m_aOrderSlot[rank]].m_vWorldDir);
		}

		while (!pool.IsEmpty() && chosenDirs.Count() < cap)
		{
			int bestK = 0;
			float bestScore = -float.MAX;
			foreach (int k, int idx : pool)
			{
				DCO_GarrisonSlot cand = b.m_aSlots[idx];
				float score = cand.m_fScore * 0.2;
				if (t == DCO_EGarrisonSlotType.WINDOW || t == DCO_EGarrisonSlotType.ROOF)
				{
					float align = (vector.Dot(cand.m_vWorldDir, m_vThreatDir) + 1) * 0.5;
					float diversity = 1;
					foreach (vector d : chosenDirs)
						diversity = Math.Min(diversity, (1 - vector.Dot(d, cand.m_vWorldDir)) * 0.5);

					score += w * align + (1 - w) * diversity;
				}

				if (score > bestScore)
				{
					bestScore = score;
					bestK = k;
				}
			}

			int picked = pool[bestK];
			pool.Remove(bestK);
			m_aOrderBuilding.Insert(b);
			m_aOrderSlot.Insert(picked);
			chosenDirs.Insert(b.m_aSlots[picked].m_vWorldDir);
		}
	}

	protected void PhaseHold(DCO_GarrisonRegistry registry, float now_ms)
	{
		CheckUnreachable(registry, now_ms);

		for (int i = m_aAssigned.Count() - 1; i >= 0; i--)
		{
			DCO_GarrisonAssignment a = m_aAssigned[i];
			if (IsAgentUsable(a.m_Agent))
				continue;

			SetAgentLeash(a.m_Agent, null);
			if (IsAgentDead(a.m_Agent))
				m_iCasualties++;
			registry.Book(a.m_Building, a.m_iSlot, null);
			DCO_GarrisonVacancy v = new DCO_GarrisonVacancy();
			v.m_Building = a.m_Building;
			v.m_iSlot = a.m_iSlot;
			v.m_iRank = a.m_iRank;
			v.m_fReadyAt_ms = now_ms + m_Waypoint.RollRefillDelay_ms();
			m_aVacancies.Insert(v);
			m_aAssigned.Remove(i);
		}

		array<AIAgent> free = {};
		GetAvailableAgents(free);
		foreach (AIAgent agent : free)
		{
			int rank = FindFreeRank(registry);
			if (rank < 0)
				break;

			Assign(registry, agent, m_aOrderBuilding[rank], m_aOrderSlot[rank], rank);
		}

		for (int vi = m_aVacancies.Count() - 1; vi >= 0; vi--)
		{
			DCO_GarrisonVacancy vac = m_aVacancies[vi];
			if (now_ms < vac.m_fReadyAt_ms)
				continue;

			m_aVacancies.Remove(vi);
			if (!vac.m_Building || vac.m_Building.m_aBookedBy[vac.m_iSlot] || vac.m_Building.m_aSlots[vac.m_iSlot].m_bUnreachable)
				continue;

			Refill(registry, vac);
		}

		if (m_Waypoint.GetPullLoneToMain())
			PullLoneToMain(registry);
	}

	protected void CheckUnreachable(DCO_GarrisonRegistry registry, float now_ms)
	{
		foreach (DCO_GarrisonAssignment a : m_aAssigned)
		{
			IEntity ent;
			if (a.m_Agent && !a.m_bFlanking)
				ent = a.m_Agent.GetControlledEntity();
			if (!ent)
				continue;

			SCR_AIUtilityComponent utility = SCR_AIUtilityComponent.Cast(a.m_Agent.FindComponent(SCR_AIUtilityComponent));
			DCO_GarrisonHoldBehavior hold;
			if (utility)
				hold = DCO_GarrisonHoldBehavior.Cast(utility.GetCurrentAction());

			float d = vector.Distance(ent.GetOrigin(), a.m_Building.m_aSlots[a.m_iSlot].m_vWorldPos);
			if (!hold || d <= 1.5 || d < a.m_fBestDist - 1)
			{
				a.m_fBestDist = Math.Min(a.m_fBestDist, d);
				if (!hold || d <= 1.5)
					a.m_fBestDist = d;
				a.m_fProgress_ms = now_ms;
				continue;
			}

			if (now_ms - a.m_fProgress_ms < 20000.0)
				continue;

			registry.MarkUnreachable(a.m_Building, a.m_iSlot);
			int rank = FindFreeRank(registry);
			if (rank < 0)
				continue;

			a.m_Building = m_aOrderBuilding[rank];
			a.m_iSlot = m_aOrderSlot[rank];
			a.m_iRank = rank;
			registry.Book(a.m_Building, a.m_iSlot, a.m_Agent);
			SendSlot(a);
		}
	}

	protected void Refill(DCO_GarrisonRegistry registry, DCO_GarrisonVacancy vac)
	{
		DCO_GarrisonSlot target = vac.m_Building.m_aSlots[vac.m_iSlot];
		DCO_GarrisonAssignment best;
		float bestCost = float.MAX;

		foreach (DCO_GarrisonAssignment a : m_aAssigned)
		{
			if (a.m_iRank <= vac.m_iRank || !a.m_Agent || a.m_bFlanking)
				continue;

			IEntity ent = a.m_Agent.GetControlledEntity();
			if (!ent)
				continue;

			float cost = vector.Distance(ent.GetOrigin(), target.m_vWorldPos);
			if (a.m_Building != vac.m_Building)
				cost += 100;
			else if (a.m_Building.m_aSlots[a.m_iSlot].m_iFloor != target.m_iFloor)
				cost += 20;

			cost -= a.m_iRank * 0.5;

			if (cost < bestCost)
			{
				bestCost = cost;
				best = a;
			}
		}

		if (!best)
			return;

		registry.Book(best.m_Building, best.m_iSlot, null);
		best.m_Building = vac.m_Building;
		best.m_iSlot = vac.m_iSlot;
		best.m_iRank = vac.m_iRank;
		registry.Book(best.m_Building, best.m_iSlot, best.m_Agent);
		SendSlot(best);
	}

	protected void PullLoneToMain(DCO_GarrisonRegistry registry)
	{
		if (m_aClaimed.Count() < 2)
			return;

		DCO_GarrisonBuilding main = m_aClaimed[0];
		for (int bi = 1; bi < m_aClaimed.Count(); bi++)
		{
			DCO_GarrisonBuilding b = m_aClaimed[bi];
			DCO_GarrisonAssignment lone;
			int count = 0;
			foreach (DCO_GarrisonAssignment a : m_aAssigned)
			{
				if (a.m_Building == b && !a.m_bFlanking)
				{
					count++;
					lone = a;
				}
			}

			if (count != 1)
				continue;

			foreach (int rank, DCO_GarrisonBuilding ob : m_aOrderBuilding)
			{
				if (ob != main || main.m_aBookedBy[m_aOrderSlot[rank]] || main.m_aSlots[m_aOrderSlot[rank]].m_bUnreachable)
					continue;

				registry.Book(lone.m_Building, lone.m_iSlot, null);
				lone.m_Building = main;
				lone.m_iSlot = m_aOrderSlot[rank];
				lone.m_iRank = rank;
				registry.Book(main, lone.m_iSlot, lone.m_Agent);
				SendSlot(lone);
				break;
			}
		}
	}

	protected int FindFreeRank(DCO_GarrisonRegistry registry)
	{
		foreach (int rank, DCO_GarrisonBuilding b : m_aOrderBuilding)
		{
			if (b && !b.m_aBookedBy[m_aOrderSlot[rank]] && !b.m_aSlots[m_aOrderSlot[rank]].m_bUnreachable)
				return rank;
		}
		return -1;
	}

	protected void Assign(DCO_GarrisonRegistry registry, AIAgent agent, DCO_GarrisonBuilding b, int slotIdx, int rank)
	{
		if (!agent)
			return;

		DCO_GarrisonAssignment a = new DCO_GarrisonAssignment();
		a.m_Agent = agent;
		a.m_Building = b;
		a.m_iSlot = slotIdx;
		a.m_iRank = rank;
		m_aAssigned.Insert(a);
		registry.Book(b, slotIdx, agent);
		SendSlot(a);
	}

	protected void SendSlot(DCO_GarrisonAssignment a)
	{
		a.m_fBestDist = float.MAX;
		a.m_fProgress_ms = GetGame().GetWorld().GetWorldTime();

		SCR_AIUtilityComponent utility = SCR_AIUtilityComponent.Cast(a.m_Agent.FindComponent(SCR_AIUtilityComponent));
		if (!utility)
			return;

		SetAgentLeash(a.m_Agent, a.m_Building.m_Entity);

		SCR_CoverManagerComponent coverMgr = SCR_CoverManagerComponent.GetInstance();
		if (coverMgr && a.m_Agent.GetControlledEntity())
			coverMgr.RegisterPosition(a.m_Agent.GetControlledEntity(), a.m_Building.m_aSlots[a.m_iSlot].m_vWorldPos);

		DCO_GarrisonHoldBehavior hold = DCO_GarrisonHoldBehavior.Find(utility, this);
		if (hold)
		{
			hold.SetSlot(a.m_Building, a.m_iSlot);
			return;
		}

		hold = new DCO_GarrisonHoldBehavior(utility, this, a.m_Building, a.m_iSlot, m_fPriorityLevel.m_Value);
		utility.AddAction(hold);
	}

	protected void GetAvailableAgents(notnull array<AIAgent> outAgents)
	{
		array<AIAgent> agents = {};
		m_Utility.m_Owner.GetAgents(agents);
		foreach (AIAgent agent : agents)
		{
			if (!IsAgentUsable(agent))
				continue;

			ChimeraCharacter character = ChimeraCharacter.Cast(agent.GetControlledEntity());
			if (!character || character.IsInVehicle())
				continue;

			bool assigned = false;
			foreach (DCO_GarrisonAssignment a : m_aAssigned)
			{
				if (a.m_Agent == agent)
				{
					assigned = true;
					break;
				}
			}

			if (!assigned)
				outAgents.Insert(agent);
		}
	}

	protected bool IsAgentDead(AIAgent agent)
	{
		ChimeraCharacter character;
		if (agent)
			character = ChimeraCharacter.Cast(agent.GetControlledEntity());
		if (!character)
			return true;

		DamageManagerComponent dmg = character.GetDamageManager();
		return dmg && dmg.IsDestroyed();
	}

	protected bool IsAgentUsable(AIAgent agent)
	{
		if (!agent || agent.GetParentGroup() != m_Utility.m_Owner)
			return false;

		ChimeraCharacter character = ChimeraCharacter.Cast(agent.GetControlledEntity());
		if (!character)
			return false;

		DamageManagerComponent dmg = character.GetDamageManager();
		return !dmg || !dmg.IsDestroyed();
	}

	protected AIAgent TakeNearest(notnull array<AIAgent> agents, vector pos)
	{
		int best = -1;
		float bestDistSq = float.MAX;
		foreach (int i, AIAgent agent : agents)
		{
			IEntity ent = agent.GetControlledEntity();
			if (!ent)
				continue;

			float d = vector.DistanceSq(ent.GetOrigin(), pos);
			if (d < bestDistSq)
			{
				bestDistSq = d;
				best = i;
			}
		}

		if (best < 0)
			return null;

		AIAgent picked = agents[best];
		agents.Remove(best);
		return picked;
	}

	protected static void SetAgentLeash(AIAgent agent, IEntity building)
	{
		if (!agent)
			return;

		DCO_AIConfigComponent cfg = DCO_AIConfigComponent.Cast(agent.FindComponent(DCO_AIConfigComponent));
		if (cfg)
			cfg.SetLeashBuilding(building);
	}

	protected void ReleaseAll()
	{
		foreach (DCO_GarrisonAssignment a : m_aAssigned)
			SetAgentLeash(a.m_Agent, null);

		DCO_GarrisonRegistry registry = DCO_GarrisonRegistry.GetInstance();
		if (registry)
		{
			foreach (DCO_GarrisonBuilding b : m_aClaimed)
				registry.Release(b, m_Utility.m_Owner);
		}

		m_aClaimed.Clear();
		m_aAssigned.Clear();
		m_aVacancies.Clear();

		if (m_Utility.DCO_GetGarrisonActivity() == this)
			m_Utility.DCO_SetGarrisonActivity(null);
	}

	override void OnActionFailed()
	{
		super.OnActionFailed();
		ReleaseAll();
		SendCancelMessagesToAllAgents();
	}

	override void OnActionCompleted()
	{
		super.OnActionCompleted();
		ReleaseAll();
		SendCancelMessagesToAllAgents();
	}

	override string GetActionDebugInfo()
	{
		return string.Format("%1 garrison buildings=%2 assigned=%3 phase=%4", this, m_aClaimed.Count(), m_aAssigned.Count(), m_ePhase);
	}
}
