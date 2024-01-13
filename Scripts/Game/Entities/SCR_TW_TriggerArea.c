[EntityEditorProps(category: "GameScripted/TrainWreck", description: "Base functionality for triggers")]
class SCR_TW_TriggerAreaClass : BaseGameTriggerEntityClass {};

class SCR_TW_TriggerArea : BaseGameTriggerEntity 
{
	ref ScriptInvoker OnTriggered = new ScriptInvoker();
	
	event override void OnActivate(IEntity ent)
	{
		Print("TrainWreck: Triggered");
		if(OnTriggered)
			OnTriggered.Invoke();	
	}
	
	
};