modded class SCR_BaseTaskManager
{
	void UpdateTaskInformation(SCR_BaseTask task, string title, string description, vector position)
	{
		task.SetTitle(title);
		task.SetDescription(description);
		task.SetOrigin(position);	
		Rpc(RPC_UpdateTaskInformation, task.GetTaskID(), title, description, position);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_UpdateTaskInformation(int taskID, string title, string description, vector position)
	{
		SCR_BaseTask task = SCR_BaseTask.Cast(GetTask(taskID));
		
		if(!task)
		{
			PrintFormat("TrainWreck: Task with ID(%1) does not exist", taskID);
			return;
		}
		
		task.SetTitle(title);
		task.SetDescription(description);
		task.SetOrigin(position);
	}
};