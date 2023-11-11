[EntityEditorProps(category: "GameScripted/TrainWreck", description: "Base functionality for triggers")]
class SCR_TW_TriggerAreaClass : GenericEntityClass {};

class SCR_TW_TriggerArea : GenericEntity 
{
	#ifdef WORKBENCH
	
	[Attribute(defvalue: "1", desc: "Show the debug shapes in Workbench", category: "Debug")]
	protected bool m_ShowDebugShapesInWorkbench;
	protected WorldEditorAPI m_API;
	protected IEntity m_PreviewEntity;
	
	#endif		
};