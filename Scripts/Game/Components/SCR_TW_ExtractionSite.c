[EntityEditorProps(category: "GameScripted/TrainWreck/Systems", description: "Extraction Point for players")]
class SCR_TW_ExtractionSiteClass: SCR_SiteSlotEntityClass
{
	
};

class SCR_TW_ExtractionSite : SCR_SiteSlotEntity
{
	[Attribute("", UIWidgets.ResourcePickerThumbnail, category: "Prefabs", params: "et", desc: "Composition prefabs")]
	private ref array<ResourceName> extractionSitePrefabs;
	
	[Attribute("", UIWidgets.ResourceNamePicker, category: "Tasks", params: "et", desc: "Trigger to spawn")]
	private ResourceName m_TriggerPrefab;
	
	private SCR_TW_TriggerArea m_Trigger;
	
	[Attribute("", UIWidgets.Auto, category: "Extraction Workflows", params: "conf class=TW_ExtractionConfig")]
	private ref array<ref TW_ExtractionWorkflow> m_ExtractionWorkflows;	
	private ref array<ref TW_ExtractionConfig> m_ExtractionConfigs = {};
	private ref TW_ExtractionConfig m_CurrentStage;
	private bool m_Started = false;
	private RplComponent rpl = TW<RplComponent>.Find(this);
	
	//! Return trigger area from occupied extraction area/site (if available)
	SCR_TW_TriggerArea GetTriggerArea() { return m_Trigger; }
	
	bool UsesTrigger() { return m_TriggerPrefab != ResourceName.Empty; }
	
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
		
		// Should a trigger spawn here?
		if(m_TriggerPrefab == ResourceName.Empty)
			return;
		
		Resource triggerResource = Resource.Load(m_TriggerPrefab);
		EntitySpawnParams params = EntitySpawnParams();
		GetTransform(params.Transform);
		
		if(m_ExtractionWorkflows.IsEmpty())
		{
			Print("TrainWreck: No trigger needed - workflows not provided", LogLevel.WARNING);
			return;
		}
		
		TW_ExtractionWorkflow workflow = m_ExtractionWorkflows.GetRandomElement();
		foreach(TW_ExtractionConfig config : workflow.States)
			m_ExtractionConfigs.Insert(config);
		
		m_Trigger = SCR_TW_TriggerArea.Cast(GetGame().SpawnEntityPrefab(triggerResource, GetGame().GetWorld(), params));
		if(m_Trigger)
			m_Trigger.OnTriggered.Insert(Start);
		
		ResourceName taskPrefab = SCR_TW_ExtractionHandler.GetInstance().GetExtractionTaskPrefab();
		if(taskPrefab)
		{
			int taskX, taskY;
			SCR_TW_Util.GetCenterOfGridSquare(GetOrigin(), taskX, taskY);
			
			string description = SCR_TW_ExtractionHandler.GetInstance().GetExtractionDescription(GetOrigin());
			SCR_BaseTask task = SCR_BaseTask.Cast(GetTaskManager().SpawnTask(taskPrefab));
			task.SetTitle("Extract");
			task.SetDescription(description);
			vector center = GetOrigin();
			center[0] = taskX;
			center[2] = taskY;
			task.SetOrigin(center);
			
			ref array<int> playerIds = {};
			GetGame().GetPlayerManager().GetPlayers(playerIds);
			GetTaskManager().CreateTaskExecutors();
			foreach(int playerId: playerIds)
			{
				SCR_BaseTaskExecutor executor = SCR_BaseTaskExecutor.GetTaskExecutorByID(playerId);
				executor.AssignNewTask(task);
			}			
		}
	}
	
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		
		if(!TW_Global.IsInRuntime())
			return;
		
		SCR_TW_ExtractionHandler.GetInstance().RegisterExtractionSite(this);				
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	private void Do_Start()
	{
		Start();
	}
	
	private void Start()
	{
		if(!TW_Global.IsServer(this))
		{
			PrintFormat("TrainWreck: This was called from client. Not server. %1 items.", SCR_TW_ExtractionHandler.GetInstance().GetCatalogCount());
			Rpc(Do_Start);			
			return;
		}
		PrintFormat("TrainWreck: This was called from server. %1 items", SCR_TW_ExtractionHandler.GetInstance().GetCatalogCount());
		
		Print("TrainWreck: Starting event cycle");
		
		if(m_Started)
			return;
		
		m_Started = true;
		NextStage();
	}
	
	private void StartStage()
	{
		if(!m_CurrentStage)
			return;
		
		PrintFormat("TrainWreck: Stage %1 is starting", m_CurrentStage.GetStateName());
		
		// We don't care about stages that have been completed
		if(m_CurrentStage.IsComplete())
			return;
		
		if(m_CurrentStage.IsPending())
		{
			m_CurrentStage.OnStateActivated(this);
			CheckStageAtInterval();
		}
	}
	
	private void NextStage()
	{
		if(m_ExtractionConfigs.IsEmpty())
		{
			// If there's nothing left in the event list, we should scrub this extraction site
			IEntity occupant = GetOccupant();
			if(!occupant)
				PrintFormat("TrainWreck: Was unable to cleanup extraction site. Occupant is null", LogLevel.WARNING);
			else
				SCR_EntityHelper.DeleteEntityAndChildren(occupant);
			return;
		}
		
		// Pop
		m_CurrentStage = m_ExtractionConfigs.Get(0);
		m_ExtractionConfigs.Remove(0);
		
		// continue forward
		StartStage();
	}
	
	private void CheckStageAtInterval()
	{
		if(!m_CurrentStage || m_CurrentStage.IsPending())
		{
			PrintFormat("TrainWreck: Current Stage(%1) is pending or null %2", m_CurrentStage.GetStateName(), m_CurrentStage, LogLevel.WARNING);
			return;
		}
		
		// If we're not complete -- invoke the update method on the stage
		if(!m_CurrentStage.IsComplete())
		{
			if(m_CurrentStage.ShouldUpdateAtInterval())
			{
				PrintFormat("TrainWreck: Calling update function on stage(%1)", m_CurrentStage.GetStateName());
				m_CurrentStage.OnUpdate();
				GetGame().GetCallqueue().CallLater(CheckStageAtInterval, SCR_TW_Util.FromSecondsToMilliseconds(m_CurrentStage.GetUpdateInterval()), false);
			}
			else
				GetGame().GetCallqueue().CallLater(CheckStageAtInterval, SCR_TW_Util.FromSecondsToMilliseconds(15), false);
		}
		
		// Completed - move forward
		else if(!m_ExtractionConfigs.IsEmpty())
			NextStage();
		else
			Print("TrainWreck: Nothing left to do");
	}
};