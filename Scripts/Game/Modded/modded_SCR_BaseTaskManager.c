modded class SCR_BaseTaskManager
{
	void DeleteLocalTask(SCR_BaseTask task)
	{		
		Rpc(RpcAsk_Owner_DeleteTask, task.GetTaskID());
		task.Remove();
		
		DeleteTask(task);		
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcAsk_Owner_DeleteTask(int taskId)
	{
		SCR_BaseTask task = GetTaskManager().GetTask(taskId);
		
		if(!task)
		{
			Print(string.Format("TrainWreck: Failed to find task %1 for deletion", taskId), LogLevel.ERROR);
			return;
		}
		
		task.Remove();
		DeleteTask(task);
		//SCR_EntityHelper.DeleteEntityAndChildren(task);
	}
	
	void UpdateTaskCompletion(int taskId, SCR_TaskState state)
	{
		Rpc(RpcAsk_Owner_UpdateTaskCompletion, taskId, state);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcAsk_Owner_UpdateTaskCompletion(int taskId, SCR_TaskState state)
	{
		SCR_BaseTask task = GetTaskManager().GetTask(taskId);
		
		if(!task)
		{
			Print(string.Format("TrainWreck: Unable to update local player task state(%1) to %2", taskId, state), LogLevel.ERROR);
			return;
		}
		
		task.SetState(state);
	}
	
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
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcAsk_All_SetTaskState(int taskId, SCR_TaskState state, bool shouldRemove)
	{
		SCR_BaseTask task = GetTaskManager().GetTask(taskId);
		
		if(!task)
		{
			Print(string.Format("TrainWreck: Was unable to find Task with ID: %1", taskId), LogLevel.ERROR);
			return;
		}
		
		task.SetState(state);
		
		if(shouldRemove)
			task.Remove();
	}
	
	void SetTaskState(int taskId, SCR_TaskState state, bool shouldRemove = false)
	{
		Rpc(RpcAsk_All_SetTaskState, taskId, state, shouldRemove);
	}		
};