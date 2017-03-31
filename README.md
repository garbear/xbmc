<a href="http://www.youtube.com/watch?feature=player_embedded&v=Moh67r0iBGA
" target="_blank"><img src="http://img.youtube.com/vi/Moh67r0iBGA/0.jpg" 
alt="retroplayer" width="240" height="180" border="10"/></a>

# Overview

RetroPlayer is a new player core for Kodi Entertainment Center. It is similar to the video and audio players, but it plays games instead of movies and music.

# Design and Components

**RetroPlayer:** Player core that plays games on the virtual file system (VFS) using game add-ons. Despite its name, it can play all types of games, not just retro ones. It borrows many ideas from the video player. Games can be paused, fast-forwarded, and rewound in realtime (watch little Mario run backwards!). Instead of bookmarks, save states are created and allow for quick browsing of the game's play history.

**Game Add-ons:** Standalone games, emulators and game streamers. From day one, RetroPlayer has been compatible with the [libretro](http://www.libretro.com/) ecosystem.

**Peripheral Add-ons:** Add-ons that expose hardware devices to Kodi. Communication with devices takes place over a bus. The peripheral add-on API is a virtual bus, alongside USB and PCI, that allows third parties to expose hardware devices to Kodi.

**Joystick input:** Various joystick APIs (DirectX, XInput, SDL, etc.) provide access to raw hardware events, like button presses and axis positions. The joystick input system maps these to physical elements on the controller, such as the X button, left trigger or right analog stick. The gesture recognition from touch input has been converted to monitor holding, double-pressing, analog stick rotation and (someday) accelerometer gestures.

**Media readers:** Plugging a cartridge into [Retrode](http://www.retrode.org/)-like devices can display game metadata and automatically launch the game. Removing a cartridge from the media reader can take a save-state so that the next time the game is inserted, gameplay begins from where it left off. Games can be cached indefinitely, so there is no need to insert the cartridge a second time (although it's possibly quicker than browsing for the cached game!). Game filenames aren't available, so game metadata is extracted from the ROM itself using [PyRomInfo](https://github.com/garbear/pyrominfo).

Many of these features are still works-in-progress, so fork the code and help out!

# Building Kodi and games

Build Kodi per usual. If you are developing binary add-ons using a local prefix, specifying it during the `./configure` step:

```
./bootstrap
./configure --prefix=$HOME/kodi
make -j8
```

Joystick support is now provided through a binary add-on. Follow the out-of-tree instructions at https://github.com/kodi-game/peripheral.joystick.

Game add-ons are hosted separately at https://github.com/kodi-game. If you would like to compile all game add-ons in one fell swoop, create a build directory out-of-tree and run the following commands:

```
cmake -DADDONS_TO_BUILD=game.* \
      -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_INSTALL_PREFIX=$HOME/workspace/kodi/addons \
      -DPACKAGE_ZIP=1 \
      $HOME/workspace/kodi/cmake/addons
make
```

where `$HOME/workspace/kodi` symlinks to the directory you cloned Kodi into.
