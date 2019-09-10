// license:BSD-3-Clause
// copyright-holders: Olivier Galibert

// A "virtual" driver to play vgm files
// Use with mame vgmplay -bitb file.vgm

#include <zlib.h>

#include "emu.h"
#include "debugger.h"
#include "imagedev/bitbngr.h"

#include "cpu/m6502/n2a03.h"
#include "cpu/h6280/h6280.h"
#include "sound/2612intf.h"
#include "sound/ym2151.h"
#include "sound/ym2413.h"
#include "sound/2203intf.h"
#include "sound/3526intf.h"
#include "sound/3812intf.h"
#include "sound/ay8910.h"
#include "sound/c6280.h"
#include "sound/sn76496.h"
#include "sound/k053260.h"
#include "sound/segapcm.h"
#include "sound/multipcm.h"
#include "sound/gb.h"
#include "sound/pokey.h"
#include "sound/c352.h"
#include "sound/okim6295.h"

#define AS_IO16             AS_1
#define MCFG_CPU_IO16_MAP   MCFG_CPU_DATA_MAP

class vgmplay_device : public cpu_device
{
public:
	enum io8_t
	{
		REG_SIZE     = 0x00000000,
		A_YM2612     = 0x00000010,
		A_YM2151     = 0x00000020,
		A_YM2413     = 0x00000030,
		A_YM2203A    = 0x00000040,
		A_YM2203B    = 0x00000050,
		A_YM3526     = 0x00000060,
		A_YM3812     = 0x00000070,
		A_AY8910A    = 0x00000080,
		A_AY8910B    = 0x00000090,
		A_SN76496    = 0x000000a0,
		A_K053260    = 0x000000b0,
		A_C6280      = 0x000000e0,
		A_OKIM6295   = 0x000000f0,
		A_SEGAPCM    = 0x00001000,
		A_GAMEBOY    = 0x00002000,
		A_NESAPU     = 0x00002030,
		A_NESRAM     = 0x00003000,
		A_MULTIPCMA  = 0x00013000,
		A_MULTIPCMB  = 0x00013010,
		A_POKEYA     = 0x00013020,
		A_POKEYB     = 0x00013030,
	};

	enum io16_t
	{
		A_C352       = 0x00000000
	};

	vgmplay_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	virtual void device_start() override;
	virtual void device_reset() override;

	virtual UINT32 execute_min_cycles() const override;
	virtual UINT32 execute_max_cycles() const override;
	virtual UINT32 execute_input_lines() const override;
	virtual void execute_run() override;
	virtual void execute_set_input(int inputnum, int state) override;

	virtual const address_space_config *memory_space_config(address_spacenum spacenum = AS_0) const override;

	virtual void state_import(const device_state_entry &entry) override;
	virtual void state_export(const device_state_entry &entry) override;
	virtual void state_string_export(const device_state_entry &entry, std::string &str) const override;

	virtual UINT32 disasm_min_opcode_bytes() const override;
	virtual UINT32 disasm_max_opcode_bytes() const override;
	virtual offs_t disasm_disassemble(char *buffer, offs_t pc, const UINT8 *oprom, const UINT8 *opram, UINT32 options) override;

	READ8_MEMBER(segapcm_rom_r);
	READ8_MEMBER(multipcma_rom_r);
	READ8_MEMBER(multipcmb_rom_r);
	READ8_MEMBER(k053260_rom_r);
	READ8_MEMBER(okim6295_rom_r);
	READ8_MEMBER(c352_rom_r);

private:
	struct rom_block {
		offs_t start_address;
		offs_t end_address;
		std::unique_ptr<UINT8[]> data;

		rom_block(rom_block &&) = default;
		rom_block(offs_t start, offs_t end, std::unique_ptr<UINT8[]> &&d) : start_address(start), end_address(end), data(std::move(d)) {}
	};

	enum { RESET, RUN, DONE };

	address_space_config m_file_config, m_io_config, m_io16_config;
	address_space *m_file, *m_io, *m_io16;

	int m_icount, m_state;

	UINT32 m_pc;

	std::list<rom_block> m_rom_blocks[2][0x40];

	std::vector<UINT8> m_data_streams[0x40];
	std::vector<UINT32> m_data_stream_starts[0x40];

	UINT32 m_ym2612_stream_offset;

	UINT8 rom_r(int chip, UINT8 type, offs_t offset);
	UINT32 handle_data_block(UINT32 address);
	void blocks_clear();
};

const device_type VGMPLAY = &device_creator<vgmplay_device>;

class vgmplay_state : public driver_device
{
public:
	vgmplay_state(const machine_config &mconfig, device_type type, const char *tag);

	virtual void machine_start() override;

	DECLARE_READ8_MEMBER(file_r);
	DECLARE_READ8_MEMBER(file_size_r);

	DECLARE_WRITE8_MEMBER(multipcm_bank_lo_a_w);
	DECLARE_WRITE8_MEMBER(multipcm_bank_hi_a_w);
	DECLARE_WRITE8_MEMBER(multipcm_bank_lo_b_w);
	DECLARE_WRITE8_MEMBER(multipcm_bank_hi_b_w);

private:
	std::vector<UINT8> m_file_data;
	required_device<bitbanger_device> m_file;
	required_device<ym2612_device>  m_ym2612;
	required_device<ym2151_device>  m_ym2151;
	required_device<ym2413_device>  m_ym2413;
	required_device<ym2203_device>  m_ym2203a;
	required_device<ym2203_device>  m_ym2203b;
	required_device<ym3526_device>  m_ym3526;
	required_device<ym3812_device>  m_ym3812;
	required_device<ay8910_device>  m_ay8910a;
	required_device<ay8910_device>  m_ay8910b;
	required_device<sn76496_device> m_sn76496;
	required_device<segapcm_device> m_segapcm;
	required_device<multipcm_device> m_multipcma;
	required_device<multipcm_device> m_multipcmb;
	required_device<gameboy_sound_device> m_dmg;
	required_device<n2a03_device> m_nescpu;
	required_shared_ptr<UINT8> m_nesram;
	required_device<k053260_device> m_k053260;
	required_device<c6280_device> m_c6280;
	required_device<h6280_device> m_h6280;
	required_device<pokey_device> m_pokeya;
	required_device<pokey_device> m_pokeyb;
	required_device<c352_device> m_c352;
	required_device<okim6295_device> m_okim6295;

	UINT32 m_multipcma_bank_l;
	UINT32 m_multipcma_bank_r;
	UINT32 m_multipcmb_bank_l;
	UINT32 m_multipcmb_bank_r;

	UINT32 r32(int offset) const;
	UINT8 r8(int offset) const;
};

vgmplay_device::vgmplay_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
	cpu_device(mconfig, VGMPLAY, "VGM Player engine", tag, owner, clock, "vgmplay_core", __FILE__),
	m_file_config("file", ENDIANNESS_LITTLE, 8, 32),
	m_io_config("io", ENDIANNESS_LITTLE, 8, 32),
	m_io16_config("io16", ENDIANNESS_LITTLE, 16, 32)
{
}

void vgmplay_device::device_start()
{
	m_icountptr = &m_icount;
	m_file = &space(AS_PROGRAM);
	m_io   = &space(AS_IO);
	m_io16  = &space(AS_IO16);

	save_item(NAME(m_pc));

	state_add(STATE_GENPC,     "GENPC",     m_pc).noshow();
	state_add(STATE_GENPCBASE, "GENPCBASE", m_pc).noshow();
}

