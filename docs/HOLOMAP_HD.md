# Redrawn holomap maps

Each island map the holomap zooms into is a 640x480, 256-colour picture in `holomap.hqr` (entries 18, 20, ... 44: twelve islands, the Citadel after the storm and Celebration after the party; `HQR_BEGIN_MAP` in [HOLO.H](../SOURCES/HOLO.H)). The arrows, Twinsen, the buggy and the dino-fly are 3D models drawn over it at positions taken from the camera record stored beside each picture.

A redrawn picture can stand in for one. [scripts/holomap_export.py](../scripts/holomap_export.py) saves the originals as `holomap_<entry>.png`, in the game palette (`ress.hqr` entry 0), with nothing but Python 3. A picture of the same name in an `hd` folder beside the game data (`sdmc:/lba2/hd/` on the Switch) is then shown in place of the original, scaled to the map's rectangle at the screen's own resolution. Any 4:3 size works; the islands must keep their place and shape, because the models are still drawn where the original puts them. About 1920x1440 is as much as a 1080p screen can show.

A widescreen picture (wider than 4:3, best exactly 16:9, such as 1920x1080) also works: its middle 4:3 (1440x1080 of a 1920x1080 picture) is the map, with the same rule about islands, and what is left and right of it fills the black beside the map: first the frame's own columns beside it when the frame is wider than 640 (`HoloHD_SetView` widens the backdrop rectangle and its key over them), then the bars beside the frame on the screen (`PlatformBackdrop::spill`). The 3D models stay inside the middle part.

## How it is shown

`HoloHD_Begin` in [HOLOHD.CPP](../SOURCES/HOLOHD.CPP), called by `InitHoloPlan` once the indexed map is in `Log`, loads the PNG (`LoadPNG_ARGB`, stb_image). Each time the map is drawn at a zoom, `HoloHD_SetView` keeps a copy of the indexed map as it now stands and the part of the picture it shows (the backdrop's `src_*` rectangle), and hands both to the present as a `PlatformBackdrop` (`SetPresentBackdrop` in [SDL.CPP](../LIB386/SVGA/SDL.CPP)); `ClearHoloPlan` takes it away (`HoloHD_End`).

The SDL2 backend ([WINDOW.CPP](../LIB386/PLATFORM/SDL2/WINDOW.CPP), `ApplyBackdrop`) clears the alpha of every converted frame pixel inside the map's rectangle whose index still equals the copy, draws the picture into that rectangle, and the frame over it with blending. Anything drawn over the map (the models, the corner brackets, the text and its window) differs from the copy and stays the frame's. The picture is dimmed by the ratio of the frame palette to the palette the map fades in to, so fades take it with them. Other backends ignore the backdrop and show the original map.

## Looking at other islands

[HOLOPLAN.CPP](../SOURCES/HOLOPLAN.CPP) and [HOLOGLOB.CPP](../SOURCES/HOLOGLOB.CPP) add, to the original holomap:

- Going from the globe to a map fades the globe out and the whole map in (the original grew the map out of the cursor in a box, which left a widescreen map's sides apart from it). The name strip at the bottom is as wide as the map (`HoloInitDialWindowSpan`).
- From the globe, the map of any island Twinsen knows opens, not only his own (`HoloPlan_IslandKnown`: he is on it, or one of its cubes is marked).
- On the map, L / R (Page Up / Page Down) step to the previous / next known island. It only changes what is shown; Twinsen stays where he is.
- The right stick zooms in (up) and out (down), from 1x to 3x, centred on the chosen place; choosing another place with the D-pad while zoomed brings the view to it. The map is scaled from a clean copy (`HoloPlanDrawMap`) and the projection follows, so the models stay on their spots.

## The globe's surface

The globes are 3D meshes wrapped in an indexed texture 256 texels wide (holomap.hqr entries 2, 4, 6, 8 and 9; Twinsun is entry 2), drawn by the software polygon filler, which only reads 256-wide textures. [scripts/holomap_export.py](../scripts/holomap_export.py) also saves them as `hologlobe_<entry>.png`. A picture of the same name in the `hd` folder replaces one when the holomap opens (`HoloHD_ReplaceTexture`): it is scaled to the texture's size and each pixel becomes the nearest of the colours the original texture uses, so it can fix or restyle the surface but not add detail.
