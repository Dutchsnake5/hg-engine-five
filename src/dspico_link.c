#include "../include/types.h"
#include "../include/config.h"

// DSPICO_DEBUG_LINK (include/config.h): a live debug link over the DSpico flashcart's USB port. The DSpico (LNH-team,
// an RP2040 that emulates the game card) lets the DS itself drive its USB port through card commands E8-EB
// (dspico-firmware docs/commands.md; the USB device stack runs on the DS, the RP2040 only passes data along). This file
// is a small USB device of its own: a CDC-ACM serial port (Windows binds its own driver, usbser, so it shows up as a
// COM port), polled once per game step from the main loop (GameSpeed_MainLoopWait). Over it the PC
// (Tools\dspico_link.py in the project folder) can read and write memory and call game functions, and the game can send
// log lines (DSpicoLink_Log, and debug_printf).
//
// The card bus is shared with the game's own file reads and Pico Loader's save writes, so every access first takes the
// game's card lock (CARD_LockRom) - but only when nobody holds it: the save code keeps the card resource for many
// frames, and waiting for it from the main loop would hang the game. When it's busy, this step is skipped.
//
// On anything that isn't a DSpico (emulators, other flashcarts) it switches itself off after one check: the card id
// must be the DSpico's (0x800000C2, or 0xC00000C2 in DSi mode) and its USB event queue must read back empty.
//
// It also needs the ROM as Tools\dspico.py puts it on the card: while USB events wait, the DSpico raises the card's
// interrupt line, which the game's ARM7 (NitroSDK CARD_IsCardIreqLo) takes for "Game Card removed" and stops the game.
// dspico.py patches that check out of the card's copy and marks the header (byte 0x1B = 0xD5, a reserved byte; the
// loader leaves the header at 0x027FFE00). Without the mark, USB is never switched on.

#ifdef DSPICO_DEBUG_LINK

#ifndef GAME_SPEED_OPTION
#error "DSPICO_DEBUG_LINK needs GAME_SPEED_OPTION (its main-loop hook polls the link)"
#endif

#define REG_MCCNT0_HI   (*(vu8 *)0x040001A1)
#define REG_MCCNT1      (*(vu32 *)0x040001A4)
#define REG_MCCMD       ((vu8 *)0x040001A8)
#define REG_MCD1        (*(vu32 *)0x04100010)

#define MC_BUSY         0x80000000
#define MC_DATA_READY   0x00800000
#define MC_WRITE        0x40000000
// settings as the DSpico's own USB code and Pico Loader use them (6.7 MHz, scrambler flags with zero seeds)
#define S_NODATA        0xA0406000 // no payload
#define S_READ4         0xA7446000 // 4 bytes back, latency 4
#define S_READ512       0xA1444000 // 512 bytes back, latency 4
#define S_WRITE512      0xE14C6000 // 512 bytes out, latency 8

// the game's card library (NitroSDK card_common.c): the resource lock's owner in cardi_common (0x021E3820) + 8,
// OS_LOCK_ID_ERROR (-3) = free
#define CARD_LOCK_OWNER (*(volatile s32 *)(0x021E3820 + 8))
#define LOCK_ID_ERROR   (-3)

void CARD_LockRom(u16 lockId);
void CARD_UnlockRom(u16 lockId);
s32 OS_GetLockID(void);

// USB event words (dspico-firmware usbEventQueue.h)
#define EV_LAST          0x80000000
#define EV_BUS_RESET     1
#define EV_UNPLUGGED     2

#define EP0_OUT          0x00
#define EP0_IN           0x80
#define EP_NOTIFY        0x81
#define EP_DATA_OUT      0x02
#define EP_DATA_IN       0x82

#define HEADER_MARK      (*(vu8 *)0x027FFE1B)
#define HEADER_MARK_OK   0xD5

#define LINK_VERSION     1
#define LOG_SIZE         1024
#define MAX_PAYLOAD      (512 - 4)

