modded class SCR_AIObservePositionBehavior : SCR_AIBehaviorBase
{
	protected bool m_bHasBinoculars;
	
	protected vector m_ObservePosition;
	
	protected SCR_InventoryStorageManagerComponent m_InventoryStorageManagerComponent;

	override void InitParameters(vector position)
	{
		super.InitParameters(position);
	}
};

modded class SCR_AIObserveUnknownFireBehavior : SCR_AIObservePositionBehavior
{
	protected const float TIMEOUT_S = 5.0;
	protected const float DURATION_MIN_S = 1.0;			// Min duration of behavior
	protected const float DIRECTION_SPAN_DEG = 32.0;	
	protected const float DURATION_S_PER_METER = 0.005;	// How duration depends on distance
	protected const float USE_BINOCULARS_DISTANCE_THRESHOLD = 70;
	
	protected const float DELAY_MIN_S = 0.1;			// Min delay before we start looking at the position
	protected const float DELAY_S_PER_METER = 0.0001;	// How the delay increases depending on distance
	
	protected bool m_bHasBinoculars;
	
	protected SCR_GadgetManagerComponent m_GadgetManagerComponent;

	override void OnActionSelected()
	{
		super.OnActionSelected();
		
		IEntity ownerEntity = m_Utility.m_OwnerEntity;
		
		if (ownerEntity)
		{
			m_GadgetManagerComponent = SCR_GadgetManagerComponent.GetGadgetManager(ownerEntity);
			
			if (m_CharacterControllerComponent)
				m_CharacterControllerComponent.SetWeaponADS(false);
			
			if (m_GadgetManagerComponent)
			{
				IEntity item = m_GadgetManagerComponent.GetGadgetByType(EGadgetType.BINOCULARS);
			}
		}
	}

	override void OnActionDeselected()
	{
		super.OnActionDeselected();
		
		m_Utility.m_LookAction.LookAt(m_vPosition.m_Value, m_Utility.m_LookAction.PRIO_UNKNOWN_TARGET, 3.0);
	}
};