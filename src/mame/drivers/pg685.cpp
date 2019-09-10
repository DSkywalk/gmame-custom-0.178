// license:BSD-3-Clause
// copyright-holders: rfka01
/***************************************************************************

    Siemens Simatic PG-685

    driver skeleton by rfka01

****************************************************************************

The Simatic PG-685 is a programming device for Siemens' S5 line of industrial
controllers. They ran PCP/M-86 and MS-DOS specially adapted for the architecture.

http://oldcomputer.info/portables/pg685/index.htm

The portable case contains a monochrome monitor (with a socket provided to
drive an external monitor), a 5,25" floppy drive with 720KB capacity (DS,80
tracks, 9 sectors p.t., 512 Byters p.s.) and a MFM hard disk drive.
The PC is made up of several boards on a non-ISA bus backplane.

There are at least two versions and several options. The PG-685's settings are
contained in NVRAM, and have to be updated using a testdisk if the two AA
batteries run out.

For this, a key switch with a reset setting plays a crucial role. Set the key
to reset, insert disk in drive but don't close. Switch on machine, close drive
and set the switch to normal operation to start the setup.

Backplane: SCN2661B, D8253C-2, SAB 8259AP


6ES5685-OUA11

John Elliott's kindly analyzed the ROM of this machine, his findings are represented
in this preliminary memory map. Olivier Galibert remarked that for a 16 bit memory
map, the address ranges have to begin on even and end on odd addresses.

static ADDRESS_MAP_START(pg685_mem, AS_PROGRAM, 16, pg685_state)
    ADDRESS_MAP_UNMAP_HIGH
    AM_RANGE(0x00000,0xbffff) AM_RAM
    AM_RANGE(0xf0000,0xf1fff) AM_RAM
    AM_RANGE(0xf9f00,0xf9f00) // Keyboard scancode
    AM_RANGE(0xf9f01,0xf9f01) // Keyboard status (read) Keyboard command (write)
    AM_RANGE(0xf9f02,0xf9f02) // 6845 Register select
    AM_RANGE(0xf9f03,0xf9f03) // 6845 Register value
    AM_RANGE(0xf9f04,0xf9f04) // PCP/M-86 keyboard handling code also checks a couple of bits read
    AM_RANGE(0xf9f20,0xf9f20) // WD 279x floppy controller
    AM_RANGE(0xf9f30,0xf9f30) // Printer data
    AM_RANGE(0xf9f31,0xf9f31) // Printer status read
    AM_RANGE(0xf9f33,0xf9f33) // Printer present?
    AM_RANGE(0xf9f42,0xf9f47) // RTC registers
    AM_RANGE(0xf9f48,0xf9f4c) // NVRAM
    AM_RANGE(0xf9f50,0xf9f50) // RTC Busy flag
    AM_RANGE(0xf9f70,0xf9f77) // WD 1010
    AM_RANGE(0xf9f78,0xf9f78) // WD 1010 separate drive/head select register
    AM_RANGE(0xf9f79,0xf9f79) // another write-only register (possibly reset or interrupt control)
    AM_RANGE(0xfa000,0xfa7ff) AM_RAM AM_SHARE ("charcopy")
    AM_RANGE(0xfb000,0xfb7ff) AM_RAM AM_SHARE ("framebuffer")
    AM_RANGE(0xfc000,0xfffff) AM_ROM AM_REGION("bios", 0)
ADDRESS_MAP_END

This machine only has a textmode screen, Tandon TM262 hard disk drive on a WD1010 controller,
Teac FD-55FV-13-U floppy drive on a Siemens (WD)-1797-02P controller, 768KB of RAM, HD68A45SP
display controller, upd8279c-25 keyboard controller.
Ports: Printer, V24, Module, AG-S5, Sinec H1, External Monitor

CPU/Video:      16KB BIOS/CHAR EPROM, NEC V20 CPU, SAB 8259AP, 12.288 MHz crystal, 2xHM6116LP-3,
                HD46505SP-1 (HD68A45SP), D8279C-2, D8251AFC
Module/Floppy:  2xP8255A, 4xHM6116LP-3, D8251AFC, 4.000000 MHz crystal, SAB 1797-02P, MM58167AN
HD:             4xD4016C, WD1010A-AL, 10,000000 MHz crystal
Memory:         27xTMS27C256-15


6ES5685-OUA12

This machine has the BMG (bit mapped graphics) option, that John Elliott described as a memory mapped
hercules card. There is a GEM/3 display driver that was indeed derived from the Hercules one.
The screen buffer starts at E000, the video card is at F9F0:80h, the beeper frequency at F9F0:36h,
the serial port at F9F0:38h.

Graphics screen, MiniScribe 8425 hard disk drive on a WD2010B-AL controller, Teac FD-55FR 511-U floppy drive
on a Siemens (WD)-1797-02P controller, 896KB of RAM, HD68A45SP display controller, upd8279c-25
keyboard controller
Ports: Printer, V24, Module, AG-S5, Sinec H1, External Monitor, E1

CPU/Mem.:       iR80286-10 CPU, N82C288, 19,660800 MHz crystal, 2x16KB EPROM (BIOS/CHAR), 24MHz crystal
                18.189 MHz crystal, D71059L, HD46505SP-1 (HD68A45SP), D8279C-2, N8251A, 2xSRM20256LM,
                RAM daughterbd:    4x514256-10
Module/Floppy:  2xi8255A, 4xHM6116LP-3, D8251AFC, 4.000000 MHz crystal, SAB 1797-02P, MM58167AN
HD:             SRM2064C-15, WD2010B-AL, 10,000000 MHz crystal

****************************************************************************/

