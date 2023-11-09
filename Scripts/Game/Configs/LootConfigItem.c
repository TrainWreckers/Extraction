/*
	This is intended to be used as a data structure for serializing/deserializing 
	TrainWreck loot tables.
*/

class TW_LootConfigItem : Managed
{
	string resourceName;
	float chanceToSpawn;
	int randomSpawnCount;
	string tags;
	
	void SetData(ResourceName resource, float chance, int spawnCount, string tags = string.Empty)
	{
		this.resourceName = resource;
		this.chanceToSpawn = chance;
		this.randomSpawnCount = spawnCount;
		this.tags = tags;
	}
}