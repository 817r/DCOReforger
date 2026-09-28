class DCO_PreemptionUtility
{
	static float GetTaskWeight(DCO_GroupUtilityComponent grp)
	{
		if (grp.IsArmor())
			return 1.0;

		switch (grp.GetTask())
		{
			case DCO_EGroupTask.NONE:				return 0.0;
			case DCO_EGroupTask.PATROL:				return 0.1;
			case DCO_EGroupTask.RECON:				return 0.45;
			case DCO_EGroupTask.REINFORCE:			return 0.6;
			case DCO_EGroupTask.DEFEND:				return 0.7;
			case DCO_EGroupTask.GARRISON:			return 0.7;
			case DCO_EGroupTask.SUPPORT_BY_FIRE:	return 0.8;
			case DCO_EGroupTask.FLANK:				return 0.9;
			case DCO_EGroupTask.ATTACK:				return 1.0;
		}
		return 1.0;
	}

	static bool IsHardProtected(DCO_GroupUtilityComponent grp)
	{
		return grp.IsInTransport() || grp.IsMortar() || grp.HasState(DCO_EGroupState.RETREATING) || grp.GetTask() == DCO_EGroupTask.TRANSPORT || grp.GetSupportPlayerGroup() >= 0;
	}

	static float ComputeTaskValue(float currentObjScore, DCO_GroupUtilityComponent grp)
	{
		if (currentObjScore <= 0.0)
			return 0.0;

		return currentObjScore * GetTaskWeight(grp);
	}

	static bool IsWorthPreempting(float newObjScore, float currentTaskValue, float margin)
	{
		if (newObjScore <= 0.0)
			return false;

		if (currentTaskValue <= 0.0)
			return true;

		return newObjScore > (currentTaskValue * margin);
	}
}
