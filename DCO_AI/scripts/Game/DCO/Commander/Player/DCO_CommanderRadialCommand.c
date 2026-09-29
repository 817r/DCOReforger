enum DCO_ERadialCommandType
{
	REPORT_INFANTRY_TEAM,
	REPORT_INFANTRY_SQUAD,
	REPORT_INFANTRY_PLATOON,
	REPORT_VEHICLE,
	REPORT_ARMOR,
	REQUEST_SUPPRESS,
	REQUEST_SUPPORT,
	REQUEST_FLANK_LEFT,
	REQUEST_FLANK_RIGHT,
	REQUEST_TRANSPORT,
	REQUEST_EXTRACT,
	REQUEST_MEDEVAC,
	ROLE_ASSAULT,
	ROLE_FIX,
	ROLE_BLOCK,
	ROLE_RECON,
	ROLE_REQUEST_OTHER,
	ACCEPT_TRANSPORT,
	REQUEST_ARMOR,
	FIRE_HE,
	FIRE_SMOKE,
	FIRE_ILLUM,
	CANCEL_SUPPORT,
	CANCEL_FIRE,
	CANCEL_TRANSPORT,
	CANCEL_ALL
}

[BaseContainerProps(), SCR_BaseGroupCommandTitleField("m_sCommandName")]
class DCO_CommanderRadialCommand : SCR_BaseGroupCommand
{
	[Attribute("0", UIWidgets.ComboBox, "Jenis perintah", "", ParamEnumArray.FromEnum(DCO_ERadialCommandType))]
	protected DCO_ERadialCommandType m_eType;

	[Attribute("0", UIWidgets.EditBox, "Jumlah peluru yang diminta (fire support). 0 = default jenis peluru.")]
	protected int m_iShellCount;

	override bool Execute(IEntity cursorTarget, IEntity groupEnt, vector targetPosition, int playerID, bool isClient)
	{
		if (isClient)
			return true;

		switch (m_eType)
		{
			case DCO_ERadialCommandType.REPORT_INFANTRY_TEAM:		DCO_PlayerContactReports.ReportFromRadial(playerID, targetPosition, 4, false); break;
			case DCO_ERadialCommandType.REPORT_INFANTRY_SQUAD:		DCO_PlayerContactReports.ReportFromRadial(playerID, targetPosition, 8, false); break;
			case DCO_ERadialCommandType.REPORT_INFANTRY_PLATOON:	DCO_PlayerContactReports.ReportFromRadial(playerID, targetPosition, 24, false); break;
			case DCO_ERadialCommandType.REPORT_VEHICLE:				DCO_PlayerContactReports.ReportFromRadial(playerID, targetPosition, 4, false); break;
			case DCO_ERadialCommandType.REPORT_ARMOR:				DCO_PlayerContactReports.ReportFromRadial(playerID, targetPosition, 4, true); break;
			case DCO_ERadialCommandType.REQUEST_SUPPRESS:			DCO_PlayerRequests.Handle(playerID, DCO_EPlayerRequest.SUPPRESS, targetPosition); break;
			case DCO_ERadialCommandType.REQUEST_SUPPORT:			DCO_PlayerRequests.Handle(playerID, DCO_EPlayerRequest.SUPPORT, targetPosition); break;
			case DCO_ERadialCommandType.REQUEST_FLANK_LEFT:			DCO_PlayerRequests.Handle(playerID, DCO_EPlayerRequest.FLANK_LEFT, targetPosition); break;
			case DCO_ERadialCommandType.REQUEST_FLANK_RIGHT:		DCO_PlayerRequests.Handle(playerID, DCO_EPlayerRequest.FLANK_RIGHT, targetPosition); break;
			case DCO_ERadialCommandType.REQUEST_TRANSPORT:			DCO_Logistics.HandlePlayer(playerID, DCO_ELogiJob.DEPLOY, targetPosition); break;
			case DCO_ERadialCommandType.REQUEST_EXTRACT:			DCO_Logistics.HandlePlayer(playerID, DCO_ELogiJob.EXTRACT, targetPosition); break;
			case DCO_ERadialCommandType.REQUEST_MEDEVAC:			DCO_Logistics.HandlePlayer(playerID, DCO_ELogiJob.MEDEVAC, targetPosition); break;
			case DCO_ERadialCommandType.ROLE_REQUEST_OTHER:			DCO_PlayerTasking.HandleRadial(playerID, 0, true); break;
			case DCO_ERadialCommandType.ACCEPT_TRANSPORT:			DCO_PlayerTasking.HandleRadial(playerID, 0, false); break;
			case DCO_ERadialCommandType.REQUEST_ARMOR:				DCO_PlayerRequests.Handle(playerID, DCO_EPlayerRequest.ARMOR, targetPosition); break;
			case DCO_ERadialCommandType.FIRE_HE:					DCO_PlayerRequests.HandleFire(playerID, SCR_EAIArtilleryAmmoType.HIGH_EXPLOSIVE, targetPosition, m_iShellCount); break;
			case DCO_ERadialCommandType.FIRE_SMOKE:					DCO_PlayerRequests.HandleFire(playerID, SCR_EAIArtilleryAmmoType.SMOKE, targetPosition, m_iShellCount); break;
			case DCO_ERadialCommandType.FIRE_ILLUM:					DCO_PlayerRequests.HandleFire(playerID, SCR_EAIArtilleryAmmoType.ILLUMINATION, targetPosition, m_iShellCount); break;
			case DCO_ERadialCommandType.CANCEL_SUPPORT:				DCO_PlayerRequests.HandleCancel(playerID, true, false, false); break;
			case DCO_ERadialCommandType.CANCEL_FIRE:				DCO_PlayerRequests.HandleCancel(playerID, false, true, false); break;
			case DCO_ERadialCommandType.CANCEL_TRANSPORT:			DCO_PlayerRequests.HandleCancel(playerID, false, false, true); break;
			case DCO_ERadialCommandType.CANCEL_ALL:					DCO_PlayerRequests.HandleCancel(playerID, true, true, true); break;
			default:												DCO_PlayerTasking.HandleRadial(playerID, ToggleBit(), false); break;
		}
		return true;
	}

	protected int ToggleBit()
	{
		switch (m_eType)
		{
			case DCO_ERadialCommandType.ROLE_ASSAULT:	return 1;
			case DCO_ERadialCommandType.ROLE_FIX:		return 2;
			case DCO_ERadialCommandType.ROLE_BLOCK:		return 4;
			case DCO_ERadialCommandType.ROLE_RECON:		return 8;
		}
		return 0;
	}

	override string GetCommandDisplayName()
	{
		int bit = ToggleBit();
		if (bit == 0)
			return super.GetCommandDisplayName();

		string state = "ON";
		SCR_GroupsManagerComponent groups = SCR_GroupsManagerComponent.GetInstance();
		if (groups && GetGame().GetPlayerController())
		{
			SCR_AIGroup grp = groups.GetPlayerGroup(GetGame().GetPlayerController().GetPlayerId());
			if (grp && (grp.DCO_GetPlayerRoles() & bit) == 0)
				state = "OFF";
		}
		return m_sCommandDisplayName + ": " + state;
	}

	override bool CanBeShown()
	{
		return CanRoleShow() && CanBeShownInCurrentLifeState();
	}
}
