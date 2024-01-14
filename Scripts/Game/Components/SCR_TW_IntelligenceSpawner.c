class SCR_TW_IntelligenceSpawnerComponentClass : ScriptComponentClass {};

class SCR_TW_IntelligenceSpawnerComponent : ScriptComponent
{
	[Attribute("1", UIWidgets.Slider, params: "1 10 1", desc: "Guaranteed number of intelligence to spawn", category: "Spawn")]
	private int m_IntelligenceCount;
	
	[Attribute("{6D56FED1E55A8F84}Prefabs/Items/Misc/IntelligenceFolder_E_01/IntelligenceFolder_E_01.et", UIWidgets.ResourceNamePicker)]
	private ResourceName m_IntelligencePrefab;
	
	void SpawnIntelligence()
	{
		InventoryStorageManagerComponent manager = InventoryStorageManagerComponent.Cast(FindComponent(InventoryStorageManagerComponent));
		
		for(int i = 0; i < m_IntelligenceCount; i++)
		{
			bool success = manager.TrySpawnPrefabToStorage(m_IntelligencePrefab, purpose: EStoragePurpose.PURPOSE_DEPOSIT);	
			
			if(!success)
				Print(string.Format("TrainWreck: Was unable to spawn intelligence folder. %1", m_IntelligencePrefab), LogLevel.ERROR);
		}
	}
}