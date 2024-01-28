enum TW_ExtractionState {
	Pending,
	Active,
	Completed
};

[BaseContainerProps(configRoot: true)]
class TW_ExtractionWorkflow
{
	[Attribute("", UIWidgets.Auto, category: "States", params: "conf class=TW_ExtractionConfig", desc: "In order, states players much go through in order to extract")]
	ref array<ref TW_ExtractionConfig> States;
};

[BaseContainerProps(configRoot: true)]
class TW_ExtractionConfig
{
	[Attribute("", UIWidgets.EditBox, category: "Information", desc: "Name of state")]
	protected string m_Name;
	
	[Attribute("", UIWidgets.CheckBox, category: "Sounds", desc: "Should a sound be played when this state has been reached?")]
	protected bool m_PlaySound;
	
	[Attribute("", UIWidgets.Auto, category: "Sounds", desc: "List of potential sounds to play when this state has been reached")]
	protected ref array<string> m_Sounds;
	
	[Attribute("", UIWidgets.CheckBox, category: "Update", desc: "Should this be updated at a particular interval?")]
	protected bool m_ShouldUpdateAtInterval;
	
	[Attribute("30", UIWidgets.Slider, category: "Update", desc: "Time in seconds to invoke update method", params: "1 300 1")]
	protected int m_UpdateInterval;
	
	[Attribute("", UIWidgets.EditBox, category: "Task", desc: "Task State")]
	protected string m_TaskTitle;
	
	[Attribute("", UIWidgets.EditBox, category: "Task", desc: "Task Description")]
	protected string m_TaskDescription;
	
	[Attribute("", UIWidgets.ResourceNamePicker, category: "Task", desc: "Prefab for task")]
	protected ResourceName m_TaskPrefab;
	
	private TW_ExtractionState m_CurrentState = TW_ExtractionState.Pending;
	
	protected SCR_BaseTask m_CurrentTask;
	
	bool IsComplete() { return m_CurrentState == TW_ExtractionState.Completed; }
	bool IsPending() { return m_CurrentState == TW_ExtractionState.Pending; }
	
	bool ShouldPlaySound() { return m_PlaySound; }
	
	bool ShouldUpdateAtInterval() { return m_ShouldUpdateAtInterval; }
	
	float GetUpdateInterval() { return m_UpdateInterval; }
	
	ref array<int> playerIds = {};
	string GetRandomSound()
	{
		if(m_Sounds.IsEmpty())
			return string.Empty;
		
		return m_Sounds.GetRandomElement();
	}
	
	string GetStateName() { return m_Name; }
	
	private void AssignTask(IEntity source)
	{
		if(!m_TaskPrefab)
			return;
		
		if(m_CurrentTask)
		{						
			GetTaskManager().SetTaskState(m_CurrentTask.GetTaskID(), SCR_TaskState.REMOVED, true);
			GetTaskManager().DeleteLocalTask(m_CurrentTask);
		}
		
		m_CurrentTask = SCR_BaseTask.Cast(GetTaskManager().SpawnTask(m_TaskPrefab));
		m_CurrentTask.SetTitle(m_TaskTitle);
		m_CurrentTask.SetDescription(m_TaskDescription);
		m_CurrentTask.SetOrigin(source.GetOrigin());				
		
		GetTaskManager().CreateTaskExecutors();
		
		playerIds.Clear();
		GetGame().GetPlayerManager().GetPlayers(playerIds);
		foreach(int playerId: playerIds)
		{
			SCR_BaseTaskExecutor executor = SCR_BaseTaskExecutor.GetTaskExecutorByID(playerId);			
			executor.AssignNewTask(m_CurrentTask);
		}
		
		GetTaskManager().UpdateTaskInformation(m_CurrentTask, m_TaskTitle, m_TaskDescription, source.GetOrigin());
	}
	
	//! When this state becomes active. Passes entity which detected activation.
	void OnStateActivated(IEntity activationEntity)
	{
		m_CurrentState = TW_ExtractionState.Active;
		
		if(ShouldPlaySound() && !m_Sounds.IsEmpty())
			SCR_TW_ExtractionHandler.GetInstance().PlaySoundOnEntity(activationEntity, GetRandomSound());
		
		AssignTask(activationEntity);
	}
	
	//! Invoked at specified interval
	void OnUpdate(){};
	
