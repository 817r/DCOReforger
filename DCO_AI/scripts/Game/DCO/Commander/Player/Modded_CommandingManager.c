modded class SCR_CommandingManagerComponent
{
	override void RequestCommandExecution(int commandIndex, RplId cursorTargetID, RplId groupRplID, vector targetPosition, int playerID, float seed)
	{
		string cmd = FindCommandNameFromIndex(commandIndex);
		DCO_Perf.Count("player_cmd:" + cmd);
		if (DCO_Log.On(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO))
		{
			string grp = "-";
			RplComponent rpl = RplComponent.Cast(Replication.FindItem(groupRplID));
			if (rpl && rpl.GetEntity())
				grp = DCO_Log.IdGroup(AIGroup.Cast(rpl.GetEntity()));
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, grp, string.Format("player_cmd player=%1 cmd=%2 grp=%3 pos=%4", playerID, cmd, grp, DCO_Log.Pos(targetPosition)));
		}
		super.RequestCommandExecution(commandIndex, cursorTargetID, groupRplID, targetPosition, playerID, seed);
	}
}
