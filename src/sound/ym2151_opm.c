// license:LGPL
// copyright-holders:Nuke.YKT

/* Nuked OPM
 * Copyright (C) 2020, 2026 Nuke.YKT
 *
 * This file is part of Nuked OPM.
 *
 * Nuked OPM is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 2.1
 * of the License, or (at your option) any later version.
 *
 * Nuked OPM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Nuked OPM. If not, see <https://www.gnu.org/licenses/>.
 *
 *  Nuked OPM emulator.
 *  Thanks:
 *      John McMaster(siliconpr0n.org):
 *          YM2151 and other FM chip decaps and die shots.
 *      gtr3qq (https://github.com/gtr3qq):
 *          YM2164 decap
 *
 * version: 1.0
 */
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "ym2151_opm.h"

#ifdef PINMAME
 #include "../ext/vgm/vgmwrite.h"
#endif

enum {
    eg_num_attack = 0,
    eg_num_decay = 1,
    eg_num_sustain = 2,
    eg_num_release = 3
};

/* logsin table */
static const uint16_t logsinrom[256] = {
    0x859, 0x6c3, 0x607, 0x58b, 0x52e, 0x4e4, 0x4a6, 0x471,
    0x443, 0x41a, 0x3f5, 0x3d3, 0x3b5, 0x398, 0x37e, 0x365,
    0x34e, 0x339, 0x324, 0x311, 0x2ff, 0x2ed, 0x2dc, 0x2cd,
    0x2bd, 0x2af, 0x2a0, 0x293, 0x286, 0x279, 0x26d, 0x261,
    0x256, 0x24b, 0x240, 0x236, 0x22c, 0x222, 0x218, 0x20f,
    0x206, 0x1fd, 0x1f5, 0x1ec, 0x1e4, 0x1dc, 0x1d4, 0x1cd,
    0x1c5, 0x1be, 0x1b7, 0x1b0, 0x1a9, 0x1a2, 0x19b, 0x195,
    0x18f, 0x188, 0x182, 0x17c, 0x177, 0x171, 0x16b, 0x166,
    0x160, 0x15b, 0x155, 0x150, 0x14b, 0x146, 0x141, 0x13c,
    0x137, 0x133, 0x12e, 0x129, 0x125, 0x121, 0x11c, 0x118,
    0x114, 0x10f, 0x10b, 0x107, 0x103, 0x0ff, 0x0fb, 0x0f8,
    0x0f4, 0x0f0, 0x0ec, 0x0e9, 0x0e5, 0x0e2, 0x0de, 0x0db,
    0x0d7, 0x0d4, 0x0d1, 0x0cd, 0x0ca, 0x0c7, 0x0c4, 0x0c1,
    0x0be, 0x0bb, 0x0b8, 0x0b5, 0x0b2, 0x0af, 0x0ac, 0x0a9,
    0x0a7, 0x0a4, 0x0a1, 0x09f, 0x09c, 0x099, 0x097, 0x094,
    0x092, 0x08f, 0x08d, 0x08a, 0x088, 0x086, 0x083, 0x081,
    0x07f, 0x07d, 0x07a, 0x078, 0x076, 0x074, 0x072, 0x070,
    0x06e, 0x06c, 0x06a, 0x068, 0x066, 0x064, 0x062, 0x060,
    0x05e, 0x05c, 0x05b, 0x059, 0x057, 0x055, 0x053, 0x052,
    0x050, 0x04e, 0x04d, 0x04b, 0x04a, 0x048, 0x046, 0x045,
    0x043, 0x042, 0x040, 0x03f, 0x03e, 0x03c, 0x03b, 0x039,
    0x038, 0x037, 0x035, 0x034, 0x033, 0x031, 0x030, 0x02f,
    0x02e, 0x02d, 0x02b, 0x02a, 0x029, 0x028, 0x027, 0x026,
    0x025, 0x024, 0x023, 0x022, 0x021, 0x020, 0x01f, 0x01e,
    0x01d, 0x01c, 0x01b, 0x01a, 0x019, 0x018, 0x017, 0x017,
    0x016, 0x015, 0x014, 0x014, 0x013, 0x012, 0x011, 0x011,
    0x010, 0x00f, 0x00f, 0x00e, 0x00d, 0x00d, 0x00c, 0x00c,
    0x00b, 0x00a, 0x00a, 0x009, 0x009, 0x008, 0x008, 0x007,
    0x007, 0x007, 0x006, 0x006, 0x005, 0x005, 0x005, 0x004,
    0x004, 0x004, 0x003, 0x003, 0x003, 0x002, 0x002, 0x002,
    0x002, 0x001, 0x001, 0x001, 0x001, 0x001, 0x001, 0x001,
    0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000
};

/* exp table */
static const uint16_t exprom[256] = {
    0x7fa, 0x7f5, 0x7ef, 0x7ea, 0x7e4, 0x7df, 0x7da, 0x7d4,
    0x7cf, 0x7c9, 0x7c4, 0x7bf, 0x7b9, 0x7b4, 0x7ae, 0x7a9,
    0x7a4, 0x79f, 0x799, 0x794, 0x78f, 0x78a, 0x784, 0x77f,
    0x77a, 0x775, 0x770, 0x76a, 0x765, 0x760, 0x75b, 0x756,
    0x751, 0x74c, 0x747, 0x742, 0x73d, 0x738, 0x733, 0x72e,
    0x729, 0x724, 0x71f, 0x71a, 0x715, 0x710, 0x70b, 0x706,
    0x702, 0x6fd, 0x6f8, 0x6f3, 0x6ee, 0x6e9, 0x6e5, 0x6e0,
    0x6db, 0x6d6, 0x6d2, 0x6cd, 0x6c8, 0x6c4, 0x6bf, 0x6ba,
    0x6b5, 0x6b1, 0x6ac, 0x6a8, 0x6a3, 0x69e, 0x69a, 0x695,
    0x691, 0x68c, 0x688, 0x683, 0x67f, 0x67a, 0x676, 0x671,
    0x66d, 0x668, 0x664, 0x65f, 0x65b, 0x657, 0x652, 0x64e,
    0x649, 0x645, 0x641, 0x63c, 0x638, 0x634, 0x630, 0x62b,
    0x627, 0x623, 0x61e, 0x61a, 0x616, 0x612, 0x60e, 0x609,
    0x605, 0x601, 0x5fd, 0x5f9, 0x5f5, 0x5f0, 0x5ec, 0x5e8,
    0x5e4, 0x5e0, 0x5dc, 0x5d8, 0x5d4, 0x5d0, 0x5cc, 0x5c8,
    0x5c4, 0x5c0, 0x5bc, 0x5b8, 0x5b4, 0x5b0, 0x5ac, 0x5a8,
    0x5a4, 0x5a0, 0x59c, 0x599, 0x595, 0x591, 0x58d, 0x589,
    0x585, 0x581, 0x57e, 0x57a, 0x576, 0x572, 0x56f, 0x56b,
    0x567, 0x563, 0x560, 0x55c, 0x558, 0x554, 0x551, 0x54d,
    0x549, 0x546, 0x542, 0x53e, 0x53b, 0x537, 0x534, 0x530,
    0x52c, 0x529, 0x525, 0x522, 0x51e, 0x51b, 0x517, 0x514,
    0x510, 0x50c, 0x509, 0x506, 0x502, 0x4ff, 0x4fb, 0x4f8,
    0x4f4, 0x4f1, 0x4ed, 0x4ea, 0x4e7, 0x4e3, 0x4e0, 0x4dc,
    0x4d9, 0x4d6, 0x4d2, 0x4cf, 0x4cc, 0x4c8, 0x4c5, 0x4c2,
    0x4be, 0x4bb, 0x4b8, 0x4b5, 0x4b1, 0x4ae, 0x4ab, 0x4a8,
    0x4a4, 0x4a1, 0x49e, 0x49b, 0x498, 0x494, 0x491, 0x48e,
    0x48b, 0x488, 0x485, 0x482, 0x47e, 0x47b, 0x478, 0x475,
    0x472, 0x46f, 0x46c, 0x469, 0x466, 0x463, 0x460, 0x45d,
    0x45a, 0x457, 0x454, 0x451, 0x44e, 0x44b, 0x448, 0x445,
    0x442, 0x43f, 0x43c, 0x439, 0x436, 0x433, 0x430, 0x42d,
    0x42a, 0x428, 0x425, 0x422, 0x41f, 0x41c, 0x419, 0x416,
    0x414, 0x411, 0x40e, 0x40b, 0x408, 0x406, 0x403, 0x400
};

/* Envelope generator */
static const bool eg_stephi[4][4] = {
    { 0, 0, 0, 0 },
    { 1, 0, 0, 0 },
    { 1, 0, 1, 0 },
    { 1, 1, 1, 0 }
};

/* Phase generator */
static const uint8_t pg_detune[8] = { 16, 17, 19, 20, 22, 24, 27, 29 };

typedef struct {
    uint16_t basefreq;
    bool approxtype;
    uint8_t slope;
} freqtable_t;

static const freqtable_t pg_freqtable[64] = {
    { 1299, 1, 19 },
    { 1318, 1, 19 },
    { 1337, 1, 19 },
    { 1356, 1, 20 },
    { 1376, 1, 20 },
    { 1396, 1, 20 },
    { 1416, 1, 21 },
    { 1437, 1, 20 },
    { 1458, 1, 21 },
    { 1479, 1, 21 },
    { 1501, 1, 22 },
    { 1523, 1, 22 },
    { 0,    0, 16 },
    { 0,    0, 16 },
    { 0,    0, 16 },
    { 0,    0, 16 },
    { 1545, 1, 22 },
    { 1567, 1, 22 },
    { 1590, 1, 23 },
    { 1613, 1, 23 },
    { 1637, 1, 23 },
    { 1660, 1, 24 },
    { 1685, 1, 24 },
    { 1709, 1, 24 },
    { 1734, 1, 25 },
    { 1759, 1, 25 },
    { 1785, 1, 26 },
    { 1811, 1, 26 },
    { 0,    0, 16 },
    { 0,    0, 16 },
    { 0,    0, 16 },
    { 0,    0, 16 },
    { 1837, 1, 26 },
    { 1864, 1, 27 },
    { 1891, 1, 27 },
    { 1918, 1, 28 },
    { 1946, 1, 28 },
    { 1975, 1, 28 },
    { 2003, 1, 29 },
    { 2032, 1, 30 },
    { 2062, 1, 30 },
    { 2092, 1, 30 },
    { 2122, 1, 31 },
    { 2153, 1, 31 },
    { 0,    0, 16 },
    { 0,    0, 16 },
    { 0,    0, 16 },
    { 0,    0, 16 },
    { 2185, 1, 31 },
    { 2216, 0, 31 },
    { 2249, 0, 31 },
    { 2281, 0, 31 },
    { 2315, 0, 31 },
    { 2348, 0, 31 },
    { 2382, 0, 30 },
    { 2417, 0, 30 },
    { 2452, 0, 30 },
    { 2488, 0, 30 },
    { 2524, 0, 30 },
    { 2561, 0, 30 },
    { 0,    0, 16 },
    { 0,    0, 16 },
    { 0,    0, 16 },
    { 0,    0, 16 }
};


