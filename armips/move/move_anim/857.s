.nds
.thumb

.include "armips/include/animscriptcmd.s"

.create "build/move/move_anim/0_857", 0

// Last Respects: reuses the Ominous Wind animation (target tint)

a010_857:
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
    loadparticle 0, 484
    waitstate
    unloadspriteresource
    resetsprite 0
    resetsprite 1
    resetsprite 2
    resetsprite 3
    playsepan 2107, 117
    jumpifside 0, _s0_00D8, _s0_015C
    end
_s0_00D8:
    addparticle 0, 0, 4
    shadetargetmon 20, 6, 26
    callfunction 36, 5, 3, 0, 1, 3, 264, "NaN", "NaN", "NaN", "NaN", "NaN"
    waitstate
    callfunction 52, 3, 4, -20, 264, "NaN", "NaN", "NaN", "NaN", "NaN", "NaN", "NaN"
    waitstate
    wait 35
    callfunction 52, 3, 5, 20, 264, "NaN", "NaN", "NaN", "NaN", "NaN", "NaN", "NaN"
    waitstate
    waitparticle
    unloadparticle 0
    waitstate
    end
_s0_015C:
    addparticle 0, 1, 4
    shadetargetmon 20, 6, 26
    callfunction 36, 5, 3, 0, 1, 3, 264, "NaN", "NaN", "NaN", "NaN", "NaN"
    waitstate
    callfunction 52, 3, 4, -20, 264, "NaN", "NaN", "NaN", "NaN", "NaN", "NaN", "NaN"
    waitstate
    wait 35
    callfunction 52, 3, 5, 20, 264, "NaN", "NaN", "NaN", "NaN", "NaN", "NaN", "NaN"
    waitstate
    waitparticle
    unloadparticle 0
    waitstate
    end

.close
