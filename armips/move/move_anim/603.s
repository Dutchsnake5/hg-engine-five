.nds
.thumb

.include "armips/include/animscriptcmd.s"

.create "build/move/move_anim/0_603", 0

// Powder: reuses the Sleep Powder animation (target tint)

a010_603:
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
    loadparticle 0, 110
    waitstate
    unloadspriteresource
    resetsprite 0
    resetsprite 1
    resetsprite 2
    resetsprite 3
    addparticle 0, 0, 4
    repeatse 1960, 117, 4, 6
    callfunction 34, 6, 8, 0, 2, 11252, 10, 10, "NaN", "NaN", "NaN", "NaN"
    waitparticle
    unloadparticle 0
    shadetargetmon 31, 4, 4
    waitstate
    end

.close