/* FM algorithm */
static const bool fm_algorithm[4][6][8] = {
    {
        { 1, 1, 1, 1, 1, 1, 1, 1 }, /* M1_0          */
        { 1, 1, 1, 1, 1, 1, 1, 1 }, /* M1_1          */
        { 0, 0, 0, 0, 0, 0, 0, 0 }, /* C1            */
        { 0, 0, 0, 0, 0, 0, 0, 0 }, /* Last operator */
        { 0, 0, 0, 0, 0, 0, 0, 0 }, /* Last operator */
        { 0, 0, 0, 0, 0, 0, 0, 1 }  /* Out           */
    },
    {
        { 0, 1, 0, 0, 0, 1, 0, 0 }, /* M1_0          */
        { 0, 0, 0, 0, 0, 0, 0, 0 }, /* M1_1          */
        { 1, 1, 1, 0, 0, 0, 0, 0 }, /* C1            */
        { 0, 0, 0, 0, 0, 0, 0, 0 }, /* Last operator */
        { 0, 0, 0, 0, 0, 0, 0, 0 }, /* Last operator */
        { 0, 0, 0, 0, 0, 1, 1, 1 }  /* Out           */
    },
    {
        { 0, 0, 0, 0, 0, 0, 0, 0 }, /* M1_0          */
        { 0, 0, 0, 0, 0, 0, 0, 0 }, /* M1_1          */
        { 0, 0, 0, 0, 0, 0, 0, 0 }, /* C1            */
        { 1, 0, 0, 1, 1, 1, 1, 0 }, /* Last operator */
        { 0, 0, 0, 0, 0, 0, 0, 0 }, /* Last operator */
        { 0, 0, 0, 0, 1, 1, 1, 1 }  /* Out           */
    },
    {
        { 0, 0, 1, 0, 0, 1, 0, 0 }, /* M1_0          */
        { 0, 0, 0, 0, 0, 0, 0, 0 }, /* M1_1          */
        { 0, 0, 0, 1, 0, 0, 0, 0 }, /* C1            */
        { 1, 1, 0, 1, 1, 0, 0, 0 }, /* Last operator */
        { 0, 0, 1, 0, 0, 0, 0, 0 }, /* Last operator */
        { 1, 1, 1, 1, 1, 1, 1, 1 }  /* Out           */
    }
};

static uint16_t lfo_counter2_table[] = {
    0x0000, 0x4000, 0x6000, 0x7000,
    0x7800, 0x7c00, 0x7e00, 0x7f00,
    0x7f80, 0x7fc0, 0x7fe0, 0x7ff0,
    0x7ff8, 0x7ffc, 0x7ffe, 0x7fff
};

INLINE uint16_t OPM_KCToFNum(const int32_t kcode)
{
    const uint8_t kcode_h = (kcode >> 4) & 63;
    const uint8_t kcode_l = kcode & 15;
    uint32_t sum = 0;
    if (pg_freqtable[kcode_h].approxtype)
    {
        int32_t i;
        for (i = 0; i < 4; i++)
        {
            if (kcode_l & (1 << i))
            {
                sum += (pg_freqtable[kcode_h].slope >> (3 - i));
            }
        }
    }
    else
    {
        const uint8_t slope = pg_freqtable[kcode_h].slope | 1;
        if (kcode_l & 1)
        {
            sum += (slope >> 3) + 2;
        }
        if (kcode_l & 2)
        {
            sum += 8;
        }
        if (kcode_l & 4)
        {
            sum += slope >> 1;
        }
        if (kcode_l & 8)
        {
            sum += slope;
            sum++;
        }
        if ((kcode_l & 12) == 12 && !(pg_freqtable[kcode_h].slope & 1))
        {
            sum += 4;
        }
    }
    return pg_freqtable[kcode_h].basefreq + (sum >> 1);
}

INLINE uint16_t OPM_LFOApplyPMS(const uint8_t lfo, const uint8_t pms)
{
    bool t;
    uint16_t out;
    uint8_t top = (lfo >> 4) & 7;
    if (pms != 7)
    {
        top >>= 1;
    }
    t = (top & 6) == 6 || ((top & 3) == 3 && pms >= 6);

    out = (uint16_t)top + ((top >> 2) & 1) + t;
    out = out * 2 + ((lfo >> 4) & 1);

    if (pms == 7)
    {
        out >>= 1;
    }
    out &= 15;
    out = (lfo & 15) + out * 16;
    switch (pms)
    {
    case 0:
    default:
        out = 0;
        break;
    case 1:
        out = (out >> 5) & 3;
        break;
    case 2:
        out = (out >> 4) & 7;
        break;
    case 3:
        out = (out >> 3) & 15;
        break;
    case 4:
        out = (out >> 2) & 31;
        break;
    case 5:
        out = (out >> 1) & 63;
        break;
    case 6:
        out = (out /*& 255*/) << 1;
        break;
    case 7:
        out = (out /*& 255*/) << 2;
        break;
    }
    return out;
}

INLINE int32_t OPM_CalcKCode(const uint16_t kcf, uint16_t lfo, const bool lfo_sign, const uint8_t dt)
{
    uint8_t t2;
    int32_t t3;
    bool b0, b1, b2, b3, w2, w3, w6;
    bool overflow1 = 0;
    bool overflow2 = 0;
    bool negoverflow = 0;
    int32_t sum;
    bool cr;
    if (!lfo_sign)
    {
        lfo = ~lfo;
    }
    sum = (int32_t)(kcf & 8191) + (lfo&8191) + (!lfo_sign);
    cr = ((int32_t)(kcf & 255) + (lfo & 255) + (!lfo_sign)) >> 8;
    if (sum & (1 << 13))
    {
        overflow1 = 1;
    }
    sum &= 8191;
    if (lfo_sign && (((sum & (3<<6)) == (3<<6)) || cr))
    {
        sum += 64;
    }
    if (!lfo_sign && !cr)
    {
        sum += (-64)&8191;
        negoverflow = 1;
    }
    if (sum & (1 << 13))
    {
        overflow2 = 1;
    }
    sum &= 8191;
    if ((!lfo_sign && !overflow1) || (negoverflow && !overflow2))
    {
        sum = 0;
    }
    if (lfo_sign && (overflow1 || overflow2))
    {
        sum = 8127;
    }
        
    t2 = sum & 63;
    if (dt == 2)
        t2 += 20;
    if (dt == 2 || dt == 3)
        t2 += 32;

    b0 = (t2 & (uint8_t)(1<<6));
    b1 = dt == 2;
    b2 = (sum & (1<<6));
    b3 = (sum & (1<<7));


    w2 = (b0 && b1 && b2);
    w3 = (b0 && b3);
    w6 = (b0 && !w2 && !w3) || (b3 && !b0 && b1);

    t2 &= 63;

    t3 = (sum >> 6) + w6 + b1 + ((int32_t)(w2 || w3)<<1) + ((int32_t)(dt == 3)<<2) + ((int32_t)(dt != 0)<<3);
    if (t3 & 128)
    {
        t2 = 63;
        t3 = 126;
    }
    sum = t3 * 64 + t2;
    return sum;
}

INLINE void OPM_PhaseCalcFNumBlock(opm_t* const chip)
{
    const uint32_t slot = (chip->cycles + 7) & 31;
    const uint32_t channel = slot & 7;
    const uint16_t kcf = (chip->ch_kc[channel] << 6) + chip->ch_kf[channel];
    const uint8_t lfo = chip->lfo_pmd ? chip->lfo_pm_lock : 0;
    const uint8_t pms = chip->opp ? chip->pg_opp_pms : chip->ch_pms[channel];
    const uint8_t dt = chip->opp ? chip->pg_opp_dt2[slot] : chip->sl_dt2[slot];
    const uint16_t lfo_pm = OPM_LFOApplyPMS(lfo & 127, pms);
    const uint32_t kcode = OPM_CalcKCode(kcf, lfo_pm, !((lfo & 0x80) && pms), dt);
    const uint16_t fnum = OPM_KCToFNum(kcode);
    const uint8_t kcode_h = kcode >> 8;
    chip->pg_fnum[slot] = fnum;
    chip->pg_kcode[slot] = kcode_h;

    if (chip->opp)
    {
        const uint32_t slotopp = chip->cycles & 31;
        const uint32_t channelopp = slotopp & 7;
        chip->pg_opp_pms = chip->ch_pms[channelopp];
        chip->pg_opp_dt2[slotopp] = chip->sl_dt2[slotopp];
    }
}

INLINE void OPM_PhaseCalcIncrement(opm_t* const chip)
{
    const uint32_t slot = chip->cycles;
    //const uint32_t channel = slot & 7;
    const uint8_t dt = chip->sl_dt1[slot];
    const uint8_t dt_l = dt & 3;
    uint8_t detune = 0;
    const uint8_t multi = chip->sl_mul[slot];
    uint8_t kcode = chip->pg_kcode[slot];
    const uint32_t fnum = chip->pg_fnum[slot];
    uint8_t block = kcode >> 2;
    uint32_t basefreq = (fnum << block) >> 2;
    uint32_t inc;
    /* Apply detune */
    if (dt_l)
    {
        uint8_t note, sum, sum_h, sum_l;
        if (kcode > 0x1c)
        {
            kcode = 0x1c;
        }
        block = kcode >> 2;
        note = kcode & 0x03;
        sum = block + 9 + ((dt_l == 3) | (dt_l & 0x02));
        sum_h = sum >> 1;
        sum_l = sum & 0x01;
        detune = pg_detune[(sum_l << 2) | note] >> (9 - sum_h);
    }
    if (dt & 0x04)
    {
        basefreq -= detune;
    }
    else
    {
        basefreq += detune;
    }
    basefreq &= 0x1ffff;
    if (multi)
    {
        inc = basefreq * multi;
    }
    else
    {
        inc = basefreq >> 1;
    }
    inc &= 0xfffff;
    chip->pg_inc[slot] = inc;
}

