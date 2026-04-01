[ComponentEditorProps(category: "GameScripted/AI/AICommander", description: "Component for AI Commander Objective")]
class CMD_AICommanderObjectiveComponentClass : ScriptComponentClass
{
}

class CMD_AICommanderObjectiveComponent : ScriptComponent
{
	[Attribute("50.0", UIWidgets.EditBox, "Base strategic value (0–100). Makin tinggi makin penting.", category: "Objective")]
	protected float m_fBaseValue;
	
	[Attribute("150.0", UIWidgets.EditBox, "Radius (meter) untuk deteksi musuh di sekitar objective ini.", category: "Objective")]
	protected float m_fThreatRadius;
 
	[Attribute("200.0", UIWidgets.EditBox, "Radius (meter) untuk cek friendly presence.", category: "Objective")]
	protected float m_fFriendlyRadius;
	
	[Attribute("30.0", UIWidgets.Slider, "Radius of the Objective", "1.0 200.0 1.0")]
	protected float m_fRadius;
	
	[Attribute("0", UIWidgets.ComboBox, "Tipe objective ini", "", ParamEnumArray.FromEnum(CMD_EObjectiveType))]
	CMD_EObjectiveType m_eObjectiveType;
 
	[Attribute("60.0", UIWidgets.EditBox, "Detik yang dibutuhkan untuk capture (groups harus di area)", category: "Objective")]
	protected float m_fCaptureHoldDuration;
 
	[Attribute("1", UIWidgets.EditBox, "Berapa group yang di-assign untuk defend setelah captured", category: "Objective")]
	protected int m_iDefendGroupCount;
 
	ref map<FactionKey, float> m_mCaptureStartTime = new map<FactionKey, float>();
	ref map<FactionKey, bool>  m_mIsCaptured       = new map<FactionKey, bool>();
	
	ref map<FactionKey, CMD_EObjectiveState> m_mObjectiveState = new map<FactionKey, CMD_EObjectiveState>();
	ref map<FactionKey, int> m_mObjectiveAssignedGroup = new map<FactionKey, int>();
	protected ref map<FactionKey, DCO_GroupUtilityComponent> m_mReconGroup = new map<FactionKey, DCO_GroupUtilityComponent>();
	
	IEntity m_OwnerEntity;
	
	protected float m_fCachedScore    = 0.0;
	protected float m_fScoreCacheAge  = 0.0;
	static const float CACHE_DURATION = 3.0;
 
	protected float m_fLastContestedTime = 0.0;
	
	float GetRadius()
	{
		return m_fRadius;
	}
	
	void MarkCompleted(FactionKey fk) { SetObjectiveState(fk, CMD_EObjectiveState.COMPLETED); }
	void MarkFailed(FactionKey fk)    { SetObjectiveState(fk, CMD_EObjectiveState.FAILED); }
	void MarkAssigned(FactionKey fk)  { SetObjectiveState(fk, CMD_EObjectiveState.ASSIGNED); }
 
	void NotifyContested(float worldTime)
	{
		m_fLastContestedTime = worldTime;
		m_fScoreCacheAge     = 0.0; // invalidate cache
	}
	
	CMD_EObjectiveState GetObjectiveState(FactionKey fk)
	{
		CMD_EObjectiveState state;
		if (m_mObjectiveState.Find(fk, state))
			return state;
		return CMD_EObjectiveState.PENDING;
	}
	
	CMD_EObjectiveAction GetObjectiveAction(FactionKey fk)
	{
		int state;
		if (m_mObjectiveAssignedGroup.Find(fk, state))
			return state;
		return CMD_EObjectiveAction.NONE;
	}
	
	protected void InitializeObjective()
	{
		AICommander_ManagerComponent.GetInstance().RegisterObjective(this);
		for(int i = 0; i < AICommander_ManagerComponent.GetInstance().m_aAvailableFactions.Count(); i++)
		{
			m_mObjectiveState.Insert(AICommander_ManagerComponent.GetInstance().m_aAvailableFactions[i], CMD_EObjectiveState.PENDING);
			m_mObjectiveAssignedGroup.Insert(AICommander_ManagerComponent.GetInstance().m_aAvailableFactions[i], 0);
		}
		
	}
	
