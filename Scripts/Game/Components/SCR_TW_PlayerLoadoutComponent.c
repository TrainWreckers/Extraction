class SCR_TW_PlayerLoadoutComponentClass : ScriptComponentClass {};

class SCR_TW_PlayerLoadoutComponent : ScriptComponent
{
	private InventoryStorageManagerComponent storageManager;
	
	void InitializePlayerInventory(string jsonLoadout)
	{
		storageManager = InventoryStorageManagerComponent.Cast(GetOwner().FindComponent(InventoryStorageManagerComponent));
		
		if(!storageManager)
		{
			Debug.Error("TrainWreck: Player does not have an inventory storage manager component");
			return;
		}
		
		
		SCR_JsonLoadContext loadContext = new SCR_JsonLoadContext();
		bool success = loadContext.ImportFromString(jsonLoadout);
		
		if(!success)
		{
			Debug.Error(string.Format("TrainWreck: Failed to parse %1", jsonLoadout));
			return;
		}
		
		/*
			
		foreach(EL_DefaultLoadoutItem loadoutItem : m_DefaultCharacterItems)
		{
			if(loadoutItem.m_Purpose != EStoragePurpose.PURPOSE_LOADOUT_PROXY)
			{
				Debug.Error(string.Format("TrainWreck: Failed to add %1 as character item. Only clothing/backpack/vest/etc are allowed as top level items", loadoutItem.m_Prefab, typename.EnumToString(EStoragePurpose, EStoragePurpose.PURPOSE_LOADOUT_PROXY)));
				continue;		
			}
		
			IEntity slotEntity = SpawnDefaultCharacterItem(storageManager, loadoutItem);
			if(!slotEntity)
				continue;
			if(!storageManager.TryInsertItem(slotEntity, loadoutItem.m_Purpose))
				SCR_EntityHelper.DeleteEntityAndChildren(slotEntity);
		}
		
		IEntity SpawnDefaultCharacterItem(InventoryStorageManagerComponent storageManager, EL_DefaultLoadoutItem loadoutItem)
		{
			IEntity slotEntity = GetGame().SpawnEntityPrefab(Resource.Load(loadoutItem.m_Prefab));
			if(!loadoutItem.m_ComponentDefaults)
			{
				foreach(EL_DefaultLoadoutItemComponent componentDefault : loadoutItem.m_ComponentDefaults)
					componentDefault.ApplyTo(slotEntity);
			}
		
			foreach(loadoutItem.m_StoredItems)
			{
				array<Managed> outComponents();
				slotEntity.FindComponents(BaseInventoryStorageComponent, outComponents);
				foreach(EL_DefaultLoadoutItem storedItem : loadoutItem.StoredItems)
				{
					IEntity spawnedItem = SpawnDefaultCharacterItem(storageManager, storedItem);
					foreach(Managed componentRef : outComponents)
					{
						BaseInventoryStorageComponent storageComponent = BaseInventoryStorageComponent.Cast(componentRef);
						if(storageComponent.GetPurpose() & storedItem.m_Purpose)
						{
							if(!storageManager.TryInsertItemInStorage(spawnedItem, storageComponent)) continue;
		
							InventoryItemCoponent inventoryItemComponent InventoryItemComponent.Cast(spawnedItem.FindCompnent(InventoryItemComponent));
							if(inventoryItemComponent && !inventoryItemComponent.GetParentSlot()) continue;
								break;
						}
					}
				}
		
				return slotEntity;
			}
		}
		
		[BaseContainerProps()]
		class EL_DefaultLoadoutItem
		{
			[Attribute()]
			ResourcEName m_Prefab;
		
			[Attribute(defvalue: "1")]
			int m_Amount;
		
			[Attribute(defvalue: EStoragePurpose.PURPOSE_DEPOSIT.ToString(), UIWidgets.ComboBox, enums: ParamEnumArray.FromEnum(EStoragePurpose))]
			EStoragePurpose m_Purpose;
		
			[Attribute()]
			ref array<ref EL_DefaultLoadoutItem> m_StoredItems;
		}
		*/
	}
		
};