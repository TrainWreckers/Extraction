[EntityEditorProps(category: "GameScripted/TrainWreck/Systems", description: "Extraction Point for players")]
class SCR_TW_ExtractionSiteClass: SCR_SiteSlotEntityClass
{
	
};

class SCR_TW_ExtractionSite : SCR_SiteSlotEntity
{
	[Attribute("", UIWidgets.ResourcePickerThumbnail, category: "Prefabs", params: "et", desc: "Composition prefabs")]
	private ref array<ResourceName> extractionSitePrefabs;
	
	[Attribute("", UIWidgets.Auto, category: "Extraction States", params: "conf class=TW_ExtractionWorkflow", desc: "In order, the states players must go through")]
	private ref array<ref TW_ExtractionWorkflow> m_ExtractionWorkflows;
	
	private ref array<ref TW_ExtractionBase> m_ExtractionStates;
	
	[Attribute("", UIWidgets.ResourceNamePicker, category: "Tasks", params: "et", desc: "Trigger to spawn")]
	private ResourceName m_TriggerPrefab;
	
	private SCR_TW_TriggerArea m_Trigger;
	private bool m_Started = false;
	
	private ref TW_ExtractionBase m_CurrentStage;
	
	void SpawnSite()
	{
		if(IsOccupied())
			return;
		
		if(extractionSitePrefabs.IsEmpty())
			return;
		
		ResourceName randomPrefab = extractionSitePrefabs.GetRandomElement();
		
		if(randomPrefab == ResourceName.Empty)
			return;
		
		Resource resource = Resource.Load(randomPrefab);
		if(!resource.IsValid())
			return;
		
		IEntity prefab = SpawnEntityInSlot(resource);
		
		Resource triggerResource = Resource.Load(m_TriggerPrefab);
		EntitySpawnParams params = EntitySpawnParams();
		GetTransform(params.Transform);
		m_Trigger = SCR_TW_TriggerArea.Cast(GetGame().SpawnEntityPrefab(triggerResource, GetGame().GetWorld(), params));		
		m_Trigger.OnTriggered.Insert(Start);
		
		if(!m_Trigger)		
			Debug.Error("Was unable to locate a trigger for extraction area");			
				
	}
	
	private void CheckStageAtInterval()
	{		
		// Can't check something that isn't set 
		if(!m_CurrentStage || m_CurrentStage.IsPending())
		{
			Print(string.Format("TrainWreck: Current stage(%1) is pending or null %2", m_CurrentStage.GetStateName(), m_CurrentStage), LogLevel.WARNING);
			return;
		}
		
		// If we're not complete -- invoke the update method on the stage 
		if(!m_CurrentStage.IsComplete())
		{
			if(m_CurrentStage.ShouldUpdateAtInterval())
			{
				Print(string.Format("TrainWreck: calling update function on stage(%1", m_CurrentStage.GetStateName()));
				m_CurrentStage.OnUpdate();		
				GetGame().GetCallqueue().CallLater(CheckStageAtInterval, SCR_TW_Util.FromSecondsToMilliseconds(m_CurrentStage.GetUpdateInterval()), false);
			}
			else 
				GetGame().GetCallqueue().CallLater(CheckStageAtInterval, SCR_TW_Util.FromSecondsToMilliseconds(15), false);
		}
		// Completed - move forward 
		else if(!m_ExtractionStates.IsEmpty())
		{
			NextStage();
		}
		else
		{
			Print("TrainWreck: Nothing left to do");
		}
	}
	
	private void NextStage()
	{
		if(m_ExtractionStates.IsEmpty())
		{
			// If there's nothing left in the event list 
			// We should scrub this extraction site 
			IEntity occupant = GetOccupant();
			
			if(!occupant)
				Print("TrainWreck: Was unable to cleanup extraction site. Occupant is null", LogLevel.ERROR);
			else
				SCR_EntityHelper.DeleteEntityAndChildren(occupant);
			
			return;
		}
		
		// Pop
		m_CurrentStage = m_ExtractionStates.Get(0);
		m_ExtractionStates.Remove(0);
		
		// Continue forward
		StartStage();
	}
	
	//! Start the current stage, activate whatever updates need to happen overtime
	private void StartStage()
	{
		if(!m_CurrentStage)
			return;
		
		Print(string.Format("TrainWreck: Stage %1 starting", m_CurrentStage.GetStateName()));
		
		// We don't care about stages that have completed
		if(m_CurrentStage.IsComplete())
			return;
		
		if(m_CurrentStage.IsPending())
		{
			m_CurrentStage.OnStateActivated(this);
			CheckStageAtInterval();
		}
	}
	
	private void Start()
	{
		Print("TrainWreck: Starting event cycle");
		
		// Just need 1 player to start the process 
		// So to prevent multiple activations - clear the script invoker 
		m_Trigger.OnTriggered.Clear();
		
		// If we've already started - exit
		if(m_Started)
			return;
		
		m_Started = true;
		
		// Start the process
		NextStage();
	}
	
	
	void ~SCR_TW_ExtractionSite()
	{
		if(!m_Trigger)
			return;
		
		m_Trigger.OnTriggered.Clear();
	}
	
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		
		if(!TW_Global.IsInRuntime())
			return;
		
		SCR_TW_ExtractionHandler.GetInstance().RegisterExtractionSite(this);				
	}
};