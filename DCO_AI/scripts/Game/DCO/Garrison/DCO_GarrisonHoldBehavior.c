class DCO_GarrisonHoldBehavior : SCR_AIBehaviorBase
{
	static const float RETURN_DIST = 1.5;

	protected DCO_GarrisonBuilding m_Building;
	protected int m_iSlot;
	protected int m_iSlotVersion;

	void DCO_GarrisonHoldBehavior(SCR_AIUtilityComponent utility, SCR_AIActivityBase groupActivity, DCO_GarrisonBuilding building, int slotIdx, float priorityLevel = PRIORITY_LEVEL_NORMAL)
	{
		m_sBehaviorTree = "AI/BehaviorTrees/Chimera/Soldier/DCO_GarrisonHold.bt";
		SetPriority(PRIORITY_BEHAVIOR_DEFEND);
		m_fPriorityLevel.m_Value = priorityLevel;
		m_Building = building;
		m_iSlot = slotIdx;
	}

	override int GetCause()
	{
		return SCR_EAIBehaviorCause.COMBAT;
	}

	void SetSlot(DCO_GarrisonBuilding building, int slotIdx)
	{
		m_Building = building;
		m_iSlot = slotIdx;
		m_iSlotVersion++;
	}

	DCO_GarrisonSlot GetSlot()
	{
		if (!m_Building || !m_Building.m_aSlots.IsIndexValid(m_iSlot))
			return null;

		return m_Building.m_aSlots[m_iSlot];
	}

	DCO_GarrisonBuilding GetBuilding() { return m_Building; }
	int GetSlotVersion() { return m_iSlotVersion; }

	static DCO_GarrisonHoldBehavior Find(notnull SCR_AIUtilityComponent utility, SCR_AIActivityBase activity)
	{
		array<ref AIActionBase> actions = {};
		utility.FindActionsOfType(DCO_GarrisonHoldBehavior, actions);
		foreach (AIActionBase action : actions)
		{
			DCO_GarrisonHoldBehavior hold = DCO_GarrisonHoldBehavior.Cast(action);
			if (!hold || hold.GetRelatedGroupActivity() != activity)
				continue;

			EAIActionState state = hold.GetActionState();
			if (state != EAIActionState.FAILED && state != EAIActionState.COMPLETED)
				return hold;
		}
		return null;
	}

	override string GetActionDebugInfo()
	{
		DCO_GarrisonSlot s = GetSlot();
		if (!s)
			return this.ToString() + " garrison (no slot)";

		return string.Format("%1 garrison slot=%2 type=%3", this, m_iSlot, typename.EnumToString(DCO_EGarrisonSlotType, s.m_eType));
	}
}

class DCO_AIGarrisonGetSlot : SCR_AIActionTask
{
	static const string PORT_POS = "SlotPos";
	static const string PORT_STANCE = "Stance";

	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		DCO_GarrisonHoldBehavior hold = DCO_GarrisonHoldBehavior.Cast(GetExecutedAction());
		if (!hold)
			return ENodeResult.FAIL;

		DCO_GarrisonSlot slot = hold.GetSlot();
		if (!slot)
			return ENodeResult.FAIL;

		SetVariableOut(PORT_POS, slot.m_vWorldPos);
		SetVariableOut(PORT_STANCE, slot.m_eStance);
		return ENodeResult.SUCCESS;
	}

	protected static ref TStringArray s_aVarsOut = {PORT_POS, PORT_STANCE};
	override TStringArray GetVariablesOut() { return s_aVarsOut; }

	static override bool VisibleInPalette() { return true; }
	static override string GetOnHoverDescription() { return "DCO: posisi + stance slot garrison."; }
}

class DCO_AIGarrisonHold : SCR_AIActionTask
{
	protected static const float LOOK_INTERVAL_MIN_MS = 3000;
	protected static const float LOOK_INTERVAL_MAX_MS = 8000;
	protected static const float LOOK_PRIORITY = 10;

	protected int m_iVersion = -1;
	protected float m_fNextLook_ms;

	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		DCO_GarrisonHoldBehavior hold = DCO_GarrisonHoldBehavior.Cast(GetExecutedAction());
		if (!hold)
			return ENodeResult.FAIL;

		DCO_GarrisonSlot slot = hold.GetSlot();
		IEntity ent = owner.GetControlledEntity();
		if (!slot || !ent)
			return ENodeResult.FAIL;

		if (m_iVersion < 0)
			m_iVersion = hold.GetSlotVersion();

		if (m_iVersion != hold.GetSlotVersion() || vector.DistanceXZ(ent.GetOrigin(), slot.m_vWorldPos) > DCO_GarrisonHoldBehavior.RETURN_DIST)
		{
			m_iVersion = -1;
			return ENodeResult.SUCCESS;
		}

		float now = GetGame().GetWorld().GetWorldTime();
		if (now >= m_fNextLook_ms)
		{
			m_fNextLook_ms = now + Math.RandomFloat(LOOK_INTERVAL_MIN_MS, LOOK_INTERVAL_MAX_MS);

			SCR_AIUtilityComponent utility = SCR_AIUtilityComponent.Cast(m_UtilityComp);
			if (utility && utility.m_LookAction)
			{
				float yaw = slot.m_vWorldDir.ToYaw() + Math.RandomFloat(-0.5, 0.5) * slot.m_fArc;
				vector lookPos = slot.m_vWorldPos + DCO_GarrisonSlot.YawToDir(yaw) * 30 + 1.5 * vector.Up;
				utility.m_LookAction.LookAt(lookPos, LOOK_PRIORITY, 2.5);
			}
		}

		return ENodeResult.RUNNING;
	}

	override void OnAbort(AIAgent owner, Node nodeCausingAbort)
	{
		m_iVersion = -1;
	}

	static override bool VisibleInPalette() { return true; }
	static override string GetOnHoverDescription() { return "DCO: tahan di slot garrison, lirik busur slot; selesai kalau melenceng / slot ganti."; }
}
