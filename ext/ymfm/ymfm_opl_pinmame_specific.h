#include "memory.h"
#include "timer.h"
#include <algorithm>

// Unified PinMAME glue code for all ymfm based OPLs (OPL/YM3526, OPL2/YM3812, OPL3/YMF262, MSX-AUDIO/Y8950),
// so a single set of C entry points can drive any of the OPL variants, selected at create() time via the chip-type enum below

enum
{
	YMFM_OPL_YM3526 = 0,	// OPL
	YMFM_OPL_YM3812 = 1,	// OPL2
	YMFM_OPL_YMF262 = 2,	// OPL3
	YMFM_OPL_Y8950 = 3		// MSX-AUDIO (only used with Y8950_YMFM set to 1 in 3812intf.c)
};

extern "C" void* ymfm_opl_create(int chiptype, int baseindex, void(*irqhandler)(int irq), double baseclock, void(*callback)(int param));
extern "C" void ymfm_opl_destroy(void* obj);
extern "C" void ymfm_opl_reset(void* obj);
extern "C" void ymfm_opl_callback(void* obj, int param);
extern "C" void ymfm_opl_invalidate_caches(void* obj);
extern "C" uint8_t ymfm_opl_read(void* obj, uint32_t offset);
extern "C" void ymfm_opl_write(void* obj, uint32_t offset, uint8_t data);
extern "C" void ymfm_opl_generate(void* obj, int16_t** output, uint32_t numsamples, uint32_t numch);
extern "C" void ymfm_opl_set_external(void* obj, const uint8_t* rom, uint32_t romsize, int chip,
	mem_read_handler io_r, mem_write_handler io_w, mem_read_handler kbd_r, mem_write_handler kbd_w);

// ymfm interface: maps the chip's timer/IRQ callbacks onto PinMAME's timer.c and the driver-supplied IRQ handler
class ymfm_interface_opl : public ymfm::ymfm_interface
{
public:
	virtual void ymfm_set_timer(uint32_t tnum, int32_t duration_in_clocks) override
	{
		// pass baseindex+tnum so the (single / shared) C timer callback can map
		// the firing back to the right chip instance as well as the right timer
		if (duration_in_clocks >= 0)
			timer_adjust(m_timer[tnum], duration_in_clocks/baseclock, baseindex + tnum, TIME_NEVER);
		else
			timer_enable(m_timer[tnum], 0);
	}

	virtual void ymfm_update_irq(bool asserted) override { if (irqhandler) (*irqhandler)(asserted ? 1 : 0); }

	// Y8950 only: ADPCM-B sample ROM, I/O port (0) and keyboard port (1)
	virtual uint8_t ymfm_external_read(ymfm::access_class type, uint32_t address) override
	{
		if (type == ymfm::ACCESS_ADPCM_B)
		{
			if (romsize == 0)
				return 0;
			// a power of two sized ROM is mirrored over the address space
			if ((romsize & (romsize - 1)) == 0)
				return rom[address & (romsize - 1)];
			return (address < romsize) ? rom[address] : 0;
		}
		if (type == ymfm::ACCESS_IO && io_r[address & 1])
			return (*io_r[address & 1])(chip);
		return 0;
	}

	virtual void ymfm_external_write(ymfm::access_class type, uint32_t address, uint8_t data) override
	{
		// writes to the sample ROM are ignored
		if (type == ymfm::ACCESS_IO && io_w[address & 1])
			(*io_w[address & 1])(chip, data);
	}

	void set_external(const uint8_t* r, uint32_t size, int c, mem_read_handler ior, mem_write_handler iow, mem_read_handler kbdr, mem_write_handler kbdw)
	{
		rom = r; romsize = r ? size : 0; chip = c;
		io_r[0] = ior; io_r[1] = kbdr;
		io_w[0] = iow; io_w[1] = kbdw;
	}

	void set_irqhandler(void(*handler)(int irq)) { irqhandler = handler; }
	void set_clock(double clock) { baseclock = clock; }
	void set_baseindex(int b) { baseindex = b; }
	void set_timercallback(void(*callback)(int param))
	{
		for (int tnum = 0; tnum < 2; tnum++)
			m_timer[tnum] = timer_alloc(callback);
	}

	void timercallback(int param) { m_engine->engine_timer_expired(param); }

private:
	void(*irqhandler)(int irq);		/* IRQ function handler */
	mame_timer* m_timer[2];			/* timer A/B */
	double baseclock;
	int baseindex;					/* (chip index)*2, added to tnum when scheduling */
	const uint8_t* rom = nullptr;	/* Y8950 ADPCM-B sample ROM */
	uint32_t romsize = 0;
	int chip = 0;					/* passed to the port handlers, as the old core does */
	mem_read_handler io_r[2] = { nullptr, nullptr };
	mem_write_handler io_w[2] = { nullptr, nullptr };
};

