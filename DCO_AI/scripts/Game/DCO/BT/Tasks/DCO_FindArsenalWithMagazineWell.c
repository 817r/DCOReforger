class SCR_AIFindArsenalWithMagazineWell : AITaskScripted
{
	protected static const string PORT_POS = "Pos";
	protected static const string PORT_PREFAB_RESOURCE_NAME = "MagazineWellTypename";

	protected static const string PORT_ARSENAL_ENTITY = "ArsenalEntity";

	[Attribute("0", UIWidgets.EditBox)]
	protected float m_fSearchRadius;

	protected ref array<IEntity> m_aQueryFoundEntities = {};
	protected typename m_sQueryResourceName;

	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		int pt = DCO_Perf.Begin();
		ENodeResult r = DCO_SimulateFindArsenal(owner, dt);
		DCO_Perf.End("bt_find_arsenal", pt);
		return r;
	}

	protected ENodeResult DCO_SimulateFindArsenal(AIAgent owner, float dt)
	{
		vector searchPos;
		typename prefabResourceName;

		GetVariableIn(PORT_POS, searchPos);
		GetVariableIn(PORT_PREFAB_RESOURCE_NAME, prefabResourceName);

		if (searchPos == vector.Zero)
			return ENodeResult.FAIL;

		m_aQueryFoundEntities.Clear();
		m_sQueryResourceName = prefabResourceName;
		DCO_Perf.Count("q:DCO_FindArsenalWithMagazineWell");
		GetGame().GetWorld().QueryEntitiesBySphere(searchPos, m_fSearchRadius, QueryCallback);

		Print(m_aQueryFoundEntities.Count().ToString() + " < FOUND ARSENAL WITH MAGAZINE WELL > " + prefabResourceName.ToString());

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

		if (nearestEntity)
			Print(nearestEntity.Type().ToString() + " < FOUND NEAREST ARSENAL TO RESUPPLY > " + prefabResourceName.ToString());

		if (!nearestEntity)
			return ENodeResult.FAIL;

		SetVariableOut(PORT_ARSENAL_ENTITY, nearestEntity);
		return ENodeResult.SUCCESS;
	}

	bool QueryCallback(IEntity e)
	{
		SCR_ServicePointComponent comp = SCR_ServicePointComponent.Cast(e.FindComponent(SCR_ServicePointComponent));
		if (comp)
		{
			Print(e.Type().ToString() + " < FOUND SERVICE POINT TYPE > " + comp.GetType());
			if (comp.GetType() == SCR_EServicePointType.ARMORY)
				m_aQueryFoundEntities.Insert(e);
		}

		return true;
	}

	override static bool VisibleInPalette() { return true; }

	override static string GetOnHoverDescription() { return "Finds nearest arsenal which has a given prefab in it."; }

	protected static ref TStringArray s_aVarsIn = { PORT_POS, PORT_PREFAB_RESOURCE_NAME };
	override TStringArray GetVariablesIn() { return s_aVarsIn; }

	protected static ref TStringArray s_aVarsOut = { PORT_ARSENAL_ENTITY };
	override TStringArray GetVariablesOut() { return s_aVarsOut; }
}