#include "emu.h"
#include "cpu/nec/nec.h"
//#include "cpu/i86/i86.h"
#include "cpu/i86/i286.h"
//#include "video/mc6845.h"

//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

class pg685_state : public driver_device
{
public:
	pg685_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, m_maincpu(*this, "maincpu") { }

	UINT32 screen_update_pg685(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
private:
	virtual void machine_reset() override;
	virtual void video_start() override;
	required_device<cpu_device> m_maincpu;
};

//**************************************************************************
//  ADDRESS MAPS
//**************************************************************************

static ADDRESS_MAP_START(pg685_mem, AS_PROGRAM, 8, pg685_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000,0xbffff) AM_RAM
	AM_RANGE(0xf0000,0xf1fff) AM_RAM
	AM_RANGE(0xfa000,0xfa7ff) AM_RAM AM_SHARE ("charcopy")
	AM_RANGE(0xfb000,0xfb7ff) AM_RAM AM_SHARE ("framebuffer")
	AM_RANGE(0xfc000,0xfffff) AM_ROM AM_REGION("bios", 0)
ADDRESS_MAP_END

static ADDRESS_MAP_START(pg685oua12_mem, AS_PROGRAM, 16, pg685_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000,0xdffff) AM_RAM
	AM_RANGE(0xe0000,0xeffff) AM_RAM AM_SHARE ("framebuffer")
	AM_RANGE(0xf0000,0xf1fff) AM_RAM
	AM_RANGE(0xfa000,0xfa7ff) AM_RAM AM_SHARE ("charcopy")
	AM_RANGE(0xfc000,0xfffff) AM_ROM AM_REGION("bios", 0)
ADDRESS_MAP_END


//**************************************************************************
//  I/O
//*************************************************************************

static INPUT_PORTS_START( pg685 )
INPUT_PORTS_END

//**************************************************************************
//  FLOPPY
//**************************************************************************

//**************************************************************************
//  HARDDISK
//**************************************************************************

//**************************************************************************
//  MACHINE EMULATION
//**************************************************************************

void pg685_state::machine_reset()
{
}

void pg685_state::video_start()
{
}



UINT32 pg685_state::screen_update_pg685(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	return 0;
}

//**************************************************************************
//  MACHINE DRIVERS
//**************************************************************************

static MACHINE_CONFIG_START( pg685, pg685_state )
	// main cpu
	MCFG_CPU_ADD("maincpu", V20, XTAL_15MHz / 3)
	MCFG_CPU_PROGRAM_MAP(pg685_mem)

	// i/o cpu

	// ram

	// video hardware

	// sound hardware

	// devices

	// rs232 port

	// keyboard

	// printer

	// floppy

	// harddisk
MACHINE_CONFIG_END

static MACHINE_CONFIG_START( pg685oua12, pg685_state )
	// main cpu
	MCFG_CPU_ADD("maincpu", I80286, XTAL_20MHz / 2)
	MCFG_CPU_PROGRAM_MAP(pg685oua12_mem)

	// i/o cpu

	// ram

	// video hardware

	// sound hardware

	// devices

	// rs232 port

	// keyboard

	// printer

	// floppy

	// harddisk

MACHINE_CONFIG_END


//**************************************************************************
//  ROM DEFINITIONS
//**************************************************************************

ROM_START( pg685 )
	ROM_REGION( 0x4000, "bios", ROMREGION_ERASEFF )
	ROM_LOAD( "pg685_oua11_s79200-g2_a901-03.bin", 0x0000, 0x4000, CRC(db13f2db) SHA1(5f65ab14d9c8acdcc5482b27e727ca43b1a7daf3) )
ROM_END

ROM_START( pg685oua12 )
	ROM_REGION( 0x4000, "bios", ROMREGION_ERASEFF )
	ROM_LOAD( "pg685_oua12_bios.bin", 0x0000, 0x4000, CRC(94b8499b) SHA1(e29086a88f1f9fa17921c3d157cce725d4591328))

	ROM_REGION( 0x4000, "chargen", 0 )
	ROM_LOAD( "pg685_oua12_s79200-g39_a901-01.bin", 0x0000, 0x4000, CRC(fa722110) SHA1(b57ee67a77ff45a2544a2ae5203bc2199adfe023))
ROM_END

//**************************************************************************
//  ROM DEFINITIONS
//**************************************************************************
/*    YEAR  NAME        PARENT    COMPAT  MACHINE     INPUT       CLASS          INIT        COMPANY FULLNAME                  FLAGS                */
COMP( 198?, pg685,      0,        0,      pg685,      pg685,      driver_device,    0,       "Siemens", "Simatic PG685 OUA11", MACHINE_NOT_WORKING | MACHINE_NO_SOUND)
COMP( 198?, pg685oua12, pg685,    0,      pg685oua12, pg685,      driver_device,    0,       "Siemens", "Simatic PG685 OUA12", MACHINE_NOT_WORKING | MACHINE_NO_SOUND)
