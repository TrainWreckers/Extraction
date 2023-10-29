modded class SCR_CampaignMilitaryBaseComponent
{
	//------------------------------------------------------------------------------------------------
	//! Event which is triggered when the owning faction changes
	override protected void OnFactionChanged(FactionAffiliationComponent owner, Faction previousFaction, Faction faction)
	{
		super.OnFactionChanged(owner, previousFaction, faction);

		if (!GetGame().InPlayMode())
			return;

		SCR_CampaignFaction newCampaignFaction = SCR_CampaignFaction.Cast(faction);

		if (!newCampaignFaction)
			return;

		SCR_GameModeCampaign campaign = SCR_GameModeCampaign.GetInstance();

		if (!campaign)
			return;

		float curTime = GetGame().GetWorld().GetWorldTime();

		if (!IsProxy())
		{
			EndCapture();
			m_mDefendersData.Clear();

			// Update signal coverage only if the base was seized during normal play, not at the start
			if (curTime > 10000)
			{
				campaign.GetBaseManager().RecalculateRadioConverage(campaign.GetFactionByEnum(SCR_ECampaignFaction.BLUFOR));
				campaign.GetBaseManager().RecalculateRadioConverage(campaign.GetFactionByEnum(SCR_ECampaignFaction.OPFOR));
			}

			ChangeRadioSettings(newCampaignFaction);

			// Reset timer for reinforcements
			if (IsHQ())
			{
				SupplyIncomeTimer(true);
				Replication.BumpMe();
			}

			// Delay respawn possibility at newly-captured bases
			if (!m_bIsHQ && m_bInitialized && newCampaignFaction.IsPlayable() && campaign.GetBaseManager().IsBasesInitDone() && curTime > SCR_GameModeCampaign.BACKEND_DELAY)
			{
				#ifndef AR_CAMPAIGN_TIMESTAMP
				m_fRespawnAvailableSince = Replication.Time() + RESPAWN_DELAY_AFTER_CAPTURE;
				#else
				ChimeraWorld world = GetOwner().GetWorld();
				m_fRespawnAvailableSince = world.GetServerTimestamp().PlusMilliseconds(RESPAWN_DELAY_AFTER_CAPTURE);
				#endif
				OnRespawnCooldownChanged();
				Replication.BumpMe();
			}

			HandleSpawnPointFaction();
			SendHQMessageBaseCaptured();

			// If some Remnants live, send them to recapture
			if (newCampaignFaction.IsPlayable())
			{
				foreach (SCR_AmbientPatrolSpawnPointComponent remnants : m_aRemnants)
				{
					AIGroup grp = remnants.GetSpawnedGroup();

					if (!grp)
						continue;

					if (!m_RetakeWP && m_HQRadio)
					{
						EntitySpawnParams params = EntitySpawnParams();
						params.TransformMode = ETransformMode.WORLD;
						params.Transform[3] = m_HQRadio.GetOrigin();
						m_SeekDestroyWP = SCR_TimedWaypoint.Cast(GetGame().SpawnEntityPrefabLocal(Resource.Load(SCR_GameModeCampaign.GetInstance().GetSeekDestroyWaypointPrefab()), null, params));
						m_SeekDestroyWP.SetHoldingTime(60);
					}

					if (m_SeekDestroyWP)
						grp.AddWaypointAt(m_SeekDestroyWP, 0);
				}
			}

			// Change barrack group according to owner's faction, if it was built
			if (m_BarrackComponent)
			{
				if (!newCampaignFaction.IsPlayable())
					m_BarrackComponent.StopHandler();
				else
				{
					ResourceName defenderGroup = newCampaignFaction.GetDefendersGroupPrefab();

					if (defenderGroup)
					{
						m_BarrackComponent.EnableSpawning(false);
						m_BarrackComponent.SetGroupPrefab(defenderGroup);
						if (!m_BarrackComponent.IsInitialized())
							m_BarrackComponent.InitializeBarrack();

						GetGame().GetCallqueue().CallLater(m_BarrackComponent.EnableSpawning, m_BarrackComponent.GetRespawnDelay()* 1000, false, true);
					}
				}
			}

			// Change owner of assigned armory
			if (m_ArmoryComponent)
				m_ArmoryComponent.ChangeOwningFaction(GetFaction());

			// Change owner of assigned armory
			if (m_RadioArmory)
				m_RadioArmory.ChangeOwningFaction(GetFaction());

			if (GetGame().GetWorld().GetWorldTime() != 0)
				GetGame().GetSaveManager().Save(ESaveType.AUTO);
		}

		if (RplSession.Mode() != RplMode.Dedicated)
		{
			SCR_CampaignFeedbackComponent feedback = SCR_CampaignFeedbackComponent.GetInstance();

			if (feedback)
				feedback.FlashBaseIcon(this, changeToDefault: true);

			if (m_MapDescriptor)
				m_MapDescriptor.HandleMapInfo();

			SetRadioChatterSignal(newCampaignFaction);

			SCR_CampaignFaction playerFaction = SCR_CampaignFaction.Cast(SCR_FactionManager.SGetLocalPlayerFaction());
			IEntity player = SCR_PlayerController.GetLocalControlledEntity();

			// TODO: Move this to PlayRadioMsg so it is checked for player being inside radio range
			if (campaign)
			{
				if (player && playerFaction && playerFaction != GetCampaignFaction() && IsHQRadioTrafficPossible(playerFaction))
				{
					if (GetCampaignFaction())
						SCR_PopUpNotification.GetInstance().PopupMsg("#AR-Campaign_BaseSeized-UC", prio: SCR_ECampaignPopupPriority.BASE_LOST, param1: GetCampaignFaction().GetFactionNameUpperCase(), param2: GetBaseNameUpperCase());
					else
					{
						SCR_CampaignFaction factionR = campaign.GetFactionByEnum(SCR_ECampaignFaction.BLUFOR);

						if (factionR)
							SCR_PopUpNotification.GetInstance().PopupMsg("#AR-Campaign_BaseSeized-UC", prio: SCR_ECampaignPopupPriority.BASE_LOST, param1: factionR.GetFactionNameUpperCase(), param2: GetBaseNameUpperCase());
					}

					if (playerFaction == m_OwningFactionPrevious)
						//campaign.ShowHint(SCR_ECampaignHints.BASE_LOST);
				}
			}
		}
		
		bool shouldDefend = newCampaignFaction.GetFactionKey() == SCR_TW_Component.GetInstance().GetINDFORFactionKey();
		
		SCR_TW_CompositionHandler.GetInstance().OnFactionChange(this, shouldDefend);
		m_OwningFactionPrevious = newCampaignFaction;
	}
}