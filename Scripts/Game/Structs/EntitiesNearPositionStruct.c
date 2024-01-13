//! Data struct for associating entities around a given position. 
class EntitiesNearPositionStruct : TW_Struct
{
	vector Position;
	ref array<IEntity> Entities;
	
	void EntitiesNearPositionStruct(vector position, notnull array<IEntity> entities)
	{
		Entities = entities;
		Position = position;
	}
}