	float ComputePriorityScore(FactionKey forFaction, float worldTime)
	{
		CMD_EObjectiveState currentState = GetObjectiveState(forFaction);
 
		if (currentState == CMD_EObjectiveState.COMPLETED || currentState == CMD_EObjectiveState.FAILED)
			return 0.0;
 
		if ((worldTime - m_fScoreCacheAge) < CACHE_DURATION)
			return m_fCachedScore;
 
		float score = m_fBaseValue;
 
		int enemyCount = CountNearbyUnits(m_fThreatRadius, forFaction, false);
		if (enemyCount > 0)
			score += Math.Clamp(enemyCount * 8.0, 0.0, 40.0);
 
		if (m_fLastContestedTime > 0.0)
		{
			float elapsed = worldTime - m_fLastContestedTime;
			if (elapsed < 120.0)
				score += Math.Lerp(25.0, 0.0, elapsed / 120.0);
		}
 
		int friendlyCount = CountNearbyUnits(m_fFriendlyRadius, forFaction, true);
		score -= Math.Clamp(friendlyCount * 5.0, 0.0, 30.0);
 
		if (currentState == CMD_EObjectiveState.ASSIGNED)
			score -= 15.0;
 
		m_fCachedScore   = Math.Max(score, 0.0);
		m_fScoreCacheAge = worldTime;
 
		return m_fCachedScore;
	}
	
	bool QueryCallback(IEntity e)
	{
		if (!nearby.Contains(e))
			nearby.Insert(e);
		return true;
	}
	
	ref array<IEntity> nearby = {};
	
	int CountNearbyUnits(float radius, FactionKey factionKey, bool isFriendly)
	{
		int count  = 0;
		vector pos = GetOwner().GetOrigin();
 
		GetGame().GetWorld().QueryEntitiesBySphere(pos, radius, null, QueryCallback, EQueryEntitiesFlags.ALL);
		
		foreach (IEntity ent : nearby)
		{
			if (!ent)
				continue;
 
			SCR_ChimeraCharacter grp = SCR_ChimeraCharacter.Cast(ent);
			if (!grp)
				continue;
			
			SCR_CharacterPerceivableComponent percive = SCR_CharacterPerceivableComponent.Cast(ent.FindComponent(SCR_CharacterPerceivableComponent));
			if (!percive)
				continue;
			
			Faction fc = percive.GetPerceivedFaction();
			if (!fc)
				continue;
 
			bool sameFaction = (fc.GetFactionKey() == factionKey);
 
			if (isFriendly && sameFaction)
				count++;
			else if (!isFriendly && !sameFaction)
				count++;
		}
 		//Print(string.Format("[CMD_Objective] %1 | %2 Is Friendly %3 Count %4",
			//GetOwner().GetName(), factionKey, isFriendly, count));		
		return count;
	}
	
	void SetObjectiveState(FactionKey fk, CMD_EObjectiveState state)
	{
		if (m_mObjectiveState.Contains(fk))
		{
			m_mObjectiveState[fk] = state;
			//Print("SET OBJ STATE TO > " + typename.EnumToString(CMD_EObjectiveState, m_mObjectiveState[fk]) + " FK : " + fk);
		}
		
	}
	
