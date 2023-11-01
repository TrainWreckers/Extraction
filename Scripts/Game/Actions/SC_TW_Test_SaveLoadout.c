//------------------------------------------------------------------------------------------------
//! modded version for to be used with the inventory 2.0 
class SCR_TW_Test_SaveLoadout : ScriptedUserAction
{	
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		int currentPlayerId = GetGame().GetPlayerController().GetPlayerId();
		
		bool success = SCR_TW_ExtractionPlayerInventoryComponent.GetInstance().SavePlayerLoadout(currentPlayerId);
		
		if(success)
			SCR_NotificationsComponent.SendLocal(ENotification.PLAYER_LOADOUT_SAVED);
		else
			SCR_NotificationsComponent.SendLocal(ENotification.PLAYER_LOADOUT_NOT_SAVED);
	}
	
	override bool CanBeShownScript(IEntity user) { return true; }
	override bool CanBePerformedScript(IEntity user) { return true; }
	override bool HasLocalEffectOnlyScript() { return true; }
};