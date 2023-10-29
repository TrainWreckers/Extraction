modded class SCR_SiteSlotEntity
{
	
		
	override void EOnInit(IEntity owner)
	{
		SCR_TW_CompositionHandler.GetInstance().ProcessEntity(this);
		super.EOnInit(owner);
	}
}