INLINE void OPM_PhaseGenerate(opm_t* const chip)
{
    uint32_t slot = (chip->cycles + 27) & 31;
    chip->pg_reset_latch[slot] = chip->pg_reset[slot];
    slot = (chip->cycles + 25) & 31;
    /* Mask increment */
    if (chip->pg_reset_latch[slot])
    {
        chip->pg_inc[slot] = 0;
    }
    /* Phase step */
    slot = (chip->cycles + 24) & 31;
    if (chip->pg_reset_latch[slot] || chip->mode_test[3])
    {
        chip->pg_phase[slot] = 0;
    }
    chip->pg_phase[slot] += chip->pg_inc[slot];
    chip->pg_phase[slot] &= 0xfffff;
}

INLINE void OPM_PhaseDebug(opm_t* const chip)
{
    chip->pg_serial >>= 1;
    if (chip->cycles == 5)
    {
        chip->pg_serial |= (chip->pg_phase[29] & 0x3ff);
    }
}

INLINE void OPM_KeyOn1(opm_t* const chip)
{
    const uint8_t cycles = (chip->cycles + 1) & 31;
    chip->kon_chanmatch = (chip->mode_kon_channel + 24 == cycles);
}

INLINE void OPM_KeyOn2(opm_t* const chip)
{
    if (chip->kon_chanmatch)
    {
        const uint32_t slot = (chip->cycles + 8) & 31;
        chip->mode_kon[(slot +  0) & 31] = chip->mode_kon_operator[0];
        chip->mode_kon[(slot +  8) & 31] = chip->mode_kon_operator[2];
        chip->mode_kon[(slot + 16) & 31] = chip->mode_kon_operator[1];
        chip->mode_kon[(slot + 24) & 31] = chip->mode_kon_operator[3];
    }
}

INLINE void OPM_EnvelopePhase1(opm_t* const chip)
{
    const uint32_t slot = (chip->cycles + 2) & 31;
    const bool kon = chip->mode_kon[slot] || chip->kon_csm;

    chip->kon2[slot] = chip->kon[slot];
    chip->kon[slot] = kon;
}

INLINE void OPM_EnvelopePhase2(opm_t* const chip)
{
    const uint32_t slot = chip->cycles;
    const uint32_t chan = slot & 7;
    uint8_t rate, ksv, ams;
    bool zr;

    if (chip->ic)
    {
        rate = 31;
    }
    else
    {
    uint8_t sel = chip->eg_state[slot];
    if (chip->kon[slot] && !chip->kon2[slot])
        sel = eg_num_attack;
    switch (sel)
    {
    case eg_num_attack:
        rate = chip->sl_ar[slot];
        break;
    case eg_num_decay:
        rate = chip->sl_d1r[slot];
        break;
    case eg_num_sustain:
        rate = chip->sl_d2r[slot];
        break;
    case eg_num_release:
        rate = chip->sl_rr[slot] * 2 + 1;
        break;
    default:
        rate = 0;
        break;
    }
    }

    zr = rate == 0;

    ksv = chip->pg_kcode[slot] >> (chip->sl_ks[slot] ^ 3);
    if (chip->sl_ks[slot] == 0 && zr)
    {
        ksv &= ~3;
    }
    rate = rate * 2 + ksv;
    if (rate & 64)
    {
        rate = 63;
    }

    if (chip->opp)
    {
        const uint32_t slotopp = (chip->cycles + 30) & 31;
        chip->eg_tl_opp = chip->opp_tl[slotopp];
    }
    else
    {
        chip->eg_tl[2] = chip->eg_tl[1];
        chip->eg_tl[1] = chip->eg_tl[0];
        chip->eg_tl[0] = chip->sl_tl[slot];
    }
    chip->eg_sl[1] = chip->eg_sl[0];
    chip->eg_sl[0] = chip->sl_d1l[slot];
    if (chip->eg_sl[0] == 15)
    {
        chip->eg_sl[0] = 31;
    }
    chip->eg_zr[1] = chip->eg_zr[0];
    chip->eg_zr[0] = zr;
    chip->eg_rate[1] = chip->eg_rate[0];
    chip->eg_rate[0] = rate;
    chip->eg_ratemax[1] = chip->eg_ratemax[0];
    chip->eg_ratemax[0] = (rate >> 1) == 31;
    ams = chip->sl_am_e[slot] ? chip->ch_ams[chan] : 0;
    switch (ams)
    {
    default:
    case 0:
        chip->eg_am = 0;
        break;
    case 1:
        chip->eg_am = chip->lfo_am_lock << 0;
        break;
    case 2:
        chip->eg_am = (uint16_t)chip->lfo_am_lock << 1;
        break;
    case 3:
        chip->eg_am = (uint16_t)chip->lfo_am_lock << 2;
        break;
    }
}

INLINE void OPM_EnvelopePhase3(opm_t* const chip)
{
    const uint32_t slot = (chip->cycles + 31) & 31;
    chip->eg_shift = (chip->eg_timershift_lock + (chip->eg_rate[0] >> 2)) & 15;
    chip->eg_inchi = eg_stephi[chip->eg_rate[0] & 3][chip->eg_timer_lock & 3];

    chip->eg_outtemp[1] = chip->eg_outtemp[0];
    chip->eg_outtemp[0] = chip->eg_level[slot] + chip->eg_am;
    if (chip->eg_outtemp[0] & 1024)
    {
        chip->eg_outtemp[0] = 1023;
    }
}

INLINE void OPM_EnvelopePhase4(opm_t* const chip)
{
    const uint32_t slot = (chip->cycles + 30) & 31;
    uint8_t inc = 0;
    bool kon, eg_off, eg_zero, slreach;
    if (chip->eg_clock & 2)
    {
        if (chip->eg_rate[1] >= 48)
        {
            inc = chip->eg_inchi + (chip->eg_rate[1] >> 2) - 11;
            if (inc > 4)
            {
                inc = 4;
            }
        }
        else if (!chip->eg_zr[1])
        {
            switch (chip->eg_shift)
            {
            case 12:
                inc = chip->eg_rate[1] != 0;
                break;
            case 13:
                inc = (chip->eg_rate[1] >> 1) & 1;
                break;
            case 14:
                inc = chip->eg_rate[1] & 1;
                break;
            }
        }
    }
    chip->eg_inc = inc;

    kon = chip->kon[slot] && !chip->kon2[slot];
    chip->pg_reset[slot] = kon;
    chip->eg_instantattack = chip->eg_ratemax[1] && (kon || !chip->eg_ratemax[1]); // apparently correct as-is

    eg_off = (chip->eg_level[slot] & 0x3f0) == 0x3f0;
    slreach = (chip->eg_level[slot] >> 4) == (chip->eg_sl[1] << 1);
    eg_zero = chip->eg_level[slot] == 0;

    chip->eg_mute = eg_off && chip->eg_state[slot] != eg_num_attack && !kon;
    chip->eg_inclinear = 0;
    if (!kon && !eg_off)
    {
        switch (chip->eg_state[slot])
        {
        case eg_num_decay:
            if (!slreach)
                chip->eg_inclinear = 1;
            break;
        case eg_num_sustain:
        case eg_num_release:
            chip->eg_inclinear = 1;
            break;
        }
    }
    chip->eg_incattack = chip->eg_state[slot] == eg_num_attack && !chip->eg_ratemax[1] && chip->kon[slot] && !eg_zero;


    // Update state
    if (chip->ic)
    {
        chip->eg_state[slot] = eg_num_release;
    }
    else if (kon)
    {
        chip->eg_state[slot] = eg_num_attack;
    }
    else if (!chip->kon[slot])
    {
        chip->eg_state[slot] = eg_num_release;
    }
    else
    {
        switch (chip->eg_state[slot])
        {
        case eg_num_attack:
            if (eg_zero)
            {
                chip->eg_state[slot] = eg_num_decay;
            }
            break;
        case eg_num_decay:
            if (eg_off)
            {
                chip->eg_state[slot] = eg_num_release;
            }
            else if (slreach)
            {
                chip->eg_state[slot] = eg_num_sustain;
            }
            break;
        case eg_num_sustain:
            if (eg_off)
            {
                chip->eg_state[slot] = eg_num_release;
            }
            break;
        case eg_num_release:
            break;
        }
    }
}

INLINE void OPM_EnvelopePhase5(opm_t* const chip)
{
    const uint32_t slot = (chip->cycles + 29) & 31;
    uint32_t level = chip->eg_instantattack ? 0 : chip->eg_level[slot];
    uint32_t step = 0;
    if (chip->eg_mute || chip->ic)
    {
        level = 0x3ff;
    }
    if (chip->eg_inc)
    {
        if (chip->eg_inclinear)
        {
            step |= 1u << (chip->eg_inc - 1);
        }
        if (chip->eg_incattack)
        {
            step |= ((~(int32_t)chip->eg_level[slot]) << chip->eg_inc) >> 5;
        }
    }
    level += step;
    chip->eg_level[slot] = (uint16_t)level;

    chip->eg_out[0] = chip->eg_outtemp[1];
    if (chip->opp)
        chip->eg_out[0] += chip->eg_tl_opp;
    else
        chip->eg_out[0] += (uint16_t)chip->eg_tl[2] << 3;
    if (chip->eg_out[0] & 1024)
    {
        chip->eg_out[0] = 1023;
    }

    if (chip->eg_test)
    {
        chip->eg_out[0] = 0;
    }

    chip->eg_test = chip->mode_test[5];
}

INLINE void OPM_EnvelopePhase6(opm_t* const chip)
{
    //uint32_t slot = (chip->cycles + 28) & 31;
    chip->eg_serial_bit = chip->eg_serial & (1<<9);
    if (chip->cycles == 3)
    {
        chip->eg_serial = chip->eg_out[0] ^ 1023;
    }
    else
    {
        chip->eg_serial <<= 1;
    }

    chip->eg_out[1] = chip->eg_out[0];
}

INLINE void OPM_EnvelopeClock(opm_t* const chip)
{
    chip->eg_clock <<= 1;
    if ((chip->eg_clockcnt & 2) || chip->mode_test[0])
    {
        chip->eg_clock |= 1;
    }
    if (chip->ic || (chip->cycles == 31 && (chip->eg_clockcnt & 2)))
    {
        chip->eg_clockcnt = 0;
    }
    else if (chip->cycles == 31)
    {
        chip->eg_clockcnt++;
    }
}

