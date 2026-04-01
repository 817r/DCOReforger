[ComponentEditorProps(category: "GameScripted/Commander", description: "Handles enemy contact response and reinforcement")]
class CMD_ThreatResponseComponentClass : ScriptComponentClass {}

class CMD_ThreatResponseComponent : ScriptComponent
{
	[Attribute("30.0", UIWidgets.EditBox, "Score minimum untuk engage threat (di bawah ini diabaikan)", category: "Threat")]
	protected float m_fEngageThreshold;

	[Attribute("65.0", UIWidgets.EditBox, "Score minimum untuk kirim reinforcement ke group yang sudah engage", category: "Threat")]
	protected float m_fReinforcementThreshold;

	[Attribute("60.0", UIWidgets.EditBox, "Detik sebelum threat entry yang tidak diupdate dihapus", category: "Threat")]
	protected float m_fThreatExpiry;

	[Attribute("40.0", UIWidgets.EditBox, "Jarak (meter) dua report dianggap threat yang sama", category: "Threat")]
	protected float m_fMergeRadius;

	[Attribute("45.0", UIWidgets.EditBox, "Interval proses threat list (detik)", category: "Threat")]
	protected float m_fThinkInterval;

	protected AICommander_BaseComponent  m_Commander;
	protected ref array<ref CMD_ThreatEntry> m_aThreats        = new array<ref CMD_ThreatEntry>;
	protected float                      m_fThinkTimer     = 0.0;

	void ReceiveContactReport(CMD_ContactReport report, DCO_GroupUtilityComponent grp)
	{
		if (!report)
			return;

		float worldTime = report.m_fReportTime;

		CMD_ThreatEntry existing = FindNearbyThreat(report.m_vPosition);
		if (existing)
		{
			existing.m_vPosition            = report.m_vPosition;
			existing.m_iEstimatedEnemyCount = Math.Max(existing.m_iEstimatedEnemyCount, report.m_iEstimatedEnemyCount);
			existing.m_fLastUpdateTime      = worldTime;
			return;
		}

		CMD_ThreatEntry entry = new CMD_ThreatEntry(report.m_vPosition, report.m_iEstimatedEnemyCount, worldTime, grp);
		m_aThreats.Insert(entry);

		/*Print(string.Format("[CMD_Threat] New contact at %1 | est. %2 enemies | reporter: %3",
			report.m_vPosition.ToString(),
			report.m_iEstimatedEnemyCount,
			report.m_sReporterGroupName));*/
	}

	void ReceiveReinforcementRequest(DCO_GroupUtilityComponent requestingGrp, float worldTime)
	{
		CMD_ThreatEntry threat = FindNearbyThreat(requestingGrp.GetOwner().GetOrigin());
		if (!threat)
		{
			//Print("NO THREAT ENTRY");
			return;
		}
			
		
		//Print(threat.m_vPosition.ToString() + "< THREAT POS REINFORCEMENT REQUEST | REINFORCEMENT SENT > " + threat.m_iReinforcementSentNumber);

		if (threat.m_iReinforcementSentNumber > 3)
			return;

		if (threat.m_fPriorityScore < m_fReinforcementThreshold)
		{
			Print(string.Format("[CMD_Threat] Reinf request denied — score %1 below threshold %2",
				threat.m_fPriorityScore, m_fReinforcementThreshold));
			return;
		}

		SendReinforcement(threat, worldTime);
	}

	protected void Think(float worldTime)
	{
		PurgeExpiredThreats(worldTime);

		int i = 0;
		while (i < m_aThreats.Count())
		{
			CMD_ThreatEntry threat = m_aThreats[i];
			if (!threat)
			{
				i = i + 1;
				continue;
			}

			ScoreThreat(threat, worldTime);
			ClassifyThreat(threat);

			if (!threat.m_bEngaged)
				TryEngage(threat, worldTime);

			i = i + 1;
		}
	}

	protected void ScoreThreat(CMD_ThreatEntry threat, float worldTime)
	{
		float score = 0.0;

		float enemyBonus = Math.Clamp(threat.m_iEstimatedEnemyCount * 6.0, 0.0, 50.0);
		score = score + enemyBonus;

		float age = worldTime - threat.m_fLastUpdateTime;
		float freshnessBonus = Math.Max(0.0, 20.0 - (age * 0.5));
		score = score + freshnessBonus;

		float proximityBonus = ComputeObjectiveProximityBonus(threat.m_vPosition);
		score = score + proximityBonus;

		if (!m_Commander)
		{
			threat.m_fPriorityScore = score;
			return;
		}

		float distToBase    = vector.Distance(threat.m_vPosition, m_Commander.GetOwner().GetOrigin());
		float distPenalty   = Math.Clamp(distToBase * 0.05, 0.0, 25.0);
		score               = Math.Max(0.0, score - distPenalty);

		threat.m_fPriorityScore = score;
	}

