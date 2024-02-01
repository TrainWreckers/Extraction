[EntityEditorProps(category: "GameScripted/TrainWreck", description: "Base functionality for triggers")]
class SCR_TW_TriggerAreaClass : BaseGameTriggerEntityClass {};

class SCR_TW_TriggerArea : BaseGameTriggerEntity 
{
	ref ScriptInvoker OnTriggered = new ScriptInvoker();
	ref ScriptInvokerEntity OnEntered = new ScriptInvokerEntity();
	ref ScriptInvokerEntity OnExited = new ScriptInvokerEntity();
	
	event override void OnActivate(IEntity ent)
	{
		Print("TrainWreck: Triggered");
		if(OnTriggered)
			OnTriggered.Invoke();
		
		if(OnEntered)
			OnEntered.Invoke(ent);
	}
	
	event override void OnDeactivate(IEntity ent)
	{
		if(OnExited)
			OnExited.Invoke(ent);		
	}
	
	void ~SCR_TW_TriggerArea()
	{
		if(OnTriggered) OnTriggered.Clear();
		if(OnEntered) OnEntered.Clear();
		if(OnExited) OnExited.Clear();
	}
};