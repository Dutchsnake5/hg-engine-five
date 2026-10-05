.nds
.thumb

.include "armips/include/animscriptcmd.s"

.create "build/move/move_anim/0_548", 0

// Searing Shot: reuses the Lava Plume animation (target tint)

a010_548:
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
    loadparticle 0, 454
    waitstate
    unloadspriteresource
    resetsprite 0
    resetsprite 1
    resetsprite 2
    resetsprite 3
    playsepan 2096, -117
    addparticle 0, 1, 3
    addparticle 0, 2, 3
    addparticle 0, 0, 3
    callfunction 36, 5, 2, 0, 1, 2, 288, "NaN", "NaN", "NaN", "NaN", "NaN"
    wait 1
    jumpifplayerattack _s0_01A0
    callfunction 34, 6, 8, 0, 1, 31, 10, 0, "NaN", "NaN", "NaN", "NaN"
    callfunction 34, 6, 16, 0, 1, 31, 10, 0, "NaN", "NaN", "NaN", "NaN"
    callfunction 34, 6, 4, 0, 1, 31, 10, 0, "NaN", "NaN", "NaN", "NaN"
    waitparticle
    unloadparticle 0
    shadetargetmon 31, 4, 4
    waitstate
    end
_s0_01A0:
    jumpifside 0, _s0_01B4, _s0_0234
    shadetargetmon 31, 4, 4
    waitstate
    end
_s0_01B4:
    callfunction 34, 6, 8, 0, 1, 31, 10, 10, "NaN", "NaN", "NaN", "NaN"
    callfunction 34, 6, 2056, 0, 1, 31, 10, 10, "NaN", "NaN", "NaN", "NaN"
    callfunction 34, 6, 2064, 0, 1, 31, 10, 10, "NaN", "NaN", "NaN", "NaN"
    waitstate
    waitparticle
    unloadparticle 0
    shadetargetmon 31, 4, 4
    waitstate
    end
_s0_0234:
    callfunction 34, 6, 8, 0, 1, 31, 10, 10, "NaN", "NaN", "NaN", "NaN"
    callfunction 34, 6, 2050, 0, 1, 31, 10, 10, "NaN", "NaN", "NaN", "NaN"
    callfunction 34, 6, 2052, 0, 1, 31, 10, 10, "NaN", "NaN", "NaN", "NaN"
    waitstate
    waitparticle
    unloadparticle 0
    shadetargetmon 31, 4, 4
    waitstate
    end
    shadetargetmon 31, 4, 4
    waitstate
    end
    shadetargetmon 31, 4, 4
    waitstate
    end

.close
