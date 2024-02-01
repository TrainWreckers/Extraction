class TW_MissionDeliveryComponentClass : ScriptComponentClass {};

class TW_MissionDeliveryComponent : ScriptComponent
{
	private ref TW_MissionDeliver m_Mission;
	private SCR_InventoryStorageManagerComponent m_StorageManager;
	private ref map<string, int> m_delivered = new map<string, int>();
	
	
	
	override void OnPostInit(IEntity owner)
	{
		if(!TW_Global.IsInRuntime() || !TW_Global.IsServer(owner))
			return;
		SCR_InventoryStorageManagerComponent manager = TW<SCR_InventoryStorageManagerComponent>.Find(owner);
		m_StorageManager = manager;
		
		manager.m_OnItemRemovedInvoker.Insert(OnItemRemoved);
		manager.m_OnItemAddedInvoker.Insert(OnItemAdded);
	}
	
	private void Clear()
	{
		// Clear out the inventory of this pickup container
		ref array<IEntity> items = {};
		m_StorageManager.GetItems(items);
		
		foreach(IEntity item : items)
		{
			m_StorageManager.TryRemoveItemFromInventory(item);
			SCR_EntityHelper.DeleteEntityAndChildren(item);
		}
	}
	
	void AddTo(ResourceName prefab, int amount)
	{
		for(int i = 0; i < amount; i++)
		{
			if(!m_StorageManager.TrySpawnPrefabToStorage(prefab, purpose: EStoragePurpose.PURPOSE_DEPOSIT))
				Print(string.Format("TrainWreck: Failed to spawn (%1/%2) of %3", i+1, amount, prefab), LogLevel.WARNING);
		}
	}
	
	override void OnDelete(IEntity owner)
	{	
		super.OnDelete(owner);
		ClearEvents();
	}
	
	void ClearEvents()
	{
		if(m_StorageManager)
		{
			m_StorageManager.m_OnItemRemovedInvoker.Remove(OnItemRemoved);
			m_StorageManager.m_OnItemAddedInvoker.Remove(OnItemAdded);
		}
	}
	
	//! Link this component to a mission
	void LinkToMission(notnull TW_MissionDeliver mission)
	{
		m_Mission = mission;
	
		Clear();
	}
	
	private void OnItemRemoved(IEntity entity, BaseInventoryStorageComponent storageComponent)
	{
		if(!m_Mission)
			return;
		
		ResourceName prefab = entity.GetPrefabData().GetPrefab().GetResourceName();
		if(m_delivered.Contains(prefab) && m_delivered.Get(prefab) > 0)
			m_delivered.Set(prefab, m_delivered.Get(prefab) - 1);
	}
	
	private void Complete()
	{
		PrintFormat("TrainWreck: %1 has all items", m_Mission.GetName());
			
		// remove all items
		Clear();
			
		m_Mission.OnDeliverComplete();
	}
	
	private void OnItemAdded(IEntity entity, BaseInventoryStorageComponent storageComponent)
	{
		if(!m_Mission) return;
		if(!m_Mission.IsAcceptableItem(entity))
			return;
		
		ResourceName prefab = entity.GetPrefabData().GetPrefab().GetResourceName();
		
		if(m_delivered.Contains(prefab))
			m_delivered.Set(prefab, m_delivered.Get(prefab) + 1);
		else 
			m_delivered.Set(prefab, 1);
		
		if(m_Mission.HasAllItems(m_delivered))
			GetGame().GetCallqueue().CallLater(Complete, SCR_TW_Util.FromSecondsToMilliseconds(2), false);
	}
};