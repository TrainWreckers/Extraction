//------------------------------------------------------------------------------------------------
//! modded version for to be used with the inventory 2.0 
class SCR_TW_OpenPlayerContainerAction : SCR_InventoryAction
{	
	override protected void PerformActionInternal(SCR_InventoryStorageManagerComponent manager, IEntity pOwnerEntity, IEntity pUserEntity)
	{
		auto vicinity = CharacterVicinityComponent.Cast( pUserEntity .FindComponent( CharacterVicinityComponent ));
		if ( !vicinity )
			return;
		
		manager.SetStorageToOpen(pOwnerEntity);
		manager.OpenInventory();
	}
	
	private bool CanUse()
	{
		int currentPlayerId = GetGame().GetPlayerController().GetPlayerId();
		
		SCR_TW_PlayerCrateComponent playerCrate = SCR_TW_PlayerCrateComponent.Cast(GetOwner().FindComponent(SCR_TW_PlayerCrateComponent));
		
		if(!playerCrate)
			return false;
		
		return playerCrate.CanOpen(GetGame().GetPlayerManager().GetPlayerName(currentPlayerId));
	}
	
	override bool CanBeShownScript(IEntity user) 
	{ 
		return CanUse();
	}
	
	override bool CanBePerformedScript(IEntity user) 
	{ 
		return CanUse();
	}
	
	override bool HasLocalEffectOnlyScript() { return true; }
};