	protected float ComputeObjectiveProximityBonus(vector threatPos)
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr || !m_Commander)
			return 0.0;

		float bestBonus = 0.0;

		foreach (CMD_AICommanderObjectiveComponent obj : mgr.m_aObjective)
		{
			if (!obj)
				continue;

			float dist = vector.Distance(threatPos, obj.GetOwner().GetOrigin());
			if (dist > 300.0)
				continue;

			float bonus = Math.Max(0.0, 25.0 - (dist * 0.08));
			if (bonus > bestBonus)
				bestBonus = bonus;
		}

		return bestBonus;
	}

	protected void ClassifyThreat(CMD_ThreatEntry threat)
	{
		float score = threat.m_fPriorityScore;

		if (score < 20.0)
			threat.m_eThreatLevel = CMD_EThreatLevel.NEGLIGIBLE;
		else if (score < m_fEngageThreshold)
			threat.m_eThreatLevel = CMD_EThreatLevel.LOW;
		else if (score < 55.0)
			threat.m_eThreatLevel = CMD_EThreatLevel.MEDIUM;
		else if (score < 75.0)
			threat.m_eThreatLevel = CMD_EThreatLevel.HIGH;
		else
			threat.m_eThreatLevel = CMD_EThreatLevel.CRITICAL;
	}

	protected void TryEngage(CMD_ThreatEntry threat, float worldTime)
	{
		if (threat.m_fPriorityScore < m_fEngageThreshold)
		{
			/*Print(string.Format("[CMD_Threat] Threat at %1 ignored (score: %2 < threshold: %3)",
				threat.m_vPosition.ToString(),
				threat.m_fPriorityScore.ToString(),
				m_fEngageThreshold.ToString()));*/
			return;
		}

		if (!m_Commander)
			return;

		CMD_EGroupRole preferredRole = CMD_EGroupRole.ASSAULT;
		if (threat.m_eThreatLevel == CMD_EThreatLevel.MEDIUM)
			preferredRole = CMD_EGroupRole.REINFORNCE;

		DCO_GroupUtilityComponent engageGrp = m_Commander.FindBestIdleGroupForRole_Public(preferredRole);

		if (!engageGrp && preferredRole == CMD_EGroupRole.REINFORNCE)
			engageGrp = m_Commander.FindBestIdleGroupForRole_Public(CMD_EGroupRole.FLANK);

		if (!engageGrp)
		{
			//Print(string.Format("[CMD_Threat] No available group to engage threat at %1",
				//threat.m_vPosition.ToString()));
			return;
		}

		SCR_AIWaypoint wp = m_Commander.SpawnMoveWP(threat.m_vPosition);
		if (!wp)
			return;

		engageGrp.SetGroupRole(CMD_EGroupRole.REINFORNCE);
		engageGrp.MoveTo(wp, worldTime);

		threat.m_bEngaged            = true;
		threat.m_sEngagingGroupName  = engageGrp;

		/*Print(string.Format("[CMD_Threat] ENGAGE threat (score: %1 | level: %2) | group: %3 → %4",
			threat.m_fPriorityScore.ToString(),
			typename.EnumToString(CMD_EThreatLevel, threat.m_eThreatLevel),
			engageGrp.GetOwner().GetName(),
			threat.m_vPosition.ToString()));*/
	}

	protected void SendReinforcement(CMD_ThreatEntry threat, float worldTime)
	{
		if (!m_Commander)
			return;

		DCO_GroupUtilityComponent reinforcement = m_Commander.FindBestIdleGroupForRole_Public(CMD_EGroupRole.REINFORNCE);
		if (!reinforcement)
		{
			//Print(string.Format("[CMD_Threat] Reinforcement requested but no group available"));
			return;
		}
		
		if (m_Commander.TryAssignTransport(reinforcement, threat.m_vPosition, worldTime))
    		return;

		SCR_AIWaypoint wp = m_Commander.SpawnMoveWP(threat.m_vPosition);
		if (!wp)
			return;

		reinforcement.SetGroupRole(CMD_EGroupRole.REINFORNCE);
		reinforcement.MoveTo(wp, worldTime);

		threat.m_iReinforcementSentNumber++;
		threat.m_bReinforcementSent = true;

		/*Print(string.Format("[CMD_Threat] REINFORCEMENT sent: %1 → %2",
			reinforcement.GetOwner().GetName(),
			threat.m_vPosition.ToString()));*/
	}

	protected CMD_ThreatEntry FindNearbyThreat(vector pos)
	{
		//Print(m_aThreats.Count().ToString() + " < Threat Count");
		for(int i = 0; i < m_aThreats.Count(); i++)
		{
			//Print(m_aThreats[i].m_vPosition.ToString() + " < POS");
			//Print(vector.DistanceXZ(m_aThreats[i].m_vPosition, pos).ToString() + " < DISTANCE");
			if (vector.DistanceXZ(m_aThreats[i].m_vPosition, pos) <= m_fMergeRadius)
			{
				return m_aThreats[i];
			}
				
		}
		return null;
	}

	protected void PurgeExpiredThreats(float worldTime)
	{
		int i = 0;
		while (i < m_aThreats.Count())
		{
			CMD_ThreatEntry t = m_aThreats[i];
			if (!t)
			{
				m_aThreats.Remove(i);
				continue;
			}

			float age = worldTime - t.m_fLastUpdateTime;
			if (age > m_fThreatExpiry)
			{
				//Print(string.Format("[CMD_Threat] Threat at %1 expired (age: %2s)",
				//	t.m_vPosition.ToString(), age.ToString()));
				m_aThreats.Remove(i);
				continue;
			}

			i = i + 1;
		}
	}

	int GetActiveThreatCount()   { return m_aThreats.Count(); }
	FactionKey GetFactionKey()
	{
		if (!m_Commander)
			return string.Empty;
		return m_Commander.GetCommanderFactionKey();
	}

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (!Replication.IsServer())
			return;

		m_fThinkTimer += timeSlice;
		if (m_fThinkTimer < m_fThinkInterval)
			return;

		m_fThinkTimer = 0.0;
		float worldTime = GetGame().GetWorld().GetWorldTime() / 1000.0;
		Think(worldTime);
	}

	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);
		SetEventMask(owner, EntityEvent.FRAME);
	}

	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		if (!Replication.IsServer())
			return;

		m_Commander = AICommander_BaseComponent.Cast(owner.FindComponent(AICommander_BaseComponent));
		if (!m_Commander)
		{
			//Print("[CMD_ThreatResponse] ERROR: AICommander_BaseComponent tidak ditemukan di entity yang sama!", LogLevel.ERROR);
			return;
		}
	}
}