[EntityEditorProps(category: "GameScripted/TrainWreck/Encounters", description: "Handles seize/defend based encounters")]
class SCR_TW_SeizeEncounterHandlerClass : SCR_TW_EncounterHandlerClass
{

};

class SCR_TW_SeizeEncounterHandler : SCR_TW_EncounterHandler
{
	protected TW_SeizeEncounterConfig m_SeizeConfig;
	
	private string _logFormat = "[SCR_TW_SeizeEncounterHandler]: <%1> -- %2";
	
	// Track the defend site that was selected
	private SCR_CampaignMilitaryBaseComponent _defendSite;
	
	// Track where squads will come from
	private ref array<SCR_CampaignMilitaryBaseComponent> _basesToSpawnAt = new ref array<SCR_CampaignMilitaryBaseComponent>;
	
	// Track the number of groups that should spawn during the seize wave
	private int _numberOfGroupstoSpawn = 1;
	
	private string taskTitle;
	private string taskDescription;
	private string taskSound;
	
	// Number of minutes until the attack commences
	private int minutesLeft = 0;
	
	// Has the attack started?
	private bool _hasStarted = false;
	
	int GetMinutesLeft() { return minutesLeft; }
	
	override void Handle()
	{
		super.Handle();
		
		if(currentPhase != SCR_TW_EncounterPhase.Active)
			return;
		
		if(m_SpawnedGroups.Count() <= 0)
			currentPhase = SCR_TW_EncounterPhase.Complete;
	}
	
	override void Setup(notnull map<FactionKey, ref array<SCR_CampaignMilitaryBaseComponent>> baseDict, FactionKey playerFaction, FactionKey opposingFaction, notnull array<IEntity> players)
	{
		if(!IsMaster())
		{
			m_HasErrored = true;
			return;
		}
		
		super.Setup(baseDict, playerFaction, opposingFaction, players);
		
		m_SeizeConfig = TW_SeizeEncounterConfig.Cast(m_Config);
		
		if(!m_SeizeConfig)
		{
			m_HasErrored = true;
			Print("SCR_TW_SeizeEncounterHandler requires a config of type TW_SeizeEncounterConfig", LogLevel.ERROR);
			return;
		}
		
		if(baseDict.Get(playerFaction).IsEmpty())
		{
			m_HasErrored = true;
			Print("SCR_TW_SeizeEncounterHandler requires at least 1 player owned base", LogLevel.ERROR);
			return;
		}
		
		if(!m_SeizeConfig.GetTaskPrefab())
		{
			Print("SCR_TW_SeizeEncounterHandler requires a Task prefab (that has a MapDescriptor Component", LogLevel.ERROR);
			m_HasErrored = true;
			return;
		}
		
		minutesLeft = m_SeizeConfig.GenerateReinforceTimer();
		StartReinforcementPeriod();		
	}
	
