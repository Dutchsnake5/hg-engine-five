.nds
.thumb

.include "armips/include/animscriptcmd.s"

.create "build/move/move_anim/0_833", 0

// Stone Axe: reuses the Stone Edge animation (extra shake)

a010_833:
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
    loadparticle 0, 462
    waitstate
    unloadspriteresource
    resetsprite 0
    resetsprite 1
    resetsprite 2
    resetsprite 3
    addparticle 0, 2, 4
    addparticle 0, 3, 4
    addparticle 0, 1, 4
    addparticle 0, 0, 4
    repeatse 1972, 117, 2, 4
    wait 25
    shaketargetmon 3, 3
    callfunction 36, 5, 2, 0, 1, 6, 264, "NaN", "NaN", "NaN", "NaN", "NaN"
    playsepan 1965, 117
    repeatse 1972, 117, 6, 3
    waitparticle
    unloadparticle 0
    waitstate
    end

.close
