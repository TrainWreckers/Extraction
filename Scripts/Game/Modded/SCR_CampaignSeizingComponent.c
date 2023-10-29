modded class SCR_SeizingComponent
{
	override void OnPostInit(IEntity owner)
	{
		m_bIgnoreNonPlayableDefenders = false;
		m_bIgnoreNonPlayableAttackers = false;
		
		super.OnPostInit(owner);

		m_RplComponent = RplComponent.Cast(owner.FindComponent(RplComponent));

		// All functionality is server-side
		if (IsProxy())
			return;

		// Attributes check
		if (m_iRadius <= 0)
		{
			Print("SCR_SeizingComponent: Invalid area radius (" + m_iRadius + ")! Terminating...", LogLevel.ERROR);
			return;
		}

		if (m_fMaximumSeizingTime < 0 || m_fMinimumSeizingTime < 0 || m_fMaximumSeizingTime < m_fMinimumSeizingTime)
		{
			Print("SCR_SeizingComponent: Invalid seizing time setting (" + m_fMinimumSeizingTime + ", " + m_fMaximumSeizingTime + ")! Terminating...", LogLevel.ERROR);
			return;
		}

		if (m_iMaximumSeizingCharacters <= 0)
		{
			Print("SCR_SeizingComponent: Invalid maximum seizing characters (" + m_iMaximumSeizingCharacters + ")! Terminating...", LogLevel.ERROR);
			return;
		}

		if (!GetGame().InPlayMode())
			return;

		SCR_SeizingComponentClass componentData = SCR_SeizingComponentClass.Cast(GetComponentData(GetOwner()));

		if (!componentData)
			return;

		m_FactionControl = SCR_FactionAffiliationComponent.Cast(owner.FindComponent(SCR_FactionAffiliationComponent));

		if (!m_FactionControl)
		{
			Print("SCR_SeizingComponent: Owner is missing SCR_FactionAffiliationComponent! Terminating...", LogLevel.ERROR);
			return;
		}

		Resource triggerResource = Resource.Load(componentData.GetTriggerPrefab());

		if (!triggerResource)
		{
			Print("SCR_SeizingComponent: Trigger resource failed to load! Terminating...", LogLevel.ERROR);
			return;
		}

		// Spawn the trigger locally on server
		m_Trigger = BaseGameTriggerEntity.Cast(GetGame().SpawnEntityPrefabLocal(triggerResource, GetGame().GetWorld()));

		if (!m_Trigger)
		{
			Print("SCR_SeizingComponent: Trigger failed to spawn! Terminating...", LogLevel.ERROR);
			return;
		}

		m_Trigger.SetSphereRadius(m_iRadius);
		owner.AddChild(m_Trigger, -1);

		// Register after-respawn cooldown method
		if (m_fRespawnCooldownPeriod > 0)
		{
			SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());

			if (gameMode)
				gameMode.GetOnPlayerSpawned().Insert(OnPlayerSpawned);
		}

		GetGame().GetCallqueue().CallLater(EvaluatePrevailingFaction, Math.RandomFloatInclusive(TRIGGER_CHECK_PERIOD_IDLE, TRIGGER_CHECK_PERIOD_IDLE + (TRIGGER_CHECK_PERIOD_IDLE * 0.2)) * 1000);
	}
}