INLINE void OPM_EnvelopeTimer(opm_t* const chip)
{
    const uint32_t cycle = (chip->cycles + 31) & 15;
    uint32_t cycle2;
    const bool inc = ((chip->cycles + 31) & 31) < 16 && (chip->eg_clock & 1) && (cycle == 0 || chip->eg_timercarry);
    const bool timerbit = (chip->eg_timer >> cycle) & 1;
    const bool sum0 = (timerbit != inc) && !chip->ic;
    chip->eg_timercarry = (timerbit && inc);
    chip->eg_timer = (chip->eg_timer & (~(1 << cycle))) | ((uint32_t)sum0 << cycle);

    cycle2 = (chip->cycles + 30) & 15;

    chip->eg_timer2 <<= 1;
    if ((chip->eg_timer & (1 << cycle2)) && !chip->eg_timerbstop)
    {
        chip->eg_timer2 |= 1;
    }

    if (chip->eg_timer & (1 << cycle2))
    {
        chip->eg_timerbstop = 1;
    }

    if (cycle == 0 || chip->ic2)
    {
        chip->eg_timerbstop = 0;
    }

    if (chip->cycles == 1 && (chip->eg_clock & 1))
    {
        chip->eg_timershift_lock = 0;
        if (chip->eg_timer2 & (8 + 32 + 128 + 512 + 2048 + 8192 + 32768))
        {
            chip->eg_timershift_lock |= 1;
        }
        if (chip->eg_timer2 & (4 + 32 + 64 + 512 + 1024 + 8192 + 16384))
        {
            chip->eg_timershift_lock |= 2;
        }
        if (chip->eg_timer2 & (4 + 8 + 16 + 512 + 1024 + 2048 + 4096))
        {
            chip->eg_timershift_lock |= 4;
        }
        if (chip->eg_timer2 & (4 + 8 + 16 + 32 + 64 + 128 + 256))
        {
            chip->eg_timershift_lock |= 8;
        }
        chip->eg_timer_lock = chip->eg_timer;
    }
}

INLINE void OPM_OperatorPhase1(opm_t* const chip)
{
    const uint32_t slot = chip->cycles;
    int16_t mod = chip->op_mod[2];
    chip->op_phase_in = chip->pg_phase[slot] >> 10;
    if (chip->op_fbshift & 8)
    {
        if (chip->op_fb[1] == 0)
        {
            mod = 0;
        }
        else
        {
            mod >>= (9 - chip->op_fb[1]);
        }
    }
    chip->op_mod_in = mod;
}

INLINE void OPM_OperatorPhase2(opm_t* const chip)
{
    //uint32_t slot = (chip->cycles + 31) & 31;
    chip->op_phase = (chip->op_phase_in + chip->op_mod_in) & 1023;
}

INLINE void OPM_OperatorPhase3(opm_t* const chip)
{
    //const uint32_t slot = (chip->cycles + 30) & 31;
    uint8_t phase = chip->op_phase;
    if (chip->op_phase & 256)
    {
        phase = ~phase;
    }
    chip->op_logsin[0] = logsinrom[phase];
    chip->op_sign <<= 1;
    chip->op_sign |= (chip->op_phase >> 9) & 1;
}

INLINE void OPM_OperatorPhase4(opm_t* const chip)
{
    //const uint32_t slot = (chip->cycles + 29) & 31;
    chip->op_logsin[1] = chip->op_logsin[0];
}

INLINE void OPM_OperatorPhase5(opm_t* const chip)
{
    //const uint32_t slot = (chip->cycles + 28) & 31;
    chip->op_logsin[2] = chip->op_logsin[1];
}

INLINE void OPM_OperatorPhase6(opm_t* const chip)
{
    //const uint32_t slot = (chip->cycles + 27) & 31;
    chip->op_atten = chip->op_logsin[2] + (chip->eg_out[1] << 2);
    if (chip->op_atten & 4096)
    {
        chip->op_atten = 4095;
    }
}

INLINE void OPM_OperatorPhase7(opm_t* const chip)
{
    //const uint32_t slot = (chip->cycles + 26) & 31;
    chip->op_exp[0] = exprom[chip->op_atten & 255];
    chip->op_pow[0] = chip->op_atten >> 8;
}

INLINE void OPM_OperatorPhase8(opm_t* const chip)
{
    //const uint32_t slot = (chip->cycles + 25) & 31;
    chip->op_exp[1] = chip->op_exp[0];
    chip->op_pow[1] = chip->op_pow[0];
}

INLINE void OPM_OperatorPhase9(opm_t* const chip)
{
    //const uint32_t slot = (chip->cycles + 24) & 31;
    int16_t out = (chip->op_exp[1] << 2) >> (chip->op_pow[1]);
    if (!chip->opp && chip->mode_test[4])
    {
        out |= 0x2000;
    }
    chip->op_out[0] = out;
}

INLINE void OPM_OperatorPhase10(opm_t* const chip)
{
    //const uint32_t slot = (chip->cycles + 23) & 31;
    int16_t out = chip->op_out[0];
    if (chip->op_sign & 64)
    {
        out ^= 0x3fff;
        out = (out + 1) & 0x3fff;
    }
    out <<= 2; out >>= 2;
    chip->op_out[1] = out;
}

INLINE void OPM_OperatorPhase11(opm_t* const chip)
{
    //const uint32_t slot = (chip->cycles + 22) & 31;
    chip->op_out[2] = chip->op_out[1];
}

INLINE void OPM_OperatorPhase12(opm_t* const chip)
{
    //const uint32_t slot = (chip->cycles + 21) & 31;
    chip->op_out[3] = chip->op_out[2];
}

INLINE void OPM_OperatorPhase13(opm_t* const chip)
{
    const uint32_t slot = (chip->cycles + 20) & 31;
    const uint32_t channel = slot & 7;
    chip->op_out[4] = chip->op_out[3];
    chip->op_connect = chip->ch_connect[channel];
    if (chip->opp)
    {
        chip->op_opp_rl = chip->ch_rl[channel];
        chip->op_opp_fb[2] = chip->op_opp_fb[1];
        chip->op_opp_fb[1] = chip->op_opp_fb[0];
        chip->op_opp_fb[0] = chip->ch_fb[channel];
    }
}

INLINE void OPM_OperatorPhase14(opm_t* const chip)
{
    const uint32_t slot = (chip->cycles + 19) & 31;
    const uint32_t channel = slot & 7;
    const uint8_t tmp = (chip->op_counter + 2) & 3;
    uint8_t rl;
    chip->op_mix = chip->op_out[5] = chip->op_out[4];
    chip->op_fbupdate = (chip->op_counter == 0);
    chip->op_c1update = (chip->op_counter == 2);
    chip->op_fbshift <<= 1;
    chip->op_fbshift |= (chip->op_counter == 2);

    chip->op_modtable[0] = fm_algorithm[tmp][0][chip->op_connect];
    chip->op_modtable[1] = fm_algorithm[tmp][1][chip->op_connect];
    chip->op_modtable[2] = fm_algorithm[tmp][2][chip->op_connect];
    chip->op_modtable[3] = fm_algorithm[tmp][3][chip->op_connect];
    chip->op_modtable[4] = fm_algorithm[tmp][4][chip->op_connect];
    rl = chip->opp ? chip->op_opp_rl : chip->ch_rl[channel];
    chip->op_mixl = fm_algorithm[chip->op_counter][5][chip->op_connect] && (rl & 1) != 0;
    chip->op_mixr = fm_algorithm[chip->op_counter][5][chip->op_connect] && (rl & 2) != 0;
}

INLINE void OPM_OperatorPhase15(opm_t* const chip)
{
    const uint32_t slot = (chip->cycles + 18) & 31;
    const uint32_t channel = slot & 7;
    int16_t mod1 = 0, mod2 = 0;
    if (chip->op_modtable[0])
    {
        mod2 |= chip->op_m1[channel][0];
    }
    if (chip->op_modtable[1])
    {
        mod1 |= chip->op_m1[channel][1];
    }
    if (chip->op_modtable[2])
    {
        mod1 |= chip->op_c1[channel];
    }
    if (chip->op_modtable[3])
    {
        mod2 |= chip->op_out[5];
    }
    if (chip->op_modtable[4])
    {
        mod1 |= chip->op_out[5];
    }
    chip->op_mod[0] = (mod1 + mod2) >> 1;
    if (chip->op_fbupdate)
    {
        chip->op_m1[channel][1] = chip->op_m1[channel][0];
        chip->op_m1[channel][0] = chip->op_out[5];
    }
    if (chip->op_c1update)
    {
        chip->op_c1[channel] = chip->op_out[5];
    }
}

INLINE void OPM_OperatorPhase16(opm_t* const chip)
{
    const uint32_t slot = (chip->cycles + 17) & 31;
    // hack
    chip->op_mod[2] = chip->op_mod[1];
    chip->op_fb[1] = chip->op_fb[0];

    chip->op_mod[1] = chip->op_mod[0];
    chip->op_fb[0] = chip->opp ? chip->op_opp_fb[2] : chip->ch_fb[slot & 7];
}

INLINE void OPM_OperatorCounter(opm_t* const chip)
{
    if ((chip->cycles & 7) == 4)
    {
        chip->op_counter++;
    }
    if (chip->cycles == 12)
    {
        chip->op_counter = 0;
    }
}

INLINE void OPM_Mixer2(opm_t* const chip)
{
    const uint32_t cycles = (chip->cycles + 30) & 31;
    const bool bit = ((cycles < 16) ? chip->mix_serial[0] : chip->mix_serial[1]) & 1;
    if ((chip->cycles & 15) == 1)
    {
        chip->mix_sign_lock = !bit;
        chip->mix_top_bits_lock = (chip->mix_bits >> 15) & 63;
    }
    if ((chip->cycles & 15) == 7)
    {
        uint8_t top = chip->mix_top_bits_lock;
        uint8_t ex;
        if (chip->mix_sign_lock)
        {
            top ^= 63;
        }
        if (top & 32)
        {
            ex = 7;
        }
        else if (top & 16)
        {
            ex = 6;
        }
        else if (top & 8)
        {
            ex = 5;
        }
        else if (top & 4)
        {
            ex = 4;
        }
        else if (top & 2)
        {
            ex = 3;
        }
        else if (top & 1)
        {
            ex = 2;
        }
        else
        {
            ex = 1;
        }
        chip->mix_sign_lock2 = chip->mix_sign_lock;
        chip->mix_exp_lock = ex;
    }
    chip->mix_out_bit <<= 1;
    switch (chip->cycles & 15)
    {
    case 0:
        chip->mix_out_bit |= !chip->mix_sign_lock2;
        break;
    case 1:
        chip->mix_out_bit |= (chip->mix_exp_lock >> 0) & 1;
        break;
    case 2:
        chip->mix_out_bit |= (chip->mix_exp_lock >> 1) & 1;
        break;
    case 3:
        chip->mix_out_bit |= (chip->mix_exp_lock >> 2) & 1;
        break;
    default:
        if (chip->mix_exp_lock)
        {
            chip->mix_out_bit |= (chip->mix_bits >> (chip->mix_exp_lock - 1)) & 1;
        }
        break;
    }
    chip->mix_bits >>= 1;
    if (bit)
        chip->mix_bits |= 1u << 20;
}

