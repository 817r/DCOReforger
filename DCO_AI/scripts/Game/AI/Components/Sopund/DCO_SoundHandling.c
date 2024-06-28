/*mooded enum ECommunicationType
{
	NONE,
	REPORT_TARGET,
	REPORT_MOVE,
	REPORT_RETURN,
	REPORT_MOUNT,
	REPORT_MOUNT_AS,
	REPORT_MOUNT_NEAREST,
	REPORT_UNMOUNT,
	REPORT_STOP,
	REPORT_FLANK,
	REPORT_DEFEND,
	REPORT_CONTACT,	
	REPORT_NO_AMMO,
	REPORT_UNDER_FIRE,
	REPORT_CLEAR,
	REPORT_ENGAGING,
	REPORT_TARGET_DOWN,
	REPORT_TARGET_LOST,
	REPORT_RELOADING,
	REPORT_MOVING,
	REPORT_COVERING,
	REPORT_NEGATIVE,
	REPORT_SEARCH_AND_DESTROY,
	REPORT_IDLE,
}*/


modded class SCR_AISoundHandling
{
	static const int NORMAL_PRIORITY = 50;  
	static const int IMMEDIATE_PRIORITY = 70;
	static const int DISTANCE_TYPE_MID = 70;
	static const int DISTANCE_TYPE_LONG = 2000;	
	
	//--------------------------------------------------------------------------------------------
	override static bool SetSignalsAndTransmit(SCR_AITalkRequest request, IEntity speakerEntity, VoNComponent von, SignalsManagerComponent signalsMgr)
	{
		ref array<float> signals = {};
		SCR_CallsignCharacterComponent callsignComponent;
		int SN_Seed = signalsMgr.FindSignal("Seed");
		SetSignal(SN_Seed, Math.RandomFloat(0, 1), signals);
		if (request.m_Entity)
			 callsignComponent = SCR_CallsignCharacterComponent.Cast(request.m_Entity.FindComponent(SCR_CallsignCharacterComponent));
		ECommunicationType commType = request.m_eCommType;
		switch (commType)
		{
			case ECommunicationType.REPORT_TARGET:
			{
				if (!request.m_Entity)
				{
					ErrorMissingEntity(commType);
					return false;
				}
				SetSignal_TargetValues(request.m_Entity, signalsMgr, signals);
				SetSignal_SoldierCalledValues(signalsMgr, SIGNAL_VALUE_SOLDIER_ALL, signals);
				von.SoundEventPriority(SCR_SoundEvent.SOUND_CP_TARGET, signals, IMMEDIATE_PRIORITY);
				break;
			}
			case ECommunicationType.REPORT_MOVE:
			{
				if (request.m_vPosition == vector.Zero)
				{
					ErrorMissingPosition(commType);
					return false;
				}
				SetSignal_SoldierCalledValues(signalsMgr, SIGNAL_VALUE_SOLDIER_ALL, signals);
				
				float distanceType = SetSignal_PositionValues(request.m_Entity, signalsMgr, request.m_vPosition, speakerEntity.GetOrigin(), signals);
				if (distanceType < DISTANCE_TYPE_MID) 														// close range distance
					von.SoundEventPriority(SCR_SoundEvent.SOUND_CP_MOVE_CLOSE,  signals, NORMAL_PRIORITY);
				else if (distanceType < DISTANCE_TYPE_LONG) 												// mid range distance
					von.SoundEventPriority(SCR_SoundEvent.SOUND_CP_MOVE_MID, signals, NORMAL_PRIORITY);
				else  																						// long range distance
					von.SoundEventPriority(SCR_SoundEvent.SOUND_CP_MOVE_LONG, signals, NORMAL_PRIORITY);
				
				break;
			}
			case ECommunicationType.REPORT_CONTACT:
			{
				if (!request.m_Entity || request.m_vPosition == vector.Zero)
				{
					ErrorMissingEntityOrPosition(commType);
					return false;
				}
				SetSignal_TargetFactionValues(request.m_Entity, signalsMgr, signals);			
				float distanceType = SetSignal_PositionValues(request.m_Entity, signalsMgr, request.m_vPosition, speakerEntity.GetOrigin(), signals);
				SetSignal_FactionValues(signalsMgr, 1, signals);
				if (distanceType < DISTANCE_TYPE_MID) 														
					von.SoundEventPriority(SCR_SoundEvent.SOUND_CP_SPOTTED_CLOSE,  signals, IMMEDIATE_PRIORITY, true);
				else if (distanceType < DISTANCE_TYPE_LONG) 												
					von.SoundEventPriority(SCR_SoundEvent.SOUND_CP_SPOTTED_MID, signals, IMMEDIATE_PRIORITY, true);
				else  																						
					von.SoundEventPriority(SCR_SoundEvent.SOUND_CP_SPOTTED_LONG, signals, NORMAL_PRIORITY);
				break;
			}
			case ECommunicationType.REPORT_RETURN:
			{
				SetSignal_SoldierCalledValues(signalsMgr, SIGNAL_VALUE_SOLDIER_ALL, signals);
				von.SoundEventPriority(SCR_SoundEvent.SOUND_CP_RETURN, signals, NORMAL_PRIORITY);
				break;
			}
			case ECommunicationType.REPORT_MOUNT:
			{
				if (!request.m_Entity)
				{
					ErrorMissingEntity(commType);
					return false;
				}
				SetSignal_TargetValues(request.m_Entity, signalsMgr, signals);
				SetSignal_SoldierCalledValues(signalsMgr, SIGNAL_VALUE_SOLDIER_ALL, signals);
				von.SoundEventPriority(SCR_SoundEvent.SOUND_CP_MOUNT_BOARD, signals, NORMAL_PRIORITY);
				break;
			}
			case ECommunicationType.REPORT_MOUNT_AS:
			{
				if (!callsignComponent)
				{
					ErrorMissingCallsign(commType);
					return false;
				}
				SetSignal_MountAsValues(signalsMgr, callsignComponent.GetCharacterCallsignIndex(), RoleInVehicleToSoundRoleEnum(request.m_EnumSignal), signals);
				von.SoundEventPriority(SCR_SoundEvent.SOUND_CP_MOUNT_ROLE, signals, NORMAL_PRIORITY);
				break;
			}
			case ECommunicationType.REPORT_MOUNT_NEAREST:
			{
				SetSignal_SoldierCalledValues(signalsMgr, SIGNAL_VALUE_SOLDIER_ALL, signals);
				von.SoundEventPriority(SCR_SoundEvent.SOUND_CP_MOUNT_NEAREST, signals, NORMAL_PRIORITY);
				break;
			}
			case ECommunicationType.REPORT_UNMOUNT:
			{
				SetSignal_SoldierCalledValues(signalsMgr, SIGNAL_VALUE_SOLDIER_ALL, signals);
				von.SoundEventPriority(SCR_SoundEvent.SOUND_CP_MOUNT_GETOUT, signals, NORMAL_PRIORITY);
				break;
			}
			case ECommunicationType.REPORT_STOP:
			{
				if (callsignComponent)
					SetSignal_SoldierCalledValues(signalsMgr, callsignComponent.GetCharacterCallsignIndex(), signals);
				von.SoundEventPriority(SCR_SoundEvent.SOUND_CP_STOP, signals, NORMAL_PRIORITY);
				break;
			}
			case ECommunicationType.REPORT_FLANK:
			{
				if (!callsignComponent)
				{
					ErrorMissingCallsign(commType);
					return false;
				}
				SetSignal_SoldierCalledValues(signalsMgr, callsignComponent.GetCharacterCallsignIndex(), signals);
				SetSignal_FlankValues(signalsMgr, request.m_EnumSignal, signals);
				von.SoundEventPriority(SCR_SoundEvent.SOUND_CP_FLANK, signals, NORMAL_PRIORITY);
				break;
			}
			case ECommunicationType.REPORT_SEARCH_AND_DESTROY:
			{
				SetSignal_SoldierCalledValues(signalsMgr, SIGNAL_VALUE_SOLDIER_ALL, signals);
				von.SoundEventPriority(SCR_SoundEvent.SOUND_CP_SEARCH_AND_DESTROY, signals, NORMAL_PRIORITY);
				break;
			}
			case ECommunicationType.REPORT_NO_AMMO:
			{
				von.SoundEventPriority(SCR_SoundEvent.SOUND_REPORTS_STATUS_NOAMMO, signals, NORMAL_PRIORITY);
				break;
			}
			case ECommunicationType.REPORT_CLEAR:
			{
				von.SoundEventPriority(SCR_SoundEvent.SOUND_REPORTS_STATUS_CLEAR, signals, NORMAL_PRIORITY);
				break;
			}
			case ECommunicationType.REPORT_ENGAGING:
			{
				von.SoundEventPriority(SCR_SoundEvent.SOUND_REPORTS_STATUS_ENGAGING, signals, NORMAL_PRIORITY);
				break;
			}
			case ECommunicationType.REPORT_TARGET_DOWN:
			{
				von.SoundEventPriority(SCR_SoundEvent.SOUND_REPORTS_STATUS_TARGETDOWN, signals, NORMAL_PRIORITY);
				break;
			}
			case ECommunicationType.REPORT_RELOADING:
			{
				von.SoundEventPriority(SCR_SoundEvent.SOUND_REPORTS_ACTIONSTATUS_RELOAD, signals, NORMAL_PRIORITY);
				break;
			}
			case ECommunicationType.REPORT_MOVING:
			{
				von.SoundEventPriority(SCR_SoundEvent.SOUND_REPORTS_ACTIONSTATUS_MOVE, signals, NORMAL_PRIORITY);
				break;
			}
			case ECommunicationType.REPORT_COVERING:
			{
				von.SoundEventPriority(SCR_SoundEvent.SOUND_REPORTS_ACTIONSTATUS_COVER, signals, NORMAL_PRIORITY);
				break;
			}
			case ECommunicationType.REPORT_DEFEND:
			{
				SetSignal_SoldierCalledValues(signalsMgr, SIGNAL_VALUE_SOLDIER_ALL, signals);
				von.SoundEventPriority(SCR_SoundEvent.SOUND_CP_DEFEND_POSITION, signals, NORMAL_PRIORITY);
				break;
			}
			case ECommunicationType.REPORT_NEGATIVE:
			{
				von.SoundEventPriority(SCR_SoundEvent.SOUND_CP_NEGATIVEFEEDBACK_NEGATIVE, signals, NORMAL_PRIORITY);
				break;
			}
			case ECommunicationType.REPORT_IDLE:
			{
				von.SoundEventPriority(SCR_SoundEvent.SOUND_CP_IDLE_SPEECH, signals, NORMAL_PRIORITY);
				break;
			}
			case ECommunicationType.REPORT_TARGET_LOST:
			{
				if (callsignComponent)
					SetSignal_SoldierCalledValues(signalsMgr, callsignComponent.GetCharacterCallsignIndex(), signals);
				von.SoundEventPriority(SCR_SoundEvent.SOUND_CP_REPORTS_AREACLEAR, signals, NORMAL_PRIORITY);
				break;
			}
			case ECommunicationType.REPORT_UNDER_FIRE:
			{
				if (callsignComponent)
					SetSignal_SoldierCalledValues(signalsMgr, callsignComponent.GetCharacterCallsignIndex(), signals);
				von.SoundEventPriority(SCR_SoundEvent.SOUND_CP_REPORTS_UNDER_FIRE, signals, IMMEDIATE_PRIORITY);
				break;
			}
		}
		return true;
	}
}