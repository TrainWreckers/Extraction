class TW_UI_ExtractionItemWidgets
{
	static const ResourceName s_sLayout = "{D3450BA7FB58C909}UI/layouts/Menus/Dialogs/ExtractionItem.layout";
	ResourceName GetLayout() { return s_sLayout; }
	
	OverlayWidget m_wLockFrame;
	ImageWidget m_wLockBackground;
	ImageWidget m_wLock;
	
	TextWidget m_wCost;
	TextWidget m_wExtractionType;
	TW_UI_ExtractionButton m_wCallButton;
	
	ref TW_ActionCost m_ActionCost;
	
	bool Init(Widget root)
	{
		m_wLockFrame = OverlayWidget.Cast(root.FindAnyWidget("m_LockFrame"));
		m_wLockBackground = ImageWidget.Cast(root.FindAnyWidget("m_wLockBackground"));
		m_wLock = ImageWidget.Cast(root.FindAnyWidget("m_Lock"));
		m_wExtractionType = TextWidget.Cast(root.FindAnyWidget("m_wExtractionType"));
		m_wCost = TextWidget.Cast(root.FindAnyWidget("m_wCost"));
		m_wCallButton = TW_UI_ExtractionButton.Cast(SCR_ButtonComponent.GetButtonComponent("m_wCallButton", root));
		return true;
	}
	
	void SetExtractionType(TW_ExtractionType type, string text, TW_ActionCost cost)
	{
		if(!m_wExtractionType)
			return;
		m_ActionCost = cost;
		
		if(m_wCost)
			m_wCost.SetText("" + cost.GetCost());
		
		m_wExtractionType.SetText(text);
		m_wCallButton.SetExtractionType(type, cost);
	}
};

class TW_UI_MissionItemWidgets
{
	static const ResourceName s_sLayout = "{CB28B4DD636D615F}UI/layouts/Menus/Dialogs/MissionItem.layout";
	ResourceName GetLayout() { return s_sLayout; }
	
	OverlayWidget m_wLockFrame;
	ImageWidget m_wLockBackground;
	ImageWidget m_wLock;
	
	TextWidget m_wText;
	TextWidget m_wCost;
	TW_UI_MissionButton m_wCallButton;
	
	bool Init(Widget root)
	{
		m_wLockFrame = OverlayWidget.Cast(root.FindAnyWidget("m_LockFrame"));
		m_wLockBackground = ImageWidget.Cast(root.FindAnyWidget("m_wLockBackground"));
		m_wLock = ImageWidget.Cast(root.FindAnyWidget("m_Lock"));
		m_wText = TextWidget.Cast(root.FindAnyWidget("m_wText"));
		m_wCost = TextWidget.Cast(root.FindAnyWidget("m_wCost"));
		m_wCallButton = TW_UI_MissionButton.Cast(SCR_ButtonComponent.GetButtonComponent("m_wCallButton", root));
		return true;
	}
	
	void SetMission(TW_Mission mission, ResourceName resourceName)
	{
		if(mission == null)
		{
			Print("TrainWreck: Mission cannot be null for Mission Item Widgets", LogLevel.ERROR);
			return;
		}
		
		ref TW_ActionCost cost = mission.GetActionCost();		
		
		m_wText.SetText(mission.GetName());
		
		if(cost)
			m_wCost.SetText("" + cost.GetCost());
		
		m_wCallButton.SetMission(mission, resourceName);
	}
};

class TW_UI_MissionButton : SCR_ButtonComponent
{
	[Attribute()]
	protected ref TW_Mission m_Mission;
	
	[Attribute()]
	protected ResourceName m_Resource;
	
	void SetMission(TW_Mission mission, ResourceName resource)
	{
		m_Mission = mission;
		m_Resource = resource;
	}
	
	ResourceName GetMissionResource() { return m_Resource; }
	TW_Mission GetMission() { return m_Mission; }
};

class TW_UI_ExtractionButton : SCR_ButtonComponent
{
	[Attribute()]
	protected TW_ExtractionType m_ExtractionType;
	protected ref TW_ActionCost m_Cost;
	
	// Set type of extraction this button calls in
	void SetExtractionType(TW_ExtractionType type, TW_ActionCost cost)
	{
		m_ExtractionType = type;
		m_Cost = cost;
	}
	
