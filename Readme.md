
## Spawn System

With upwards of thousands of spawn points in a world we came across a combination of issues. 

To keep things interesting, things need to happen around players! Doesn't make sense for things to spawn across the map. So the implementation needs to be fast and run fairly frequent.

1. Players move around a lot. Thus, the spawn system needs to be fairly active.
2. Units outside of player areas should be cleaned up.
3. Things shouldn't spawn too close to player


### World Wide

Originally, we utilized a world-wide system where N amount of spawn points were queried P amount of players. (N * P). 

- This requires distance calculation checks which is slow. Thus should be ran infrequently.
- Since we already performed the check, we can use the same value to check minimum spawn distance

![](./worldwide.gif)

### Radius

Can use the engine's built-in query entities in sphere. This still performs distance checks, however we'd also need to perform secondary distance checks to ensure they are outside of a minimum spawn radius.

![](./radius.gif)

### Chunks

Similar to minecraft we can create a `chunk` system. When spawn points, or anything we need we can register them against a grid! 

```txt
- Divide position by grid size
- Register this coordinate against a map/dictionary
- Player Position / grid size ->
    +/- chunk radius - can activate/deactivate spawn points
      since we have references to all things in that coordinate we can easily manage/work with things.
```

![](./chunks.gif)