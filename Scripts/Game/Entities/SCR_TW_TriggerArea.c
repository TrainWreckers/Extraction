[EntityEditorProps(category: "GameScripted/TrainWreck", description: "Base functionality for triggers")]
class SCR_TW_TriggerAreaClass : BaseGameTriggerEntityClass {};

class SCR_TW_TriggerArea : BaseGameTriggerEntity 
{
	[Attribute("1", UIWidgets.CheckBox, category: "Extraction Period", desc: "Use timer from Extraction GameMode")]
	private bool m_UseExtractionTimePeriod;
	
	[Attribute("30", UIWidgets.Slider, category: "Extraction Period", desc: "Custom Extraction Period")]
	private int customExtractionPeriod;
	
	private int timerStartAt;
	private ref map<int, IEntity> playersInside = new map<int, IEntity>();
	
	override void EOnInit(IEntity owner)
	{
		if(!GetGame().InPlayMode())
			return;
		
		RplComponent rpl = RplComponent.Cast(owner.FindComponent(RplComponent));
		
		if(!rpl)
			return;
		
		if(!rpl.IsMaster() && rpl.Role() != RplRole.Authority)
			return;
		
		if(m_UseExtractionTimePeriod)
			timerStartAt = SCR_TW_ExtractionHandler.GetInstance().GetExtractionTimePeriod();
		else
			timerStartAt = customExtractionPeriod;
	}
	
	override event protected void OnActivate(IEntity ent)
	{
		int playerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(ent);
		
		if(playerId < 0)
			return;
		
		string name = GetGame().GetPlayerManager().GetPlayerName(playerId);		
		SCR_TW_ExtractionHandler.GetInstance().PopUpMessage(name, "Has entered extraction zone");
		
		if(playersInside.Contains(playerId))
			playersInside.Set(playerId, ent);
		else
			playersInside.Insert(playerId, ent);
		
		GetGame().GetCallqueue().CallLater(PlayerExtractionTimerLoop, 1000, false, playerId, timerStartAt);
	}
	
	override event protected void OnDeactivate(IEntity ent)
	{
		int playerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(ent);
		string name = GetGame().GetPlayerManager().GetPlayerName(playerId);
		
		if(playerId >= 0)
			SCR_TW_ExtractionHandler.GetInstance().PopUpMessage(name, "Has exited extraction zone");
		
		if(playersInside.Contains(playerId))
			playersInside.Remove(playerId);
	}
	
	
	private void PlayerExtractionTimerLoop(int playerId, int timerInSeconds)
	{
		// If player is no longer in the area, don't continue countdown
		if(!playersInside.Contains(playerId))
		{
			Print(string.Format("TrainWreck: Player %1 is no longer in extraction zone", playerId), LogLevel.WARNING);
			return;
		}
		
		// Still have time left?
		if(timerInSeconds > 0)
		{			
			timerInSeconds -= 1;
			Print(string.Format("TrainWreck: Extracting %1 in %2 seconds", playerId, timerInSeconds), LogLevel.WARNING);
			GetGame().GetCallqueue().CallLater(PlayerExtractionTimerLoop, 1000, false, playerId, timerInSeconds);
			return;
		}		
				
		string name = GetGame().GetPlayerManager().GetPlayerName(playerId);
		Print(string.Format("TrainWreck: Player %1 has extracted", name), LogLevel.WARNING);
		
		// Timer has elapsed, time to extract the player
		SCR_TW_ExtractionHandler.GetInstance().PopUpMessage(name, "Successfully extracted");
		
		// Upate player inventory
		SCR_TW_ExtractionPlayerInventoryComponent.GetInstance().UpdatePlayerInventory(playerId);
		
		DeletePlayer(playerId);
	}
	
	protected void DeletePlayer(int playerId)
	{
		Rpc(RpcDo_DeletePlayerById, playerId);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcDo_DeletePlayerById(int playerId)
	{
		IEntity player = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		
		SCR_CharacterControllerComponent character = SCR_CharacterControllerComponent.Cast(player.FindComponent(SCR_CharacterControllerComponent));
		
		if(!character)
		{
			Debug.Error("TrainWreck: Unable to find character controller on player");
			return;
		}
		
		character.ForceDeath();
		
		if(player)
		{
			SCR_EntityHelper.DeleteEntityAndChildren(player);
			delete player;
			
			SCR_TW_ExtractionHandler.GetInstance().SaveAndDeleteCrate(playerId);
		}
	}
};