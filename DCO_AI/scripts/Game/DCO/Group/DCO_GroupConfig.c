[ComponentEditorProps(category: "GameScripted/Group")]
class DCO_GroupConfigComponentClass : ScriptComponentClass
{
}

class DCO_GroupConfigComponent : ScriptComponent
{
	[Attribute("0", UIWidgets.Flags, "What is this group Capable of", category: "Capabilities and Role", enums: ParamEnumArray.FromEnum(DCO_EAIGroupCapabilities))]
	protected DCO_EAIGroupCapabilities m_fGroupCapabilities;

	[Attribute("50", UIWidgets.Auto, "What is the max distance from SL before attempt to Regroup", category: "Group Cohession")]
	protected float m_fCohessionDistance;

	[Attribute("1", UIWidgets.Auto, "What is the Multiplier of Cohession Distance", category: "Group Cohession", params: "0.01 5 0.01")]
	protected float m_fCohessionDistanceMult;

	[Attribute("2500", UIWidgets.Slider, "Maximum distance (m) at which the squad fires at infantry targets.", category: "Engagement", params: "0 2500 10")]
	protected float m_fEngagementDistanceInfantry;

	[Attribute("800", UIWidgets.Slider, "Maximum distance (m) at which the squad fires at vehicle targets.", category: "Engagement", params: "0 2500 10")]
	protected float m_fEngagementDistanceVehicle;

	[Attribute("-1", UIWidgets.Slider, "Maximum distance (m) at which the squad investigates unclear contacts. -1 = use global.", category: "Investigate", params: "-1 1500 10")]
	protected float m_fInvestigateMaxDist;

	[Attribute("-1", UIWidgets.Slider, "Chance the squad investigates an unclear contact (rolled once per contact). -1 = use global.", category: "Investigate", params: "-1 1 0.05")]
	protected float m_fInvestigateChance;

	[Attribute("-1", UIWidgets.ComboBox, "Defend and idle position preference for this squad. Follow global = DCO Global Combat setting.", category: "Cover", enums: { ParamEnum("Follow global", "-1"), ParamEnum("Balanced", "0"), ParamEnum("Indoor", "1"), ParamEnum("Outdoor", "2") })]
	protected int m_iCoverPreference;

	int GetCoverPreference()
	{
		if (m_iCoverPreference >= 0)
			return m_iCoverPreference;
		DCO_GlobalAIComponent global = DCO_GlobalAIComponent.GetFor(GetOwner());
		if (global)
			return global.GetCoverPreference();
		return 0;
	}

	void SetCoverPreference(int pref)	{ m_iCoverPreference = Math.ClampInt(pref, -1, 2); }


	protected AIGroup m_Group;

	protected AIGroup GetGroup()
	{
		if (!m_Group)
			m_Group = AIGroup.Cast(GetOwner());

		return m_Group;
	}

	float GetCohesionDistance()
	{
		return m_fCohessionDistance * m_fCohessionDistanceMult;
	}

	float GetCohesionMult()
	{
		return m_fCohessionDistanceMult;
	}

	float GetEngagementDistanceInfantry()
	{
		return Math.Clamp(m_fEngagementDistanceInfantry, 0, 2500.0);
	}

	float GetEngagementDistanceVehicle()
	{
		return Math.Clamp(m_fEngagementDistanceVehicle, 0, 2500.0);
	}

	void SetEngagementDistance(float infantry, float vehicle)
	{
		m_fEngagementDistanceInfantry = Math.Clamp(infantry, 0, 2500.0);
		m_fEngagementDistanceVehicle = Math.Clamp(vehicle, 0, 2500.0);
	}

	float GetInvestigateMaxDist()
	{
		if (m_fInvestigateMaxDist >= 0)
			return m_fInvestigateMaxDist;

		DCO_GlobalAIComponent global = DCO_GlobalAIComponent.GetFor(GetOwner());
		if (global)
			return global.GetInvestigateMaxDist();

		return 500;
	}

	float GetInvestigateChance()
	{
		if (m_fInvestigateChance >= 0)
			return m_fInvestigateChance;

		DCO_GlobalAIComponent global = DCO_GlobalAIComponent.GetFor(GetOwner());
		if (global)
			return global.GetInvestigateChance();

		return 1;
	}

	void SetInvestigateMaxDist(float dist)
	{
		m_fInvestigateMaxDist = Math.Clamp(dist, 0, 1500);
	}

	void SetInvestigateChance(float chance)
	{
		m_fInvestigateChance = Math.Clamp(chance, 0, 1);
	}

	bool ShouldReturnToFormation(vector who)
	{
		AIGroup group = GetGroup();

		if (!group)
			return false;

		IEntity leader = group.GetLeaderEntity();

		if (!leader)
			return false;

		return vector.Distance(leader.GetOrigin(), who) > GetCohesionDistance();
	}

	bool GroupCapableOf(DCO_EAIGroupCapabilities cap)
	{
		return GetCapabilities() & cap;
	}

	int GetCapabilities()
	{
		if (m_fGroupCapabilities != 0)
			return m_fGroupCapabilities;

		AIGroup group = GetGroup();
		if (!group)
			return 0;

		int caps = 0;
		array<AIAgent> agents = {};
		array<IEntity> weapons = {};
		group.GetAgents(agents);
		foreach (AIAgent agent : agents)
		{
			IEntity member = agent.GetControlledEntity();
			if (!member)
				continue;

			BaseWeaponManagerComponent wm = BaseWeaponManagerComponent.Cast(member.FindComponent(BaseWeaponManagerComponent));
			if (!wm)
				continue;

			weapons.Clear();
			wm.GetWeaponsList(weapons);
			foreach (IEntity weapon : weapons)
			{
				WeaponComponent wc = WeaponComponent.Cast(weapon.FindComponent(WeaponComponent));
				if (!wc)
					continue;

				switch (wc.GetWeaponType())
				{
					case EWeaponType.WT_ROCKETLAUNCHER: caps |= DCO_EAIGroupCapabilities.ANTI_ARMOR; break;
					case EWeaponType.WT_MACHINEGUN:     caps |= DCO_EAIGroupCapabilities.SUPPRESSING; break;
					case EWeaponType.WT_SNIPERRIFLE:    caps |= DCO_EAIGroupCapabilities.LONG_RANGE; break;
				}
			}
		}
		return caps;
	}

	void AddUnitState(DCO_EAIGroupCapabilities state)
	{
		m_fGroupCapabilities = m_fGroupCapabilities | state;
	}

	void RemoveUnitState(DCO_EAIGroupCapabilities state)
	{
		if (m_fGroupCapabilities & state)
			m_fGroupCapabilities = m_fGroupCapabilities & ~state;
	}
}