enum { ST_UNCHECKED, ST_OFF, ST_ON };
enum { CTL_IDLE, CTL_IN_DATA, CTL_OUT_DATA, CTL_SET_ADDRESS };

static u8 sState;
static u8 sCtl;
static u8 sAddress;
static u8 sConfigured;
static u8 sInBusy;
static u8 sCmdPending;  // a command arrived and waits for the IN endpoint
static s16 sLockId = -1;
static u16 sLogHead, sLogTail;
// buffers from heap 0 once a DSpico is found (the always-loaded code area has no room for them)
static u32 *sBuf;       // 512 bytes: card transfers
static u32 *sCmd;       // 512 bytes: the command being answered
static u8 *sLog;        // LOG_SIZE bytes
static u8 sLineCoding[7] = { 0x00, 0xC2, 0x01, 0x00, 0, 0, 8 }; // 115200 8N1 (meaningless on USB, but asked for)

static const u8 sDeviceDesc[18] = {
    18, 1, 0x00, 0x02, 0xEF, 0x02, 0x01, 64, 0xFE, 0xCA, 0x11, 0x40, 0x00, 0x01, 0, 1, 0, 1,
};

static const u8 sConfigDesc[75] = {
    9, 2, 75, 0, 2, 1, 0, 0x80, 50,
    8, 0x0B, 0, 2, 2, 2, 0, 0,          // interface association: CDC
    9, 4, 0, 0, 1, 2, 2, 0, 0,          // interface 0: communication
    5, 0x24, 0x00, 0x10, 0x01,          // header
    5, 0x24, 0x01, 0x00, 0x01,          // call management
    4, 0x24, 0x02, 0x02,                // ACM
    5, 0x24, 0x06, 0x00, 0x01,          // union
    7, 5, EP_NOTIFY, 3, 8, 0, 16,
    9, 4, 1, 0, 2, 0x0A, 0, 0, 0,       // interface 1: data
    7, 5, EP_DATA_OUT, 2, 64, 0, 0,
    7, 5, EP_DATA_IN, 2, 64, 0, 0,
};

static const char sProduct[] = "Timeless Jade debug link";

// ---- card bus ----------------------------------------------------------------------------------------------------

static void Card_Command(u32 hi, u32 lo, u32 settings, u32 *data, u32 words)
{
    u32 i;

    while (REG_MCCNT1 & MC_BUSY) {}
    REG_MCCNT0_HI = 0x80; // card enabled, ROM mode, no transfer interrupt
    for (i = 0; i < 4; i++) {
        REG_MCCMD[i] = hi >> (24 - 8 * i);
        REG_MCCMD[4 + i] = lo >> (24 - 8 * i);
    }
    REG_MCCNT1 = settings;
    i = 0;
    while (REG_MCCNT1 & MC_BUSY) {
        if (REG_MCCNT1 & MC_DATA_READY) {
            if (settings & MC_WRITE) {
                REG_MCD1 = i < words ? data[i] : 0;
            } else {
                u32 v = REG_MCD1;
                if (i < words) {
                    data[i] = v;
                }
            }
            i++;
        }
    }
}

static void Usb_Command(u32 sub, u32 arg, u32 lo)
{
    Card_Command(0xE8000000 | (sub << 16) | arg, lo, S_NODATA, NULL, 0);
}

static u32 Usb_GetEvent(void)
{
    u32 ev = 0;
    Card_Command(0xEB000000, 0, S_READ4, &ev, 1);
    return ev;
}

// sends `len` bytes from sBuf on an IN endpoint (one transfer, len <= 512; 0 = zero-length packet)
static void Usb_SendBuf(u32 ep, u32 len)
{
    u32 hi = 0xE9000000 | (ep << 8) | 1; // buffer 0, start the transfer
    if (len == 0) {
        Card_Command(hi, 0, S_NODATA, NULL, 0);
    } else {
        Card_Command(hi, len, S_WRITE512, sBuf, 128);
    }
}

static void Usb_Receive(u32 ep, u32 len)
{
    Usb_Command(0x10, ep << 8, len); // BEGIN_TRANSFER into buffer 0
}

