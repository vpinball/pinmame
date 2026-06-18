#include "memory.h"
#include "timer.h"
#include <algorithm>

// Unified PinMAME glue code for all ymfm based OPLs (OPL/YM3526, OPL2/YM3812, OPL3/YMF262),
// so a single set of C entry points can drive any of the OPL variants, selected at create() time via the chip-type enum below

enum
{
	YMFM_OPL_YM3526 = 0,	// OPL
	YMFM_OPL_YM3812 = 1,	// OPL2
	YMFM_OPL_YMF262 = 2		// OPL3
};

extern "C" void* ymfm_opl_create(int chiptype, int baseindex, void(*irqhandler)(int irq), double baseclock, void(*callback)(int param));
extern "C" void ymfm_opl_destroy(void* obj);
extern "C" void ymfm_opl_reset(void* obj);
extern "C" void ymfm_opl_callback(void* obj, int param);
extern "C" void ymfm_opl_invalidate_caches(void* obj);
extern "C" uint8_t ymfm_opl_read(void* obj, uint32_t offset);
extern "C" void ymfm_opl_write(void* obj, uint32_t offset, uint8_t data);
extern "C" void ymfm_opl_generate(void* obj, int16_t** output, uint32_t numsamples, uint32_t numch);

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
	ymfm_opl_impl(int baseindex, void(*irqhandler)(int irq), double baseclock, void(*callback)(int param))
	{
		intf.set_irqhandler(irqhandler);
		intf.set_clock(baseclock);
		intf.set_baseindex(baseindex);
		intf.set_timercallback(callback);
		chip = new Chip(intf);
		chip->reset();
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

		const uint32_t outputs = std::min<uint32_t>(numch, chip->OUTPUTS); // Chip::OUTPUTS

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
};

// C-wirings for 3812intf.c / 262intf.c

void* ymfm_opl_create(int chiptype, int baseindex, void(*irqhandler)(int irq), double baseclock, void(*callback)(int param))
{
	switch (chiptype)
	{
		case YMFM_OPL_YM3526: return new ymfm_opl_impl<ymfm::ym3526>(baseindex, irqhandler, baseclock, callback);
		case YMFM_OPL_YM3812: return new ymfm_opl_impl<ymfm::ym3812>(baseindex, irqhandler, baseclock, callback);
		case YMFM_OPL_YMF262: return new ymfm_opl_impl<ymfm::ymf262>(baseindex, irqhandler, baseclock, callback);
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
