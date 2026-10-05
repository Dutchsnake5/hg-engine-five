.nds
.thumb

.include "armips/include/animscriptcmd.s"

.create "build/move/move_anim/0_516", 0

// Reflect Type: reuses the Conversion 2 animation (user tint)

a010_516:
    initspriteresource
    loadspriteresource 0
    loadspriteresource 1
    loadspriteresource 2
    loadspriteresource 3
    loadspritemaybe 4, 0, 0, 0
    loadspritemaybe 5, 0, 1, 1
    loadspritemaybe 6, 0, 2, 2
    loadspritemaybe 7, 0, 3, 3
    callfunction 78, 1, 0, "NaN", "NaN", "NaN", "NaN", "NaN", "NaN", "NaN", "NaN", "NaN"
    loadparticle 0, 196
    waitstate
    unloadspriteresource
    resetsprite 0
    resetsprite 1
    resetsprite 2
    resetsprite 3
    addparticle 0, 0, 4
    addparticle 0, 1, 17
    cmd37 6, 0, 1, 1, 1, 0, 0, "NaN", "NaN"
    repeatse 1987, 117, 5, 4
    addparticle 0, 2, 3
    waitse 1983, 117, 45
    waitse 1899, -117, 65
    shadeattackingmon 14, 22, 31
    waitparticle
    unloadparticle 0
    waitstate
    end

.close
