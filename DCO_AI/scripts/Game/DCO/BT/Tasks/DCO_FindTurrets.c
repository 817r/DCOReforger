class DCO_IdleTurretRules
{
	static string BlockReason(SCR_AIGroup group)
	{
		if (!group)
			return "no_group";
		SCR_AIGroupUtilityComponent gu = group.GetGroupUtilityComponent();
		if (!gu || !gu.DCO_IsIdle())
			return "not_idle";
		if (gu.m_Perception)
		{
			foreach (SCR_AIGroupTargetCluster c : gu.m_Perception.m_aTargetClusters)
			{
				if (c && c.m_State && c.m_State.m_iCountAlive > 0 && c.m_State.GetTimeSinceLastNewInformation() < 60.0)
					return "contact";
			}
		}
		return "";
	}

	static int Cap(SCR_AIGroup group)
	{
		int alive = group.GetAgentsCount();
		if (alive <= 1)
			return 0;
		if (alive <= 3)
			return 1;
		return alive / 2;
	}

	static bool OwnedByGroup(IEntity occupant, SCR_AIGroup group)
	{
		if (!occupant || !group)
			return false;
		AIControlComponent ctrl = AIControlComponent.Cast(occupant.FindComponent(AIControlComponent));
		if (!ctrl || !ctrl.GetControlAIAgent())
			return false;
		return ctrl.GetControlAIAgent().GetParentGroup() == group;
	}

	static string Kind(SCR_AIVehicleUsageComponent usage, SCR_AIGroup group)
	{
		if (!usage)
			return "";
		if (usage.GetVehicleType() == EAIVehicleType.STATIC_WEAPON)
		{
			if (usage.m_DCOOwnerGroup && usage.m_DCOOwnerGroup != group && usage.DCO_OwnerAlive())
				return "";
			return "static";
		}
		if (usage.GetVehicleType() == EAIVehicleType.GROUND_VEHICLE_TRACKED || usage.GetVehicleType() == EAIVehicleType.GROUND_VEHICLE_WHEELED)
		{
			if (usage.m_DCOOwnerGroup == group && usage.DCO_OwnerAlive())
				return "own_vehicle";
		}
		return "";
	}
}

class SCR_DCOAIFindTurrets: AITaskScripted
{
	[Attribute("0", UIWidgets.EditBox)]
	protected float m_fRadius;

	protected TagSystem m_tagSystem;
	protected SCR_AIGroup m_groupOwner;

	static override bool VisibleInPalette() {return true;}

	override void OnInit(AIAgent owner)
	{
		ChimeraWorld world = ChimeraWorld.CastFrom(GetGame().GetWorld());
		m_tagSystem = TagSystem.Cast(world.FindSystem(TagSystem));
		if (!m_tagSystem)
		{
			NodeError(this, owner, "SCR_AIFindTurret: TagManager is not present in the world, cannot find static turrets.");
		}
		m_groupOwner = SCR_AIGroup.Cast(owner);
		if (!m_groupOwner)
		{
			m_groupOwner = SCR_AIGroup.Cast(owner.GetParentGroup());
			if (!m_groupOwner)
				NodeError(this, owner, "Node is not run on SCR_AIGroup agent or owner is not member of SCR_AIGroup!");
		}
	}

	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		ClearVariable("TurretNumber");
		SetVariableOut("TurretsFound", false);
		if (!m_tagSystem || !m_groupOwner)
			return ENodeResult.SUCCESS;

		string block = DCO_IdleTurretRules.BlockReason(m_groupOwner);
		if (!block.IsEmpty())
		{
			SkipLog(block);
			return ENodeResult.SUCCESS;
		}

		vector center;
		float radius;
		GetVariableIn("OriginIn", center);
		if (!GetVariableIn("RadiusIn", radius))
			radius = m_fRadius;

		int cap = DCO_IdleTurretRules.Cap(m_groupOwner);
		int used;
		int turretCount;
		int otherOwner;
		bool capped;

		array<IEntity> entities = {};
		m_tagSystem.GetTagsInRange(entities, center, radius, ETagCategory.StaticTurret);
		array<BaseCompartmentSlot> compartmentSlots = {};
		foreach (IEntity ent : entities)
		{
			BaseCompartmentManagerComponent compComp = BaseCompartmentManagerComponent.Cast(ent.FindComponent(BaseCompartmentManagerComponent));
			if (!compComp)
				continue;
			SCR_AIVehicleUsageComponent vehicleUsageComp = SCR_AIVehicleUsageComponent.Cast(ent.FindComponent(SCR_AIVehicleUsageComponent));
			if (!vehicleUsageComp)
				continue;
			if (DCO_IdleTurretRules.Kind(vehicleUsageComp, m_groupOwner).IsEmpty())
			{
				otherOwner++;
				continue;
			}
			compartmentSlots.Clear();
			compComp.GetCompartments(compartmentSlots);
			foreach (BaseCompartmentSlot slot : compartmentSlots)
			{
				TurretCompartmentSlot turretComp = TurretCompartmentSlot.Cast(slot);
				if (!turretComp)
					continue;
				IEntity occ = turretComp.GetOccupant();
				if (occ && DCO_IdleTurretRules.OwnedByGroup(occ, m_groupOwner))
				{
					used++;
					break;
				}
				if (!turretComp.AttachedOccupant() && turretComp.IsCompartmentAccessible() && !turretComp.IsReserved())
				{
					if (used + turretCount >= cap)
					{
						capped = true;
						break;
					}
					turretCount++;
					array<BaseCompartmentSlot> alloc = {};
					m_groupOwner.GetAllocatedCompartments(alloc);
					if (alloc.Contains(turretComp))
						DCO_Perf.Count("turret:already_allocated");
					else
						m_groupOwner.AllocateCompartment(turretComp);
					break;
				}
			}
		}

		if (otherOwner > 0)
			SkipLog("other_owner");
		if (capped)
			SkipLog("cap_reached");

		if (turretCount > 0)
			SetVariableOut("TurretNumber", turretCount);
		SetVariableOut("TurretsFound", turretCount > 0);
		return ENodeResult.SUCCESS;
	}

	protected void SkipLog(string reason)
	{
		DCO_Perf.Count("idle_turret:skip_" + reason);
		if (DCO_Log.On(DCO_ELogChannel.SQUAD, DCO_ELogLevel.DEBUG))
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.DEBUG, DCO_Log.IdGroup(m_groupOwner), string.Format("idle_turret_skip grp=%1 reason=%2", DCO_Log.IdGroup(m_groupOwner), reason));
	}

	protected ref TStringArray m_aDCOVarsOut = {
		"TurretNumber",
		"TurretsFound"
	};
	override TStringArray GetVariablesOut()
	{
		return m_aDCOVarsOut;
	}

	protected ref TStringArray m_aDCOVarsIn = {
		"OriginIn",
		"RadiusIn"
	};
	override TStringArray GetVariablesIn()
	{
		return m_aDCOVarsIn;
	}

	static override string GetOnHoverDescription()
	{
		return "DCO idle: allocates free turrets (static weapons, or turrets of the group's own vehicles) near the leader, max half the group, only while the group is idle and not in contact.";
	}
};

modded class SCR_AIVehicleUsageComponent
{
	SCR_AIGroup m_DCOOwnerGroup;

	bool DCO_OwnerAlive()
	{
		return m_DCOOwnerGroup && m_DCOOwnerGroup.GetAgentsCount() > 0;
	}
}
