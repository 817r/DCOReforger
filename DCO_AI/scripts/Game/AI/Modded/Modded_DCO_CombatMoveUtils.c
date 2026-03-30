class DCO_CombatMoveUtility
{
	static bool IsAimingAndMovementPossible(ECharacterStance stance, EMovementType moveType, SCR_EAICombatMoveDirection dir)
	{
		if (stance == ECharacterStance.PRONE)
			return false;
		
		if (moveType == EMovementType.SPRINT)
			return false;
		
		if (dir == SCR_EAICombatMoveDirection.BACKWARD)
			return false;
		
		return true;
	}
}	