void vgmplay_device::device_reset()
{
	m_pc = 0;
	m_state = RESET;

	m_ym2612_stream_offset = 0;
	blocks_clear();
}

UINT32 vgmplay_device::execute_min_cycles() const
{
	return 0;
}

UINT32 vgmplay_device::execute_max_cycles() const
{
	return 65536;
}

UINT32 vgmplay_device::execute_input_lines() const
{
	return 0;
}

void vgmplay_device::blocks_clear()
{
	for(int i = 0; i < 0x40; i++) {
		m_rom_blocks[0][i].clear();
		m_rom_blocks[1][i].clear();
		m_data_streams[i].clear();
		m_data_stream_starts[i].clear();
	}
}

UINT32 vgmplay_device::handle_data_block(UINT32 address)
{
	UINT32 size = m_file->read_dword(m_pc+3);
	int second = (size & 0x80000000) ? 1 : 0;
	size &= 0x7fffffff;

	UINT8 type = m_file->read_byte(m_pc+2);
	if(type < 0x40) {
		UINT32 start = m_data_streams[type].size();
		m_data_stream_starts[type].push_back(start);
		m_data_streams[type].resize(start + size);
		for(UINT32 i=0; i<size; i++)
			m_data_streams[type][start+i] = m_file->read_byte(m_pc+7+i);

	} else if(type < 0x7f)
		logerror("ignored compressed stream size %x type %02x\n", size, type);

	else if(type < 0x80)
		logerror("ignored compression table size %x\n", size);

	else if(type < 0xc0) {
		//UINT32 rs = m_file->read_dword(m_pc+7);
		UINT32 start = m_file->read_dword(m_pc+11);
		std::unique_ptr<UINT8[]> block = std::make_unique<UINT8[]>(size - 8);
		for(UINT32 i=0; i<size-8; i++)
			block[i] = m_file->read_byte(m_pc+15+i);
		m_rom_blocks[second][type - 0x80].emplace_front(start, start+size-9, std::move(block));
	} else if (type == 0xc2) {
		UINT16 start = m_file->read_word(m_pc+7);
		UINT32 data_size = size - 2;
		for (int i = 0; i < data_size; i++)
		{
			m_io->write_byte(A_NESRAM + start + i, m_file->read_byte(m_pc + 9 + i));
		}
	} else {
		logerror("ignored ram block size %x type %02x\n", size, type);
	}
	return 7+size;
}

void vgmplay_device::execute_run()
{
	while(m_icount > 0) {
		switch(m_state) {
		case RESET: {
			UINT32 size = m_io->read_dword(REG_SIZE);
			if(!size) {
				m_pc = 0;
				m_state = DONE;
				break;
			}
			UINT32 version = m_file->read_dword(8);
			m_pc = version < 0x150 ? 0x40 : 0x34 + m_file->read_dword(0x34);
			m_state = RUN;
			break;
		}
		case RUN: {
			if(machine().debug_flags & DEBUG_FLAG_ENABLED)
				debugger_instruction_hook(this, m_pc);
			UINT8 code = m_file->read_byte(m_pc);
			switch(code) {
			case 0x4f:
				m_io->write_byte(A_SN76496+0, m_file->read_byte(m_pc+1));
				m_pc += 2;
				break;

			case 0x50:
				m_io->write_byte(A_SN76496+1, m_file->read_byte(m_pc+1));
				m_pc += 2;
				break;

			case 0x51:
				m_io->write_byte(A_YM2413+0, m_file->read_byte(m_pc+1));
				m_io->write_byte(A_YM2413+1, m_file->read_byte(m_pc+2));
				m_pc += 3;
				break;

			case 0x52:
				m_io->write_byte(A_YM2612+0, m_file->read_byte(m_pc+1));
				m_io->write_byte(A_YM2612+1, m_file->read_byte(m_pc+2));
				m_pc += 3;
				break;

			case 0x53:
				m_io->write_byte(A_YM2612+2, m_file->read_byte(m_pc+1));
				m_io->write_byte(A_YM2612+3, m_file->read_byte(m_pc+2));
				m_pc += 3;
				break;

			case 0x54:
				m_io->write_byte(A_YM2151+0, m_file->read_byte(m_pc+1));
				m_io->write_byte(A_YM2151+1, m_file->read_byte(m_pc+2));
				m_pc += 3;
				break;

			case 0x55:
				m_io->write_byte(A_YM2203A+0, m_file->read_byte(m_pc+1));
				m_io->write_byte(A_YM2203A+1, m_file->read_byte(m_pc+2));
				m_pc += 3;
				break;

			case 0xA5:
				m_io->write_byte(A_YM2203B+0, m_file->read_byte(m_pc+1));
				m_io->write_byte(A_YM2203B+1, m_file->read_byte(m_pc+2));
				m_pc += 3;
				break;

			case 0x5a:
				m_io->write_byte(A_YM3812+0, m_file->read_byte(m_pc+1));
				m_io->write_byte(A_YM3812+1, m_file->read_byte(m_pc+2));
				m_pc += 3;
				break;

			case 0x5b:
				m_io->write_byte(A_YM3526+0, m_file->read_byte(m_pc+1));
				m_io->write_byte(A_YM3526+1, m_file->read_byte(m_pc+2));
				m_pc += 3;
				break;

			case 0x61: {
				UINT32 duration = m_file->read_word(m_pc+1);
				m_icount -= duration;
				m_pc += 3;
				break;
			}

			case 0x62:
				m_icount -= 735;
				m_pc++;
				break;

			case 0x63:
				m_icount -= 882;
				m_pc++;
				break;

			case 0x66:
			{
				UINT32 loop_offset = m_file->read_dword(0x1c);
				if (!loop_offset)
				{
					m_state = DONE;
					break;
				}

				m_pc = 0x1c + loop_offset;
				break;
			}

			case 0x67:
				m_pc += handle_data_block(m_pc);
				break;

			case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: case 0x76: case 0x77:
			case 0x78: case 0x79: case 0x7a: case 0x7b: case 0x7c: case 0x7d: case 0x7e: case 0x7f:
				m_icount -= 1+(code & 0xf);
				m_pc += 1;
				break;

			case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: case 0x86: case 0x87:
			case 0x88: case 0x89: case 0x8a: case 0x8b: case 0x8c: case 0x8d: case 0x8e: case 0x8f:
				if(!m_data_streams[0].empty()) {
					if(m_ym2612_stream_offset >= int(m_data_streams[0].size()))
						m_ym2612_stream_offset = 0;

					m_io->write_byte(A_YM2612+0, 0x2a);
					m_io->write_byte(A_YM2612+1, m_data_streams[0][m_ym2612_stream_offset]);
					m_ym2612_stream_offset++;
				}
				m_pc += 1;
				m_icount -= code & 0xf;
				break;

			case 0xa0:
			{
				UINT8 reg = m_file->read_byte(m_pc+1);
				if (reg & 0x80)
				{
					m_io->write_byte(A_AY8910A+1, reg & 0x7f);
					m_io->write_byte(A_AY8910A+0, m_file->read_byte(m_pc+2));
				}
				else
				{
					m_io->write_byte(A_AY8910B+1, reg & 0x7f);
					m_io->write_byte(A_AY8910B+0, m_file->read_byte(m_pc+2));
				}
				m_pc += 3;
				break;
			}

			case 0xb3:
				m_io->write_byte(A_GAMEBOY + m_file->read_byte(m_pc+1), m_file->read_byte(m_pc+2));
				m_pc += 3;
				break;

			case 0xc0:
				m_io->write_byte(A_SEGAPCM + (m_file->read_word(m_pc+1) & 0x7ff), m_file->read_byte(m_pc+3));
				m_pc += 4;
				break;

			case 0xb4:
				m_io->write_byte(A_NESAPU + m_file->read_byte(m_pc+1), m_file->read_byte(m_pc+2));
				m_pc += 3;
				break;

			case 0xb5:
			{
				UINT8 offset = m_file->read_byte(m_pc+1);
				if (offset & 0x80)
					m_io->write_byte(A_MULTIPCMB + (offset & 0x7f), m_file->read_byte(m_pc+2));
				else
					m_io->write_byte(A_MULTIPCMA + (offset & 0x7f), m_file->read_byte(m_pc+2));
				m_pc += 3;
				break;
			}

			case 0xb8:
				m_io->write_byte(A_OKIM6295 + m_file->read_byte(m_pc+1), m_file->read_byte(m_pc+2));
				m_pc += 3;
				break;

			case 0xb9:
				m_io->write_byte(A_C6280 + m_file->read_byte(m_pc+1), m_file->read_byte(m_pc+2));
				m_pc += 3;
				break;

			case 0xba:
				m_io->write_byte(A_K053260 + m_file->read_byte(m_pc+1), m_file->read_byte(m_pc+2));
				m_pc += 3;
				break;

			case 0xbb:
			{
				UINT8 offset = m_file->read_byte(m_pc+1);
				if (offset & 0x80)
					m_io->write_byte(A_POKEYA + (offset & 0x7f), m_file->read_byte(m_pc+2));
				else
					m_io->write_byte(A_POKEYB + (offset & 0x7f), m_file->read_byte(m_pc+2));
				m_pc += 3;
				break;
			}

			case 0xc3:
			{
				UINT8 offset = m_file->read_byte(m_pc+1);
				if (offset & 0x80)
				{
					m_io->write_byte(A_MULTIPCMB + 4 + (offset & 0x7f), m_file->read_byte(m_pc+3));
					m_io->write_byte(A_MULTIPCMB + 8 + (offset & 0x7f), m_file->read_byte(m_pc+2));
				}
				else
				{
					m_io->write_byte(A_MULTIPCMA + 4 + (offset & 0x7f), m_file->read_byte(m_pc+3));
					m_io->write_byte(A_MULTIPCMA + 8 + (offset & 0x7f), m_file->read_byte(m_pc+2));
				}
				m_pc += 4;
				break;
			}

			case 0xe0:
				m_ym2612_stream_offset = m_file->read_dword(m_pc+1);
				m_pc += 5;
				break;

			case 0xe1:
			{
				UINT32 addr = (m_file->read_byte(m_pc+1) << 8) | m_file->read_byte(m_pc+2);
				UINT16 data = (m_file->read_byte(m_pc+3) << 8) | m_file->read_byte(m_pc+4);
				m_io16->write_word(A_C352 + (addr << 1), data);
				m_pc += 5;
				break;
			}

			default:
				logerror("unhandled code %02x (%02x %02x %02x %02x)\n", code, m_file->read_byte(m_pc+1), m_file->read_byte(m_pc+2), m_file->read_byte(m_pc+3), m_file->read_byte(m_pc+4));
				m_state = DONE;
				m_icount = 0;
				break;
			}
			break;
		}
		case DONE: {
			static bool done = false;
			if(!done) {
				logerror("done\n");
				done = true;
			}
			if(machine().debug_flags & DEBUG_FLAG_ENABLED)
				debugger_instruction_hook(this, m_pc);
			m_icount = 0;
			break;
		}
		}
	}
}

