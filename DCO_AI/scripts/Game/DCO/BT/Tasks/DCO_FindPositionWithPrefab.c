class SCR_AIFindPositionWithPrefab : AITaskScripted
{


	[Attribute("0", UIWidgets.EditBox)]
	protected float m_fSearchRadius;

	protected ref array<IEntity> m_aQueryFoundEntities = {};
	protected ResourceName m_sQueryResourceName;

	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		vector searchPos;
		ResourceName prefabResourceName;

		GetVariableIn("Pos", searchPos);
		GetVariableIn("PrefabResourceName", prefabResourceName);

		if (searchPos == vector.Zero || prefabResourceName.IsEmpty())
			return ENodeResult.FAIL;

		m_aQueryFoundEntities.Clear();
		m_sQueryResourceName = prefabResourceName;
		DCO_Perf.Count("q:DCO_FindPositionWithPrefab");
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
			return ENodeResult.FAIL;

		SetVariableOut("ArsenalEntity", nearestEntity);
		return ENodeResult.SUCCESS;
	}

	bool QueryCallback(IEntity e)
	{
		SCR_InventoryStorageManagerComponent comp = SCR_InventoryStorageManagerComponent.Cast(e.FindComponent(SCR_InventoryStorageManagerComponent));
		SCR_CharacterPerceivableComponent perc = SCR_CharacterPerceivableComponent.Cast(e.FindComponent(SCR_CharacterPerceivableComponent));
		if (comp && SCR_InventoryStorageManagerComponent.IsItemInStorage(comp, m_sQueryResourceName, null) && perc.isDead)
			m_aQueryFoundEntities.Insert(e);

		return true;
	}

	override static bool VisibleInPalette() { return true; }

	override static string GetOnHoverDescription() { return "Finds nearest Entity which has a given prefab in it."; }

	protected static ref TStringArray s_aVarsIn;
	protected static TStringArray GetVarsIn()
	{
		if (!s_aVarsIn)
			s_aVarsIn = { "Pos", "PrefabResourceName" };
		return s_aVarsIn;
	}
	override TStringArray GetVariablesIn() { return GetVarsIn(); }

	protected static ref TStringArray s_aVarsOut;
	protected static TStringArray GetVarsOut()
	{
		if (!s_aVarsOut)
			s_aVarsOut = { "ArsenalEntity" };
		return s_aVarsOut;
	}
	override TStringArray GetVariablesOut() { return GetVarsOut(); }
}