// Type-erased base so the C wrappers can treat all OPL variants uniformly
struct ymfm_opl_base
{
	virtual ~ymfm_opl_base() {}
	virtual void reset() = 0;
	virtual void invalidate_caches() = 0;
	virtual uint8_t read(uint32_t offset) = 0;
	virtual void write(uint32_t offset, uint8_t data) = 0;
	virtual void generate(int16_t** output, uint32_t numsamples, uint32_t numch) = 0;
	ymfm_interface_opl intf;
};

template<class Chip>
struct ymfm_opl_impl : public ymfm_opl_base
{
	ymfm_opl_impl(int baseindex, void(*irqhandler)(int irq), double baseclock, void(*callback)(int param), uint32_t _chip_outputs)
	{
		intf.set_irqhandler(irqhandler);
		intf.set_clock(baseclock);
		intf.set_baseindex(baseindex);
		intf.set_timercallback(callback);
		chip = new Chip(intf);
		chip->reset();
		chip_outputs = _chip_outputs;
	}
	virtual ~ymfm_opl_impl() { delete chip; }

	virtual void reset() override { chip->reset(); }
	virtual void invalidate_caches() override { chip->invalidate_caches(); }
	virtual uint8_t read(uint32_t offset) override { return chip->read(offset); }
	virtual void write(uint32_t offset, uint8_t data) override { chip->write(offset, data); }

	virtual void generate(int16_t** output, uint32_t numsamples, uint32_t numch) override
	{
		constexpr uint32_t MAX_SAMPLES = 256;
		static typename Chip::output_data o[MAX_SAMPLES];

		const uint32_t outputs = std::min<uint32_t>(numch, chip_outputs/*Chip::OUTPUTS*/); //!! workaround for xpinmame linux build

		for (uint32_t sampindex = 0; sampindex < numsamples; sampindex += MAX_SAMPLES)
		{
			const uint32_t cursamples = std::min(numsamples - sampindex, MAX_SAMPLES);
			chip->generate(o, cursamples);

			for (uint32_t ch = 0; ch < numch; ch++)
			{
				const uint32_t ch2 = ch < outputs ? ch : (outputs - 1);
				for (uint32_t index = 0; index < cursamples; index++)
					output[ch][sampindex + index] = (int16_t)o[index].data[ch2];
			}
		}
	}

	Chip* chip;
	uint32_t chip_outputs;
};

// C-wirings for 3812intf.c / 262intf.c (Y8950 also in 3812intf.c)

void* ymfm_opl_create(int chiptype, int baseindex, void(*irqhandler)(int irq), double baseclock, void(*callback)(int param))
{
	switch (chiptype)
	{
		case YMFM_OPL_YM3526: return new ymfm_opl_impl<ymfm::ym3526>(baseindex, irqhandler, baseclock, callback, ymfm::ym3526::OUTPUTS);
		case YMFM_OPL_YM3812: return new ymfm_opl_impl<ymfm::ym3812>(baseindex, irqhandler, baseclock, callback, ymfm::ym3812::OUTPUTS);
		case YMFM_OPL_YMF262: return new ymfm_opl_impl<ymfm::ymf262>(baseindex, irqhandler, baseclock, callback, ymfm::ymf262::OUTPUTS);
		case YMFM_OPL_Y8950:  return new ymfm_opl_impl<ymfm::y8950>(baseindex, irqhandler, baseclock, callback, ymfm::y8950::OUTPUTS);
		default: return nullptr;
	}
}

void ymfm_opl_destroy(void* obj) { if (obj) delete (ymfm_opl_base*)obj; }
void ymfm_opl_reset(void* obj) { ((ymfm_opl_base*)obj)->reset(); }
void ymfm_opl_callback(void* obj, int param) { ((ymfm_opl_base*)obj)->intf.timercallback(param); }
void ymfm_opl_invalidate_caches(void* obj) { ((ymfm_opl_base*)obj)->invalidate_caches(); }
uint8_t ymfm_opl_read(void* obj, uint32_t offset) { return ((ymfm_opl_base*)obj)->read(offset); }
void ymfm_opl_write(void* obj, uint32_t offset, uint8_t data) { ((ymfm_opl_base*)obj)->write(offset, data); }
void ymfm_opl_generate(void* obj, int16_t** output, uint32_t numsamples, uint32_t numch) { ((ymfm_opl_base*)obj)->generate(output, numsamples, numch); }
void ymfm_opl_set_external(void* obj, const uint8_t* rom, uint32_t romsize, int chip,
	mem_read_handler io_r, mem_write_handler io_w, mem_read_handler kbd_r, mem_write_handler kbd_w)
{
	((ymfm_opl_base*)obj)->intf.set_external(rom, romsize, chip, io_r, io_w, kbd_r, kbd_w);
}