	void SetObjectiveGroup(FactionKey fk, int number)
	{
		if (m_mObjectiveAssignedGroup.Contains(fk))
		{
			int num = m_mObjectiveAssignedGroup[fk] + number;
			m_mObjectiveAssignedGroup[fk] = num;
			//Print(m_mObjectiveAssignedGroup[fk].ToString() + " < Number of assigned Group | Num > " + num);
			
		}
	}
	
	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		m_OwnerEntity = owner;
		SetEventMask(owner, EntityEvent.INIT);
	}
	
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		InitializeObjective();
	}

	int GetRequiredGroupCount()
	{
		int radiusTier;
		if (m_fRadius < 70.0)
			radiusTier = 1;
		else if (m_fRadius < 175.0)
			radiusTier = 2;
		else
			radiusTier = Math.Round(m_fRadius / 175) + 1;
 
		int priorityTier;
		if (m_fBaseValue < 34.0)
			priorityTier = 0;
		else if (m_fBaseValue < 67.0)
			priorityTier = 1;
		else
			priorityTier = 2;
 
		int typeBonus = 0;
		if (m_eObjectiveType == CMD_EObjectiveType.CAPTURE)
			typeBonus = 1;
		else if (m_eObjectiveType == CMD_EObjectiveType.DESTROY)
			typeBonus = 2;
		float grp = radiusTier + priorityTier; //+ typeBonus;
 
		return grp;
	}
 
	CMD_EObjectiveType GetObjectiveType()    { return m_eObjectiveType; }
	int GetDefendGroupCount()                { return m_iDefendGroupCount; }
 
	int GetCurrentAssignedGroupCount(FactionKey fk)
	{
		int count;
		if (m_mObjectiveAssignedGroup.Find(fk, count))
			return count;
		return 0;
	}
	

	
	void SetReconGroup(FactionKey fk, DCO_GroupUtilityComponent grp)
	{
	    if (!m_mReconGroup.Contains(fk))
	        m_mReconGroup.Insert(fk, grp);
	    else
	        m_mReconGroup.Set(fk, grp);
	}
	
	bool IsReconArrived(FactionKey fk, float worldTime)
	{
	    DCO_GroupUtilityComponent reconGrp;
	    if (!m_mReconGroup.Find(fk, reconGrp))
	        return false;
	
	    if (!reconGrp)
	        return false;
	
	    return reconGrp.CheckOrderComplete(worldTime);
	}
 
	void ResetAssignedGroupCount(FactionKey fk)
	{
		int current = GetCurrentAssignedGroupCount(fk);
		SetObjectiveGroup(fk, -current);
	}

	void StartCaptureTimer(FactionKey fk, float worldTime)
	{
		if (!m_mCaptureStartTime.Contains(fk))
			m_mCaptureStartTime.Insert(fk, worldTime);
		else
			m_mCaptureStartTime.Set(fk, worldTime);
 
		//Print(string.Format("[CMD_Objective] %1 | %2 capture timer started (hold %3s)",
			//GetOwner().GetName(), fk, m_fCaptureHoldDuration.ToString()));
	}
 
	bool IsCaptureTimerRunning(FactionKey fk)
	{
		return m_mCaptureStartTime.Contains(fk);
	}
 
	bool IsCaptureTimerComplete(FactionKey fk, float worldTime)
	{
		float startTime;
		if (!m_mCaptureStartTime.Find(fk, startTime))
			return false;
		
		float elapsed = worldTime - startTime;
		
		if (CountNearbyUnits(m_fRadius, fk, true) < CountNearbyUnits(m_fRadius, fk, false))
		{
			elapsed = 0;
		}
		
		return elapsed >= m_fCaptureHoldDuration;
	}
 
	float GetCaptureProgress(FactionKey fk, float worldTime)
	{
		float startTime;
		if (!m_mCaptureStartTime.Find(fk, startTime))
			return 0.0;
 
		float elapsed = worldTime - startTime;
		if (CountNearbyUnits(m_fRadius, fk, true) < CountNearbyUnits(m_fRadius, fk, false))
		{
			elapsed = 0;
		}
		//Print(string.Format("[CMD_Objective] %1 Elapsed %2 Hold Duration %3 Faction",
			//elapsed, m_fCaptureHoldDuration, fk));		
		return Math.Clamp(elapsed / m_fCaptureHoldDuration, 0.0, 1.0);
	}
 
	bool IsCapturedBy(FactionKey fk)
	{
		bool captured;
		if (m_mIsCaptured.Find(fk, captured))
			return captured;
		return false;
	}
 
	void SetCapturedBy(FactionKey fk, bool val)
	{
		if (!m_mIsCaptured.Contains(fk))
			m_mIsCaptured.Insert(fk, val);
		else
			m_mIsCaptured.Set(fk, val);
	}
 
	bool IsGroupSlotFull(FactionKey fk)
	{
		return GetCurrentAssignedGroupCount(fk) >= GetRequiredGroupCount();
	}
}