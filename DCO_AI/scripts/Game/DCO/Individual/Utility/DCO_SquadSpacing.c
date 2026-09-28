class DCO_SquadSpacing
{
	static const float MIN_SPACING = 5.0;

	protected static ref array<AIAgent> s_aAgents = {};

	static void CollectMatePositions(AIAgent owner, notnull array<vector> outPositions)
	{
		outPositions.Clear();
		AIGroup group = owner.GetParentGroup();
		if (!group)
			return;

		s_aAgents.Clear();
		group.GetAgents(s_aAgents);
		foreach (AIAgent mate : s_aAgents)
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
