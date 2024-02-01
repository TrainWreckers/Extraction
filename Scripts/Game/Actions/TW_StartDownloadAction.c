class TW_StartDownloadAction : ScriptedUserAction
{		
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		controller.OnDownloadStart();
	}
	
	override bool CanBeShownScript(IEntity user) { return true; }
	override bool CanBePerformedScript(IEntity user) { return true; }
	override bool HasLocalEffectOnlyScript() { return true; }
};