	private void StartReinforcementPeriod()
	{
		Print(string.Format(_logFormat, "StartReinforcementPeriod", "Setting up reinforcement period"));
		
		_defendSite = SCR_TW_Util.GetClosestLocationToPlayer(baseDict.Get(playerFaction), players, 0);
		
		SCR_CampaignMilitaryBaseComponent closestNon = SCR_TW_Component.GetCampaignBaseManager().FindClosestBaseNotFaction(playerFaction, _defendSite.GetOwner().GetOrigin());
		FactionKey usingFaction = closestNon.GetFaction().GetFactionKey();
		
		if(!_defendSite)
		{
			Print(string.Format(_logFormat, "StartReinforcementPeriod", "Invalid defend site. Exiting..."), LogLevel.ERROR);
			m_HasErrored = true;
			return;
		}
		
		ref array<string> locationTexts = new ref array<string>;
		_numberOfGroupstoSpawn = m_SeizeConfig.GenerateSpawnCount();
		for(int i = 0; i < _numberOfGroupstoSpawn; i++)
		{
			// Figure out where to spawn enemy groups
			SCR_CampaignMilitaryBaseComponent enemyBase = baseDict.Get(usingFaction).GetRandomElement();
			
			if(!enemyBase)
			{
				Print(string.Format(_logFormat, "StartReinforcementPeriod", "Unable to locate enemy base"), LogLevel.WARNING);
				continue;
			}
			
			_basesToSpawnAt.Insert(enemyBase);
			
			if(!locationTexts.Contains(enemyBase.GetBaseName()))
				locationTexts.Insert(enemyBase.GetBaseName());
		}
		
		string combinedLocations = SCR_StringHelper.Join(", ", locationTexts, false);
		Print(string.Format("Seize Encounter Handler: %1 Groups will be enroute to %2. From %3", _numberOfGroupstoSpawn, _defendSite.GetBaseName(), combinedLocations));
		
		m_Task = SCR_BaseTask.Cast(GetTaskManager().SpawnTask(m_SeizeConfig.GetTaskPrefab()));
		
		// set the position of our task
		vector mat[4];
		_defendSite.GetOwner().GetTransform(mat);
		m_Task.SetTransform(mat);
		
		// format the text that shall appear for the task
		taskTitle = string.Format("Defend %1", _defendSite.GetBaseName());
		taskDescription = string.Format("Intel reports %1 fighters are launching an offensive from %2 to %3. Defend %3 at all costs! You have about %4 minutes to prepare", usingFaction, combinedLocations, _defendSite.GetBaseName(), minutesLeft);
		taskSound = SCR_SoundEvent.SOUND_SIREN;
		
		// update task texts
		m_Task.SetTitle(taskTitle);
		m_Task.SetDescription(taskDescription);
		
		GetTaskManager().RegisterTask(m_Task);
		GetTaskManager().CreateTaskExecutors();
		
		ref array<int> playerIds = new ref array<int>;
		GetGame().GetPlayerManager().GetPlayers(playerIds);
		
		foreach(int playerId : playerIds)
		{
			SCR_BaseTaskExecutor executor = SCR_BaseTaskExecutor.GetTaskExecutorByID(playerId);
			
			if(!executor)
			{
				Print("Could not assign defend task", LogLevel.WARNING);
				continue;
			}
			
			Print(string.Format("Assigning defend task to %1", executor.GetPlayerName()));
			executor.AssignNewTask(m_Task);
		}
		
		// Display Task Text to everyone
		SCR_TW_AmbientEncounters.GetInstance().DisplayText(taskTitle, taskDescription, 15, taskSound);
		
		GetGame().GetCallqueue().CallLater(StartSeizePeriod, delay: SCR_TW_Util.FromMinutesToMilliseconds(minutesLeft));
		
		if(minutesLeft > 0)
		{
			GetGame().GetCallqueue().CallLater(CountdownMethod, delay: SCR_TW_Util.FromMinutesToMilliseconds(1));
		}
		
		Print(string.Format("The seize event will start in %1 minutes", minutesLeft));
	}
	
	void CountdownMethod()
	{
		Print(string.Format("CountdownMethod: Minutes Left %1", minutesLeft), LogLevel.DEBUG);
		
		if(minutesLeft <= 0)
			return;
		
		minutesLeft--;
		string description = "";
		
		if(minutesLeft > 0)
			description = string.Format("%1 Minutes Left!", minutesLeft);
		else
			description = "Less than 1 minute left!";
		
		SCR_TW_AmbientEncounters.GetInstance().DisplayText(taskTitle, description, 15, SCR_SoundEvent.SOUND_CP_POSITIVEFEEDBACK);
		
		if(minutesLeft > 0)
		{
			GetGame().GetCallqueue().CallLater(CountdownMethod, SCR_TW_Util.FromMinutesToMilliseconds(1));
		}
	}
	
	void StartSeizePeriod()
	{
		Print(string.Format("Starting Seize Period: %1", _defendSite.GetBaseName()), LogLevel.DEBUG);
		
		for(int i = 0; i < _numberOfGroupstoSpawn; i++)
		{
			SCR_CampaignMilitaryBaseComponent enemyBase = _basesToSpawnAt.GetRandomElement();
			SCR_AIGroup group = SCR_TW_Util.SpawnGroup(m_GroupPrefabsConfig.GetRandomInfantryPrefab(), enemyBase.GetOwner().GetOrigin(), 200);
			
			if(!group)
			{
				Print(string.Format("Failed to create attacking group from %1", enemyBase.GetBaseName()), LogLevel.WARNING);
				continue;
			}
			
			AIWaypoint waypoint = SCR_TW_Util.CreateWaypointAt(GetWaypointsConfig().GetAttackWaypointPrefab(), _defendSite.GetOwner().GetOrigin());
			
			if(!waypoint)
			{
				Print(string.Format("Failed to create waypoint for group %1", enemyBase.GetBaseName()), LogLevel.WARNING);				
				SCR_TW_Util.DeleteGroup(group);
			}
			
			group.AddWaypoint(waypoint);
			m_SpawnedGroups.Insert(group);
		}
		
		string text = string.Format("Hold the line!! Recon indicates a force of about %1 squads are inbound", m_SpawnedGroups.Count());
		Print(string.Format("%1 groups are associated with this defend task", m_SpawnedGroups.Count()));
		
		SCR_TW_AmbientEncounters.GetInstance().DisplayText(taskTitle, text, 15, SCR_SoundEvent.SOUND_SIREN);
		_hasStarted = true;
		currentPhase = SCR_TW_EncounterPhase.Active;
	}
};