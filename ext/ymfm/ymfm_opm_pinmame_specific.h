#include "memory.h"
#include "timer.h"
#include "../vgm/vgmwrite.h"

extern "C" void* ymfm_ym2151_create(void(*irqhandler)(int irq), mem_write_handler porthandler, double baseclock, void(*callback)(int param));
extern "C" void ymfm_ym2151_destroy(void* obj);

extern "C" void ymfm_ym2151_reset(void* obj);

extern "C" void ymfm_ym2151_callback(void* obj, int param);

extern "C" void ymfm_ym2151_invalidate_caches(void* obj);

extern "C" uint8_t ymfm_ym2151_read(void* obj, uint32_t offset);

extern "C" void ymfm_ym2151_write(void* obj, uint32_t offset, uint8_t data);

extern "C" void ymfm_ym2151_generate(void* obj, int16_t** output, uint32_t numsamples);

// classes

class ymfm_interface_ym2151 : public ymfm::ymfm_interface // need to derive a new class from that, to override essential functions and pass in stuff
{
public:

	// the chip implementation calls this when one of the two internal timers
	// has changed state; our responsibility is to arrange to call the engine's
	// engine_timer_expired() method after the provided number of clocks; if
	// duration_in_clocks is negative, we should cancel any outstanding timers
	virtual void ymfm_set_timer(uint32_t tnum, int32_t duration_in_clocks) override
	{
		if (duration_in_clocks >= 0)
			timer_adjust(m_timer[tnum], duration_in_clocks/baseclock/*attotime::from_ticks(duration_in_clocks, device_t::clock())*/, tnum, TIME_NEVER); //!! correct like this ???
		else
			timer_enable(m_timer[tnum],0);
	}

	virtual void ymfm_update_irq(bool asserted) override { if (irqhandler) (*irqhandler)(asserted); }

	// the chip implementation calls this whenever data is written outside
	// of the chip; our responsibility is to pass the written data on to any consumers
	virtual void ymfm_external_write(ymfm::access_class type, uint32_t address, uint8_t data) override
	{
		if (type == ymfm::ACCESS_IO && porthandler)
			(*porthandler)(0, data);
	}

	void set_irqhandler(void(*handler)(int irq)) { irqhandler = handler; }
	void set_portwritehandler(mem_write_handler handler) { porthandler = handler; }
	void set_clock(double clock) { baseclock = clock; }
	void set_timercallback(void(*callback)(int param))
	{
		// allocate our timers
		for (int tnum = 0; tnum < 2; tnum++)
			m_timer[tnum] = timer_alloc(callback);
	}

	void timercallback(int param) { m_engine->engine_timer_expired(param); }

private:
	void(*irqhandler)(int irq);		/* IRQ function handler */
	mem_write_handler porthandler;	/* port write function handler */
	mame_timer* m_timer[2];			/* timer A/B */
	double baseclock;
};

class ymfm_ym2151
{
public:
	ymfm_ym2151(void(*irqhandler)(int irq), mem_write_handler porthandler, double baseclock, void(*callback)(int param))
	{
		intf.set_irqhandler(irqhandler);
		intf.set_portwritehandler(porthandler);
		intf.set_clock(baseclock);
		intf.set_timercallback(callback);
	}
	~ymfm_ym2151() {}

	ymfm::ym2151* chip;
	ymfm_interface_ym2151 intf;
};

// C-wirings for 2151intf.c

void* ymfm_ym2151_create(void(*irqhandler)(int irq), mem_write_handler porthandler, double baseclock, void(*callback)(int param))
{	
	ymfm_ym2151* obj = new ymfm_ym2151(irqhandler,porthandler,baseclock,callback);
	obj->chip = new ymfm::ym2151(obj->intf);
	obj->chip->reset();
	return obj;
}

void ymfm_ym2151_destroy(void* obj) { if (obj) { delete ((ymfm_ym2151*)obj)->chip; delete (ymfm_ym2151*)obj; } }

void ymfm_ym2151_reset(void* obj) { ((ymfm_ym2151*)obj)->chip->reset(); }

void ymfm_ym2151_callback(void* obj, int param) { ((ymfm_ym2151*)obj)->intf.timercallback(param); }

uint32_t ymfm_ym2151_sample_rate(void* obj, uint32_t input_clock) { return ((ymfm_ym2151*)obj)->chip->sample_rate(input_clock); }
void ymfm_ym2151_invalidate_caches(void* obj) { ((ymfm_ym2151*)obj)->chip->invalidate_caches(); }

uint8_t ymfm_ym2151_read_status(void* obj) { return ((ymfm_ym2151*)obj)->chip->read_status(); }
uint8_t ymfm_ym2151_read(void* obj, uint32_t offset) { return ((ymfm_ym2151*)obj)->chip->read(offset); }

void ymfm_ym2151_write(void* obj, uint32_t offset, uint8_t data) { ((ymfm_ym2151*)obj)->chip->write(offset,data); }

void ymfm_ym2151_generate(void* obj, int16_t** output, uint32_t numsamples)
{
#if 0 // allocate mem
	ymfm::ym2151::output_data* const __restrict o = new ymfm::ym2151::output_data[numsamples];
	((ymfm_ym2151*)obj)->chip->generate(o, numsamples);
	for (unsigned int i = 0; i < numsamples; ++i)
	{
		output[0][i] = (int16_t)o[i].data[0];
		output[1][i] = (int16_t)o[i].data[1];
	}
	delete[] o;
#else // use 'large' chunk
	constexpr unsigned int MAX_SAMPLES = 256;
	static ymfm::ym2151::output_data o[MAX_SAMPLES];

	for (uint32_t sampindex = 0; sampindex < numsamples; sampindex += MAX_SAMPLES)
	{
		const uint32_t cursamples = std::min(numsamples - sampindex, MAX_SAMPLES);
		((ymfm_ym2151*)obj)->chip->generate(o, cursamples);

		for (uint32_t index = 0; index < cursamples; index++)
		{
			output[0][sampindex + index] = (int16_t)o[index].data[0];
			output[1][sampindex + index] = (int16_t)o[index].data[1];
		}
	}
#endif
}
