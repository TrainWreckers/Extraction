enum TW_MissionType
{
	Deliver,
	ExtractWith,
	Download
};

enum TW_RewardType
{
	Intelligence
};

enum TW_MissionSpawnArea
{
	//! Random extraction site on map
	ExtractionSite,
	
	//! Random event site on map
	EventSite,
	
	//! Random position around a player
	RandomNearPlayer,
	
	//! Extraction site that is opposite of map from player positions
	ExtractionSiteOpposite,
	
	//! Event site that is opposite of the map from player positions
	EventSiteOpposite
};

//! Represents a type of reward to provide for completing a mission
[BaseContainerProps(configRoot: true)]
class TW_MissionReward
{
	[Attribute("0", UIWidgets.ComboBox, category: "Reward", "", enums: ParamEnumArray.FromEnum(TW_RewardType) )]
	protected TW_RewardType m_RewardType;
	
	[Attribute("0", UIWidgets.CheckBox, category: "Reward", desc: "Should rewards be randomized between 1 and amount")]	
	protected bool m_UseRandomCount;
	
	[Attribute("1", UIWidgets.Slider, params: "1 100 1", category: "Reward")]
	protected int m_Amount;
	
	//! Get reward type for completing mission
	TW_RewardType GetRewardType() { return m_RewardType; }
	
	//! Get amount of reward to give to player(s)
	int GetRewardAmount()
	{
		if(m_UseRandomCount)
			return Math.RandomIntInclusive(1, m_Amount);
		return m_Amount;
	}
};

//! Represents the basic structure for missions
[BaseContainerProps(configRoot: true)]
class TW_Mission
{
	protected int m_MissionId;
	
	[Attribute("", UIWidgets.EditBox, category: "Basic Info", desc: "Name of mission")]
	protected string m_Name;
	
	[Attribute("", UIWidgets.EditBoxMultiline, category: "Basic Info", desc: "Description of mission")]
	protected string m_Description;
	
	[Attribute("", UIWidgets.ComboBox, category: "Mission", desc: "Type of mission", enums: ParamEnumArray.FromEnum(TW_MissionType))]
	protected TW_MissionType m_MissionType;	
	
	[Attribute("", UIWidgets.Auto, category: "Mission", desc: "Reward for completing the mission", params: "conf class=TW_MissionReward")]
	protected ref TW_MissionReward m_Reward;
	
	[Attribute("", UIWidgets.ResourceNamePicker, category: "Prefabs", desc: "Prefab to use for task", params: "et")]
	protected ResourceName m_TaskPrefab;
	
	[Attribute("", UIWidgets.ComboBox, category: "Mission", desc: "Location rules", enums: ParamEnumArray.FromEnum(TW_MissionSpawnArea))]
	protected TW_MissionSpawnArea m_SpawnRule;
	
	protected SCR_BaseTask m_Task;
	
	//! Name or title of mission
	string GetName() { return m_Name; }
	
	//! Description of mission
	string GetDescription() { return m_Description; }
	
	//! Type of mission
	TW_MissionType GetMissionType() { return m_MissionType; }
	
	//! Retrieve the task used for tracking this mission
	SCR_BaseTask GetTask() { return m_Task; }
	
	//! Retrieve reward information for completing task
	TW_MissionReward GetReward() { return m_Reward; }
	
	//! Retrieve rule for spawning this mission
	TW_MissionSpawnArea GetSpawnLocationRule() { return m_SpawnRule; }	
	
	//! Any logic for detecting whether a mission can start should go here
	bool CanStart() { return true; }
	
	//! Any logic for detecting whether a mission is done/completed should go here
	bool IsSuccess() { return false; }
	
	//! Any setup/init logic for a mission should go here
	void InitializeMission(){}
	
	//! Completion logic/cleanup for mission should go here
	void Finish(){}
	
	//! Set mission's ID for later identification
	void SetMissionId(int id)
	{
		m_MissionId = id;
	}
	
	//! Retrieve mission's identifier
	int GetMissionId() { return m_MissionId; }
	
	//! Create task based on local information
	SCR_BaseTask CreateTask(IEntity onEntity, SCR_TaskState updatePreviousTaskState = SCR_TaskState.FINISHED)
	{
		if(m_Task)
			GetTaskManager().UpdateTaskCompletion(m_Task.GetTaskID(), updatePreviousTaskState);
		
		m_Task = GetTaskManager().SpawnTask(m_TaskPrefab);
		GetTaskManager().UpdateTaskInformation(m_Task, m_Name, m_Description, onEntity.GetOrigin());		
		
		ref array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);
		foreach(int playerId: playerIds)
		{
			SCR_BaseTaskExecutor executor = SCR_BaseTaskExecutor.GetTaskExecutorByID(playerId);			
			executor.AssignNewTask(m_Task);
		}
		
		return m_Task;
	}
};

[BaseContainerProps(configRoot: true)]
class TW_MissionDeliver : TW_Mission
{	
	[Attribute("", UIWidgets.Auto, category: "Delivery", desc: "Entity to deliver something to")]
	protected ref array<ResourceName> m_DropOffPrefabs;
	
	[Attribute("", UIWidgets.Auto, category: "Items", desc: "Category to pull items from", params: "conf class=TW_Category")]
	protected ref TW_Category m_Category;
	
	[Attribute("0", UIWidgets.CheckBox, category: "Items", desc: "Should a specific item be selected from category")]
	protected bool m_UseSpecificItem;
	
	[Attribute("1", UIWidgets.Slider, category: "Items", desc: "Random number of items associated with mission", params: "1 20 1")]
	protected int m_MinAmount;
	
	[Attribute("2", UIWidgets.Slider, category: "Items", desc: "Random number of items associated with mission", params: "1 20 1")]
	protected int m_MaxAmount;
	