INLINE void OPM_Output(opm_t* const chip)
{
    const uint32_t slot = (chip->cycles + 27) & 31;
    chip->smp_so = (chip->mix_out_bit & 1);
    chip->smp_sh1 = (slot & 24) ==  8 && !chip->ic;
    chip->smp_sh2 = (slot & 24) == 24 && !chip->ic;
}

INLINE void OPM_DAC(opm_t* const chip)
{
    if (chip->dac_osh1 && !chip->smp_sh1)
    {
        const uint8_t exp = (chip->dac_bits >> 10) & 7;
        int32_t mant = chip->dac_bits & 1023;
        mant -= 512;
        chip->dac_output[1] = (mant << exp) >> 1;
    }
    if (chip->dac_osh2 && !chip->smp_sh2)
    {
        const uint8_t exp = (chip->dac_bits >> 10) & 7;
        int32_t mant = chip->dac_bits & 1023;
        mant -= 512;
        chip->dac_output[0] = (mant << exp) >> 1;
    }
    chip->dac_bits >>= 1;
    if(chip->smp_so)
        chip->dac_bits |= (uint16_t)1 << 12;
    chip->dac_osh1 = chip->smp_sh1;
    chip->dac_osh2 = chip->smp_sh2;
}

INLINE void OPM_Mixer(opm_t* const chip)
{
    //const uint32_t slot = (chip->cycles + 18) & 31;
    //const uint32_t channel = slot & 7;
    // Right channel
    chip->mix_serial[1] >>= 1;
    if (chip->cycles == 13)
    {
        chip->mix_serial[1] |= (chip->mix[1] & 1023) << 4;
    }
    if (chip->cycles == 14)
    {
        chip->mix_serial[1] |= ((chip->mix2[1] >> 10) & 31) << 13;
        chip->mix_serial[1] |= (((chip->mix2[1] >> 17) & 1) ^ 1) << 18;
        chip->mix_clamp_low[1] = 0;
        chip->mix_clamp_high[1] = 0;
        switch ((chip->mix2[1]>>15) & 7)
        {
        case 0:
        default:
            break;
        case 1:
        case 2:
        case 3:
            chip->mix_clamp_high[1] = 1;
            break;
        case 4:
        case 5:
        case 6:
            chip->mix_clamp_low[1] = 1;
            break;
        case 7:
            break;
        }
    }
    if (chip->mix_clamp_low[1])
    {
        chip->mix_serial[1] &= ~2;
    }
    if (chip->mix_clamp_high[1])
    {
        chip->mix_serial[1] |= 2;
    }
    // Left channel
    chip->mix_serial[0] >>= 1;
    if (chip->cycles == 29)
    {
        chip->mix_serial[0] |= (chip->mix[0] & 1023) << 4;
    }
    if (chip->cycles == 30)
    {
        chip->mix_serial[0] |= ((chip->mix2[0] >> 10) & 31) << 13;
        chip->mix_serial[0] |= (((chip->mix2[0] >> 17) & 1) ^ 1) << 18;
        chip->mix_clamp_low[0] = 0;
        chip->mix_clamp_high[0] = 0;
        switch ((chip->mix2[0]>>15) & 7)
        {
        case 0:
        default:
            break;
        case 1:
        case 2:
        case 3:
            chip->mix_clamp_high[0] = 1;
            break;
        case 4:
        case 5:
        case 6:
            chip->mix_clamp_low[0] = 1;
            break;
        case 7:
            break;
        }
    }
    if (chip->mix_clamp_low[0])
    {
        chip->mix_serial[0] &= ~2;
    }
    if (chip->mix_clamp_high[0])
    {
        chip->mix_serial[0] |= 2;
    }
    chip->mix2[0] = chip->mix[0];
    chip->mix2[1] = chip->mix[1];
    if (chip->cycles == 13)
    {
        chip->mix[1] = 0;
    }
    if (chip->cycles == 29)
    {
        chip->mix[0] = 0;
    }
    if (chip->op_mixl)
        chip->mix[0] += chip->op_mix;
    if (chip->op_mixr)
        chip->mix[1] += chip->op_mix;
}

INLINE void OPM_Noise(opm_t* const chip)
{
    const bool noise_step = chip->ic || chip->noise_update;
    bool bit = 0;
    if (noise_step)
    {
        if (!chip->ic)
        {
            const bool rst = !(chip->noise_lfsr & 0xffff) && !chip->noise_bit;
            const bool xr = ((chip->noise_lfsr >> 2) & 1) ^ chip->noise_bit;
            bit = rst || xr;
        }
        chip->noise_bit = chip->noise_lfsr & 1;
    }
    else
        bit = chip->noise_lfsr & 1;
    chip->noise_lfsr >>= 1;
    if(bit)
        chip->noise_lfsr |= 1u << 15;
}

INLINE void OPM_NoiseTimer(opm_t* const chip)
{
    uint32_t timer = chip->noise_timer;

    chip->noise_update = chip->noise_timer_of;

    if ((chip->cycles & 15) == 15)
    {
        timer++;
        timer &= 31;
    }
    if (chip->ic || (chip->noise_timer_of && ((chip->cycles & 15) == 15)))
    {
        timer = 0;
    }

    chip->noise_timer_of = chip->noise_timer == (chip->noise_freq ^ 31);
    chip->noise_timer = timer;
}

INLINE void OPM_DoTimerA(opm_t* const chip)
{
    uint16_t value = chip->timer_a_val;
    value += chip->timer_a_inc;
    chip->timer_a_of = value & ((uint16_t)1<<10);
    if (chip->timer_a_do_reset)
    {
        value = 0;
    }
    if (chip->timer_a_do_load)
    {
        value = chip->timer_a_reg;
    }

    chip->timer_a_val = value & 1023;
}

INLINE void OPM_DoTimerA2(opm_t* const chip)
{
    if (chip->cycles == 1)
    {
        chip->timer_a_load = chip->timer_loada;
    }
    chip->timer_a_inc = chip->mode_test[2] || (chip->timer_a_load && chip->cycles == 0);
    chip->timer_a_do_load = chip->timer_a_of || (chip->timer_a_load && chip->timer_a_temp);
    chip->timer_a_do_reset = chip->timer_a_temp;
    chip->timer_a_temp = !chip->timer_a_load;
    if (chip->timer_reseta || chip->ic)
    {
        chip->timer_a_status = 0;
    }
    else
    {
        chip->timer_a_status |= chip->timer_irqa && chip->timer_a_of;
    }
    chip->timer_reseta = 0;
}

INLINE void OPM_DoTimerB(opm_t* const chip)
{
    uint16_t value = chip->timer_b_val;
    value += chip->timer_b_inc;
    chip->timer_b_of = value & ((uint16_t)1<<8);
    if (chip->timer_b_do_reset)
    {
        value = 0;
    }
    if (chip->timer_b_do_load)
    {
        value = chip->timer_b_reg;
    }

    chip->timer_b_val = value;

    if (chip->cycles == 0)
    {
        chip->timer_b_sub++;
    }

    if (chip->opp)
    {
        chip->timer_b_sub_of = chip->timer_b_sub & ((uint8_t)1<<5);
        chip->timer_b_sub &= 31;
    }
    else
    {
        chip->timer_b_sub_of = chip->timer_b_sub & ((uint8_t)1<<4);
        chip->timer_b_sub &= 15;
    }
    if (chip->ic)
    {
        chip->timer_b_sub = 0;
    }
}

INLINE void OPM_DoTimerB2(opm_t* const chip)
{
    chip->timer_b_inc = chip->mode_test[2] || (chip->timer_loadb && chip->timer_b_sub_of);
    chip->timer_b_do_load = chip->timer_b_of || (chip->timer_loadb && chip->timer_b_temp);
    chip->timer_b_do_reset = chip->timer_b_temp;
    chip->timer_b_temp = !chip->timer_loadb;
    if (chip->timer_resetb || chip->ic)
    {
        chip->timer_b_status = 0;
    }
    else
    {
        chip->timer_b_status |= chip->timer_irqb && chip->timer_b_of;
    }
    chip->timer_resetb = 0;
}

INLINE void OPM_DoTimerIRQ(opm_t* const chip)
{
#ifdef PINMAME
    const bool old_timer_irq = chip->timer_irq;
#endif
    chip->timer_irq = chip->timer_a_status || chip->timer_b_status;
#ifdef PINMAME
    if ((chip->timer_irq != old_timer_irq) && (chip->irqhandler)) (*chip->irqhandler)(chip->timer_irq);
#endif
}

INLINE void OPM_DoLFOMult(opm_t* const chip)
{
    const bool ampm_sel = (chip->lfo_bit_counter & 8);
    const uint8_t dp = ampm_sel ? chip->lfo_pmd : chip->lfo_amd;
    bool bit, b1, b2;
    uint8_t sum;

    chip->lfo_out2_b = chip->lfo_out2;

    switch (chip->lfo_bit_counter & 7)
    {
    case 0:
        bit = (dp & 64) && (chip->lfo_out1 & 64);
        break;
    case 1:
        bit = (dp & 32) && (chip->lfo_out1 & 32);
        break;
    case 2:
        bit = (dp & 16) && (chip->lfo_out1 & 16);
        break;
    case 3:
        bit = (dp &  8) && (chip->lfo_out1 & 8);
        break;
    case 4:
        bit = (dp &  4) && (chip->lfo_out1 & 4);
        break;
    case 5:
        bit = (dp &  2) && (chip->lfo_out1 & 2);
        break;
    case 6:
        bit = (dp &  1) && (chip->lfo_out1 & 1);
        break;
    default:
    case 7:
        bit = 0;
        break;
    }

    b1 = !(chip->lfo_bit_counter & 7) ? 0 : (chip->lfo_out2 & 1);
    b2 = (chip->cycles & 15) == 15 ? 0 : chip->lfo_mult_carry;
    sum = bit + b1 + b2;
    chip->lfo_out2 >>= 1;
    chip->lfo_out2 |= (uint32_t)(sum & 1) << 15;
    chip->lfo_mult_carry = (sum & 2);
}