void vgmplay_device::execute_set_input(int inputnum, int state)
{
}

const address_space_config *vgmplay_device::memory_space_config(address_spacenum spacenum) const
{
	switch(spacenum) {
	case AS_PROGRAM: return &m_file_config;
	case AS_IO:      return &m_io_config;
	case AS_IO16:    return &m_io16_config;
	default:         return nullptr;
	}
}

void vgmplay_device::state_import(const device_state_entry &entry)
{
}

void vgmplay_device::state_export(const device_state_entry &entry)
{
}

void vgmplay_device::state_string_export(const device_state_entry &entry, std::string &str) const
{
}

UINT32 vgmplay_device::disasm_min_opcode_bytes() const
{
	return 1;
}

UINT32 vgmplay_device::disasm_max_opcode_bytes() const
{
	return 9;
}

offs_t vgmplay_device::disasm_disassemble(char *buffer, offs_t pc, const UINT8 *oprom, const UINT8 *opram, UINT32 options)
{
	switch(oprom[0]) {
	case 0x4f:
		sprintf(buffer, "psg r06 = %02x", oprom[1]);
		return 2 | DASMFLAG_SUPPORTED;

	case 0x50:
		sprintf(buffer, "psg write %02x", oprom[1]);
		return 2 | DASMFLAG_SUPPORTED;

	case 0x51:
		sprintf(buffer, "ym2413 r%02x = %02x", oprom[1], oprom[2]);
		return 3 | DASMFLAG_SUPPORTED;

	case 0x52:
		sprintf(buffer, "ym2612.0 r%02x = %02x", oprom[1], oprom[2]);
		return 3 | DASMFLAG_SUPPORTED;

	case 0x53:
		sprintf(buffer, "ym2612.1 r%02x = %02x", oprom[1], oprom[2]);
		return 3 | DASMFLAG_SUPPORTED;

	case 0x54:
		sprintf(buffer, "ym2151 r%02x = %02x", oprom[1], oprom[2]);
		return 3 | DASMFLAG_SUPPORTED;

	case 0x55:
		sprintf(buffer, "ym2203a r%02x = %02x", oprom[1], oprom[2]);
		return 3 | DASMFLAG_SUPPORTED;

	case 0x56:
		sprintf(buffer, "ym2608.0 r%02x = %02x", oprom[1], oprom[2]);
		return 3 | DASMFLAG_SUPPORTED;

	case 0x57:
		sprintf(buffer, "ym2608.1 r%02x = %02x", oprom[1], oprom[2]);
		return 3 | DASMFLAG_SUPPORTED;

	case 0x58:
		sprintf(buffer, "ym2610.0 r%02x = %02x", oprom[1], oprom[2]);
		return 3 | DASMFLAG_SUPPORTED;

	case 0x59:
		sprintf(buffer, "ym2610.1 r%02x = %02x", oprom[1], oprom[2]);
		return 3 | DASMFLAG_SUPPORTED;

	case 0x5a:
		sprintf(buffer, "ym3812 r%02x = %02x", oprom[1], oprom[2]);
		return 3 | DASMFLAG_SUPPORTED;

	case 0x5b:
		sprintf(buffer, "ym3526 r%02x = %02x", oprom[1], oprom[2]);
		return 3 | DASMFLAG_SUPPORTED;

	case 0x5c:
		sprintf(buffer, "y8950 r%02x = %02x", oprom[1], oprom[2]);
		return 3 | DASMFLAG_SUPPORTED;

	case 0x5d:
		sprintf(buffer, "ymz280b r%02x = %02x", oprom[1], oprom[2]);
		return 3 | DASMFLAG_SUPPORTED;

	case 0x5e:
		sprintf(buffer, "ymf262.0 r%02x = %02x", oprom[1], oprom[2]);
		return 3 | DASMFLAG_SUPPORTED;

	case 0x5f:
		sprintf(buffer, "ymf262.1 r%02x = %02x", oprom[1], oprom[2]);
		return 3 | DASMFLAG_SUPPORTED;

	case 0x61: {
		UINT32 duration = oprom[1] | (oprom[2] << 8);
		sprintf(buffer, "wait %d", duration);
		return 3 | DASMFLAG_SUPPORTED;
	}

	case 0x62:
		sprintf(buffer, "wait 735");
		return 1 | DASMFLAG_SUPPORTED;

	case 0x63:
		sprintf(buffer, "wait 882");
		return 1 | DASMFLAG_SUPPORTED;

	case 0x66:
		sprintf(buffer, "end");
		return 1 | DASMFLAG_SUPPORTED;

	case 0x67: {
		static const char *const basic_types[8] = {
			"ym2612 pcm",
			"rf5c68 pcm",
			"rf5c164 pcm",
			"pwm pcm",
			"okim6258 adpcm",
			"huc6280 pcm",
			"scsp pcm",
			"nes apu dpcm"
		};

		static const char *const rom_types[20] = {
			"sega pcm rom",
			"ym2608 delta-t rom",
			"ym2610 adpcm rom",
			"ym2610 delta-t rom",
			"ymf278b rom",
			"ymf271 rom",
			"ymz280b rom",
			"ymf278b rom",
			"y8950 delta-t rom",
			"multipcm rom",
			"upd7759 rom",
			"okim6295 rom",
			"k054539 rom",
			"c140 rom",
			"k053260 rom",
			"qsound rom",
			"es5505/es5506 rom",
			"x1-010 rom",
			"c352 rom",
			"ga20 rom"
		};

		static const char *const ram_types[3] = {
			"rf5c68 ram",
			"rf5c164 ram",
			"nes apu ram"
		};

		static const char *const ram2_types[2] = {
			"scsp ram",
			"es5503 ram"
		};

		UINT8 type = oprom[2];
		UINT32 size = oprom[3] | (oprom[4] << 8) | (oprom[5] << 16) | (oprom[6] << 24);
		if(type < 0x8)
			sprintf(buffer, "data-block %x, %s", size, basic_types[type]);
		else if(type < 0x40)
			sprintf(buffer, "data-block %x, %02x", size, type);
		else if(type < 0x48)
			sprintf(buffer, "data-block %x comp., %s", size, basic_types[type & 0x3f]);
		else if(type < 0x7f)
			sprintf(buffer, "data-block %x comp., %02x", size, type & 0x3f);
		else if(type < 0x80)
			sprintf(buffer, "decomp-table %x, %02x/%02x", size, oprom[7], oprom[8]);
		else if(type < 0x94)
			sprintf(buffer, "data-block %x, %s", size, rom_types[type & 0x7f]);
		else if(type < 0xc0)
			sprintf(buffer, "data-block %x, rom %02x", size, type);
		else if(type < 0xc3)
			sprintf(buffer, "data-block %x, %s", size, ram_types[type & 0x1f]);
		else if(type < 0xe0)
			sprintf(buffer, "data-block %x, ram %02x", size, type);
		else if(type < 0xe2)
			sprintf(buffer, "data-block %x, %s", size, ram2_types[type & 0x1f]);
		else
			sprintf(buffer, "data-block %x, ram %02x", size, type);
		return (7+size) | DASMFLAG_SUPPORTED;
	}

	case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: case 0x76: case 0x77:
	case 0x78: case 0x79: case 0x7a: case 0x7b: case 0x7c: case 0x7d: case 0x7e: case 0x7f:
		sprintf(buffer, "wait %d", 1+(oprom[0] & 0x0f));
		return 1 | DASMFLAG_SUPPORTED;

	case 0x80:
		sprintf(buffer, "ym2612.0 r2a = rom++");
		return 1 | DASMFLAG_SUPPORTED;

	case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: case 0x86: case 0x87:
	case 0x88: case 0x89: case 0x8a: case 0x8b: case 0x8c: case 0x8d: case 0x8e: case 0x8f:
		sprintf(buffer, "ym2612.0 r2a = rom++; wait %d", oprom[0] & 0xf);
		return 1 | DASMFLAG_SUPPORTED;

	case 0xa0:
		sprintf(buffer, "ay8910 r%02x = %02x", oprom[1], oprom[2]);
		return 3 | DASMFLAG_SUPPORTED;

	case 0xa5:
		sprintf(buffer, "ym2203b r%02x = %02x", oprom[1], oprom[2]);
		return 3 | DASMFLAG_SUPPORTED;

	case 0xb0:
		sprintf(buffer, "rf5c68 r%02x = %02x", oprom[1], oprom[2]);
		return 3 | DASMFLAG_SUPPORTED;

	case 0xb1:
		sprintf(buffer, "rf5c164 r%02x = %02x", oprom[1], oprom[2]);
		return 3 | DASMFLAG_SUPPORTED;

	case 0xb2:
		sprintf(buffer, "pwm r%x = %03x", oprom[1] >> 4, oprom[2] | ((oprom[1] & 0xf) << 8));
		return 3 | DASMFLAG_SUPPORTED;

	case 0xb3:
		sprintf(buffer, "dmg r%02x = %02x", oprom[1], oprom[2]);
		return 3 | DASMFLAG_SUPPORTED;

	case 0xb4:
		sprintf(buffer, "nesapu r%02x = %02x", oprom[1], oprom[2]);
		return 3 | DASMFLAG_SUPPORTED;

	case 0xb5:
		sprintf(buffer, "multipcm r%02x = %02x", oprom[1], oprom[2]);
		return 3 | DASMFLAG_SUPPORTED;

	case 0xb6:
		sprintf(buffer, "upd7759 r%02x = %02x", oprom[1], oprom[2]);
		return 3 | DASMFLAG_SUPPORTED;

	case 0xb7:
		sprintf(buffer, "okim6258 r%02x = %02x", oprom[1], oprom[2]);
		return 3 | DASMFLAG_SUPPORTED;

	case 0xb8:
		sprintf(buffer, "okim6295 r%02x = %02x", oprom[1], oprom[2]);
		return 3 | DASMFLAG_SUPPORTED;

	case 0xb9:
		sprintf(buffer, "huc6280 r%02x = %02x", oprom[1], oprom[2]);
		return 3 | DASMFLAG_SUPPORTED;

	case 0xba:
		sprintf(buffer, "k053260 r%02x = %02x", oprom[1], oprom[2]);
		return 3 | DASMFLAG_SUPPORTED;

	case 0xbb:
		sprintf(buffer, "pokey r%02x = %02x", oprom[1], oprom[2]);
		return 3 | DASMFLAG_SUPPORTED;

	case 0xc0:
		sprintf(buffer, "segapcm %04x = %02x", oprom[1] | (oprom[2] << 8), oprom[3]);
		return 4 | DASMFLAG_SUPPORTED;

	case 0xc1:
		sprintf(buffer, "rf5c68 %04x = %02x", oprom[1] | (oprom[2] << 8), oprom[3]);
		return 4 | DASMFLAG_SUPPORTED;

	case 0xc2:
		sprintf(buffer, "rf5c163 %04x = %02x", oprom[1] | (oprom[2] << 8), oprom[3]);
		return 4 | DASMFLAG_SUPPORTED;

	case 0xc3:
		sprintf(buffer, "multipcm c%02x.off = %04x", oprom[1], oprom[2] | (oprom[3] << 8));
		return 4 | DASMFLAG_SUPPORTED;

	case 0xc4:
		sprintf(buffer, "qsound %02x = %04x", oprom[3], oprom[2] | (oprom[1] << 8));
		return 4 | DASMFLAG_SUPPORTED;

	case 0xd0:
		sprintf(buffer, "ymf278b r%02x.%02x = %02x", oprom[1], oprom[2], oprom[3]);
		return 4 | DASMFLAG_SUPPORTED;

	case 0xd1:
		sprintf(buffer, "ymf271 r%02x.%02x = %02x", oprom[1], oprom[2], oprom[3]);
		return 4 | DASMFLAG_SUPPORTED;

	case 0xd2:
		sprintf(buffer, "scc1 r%02x.%02x = %02x", oprom[1], oprom[2], oprom[3]);
		return 4 | DASMFLAG_SUPPORTED;

	case 0xd3:
		sprintf(buffer, "k054539 r%02x.%02x = %02x", oprom[1], oprom[2], oprom[3]);
		return 4 | DASMFLAG_SUPPORTED;

	case 0xd4:
		sprintf(buffer, "c140 r%02x.%02x = %02x", oprom[1], oprom[2], oprom[3]);
		return 4 | DASMFLAG_SUPPORTED;

	case 0xe0: {
		UINT32 off = oprom[1] | (oprom[2] << 8) | (oprom[3] << 16) | (oprom[4] << 24);
		sprintf(buffer, "ym2612 offset = %x", off);
		return 5 | DASMFLAG_SUPPORTED;
	}

	case 0xe1: {
		UINT16 addr = (oprom[1] << 8) | oprom[2];
		UINT16 data = (oprom[3] << 8) | oprom[4];
		sprintf(buffer, "c352 r%04x = %04x", addr, data);
		return 5 | DASMFLAG_SUPPORTED;
	}

	default:
		sprintf(buffer, "?? %02x", oprom[0]);
		return 1 | DASMFLAG_SUPPORTED;
	}
}