static void Usb_ReadBuf(u32 ep)
{
    Card_Command(0xEA000000 | (ep << 8), 0, S_READ512, sBuf, 128);
}

static void Usb_Stall0(void)
{
    Usb_Command(0x09, EP0_IN << 8, 0);
    Usb_Command(0x09, EP0_OUT << 8, 0);
}

static void Ep0_Send(const u8 *src, u32 len, u32 max)
{
    u32 i;
    u8 *dst = (u8 *)sBuf;

    if (len > max) {
        len = max;
    }
    for (i = 0; i < len; i++) {
        dst[i] = src[i];
    }
    Usb_SendBuf(EP0_IN, len);
    sCtl = CTL_IN_DATA;
}

static void Ep0_Ack(void)
{
    Usb_SendBuf(EP0_IN, 0);
}

// ---- USB requests ----------------------------------------------------------------------------------------------

static void Usb_Setup(u32 w0, u32 w1)
{
    u32 type = (w1 >> 29) & 3; // bmRequestType bits 5-6
    u32 request = (w1 >> 16) & 0xFF;
    u32 value = w1 & 0xFFFF;
    u32 length = (w0 >> 16) & 0x1FFF;
    u8 tmp[2 + 2 * (sizeof(sProduct) - 1)];
    u32 i;

    sCtl = CTL_IDLE;
    if (type == 0) {
        switch (request) {
        case 6: // GET_DESCRIPTOR
            switch (value >> 8) {
            case 1:
                Ep0_Send(sDeviceDesc, sizeof(sDeviceDesc), length);
                return;
            case 2:
                Ep0_Send(sConfigDesc, sizeof(sConfigDesc), length);
                return;
            case 3:
                tmp[1] = 3;
                if ((value & 0xFF) == 0) {
                    tmp[0] = 4;
                    tmp[2] = 0x09;
                    tmp[3] = 0x04;
                } else if ((value & 0xFF) == 1) {
                    tmp[0] = sizeof(tmp);
                    for (i = 0; i < sizeof(sProduct) - 1; i++) {
                        tmp[2 + 2 * i] = sProduct[i];
                        tmp[3 + 2 * i] = 0;
                    }
                } else {
                    break;
                }
                Ep0_Send(tmp, tmp[0], length);
                return;
            }
            break;
        case 5: // SET_ADDRESS: the firmware sends the status stage itself
            sAddress = value;
            Usb_Command(0x02, 0, 0);
            sCtl = CTL_SET_ADDRESS;
            return;
        case 9: // SET_CONFIGURATION
            if (value == 1 && !sConfigured) {
                Usb_Command(0x0D, (EP_NOTIFY << 8) | 3, 8);
                Usb_Command(0x0D, (EP_DATA_OUT << 8) | 2, 64);
                Usb_Command(0x0D, (EP_DATA_IN << 8) | 2, 64);
                Usb_Receive(EP_DATA_OUT, 512);
                sInBusy = FALSE;
                sCmdPending = FALSE;
            }
            sConfigured = value == 1;
            Ep0_Ack();
            return;
        case 8: // GET_CONFIGURATION
            tmp[0] = sConfigured;
            Ep0_Send(tmp, 1, length);
            return;
        case 0: // GET_STATUS
            tmp[0] = tmp[1] = 0;
            Ep0_Send(tmp, 2, length);
            return;
        case 1: // CLEAR_FEATURE
            if ((w1 >> 24 & 0x1F) == 2) {
                Usb_Command(0x0A, (w0 & 0xFF) << 8, 0);
            }
            // fall through
        case 3:    // SET_FEATURE
        case 0x0B: // SET_INTERFACE
            Ep0_Ack();
            return;
        }
    } else if (type == 1) {
        switch (request) {
        case 0x20: // SET_LINE_CODING: 7 bytes follow
            Usb_Receive(EP0_OUT, length);
            sCtl = CTL_OUT_DATA;
            return;
        case 0x21: // GET_LINE_CODING
            Ep0_Send(sLineCoding, sizeof(sLineCoding), length);
            return;
        case 0x22: // SET_CONTROL_LINE_STATE
        case 0x23: // SEND_BREAK
            Ep0_Ack();
            return;
        }
    }
    Usb_Stall0();
}

