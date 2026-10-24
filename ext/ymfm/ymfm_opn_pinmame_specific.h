#include "memory.h"
#include "timer.h"
#include <algorithm>
#include <cstring>

// PinMAME glue code for the ymfm based OPN (YM2203), FM and SSG parts, used by 2203intf.c
// with YM2203_YMFM set to 1 there (the old fm.c/ay8910.c cores are used otherwise)

extern "C" void* ymfm_ym2203_create(int baseindex, void(*irqhandler)(int irq), double baseclock, void(*callback)(int param),
	mem_read_handler portAread, mem_read_handler portBread, mem_write_handler portAwrite, mem_write_handler portBwrite);
extern "C" void ymfm_ym2203_destroy(void* obj);
extern "C" void ymfm_ym2203_reset(void* obj);
extern "C" void ymfm_ym2203_callback(void* obj, int param);
extern "C" void ymfm_ym2203_invalidate_caches(void* obj);
extern "C" uint8_t ymfm_ym2203_read(void* obj, uint32_t offset);
extern "C" void ymfm_ym2203_write(void* obj, uint32_t offset, uint8_t data);
extern "C" void ymfm_ym2203_generate(void* obj, int16_t* fm, int32_t* ssg, uint32_t numsamples);

// maps the chip's timer/IRQ callbacks onto PinMAME's timer.c and the driver supplied IRQ handler,
// and the two SSG I/O ports onto the driver supplied port handlers
class ymfm_interface_opn : public ymfm::ymfm_interface
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

	// busy flag of the status register, as MAME (ym_generic_device) and the old fm.c core do
	virtual void ymfm_set_busy_end(uint32_t clocks) override { busy_end = timer_get_time() + clocks/baseclock; }
	virtual bool ymfm_is_busy() override { return timer_get_time() < busy_end; }

	virtual uint8_t ymfm_external_read(ymfm::access_class type, uint32_t address) override
	{
		if (type == ymfm::ACCESS_IO && port_r[address & 1])
			return (*port_r[address & 1])(0);
		return 0;
	}

	virtual void ymfm_external_write(ymfm::access_class type, uint32_t address, uint8_t data) override
	{
		if (type == ymfm::ACCESS_IO && port_w[address & 1])
			(*port_w[address & 1])(0, data);
	}

	void set_irqhandler(void(*handler)(int irq)) { irqhandler = handler; }
	void set_clock(double clock) { baseclock = clock; }
	void set_baseindex(int b) { baseindex = b; }
	void set_ports(mem_read_handler ar, mem_read_handler br, mem_write_handler aw, mem_write_handler bw)
	{
		port_r[0] = ar; port_r[1] = br;
		port_w[0] = aw; port_w[1] = bw;
	}
	void set_timercallback(void(*callback)(int param))
	{
		for (int tnum = 0; tnum < 2; tnum++)
			m_timer[tnum] = timer_alloc(callback);
	}

	void timercallback(int param) { m_engine->engine_timer_expired(param); }

private:
	void(*irqhandler)(int irq);		/* IRQ function handler */
	mem_read_handler port_r[2];		/* SSG I/O port A/B input */
	mem_write_handler port_w[2];	/* SSG I/O port A/B output */
	mame_timer* m_timer[2];			/* timer A/B */
	double baseclock;
	int baseindex;					/* (chip index)*2, added to tnum when scheduling */
	double busy_end = 0.;			/* end of the busy period after a write */
};

struct ymfm_ym2203
{
	ymfm_ym2203(int baseindex, void(*irqhandler)(int irq), double baseclock, void(*callback)(int param),
		mem_read_handler ar, mem_read_handler br, mem_write_handler aw, mem_write_handler bw)
	{
		intf.set_irqhandler(irqhandler);
		intf.set_clock(baseclock);
		intf.set_baseindex(baseindex);
		intf.set_ports(ar, br, aw, bw);
		intf.set_timercallback(callback);
		chip = new ymfm::ym2203(intf);
		// same as MAME: output at clock/12, FM repeated, SSG resampled to that rate
		chip->set_fidelity(ymfm::OPN_FIDELITY_MED);
		reset();
	}
	~ymfm_ym2203() { delete chip; }

