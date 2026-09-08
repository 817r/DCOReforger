class DCO_PreemptionUtility
{
	static float GetRoleWeight(CMD_EGroupRole role)
	{
		if (role == CMD_EGroupRole.NONE)
			return 0.0;

		if (role == CMD_EGroupRole.RESERVE)
			return 0.1;

		if (role == CMD_EGroupRole.RECON)
			return 0.45;

		if (role == CMD_EGroupRole.REINFORNCE)
			return 0.6;

		if (role == CMD_EGroupRole.DEFEND)
			return 0.7;

		if (role == CMD_EGroupRole.SUPPRESS)
			return 0.8;

		if (role == CMD_EGroupRole.FLANK)
			return 0.9;

		if (role == CMD_EGroupRole.ASSAULT)
			return 1.0;

		if (role == CMD_EGroupRole.ARMORED)
			return 1.0;

		return 1.0;
	}

	static bool IsRoleHardProtected(CMD_EGroupRole role)
	{
		if (role == CMD_EGroupRole.TRANSPORT)
			return true;

		if (role == CMD_EGroupRole.ARTILLERY)
			return true;

		if (role == CMD_EGroupRole.RETREAT)
			return true;

		return false;
	}

	static float ComputeTaskValue(float currentObjScore, CMD_EGroupRole role)
	{
		if (currentObjScore <= 0.0)
			return 0.0;

		return currentObjScore * GetRoleWeight(role);
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