// ---- the debug protocol ------------------------------------------------------------------------------------------
// PC -> DS, one packet per USB transfer: u8 command, u8 sequence, u16 length, u32 address, then data
//   'P' ping -> 'p' (u32 version)
//   'R' read `length` bytes at address -> 'r' (the bytes)
//   'W' write the data at address -> 'w'
//   'C' call the function at address (Thumb: | 1) with the 4 u32 arguments in the data -> 'c' (u32 result)
// DS -> PC: u8 type, u8 sequence, u16 length, then data; type 'L' = log text

void DSpicoLink_Log(const char *text);

typedef u32 (*DebugCall)(u32, u32, u32, u32);

static void Link_Reply(u32 type, u32 seq, u32 len)
{
    sBuf[0] = type | (seq << 8) | (len << 16);
    Usb_SendBuf(EP_DATA_IN, 4 + len);
    sInBusy = TRUE;
}

static void Link_HandleCommand(void)
{
    u8 *cmd = (u8 *)sCmd;
    u32 seq = cmd[1];
    u32 len = cmd[2] | (cmd[3] << 8);
    u8 *addr = (u8 *)sCmd[1];
    u8 *out = (u8 *)&sBuf[1];
    u32 i;

    switch (cmd[0]) {
    case 'P':
        sBuf[1] = LINK_VERSION;
        Link_Reply('p', seq, 4);
        break;
    case 'R':
        if (len > MAX_PAYLOAD) {
            len = MAX_PAYLOAD;
        }
        for (i = 0; i < len; i++) {
            out[i] = addr[i];
        }
        Link_Reply('r', seq, len);
        break;
    case 'W':
        if (len > 512 - 8) {
            len = 512 - 8;
        }
        for (i = 0; i < len; i++) {
            addr[i] = cmd[8 + i];
        }
        Link_Reply('w', seq, 0);
        break;
    case 'C':
        sBuf[1] = ((DebugCall)addr)(sCmd[2], sCmd[3], sCmd[4], sCmd[5]);
        Link_Reply('c', seq, 4);
        break;
    default:
        Link_Reply('?', seq, 0);
        break;
    }
}

static void Link_SendLog(void)
{
    u8 *out = (u8 *)&sBuf[1];
    u32 n = 0;

    while (sLogTail != sLogHead && n < MAX_PAYLOAD) {
        out[n++] = sLog[sLogTail];
        sLogTail = (sLogTail + 1) % LOG_SIZE;
    }
    if (n) {
        Link_Reply('L', 0, n);
    }
}

/**
 * @brief adds text to the log sent to the PC (kept in a 1 KB ring until the PC reads it; the oldest text is dropped
 *        when it's full). Safe to call from anywhere; does nothing until the link is up (and without a DSpico)
 */
void DSpicoLink_Log(const char *text)
{
    u32 irq;

    if (sState != ST_ON) {
        return;
    }
    irq = OS_DisableInterrupts();
    while (*text) {
        u16 next = (sLogHead + 1) % LOG_SIZE;
        if (next == sLogTail) {
            sLogTail = (sLogTail + 1) % LOG_SIZE;
        }
        sLog[sLogHead] = *text++;
        sLogHead = next;
    }
    OS_RestoreInterrupts(irq);
}

// ---- polling -----------------------------------------------------------------------------------------------------

static BOOL Link_Lock(void)
{
    u32 irq;
    BOOL got = FALSE;

    if (sLockId < 0) {
        sLockId = OS_GetLockID();
        if (sLockId < 0) {
            return FALSE;
        }
    }
    irq = OS_DisableInterrupts();
    if (CARD_LOCK_OWNER == LOCK_ID_ERROR) {
        CARD_LockRom(sLockId);
        got = TRUE;
    }
    OS_RestoreInterrupts(irq);
    return got;
}

