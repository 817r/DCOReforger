modded class SCR_AIGetCoverParameters : AITaskScripted
{
	override void CalculateCoverStance(float height, out ECharacterStance outStance, out ECharacterStance outStanceHide)
	{
		if (height > 0.85)
		{
			outStance = ECharacterStance.STAND;
			outStanceHide = ECharacterStance.CROUCH;
		}
		else if (height > 0.4)
		{
			outStance = ECharacterStance.CROUCH;
			outStanceHide = ECharacterStance.CROUCH;
		}
		else if (height > 0.2)
		{
			outStance = ECharacterStance.PRONE;
			outStanceHide = ECharacterStance.PRONE;		
		}
		else
		{
			outStance = ECharacterStance.CROUCH;
			outStanceHide = ECharacterStance.PRONE;
		}
	}
}