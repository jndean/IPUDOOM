
![IPUDOOM](README_imgs/IPUDOOM.png)

A WIP to put Doom 1993 on the Graphcore Mk2 IPU (an AI accelerator chip).

```bash
# Install dependencies
sudo apt update 
sudo apt-get install -y libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libsdl2-net-dev libpng-dev g++-7
# Build binary
make
# Download shareware resource pack
wget https://distro.ibiblio.org/slitaz/sources/packages/d/doom1.wad
# Run
./build/doom -iwad doom1.wad -width 320 -nosound -nomouse
```


### Progress Log:
Started with vanilla Doom running on the CPU, began started offloading subsystems to the IPU (just Tile 0  at first): 

- [x] Create IPU hooks for key methods like G_Ticker and G_Responder so IPU can step game time and respond to keypresses in real time. IPU uses callbacks on the host to load and unpack all level geometry from disk whenever the player starts a new level.

- [x] Implement an IPU-specific memory allocator with seperate memory pools for different lifetimes: static lifetime, level lifetime, frame lifetime. Makes fragmanetation easy to avoid => reduces SRAM footprint.

- [x] Implement all methods used by the automap (vector rendering, sprite rendering, AM event responder). Automap is now disabled on CPU, renders entirely on IPU.

![Automap](README_imgs/Automap.gif)

- [x] Implement BinarySpacePartion search (stackless recursion version for IPU), solidseg occlusion and floor/ceiling clipping to get (untextured) rendering of vertical walls running on a sinlge IPU tile.

![Gameplay with untextured walls](README_imgs/BlankWalls_noCPU.gif)

- [x] Split rendering across 32 render tiles. Reformat textures into a big buffer that can be striped over other dedicated texture tiles, and accessed by the render tiles using JIT-patched exchange programs to enable fetches based on dynamic indices. So now the IPU can texture walls in real time.

![Gameplay with textured walls (but nothing else)](README_imgs/WallsTextured_noCPU.gif) ![Gameplay showing rendering striped across 32 tiles](README_imgs/WallsTileGrey_noCPU.gif )

- [x] Implement lighting model (add shadows to walls): the texture tiles translate the colours during texture column fetch requests to save SRAM on the render tiles.

![Side-by-side of room with and without shadows](README_imgs/WallsLighting.PNG)

- [x] Port the visplane system to the render tiles, so IPU can render floors and ceilings, (untextured for now - instead coloured to show the visplane subdivision). Also implement skybox.

![Gameplay with visplanes and skybox visible](README_imgs/VisplanesSkybox_noCPU.gif)

- [x] Extend the JIT-patching texture exchange to support span textures and zlighting, so IPU can texture and shade floors + ceilings.

[GIF TODO]

Immediate next steps:
- [ ] Implement temporary(?) system to notify IPU of map state changes, so that doors open and close properly. Perhaps the same could work for enemys and projectiles.
- [ ] Implement vissprite system and masked col rendering to add objects / enemies to levels. 

Longer term next steps:

- [ ] Render HUD (probably using dedicated HUD tiles)
- [ ] Move beyond just the rendering?

  ...

- [ ] Profit?

