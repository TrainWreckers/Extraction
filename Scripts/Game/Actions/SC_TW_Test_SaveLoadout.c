//------------------------------------------------------------------------------------------------
//! modded version for to be used with the inventory 2.0 
class SCR_TW_Test_SaveLoadout : ScriptedUserAction
{	
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		int currentPlayerId = GetGame().GetPlayerController().GetPlayerId();		
		SCR_TW_ExtractionPlayerInventoryComponent.GetInstance().UpdatePlayerInventory(currentPlayerId);		
	}
	
	override bool CanBeShownScript(IEntity user) { return true; }
	override bool CanBePerformedScript(IEntity user) { return true; }
	override bool HasLocalEffectOnlyScript() { return true; }
};