modded class DCO_GlobalAIComponent
{
	protected static bool s_bDCO_SpawnFaction;
	protected static ref map<string, IEntity> s_mDCO_FactionEntity;
	protected static ref map<string, DCO_GlobalAIComponent> s_mDCO_FactionComp;
	protected static ref map<string, ref map<string, vector>> s_mDCO_Overrides;
	protected bool m_bDCO_Faction;

	override void OnPostInit(IEntity owner)
	{
		if (s_bDCO_SpawnFaction)
		{
			m_bDCO_Faction = true;
			return;
		}
		super.OnPostInit(owner);
	}

	override void EOnInit(IEntity owner)
	{
		if (m_bDCO_Faction)
			return;
		super.EOnInit(owner);
	}

	override void OnDelete(IEntity owner)
	{
		if (m_bDCO_Faction)
			return;
		super.OnDelete(owner);
	}

	override void SetShareToPlayers(bool b)
	{
		if (m_bDCO_Faction)
		{
			m_bShareToPlayers = b;
			return;
		}
		super.SetShareToPlayers(b);
	}

	bool DCO_IsFactionLayer()
	{
		return m_bDCO_Faction;
	}

	override protected string DCO_DocScope()
	{
		if (m_bDCO_Faction)
			return "faction";
		return "global";
	}

	static bool DCO_FactionActive()
	{
		return s_mDCO_FactionComp && !s_mDCO_FactionComp.IsEmpty();
	}

	static DCO_GlobalAIComponent GetForFaction(string factionKey)
	{
		if (factionKey.IsEmpty() || !DCO_FactionActive())
			return GetInstance();
		DCO_GlobalAIComponent comp;
		if (s_mDCO_FactionComp.Find(factionKey, comp) && comp)
			return comp;
		return GetInstance();
	}

	static DCO_GlobalAIComponent GetFor(IEntity entity)
	{
		if (!entity || !DCO_FactionActive())
			return GetInstance();
		return GetForFaction(DCO_FactionOf(entity));
	}

	static DCO_GlobalAIComponent GetForUtility(SCR_AIUtilityComponent utility)
	{
		if (!utility || !DCO_FactionActive())
			return GetInstance();
		return GetFor(utility.m_OwnerEntity);
	}

	static DCO_GlobalAIComponent GetForCommander(AICommander_BaseComponent cmd)
	{
		if (!cmd || !DCO_FactionActive())
			return GetInstance();
		return GetForFaction(cmd.GetCommanderFactionKey());
	}

	static DCO_GlobalAIComponent GetForFactionObj(Faction faction)
	{
		if (!faction || !DCO_FactionActive())
			return GetInstance();
		return GetForFaction(faction.GetFactionKey());
	}

	static IEntity DCO_OwnerOf(ScriptComponent component)
	{
		if (!component)
			return null;
		return component.GetOwner();
	}

	static bool BehaviorOnUtility(SCR_AIUtilityComponent utility, DCO_EBehavior bit)
	{
		DCO_GlobalAIComponent g = GetForUtility(utility);
		if (g && (g.m_iBehaviorFlags & bit) == 0)
			return false;
		return !utility || DCO_GroupFeatures.BehaviorAllowedAgent(utility.GetAIAgent(), bit);
	}

	static bool BehaviorOnFor(IEntity entity, DCO_EBehavior bit)
	{
		DCO_GlobalAIComponent g = GetFor(entity);
		if (g && (g.m_iBehaviorFlags & bit) == 0)
			return false;
		return DCO_GroupFeatures.BehaviorAllowed(entity, bit);
	}

	static bool BehaviorOnCommander(AICommander_BaseComponent cmd, DCO_EBehavior bit)
	{
		DCO_GlobalAIComponent g = GetForCommander(cmd);
		return !g || (g.m_iBehaviorFlags & bit) != 0;
	}

	static string DCO_FactionOf(IEntity entity)
	{
		if (!entity)
			return string.Empty;
		SCR_AIGroup group = SCR_AIGroup.Cast(entity);
		if (group)
		{
			Faction groupFaction = group.GetFaction();
			if (groupFaction)
				return groupFaction.GetFactionKey();
			return string.Empty;
		}
		AIAgent agent = AIAgent.Cast(entity);
		if (agent && agent.GetControlledEntity())
			entity = agent.GetControlledEntity();
		FactionAffiliationComponent affiliation = FactionAffiliationComponent.Cast(entity.FindComponent(FactionAffiliationComponent));
		if (affiliation && affiliation.GetAffiliatedFaction())
			return affiliation.GetAffiliatedFaction().GetFactionKey();
		return string.Empty;
	}

	string DCO_ExportConfig()
	{
		m_CfgSave = new SCR_JsonSaveContext();
		m_iCfgMode = 1;
		SyncConfig();
		m_iCfgMode = 0;
		string json = m_CfgSave.ExportToString();
		m_CfgSave = null;
		return json;
	}

	void DCO_ImportConfig(string json)
	{
		SCR_JsonLoadContext ctx = new SCR_JsonLoadContext();
		if (!ctx.ImportFromString(json))
			return;
		m_iCfgMode = 0;
		m_CfgLoad = ctx;
		m_aCfgMissing.Clear();
		m_iCfgApplied = 0;
		SyncConfig();
		m_CfgLoad = null;
	}

	static DCO_Doctrine DCO_DoctrineFor(string factionKey, out string source)
	{
		DCO_Doctrine d;
		string builtinName = DCO_DoctrineStore.FactionDefault(factionKey);
		DCO_GlobalAIComponent gx = GetInstance();
		DCO_Doctrine gd;
		if (gx)
			gd = gx.DCO_GetDoctrine();
		bool globalExplicit = gd && (gd.m_sPreset != DCO_DoctrineStore.DefaultName() || gd.m_bModified);
		if (globalExplicit && !builtinName.IsEmpty())
		{
			d = gd.Copy();
			source = "global_explicit_over_builtin";
			DCO_Perf.Count("doctrine:global_over_builtin");
		}
		else if (!builtinName.IsEmpty())
		{
			bool found;
			d = DCO_DoctrineStore.Resolve(builtinName, found);
			source = "faction_builtin";
		}
		else
		{
			DCO_GlobalAIComponent g = GetInstance();
			if (g)
				d = g.DCO_GetDoctrine().Copy();
			else
			{
				bool fallbackFound;
				d = DCO_DoctrineStore.Resolve(DCO_DoctrineStore.DefaultName(), fallbackFound);
			}
			source = "global";
		}
		map<string, vector> overrides;
		if (s_mDCO_Overrides && s_mDCO_Overrides.Find(factionKey, overrides) && overrides)
		{
			bool any;
			foreach (string className, vector value : overrides)
			{
				if (DCO_DoctrineGM.IsDoctrineClass(className))
					any = true;
			}
			if (any)
			{
				d = DCO_DoctrineGM.ApplyOverrides(d, overrides);
				source = source + "+faction_gm";
			}
		}
		return d;
	}

	static void DCO_GetOverrides(string factionKey, notnull array<string> outClasses, notnull array<vector> outValues)
	{
		if (!s_mDCO_Overrides)
			return;
		map<string, vector> overrides;
		if (!s_mDCO_Overrides.Find(factionKey, overrides) || !overrides)
			return;
		foreach (string className, vector value : overrides)
		{
			outClasses.Insert(className);
			outValues.Insert(value);
		}
	}

	static void DCO_SetOverrides(string factionKey, notnull array<string> classes, notnull array<vector> values, SCR_AttributesManagerEditorComponent manager)
	{
		if (factionKey.IsEmpty())
			return;
		if (!s_mDCO_Overrides)
			s_mDCO_Overrides = new map<string, ref map<string, vector>>();
		map<string, vector> overrides = new map<string, vector>();
		foreach (int i, string className : classes)
		{
			if (i < values.Count())
				overrides.Set(className, values[i]);
		}
		s_mDCO_Overrides.Set(factionKey, overrides);
		DCO_BumpDoc();
		DCO_Rebuild(factionKey, manager);
	}

	static void DCO_RebuildAll(SCR_AttributesManagerEditorComponent manager)
	{
		array<string> keys = {};
		DCO_CfgFactionKeys(keys);
		if (s_mDCO_Overrides)
		{
			foreach (string key, map<string, vector> overrides : s_mDCO_Overrides)
			{
				if (!keys.Contains(key))
					keys.Insert(key);
			}
		}
		foreach (string key : keys)
		{
			DCO_Rebuild(key, manager);
		}
	}

	protected static void DCO_Rebuild(string factionKey, SCR_AttributesManagerEditorComponent manager)
	{
		map<string, vector> overrides;
		bool hasOv = s_mDCO_Overrides && s_mDCO_Overrides.Find(factionKey, overrides) && overrides && !overrides.IsEmpty();
		if (!hasOv && !DCO_CfgHasFaction(factionKey))
		{
			DCO_RemoveFaction(factionKey);
			return;
		}
		DCO_GlobalAIComponent base = GetInstance();
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (!base || !gameMode)
		{
			DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.WARN, "-", string.Format("faction_layer_skip faction=%1 reason=no_global_or_gamemode", factionKey));
			return;
		}
		IEntity holder = DCO_EnsureHolder(factionKey);
		if (!holder)
		{
			DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.WARN, "-", string.Format("faction_layer_skip faction=%1 reason=holder_prefab", factionKey));
			return;
		}
		DCO_GlobalAIComponent comp = DCO_GlobalAIComponent.Cast(holder.FindComponent(DCO_GlobalAIComponent));
		if (!comp)
			return;

		comp.DCO_ImportConfig(base.DCO_ExportConfig());
		if (manager)
		{
			array<SCR_BaseEditorAttribute> attributes = {};
			manager.DCO_FactionAttributes(attributes);
			foreach (SCR_BaseEditorAttribute attribute : attributes)
			{
				SCR_BaseEditorAttributeVar var = attribute.ReadVariable(gameMode, manager);
				if (var)
					attribute.WriteVariable(holder, SCR_BaseEditorAttributeVar.CreateVector(var.GetVector()), manager, -1);
			}
		}
		comp.DCO_CfgApplyFaction(factionKey);
		if (manager && hasOv)
		{
			foreach (string className, vector value : overrides)
			{
				SCR_BaseEditorAttribute ovAttr = manager.DCO_AttributeByClass(className);
				if (ovAttr && ovAttr.ReadVariable(holder, manager))
					ovAttr.WriteVariable(holder, SCR_BaseEditorAttributeVar.CreateVector(value), manager, -1);
			}
		}
		if (!s_mDCO_FactionComp)
			s_mDCO_FactionComp = new map<string, DCO_GlobalAIComponent>();
		s_mDCO_FactionComp.Set(factionKey, comp);
	}

	override protected void DCO_OnCfgLoaded()
	{
		if (m_bDCO_Faction || !Replication.IsServer())
			return;
		GetGame().GetCallqueue().CallLater(DCO_BuildCfgFactions, 200, false, 0);
	}

	static void DCO_BuildCfgFactions(int attempt)
	{
		if (!GetInstance() || !GetGame().GetGameMode() || !GetGame().GetFactionManager())
		{
			if (attempt < 10)
			{
				DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", string.Format("cfg_faction_layers_wait attempt=%1 (game mode or faction manager not ready, retry in 1 s)", attempt + 1));
				GetGame().GetCallqueue().CallLater(DCO_BuildCfgFactions, 1000, false, attempt + 1);
			}
			else
				DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.ERROR, "-", "cfg_faction_layers_failed reason=not_ready attempts=10 (JSON faction blocks not applied this session)");
			return;
		}
		DCO_CfgValidateFactions();
		array<string> keys = {};
		DCO_CfgFactionKeys(keys);
		foreach (string key : keys)
			DCO_Rebuild(key, null);
		DCO_BumpDoc();
		DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", string.Format("cfg_faction_layers_built n=%1 active=%2", keys.Count(), DCO_FactionActive()));
		if (!keys.IsEmpty() && !DCO_FactionActive())
			DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.ERROR, "-", "cfg_faction_layers_failed reason=rebuild (see faction_layer_skip)");
	}

	protected static IEntity DCO_EnsureHolder(string factionKey)
	{
		if (!s_mDCO_FactionEntity)
			s_mDCO_FactionEntity = new map<string, IEntity>();
		IEntity holder;
		if (s_mDCO_FactionEntity.Find(factionKey, holder) && holder)
			return holder;
		Resource res = Resource.Load("{3AEAEDAE5BFEB40D}Prefabs/DCO/DCO_FactionSettings.et");
		if (!res || !res.IsValid())
			return null;
		EntitySpawnParams spawn = new EntitySpawnParams();
		spawn.TransformMode = ETransformMode.WORLD;
		s_bDCO_SpawnFaction = true;
		holder = GetGame().SpawnEntityPrefab(res, GetGame().GetWorld(), spawn);
		s_bDCO_SpawnFaction = false;
		if (holder)
			s_mDCO_FactionEntity.Set(factionKey, holder);
		return holder;
	}

	protected static void DCO_RemoveFaction(string factionKey)
	{
		if (s_mDCO_FactionComp)
			s_mDCO_FactionComp.Remove(factionKey);
		if (!s_mDCO_FactionEntity)
			return;
		IEntity holder;
		if (s_mDCO_FactionEntity.Find(factionKey, holder) && holder)
			SCR_EntityHelper.DeleteEntityAndChildren(holder);
		s_mDCO_FactionEntity.Remove(factionKey);
	}
}
