class DCO_SaHazardQuery
{
	IEntity m_Hit;

	bool OnEntity(IEntity e)
	{
		if (!e || !e.FindComponent(SCR_SpecialCollisionHandlerComponent))
			return true;
		m_Hit = e;
		return false;
	}

	static IEntity Find(vector pos, float radius)
	{
		DCO_SaHazardQuery q = new DCO_SaHazardQuery();
		DCO_Perf.Count("q:DCO_SaHazard");
		GetGame().GetWorld().QueryEntitiesBySphere(pos, radius, q.OnEntity);
		return q.m_Hit;
	}
}

modded class SCR_AIPerformActionBehavior
{
	override void OnActionSelected()
	{
		super.OnActionSelected();
		if (!m_Utility || !m_SmartActionComponent || !m_SmartActionComponent.m_Value)
			return;
		if (DCO_Stress.T(m_Utility, "sa.hazard_skip", 1.0) <= 0)
			return;
		AISmartActionComponent sa = AISmartActionComponent.Cast(m_SmartActionComponent.m_Value);
		if (!sa || !sa.GetOwner())
			return;
		IEntity owner = sa.GetOwner();
		vector pos = owner.CoordToParent(sa.GetActionOffset());
		IEntity hit = DCO_SaHazardQuery.Find(pos, DCO_Stress.T(m_Utility, "sa.hazard_r", 1.5));
		if (!hit)
		{
			DCO_Perf.Count("sa:hazard_clear");
			return;
		}
		sa.SetActionAccessible(false);
		DCO_Perf.Count("sa:hazard_skip");
		string obj = hit.ClassName();
		if (hit.GetPrefabData() && !hit.GetPrefabData().GetPrefabName().IsEmpty())
			obj = FilePath.StripExtension(FilePath.StripPath(hit.GetPrefabData().GetPrefabName()));
		else
		{
			VObject vo = hit.GetVObject();
			if (vo)
				obj = obj + ":" + FilePath.StripExtension(FilePath.StripPath(vo.GetResourceName()));
		}
		DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(m_Utility.m_OwnerEntity), string.Format("sa_hazard_skip unit=%1 sa_pos=%2 hazard=%3 hazard_d=%4 owner=%5",
			DCO_Log.IdUnit(m_Utility.m_OwnerEntity), DCO_Util.XZ(pos), obj, Math.Round(vector.DistanceXZ(pos, hit.GetOrigin()) * 10) / 10, owner.ClassName()));
		Fail();
	}
}
