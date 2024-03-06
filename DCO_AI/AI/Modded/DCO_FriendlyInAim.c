modded class SCR_AIIsFriendlyInAim : DecoratorScripted
{
	protected const string PORT_OUT_FRIENDLY_IN_AIM_IS_MOVING = "FriendlyInAimIsMoving";
	
	override protected void OnInit(AIAgent owner)
	{
		super.OnInit(owner);
	}
	
	protected override bool TestFunction(AIAgent owner)
	{
		bool friendlyInAimIsMoving;
		
		IEntity friendlyInLineOfFire;
		
		if (m_PerceptionComponent)
		{
			friendlyInLineOfFire = m_PerceptionComponent.GetFriendlyInLineOfFire();

			if (friendlyInLineOfFire)
			{
				CharacterControllerComponent characterControllerComponent = CharacterControllerComponent.Cast(friendlyInLineOfFire.FindComponent(CharacterControllerComponent));
				
				if (characterControllerComponent)
				{
					int currentMovementPhase = characterControllerComponent.GetCurrentMovementPhase();
					
					if (currentMovementPhase > EMovementType.WALK)
						friendlyInAimIsMoving = true;
				}
			}
		}
		
		SetVariableOut(PORT_OUT_FRIENDLY_IN_AIM_IS_MOVING, friendlyInAimIsMoving);
		
		return friendlyInLineOfFire != null;
	}

	protected static ref TStringArray s_aVarsOut =
	{
		PORT_OUT_FRIENDLY_IN_AIM_IS_MOVING
	};
	
	override TStringArray GetVariablesOut()
	{
		return s_aVarsOut;
	}
};