	private ref map<string, int> itemsForDelivery = new map<string, int>();
	private int totalRequiredAmount;
	private bool m_Complete = false;
	
	//! Entity being used as a dropoff point for delivery mission
	protected TW_MissionDeliveryComponent m_DropOffComp;
	
	//! Retrieve the delivery component attached to this mission
	TW_MissionDeliveryComponent GetDropOffComp() { return m_DropOffComp; }
	
	//! Retrieve number of items needed for fulfilling mission
	int GetAmount() 
	{ 
		if(totalRequiredAmount <= 0)
			totalRequiredAmount = Math.RandomIntInclusive(m_MinAmount, m_MaxAmount); 	
		return totalRequiredAmount;
	}
	
	//! Does the provided item fit criteria for delivery
	bool IsAcceptableItem(IEntity entity)
	{				
		IEntity root = SCR_EntityHelper.GetMainParent(entity, self: true);
		ResourceName prefabData = entity.GetPrefabData().GetPrefab().GetResourceName();
		return m_Category.HasPrefab(prefabData);
	}
	
	//! Check whether tracked items meet all criteria for delivery
	bool HasAllItems(notnull map<string, int> delivered)
	{
		int acceptedItems = 0;
		
		if(m_UseSpecificItem)
		{
			foreach(string name, int count : itemsForDelivery)
			{
				if(!delivered.Contains(name))
					return false;
				
				if(delivered.Get(name) < count)
					return false;
			}
			
			return true;
		}
		
		// Must be anything in category
		foreach(string name, int count : delivered)
		{
			if(m_Category.HasPrefab(name))
				acceptedItems += count;
		}
		
		return acceptedItems >= totalRequiredAmount;
	}
	
	override void InitializeMission()
	{
		InitializeSelectedPrefabs();
		
		if(m_UseSpecificItem && itemsForDelivery.IsEmpty())
			Debug.Error("Delivery mission should have a populated items for delivery map");
				
		SpawnDelivery();
	}
	
	protected void SpawnDelivery()
	{
		IEntity spawnOn = SCR_TW_ExtractionSpawnHandler.GetInstance().GetLocationBySpawnRule(m_SpawnRule);
		
		if(!spawnOn)
		{
			Print(string.Format("TrainWreck: Failed to locate delivery location for %1", m_Name + " " + m_Description), LogLevel.ERROR);
			return;
		}
		
		SCR_TW_EventSite site = SCR_TW_EventSite.Cast(spawnOn);
		
		if(!site)
		{
			Print("TrainWreck: Was unable to locate SCR_TW_EventSite component on location", LogLevel.ERROR);
			return;
		}
		
		site.IgnoreGC(true);
		
		ResourceName selectedPrefab = m_DropOffPrefabs.GetRandomElement();
		Resource resource = Resource.Load(selectedPrefab);
		
		IEntity dropoffEntity = site.SpawnEntityInSlot(resource);
		GetGame().GetCallqueue().CallLater(FinishSpawnDelivery, SCR_TW_Util.FromSecondsToMilliseconds(1), false, dropoffEntity, selectedPrefab);
	}
	
	protected void FinishSpawnDelivery(IEntity spawnedEntity, ResourceName selectedPrefab)
	{
		if(!spawnedEntity) return;
		ref array<IEntity> entities = {};
		SCR_EntityHelper.GetHierarchyEntityList(spawnedEntity, entities);
		int count = entities.Count();
		
		foreach(IEntity object : entities)
		{
			if(!object) continue;
			
			TW_MissionDeliveryComponent comp = TW<TW_MissionDeliveryComponent>.Find(object);
			
			if(comp)
			{
				m_DropOffComp = comp;
				break;
			}
		}
		
		if(!m_DropOffComp)
		{
			Print(string.Format("TrainWreck: Spawned delivery prefab %1 does not have something with TW_MissionDeliveryComponent", selectedPrefab), LogLevel.ERROR);
			return;
		}
				
		m_DropOffComp.LinkToMission(this);
		
		CreateTask(m_DropOffComp.GetOwner());
	}
	
	protected void InitializeSelectedPrefabs()
	{
		totalRequiredAmount= GetAmount();		
		itemsForDelivery = new map<string, int>();
		
		if(m_UseSpecificItem)
		{
			ResourceName randomItem = m_Category.GetRandomElement();
			if(!randomItem)
				Debug.Error("Invalid item from category");
			itemsForDelivery.Insert(randomItem, totalRequiredAmount);
			m_Description = string.Format("You must deliver the following items:\n %1 x %2", WidgetManager.Translate(SCR_TW_Util.GetPrefabDisplayName(randomItem)), totalRequiredAmount);					
		}
		else
			m_Description = string.Format("You must deliver %1 items from the %2 category.\n%3", totalRequiredAmount, m_Category.GetName(), m_Category.GetNameList());
	}
	
	void OnDeliverComplete()
	{
		GetTaskManager().UpdateTaskCompletion(m_Task.GetTaskID(), SCR_TaskState.FINISHED);
		SCR_TW_ExtractionHandler.GetInstance().PopUpMessage(m_Name, "Delivery complete!");
		
		// Now spawn in the rewards
		TW_RewardType rewardType = GetReward().GetRewardType();
		switch(rewardType)
		{
			case TW_RewardType.Intelligence:
			{
				m_DropOffComp.AddTo(SCR_TW_ExtractionSpawnHandler.GetInstance().GetIntelligenceRewardPrefab(), GetReward().GetRewardAmount());
				break;
			}
		}
		
		m_Complete = true;
	}
	
	void ~TW_MissionDeliver()
	{
		if(m_DropOffComp)
		{
			m_DropOffComp.ClearEvents();
		}
	}
	
	override bool IsSuccess() { return m_Complete; }
};