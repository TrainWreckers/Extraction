//------------------------------------------------------------------------------------------------
//! modded version for to be used with the inventory 2.0 
class TW_RadioTower_TurnOnAction : ScriptedUserAction
{	
	TW_RadioTower_Component _tower;
	
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		_tower = TW<TW_RadioTower_Component>.Find(pOwnerEntity);	
	}
	
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		_tower.TurnOn();
	}
	
	override bool CanBeShownScript(IEntity user) { return _tower && _tower.IsTowerActive(); }
	override bool CanBePerformedScript(IEntity user) { return _tower && _tower.IsPoweredOn(); }
	override bool HasLocalEffectOnlyScript() { return true; }
};