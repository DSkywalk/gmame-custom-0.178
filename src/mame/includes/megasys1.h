// license:BSD-3-Clause
// copyright-holders:Luca Elia
/***************************************************************************

                            -= Jaleco Mega System 1 =-

                    driver by   Luca Elia (l.elia@tin.it)

***************************************************************************/

#include "machine/gen_latch.h"
#include "sound/okim6295.h"
#include "video/ms1_tmap.h"


class megasys1_state : public driver_device
{
public:
	megasys1_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		m_objectram(*this, "objectram"),
		m_tmap(*this, "scroll%u", 0),
		m_ram(*this, "ram"),
		m_maincpu(*this, "maincpu"),
		m_audiocpu(*this, "audiocpu"),
		m_oki1(*this, "oki1"),
		m_oki2(*this, "oki2"),
		m_gfxdecode(*this, "gfxdecode"),
		m_palette(*this, "palette"),
		m_screen(*this, "screen"),
		m_soundlatch(*this, "soundlatch"),
		m_soundlatch2(*this, "soundlatch2"),
		m_soundlatch_z(*this, "soundlatch_z"),
		m_rom_maincpu(*this, "maincpu"),
		m_io_system(*this, "SYSTEM"),
		m_io_p1(*this, "P1"),
		m_io_p2(*this, "P2"),
		m_io_dsw(*this, "DSW"),
		m_io_dsw1(*this, "DSW1"),
		m_io_dsw2(*this, "DSW2")
		{ }

	required_shared_ptr<UINT16> m_objectram;
	optional_device_array<megasys1_tilemap_device, 3> m_tmap;
	required_shared_ptr<UINT16> m_ram;
	required_device<cpu_device> m_maincpu;
	optional_device<cpu_device> m_audiocpu;
	optional_device<okim6295_device> m_oki1;
	optional_device<okim6295_device> m_oki2;
	required_device<gfxdecode_device> m_gfxdecode;
	required_device<palette_device> m_palette;
	required_device<screen_device> m_screen;
	optional_device<generic_latch_16_device> m_soundlatch;
	optional_device<generic_latch_16_device> m_soundlatch2;
	optional_device<generic_latch_8_device> m_soundlatch_z;
	required_region_ptr<UINT16> m_rom_maincpu;
	required_ioport m_io_system;
	required_ioport m_io_p1;
	required_ioport m_io_p2;
	optional_ioport m_io_dsw;
	optional_ioport m_io_dsw1;
	optional_ioport m_io_dsw2;

	bitmap_ind16 m_sprite_buffer_bitmap;

	UINT16 *m_spriteram;
	UINT16 m_ip_select_values[7];
	UINT16 m_ip_latched;
	UINT8 m_ignore_oki_status;
	UINT16 m_protection_val;
	UINT16 m_active_layers;
	UINT16 m_sprite_bank;
	UINT16 m_screen_flag;
	UINT16 m_sprite_flag;
	int m_hardware_type_z;
	std::unique_ptr<UINT16[]> m_buffer_objectram;
	std::unique_ptr<UINT16[]> m_buffer2_objectram;
	std::unique_ptr<UINT16[]> m_buffer_spriteram16;
	std::unique_ptr<UINT16[]> m_buffer2_spriteram16;
	int m_layers_order[16];