UINT8 vgmplay_device::rom_r(int chip, UINT8 type, offs_t offset)
{
	for(const auto &b : m_rom_blocks[chip][type - 0x80])
	{
		if(offset >= b.start_address && offset <= b.end_address)
		{
			return b.data[offset - b.start_address];
		}
	}
	return 0;
}

READ8_MEMBER(vgmplay_device::segapcm_rom_r)
{
	return rom_r(0, 0x80, offset);
}

READ8_MEMBER(vgmplay_device::multipcma_rom_r)
{
	return rom_r(0, 0x89, offset);
}

READ8_MEMBER(vgmplay_device::multipcmb_rom_r)
{
	return rom_r(1, 0x89, offset);
}

READ8_MEMBER(vgmplay_device::okim6295_rom_r)
{
	return rom_r(0, 0x8b, offset);
}

READ8_MEMBER(vgmplay_device::k053260_rom_r)
{
	return rom_r(0, 0x8e, offset);
}

READ8_MEMBER(vgmplay_device::c352_rom_r)
{
	return rom_r(0, 0x92, offset);
}

vgmplay_state::vgmplay_state(const machine_config &mconfig, device_type type, const char *tag)
	: driver_device(mconfig, type, tag)
	, m_file(*this, "file")
	, m_ym2612(*this, "ym2612")
	, m_ym2151(*this, "ym2151")
	, m_ym2413(*this, "ym2413")
	, m_ym2203a(*this, "ym2203a")
	, m_ym2203b(*this, "ym2203b")
	, m_ym3526(*this, "ym3526")
	, m_ym3812(*this, "ym3812")
	, m_ay8910a(*this, "ay8910a")
	, m_ay8910b(*this, "ay8910b")
	, m_sn76496(*this, "sn76496")
	, m_segapcm(*this, "segapcm")
	, m_multipcma(*this, "multipcma")
	, m_multipcmb(*this, "multipcmb")
	, m_dmg(*this, "dmg")
	, m_nescpu(*this, "nescpu")
	, m_nesram(*this, "nesapu_ram")
	, m_k053260(*this, "k053260")
	, m_c6280(*this, "c6280")
	, m_h6280(*this, "h6280")
	, m_pokeya(*this, "pokeya")
	, m_pokeyb(*this, "pokeyb")
	, m_c352(*this, "c352")
	, m_okim6295(*this, "okim6295")
{
}