	TW_ActionCost GetCost() { return m_Cost; }
	
	//! Type of extraction to call in when clicked
	TW_ExtractionType GetExtractionType()
	{
		return m_ExtractionType;
	}
}

class TW_UI_ExtractionDisplay : ChimeraMenuBase
{	
	[Attribute("{2EFEA2AF1F38E7F0}UI/Textures/Icons/icons_wrapperUI-64.m_sImageSet")];
	protected string m_sImageSet;

  	[Attribute("{ABC6B36856013403}UI/Textures/Icons/icons_wrapperUI-64-glow.m_sImageSet")];
	protected string m_sImageSetGlow;
	
	const ref Color COLOR_ORANGE = Color.FromSRGBA(255, 255, 255, 255);
	
	protected WorkspaceWidget m_wWorkspace;
	protected Widget m_wRoot;
	
	protected VerticalLayoutWidget m_ExtractionsList;
	protected SCR_ButtonComponent m_ButtonClose;
	
	protected bool m_ShowHighTier = false;
	protected ref set<ERadioType> m_HighTierRadios = new set<ERadioType>();
	
	private static ref map<ResourceName, ref TW_CallableItem> resourceMissionMap = new map<ResourceName, ref TW_CallableItem>();
	private static ref array<ref TW_ExtractionCall> m_Extractions = {};
	private SCR_InventoryStorageManagerComponent m_StorageManager;
	private ref map<ResourceName, int> m_PlayerItems = new map<ResourceName, int>();
	
	void SetRadioType(ERadioType availableRadio)
	{
		m_ShowHighTier = m_HighTierRadios.Contains(availableRadio);		
		GenerateTable();
	}
	
	override void OnMenuInit()
	{
		super.OnMenuInit();
		
		ChimeraWorld world = ChimeraWorld.CastFrom(GetGame().GetWorld());
		if(!world) return;
		
		m_wWorkspace = GetGame().GetWorkspace();
		m_wRoot = GetRootWidget();
		
		m_ExtractionsList = VerticalLayoutWidget.Cast(m_wRoot.FindAnyWidget("m_Extractions"));
		m_ButtonClose = SCR_ButtonComponent.GetButtonComponent("ButtonExit", m_wRoot);
		
		m_ButtonClose.m_OnClicked.Clear();
		m_ButtonClose.m_OnClicked.Insert(Close);
		
		m_HighTierRadios.Insert(ERadioType.ANPRC77);
		m_HighTierRadios.Insert(ERadioType.R107M);
		
		if(resourceMissionMap.IsEmpty() || m_Extractions.IsEmpty())
		{
			resourceMissionMap.Clear();
			m_Extractions.Clear();
			InitializeMappings();
		}
		
		IEntity character = GetGame().GetPlayerController().GetControlledEntity();		
		m_StorageManager = TW<SCR_InventoryStorageManagerComponent>.Find(character);
		ref array<IEntity> items = {};
		m_StorageManager.GetItems(items);
		
		foreach(IEntity entity : items)
		{
			ResourceName name = entity.GetPrefabData().GetPrefab().GetResourceName();
			if(m_PlayerItems.Contains(name))
				m_PlayerItems.Set(name, m_PlayerItems.Get(name) + 1);
			else 
				m_PlayerItems.Insert(name, 1);
		}
	}
	
	private void InitializeMappings()
	{
		ref array<ResourceName> allMissions = {};
		TW_MissionHandlerComponent.GetInstance().GetMissionPool(allMissions);
		
		foreach(ResourceName resourceName : allMissions)
		{
			Resource resource = Resource.Load(resourceName);
			ref TW_Mission mission = TW_Mission.Cast(BaseContainerTools.CreateInstanceFromContainer(resource.GetResource().ToBaseContainer()));
			if(!resourceMissionMap.Contains(resourceName))
				resourceMissionMap.Insert(resourceName, mission);
		}
		
		TW_MissionHandlerComponent.GetInstance().GetExtractionPool(m_Extractions);
	}
	