INLINE void OPM_DoLFO1(opm_t* const chip)
{
    uint16_t counter2 = chip->lfo_counter2;
    const bool of_old = chip->lfo_counter2_of;
    bool lfo_bit, carry;
    uint8_t sum;
    bool bb, sb, x, w2, w3, mulm, mb;
    bool lfo_pm_sign;
    const bool ampm_sel = (chip->lfo_bit_counter & 8);
    counter2 += (chip->lfo_counter1_of1 & 2) || chip->mode_test[3];
    chip->lfo_counter2_of = counter2 & ((uint16_t)1<<15);
    if (chip->ic)
    {
        counter2 = 0;
    }
    if (chip->lfo_counter2_load)
    {
        counter2 = lfo_counter2_table[chip->lfo_freq_hi];
    }
    chip->lfo_counter2 = counter2 & 32767;
    chip->lfo_counter2_load = chip->lfo_frq_update || of_old;
    chip->lfo_frq_update = 0;
    if ((chip->cycles & 15) == 12)
    {
        chip->lfo_counter1++;
    }
    chip->lfo_counter1_of1 <<= 1;
    chip->lfo_counter1_of1 |= (chip->lfo_counter1 >> 4) & 1;
    chip->lfo_counter1 &= 15;
    if (chip->ic)
    {
        chip->lfo_counter1 = 0;
    }

    if ((chip->cycles & 15) == 5)
    {
        chip->lfo_counter2_of_lock2 = chip->lfo_counter2_of_lock;
    }

    chip->lfo_counter3 += chip->lfo_counter3_clock;
    if (chip->ic)
    {
        chip->lfo_counter3 = 0;
    }

    chip->lfo_counter3_clock = (chip->cycles & 15) == 13 && chip->lfo_counter2_of_lock2;

    if ((chip->cycles & 15) == 15)
    {
        chip->lfo_trig_sign = (chip->lfo_val & 0x80);
        chip->lfo_saw_sign = (chip->lfo_val & 0x100);
    }

    lfo_pm_sign = chip->lfo_wave == 2 ? chip->lfo_trig_sign : chip->lfo_saw_sign;


    x = chip->lfo_clock && chip->lfo_wave != 3 && (chip->cycles & 15) == 15;
    w2 = chip->lfo_wave == 2 && x;
    w3 = !chip->ic && !chip->mode_test[1] && (!chip->lfo_clock_lock || chip->lfo_wave != 3) && (chip->lfo_val & 0x8000);

    mulm = ((chip->cycles + 1) & 15) < 8;

    bb = ampm_sel ? chip->lfo_saw_sign : (chip->lfo_wave != 2 || !chip->lfo_trig_sign);
    bb ^= w3;

    sb = ampm_sel ? ((chip->cycles & 15) == 6) : !chip->lfo_saw_sign;

    mb = mulm && (chip->lfo_wave == 1 ? sb : bb);

    chip->lfo_out1 <<= 1;
    chip->lfo_out1 |= mb;

    carry = x || ((chip->cycles & 15) != 15 && chip->lfo_val_carry && chip->lfo_wave != 3);
    sum = carry + w2 + w3;
    lfo_bit = sum & 1;
    if (chip->lfo_wave == 3 && chip->lfo_clock_lock)
    {
        const bool noise = chip->noise_lfsr & 1;
        lfo_bit |= noise;
    }
    chip->lfo_val_carry = sum >> 1;
    chip->lfo_val <<= 1;
    chip->lfo_val |= lfo_bit;


    if ((chip->cycles & 15) == 15 && (chip->lfo_bit_counter & 7) == 7)
    {
        const uint8_t tmp = (chip->lfo_out2_b >> 8);
        if (ampm_sel)
        {
            chip->lfo_pm_lock = tmp;
            if (lfo_pm_sign)
                chip->lfo_pm_lock ^= (uint8_t)1 << 7;
        }
        else
        {
            chip->lfo_am_lock = tmp;
        }
    }

    if ((chip->cycles & 15) == 14)
    {
        chip->lfo_bit_counter++;
    }
    if ((chip->cycles & 15) != 12 && chip->lfo_counter1_of2)
    {
        chip->lfo_bit_counter = 0;
    }
    chip->lfo_counter1_of2 = chip->lfo_counter1 == 2;
}

INLINE void OPM_DoLFO2(opm_t* const chip)
{
    chip->lfo_clock_test = chip->lfo_clock;
    chip->lfo_clock = (chip->lfo_counter2_of || chip->lfo_test || chip->lfo_counter3_step);
    if ((chip->cycles & 15) == 14)
    {
        chip->lfo_counter2_of_lock = chip->lfo_counter2_of;
        chip->lfo_clock_lock = chip->lfo_clock;
    }
    chip->lfo_counter3_step = 0;
    if (chip->lfo_counter3_clock)
    {
        if (!(chip->lfo_counter3 & 1))
        {
            chip->lfo_counter3_step = (chip->lfo_freq_lo & 8);
        }
        else if (!(chip->lfo_counter3 & 2))
        {
            chip->lfo_counter3_step = (chip->lfo_freq_lo & 4);
        }
        else if (!(chip->lfo_counter3 & 4))
        {
            chip->lfo_counter3_step = (chip->lfo_freq_lo & 2);
        }
        else if (!(chip->lfo_counter3 & 8))
        {
            chip->lfo_counter3_step = (chip->lfo_freq_lo & 1);
        }
    }
    chip->lfo_test = chip->mode_test[2];
}

INLINE void OPM_CSM(opm_t* const chip)
{
    chip->kon_csm = chip->kon_csm_lock;
    if (chip->cycles == 1)
    {
        chip->kon_csm_lock = chip->timer_a_do_load && chip->mode_csm;
    }
}

INLINE void OPM_NoiseChannel(opm_t* const chip)
{
    chip->nc_active |= chip->eg_serial_bit;
    if (chip->cycles == 13)
    {
        chip->nc_active = 0;
    }
    chip->nc_out <<= 1;
    chip->nc_out |= chip->nc_sign != chip->eg_serial_bit;
    chip->nc_sign = !chip->nc_sign_lock;
    if (chip->cycles == 12)
    {
        chip->nc_active_lock = chip->nc_active;
        chip->nc_sign_lock2 = chip->nc_active_lock && !chip->nc_sign_lock;
        chip->nc_sign_lock = (chip->noise_lfsr & 1);

        if (chip->noise_en)
        {
            if (chip->nc_sign_lock2)
            {
                chip->op_mix = ((chip->nc_out & ~1) << 2) | -4089;
            }
            else
            {
                chip->op_mix = ((chip->nc_out & ~1) << 2);
            }
        }
    }
}

INLINE void OPM_DoIO(opm_t* const chip)
{
    // Busy
    chip->write_busy_cnt += chip->write_busy;
    chip->write_busy = (!(chip->write_busy_cnt >> 5) && chip->write_busy && !chip->ic) || chip->write_d_en;
    chip->write_busy_cnt &= 0x1f;
    if (chip->ic)
    {
        chip->write_busy_cnt = 0;
    }
    // Write signal check
    chip->write_a_en = chip->write_a;
    chip->write_d_en = chip->write_d;
    chip->write_a = 0;
    chip->write_d = 0;
}

