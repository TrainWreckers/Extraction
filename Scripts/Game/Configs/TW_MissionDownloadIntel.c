//! Only 1 download can be active at a time
[BaseContainerProps(configRoot: true)]
class TW_MissionDownload : TW_Mission
{
	[Attribute("90", UIWidgets.Slider, params: "30 300 5", category: "Reward", desc: "Time in seconds where players will receive intelligence for keeping the download active")]
	protected int m_RewardIntervalInSeconds;
	
	[Attribute("", UIWidgets.EditBox, category: "Notification", desc: "Title of notification")]
	protected string m_NotificationTitle;
	
	[Attribute("Downloaded a total of %1 %2", UIWidgets.EditBox, category: "Notification", desc: "Format of notification content")]
	protected string m_NotificationDescriptionFormat;
	
	[Attribute("", UIWidgets.ResourceNamePicker, category: "Trigger", desc: "Trigger prefab to use")]
	protected ResourceName m_TriggerPrefab;
	
	[Attribute("", UIWidgets.ResourceNamePicker, category: "Global Waypoint", desc: "Waypoints to assign groups", params: "et")]
	protected ResourceName m_AIWaypoint;
	
	[Attribute("", UIWidgets.Auto, category: "Compositions", desc: "Compositions for download event", params: "et")]
	protected ref array<ResourceName> m_EventPrefabs;
	
	//! Players that are in the trigger area
	protected ref map<int, IEntity> m_PlayersInArea = new map<int, IEntity>();
	
	//! Total amount of reward that has been received for keeping the download active
	protected int m_CollectedAmount;
	
	//! Trigger which detects players
	protected SCR_TW_TriggerArea m_Trigger;
	
	//! Will help globally determine if a download is active
	protected static bool s_IsDownloadActive;
	
	// Is the download active
	static bool IsDownloadActive() { return s_IsDownloadActive; }
	
	protected static ref TW_MissionDownload s_ActiveMission;
	static TW_MissionDownload GetActiveMission() { return s_ActiveMission; }
	
	IEntity GetMissionEntity() { return m_CompositionEntity; }
	ResourceName GetWaypointPrefab() { return m_AIWaypoint; }
	
	//! Composition containing download event
	protected IEntity m_CompositionEntity;
	
	protected TW_RewardSpawnerComponent m_RewardSpawner;
	
	//! Composition containing the download event
	IEntity GetCompositionEntity() { return m_CompositionEntity; }		
	
	void RemovePlayer(int playerId)
	{
		if(!m_PlayersInArea.Contains(playerId))
			return;
		
		IEntity player = m_PlayersInArea.Get(playerId);
		
		// Simply because we have some logic in here for checking failure
		OnPlayerExited(player);
	}
	
	override void InitializeMission()
	{
		if(s_ActiveMission)
		{
			Print("TrainWreck: An active download mission already exists. Unable to initialize", LogLevel.WARNING);
			return;
		}
		
		s_ActiveMission = this;
		
		if(m_EventPrefabs.IsEmpty())
		{
			Print("TrainWreck: No event prefabs assigned to mission. Please provide at least 1 composition prefab to use", LogLevel.ERROR);
			return;
		}
		
		IEntity spawnLocation = SCR_TW_ExtractionSpawnHandler.GetInstance().GetLocationBySpawnRule(GetSpawnLocationRule());
		SCR_TW_EventSite site = SCR_TW_EventSite.Cast(spawnLocation);
		
		if(!site)
		{
			Print(string.Format("TrainWreck: Currently using event sites for missions. Need to implement others. Type: %1", GetMissionType()), LogLevel.ERROR);
			return;	
		}
		
		ResourceName randomCompositionPrefab = m_EventPrefabs.GetRandomElement();
		Resource compositionResource = Resource.Load(randomCompositionPrefab);
		
		if(!compositionResource.IsValid())
		{
			Print(string.Format("TrainWreck: Invalid resource for composition prefab %1", randomCompositionPrefab), LogLevel.ERROR);
			return;
		}
		
		// Spawn composition within the site
		m_CompositionEntity = site.SpawnEntityInSlot(compositionResource);		
		GetGame().GetCallqueue().CallLater(CheckComposition, 500, false);
	}
	
