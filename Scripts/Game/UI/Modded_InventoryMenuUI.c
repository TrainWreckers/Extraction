modded class SCR_InventoryMenuUI 
{
	protected MagazineComponent m_MagComp;
	
	MagazineComponent GetMagazineComponent(IEntity item)
	{
		return MagazineComponent.Cast(item.FindComponent(MagazineComponent));
	}
	
	private void CanFocusedItemBeRepacked()
	{
		if(!m_pFocusedSlotUI)
			return;
		
		InventoryItemComponent itemComp = m_pFocusedSlotUI.GetInventoryItemComponent();
		
		if(!itemComp)
			return;
		
		MagazineComponent magComp = GetMagazineComponent(itemComp.GetOwner());
		
		if(!magComp)
			return;
		
		m_MagComp = magComp;
		bool arsenalItem = IsStorageArsenal(m_pFocusedSlotUI.GetStorageUI().GetCurrentNavigationStorage());
		bool characterItem = IsStorageCharacter(m_pFocusedSlotUI.GetStorageUI().GetCurrentNavigationStorage());
		
		if(!arsenalItem && characterItem && SCR_TW_Util.IsMagazineNotFull(m_MagComp) && SCR_TW_Util.CharacterMagCount(m_MagComp, GetInventoryStorageManager()))
			m_pNavigationBar.SetButtonEnabled("ButtonRepackMag", (m_MagComp != null));
	}
	
	override void NavigationBarUpdate()
	{
		super.NavigationBarUpdate();
		
		CanFocusedItemBeRepacked();
	}
	
	override void NavigationBarUpdateGamepad()
	{
		super.NavigationBarUpdateGamepad();
		CanFocusedItemBeRepacked();
	}
	
	
	
	protected bool IsStorageCharacter(BaseInventoryStorageComponent storage)
	{
		if(!storage)
			return false;
		
		if(!SCR_EntityHelper.GetMainParent(storage.GetOwner()))
			return false;
		
		return (SCR_EntityHelper.GetMainParent(storage.GetOwner()).FindComponent(SCR_CharacterInventoryStorageComponent));
	}
	
	override void OnAction(SCR_InputButtonComponent comp, string action, SCR_InventoryStorageBaseUI pParentStorage = null, int traverseStorageIndex = -1)
	{
	    super.OnAction(comp, action, pParentStorage, traverseStorageIndex);
	    switch (action)
		{
	    	case "TW_RepackMag":
			{
	      		Action_RepackMag();
				break;
	    	}				
	    }
	  }
	
	protected void Action_RepackMag()
	{
		int maxAmmoCount = m_MagComp.GetMaxAmmoCount();
		int currentAmmoCount = m_MagComp.GetAmmoCount();
		
		IEntity character = SCR_EntityHelper.GetMainParent(m_MagComp.GetOwner());
		
		SCR_MagazinePredicate magPredicate = new SCR_MagazinePredicate();
		BaseMagazineWell magWell = m_MagComp.GetMagazineWell();
		magPredicate.magWellType = magWell.Type();
		
		array<IEntity> magEntities = new array<IEntity>;
		GetInventoryStorageManager().FindItems(magEntities, magPredicate);
		magEntities.RemoveItem(m_MagComp.GetOwner());
		
		array<MagazineComponent> mags = SCR_TW_Util.SortedMagazines(magEntities);
		magEntities.Clear();
		
		foreach(MagazineComponent comp : mags)
		{
			// If the mag we're trying to repack is at max capacity we can stop 
			if(m_MagComp.GetAmmoCount() == maxAmmoCount)
				break; // Can't return because we want to yeet any mags that need to be discarded
			
			// So long as the mag combination doesn't exceed our max ammo count 
			// we can safely add the two together
			if(m_MagComp.GetAmmoCount() + comp.GetAmmoCount() < maxAmmoCount)
			{
				m_MagComp.SetAmmoCount(m_MagComp.GetAmmoCount() + comp.GetAmmoCount());
				
				// The mag we just added shall get deleted once complete
				magEntities.Insert(comp.GetOwner());
				continue;
			}
			else
			{
				// What's left over after topping of target mag? 
				int remainder = (m_MagComp.GetAmmoCount() + comp.GetAmmoCount()) % maxAmmoCount;
				
				// Update target mag to be it's max 
				m_MagComp.SetAmmoCount(maxAmmoCount);
				
				if(remainder > 0)
					comp.SetAmmoCount(remainder);
				else
					// consumed mag gets deleted later 
					magEntities.Insert(comp.GetOwner());
				
				// Since we have topped off our target mag we can stop iterating
				break;
			}
		}
		
		foreach(IEntity mag : magEntities)
			SCR_EntityHelper.DeleteEntityAndChildren(mag);
		
		SCR_TW_Util.ResetInventoryMenu();
	}
};