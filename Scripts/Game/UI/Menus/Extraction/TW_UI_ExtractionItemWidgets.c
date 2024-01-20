class TW_UI_ExtractionItemWidgets
{
	static const ResourceName s_sLayout = "{D3450BA7FB58C909}UI/layouts/Menus/Dialogs/ExtractionItem.layout";
	ResourceName GetLayout() { return s_sLayout; }
	
	OverlayWidget m_wLockFrame;
	ImageWidget m_wLockBackground;
	ImageWidget m_wLock;
	
	TextWidget m_wExtractionType;
	TW_UI_ExtractionButton m_wCallButton;
	
	bool Init(Widget root)
	{
		m_wLockFrame = OverlayWidget.Cast(root.FindAnyWidget("m_LockFrame"));
		m_wLockBackground = ImageWidget.Cast(root.FindAnyWidget("m_wLockBackground"));
		m_wLock = ImageWidget.Cast(root.FindAnyWidget("m_Lock"));
		m_wExtractionType = TextWidget.Cast(root.FindAnyWidget("m_wExtractionType"));
		m_wCallButton = TW_UI_ExtractionButton.Cast(SCR_ButtonComponent.GetButtonComponent("m_wCallButton", root));
		return true;
	}
	
	void SetExtractionType(TW_ExtractionType type, string text)
	{
		if(!m_wExtractionType)
			return;
		
		m_wExtractionType.SetText(text);
		m_wCallButton.SetExtractionType(type);
	}
};

class TW_UI_ExtractionButton : SCR_ButtonComponent
{
	[Attribute()]
	protected TW_ExtractionType m_ExtractionType;
	
	// Set type of extraction this button calls in
	void SetExtractionType(TW_ExtractionType type)
	{
		m_ExtractionType = type;
	}
	
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
	protected ref map<TW_ExtractionType, string> types = new map<TW_ExtractionType, string>();
	protected VerticalLayoutWidget m_ExtractionsList;
	protected SCR_ButtonComponent m_ButtonClose;
	
	protected bool m_ShowHighTier = false;
	protected ref set<ERadioType> m_HighTierRadios = new set<ERadioType>();
	
	void SetRadioType(ERadioType availableRadio)
	{
		m_ShowHighTier = m_HighTierRadios.Contains(availableRadio);
		
		if(m_ShowHighTier)
		{
			types.Insert(TW_ExtractionType.TIER_1, "Tier 1");
			types.Insert(TW_ExtractionType.TIER_2, "Tier 2");
			types.Insert(TW_ExtractionType.TIER_3, "Tier 3");
		}		
		
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
		
		types.Insert(TW_ExtractionType.STANDARD, "Standard");
	}
	
	private void GenerateTable()
	{
		foreach(TW_ExtractionType type, string text : types)
		{
			Widget newRow = m_wWorkspace.CreateWidgets(TW_UI_ExtractionItemWidgets.s_sLayout, m_ExtractionsList);
			ref TW_UI_ExtractionItemWidgets widgets = new TW_UI_ExtractionItemWidgets();
			if(widgets.Init(newRow))
			{
				widgets.SetExtractionType(type, text);
				widgets.m_wCallButton.m_OnClicked.Clear();
				widgets.m_wCallButton.m_OnClicked.Insert(OnCallExtraction);
			}
		}
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
		Close();
	}
	
};