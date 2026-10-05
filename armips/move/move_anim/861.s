.nds
.thumb

.include "armips/include/animscriptcmd.s"

.create "build/move/move_anim/0_861", 0

// Spicy Extract: reuses the Sweet Scent animation (target tint)

a010_861:
    jumpifside 0, _s0_0014, _s0_0190
    shadetargetmon 31, 4, 4
    waitstate
    end
_s0_0014:
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
    loadparticle 0, 248
    waitstate
    unloadspriteresource
    resetsprite 0
    resetsprite 1
    resetsprite 2
    resetsprite 3
    playsepan 2037, 0
    addparticle 0, 0, 17
    cmd37 6, 0, 2, 3, 0, 0, 0, "NaN", "NaN"
    cmd37 4, 1, -2000, 8000, 0, "NaN", "NaN", "NaN", "NaN"
    addparticle 0, 2, 0
    addparticle 0, 4, 4
    callfunction 34, 5, 8, 0, 2, 23199, 12, "NaN", "NaN", "NaN", "NaN", "NaN"
    callfunction 34, 5, 16, 0, 2, 23199, 12, "NaN", "NaN", "NaN", "NaN", "NaN"
    waitparticle
    unloadparticle 0
    shadetargetmon 31, 4, 4
    waitstate
    end
_s0_0190:
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
    loadparticle 0, 248
    waitstate
    unloadspriteresource
    resetsprite 0
    resetsprite 1
    resetsprite 2
    resetsprite 3
    playsepan 2037, 0
    addparticle 0, 1, 17
    cmd37 6, 0, 2, 3, 0, 0, 0, "NaN", "NaN"
    cmd37 4, 1, -2000, 8000, 0, "NaN", "NaN", "NaN", "NaN"
    addparticle 0, 3, 0
    addparticle 0, 4, 4
    callfunction 34, 5, 8, 0, 2, 23199, 12, "NaN", "NaN", "NaN", "NaN", "NaN"
    callfunction 34, 5, 16, 0, 2, 23199, 12, "NaN", "NaN", "NaN", "NaN", "NaN"
    waitparticle
    unloadparticle 0
    shadetargetmon 31, 4, 4
    waitstate
    end

.close
