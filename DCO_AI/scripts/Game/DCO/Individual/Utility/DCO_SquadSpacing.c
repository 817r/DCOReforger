class DCO_SquadSpacing
{

	protected static ref array<AIAgent> s_aAgents;
	protected static array<AIAgent> GetAgentsStore()
	{
		if (!s_aAgents)
			s_aAgents = {};
		return s_aAgents;
	}

	static void CollectMatePositions(AIAgent owner, notnull array<vector> outPositions)
	{
		outPositions.Clear();
		AIGroup group = owner.GetParentGroup();
		if (!group)
			return;

		GetAgentsStore().Clear();
		GetAgentsStore();
		group.GetAgents(s_aAgents);
		foreach (AIAgent mate : GetAgentsStore())
		{
			if (!mate || mate == owner)
				continue;

			IEntity mateEntity = mate.GetControlledEntity();
			ChimeraCharacter character = ChimeraCharacter.Cast(mateEntity);
			if (!character || character.IsInVehicle())
				continue;

			SCR_AIUtilityComponent util = SCR_AIUtilityComponent.Cast(mate.FindComponent(SCR_AIUtilityComponent));
			SCR_AICoverLock cover;
			if (util && util.m_CombatMoveState)
				cover = util.m_CombatMoveState.GetAssignedCover();

			if (cover && cover.IsValid())
				outPositions.Insert(cover.GetPosition());
			else
				outPositions.Insert(mateEntity.GetOrigin());
		}
	}

	static float NearestDistance(vector pos, notnull array<vector> matePositions)
	{
		float best = float.MAX;
		foreach (vector matePos : matePositions)
			best = Math.Min(best, vector.Distance(pos, matePos));

		return best;
	}
}