INLINE void OPM_DoRegWrite(opm_t* const chip)
{
    const uint32_t cycles = chip->opp ? (chip->cycles + 1) & 31 : chip->cycles;
    const uint32_t channel = cycles & 7;
    const uint32_t slot = cycles;
#ifdef PINMAME
    bool signal_porthandler = 0;
#endif

    if (chip->opp)
    {
        const uint32_t channel_d1 = (cycles + 7) & 7;
        const uint32_t channel_d4 = (cycles + 4) & 7;
        if (chip->mode_test[4])
        {
            // Clear registers
            chip->ch_ramp_div[channel] = 0;
            chip->ch_rl[channel_d4] = 0;
            chip->ch_fb[channel_d4] = 0;
            chip->ch_connect[channel_d4] = 0;
            chip->ch_kc[channel_d1] = 0;
            chip->ch_kf[channel_d1] = 0;
            chip->ch_pms[channel] = 0;
            chip->ch_ams[channel] = 0;
            
            chip->sl_dt1[slot] = 0;
            chip->sl_mul[slot] = 0;
            chip->sl_tl[slot] = 0;
            chip->sl_ks[slot] = 0;
            chip->sl_ar[slot] = 0;
            chip->sl_am_e[slot] = 0;
            chip->sl_d1r[slot] = 0;
            chip->sl_dt2[slot] = 0;
            chip->sl_d2r[slot] = 0;
            chip->sl_d1l[slot] = 0;
            chip->sl_rr[slot] = 0;
        }
        else
        {
            if (chip->reg_20_delay & 8) // RL, FB, CONNECT
            {
                chip->ch_rl[channel_d4] = chip->reg_data >> 6;
                chip->ch_fb[channel_d4] = (chip->reg_data >> 3) & 0x07;
                chip->ch_connect[channel_d4] = chip->reg_data & 0x07;
            }
            if (chip->reg_28_delay) // KC
            {
                chip->ch_kc[channel_d1] = chip->reg_data & 0x7f;
            }
            if (chip->reg_30_delay) // KF
            {
                chip->ch_kf[channel_d1] = chip->reg_data >> 2;
            }
            // Register write
            if (chip->reg_data_ready)
            {
                // Channel
                if (chip->reg_address == channel) // Ramp div
                {
                    chip->ch_ramp_div[channel] = chip->reg_data;
                }
                if (chip->reg_address == (0x38 | channel)) // PMS, AMS
                {
                    chip->ch_pms[channel] = (chip->reg_data >> 4) & 0x07;
                    chip->ch_ams[channel] = chip->reg_data & 0x03;
                }
                // Slot
                if ((chip->reg_address & 0x1f) == slot)
                {
                    switch (chip->reg_address & 0xe0)
                    {
                    case 0x40: // DT1, MUL
                        chip->sl_dt1[slot] = (chip->reg_data >> 4) & 0x07;
                        chip->sl_mul[slot] = chip->reg_data & 0x0f;
                        break;
                    case 0x60: // TL
                        chip->sl_tl[slot] = chip->reg_data;
                        break;
                    case 0x80: // KS, AR
                        chip->sl_ks[slot] = chip->reg_data >> 6;
                        chip->sl_ar[slot] = chip->reg_data & 0x1f;
                        break;
                    case 0xa0: // AMS-EN, D1R
                        chip->sl_am_e[slot] = chip->reg_data >> 7;
                        chip->sl_d1r[slot] = chip->reg_data & 0x1f;
                        break;
                    case 0xc0: // DT2, D2R
                        chip->sl_dt2[slot] = chip->reg_data >> 6;
                        chip->sl_d2r[slot] = chip->reg_data & 0x1f;
                        break;
                    case 0xe0: // D1L, RR
                        chip->sl_d1l[slot] = chip->reg_data >> 4;
                        chip->sl_rr[slot] = chip->reg_data & 0x0f;
                        break;
                    default:
                        break;
                    }
                }
            }
        }

        chip->reg_20_delay <<= 1;
        chip->reg_20_delay |= chip->reg_data_ready && chip->reg_address == (0x20 | channel);
        chip->reg_28_delay = chip->reg_data_ready && chip->reg_address == (0x28 | channel);
        chip->reg_30_delay = chip->reg_data_ready && chip->reg_address == (0x30 | channel);
    }
    else
    {
        // Register write
        if (chip->reg_data_ready)
        {
            // Channel
            if ((chip->reg_address & 0xe7) == (0x20 | channel))
            {
                switch (chip->reg_address & 0x18)
                {
                case 0x00: // RL, FB, CONNECT
                    chip->ch_rl[channel] = chip->reg_data >> 6;
                    chip->ch_fb[channel] = (chip->reg_data >> 3) & 0x07;
                    chip->ch_connect[channel] = chip->reg_data & 0x07;
                    break;
                case 0x08: // KC
                    chip->ch_kc[channel] = chip->reg_data & 0x7f;
                    break;
                case 0x10: // KF
                    chip->ch_kf[channel] = chip->reg_data >> 2;
                    break;
                case 0x18: // PMS, AMS
                    chip->ch_pms[channel] = (chip->reg_data >> 4) & 0x07;
                    chip->ch_ams[channel] = chip->reg_data & 0x03;
                    break;
                default:
                    break;
                }
            }
            // Slot
            if ((chip->reg_address & 0x1f) == slot)
            {
                switch (chip->reg_address & 0xe0)
                {
                case 0x40: // DT1, MUL
                    chip->sl_dt1[slot] = (chip->reg_data >> 4) & 0x07;
                    chip->sl_mul[slot] = chip->reg_data & 0x0f;
                    break;
                case 0x60: // TL
                    chip->sl_tl[slot] = chip->reg_data & 0x7f;
                    break;
                case 0x80: // KS, AR
                    chip->sl_ks[slot] = chip->reg_data >> 6;
                    chip->sl_ar[slot] = chip->reg_data & 0x1f;
                    break;
                case 0xa0: // AMS-EN, D1R
                    chip->sl_am_e[slot] = chip->reg_data >> 7;
                    chip->sl_d1r[slot] = chip->reg_data & 0x1f;
                    break;
                case 0xc0: // DT2, D2R
                    chip->sl_dt2[slot] = chip->reg_data >> 6;
                    chip->sl_d2r[slot] = chip->reg_data & 0x1f;
                    break;
                case 0xe0: // D1L, RR
                    chip->sl_d1l[slot] = chip->reg_data >> 4;
                    chip->sl_rr[slot] = chip->reg_data & 0x0f;
                    break;
                default:
                    break;
                }
            }
        }
    }


    // Mode write
    if (chip->write_d_en)
    {
        if (chip->mode_address == (chip->opp ? 9 : 1))
        {
            int32_t i;
            for (i = 0; i < 8; i++)
            {
                chip->mode_test[i] = (chip->write_data >> i) & 0x01;
            }
        }
        switch (chip->mode_address)
        {
        case 0x08:
        {
            int32_t i;
            for (i = 0; i < 4; i++)
            {
                chip->mode_kon_operator[i] = (chip->write_data >> (i + 3)) & 0x01;
            }
            chip->mode_kon_channel = chip->write_data & 0x07;
            break;
        }
        case 0x0f:
            chip->noise_en = chip->write_data >> 7;
            chip->noise_freq = chip->write_data & 0x1f;
            break;
        case 0x10:
            chip->timer_a_reg &= 0x03;
            chip->timer_a_reg |= chip->write_data << 2;
            break;
        case 0x11:
            chip->timer_a_reg &= 0x3fc;
            chip->timer_a_reg |= chip->write_data & 0x03;
            break;
        case 0x12:
            chip->timer_b_reg = chip->write_data;
            break;
        case 0x14:
            chip->mode_csm     = chip->write_data & ((uint8_t)1<<7);
            chip->timer_irqb   = chip->write_data & ((uint8_t)1<<3);
            chip->timer_irqa   = chip->write_data & ((uint8_t)1<<2);
            chip->timer_resetb = chip->write_data & ((uint8_t)1<<5);
            chip->timer_reseta = chip->write_data & ((uint8_t)1<<4);
            chip->timer_loadb  = chip->write_data & ((uint8_t)1<<1);
            chip->timer_loada  = chip->write_data & ((uint8_t)1<<0);
            break;
        case 0x18:
            chip->lfo_freq_hi = chip->write_data >> 4;
            chip->lfo_freq_lo = chip->write_data & 0x0f;
            chip->lfo_frq_update = 1;
            break;
        case 0x19:
            if (chip->write_data & 0x80)
            {
                chip->lfo_pmd = chip->write_data & 0x7f;
            }
            else
            {
                chip->lfo_amd = chip->write_data;
            }
            break;
        case 0x1b:
            chip->lfo_wave = chip->write_data &  (uint8_t)0x03;
            chip->io_ct1   = chip->write_data & ((uint8_t)1<<6);
            chip->io_ct2   = chip->write_data & ((uint8_t)1<<7);
#ifdef PINMAME
            signal_porthandler = 1;
#endif
            break;
        }
    }

    // Register data write
    chip->reg_data_ready = chip->reg_data_ready && !chip->write_a_en;
    if (chip->reg_address_ready && chip->write_d_en)
    {
        chip->reg_data = chip->write_data;
        chip->reg_data_ready = 1;
    }

    // Register address write
    chip->reg_address_ready = chip->reg_address_ready && !chip->write_a_en;
    if (chip->write_a_en && ((chip->write_data & 0xe0) || (chip->opp && !(chip->write_data & 0xf8))))
    {
        chip->reg_address = chip->write_data;
        chip->reg_address_ready = 1;
    }
    if (chip->write_a_en)
    {
        chip->mode_address = chip->write_data;
    }

#ifdef PINMAME
    if (chip->porthandler && signal_porthandler) (*chip->porthandler)(0, chip->write_data >> 6);
#endif
}

INLINE void OPM_DoIC(opm_t* const chip)
{
    const uint32_t channel = chip->cycles & 7;
    const uint32_t slot = chip->cycles;
    if (chip->ic)
    {
        if (chip->opp)
        {
            uint32_t i;
            for (i = 0; i < 2; i++)
            {
                const uint8_t ch = (channel + 4 * i) & 7;
                chip->ch_ramp_div[ch] = 0;
                chip->ch_rl[ch] = 0;
                chip->ch_fb[ch] = 0;
                chip->ch_connect[ch] = 0;
                chip->ch_kc[ch] = 0;
                chip->ch_kf[ch] = 0;
                chip->ch_pms[ch] = 0;
                chip->ch_ams[ch] = 0;
            }
            for (i = 0; i < 8; i++)
            {
                const uint8_t sl = (slot + 4 * i) & 31;
                chip->sl_dt1[sl] = 0;
                chip->sl_mul[sl] = 0;
                chip->sl_tl[sl] = 0;
                chip->sl_ks[sl] = 0;
                chip->sl_ar[sl] = 0;
                chip->sl_am_e[sl] = 0;
                chip->sl_d1r[sl] = 0;
                chip->sl_dt2[sl] = 0;
                chip->sl_d2r[sl] = 0;
                chip->sl_d1l[sl] = 0;
                chip->sl_rr[sl] = 0;
            }
        }
        else
        {
            chip->ch_rl[channel] = 0;
            chip->ch_fb[channel] = 0;
            chip->ch_connect[channel] = 0;
            chip->ch_kc[channel] = 0;
            chip->ch_kf[channel] = 0;
            chip->ch_pms[channel] = 0;
            chip->ch_ams[channel] = 0;

            chip->sl_dt1[slot] = 0;
            chip->sl_mul[slot] = 0;
            chip->sl_tl[slot] = 0;
            chip->sl_ks[slot] = 0;
            chip->sl_ar[slot] = 0;
            chip->sl_am_e[slot] = 0;
            chip->sl_d1r[slot] = 0;
            chip->sl_dt2[slot] = 0;
            chip->sl_d2r[slot] = 0;
            chip->sl_d1l[slot] = 0;
            chip->sl_rr[slot] = 0;
        }

        chip->timer_a_reg = 0;
        chip->timer_b_reg = 0;
        chip->timer_irqa = 0;
        chip->timer_irqb = 0;
        chip->timer_loada = 0;
        chip->timer_loadb = 0;
        chip->mode_csm = 0;

        chip->mode_test[0] = 0;
        chip->mode_test[1] = 0;
        chip->mode_test[2] = 0;
        chip->mode_test[3] = 0;
        chip->mode_test[4] = 0;
        chip->mode_test[5] = 0;
        chip->mode_test[6] = 0;
        chip->mode_test[7] = 0;
        chip->noise_en = 0;
        chip->noise_freq = 0;

        chip->mode_kon_channel = 0;
        chip->mode_kon_operator[0] = 0;
        chip->mode_kon_operator[1] = 0;
        chip->mode_kon_operator[2] = 0;
        chip->mode_kon_operator[3] = 0;
        chip->mode_kon[(slot + 8) & 31] = 0;

        chip->lfo_pmd = 0;
        chip->lfo_amd = 0;
        chip->lfo_wave = 0;
        chip->lfo_freq_hi = 0;
        chip->lfo_freq_lo = 0;

        chip->io_ct1 = 0;
        chip->io_ct2 = 0;
#ifdef PINMAME
        if (chip->porthandler) (*chip->porthandler)(0, 0); //!! correct?
#endif

        chip->reg_address = 0;
        chip->reg_data = 0;
    }
    chip->ic2 = chip->ic;
}

