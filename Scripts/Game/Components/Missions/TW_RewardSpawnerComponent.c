class TW_RewardSpawnerComponentClass : ScriptComponentClass {};


//! Component used to help spawn rewards of varying types
class TW_RewardSpawnerComponent : ScriptComponent
{
	protected SCR_InventoryStorageManagerComponent m_StorageManager;
	
	override void OnPostInit(IEntity owner)
	{
		if(!TW_Global.IsInRuntime())
			return;
		
		m_StorageManager = TW<SCR_InventoryStorageManagerComponent>.Find(owner);
		if(!m_StorageManager)
			Debug.Error("Reward Spawner Component requires an Inventory Storage Manager");
	}
	
	void SpawnReward(TW_MissionReward reward)
	{
		int amount = reward.GetRewardAmount();
		ResourceName prefab = ResourceName.Empty;
		
		switch(reward.GetRewardType())
		{
			case TW_RewardType.Intelligence:
				prefab = SCR_TW_ExtractionSpawnHandler.GetInstance().GetIntelligenceRewardPrefab();				
				break;
		}
		
		if(prefab.IsEmpty())
		{
			Print(string.Format("TrainWreck: Failed to spawn reward. Reward Type: %1", reward.GetRewardType()), LogLevel.ERROR);
			return;
		}
		
		for(int i = 0; i < amount; i++)
		{
			if(!m_StorageManager.TrySpawnPrefabToStorage(prefab, purpose: EStoragePurpose.PURPOSE_DEPOSIT))
				Print(string.Format("TrainWreck: Failed to insert type %1 (%2/%3)", reward.GetRewardType(), i+1, amount), LogLevel.WARNING);
		}
	}
	
	void ClearContents()
	{
		ref array<IEntity> items = {};
		m_StorageManager.GetItems(items);
		foreach(IEntity item : items)
		{
			m_StorageManager.TryRemoveItemFromInventory(item);
			SCR_EntityHelper.DeleteEntityAndChildren(item);
		}
	}
};