	protected void RewardLoop()
	{
		if(!s_IsDownloadActive)
			return;
		
		m_CollectedAmount += 1;
		m_RewardSpawner.SpawnReward(m_Reward);
		
		foreach(int playerId, IEntity playerEntity : m_PlayersInArea)
			SCR_NotificationsComponent.SendToPlayer(playerId, ENotification.TW_Download_Tick, m_CollectedAmount);
		
		GetGame().GetCallqueue().CallLater(RewardLoop, SCR_TW_Util.FromSecondsToMilliseconds(m_RewardIntervalInSeconds), false);
	}
	
	protected void CheckComposition()
	{
		// Find the reward spawner within the composition
		ref array<IEntity> entities = {};		
		SCR_EntityHelper.GetHierarchyEntityList(m_CompositionEntity, entities);
		
		foreach(IEntity entity : entities)
		{
			m_RewardSpawner = TW<TW_RewardSpawnerComponent>.Find(entity);
			
			if(m_RewardSpawner)
				break;
		}
		
		if(!m_RewardSpawner)
		{
			Print(string.Format("TrainWreck: failed to locate TW_RewardSpawnerComponent within %1", m_CompositionEntity.GetPrefabData().GetPrefab().GetResourceName()), LogLevel.ERROR);
			SCR_EntityHelper.DeleteEntityAndChildren(m_CompositionEntity);
			return;
		}
		
		// Create trigger for knowing if players are in the area or not
		CreateTrigger();
		CreateTask(m_CompositionEntity);
	}
	
	protected void CreateTrigger()
	{
		if(m_TriggerPrefab.IsEmpty())
			Debug.Error("Invalid trigger prefab for Download Mission");
		
		Resource triggerResource = Resource.Load(m_TriggerPrefab);
		
		if(!triggerResource.IsValid())
		{
			Print(string.Format("TrainWreck: Invalid trigger prefab for download mission. %1", m_TriggerPrefab), LogLevel.ERROR);
			return;
		}
		
		EntitySpawnParams params = EntitySpawnParams();
		m_CompositionEntity.GetTransform(params.Transform);
		m_Trigger = SCR_TW_TriggerArea.Cast(GetGame().SpawnEntityPrefab(triggerResource, GetGame().GetWorld(), params));
		
		m_Trigger.OnEntered.Insert(OnPlayerEntered);
		m_Trigger.OnExited.Insert(OnPlayerExited);
	}
	
	void StartDownload()
	{		
		// IF we're already active -- don't start another reward loop
		if(s_IsDownloadActive)
			return;
		
		Print("TrainWreck: Starting download reward loop");
		SCR_TW_ExtractionHandler.GetInstance().PopUpMessage("Download", "Hunker down, you're actively being targeted");
		s_IsDownloadActive = true;
		GetGame().GetCallqueue().CallLater(RewardLoop, SCR_TW_Util.FromSecondsToMilliseconds(m_RewardIntervalInSeconds), false);
	}
	
	protected void OnPlayerEntered(IEntity playerEntity)
	{
		if(!playerEntity)
			return;
		
		int playerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(playerEntity);
		
		if(!m_PlayersInArea.Contains(playerId))
			m_PlayersInArea.Set(playerId, playerEntity);
	}
	
	protected void OnPlayerExited(IEntity playerEntity)
	{
		if(!playerEntity)
			return;
		
		int playerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(playerEntity);
		
		if(m_PlayersInArea.Contains(playerId))
			m_PlayersInArea.Remove(playerId);
		
		// If all players in the area have died/left then the download needs to stop and end
		if(m_PlayersInArea.IsEmpty())
			Failure();
		else if(s_IsDownloadActive)
		{
			int total = m_PlayersInArea.Count();
			foreach(int id, IEntity player : m_PlayersInArea)			
				SCR_NotificationsComponent.SendToPlayer(id, ENotification.TW_Download_PlayerLeft, total);
		}
	}
	
	protected void Failure()
	{
		SCR_TW_ExtractionHandler.GetInstance().PopUpMessage("Download Ended", "Lost control over download site");
		GetTaskManager().UpdateTaskCompletion(m_Task.GetTaskID(), SCR_TaskState.CANCELLED);		
		Cleanup();
	}
	
	protected void Cleanup()
	{
		s_IsDownloadActive = false;
		m_PlayersInArea.Clear();
		m_RewardSpawner.ClearContents();
		m_CollectedAmount = 0;
	}
};