	int m_mcu_hs;
	UINT16 m_mcu_hs_ram[0x10];
	DECLARE_WRITE_LINE_MEMBER(sound_irq);
	DECLARE_READ16_MEMBER(ip_select_r);
	DECLARE_WRITE16_MEMBER(ip_select_w);
	DECLARE_READ16_MEMBER(protection_peekaboo_r);
	DECLARE_WRITE16_MEMBER(protection_peekaboo_w);
	DECLARE_READ16_MEMBER(megasys1A_mcu_hs_r);
	DECLARE_WRITE16_MEMBER(megasys1A_mcu_hs_w);
	DECLARE_READ16_MEMBER(iganinju_mcu_hs_r);
	DECLARE_WRITE16_MEMBER(iganinju_mcu_hs_w);
	DECLARE_READ16_MEMBER(soldamj_spriteram16_r);
	DECLARE_WRITE16_MEMBER(soldamj_spriteram16_w);
	DECLARE_READ16_MEMBER(stdragon_mcu_hs_r);
	DECLARE_WRITE16_MEMBER(stdragon_mcu_hs_w);
	DECLARE_WRITE16_MEMBER(active_layers_w);
	DECLARE_WRITE16_MEMBER(sprite_bank_w);
	DECLARE_READ16_MEMBER(sprite_flag_r);
	DECLARE_WRITE16_MEMBER(sprite_flag_w);
	DECLARE_WRITE16_MEMBER(screen_flag_w);
	DECLARE_WRITE16_MEMBER(soundlatch_w);
	DECLARE_WRITE16_MEMBER(soundlatch_z_w);
	DECLARE_WRITE16_MEMBER(soundlatch_c_w);
	DECLARE_WRITE16_MEMBER(monkelf_scroll0_w);
	DECLARE_WRITE16_MEMBER(monkelf_scroll1_w);
	void megasys1_set_vreg_flag(int which, int data);
	DECLARE_READ8_MEMBER(oki_status_1_r);
	DECLARE_READ8_MEMBER(oki_status_2_r);
	DECLARE_WRITE16_MEMBER(okim6295_both_1_w);
	DECLARE_WRITE16_MEMBER(okim6295_both_2_w);
	DECLARE_DRIVER_INIT(64street);
	DECLARE_DRIVER_INIT(chimerab);
	DECLARE_DRIVER_INIT(peekaboo);
	DECLARE_DRIVER_INIT(soldam);
	DECLARE_DRIVER_INIT(astyanax);
	DECLARE_DRIVER_INIT(stdragon);
	DECLARE_DRIVER_INIT(hayaosi1);
	DECLARE_DRIVER_INIT(soldamj);
	DECLARE_DRIVER_INIT(phantasm);
	DECLARE_DRIVER_INIT(jitsupro);
	DECLARE_DRIVER_INIT(iganinju);
	DECLARE_DRIVER_INIT(cybattlr);
	DECLARE_DRIVER_INIT(rodlandj);
	DECLARE_DRIVER_INIT(rittam);
	DECLARE_DRIVER_INIT(rodlandjb);
	DECLARE_DRIVER_INIT(avspirit);
	DECLARE_DRIVER_INIT(monkelf);
	DECLARE_DRIVER_INIT(edf);
	DECLARE_DRIVER_INIT(edfp);
	DECLARE_DRIVER_INIT(bigstrik);
	DECLARE_DRIVER_INIT(rodland);
	DECLARE_DRIVER_INIT(edfbl);
	DECLARE_DRIVER_INIT(stdragona);
	DECLARE_DRIVER_INIT(stdragonb);
	DECLARE_DRIVER_INIT(systemz);
	DECLARE_MACHINE_RESET(megasys1);
	DECLARE_VIDEO_START(megasys1);
	DECLARE_PALETTE_INIT(megasys1);
	DECLARE_MACHINE_RESET(megasys1_hachoo);
	UINT32 screen_update_megasys1(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	void screen_eof_megasys1(screen_device &screen, bool state);
	INTERRUPT_GEN_MEMBER(megasys1D_irq);
	TIMER_DEVICE_CALLBACK_MEMBER(megasys1A_scanline);
	TIMER_DEVICE_CALLBACK_MEMBER(megasys1A_iganinju_scanline);
	TIMER_DEVICE_CALLBACK_MEMBER(megasys1B_scanline);
	DECLARE_WRITE16_MEMBER(ms1_ram_w);

	void megasys1_priority_create();
	void mix_sprite_bitmap(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	void partial_clear_sprite_bitmap(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect, UINT8 param);
	void draw_sprites(screen_device &screen, bitmap_ind16 &bitmap,const rectangle &cliprect);
	inline void draw_16x16_priority_sprite(screen_device &screen, bitmap_ind16 &bitmap,const rectangle &cliprect, INT32 code, INT32 color, INT32 sx, INT32 sy, INT32 flipx, INT32 flipy, UINT8 mosaic, UINT8 mosaicsol, INT32 priority);
	void rodland_gfx_unmangle(const char *region);
	void jitsupro_gfx_unmangle(const char *region);
	void stdragona_gfx_unmangle(const char *region);
};