	void reset()
	{
		chip->reset();
		address = 0;
		memset(ssg_regs, 0, sizeof(ssg_regs));
		memset(ssg_center, 0, sizeof(ssg_center));
		memset(ssg_last, 0, sizeof(ssg_last));
	}

	void write(uint32_t offset, uint8_t data)
	{
		// shadow the SSG amplitude registers, see generate()
		if ((offset & 1) == 0)
			address = data;
		else if (address < 0x10)
			ssg_regs[address] = data;
		chip->write(offset, data);
	}

	ymfm::ym2203* chip;
	ymfm_interface_opn intf;
	uint8_t address = 0;
	uint8_t ssg_regs[16] = {};
	int32_t ssg_center[3] = {};		/* half the level of each SSG channel when it is high */
	int32_t ssg_last[3] = {};		/* previous output of each SSG channel */
};

// C-wirings for 2203intf.c

void* ymfm_ym2203_create(int baseindex, void(*irqhandler)(int irq), double baseclock, void(*callback)(int param),
	mem_read_handler portAread, mem_read_handler portBread, mem_write_handler portAwrite, mem_write_handler portBwrite)
{
	return new ymfm_ym2203(baseindex, irqhandler, baseclock, callback, portAread, portBread, portAwrite, portBwrite);
}

void ymfm_ym2203_destroy(void* obj) { if (obj) delete (ymfm_ym2203*)obj; }
void ymfm_ym2203_reset(void* obj) { ((ymfm_ym2203*)obj)->reset(); }
void ymfm_ym2203_callback(void* obj, int param) { ((ymfm_ym2203*)obj)->intf.timercallback(param); }
void ymfm_ym2203_invalidate_caches(void* obj) { ((ymfm_ym2203*)obj)->chip->invalidate_caches(); }
uint8_t ymfm_ym2203_read(void* obj, uint32_t offset) { return ((ymfm_ym2203*)obj)->chip->read(offset); }
void ymfm_ym2203_write(void* obj, uint32_t offset, uint8_t data) { ((ymfm_ym2203*)obj)->write(offset, data); }

// fm gets the FM output, ssg the sum of the three SSG channels. These are unipolar (0 when low, the
// channel's level when high): each one is centred around 0 by removing half its level, as ay8910.c
// does, so that what is left for the DC filter in 2203intf.c is small and a note's first edge does
// not overshoot. The level is the last non-zero output seen on two samples in a row, or 0 once the
// channel's amplitude is set to 0: ymfm resamples the SSG to the output rate, so the sample on an edge
// is a mix of the low and high states, and taking half of that as the level would shift the whole
// following half period (the tone came out uneven, up to 3dB too quiet on the RMS)
void ymfm_ym2203_generate(void* obj, int16_t* fm, int32_t* ssg, uint32_t numsamples)
{
	constexpr uint32_t MAX_SAMPLES = 256;
	static ymfm::ym2203::output_data o[MAX_SAMPLES];
	ymfm_ym2203* const self = (ymfm_ym2203*)obj;
	ymfm::ym2203* const chip = self->chip;

	for (uint32_t sampindex = 0; sampindex < numsamples; sampindex += MAX_SAMPLES)
	{
		const uint32_t cursamples = std::min(numsamples - sampindex, MAX_SAMPLES);
		chip->generate(o, cursamples);

		for (uint32_t index = 0; index < cursamples; index++)
		{
			int32_t sum = 0;
			fm[sampindex + index] = (int16_t)o[index].data[0];
			for (int ch = 0; ch < 3; ch++)
			{
				const int32_t v = o[index].data[1 + ch];
				if (v > 0 && v == self->ssg_last[ch])
					self->ssg_center[ch] = v / 2;
				else if ((self->ssg_regs[8 + ch] & 0x1f) == 0)
					self->ssg_center[ch] = 0;
				self->ssg_last[ch] = v;
				sum += v - self->ssg_center[ch];
			}
			ssg[sampindex + index] = sum;
		}
	}
}
