[BaseContainerProps(configRoot: true)]
class TW_Event_RadioTower_PowerOn : TW_ExtractionBase
{
	private TW_RadioTower_Component _tower;
	private bool _activated = false;
	
	void OnPowered()
	{
		if(_activated)
			return;
		
		_activated = true;
		
		// We are considered complete since power has been restored
		OnStateDeactivated();
	}
	
	override void OnStateActivated(IEntity activationEntity)
	{
		super.OnStateActivated(activationEntity);
		
		_tower = SCR_TW_ExtractionHandler.GetInstance().GetRandomTower();
		
		if(!_tower)
		{
			Print(string.Format("TrainWreck: %1 -> Failed to get tower", GetStateName()), LogLevel.ERROR);
			return;
		}	
		
		_tower.SetActive(true);
		_tower.OnPowerOnInvoker.Insert(OnPowered);
	}
	
	override void OnStateDeactivated()
	{
		super.OnStateDeactivated();
		_tower.SetActive(false);
		if(_tower)
			_tower.OnPowerOnInvoker.Remove(OnPowered);
	}
};