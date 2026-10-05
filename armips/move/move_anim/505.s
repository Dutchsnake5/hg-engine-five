.nds
.thumb

.include "armips/include/animscriptcmd.s"

.create "build/move/move_anim/0_505", 0

// Ally Switch: reuses the Agility animation (screen flash)

a010_505:
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
    loadparticle 0, 127
    waitstate
    unloadspriteresource
    resetsprite 0
    resetsprite 1
    resetsprite 2
    resetsprite 3
    jumpifside 0, _s0_00CC, _s0_0208
    end
_s0_00CC:
    initspriteresource
    loadspriteresource 0
    loadspriteresource 1
    loadspritemaybe 0, 0, 0, 0
    loadspritemaybe 0, 0, 1, 1
    loadspriteresource 4
    loadspritemaybe 2, 0, 4, 4
    cmd52 2, 0, 4
    cmd0C 0, -32
    cmd0C 1, 0
    changebg 36, 0x20001
    playsepan 1373, 0
    addparticle 0, 0, 0
    waitforchangebg
    wait 19
    repeatse 1983, -117, 4, 2
    wait 1
    callfunction 8, 0, "NaN", "NaN", "NaN", "NaN", "NaN", "NaN", "NaN", "NaN", "NaN", "NaN"
    waitstate
    resetsprite 0
    resetsprite 1
    unloadspriteresource
    cmd53 0
    resetsprite 4
    cmd0C 0, -64
    cmd0C 1, 0
    resetbg 36, 0x40001
    waitforchangebg
    flashscreencolor 31, 14, 24
    waitparticle
    unloadparticle 0
    waitstate
    end
_s0_0208:
    initspriteresource
    loadspriteresource 0
    loadspriteresource 1
    loadspritemaybe 0, 0, 0, 0
    loadspritemaybe 0, 0, 1, 1
    loadspriteresource 4
    loadspritemaybe 2, 0, 4, 4
    cmd52 2, 0, 4
    cmd0C 0, -64
    cmd0C 1, 0
    changebg 47, 0x20001
    playsepan 1373, 0
    addparticle 0, 1, 0
    waitforchangebg
    wait 19
    repeatse 1983, -117, 4, 2
    wait 1
    callfunction 8, 0, "NaN", "NaN", "NaN", "NaN", "NaN", "NaN", "NaN", "NaN", "NaN", "NaN"
    waitstate
    resetsprite 0
    resetsprite 1
    unloadspriteresource
    cmd53 0
    resetsprite 4
    cmd0C 0, -64
    cmd0C 1, 0
    resetbg 36, 0x40001
    waitforchangebg
    flashscreencolor 31, 14, 24
    waitparticle
    unloadparticle 0
    waitstate
    end

.close
