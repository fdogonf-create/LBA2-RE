# Redrawn holomap maps

Each island map the holomap zooms into is a 640x480, 256-colour picture in `holomap.hqr` (entries 18, 20, ... 44: twelve islands, the Citadel after the storm and Celebration after the party; `HQR_BEGIN_MAP` in [HOLO.H](../SOURCES/HOLO.H)). The arrows, Twinsen, the buggy and the dino-fly are 3D models drawn over it at positions taken from the camera record stored beside each picture.

A redrawn picture can stand in for one. [scripts/holomap_export.py](../scripts/holomap_export.py) saves the originals as `holomap_<entry>.png`, in the game palette (`ress.hqr` entry 0), with nothing but Python 3. A picture of the same name in an `hd` folder beside the game data (`sdmc:/lba2/hd/` on the Switch) is then shown in place of the original, scaled to the map's rectangle at the screen's own resolution. Any 4:3 size works; the islands must keep their place and shape, because the models are still drawn where the original puts them. About 1920x1440 is as much as a 1080p screen can show.

## How it is shown

`HoloHD_Begin` in [HOLOHD.CPP](../SOURCES/HOLOHD.CPP), called by `InitHoloPlan` once the indexed map is in `Log`, loads the PNG (`LoadPNG_ARGB`, stb_image) and keeps a copy of the indexed map as it was drawn. It hands both to the present as a `PlatformBackdrop` (`SetPresentBackdrop` in [SDL.CPP](../LIB386/SVGA/SDL.CPP)); `ClearHoloPlan` takes it away (`HoloHD_End`).

The SDL2 backend ([WINDOW.CPP](../LIB386/PLATFORM/SDL2/WINDOW.CPP), `ApplyBackdrop`) clears the alpha of every converted frame pixel inside the map's rectangle whose index still equals the copy, draws the picture into that rectangle, and the frame over it with blending. Anything drawn over the map (the models, the corner brackets, the text and its window, the zoom-in animation) differs from the copy and stays the frame's. The picture is dimmed by the ratio of the frame palette to the palette the map fades in to, so fades take it with them. Other backends ignore the backdrop and show the original map.
