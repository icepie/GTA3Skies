# GTA3 Android Wet Road Reflections

## Background
In the original PC/PS2 versions of Grand Theft Auto III and Vice City, when it rains (`CWeather::WetRoads > 0`), the game renders long, vertical streaks of light on the asphalt below streetlights and neon signs. This significantly enhances the rainy atmosphere.

During the development of the 10th Anniversary Android port, Rockstar Games completely stubbed out this feature for performance reasons. If you reverse engineer the Android `libR1.so`, the function responsible for this (`CCoronas::RenderReflections`) looks like this:

```cpp
/* CCoronas::RenderReflections() */
void CCoronas::RenderReflections(void)
{
  return; // Completely stubbed out!
}
```

## How GTA3Skies Restores It
Since the engine's original function is empty, we cannot simply flip a configuration flag. Instead, the `GTA3Skies` mod hooks the empty `RenderReflections` call and manually reimplements the PC logic in C++:

1. **Iterating Coronas**: We iterate through the engine's `CCoronas::aCoronas` array (which holds all active lights).
2. **Raycasting**: We use the engine's `CWorld::ProcessVerticalLine` to shoot a ray straight down from each light source to find the exact height of the ground beneath it (`heightAboveRoad`).
3. **Z-Mirroring**: We mirror the light's Z-coordinate below the ground.
4. **Buffered Rendering**: We use `RenderBufferedOneXLUSprite` to draw a stretched, semi-transparent light streak at the mirrored coordinate. Using the *buffered* variant is critical, as it ensures the reflections are drawn at the very end of the rendering pipeline, blending perfectly with the road without appearing "on top" of cars or pedestrians.

## Configuration (GTA3Skies.ini)
You can toggle this feature and adjust its behavior in `GTA3Skies.ini`:

```ini
[Preferences]
GTA3WetRoadReflections=1
```

## Debugging
For easier development, you can force the reflections to appear even when it's not raining (e.g., using a CLEO quick-weather script) by using the debug variables in the code.
