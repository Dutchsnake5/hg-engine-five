.nds
.thumb

.include "armips/include/animscriptcmd.s"

.create "build/move/move_anim/0_606", 0

// Happy Hour: reuses the Charm animation (screen flash)

a010_606:
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
    loadparticle 0, 223
    waitstate
    unloadspriteresource
    resetsprite 0
    resetsprite 1
    resetsprite 2
    resetsprite 3
    addparticle 0, 0, 3
    callfunction 25, 0, "NaN", "NaN", "NaN", "NaN", "NaN", "NaN", "NaN", "NaN", "NaN", "NaN"
    loop 3
    playsepan 2025, -117
    wait 8
    doloop
    playsepan 2025, -117
    waitparticle
    unloadparticle 0
    waitstate
    flashscreencolor 31, 28, 4
    waitstate
    end

.close
