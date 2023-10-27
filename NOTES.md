
# Things that are interesting about IpuDoom

 - Fixed precision int math
    - 64-bit int division
        - wasn't supported in the supervisor context, drop into a worker thread for each individual division, do double division
- Avoid recursive algorithms, e.g. make a non-recursive BSP
- Split column rendering over 32 tiles
- Memory
    - Save 50-60k of lookup tabes for transcendentals by using float functions
    - Store textures on other tiles, JIT-fetch as needed (by live patching exchange programs to select a tile)
        - Can call exchanges from deep in the call stack, don't need to exit the vertex


#### Things I don't support, to make my life easier:
 - gamemodes other than shareware
 - multiplayer
 - cheating
