modded class SCR_PlayerController
{
	bool m_isLocked = false;
	
	void OnDownloadStart()
	{
		Print("TrainWreck: Starting download");
		Rpc(RplAsk_Server_StartDownload);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	private void RplAsk_Server_StartDownload()
	{
		Print("TrainWreck: Starting download (Server)");
		TW_MissionDownload mission = TW_MissionDownload.GetActiveMission();
		if(!mission)
			return;
		
		mission.StartDownload();
	}
	
	void OnPlayerSpawnedEvent(RplId playerCrateId)
	{
		if(!playerCrateId.IsValid())
		{
			Debug.Error(string.Format("Invalid crate ID. IsServer: %1. IsClient: %2", 
			    Replication.IsServer(),
				Replication.IsClient())
			);
			return;
		}
		
		Rpc(RpcAsk_Owner_OpenLootManager, playerCrateId);
	}	
	
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcAsk_Owner_OpenLootManager(RplId crateId)
	{		
		CheckForReplicatedItem(crateId);		
	}
	
	protected void OpenLootManager_Owner(IEntity crate)
	{		
		SCR_TW_PlayerCrateComponent crateComp = SCR_TW_PlayerCrateComponent.Cast(crate.FindComponent(SCR_TW_PlayerCrateComponent));
		
		if(!crateComp)
		{
			Debug.Error("TrainWreck: No SCR_TW_PlayerCrateComponent found");
			return;
		}
		
		MenuBase menu = GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.Inventory20Menu);
		
		SCR_InventoryMenuUI inventory = SCR_InventoryMenuUI.Cast(menu);
		inventory.SetOpenStorageToLootCrateContents(crateComp);
	}
	
	protected void CheckForReplicatedItem(RplId id, int currentAttempt = 0, int maxAttempts = 100)
	{
		if(currentAttempt >= maxAttempts)
		{
			Print(string.Format("TrainWreck: Failed to find replicated item with Id: %1. Attempt(%2/%3)", id, currentAttempt, maxAttempts), LogLevel.ERROR);
			return;
		}
		
		currentAttempt++;
		
		if(!id.IsValid())
		{
			GetGame().GetCallqueue().CallLater(CheckForReplicatedItem, 100, false, id, currentAttempt, maxAttempts);
			return;
		}
		
		RplComponent comp = RplComponent.Cast(Replication.FindItem(id));
		
		if(!comp)
		{
			GetGame().GetCallqueue().CallLater(CheckForReplicatedItem, 100, false, id, currentAttempt, maxAttempts);
			return;
		}
		
		IEntity entity = comp.GetEntity();
		if(!entity)
		{
			GetGame().GetCallqueue().CallLater(CheckForReplicatedItem, 100, false, id, currentAttempt, maxAttempts);
			return;
		}
		
		OpenLootManager_Owner(entity);
	}
	
	void OnLootManagerClosed()
	{
		Rpc(RpcAsk_Authority_OnLootManagerClosed, GetPlayerId());
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_Authority_OnLootManagerClosed(int playerId)
	{
		SCR_TW_ExtractionHandler.GetInstance().SaveAndDeleteCrate(playerId);
	}
	
	void MarkEventSiteAsVisited(SCR_TW_EventSite site)
	{
		Rpc(RpcAsk_MarkSiteAsVisited, Replication.FindId(site), true);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_MarkSiteAsVisited(RplId siteId, bool value)
	{
		SCR_TW_EventSite site = SCR_TW_EventSite.Cast(Replication.FindItem(siteId));
		
		if(!site)
		{
			PrintFormat("TrainWreck: Invalid Event Site Replication Id: %1", siteId);
			return;
		}
		
		site.SetVisited(value);
	}
	
	void CallForExtraction(TW_ExtractionType type)
	{
		if(!TW_Global.IsServer(this))
			Rpc(RpcAsk_CallForExtraction, type);
		else 
			RpcAsk_CallForExtraction(type);
	}
	
	void CallForMission(ResourceName resource)
	{
		if(!TW_Global.IsServer(this))
			Rpc(RpcAsk_CallForMission, resource);
		else
			RpcAsk_CallForMission(resource);
	}
	
	void CombineMags(MagazineComponent fromMag, MagazineComponent toMag, SCR_InventoryStorageManagerComponent managerComp)
	{
		if(!fromMag || !toMag)
		{
			Print("Could not locate magazine components", LogLevel.ERROR);
			return;
		}
		
		int fromCount = fromMag.GetAmmoCount();
		int toCount = toMag.GetAmmoCount();
		int maxCount = toMag.GetMaxAmmoCount();
		
		IEntity fromEntity = fromMag.GetOwner();
		IEntity toEntity = toMag.GetOwner();
		BaseInventoryStorageComponent storage = TW<BaseInventoryStorageComponent>.Find(managerComp.GetOwner());
		
		if(!storage)
		{
			Print("TrainWreck: Was unable to locate storage or manager components", LogLevel.ERROR);
			return;
		}
		
		if(fromCount + toCount <= maxCount)
		{
			toMag.SetAmmoCount(fromCount + toCount);
			
			if(managerComp.TryRemoveItemFromInventory(toEntity, storage))
			{
				if(!managerComp.TryInsertItemInStorage(toEntity, storage))
				{
					Print("TrainWreck: Was unable to successfully add packed magazine back into storage", LogLevel.ERROR);
				}
			}
			
			// From mag was fully exhausted 			
			SCR_EntityHelper.DeleteEntityAndChildren(fromMag.GetOwner());
		}
		else
		{
			int remainder = (fromCount + toCount) % maxCount;
			
			toMag.SetAmmoCount(maxCount);
			
			if(managerComp.TryRemoveItemFromInventory(toEntity, storage))
			{
				if(!managerComp.TryInsertItemInStorage(toEntity, storage))
				{
					Print("TrainWreck: Was unable to successfully add packed magazine back into storage", LogLevel.ERROR);
				}
			}
			
			if(remainder > 0)
			{
				fromMag.SetAmmoCount(remainder);
				if(managerComp.TryRemoveItemFromInventory(fromEntity, storage))
				{
					if(!managerComp.TryInsertItemInStorage(fromEntity, storage))
					{
						Print("TrainWreck: Was unable to successfully add packed magazine back into storage", LogLevel.ERROR);
					}
				}
			}				
			else 
				SCR_EntityHelper.DeleteEntityAndChildren(fromMag.GetOwner());
		}	
	}	
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_CallForExtraction(TW_ExtractionType type)
	{
		SCR_TW_ExtractionHandler.GetInstance().CallExtraction(type);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_CallForMission(ResourceName resourceName)
	{
		if(!TW_MissionHandlerComponent.GetInstance().CanSpawnMission())
		{
			Print("TrainWreck: Too many active missions", LogLevel.WARNING);
			return;
		}
		
		TW_MissionHandlerComponent.GetInstance().SpawnMission(resourceName);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_CombineMags(RplId fromMagazineComponent, RplId toMagazineComponent, RplId storageManagerComponent)
	{
		if(!fromMagazineComponent.IsValid())
		{
			Print("Invalid from magazine", LogLevel.ERROR);
			return;
		}
		
		if(!toMagazineComponent.IsValid())
		{
			Print("Invalid to magazine", LogLevel.ERROR);
			return;
		}
		
		if(!storageManagerComponent.IsValid())
		{
			Print("Invalid manager", LogLevel.ERROR);
			return;
		}
		
		MagazineComponent fromMag = MagazineComponent.Cast(Replication.FindItem(fromMagazineComponent));
		MagazineComponent toMag = MagazineComponent.Cast(Replication.FindItem(toMagazineComponent));
		SCR_InventoryStorageManagerComponent managerComponent = SCR_InventoryStorageManagerComponent.Cast(Replication.FindItem(storageManagerComponent));
		
		CombineMags(fromMag, toMag, managerComponent);		
	}
};