	//! When this statei s no longer active/completed
	void OnStateDeactivated()
	{
		PrintFormat("TrainWreck: %1 deactivated/ended", GetStateName());
		
		m_CurrentState = TW_ExtractionState.Completed;
				
		if(m_CurrentTask)
			GetTaskManager().DeleteLocalTask(m_CurrentTask);
	}
};

[BaseContainerProps(configRoot: true)]
class TW_ExtractionConfig_Fortify : TW_ExtractionConfig
{
	[Attribute("15", UIWidgets.Slider, category: "Notify Info", params: "5 60 5", desc: "Time in seconds between notifications of current timer")]
	protected int m_NotifyTimerInterval;
	
	[Attribute("60", UIWidgets.Slider, category: "Notify Info", params: "5 300 5", desc: "Time in seconds players are given to fortify")]	
	protected int m_FortifyLength;
	
	private bool isCountingDown = false;
	
	override void OnStateActivated(IEntity activationEntity)
	{
		super.OnStateActivated(activationEntity);
		isCountingDown = true;
		
		GetGame().GetCallqueue().CallLater(FortifyPeriod, SCR_TW_Util.FromSecondsToMilliseconds(m_FortifyLength), false);
		GetGame().GetCallqueue().CallLater(Notify, SCR_TW_Util.FromSecondsToMilliseconds(m_NotifyTimerInterval), false, m_FortifyLength - m_NotifyTimerInterval);
	}
	
	private void FortifyPeriod()
	{
		isCountingDown = false;
		SCR_TW_ExtractionHandler.GetInstance().PopUpMessage("Fortify", "Time is up");
		OnStateDeactivated();
	}
	
	private void Notify(int secondsLeft)
	{
		Print(string.Format("TrainWreck: Fortify %1", secondsLeft));
		string desc = string.Format("%1 seconds left to fortify", secondsLeft);
		SCR_TW_ExtractionHandler.GetInstance().PopUpMessage("Fortify", desc);
		
		secondsLeft -= m_NotifyTimerInterval;
		
		if(secondsLeft > 0)
			GetGame().GetCallqueue().CallLater(Notify, SCR_TW_Util.FromSecondsToMilliseconds(m_NotifyTimerInterval), false, secondsLeft);
	}
};

[BaseContainerProps(configRoot: true)]
class TW_ExtractionConfig_Defend : TW_ExtractionConfig 
{
	[Attribute("3", UIWidgets.Slider, params: "2 20 2", category: "Spawn", desc: "Minimum number of prefabs/groups to spawn")]
	protected int m_MinSpawnCount;
	
	[Attribute("5", UIWidgets.Slider, params: "2 20 2", category: "Spawn", desc: "Maximum number of prefabs/groups to spawn")]
	protected int m_MaxSpawnCount;
	
	[Attribute("", UIWidgets.ResourcePickerThumbnail, category: "Spawn", desc: "Prefabs that may spawn")]
	protected ref array<ResourceName> m_Prefabs;
	
	[Attribute("5", UIWidgets.Slider, category: "Spawn Timer", desc: "Minimum time between spawning prefabs")]	
	protected float m_MinSpawnInterval;
	
	[Attribute("10", UIWidgets.Slider, category: "Spawn Timer", desc: "Maximum time between spawning prefabs")]
	protected float m_MaxSpawnInterval;
	
	[Attribute("200", UIWidgets.Slider, category: "Spawn", desc: "Minimum distance for prefabs to spawn")]
	protected float m_MinimumDistanceToSpawn;
	
	[Attribute("", UIWidgets.ResourceNamePicker, category: "Waypoints", desc: "Waypoint types to randomly assign")]
	protected ref array<ResourceName> m_WaypointPrefabs;
	
	[Attribute("180", UIWidgets.Slider, category: "Defend", desc: "If players inside area survive for X time - they can extract")]
	protected float m_DefendTimer;
	
	[Attribute("75", UIWidgets.Slider, category: "Defend", desc: "Radius players must be in to be considered valid for extraction", params: "10 200 10")]
	protected float m_DefendRadius;
	
	private bool m_HasSpawned = false;
	private bool m_TimerCompleted = false;
	private ref array<SCR_AIGroup> m_SpawnedGroups = {};		
	private IEntity activationSource;
	private ref array<IEntity> players = {};
		
