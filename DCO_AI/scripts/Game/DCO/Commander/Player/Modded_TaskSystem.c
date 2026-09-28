modded class SCR_TaskSystem
{
	protected static const string DCO_TASK_PREFIX = "DCO_";
	protected static const ref array<string> DCO_COMPASS = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};

	override protected void OnInit()
	{
		super.OnInit();
		if (System.IsConsoleApp())
			return;

		SCR_Task.GetOnTaskAssigneeAdded().Insert(DCO_OnTaskAssigneeAdded);
		SCR_Task.GetOnTaskStateChanged().Insert(DCO_OnTaskStateChanged);
	}

	override protected void OnCleanup()
	{
		SCR_Task.GetOnTaskAssigneeAdded().Remove(DCO_OnTaskAssigneeAdded);
		SCR_Task.GetOnTaskStateChanged().Remove(DCO_OnTaskStateChanged);
		DCO_OrderBanner.Reset();
		super.OnCleanup();
	}

	protected void DCO_OnTaskAssigneeAdded(SCR_Task task, SCR_TaskExecutor executor, int requesterID)
	{
		SCR_TaskExecutorGroup grp = SCR_TaskExecutorGroup.Cast(executor);
		if (!grp || grp.GetGroupID() != DCO_GetLocalGroupID() || !DCO_IsCommanderTask(task))
			return;

		DCO_Banner("NEW ORDERS", task, DCO_DistanceText(task), Color.FromRGBA(243, 178, 49, 255), SCR_SoundEvent.TASK_CREATED);
	}

	protected void DCO_OnTaskStateChanged(SCR_Task task, SCR_ETaskState newState)
	{
		if (!DCO_IsCommanderTask(task))
			return;

		int gid = DCO_GetLocalGroupID();
		if (gid < 0 || !task.IsTaskAssignedTo(SCR_TaskSystem.TaskExecutorFromGroup(gid)))
			return;

		switch (newState)
		{
			case SCR_ETaskState.COMPLETED:
				DCO_Banner("ORDER COMPLETE", task, "", Color.FromRGBA(110, 214, 96, 255), SCR_SoundEvent.TASK_SUCCEED);
				break;
			case SCR_ETaskState.FAILED:
				DCO_Banner("ORDER FAILED", task, "", Color.FromRGBA(222, 72, 60, 255), SCR_SoundEvent.TASK_FAILED);
				break;
			case SCR_ETaskState.CANCELLED:
				DCO_Banner("ORDER CANCELLED", task, "", Color.FromRGBA(160, 160, 160, 255), SCR_SoundEvent.TASK_CANCELED);
				break;
		}
	}

	protected bool DCO_IsCommanderTask(SCR_Task task)
	{
		return task && task.GetTaskID().StartsWith(DCO_TASK_PREFIX);
	}

	protected int DCO_GetLocalGroupID()
	{
		SCR_PlayerControllerGroupComponent comp = SCR_PlayerControllerGroupComponent.GetLocalPlayerControllerGroupComponent();
		if (!comp)
			return -1;
		return comp.GetGroupID();
	}

	protected string DCO_DistanceText(SCR_Task task)
	{
		IEntity me = SCR_PlayerController.GetLocalControlledEntity();
		if (!me)
			return string.Empty;

		vector dir = task.GetTaskPosition() - me.GetOrigin();
		float yaw = Math.Atan2(dir[0], dir[2]) * Math.RAD2DEG;
		if (yaw < 0)
			yaw += 360;

		int dist = Math.Round(vector.DistanceXZ(task.GetTaskPosition(), me.GetOrigin()) / 10) * 10;
		int sector = Math.Round(yaw / 45);
		return string.Format("%1 m  ·  %2", dist, DCO_COMPASS[sector % 8]);
	}

	protected void DCO_Banner(string header, SCR_Task task, string sub, Color accent, string sound)
	{
		string title = task.GetTaskName();
		title.ToUpper();
		DCO_OrderBanner.Show(header, title, sub, task.GetTaskIconSetName(), accent, sound);
	}
}
