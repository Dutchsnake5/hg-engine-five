.nds
.thumb

.include "armips/include/animscriptcmd.s"

.create "build/move/move_anim/0_678", 0

// Throat Chop: reuses the Karate Chop animation (target tint)

a010_678:
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
    loadparticle 0, 33
    waitstate
    unloadspriteresource
    resetsprite 0
    resetsprite 1
    resetsprite 2
    resetsprite 3
    addparticle 0, 2, 4
    addparticle 0, 0, 4
    addparticle 0, 1, 4
    callfunction 36, 5, 1, 0, 1, 2, 264, "NaN", "NaN", "NaN", "NaN", "NaN"
    playsepan 2119, 117
    waitse 1920, 117, 5
    waitparticle
    unloadparticle 0
    shadetargetmon 6, 4, 8
    waitstate
    end

.close