	string GetLayoutFor(TW_CallableItem item)
	{
		if(TW_ExtractionCall.Cast(item))
			return TW_UI_ExtractionItemWidgets.s_sLayout;
		if(TW_Mission.Cast(item))
			return TW_UI_MissionItemWidgets.s_sLayout;
		
		return string.Empty;
	}
	
	private void GenerateTable()
	{
		foreach(TW_ExtractionCall extraction : m_Extractions)
		{
			if(extraction.RequiresHigherTierRadio() && ! m_ShowHighTier)
				continue;
			Widget newRow = m_wWorkspace.CreateWidgets(TW_UI_ExtractionItemWidgets.s_sLayout, m_ExtractionsList);
			ref TW_UI_ExtractionItemWidgets widgets = new TW_UI_ExtractionItemWidgets();
			if(widgets.Init(newRow))
			{
				widgets.SetExtractionType(extraction.GetExtractionType(), extraction.GetExtractionName(), extraction.GetActionCost());
				widgets.m_wCallButton.m_OnClicked.Clear();
				widgets.m_wCallButton.m_OnClicked.Insert(OnCallExtraction);
			}
		}
		
		foreach(ResourceName resourceName, TW_CallableItem callableItem : resourceMissionMap)
		{
			if(callableItem.RequiresHigherTierRadio() && !m_ShowHighTier)
				continue;
			
			string layout = GetLayoutFor(callableItem);
			
			if(layout == string.Empty)
			{
				Print(string.Format("TrainWreck: Unable to find layout for %1", callableItem.Type()), LogLevel.WARNING);
				continue;
			}
			
			Widget newRow = m_wWorkspace.CreateWidgets(layout, m_ExtractionsList);
			
			TW_Mission mission = TW_Mission.Cast(callableItem);
			if(mission)
			{
				ref TW_UI_MissionItemWidgets widgets = new TW_UI_MissionItemWidgets();
				if(widgets.Init(newRow))
				{
					widgets.SetMission(mission, resourceName);
					widgets.m_wCallButton.m_OnClicked.Clear();
					widgets.m_wCallButton.m_OnClicked.Insert(OnCallMission);
				}
			}
		}
	}
	
	//! Does the player have enough of specified prefab to purchase item
	bool HasRequiredItemsFor(TW_CallableItem callable)
	{
		if(!callable) return false;
		
		TW_ActionCost cost = callable.GetActionCost();
		
		if(!cost)
		{
			Print("TrainWreck: Action cost cannot be null", LogLevel.ERROR);
			return false;
		}
		
		if(!m_PlayerItems.Contains(cost.GetResource()))
			return false;
		
		return m_PlayerItems.Get(cost.GetResource()) >= cost.GetCost();
	}
	
	
	//! Remove specified amount of a resource from player's inventory
	private void RemoveAmount(TW_ActionCost cost)
	{
		int remaining = cost.GetCost();
		ref array<IEntity> items = {};
		m_StorageManager.GetItems(items);
		foreach(IEntity entity : items)
		{
			if(remaining <= 0)
				break;
			if(!entity)
				continue;
			
			ResourceName resource = entity.GetPrefabData().GetPrefab().GetResourceName();
			if(!resource)
				continue;
			
			if(resource != cost.GetResource())
				continue;
			
			m_StorageManager.TryRemoveItemFromInventory(entity);
			SCR_EntityHelper.DeleteEntityAndChildren(entity);
			remaining --;
		}
	}
	
	void OnCallMission(TW_UI_MissionButton button)
	{
		PrintFormat("TrainWreck: User clicked on mission: %1", button.GetMission().GetName());
		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		
		if(!controller)
		{
			Print("TrainWreck: No player controller was found on local player entity", LogLevel.ERROR);
			return;
		}
		
		controller.CallForMission(button.GetMissionResource());
		GetGame().GetMenuManager().CloseAllMenus();
	}
	
	void OnCallExtraction(TW_UI_ExtractionButton button)
	{
		TW_ExtractionType calledType = button.GetExtractionType();
		PrintFormat("TrainWreck: User clicked on %1", calledType);
		
		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		
		if(!controller)
		{
			Print("TrainWreck: No Player Controller was found on local player entity", LogLevel.ERROR);
			return;
		}
		
		controller.CallForExtraction(calledType);		
		GetGame().GetMenuManager().CloseAllMenus();
	}
	
};