

![IPUDOOM](IPUDOOM.png)

A WIP to put Doom 1993 on the IPU.

```
# Install dependencies
sudo apt update; sudo apt-get install -y libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libsdl2-net-dev libpng-dev g++-7
# Build binary
make
# Download shareware resource pack
wget https://distro.ibiblio.org/slitaz/sources/packages/d/doom1.wad
# Run
./build/doom -iwad doom1.wad -width 320 -nosound 
```



Activity Log:

- [x] Run Doom on CPU

- [x] POC IPU modifies frame buffer every frame

- [x] Create IPU hooks for key methods like G_Ticker, G_Responder so IPU can step time and respond to keypresses

- [x] Move level geometry to IPU on level load

- [x] Implement all methods used by the automap (vector rendering, sprite rendering, AM event responder)

- [x] Automap disabled on CPU, runs and renders entirely on IPU! (enabling CPU to continue raycasting to update map)

  ![automap](https://static.wikia.nocookie.net/doom/images/9/9c/Automap.png)

- [ ] ~~Either implement coroutines in x86 to allow further dev on IPUModel~~, or implement frame streaming to local machine to allow dev on remote IPU

- [ ] Move core game loop to IPU as a coroutine, triggering CPU callbacks every tic to do unimplemented work

- [ ] Continue implementing main game systems on IPU...

  ...

- [ ] Profit