UINT32 vgmplay_state::r32(int off) const
{
	if(off + 3 < int(m_file_data.size()))
		return m_file_data[off] | (m_file_data[off+1] << 8) | (m_file_data[off+2] << 16) | (m_file_data[off+3] << 24);
	return 0;
}

UINT8 vgmplay_state::r8(int off) const
{
	if(off < int(m_file_data.size()))
		return m_file_data[off];
	return 0;
}

void vgmplay_state::machine_start()
{
	//m_nescpu->
	UINT32 size = 0;
	if(m_file->exists()) {
		size = m_file->length();
		m_file_data.resize(size);
		m_file->input(&m_file_data[0], size);

		// Decompress gzip-compressed files (aka vgz)
		if(m_file_data[0] == 0x1f && m_file_data[1] == 0x8b) {
			std::vector<UINT8> decomp;
			int bs = m_file_data.size();
			decomp.resize(2*bs);
			z_stream str;
			str.zalloc = nullptr;
			str.zfree = nullptr;
			str.opaque = nullptr;
			str.data_type = 0;
			str.next_in = &m_file_data[0];
			str.avail_in = m_file_data.size();
			str.total_in = 0;
			str.total_out = 0;
			int err = inflateInit2(&str, 31);
			if(err != Z_OK) {
				logerror("gzip header but not a gzip file\n");
				m_file_data.clear();
				return;
			}
			do {
				if(str.total_out >= decomp.size())
					decomp.resize(decomp.size() + bs);
				str.next_out = &decomp[str.total_out];
				str.avail_out = decomp.size() - str.total_out;
				err = inflate(&str, Z_SYNC_FLUSH);
			} while(err == Z_OK);
			if(err != Z_STREAM_END) {
				logerror("broken gzip file\n");
				m_file_data.clear();
				return;
			}
			m_file_data.resize(str.total_out);
			memcpy(&m_file_data[0], &decomp[0], str.total_out);
		}

		if(m_file_data.size() < 0x40 || r32(0) != 0x206d6756) {
			logerror("Not a vgm/vgz file\n");
			m_file_data.clear();
			return;
		}

		UINT32 version = r32(8);
		logerror("File version %x.%02x\n", version >> 8, version & 0xff);

		UINT32 header_size = 0;
		if(version < 0x151)
			header_size = 0x40;
		else if(version < 0x161)
			header_size = 0x80;
		else if(version < 0x171)
			header_size = 0xc0;
		else
			header_size = 0x100;
		logerror("Header size according to version is %x, header size according to header is %x\n", header_size, r32(0x34) + 0x34);

		UINT32 data_start = header_size;
		if (version >= 0x150 && r32(0x34))
			data_start = r32(0x34) + 0x34;

		// Parse clocks
		if(r32(0x0c))
			m_sn76496->set_unscaled_clock(r32(0x0c));
		if(r32(0x10))
			m_ym2413->set_unscaled_clock(r32(0x10));
		if(version >= 0x110 && r32(0x2c))
			m_ym2612->set_unscaled_clock(r32(0x2c));
		if(version >= 0x110 && r32(0x30))
			m_ym2151->set_unscaled_clock(r32(0x30));

		if(version >= 0x151 && r32(0x38))
			m_segapcm->set_unscaled_clock(r32(0x38));

		if (data_start > 0x40)
		{
			if(version >= 0x151 && r32(0x40))
				logerror("Warning: file requests an unsupported RF5C68\n");
			if(version >= 0x151 && r32(0x44)) {
				UINT32 clock = r32(0x44);
				m_ym2203a->set_unscaled_clock(clock & ~0x40000000);
				if (clock & 0x40000000)
				{
					clock &= ~0x40000000;
					m_ym2203b->set_unscaled_clock(clock);
				}
			}
			if(version >= 0x151 && r32(0x48))
				logerror("Warning: file requests an unsupported YM2608\n");
			if(version >= 0x151 && r32(0x4c))
				logerror("Warning: file requests an unsupported %s\n", r32(0x4c) & 0x80000000 ? "YM2610B" : "YM2610");
			if(version >= 0x151 && r32(0x50)) {
				m_ym3812->set_unscaled_clock(r32(0x50));
			}
			if(version >= 0x151 && r32(0x54)) {
				m_ym3526->set_unscaled_clock(r32(0x54));
			}
			if(version >= 0x151 && r32(0x58))
				logerror("Warning: file requests an unsupported Y8950\n");
			if(version >= 0x151 && r32(0x5c))
				logerror("Warning: file requests an unsupported YMF262\n");
			if(version >= 0x151 && r32(0x60))
				logerror("Warning: file requests an unsupported YMF278B\n");
			if(version >= 0x151 && r32(0x64))
				logerror("Warning: file requests an unsupported YMF271\n");
			if(version >= 0x151 && r32(0x68))
				logerror("Warning: file requests an unsupported YMZ280B\n");
			if(version >= 0x151 && r32(0x6c))
				logerror("Warning: file requests an unsupported RF5C164\n");
			if(version >= 0x151 && r32(0x70))
				logerror("Warning: file requests an unsupported PWM\n");
			if(version >= 0x151 && r32(0x74)) {
				UINT32 clock = r32(0x74);
				m_ay8910a->set_unscaled_clock(clock & ~0x40000000);
				if (clock & 0x40000000) {
					clock &= ~0x40000000;
					m_ay8910b->set_unscaled_clock(clock);
				}
			}
			if(version >= 0x151 && r8(0x78)) {
				UINT8 type = r8(0x78);
				if (type & 0x10)
				{
					ay8910_device::set_psg_type(*m_ay8910a, ay8910_device::PSG_TYPE_YM);
					ay8910_device::set_psg_type(*m_ay8910b, ay8910_device::PSG_TYPE_YM);
				}
			}
			if(version >= 0x151 && r8(0x79)) {
				UINT8 flags = r8(0x79);
				UINT8 to_set = 0;
				if (flags & 1)
					to_set |= AY8910_LEGACY_OUTPUT;
				if (flags & 2)
					to_set |= AY8910_SINGLE_OUTPUT;
				if (flags & 4)
					to_set |= AY8910_DISCRETE_OUTPUT;
				ay8910_device::set_flags(*m_ay8910a, to_set);
				ay8910_device::set_flags(*m_ay8910b, to_set);
			}
			if(version >= 0x151 && r8(0x7a)) {
				UINT8 flags = r8(0x7a);
				UINT8 to_set = 0;
				if (flags & 1)
					to_set |= AY8910_LEGACY_OUTPUT;
				if (flags & 2)
					to_set |= AY8910_SINGLE_OUTPUT;
				if (flags & 4)
					to_set |= AY8910_DISCRETE_OUTPUT;
				ay8910_device::set_flags(*m_ym2203a, to_set);
				ay8910_device::set_flags(*m_ym2203b, to_set);
			}
		}

		if (data_start > 0x80)
		{
			if(version >= 0x161 && r32(0x80)) {
				m_dmg->set_unscaled_clock(r32(0x80));
			}
			if(version >= 0x161 && r32(0x84)) {
				m_nescpu->set_unscaled_clock(r32(0x84));
				m_nescpu->m_apu->set_unscaled_clock(r32(0x84));
			}
			if(version >= 0x161 && r32(0x88)) {
				UINT32 clock = r32(0x88);
				m_multipcma->set_unscaled_clock(clock & ~0x40000000);
				if (clock & 0x40000000) {
					clock &= ~0x40000000;
					m_multipcmb->set_unscaled_clock(clock);
				}
			}
			if(version >= 0x161 && r32(0x98)) {
				UINT32 clock = r32(0x98);
				UINT32 pin7 = 0;
				if (clock & 0x80000000) {
					clock &= ~0x80000000;
					pin7 = 1;
				}
				okim6295_device::static_set_pin7(*m_okim6295, pin7);
				m_okim6295->set_unscaled_clock(clock);
			}
			if(version >= 0x161 && r32(0xac)) {
				m_k053260->set_unscaled_clock(r32(0xac));
			}
			if(version >= 0x161 && r32(0xa4)) {
				m_c6280->set_unscaled_clock(r32(0xa4));
			}
			if(version >= 0x161 && r32(0xb0)) {
				UINT32 clock = r32(0xb0);
				m_pokeya->set_unscaled_clock(clock);
				if (clock & 0x40000000) {
					clock &= ~0x40000000;
					m_pokeyb->set_unscaled_clock(clock);
				}
			}
		}

		if (data_start > 0xc0)
		{
			if(version >= 0x171 && r8(0xd6)) {
				c352_device::static_set_divider(*m_c352, r8(0xd6) * 4);
			}
			if(version >= 0x171 && r32(0xdc)) {
				m_c352->set_unscaled_clock(r32(0xdc));
			}
		}
	}
}

