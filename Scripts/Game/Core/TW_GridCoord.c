class TW_GridCoord<Class T>
{
	int x;
	int y;
	
	private ref array<ref T> data;
	
	void TW_GridCoord(int x, int y)
	{
		this.x = x;
		this.y = y;
	}		
	
	void Add(T item) { data.Insert(item); }
	void RemoveItem(T item) { data.RemoveItem(item); }
	
	int GetData(notnull out array<T> items)
	{
		int count = data.Count();
		
		for(int i = 0; i < count; i++)
			items.Insert(data.Get(i));
		
		return count;
	}
};

class TW_GridCoordManager<Class T>
{
	private ref map<int, map<int, ref TW_GridCoord<ref T>>> grid = new map<int, map<int, ref TW_GridCoord<ref T>>>();
	
	//! Does grid have coordinate
	bool HasCoord(int x, int y)
	{
		if(!grid.Contains(x))
			return false;
		
		if(!grid.Get(x).Contains(y))
			return false;
		
		return true;
	}
	
	//! Retrieve Coordinate
	TW_GridCoord GetCoord(int x, int y)
	{
		if(!HasCoord(x, y))
			return null;
		
		return grid.Get(x).Get(y);
	}
	
	void InsertCoord(TW_GridCoord coord)
	{
		// If not X coordinate exists we have to insert x and y
		if(!grid.Contains(coord.x))
		{
			grid.Insert(coord.x, new map<int, ref TW_GridCoord<ref T>>());
			grid.Get(coord.x).Insert(y, coord);
		}
		
		// If X but no Y -- insert Y and coord
		if(!grid.Get(coord.x).Contains(coord.y))
			grid.Get(coord.x).Insert(coord.y, coord);
		
	}
	
	//! Retrieve coords around center
	int GetNeighbors(notnull out array<ref TW_GridCoord> items, int x, int y, int radius = 1, bool includeCenter = false)
	{
		int leftBounds = x - radius;
		int rightBounds = x + radius;
		int upperBounds = y + radius;
		int lowerBounds = y - radius;
		int count = 0;
		
		for(int gridX = leftBounds; gridX < rightBounds; gridX++)
		{
			for(int gridY = lowerBounds; gridY < upperBounds; gridY++)
			{
				if(gridX == x && gridY == y && !includeCenter)
					continue;
				
				if(HasCoord(gridX, gridY))
				{
					items.Insert(GetCoord(gridX, gridY));
					count++;
				}
			}
		}
		
		return count;
	}
}