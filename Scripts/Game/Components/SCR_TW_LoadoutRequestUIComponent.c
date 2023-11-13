modded class SCR_LoadoutRequestUIComponent : SCR_DeployRequestUIBaseComponent
{
	protected override void DeleteEntity(IEntity entity)
	{
		if(!entity)
			return;
		IEntity child = entity.GetChildren();
		while (child)
		{
			IEntity sibling = child.GetSibling();
			DeleteEntity(sibling);
			child = sibling;
		}

		delete entity;
	}
};