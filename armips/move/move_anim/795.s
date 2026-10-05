.nds
.thumb

.include "armips/include/animscriptcmd.s"

.create "build/move/move_anim/0_795", 0

// Obstruct: reuses the Detect animation (user tint)

a010_795:
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
    loadparticle 0, 217
    waitstate
    unloadspriteresource
    resetsprite 0
    resetsprite 1
    resetsprite 2
    resetsprite 3
    jumpifcontest _s0_038C
    jumpifside 0, _s0_0194, _s0_0258
    callfunction 33, 5, 0, 1, 0, 12, 0, "NaN", "NaN", "NaN", "NaN", "NaN"
    waitstate
    callfunction 34, 6, 2, 0, 1, 32767, 12, 0, "NaN", "NaN", "NaN", "NaN"
    wait 2
    playsepan 1840, -117
    wait 8
    addparticle 0, 0, 3
    addparticle 0, 1, 3
    playsepan 2020, -117
    waitparticle
    unloadparticle 0
    callfunction 33, 5, 0, 1, 12, 0, 0, "NaN", "NaN", "NaN", "NaN", "NaN"
    waitstate
    end
_s0_0194:
    callfunction 33, 5, 0, 1, 0, 12, 0, "NaN", "NaN", "NaN", "NaN", "NaN"
    waitstate
    callfunction 34, 6, 2, 0, 1, 8326, 12, 0, "NaN", "NaN", "NaN", "NaN"
    wait 2
    playsepan 1840, -117
    wait 8
    addparticle 0, 0, 3
    addparticle 0, 1, 3
    playsepan 2020, -117
    waitparticle
    unloadparticle 0
    callfunction 33, 5, 0, 1, 12, 0, 0, "NaN", "NaN", "NaN", "NaN", "NaN"
    waitstate
    end
_s0_0258:
    callfunction 33, 5, 0, 1, 0, 12, 0, "NaN", "NaN", "NaN", "NaN", "NaN"
    waitstate
    callfunction 34, 6, 2, 0, 1, 8326, 12, 0, "NaN", "NaN", "NaN", "NaN"
    wait 2
    playsepan 1840, -117
    wait 8
    addparticle 0, 0, 17
    cmd37 6, 0, 1, 5, 0, 0, 0, "NaN", "NaN"
    cmd37 4, 1, -6880, 0, 0, "NaN", "NaN", "NaN", "NaN"
    addparticle 0, 1, 17
    cmd37 6, 0, 1, 5, 0, 0, 0, "NaN", "NaN"
    cmd37 4, 1, -6880, 0, 0, "NaN", "NaN", "NaN", "NaN"
    playsepan 2020, -117
    waitparticle
    unloadparticle 0
    callfunction 33, 5, 0, 1, 12, 0, 0, "NaN", "NaN", "NaN", "NaN", "NaN"
    waitstate
    end
_s0_038C:
    callfunction 33, 5, 0, 1, 0, 12, 0, "NaN", "NaN", "NaN", "NaN", "NaN"
    waitstate
    callfunction 34, 6, 2, 0, 1, 8326, 12, 0, "NaN", "NaN", "NaN", "NaN"
    wait 2
    playsepan 1840, -117
    wait 8
    addparticle 0, 0, 17
    cmd37 6, 0, 1, 5, 0, 0, 0, "NaN", "NaN"
    cmd37 4, 1, -8256, 0, 0, "NaN", "NaN", "NaN", "NaN"
    addparticle 0, 1, 17
    cmd37 6, 0, 1, 5, 0, 0, 0, "NaN", "NaN"
    cmd37 4, 1, -8256, 0, 0, "NaN", "NaN", "NaN", "NaN"
    playsepan 2020, -117
    waitparticle
    unloadparticle 0
    callfunction 33, 5, 0, 1, 12, 0, 0, "NaN", "NaN", "NaN", "NaN", "NaN"
    waitstate
    end

.close
