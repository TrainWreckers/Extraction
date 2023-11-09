/*
	This is intended to be used as a data structure for serializing/deserializing 
	TrainWreck loot tables.
*/

class TW_LootConfigItem
{
	string resourceName;
	int chanceToSpawn;
	int randomSpawnCount;
	ref array<string> tags;
	
	void SetData(ResourceName resource, int chance, int spawnCount, array<string> tags = null)
	{
		this.resourceName = resource;
		this.chanceToSpawn = chance;
		this.randomSpawnCount = spawnCount;
		
		if(tags)
			this.tags = tags;
		else 
			this.tags = {};
	}
}