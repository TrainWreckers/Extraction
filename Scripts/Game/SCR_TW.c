#ifndef TW_DEBUG_FACTION_LOCATIONS
#define TW_DEBUG_FACTION_LOCATIONS
#endif

[EntityEditorProps(category: "GameScripted/TrainWreck/Systems", description: "Entity that takes care of managing composition spawning.", color: "0 0 255 255")]
class SCR_TW_ComponentClass: SCR_BaseGameModeComponentClass
{
	
};

class SCR_TW_Component: SCR_BaseGameModeComponent
{	
	static SCR_TW_Component s_Instance;
	
	static SCR_TW_Component GetInstance()
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (!gameMode)
			return null;

		if (!s_Instance)
			s_Instance = SCR_TW_Component.Cast(gameMode.FindComponent(SCR_TW_Component));

		return s_Instance;
	}
	
	private static SCR_GameModeCampaign _globalCampaign;
	
	static SCR_GameModeCampaign GetGlobalCampaign()
	{
		if(_globalCampaign)
			return _globalCampaign;
		
		_globalCampaign = SCR_GameModeCampaign.Cast(GetGame().GetGameMode());
		
		return _globalCampaign;
	}
	
	static FactionKey GetBLUFORFactionKey()
	{
		SCR_GameModeCampaign campaign = GetGlobalCampaign();
		return campaign.GetFactionKeyByEnum(SCR_ECampaignFaction.BLUFOR);
	}
	
	static FactionKey GetOPFORFactionKey()
	{
		SCR_GameModeCampaign campaign = GetGlobalCampaign();
		return campaign.GetFactionKeyByEnum(SCR_ECampaignFaction.OPFOR);
	}
	
	static FactionKey GetINDFORFactionKey()
	{
		SCR_GameModeCampaign campaign = GetGlobalCampaign();
		return campaign.GetFactionKeyByEnum(SCR_ECampaignFaction.INDFOR);
	}
	
	private static SCR_CampaignMilitaryBaseManager _campaignBaseManager;
	
	static SCR_CampaignMilitaryBaseManager GetCampaignBaseManager()
	{
		return SCR_GameModeCampaign.GetInstance().GetBaseManager();	
	}
	
	static SCR_AmbientPatrolManager GetPatrolManager() 
	{
		return SCR_AmbientPatrolManager.GetInstance();
	}
	
	static void GetFactionBaseDict(notnull map<FactionKey, ref array<SCR_CampaignMilitaryBaseComponent>> baseDict)
	{				
		ref array<SCR_CampaignMilitaryBaseComponent> bases = new ref array<SCR_CampaignMilitaryBaseComponent>;
		int count = GetCampaignBaseManager().GetCampaignBases(bases);
		
		for(int i = 0; i < count; i++)
		{		
			SCR_CampaignMilitaryBaseComponent campaignBase = bases.Get(i);
			
			if(!campaignBase)
				continue;			
			
			// Base cannot be disabled
			// Base can be a control point or something used as HQ? I think HQ is fine... we'll find out
			
			FactionKey faction = FactionKey.Empty;
			
			if(!campaignBase.GetFaction())
			{
				faction = GetINDFORFactionKey();
			}
			else
				faction = campaignBase.GetFaction().GetFactionKey();								
			
			if(!baseDict.Contains(faction))
			{
				baseDict.Insert(faction, new ref array<SCR_CampaignMilitaryBaseComponent>);
				baseDict.Get(faction).Insert(campaignBase);										
			}
			else
			{
				baseDict.Get(faction).Insert(campaignBase);
			}
				
			Print(string.Format("%1: %2", faction, campaignBase.GetBaseName()), LogLevel.NORMAL);
		}
		
		Print(string.Format("Total bases: %1", count), LogLevel.WARNING);
	}
}