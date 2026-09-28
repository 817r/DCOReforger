class SCR_AIFindPositionWithMagazineWell : AITaskScripted
{
	protected static const string PORT_POS = "Pos";
	protected static const string PORT_PREFAB_RESOURCE_NAME = "MagazineWell";

	protected static const string PORT_ARSENAL_ENTITY = "ArsenalEntity";

	[Attribute("50", UIWidgets.EditBox)]
	protected float m_fSearchRadius;

	protected ref array<IEntity> m_aQueryFoundEntities = {};
	protected typename prefabResourceName;

	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		int pt = DCO_Perf.Begin();
		ENodeResult r = DCO_SimulateFindMagazine(owner, dt);
		DCO_Perf.End("bt_find_magazine", pt);
		return r;
	}

	protected ENodeResult DCO_SimulateFindMagazine(AIAgent owner, float dt)
	{
		vector searchPos;

		GetVariableIn(PORT_POS, searchPos);
		GetVariableIn(PORT_PREFAB_RESOURCE_NAME, prefabResourceName);

		if (searchPos == vector.Zero)
			return ENodeResult.FAIL;

		m_aQueryFoundEntities.Clear();
		DCO_Perf.Count("q:DCO_FindPositionWithMagazineWell");
		GetGame().GetWorld().QueryEntitiesBySphere(searchPos, m_fSearchRadius, QueryCallback);

		IEntity nearestEntity = null;
		float smallestDistSq = float.MAX;

		foreach (IEntity e : m_aQueryFoundEntities)
		{
			float distSq = vector.DistanceSq(e.GetOrigin(), searchPos);
			if (distSq < smallestDistSq)
			{
				nearestEntity = e;
				smallestDistSq = distSq;
			}
		}

		if (!nearestEntity)
		{
			return ENodeResult.FAIL;
		}

		SetVariableOut(PORT_ARSENAL_ENTITY, nearestEntity);
		return ENodeResult.SUCCESS;
	}

	bool QueryCallback(IEntity e)
	{
		array<IEntity> outItems = {};
		array<typename> components = {};
        components.Insert(MagazineComponent);
		SCR_InventoryStorageManagerComponent comp = SCR_InventoryStorageManagerComponent.Cast(e.FindComponent(SCR_InventoryStorageManagerComponent));
		SCR_CharacterPerceivableComponent perc = SCR_CharacterPerceivableComponent.Cast(e.FindComponent(SCR_CharacterPerceivableComponent));
		if (comp && perc.isDead)
		{
			comp.FindItemsWithComponents(outItems, components);
		}

		foreach (IEntity ent : outItems)
		{
			MagazineComponent magComp = MagazineComponent.Cast(ent.FindComponent(MagazineComponent));
			if (magComp)
			{
				if (!magComp.GetMagazineWell())
					continue;

				typename currMw = magComp.GetMagazineWell().Type();
				if (prefabResourceName == currMw)
				{
					m_aQueryFoundEntities.Insert(e);
				}
			}
		}

		return true;
	}

	override static bool VisibleInPalette() { return true; }

	override static string GetOnHoverDescription() { return "Finds nearest Entity which has a given prefab in it."; }

	protected static ref TStringArray s_aVarsIn = { PORT_POS, PORT_PREFAB_RESOURCE_NAME };
	override TStringArray GetVariablesIn() { return s_aVarsIn; }

	protected static ref TStringArray s_aVarsOut = { PORT_ARSENAL_ENTITY };
	override TStringArray GetVariablesOut() { return s_aVarsOut; }
}
