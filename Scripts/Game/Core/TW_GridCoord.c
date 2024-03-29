class TW_GridCoord<Class T>
{
	int x;
	int y;
	
	private ref array<T> data = {};
	
	void TW_GridCoord(int x, int y)
	{
		this.x = x;
		this.y = y;
	}		
	
	void Add(T item) { data.Insert(item); }
	void RemoveItem(T item) { data.RemoveItem(item); }
	
	T GetRandomElement()
	{
		return data.GetRandomElement();
	}
	
	array<T> GetAll() { return data; }
	
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
	private ref map<int, ref map<int, ref TW_GridCoord<T>>> grid = new map<int, ref map<int, ref TW_GridCoord<T>>>();
	private int GridSize = 1000;
	
	private ref set<string> activeCoords = new set<string>();
	
	private int pointerIndex = -1;
	private int activeCoordsCount = 0;
	
	void TW_GridCoordManager(int gridSize = 1000)
	{
		GridSize = gridSize;
	}
	
	//! Remove multiple chunks
	void RemoveCoords(notnull set<string> coords)
	{
		int x, y;
		foreach(string coord : coords)
		{
			SCR_TW_Util.FromGridString(coord, x, y);
			
			if(!HasCoord(x, y))
				continue;
			
			ref map<int, ref TW_GridCoord<T>> sub = grid.Get(x);
			sub.Remove(y);
			
			if(sub.Count() <= 0)
				sub.Remove(x);
		}
	}
	
	//! Remove sector from grid
	void RemoveCoord(vector position)
	{
		int x = (int)(position[0] / GridSize);
		int y = (int)(position[2] / GridSize);
		
		if(!HasCoord(x,y))
			return;
		
		ref map<int, ref TW_GridCoord<T>> sub = grid.Get(x);
		sub.Remove(y);
		
		// If nothing is left --> delete the X as well
		if(sub.Count() <= 0)
			grid.Remove(x);
	}
	
	//! Remove item from its sector
	void RemoveItem(vector position, T item)
	{
		int x = (int)(position[0] / GridSize);
		int y = (int)(position[2] / GridSize);
		
		if(!HasCoord(x,y))
			return;
		
		ref TW_GridCoord<T> coord = GetCoord(x, y);
		coord.RemoveItem(item);
	}	
	
	//! Round robin through grid, return random item.
	T GetNextItemFromPointer(notnull set<string> incomingCoords)
	{
		if(incomingCoords != activeCoords || incomingCoords.Count() != activeCoords.Count())
		{
			Print(string.Format("TrainWreck: Updating active coords %1", activeCoords));
			activeCoords = incomingCoords;
			activeCoordsCount = activeCoords.Count();	
			pointerIndex = -1; // Reset to beginning
		}
		
		// Increment, round robin style
		pointerIndex++;
		
		// Clamp
		if(pointerIndex >= activeCoordsCount)
			pointerIndex = 0;
		
		// Convert text coords into something we can grab		
		string textCoord = activeCoords.Get(pointerIndex);
		int x, y;
		SCR_TW_Util.FromGridString(textCoord, x, y);
		
		if(!HasCoord(x,y))
			return null;
		
		ref TW_GridCoord<T> coord = GetCoord(x,y);
		
		// Failsafe in the event the value is null
		if(!coord) 
			return null;
		
		// Grab random item from coord
		return coord.GetRandomElement();
	}		
	
	int GetCoordCount(out int emptyValues)
	{
		int count = 0;
		
		foreach(int x, ref map<int, ref TW_GridCoord<T>> yCoords : grid)
		{
			foreach(int y, ref TW_GridCoord<T> coord : yCoords)
			{
				count++;
				
				if(!coord)
					emptyValues++;
			}
		}
		
		return count;
	}
	
	//! Does grid have coordinate
	bool HasCoord(int x, int y)
	{
		if(!grid.Contains(x))
			return false;
		
		map<int, ref TW_GridCoord<T>> sub = grid.Get(x);
		
		return sub.Contains(y);
	}
	
	//! Retrieve Coordinate
	TW_GridCoord<T> GetCoord(int x, int y)
	{
		if(!HasCoord(x, y))
			return null;
		
		return grid.Get(x).Get(y);
	}
	
	void InsertCoord(TW_GridCoord<T> coord)
	{
		// If not X coordinate exists we have to insert x and y
		if(!grid.Contains(coord.x))
		{
			ref map<int, ref TW_GridCoord<T>> sub = new map<int, ref TW_GridCoord<T>>();
			sub.Insert(coord.y, coord);			
			grid.Insert(coord.x, sub);
		}
		
		// If X but no Y -- insert Y and coord
		if(!grid.Get(coord.x).Contains(coord.y))
		{
			ref map<int, ref TW_GridCoord<T>> sub = grid.Get(coord.x);
			Print(string.Format("TrainWreck Inserting Coord<%1, %2>", coord.x, coord.y));
			sub.Insert(coord.y, coord);
		}
	}
	
	//! Try to insert item via world position
	void InsertByWorld(vector position, T item)
	{
		int x = (int)(position[0] / GridSize);
		int y = (int)(position[2] / GridSize);
		
		ref TW_GridCoord<T> coord;
		
		if(!HasCoord(x, y))
		{
			coord = new TW_GridCoord<T>(x,y);
			InsertCoord(coord);
		}		
		else 
			coord = GetCoord(x,y);
		
		coord.Add(item);		
	}
	
	//! Retrieve coords around center
	int GetNeighbors(notnull out array<ref TW_GridCoord<T>> items, int x, int y, int radius = 1, bool includeCenter = true)
	{
		int leftBounds = x - radius;
		int rightBounds = x + radius;
		int upperBounds = y + radius;
		int lowerBounds = y - radius;
		int count = 0;
		
		for(int gridX = leftBounds; gridX <= rightBounds; gridX++)
		{
			for(int gridY = lowerBounds; gridY <= upperBounds; gridY++)
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
	
	//! Get all chunks around player positions
	int GetChunksAround(notnull out array<ref TW_GridCoord<T>> chunks, notnull set<string> textCoords, int radius = 1)
	{
		ref set<string> completedCoords = new set<string>();
		
		int x;
		int y;
		int totalCount = 0;
		
		foreach(string textCoord : textCoords)
		{
			if(completedCoords.Contains(textCoord))
				continue;
			
			SCR_TW_Util.FromGridString(textCoord, x, y);
					
			// If this coordinate has already been checked -- continue
			if(completedCoords.Contains(textCoord))
				continue;
					
			completedCoords.Insert(textCoord);
				
			if(HasCoord(x, y))
			{
				ref TW_GridCoord<T> chunk = GetCoord(x, y);
				chunks.Insert(chunk);
				totalCount++;
			}			
		}
		
		return totalCount;
	}
	
	int GetNeighborsAround(notnull out array<T> data, notnull set<string> textCoords, int radius = 1)
	{
		ref set<string> completedCoords = new set<string>();
		
		int x;
		int y;
		int totalCount = 0;
		
		foreach(string textCoord : textCoords)
		{
			if(completedCoords.Contains(textCoord))
				continue;
			
			SCR_TW_Util.FromGridString(textCoord, x, y);
			
			// If this coordinate has already been checked -- continue
			if(completedCoords.Contains(textCoord))
				continue;
					
			completedCoords.Insert(textCoord);
					
			if(HasCoord(x, y))
			{
				ref TW_GridCoord<T> chunk = GetCoord(x, y);
				totalCount += chunk.GetData(data);
			}
		}
		
		return totalCount;
	}	
	
	int GetNeighboringItems(notnull out array<T> items, int x, int y, int radius = 1, bool includeCenter = true)
	{
		ref array<ref TW_GridCoord<T>> neighbors = {};
		int chunks = GetNeighbors(neighbors, x, y, radius, includeCenter);
		
		if(chunks < 0)
			return 0;
		
		int count = 0;
		foreach(ref TW_GridCoord<T> chunk : neighbors)
		{
			count += chunk.GetData(items);
		}
		
		return count;
	}
	
	int GetAllItems(notnull out array<T> items)
	{
		int count = 0;
		
		foreach(int x, ref map<int, ref TW_GridCoord<T>> values : grid)
			foreach(int y, ref TW_GridCoord<T> value : values)
				count += value.GetData(items);
		
		return count;
	}
}