INLINE void OPP_TLRamp(opm_t* const chip)
{
    const uint32_t slot = chip->cycles;
    const uint32_t channel = slot & 7;
    const bool step = ((chip->cycles + 1) & 31) < 8;
    const uint8_t ramp = chip->ch_ramp_div[channel];

    const bool match = (ramp == chip->opp_tl_cnt[channel]);
    uint8_t tl = chip->sl_tl[slot];

    if (chip->ic || (step && match))
        chip->opp_tl_cnt[channel] = 0;
    else
        chip->opp_tl_cnt[channel] += step;

    if (tl & 128)
    {
        if (match)
        {
            const uint16_t val = chip->opp_tl[slot] >> 3;
            tl &= 127;
            if (val < tl)
                chip->opp_tl[slot]++;
            else if (val > tl)
                chip->opp_tl[slot]--;
        }
    }
    else
        chip->opp_tl[slot] = (uint16_t)tl << 3;
}

void OPM_Clock(opm_t* const chip, int16_t * const output, bool * const sh1, bool * const sh2, bool * const so)
{
    OPM_Output(chip);
    OPM_DAC(chip);
    OPM_Mixer2(chip);
    OPM_Mixer(chip);

    OPM_OperatorPhase16(chip);
    OPM_OperatorPhase15(chip);
    OPM_OperatorPhase14(chip);
    OPM_OperatorPhase13(chip);
    OPM_OperatorPhase12(chip);
    OPM_OperatorPhase11(chip);
    OPM_OperatorPhase10(chip);
    OPM_OperatorPhase9(chip);
    OPM_OperatorPhase8(chip);
    OPM_OperatorPhase7(chip);
    OPM_OperatorPhase6(chip);
    OPM_OperatorPhase5(chip);
    OPM_OperatorPhase4(chip);
    OPM_OperatorPhase3(chip);
    OPM_OperatorPhase2(chip);
    OPM_OperatorPhase1(chip);
    OPM_OperatorCounter(chip);

    OPM_EnvelopeTimer(chip);
    OPM_EnvelopePhase6(chip);
    OPM_EnvelopePhase5(chip);
    OPM_EnvelopePhase4(chip);
    OPM_EnvelopePhase3(chip);
    OPM_EnvelopePhase2(chip);
    OPM_EnvelopePhase1(chip);

    if (chip->opp)
        OPP_TLRamp(chip);

    OPM_PhaseDebug(chip);
    OPM_PhaseGenerate(chip);
    OPM_PhaseCalcIncrement(chip);
    OPM_PhaseCalcFNumBlock(chip);

    OPM_DoTimerIRQ(chip);
    OPM_DoTimerA(chip);
    OPM_DoTimerB(chip);
    OPM_DoLFOMult(chip);
    OPM_DoLFO1(chip);
    OPM_Noise(chip);
    OPM_KeyOn2(chip);
    OPM_DoRegWrite(chip);
    OPM_EnvelopeClock(chip);
    OPM_NoiseTimer(chip);
    OPM_KeyOn1(chip);
    OPM_DoIO(chip);
    OPM_DoTimerA2(chip);
    OPM_DoTimerB2(chip);
    OPM_DoLFO2(chip);
    OPM_CSM(chip);
    OPM_NoiseChannel(chip);
    OPM_DoIC(chip);
    if (sh1)
    {
        *sh1 = chip->smp_sh1;
    }
    if (sh2)
    {
        *sh2 = chip->smp_sh2;
    }
    if (so)
    {
        *so = chip->smp_so;
    }
    if (output)
    {
        output[0] = chip->dac_output[0];
        output[1] = chip->dac_output[1];
    }
    chip->cycles = (chip->cycles + 1) & 31;
}

void OPM_Write(opm_t* const chip, const uint8_t port, const uint8_t data)
{
#ifdef PINMAME
    vgm_write(chip->vgm_idx, 0x00, port, data);
#endif

    chip->write_data = data;
    if (chip->ic)
    {
        return;
    }
    if (port & 0x01)
    {
        chip->write_d = 1;
    }
    else
    {
        chip->write_a = 1;
    }
}

uint8_t OPM_Read(const opm_t* const chip, const uint8_t port)
{
    if (!(port & 1))
        return 0xff;
    if (chip->mode_test[6])
    {
        const uint16_t testdata = chip->op_out[5] | (((uint16_t)!chip->eg_serial_bit) << 14) | ((chip->pg_serial & 1) << 15);
        if (chip->mode_test[7])
        {
            return testdata;
        }
        else
        {
            return testdata >> 8;
        }
    }
    return ((uint8_t)chip->write_busy << 7) | ((uint8_t)chip->timer_b_status << 1) | chip->timer_a_status;
}

bool OPM_ReadIRQ(const opm_t* const chip)
{
    return chip->timer_irq;
}

bool OPM_ReadCT1(const opm_t* const chip)
{
    if (chip->opp)
        return chip->io_ct2;

    return chip->mode_test[3] ? chip->lfo_clock_test : chip->io_ct1;
}

bool OPM_ReadCT2(const opm_t* const chip)
{
    if (chip->opp)
        return chip->mode_test[3] ? chip->lfo_clock_test : chip->io_ct1;

    return chip->io_ct2;
}

void OPM_SetIC(opm_t* const chip, bool ic)
{
    if (chip->ic != ic)
    {
        chip->ic = ic;
        if (!ic)
        {
            chip->cycles = 0;
        }
    }
}

void OPM_Reset(opm_t* const chip, uint32_t flags, double clock)
{
#ifdef PINMAME
    void(*irqhandler)(int irq) = chip->irqhandler;
    mem_write_handler porthandler = chip->porthandler;
#endif
    uint32_t i;
    memset(chip, 0, sizeof(opm_t));
    chip->opp = (flags & opm_flags_ym2164);
    OPM_SetIC(chip, 1);
    for (i = 0; i < 32 * 64; i++)
    {
        OPM_Clock(chip, NULL, NULL, NULL, NULL);
    }
    OPM_SetIC(chip, 0);
#ifdef PINMAME
    if (clock != 0.) // init?
    {
        chip->vgm_idx = vgm_open(VGMC_YM2151, clock);
    }
    else // or 'real' reset?
    {
        chip->porthandler = porthandler;
        chip->irqhandler = irqhandler;
    }
#endif
}

#ifdef PINMAME
// ported from VGM variant
void OPM_WriteBuffered(opm_t* const chip, uint8_t port, uint8_t data)
{
    uint64_t time1;

    if (chip->writebuf[chip->writebuf_last].port & 0x02)
    {
        uint64_t skip;
        chip->writebuf[chip->writebuf_last].port &= 0x01;
        OPM_Write(chip, chip->writebuf[chip->writebuf_last].port,
                        chip->writebuf[chip->writebuf_last].data);

        chip->writebuf_cur = (chip->writebuf_last + 1) % OPN_WRITEBUF_SIZE;
        skip = chip->writebuf[chip->writebuf_last].time - chip->writebuf_samplecnt;
        chip->writebuf_samplecnt = chip->writebuf[chip->writebuf_last].time;
        while (skip--)
        {
            //int16_t buffer[2];
            OPM_Clock(chip, /*buffer*/NULL, NULL, NULL, NULL);
        }
    }

    chip->writebuf[chip->writebuf_last].port = (port & 0x01) | 0x02;
    chip->writebuf[chip->writebuf_last].data = data;
    time1 = chip->writebuf_lasttime + OPN_WRITEBUF_DELAY;

    if (time1 < chip->writebuf_samplecnt)
    {
        time1 = chip->writebuf_samplecnt;
    }

    chip->writebuf[chip->writebuf_last].time = time1;
    chip->writebuf_lasttime = time1;
    chip->writebuf_last = (chip->writebuf_last + 1) % OPN_WRITEBUF_SIZE;
}

void OPM_FlushBuffer(opm_t* chip)
{
    while (chip->writebuf[chip->writebuf_cur].port & 0x02)
    {
        uint64_t skip;
        chip->writebuf[chip->writebuf_cur].port &= 0x01;
        OPM_Write(chip, chip->writebuf[chip->writebuf_cur].port,
                        chip->writebuf[chip->writebuf_cur].data);

        skip = chip->writebuf[chip->writebuf_cur].time - chip->writebuf_samplecnt;
        chip->writebuf_samplecnt = chip->writebuf[chip->writebuf_cur].time;
        chip->writebuf_cur = (chip->writebuf_cur + 1) % OPN_WRITEBUF_SIZE;

        while (skip--)
        {
            //int16_t buffer[2];
            OPM_Clock(chip, /*buffer*/NULL, NULL, NULL, NULL);
        }
    }
}

INLINE void OPM_GenerateOne(opm_t* const chip, float fbuf[2])
{
    uint32_t i;
    int16_t buf[2];
    for (i = 0; i < 32; i++) // over every 32 cycles a stereo sample is created
    {
        OPM_Clock(chip, (i == 15) ? buf : NULL, NULL, NULL, NULL); // so grab that at some point (where in the middle both channels are in sync for sure)

        while (chip->writebuf[chip->writebuf_cur].time <= chip->writebuf_samplecnt)
        {
            if (!(chip->writebuf[chip->writebuf_cur].port & 0x02))
            {
                break;
            }
            chip->writebuf[chip->writebuf_cur].port &= 0x01;
            OPM_Write(chip, chip->writebuf[chip->writebuf_cur].port,
                            chip->writebuf[chip->writebuf_cur].data);
            chip->writebuf_cur = (chip->writebuf_cur + 1) % OPN_WRITEBUF_SIZE;
        }
        chip->writebuf_samplecnt++;
    }
    fbuf[0] = (float)buf[0];
    fbuf[1] = (float)buf[1];
}

// PinMAME specific
void OPM_GenerateStream(opm_t* const chip, float **sndptr, uint32_t numsamples)
{
    uint32_t i;
    for (i = 0; i < numsamples; i++)
    {
        float buffer[2];
        OPM_GenerateOne(chip, buffer);
        sndptr[0][i] = buffer[0]*(float)(1.0/32768.0);
        sndptr[1][i] = buffer[1]*(float)(1.0/32768.0);
    }
}

void OPM_SetPortWriteHandler(opm_t* const chip, mem_write_handler handler)
{
    chip->porthandler = handler;
}

void OPM_SetIrqHandler(opm_t* const chip, void(*handler)(int irq))
{
    chip->irqhandler = handler;
}
#endif