static void Link_Start(void)
{
    u32 id = 0;

    if (HEADER_MARK != HEADER_MARK_OK) {
        sState = ST_OFF;
        return;
    }
    Card_Command(0xB8000000, 0, S_READ4, &id, 1);
    if ((id & ~0x40000000) == 0x800000C2) {
        Usb_Command(0x0E, 0, 0); // clear the event queue
        if (Usb_GetEvent() == 0) {
            sBuf = sys_AllocMemoryLo(0, 1024 + LOG_SIZE);
            if (sBuf == NULL) {
                sState = ST_OFF;
                return;
            }
            sCmd = sBuf + 128;
            sLog = (u8 *)(sBuf + 256);
            Usb_Command(0x0F, 0, 0); // deinit (a soft reset leaves it running)
            Usb_Command(0x01, 0, 0); // init
            Usb_Command(0x11, 0, 0); // interrupt enable (the firmware queues events from its USB interrupt)
            Usb_Command(0x04, 0, 0); // connect
            sState = ST_ON;
            DSpicoLink_Log("[Timeless Jade] debug link up\n");
            return;
        }
    }
    sState = ST_OFF;
}

static void Link_Event(u32 ev)
{
    u32 ep, bytes, n;

    if ((ev >> 30) == 1) { // SETUP: a second word follows
        Usb_Setup(ev, Usb_GetEvent() & ~EV_LAST);
    } else if ((ev >> 28) == 3) { // transfer complete
        ep = ev & 0xFF;
        bytes = (ev >> 8) & 0x1FFF;
        if (ep == EP0_IN) {
            if (sCtl == CTL_IN_DATA) {
                Usb_Receive(EP0_OUT, 0); // status stage
            } else if (sCtl == CTL_SET_ADDRESS) {
                Usb_Command(0x0C, sAddress << 8, 0);
            }
            sCtl = CTL_IDLE;
        } else if (ep == EP0_OUT) {
            if (sCtl == CTL_OUT_DATA) {
                Usb_ReadBuf(EP0_OUT);
                for (n = 0; n < sizeof(sLineCoding) && n < bytes; n++) {
                    sLineCoding[n] = ((u8 *)sBuf)[n];
                }
                Ep0_Ack();
            }
            sCtl = CTL_IDLE;
        } else if (ep == EP_DATA_IN) {
            sInBusy = FALSE;
        } else if (ep == EP_DATA_OUT) {
            Usb_ReadBuf(EP_DATA_OUT);
            for (n = 0; n < 128; n++) {
                sCmd[n] = sBuf[n];
            }
            sCmdPending = bytes >= 8;
            if (!sCmdPending) {
                Usb_Receive(EP_DATA_OUT, 512);
            }
        }
    } else if (ev == EV_BUS_RESET || ev == EV_UNPLUGGED) {
        sConfigured = FALSE;
        sInBusy = FALSE;
        sCmdPending = FALSE;
        sCtl = CTL_IDLE;
    }
}

/**
 * @brief called once per game step from the main loop (GameSpeed_MainLoopWait)
 */
void DSpicoLink_Poll(void)
{
    u32 ev;
    u32 n;

    if (sState == ST_OFF || !Link_Lock()) {
        return;
    }
    if (sState == ST_UNCHECKED) {
        Link_Start();
    }
    if (sState == ST_ON) {
        for (n = 0; n < 32; n++) {
            ev = Usb_GetEvent();
            if ((ev & ~EV_LAST) == 0) {
                break;
            }
            Link_Event(ev & ~EV_LAST);
        }
        if (sConfigured && !sInBusy) {
            if (sCmdPending) {
                sCmdPending = FALSE;
                Link_HandleCommand();
                Usb_Receive(EP_DATA_OUT, 512);
            } else {
                Link_SendLog();
            }
        }
    }
    CARD_UnlockRom(sLockId);
}

#endif // DSPICO_DEBUG_LINK