READ8_MEMBER(vgmplay_state::file_r)
{
	if(offset < m_file_data.size())
		return m_file_data[offset];
	return 0;
}

READ8_MEMBER(vgmplay_state::file_size_r)
{
	UINT32 size = m_file_data.size();
	return size >> (8*offset);
}

WRITE8_MEMBER(vgmplay_state::multipcm_bank_hi_a_w)
{
	if (offset & 1)
		m_multipcma_bank_l = (m_multipcma_bank_l & 0xff) | (data << 16);
	if (offset & 2)
		m_multipcma_bank_r = (m_multipcma_bank_r & 0xff) | (data << 16);
}

WRITE8_MEMBER(vgmplay_state::multipcm_bank_lo_a_w)
{
	if (offset & 1)
		m_multipcma_bank_l = (m_multipcma_bank_l & 0xff00) | data;
	if (offset & 2)
		m_multipcma_bank_r = (m_multipcma_bank_r & 0xff00) | data;

	m_multipcma->set_bank(m_multipcma_bank_l << 16, m_multipcma_bank_r << 16);
}

WRITE8_MEMBER(vgmplay_state::multipcm_bank_hi_b_w)
{
	if (offset & 1)
		m_multipcmb_bank_l = (m_multipcmb_bank_l & 0xff) | (data << 16);
	if (offset & 2)
		m_multipcmb_bank_r = (m_multipcmb_bank_r & 0xff) | (data << 16);
}