	int GetSpawnCount() { return Math.RandomIntInclusive(m_MinSpawnCount, m_MaxSpawnCount); }
	float GetNextSpawnInterval() { return Math.RandomFloatInclusive(m_MinSpawnInterval, m_MaxSpawnInterval); }
	ResourceName GetRandomWaypointPrefab() { return m_WaypointPrefabs.GetRandomElement(); }
	ResourceName GetRandomPrefab() { return m_Prefabs.GetRandomElement(); }	
	
	private void Countdown()
	{
		m_TimerCompleted = true;
	}
	
	private void SpawnChain(int count)
	{
		if(count <= 0)
			return;
		
		m_HasSpawned = true;
		
		ResourceName groupPrefab = GetRandomPrefab();		
		SCR_AIGroup group = SCR_TW_Util.SpawnGroup(groupPrefab, activationSource.GetOrigin(), m_MinimumDistanceToSpawn * 2.5, m_MinimumDistanceToSpawn);
		
		if(!group)
			Print(string.Format("TrainWreck: SpawnChain: Was unable to spawn %1", groupPrefab), LogLevel.ERROR);
		else
		{			
			m_SpawnedGroups.Insert(group);
			group.SetIgnoreWandering(true);
			group.SetIgnoreGlobalCount(true);
			
			ResourceName waypointPrefab = GetRandomWaypointPrefab();
			AIWaypoint waypoint = SCR_TW_Util.CreateWaypointAt(waypointPrefab, activationSource.GetOrigin());
			
			if(waypoint)
				group.AddWaypoint(waypoint);
			else
				Print(string.Format("TrainWreck: SpawnChain: Was unable to create waypoint %1 for %2", waypointPrefab, groupPrefab), LogLevel.ERROR);				
		}
		
		if(count > 0)
			GetGame().GetCallqueue().CallLater(SpawnChain, SCR_TW_Util.FromSecondsToMilliseconds(GetNextSpawnInterval()), false, count - 1);
	}
	
	private bool CheckForCompletion()
	{
		// If we have started the event, no one is alive OR timer is exhausted - we've completed
		return m_TimerCompleted;
	}
	
	override void OnStateActivated(IEntity activationEntity)
	{
		super.OnStateActivated(activationEntity);
		activationSource = activationEntity;
		
		ref array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);
		
		players = {};
		
		foreach(int playerId : playerIds)
		{
			IEntity playerEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
			
			if(playerEntity)
				players.Insert(playerEntity);
		}			
		
		GetGame().GetCallqueue().CallLater(SpawnChain, SCR_TW_Util.FromSecondsToMilliseconds(1), false, GetSpawnCount());
		GetGame().GetCallqueue().CallLater(Countdown, SCR_TW_Util.FromSecondsToMilliseconds(m_DefendTimer), false);				
	}
	
	override void OnUpdate()
	{
		super.OnUpdate();
		
		if(CheckForCompletion())
			OnStateDeactivated();
	}
	
	override void OnStateDeactivated()
	{
		super.OnStateDeactivated();
		if(players.IsEmpty())
			return;
		
		ref array<string> extractedPlayerNames = {};
			
		foreach(IEntity player : players)
		{
			// Player must have died, or something. Old reference
			if(!player)
				continue;
			
			// Ensure players are within the intended extraction distance
			float distance = vector.Distance(activationSource.GetOrigin(), player.GetOrigin());
			if(distance > m_DefendRadius)
			{
				if(m_CurrentTask)
					m_CurrentTask.SetState(SCR_TaskState.CANCELLED);
				continue;
			}
			
			// If players meet the criteria they'll be considered for extraction
			int playerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(player);
			
			string playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
			extractedPlayerNames.Insert(playerName);
			
			SCR_TW_ExtractionPlayerInventoryComponent.GetInstance().UpdatePlayerInventory(playerId);
			SCR_TW_ExtractionHandler.GetInstance().DeletePlayer(playerId);
			
			if(m_CurrentTask)
				m_CurrentTask.SetState(SCR_TaskState.FINISHED);			
		}
		
		if(!extractedPlayerNames.IsEmpty())
		{
			SCR_TW_ExtractionHandler.GetInstance().PopUpMessage(SCR_StringHelper.Join(", ", extractedPlayerNames), "Successfully extracted");
		}
	}
};