WRITE8_MEMBER(vgmplay_state::multipcm_bank_lo_b_w)
{
	if (offset & 1)
		m_multipcmb_bank_l = (m_multipcmb_bank_l & 0xff00) | data;
	if (offset & 2)
		m_multipcmb_bank_r = (m_multipcmb_bank_r & 0xff00) | data;

	m_multipcmb->set_bank(m_multipcmb_bank_l << 16, m_multipcmb_bank_r << 16);
}

static INPUT_PORTS_START( vgmplay )
INPUT_PORTS_END

static ADDRESS_MAP_START( file_map, AS_PROGRAM, 8, vgmplay_state )
	AM_RANGE(0x00000000, 0xffffffff) AM_READ(file_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( soundchips16_map, AS_IO16, 16, vgmplay_state )
	AM_RANGE(vgmplay_device::A_C352,         vgmplay_device::A_C352+0x7fff)   AM_DEVWRITE    ("c352",          c352_device, write)
ADDRESS_MAP_END

static ADDRESS_MAP_START( soundchips_map, AS_IO, 8, vgmplay_state )
	AM_RANGE(vgmplay_device::REG_SIZE,       vgmplay_device::REG_SIZE+3)      AM_READ(file_size_r)
	AM_RANGE(vgmplay_device::A_YM2612,       vgmplay_device::A_YM2612+3)      AM_DEVWRITE    ("ym2612",        ym2612_device, write)
	AM_RANGE(vgmplay_device::A_YM2151,       vgmplay_device::A_YM2151+1)      AM_DEVWRITE    ("ym2151",        ym2151_device, write)
	AM_RANGE(vgmplay_device::A_YM2413,       vgmplay_device::A_YM2413+1)      AM_DEVWRITE    ("ym2413",        ym2413_device, write)
	AM_RANGE(vgmplay_device::A_YM2203A,      vgmplay_device::A_YM2203A+1)     AM_DEVWRITE    ("ym2203a",       ym2203_device, write)
	AM_RANGE(vgmplay_device::A_YM2203B,      vgmplay_device::A_YM2203B+1)     AM_DEVWRITE    ("ym2203b",       ym2203_device, write)
	AM_RANGE(vgmplay_device::A_YM3526,       vgmplay_device::A_YM3526+1)      AM_DEVWRITE    ("ym3526",        ym3526_device, write)
	AM_RANGE(vgmplay_device::A_YM3812,       vgmplay_device::A_YM3812+1)      AM_DEVWRITE    ("ym3812",        ym3812_device, write)
	AM_RANGE(vgmplay_device::A_AY8910A,      vgmplay_device::A_AY8910A)       AM_DEVWRITE    ("ay8910a",       ay8910_device, data_w)
	AM_RANGE(vgmplay_device::A_AY8910A+1,    vgmplay_device::A_AY8910A+1)     AM_DEVWRITE    ("ay8910a",       ay8910_device, address_w)
	AM_RANGE(vgmplay_device::A_AY8910B,      vgmplay_device::A_AY8910B)       AM_DEVWRITE    ("ay8910b",       ay8910_device, data_w)
	AM_RANGE(vgmplay_device::A_AY8910B+1,    vgmplay_device::A_AY8910B+1)     AM_DEVWRITE    ("ay8910b",       ay8910_device, address_w)
//  AM_RANGE(vgmplay_device::A_SN76496+0,    vgmplay_device::A_SN76496+0)     AM_DEVWRITE    ("sn76496",       sn76496_device, stereo_w)
	AM_RANGE(vgmplay_device::A_SN76496+1,    vgmplay_device::A_SN76496+1)     AM_DEVWRITE    ("sn76496",       sn76496_device, write)
	AM_RANGE(vgmplay_device::A_K053260,      vgmplay_device::A_K053260+0x2f)  AM_DEVWRITE    ("k053260",       k053260_device, write)
	AM_RANGE(vgmplay_device::A_C6280,        vgmplay_device::A_C6280+0xf)     AM_DEVWRITE    ("c6280",         c6280_device, c6280_w)
	AM_RANGE(vgmplay_device::A_OKIM6295,     vgmplay_device::A_OKIM6295)      AM_DEVWRITE    ("okim6295",      okim6295_device, write)
	AM_RANGE(vgmplay_device::A_SEGAPCM,      vgmplay_device::A_SEGAPCM+0x7ff) AM_DEVWRITE    ("segapcm",       segapcm_device, sega_pcm_w)
	AM_RANGE(vgmplay_device::A_GAMEBOY,      vgmplay_device::A_GAMEBOY+0x16)  AM_DEVWRITE    ("dmg",           gameboy_sound_device, sound_w)
	AM_RANGE(vgmplay_device::A_GAMEBOY+0x20, vgmplay_device::A_GAMEBOY+0x2f)  AM_DEVWRITE    ("dmg",           gameboy_sound_device, wave_w)
	AM_RANGE(vgmplay_device::A_NESAPU,       vgmplay_device::A_NESAPU+0x1f)   AM_DEVWRITE    ("nescpu:nesapu", nesapu_device, write)
	AM_RANGE(vgmplay_device::A_NESRAM,       vgmplay_device::A_NESRAM+0xffff) AM_RAM AM_SHARE("nesapu_ram")
	AM_RANGE(vgmplay_device::A_MULTIPCMA,    vgmplay_device::A_MULTIPCMA+3)   AM_DEVWRITE    ("multipcma",     multipcm_device, write )
	AM_RANGE(vgmplay_device::A_MULTIPCMA+4,  vgmplay_device::A_MULTIPCMA+7)   AM_WRITE(multipcm_bank_hi_a_w)
	AM_RANGE(vgmplay_device::A_MULTIPCMA+8,  vgmplay_device::A_MULTIPCMA+11)  AM_WRITE(multipcm_bank_lo_a_w)
	AM_RANGE(vgmplay_device::A_MULTIPCMB,    vgmplay_device::A_MULTIPCMB+3)   AM_DEVWRITE    ("multipcmb",     multipcm_device, write )
	AM_RANGE(vgmplay_device::A_MULTIPCMB+4,  vgmplay_device::A_MULTIPCMB+7)   AM_WRITE(multipcm_bank_hi_b_w)
	AM_RANGE(vgmplay_device::A_MULTIPCMB+8,  vgmplay_device::A_MULTIPCMB+11)  AM_WRITE(multipcm_bank_lo_b_w)
	AM_RANGE(vgmplay_device::A_POKEYA,       vgmplay_device::A_POKEYA+0xf)    AM_DEVWRITE    ("pokeya",        pokey_device, write)
	AM_RANGE(vgmplay_device::A_POKEYB,       vgmplay_device::A_POKEYB+0xf)    AM_DEVWRITE    ("pokeyb",        pokey_device, write)
ADDRESS_MAP_END

static ADDRESS_MAP_START( segapcm_map, AS_0, 8, vgmplay_state )
	AM_RANGE(0, 0x1fffff) AM_DEVREAD("vgmplay", vgmplay_device, segapcm_rom_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( multipcma_map, AS_0, 8, vgmplay_state )
	AM_RANGE(0, 0x3fffff) AM_DEVREAD("vgmplay", vgmplay_device, multipcma_rom_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( multipcmb_map, AS_0, 8, vgmplay_state )
	AM_RANGE(0, 0x3fffff) AM_DEVREAD("vgmplay", vgmplay_device, multipcmb_rom_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( k053260_map, AS_0, 8, vgmplay_state )
	AM_RANGE(0, 0x1fffff) AM_DEVREAD("vgmplay", vgmplay_device, k053260_rom_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( okim6295_map, AS_0, 8, vgmplay_state )
	AM_RANGE(0, 0x3ffff) AM_DEVREAD("vgmplay", vgmplay_device, okim6295_rom_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( c352_map, AS_0, 8, vgmplay_state )
	AM_RANGE(0, 0xffffff) AM_DEVREAD("vgmplay", vgmplay_device, c352_rom_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( nescpu_map, AS_PROGRAM, 8, vgmplay_state )
	AM_RANGE(0, 0xffff) AM_RAM AM_SHARE("nesapu_ram")
ADDRESS_MAP_END

static ADDRESS_MAP_START( h6280_map, AS_PROGRAM, 8, vgmplay_state )
	AM_RANGE(0, 0xffff) AM_NOP
ADDRESS_MAP_END

static ADDRESS_MAP_START( h6280_io_map, AS_IO, 8, vgmplay_state )
	AM_RANGE(0, 3) AM_NOP
ADDRESS_MAP_END

static MACHINE_CONFIG_START( vgmplay, vgmplay_state )
	MCFG_CPU_ADD("vgmplay", VGMPLAY, 44100)
	MCFG_CPU_PROGRAM_MAP( file_map )
	MCFG_CPU_IO_MAP( soundchips_map )
	MCFG_CPU_IO16_MAP( soundchips16_map )

	MCFG_DEVICE_ADD("file", BITBANGER, 0)

	MCFG_SPEAKER_STANDARD_STEREO("lspeaker", "rspeaker")

	MCFG_SOUND_ADD("ym2612", YM2612, 7670454)
	MCFG_SOUND_ROUTE(0, "lspeaker", 1)
	MCFG_SOUND_ROUTE(1, "rspeaker", 1)

	MCFG_SOUND_ADD("ym2151", YM2151, 3579545)
	MCFG_SOUND_ROUTE(0, "lspeaker", 1)
	MCFG_SOUND_ROUTE(1, "rspeaker", 1)

	MCFG_SOUND_ADD("ym2413", YM2413, 3579545)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "lspeaker", 1)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "rspeaker", 1)

	MCFG_SOUND_ADD("sn76496", SN76496, 3579545)
	MCFG_SOUND_ROUTE(0, "lspeaker", 0.5)
	MCFG_SOUND_ROUTE(0, "rspeaker", 0.5)

	MCFG_SOUND_ADD("segapcm", SEGAPCM, 4000000)
	MCFG_SEGAPCM_BANK(BANK_512) // Should be configurable for yboard...
	MCFG_DEVICE_ADDRESS_MAP(AS_0, segapcm_map)
	MCFG_SOUND_ROUTE(0, "lspeaker", 1)
	MCFG_SOUND_ROUTE(1, "rspeaker", 1)

	MCFG_SOUND_ADD("multipcma", MULTIPCM, 8000000)
	MCFG_DEVICE_ADDRESS_MAP(AS_0, multipcma_map)
	MCFG_SOUND_ROUTE(0, "lspeaker", 1)
	MCFG_SOUND_ROUTE(1, "rspeaker", 1)

	MCFG_SOUND_ADD("multipcmb", MULTIPCM, 8000000)
	MCFG_DEVICE_ADDRESS_MAP(AS_0, multipcmb_map)
	MCFG_SOUND_ROUTE(0, "lspeaker", 1)
	MCFG_SOUND_ROUTE(1, "rspeaker", 1)

	MCFG_SOUND_ADD("dmg", GAMEBOY, 0)
	MCFG_SOUND_ROUTE(0, "lspeaker", 1)
	MCFG_SOUND_ROUTE(1, "rspeaker", 1)

	MCFG_SOUND_ADD("ay8910a", AY8910, 1500000)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "lspeaker", 0.33)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "rspeaker", 0.33)

	MCFG_SOUND_ADD("ay8910b", AY8910, 1500000)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "lspeaker", 0.33)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "rspeaker", 0.33)

	MCFG_SOUND_ADD("ym2203a", YM2203, 4000000)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "lspeaker", 0.25)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "rspeaker", 0.25)

	MCFG_SOUND_ADD("ym2203b", YM2203, 4000000)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "lspeaker", 0.25)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "rspeaker", 0.25)

	MCFG_SOUND_ADD("ym3526", YM3526, 4000000)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "lspeaker", 0.5)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "rspeaker", 0.5)

	MCFG_SOUND_ADD("ym3812", YM3812, 4000000)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "lspeaker", 0.50)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "rspeaker", 0.50)

	MCFG_CPU_ADD("nescpu", N2A03, 1000000)
	MCFG_CPU_PROGRAM_MAP(nescpu_map)
	MCFG_DEVICE_DISABLE()

	MCFG_DEVICE_MODIFY("nescpu:nesapu")
	MCFG_SOUND_ROUTES_RESET()
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, ":lspeaker", 0.50)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, ":rspeaker", 0.50)

	MCFG_CPU_ADD("h6280", H6280, 1000000)
	MCFG_CPU_PROGRAM_MAP(h6280_map)
	MCFG_CPU_IO_MAP(h6280_io_map)
	MCFG_DEVICE_DISABLE()

	MCFG_SOUND_ADD("c6280", C6280, 3579545)
	MCFG_C6280_CPU("h6280")
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "lspeaker", 1)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "rspeaker", 1)

	MCFG_K053260_ADD("k053260", 3579545)
	MCFG_DEVICE_ADDRESS_MAP(AS_0, k053260_map)
	MCFG_SOUND_ROUTE(0, "lspeaker", 1)
	MCFG_SOUND_ROUTE(1, "rspeaker", 1)

	MCFG_SOUND_ADD("pokeya", POKEY, 1789772)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "lspeaker", 0.5)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "rspeaker", 0.5)

	MCFG_SOUND_ADD("pokeyb", POKEY, 1789772)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "lspeaker", 0.5)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "rspeaker", 0.5)

	MCFG_C352_ADD("c352", 1000000, 288)
	MCFG_DEVICE_ADDRESS_MAP(AS_0, c352_map)
	MCFG_SOUND_ROUTE(0, "lspeaker", 1)
	MCFG_SOUND_ROUTE(1, "rspeaker", 1)

	MCFG_OKIM6295_ADD("okim6295", 1000000, OKIM6295_PIN7_HIGH)
	MCFG_DEVICE_ADDRESS_MAP(AS_0, okim6295_map)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "lspeaker", 0.25)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "rspeaker", 0.25)
MACHINE_CONFIG_END

ROM_START( vgmplay )
ROM_END

CONS( 2016, vgmplay, 0, 0, vgmplay, vgmplay, driver_device, 0, "MAME", "VGM player", 0)

