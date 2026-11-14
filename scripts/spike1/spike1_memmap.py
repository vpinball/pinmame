#!/usr/bin/env python3
# license:BSD-3-Clause
"""PinMAME memory maps for SPIKE 1 titles, from the game program's own tables.

usage: spike1_memmap.py <game program> <rom> [<rom>...] [-o <map.json>] [--platform <platform.json>] [--free-only]

A SPIKE 1 game is a static ARM Linux program with its symbol table (all titles but Heavy Metal, and
not the ELG titles, Transformers and Avengers The Pin). Its variables sit at fixed addresses, which
libpinmame reads through the driver (core_readMemoryHook): the map's addresses are the program's own.
Everything comes from the program, nothing is typed in by hand:

  game_state      the statics player_up and ball_in_play (a byte each, from 1), total_players,
                  the credit count (byte 4 of cd_default_credit_block_ram), game over (bit 0x10 of
                  sys_gs_flags: set in attract mode, clear from START to the end of the match) and
                  the scores (player_scores_buffer, or player_scores before Ghostbusters: four
                  players, 8 bytes each where the score setter takes an unsigned long long, 4 where
                  it takes an unsigned long)
  high_scores,    hstd_table: its first table (HIGHEST SCORES) and the mode champions after it,
  mode_champions  each entry's block (hstd_entry_table) holding the initials at +0 and the value
                  at +0x18, as wide as the scores
  adjustments     adjustment_table: 32-byte entries, +0 the value's block (an i32, then its check
                  word), +0x18 a table of the name in four languages (English first). Keyed by the
                  table's own index
  audits          audit_display_table (the service menu's order: EARNINGS AUDIT #1 is entry 1) for
                  the audits it shows from a block of audit_table rather than computing them; a
                  block's first u32 is the count. Keyed by the display table's index

The platform file (--platform) is the same for every title. --free-only marks the sets as home
models that always play free (Spider-Man The Pin).
"""
import argparse
import bisect
import json
import struct
import sys


class Program:
    """A static ARM ELF32 program: its symbols and the bytes of its loaded segments."""

    def __init__(self, path):
        self.b = open(path, 'rb').read()
        b = self.b
        if b[:4] != b'\x7fELF' or b[4] != 1 or b[5] != 1:
            raise SystemExit('%s: not a 32-bit little-endian ELF' % path)
        phoff, shoff = struct.unpack_from('<II', b, 0x1c)
        phentsize, phnum, shentsize, shnum = struct.unpack_from('<HHHH', b, 0x2a)
        self.segs = []
        for i in range(phnum):
            t, off, va, pa, fsz, msz, fl, al = struct.unpack_from('<8I', b, phoff + i * phentsize)
            if t == 1:  # PT_LOAD
                self.segs.append((va, off, fsz, msz))
        secs = [struct.unpack_from('<10I', b, shoff + i * shentsize) for i in range(shnum)]
        self.syms = {}
        for s in secs:
            if s[1] != 2:  # SHT_SYMTAB
                continue
            strtab = secs[s[6]]
            for k in range(s[5] // 16):
                name, value, size = struct.unpack_from('<III', b, s[4] + k * 16)
                o = strtab[4] + name
                n = b[o:b.index(b'\0', o)].decode('latin1')
                if n and not n.startswith('$') and (n not in self.syms or (size and not self.syms[n][1])):
                    self.syms[n] = (value, size)
        if not self.syms:
            raise SystemExit('%s: no symbol table (Heavy Metal ships without one)' % path)
        self._sized = sorted((v, s, n) for n, (v, s) in self.syms.items() if s)
        self._starts = [x[0] for x in self._sized]

    def read(self, va, n):
        for sva, off, fsz, msz in self.segs:
            if sva <= va and va + n <= sva + msz:
                rel = va - sva
                data = self.b[off + rel: off + min(rel + n, fsz)] if rel < fsz else b''
                return data + bytes(n - len(data))
        raise ValueError('address %#x not in the program' % va)

    def u32(self, va):
        return struct.unpack('<I', self.read(va, 4))[0]

    def cstr(self, va, limit=64):
        for sva, off, fsz, msz in self.segs:
            if sva <= va < sva + fsz:
                raw = self.b[off + va - sva: off + min(va - sva + limit, fsz)]
                return raw.split(b'\0')[0].decode('latin1')
        return ''

    def sym(self, name):
        if name not in self.syms:
            raise SystemExit('the program has no %s' % name)
        return self.syms[name][0]

    def find(self, suffix):
        """A static by its name: plain, or as a C++ file static (_ZL<length><name>)."""
        for n in (suffix, '_ZL%d%s' % (len(suffix), suffix)):
            if n in self.syms:
                return self.syms[n]
        return None

    def name_at(self, va):
        i = bisect.bisect_right(self._starts, va) - 1
        if i >= 0:
            v, s, n = self._sized[i]
            if v <= va < v + s:
                return n
        return None

    def table(self, descriptor):
        """A table by its 12-byte descriptor: (data address, count, entry size)."""
        return struct.unpack('<III', self.read(self.sym(descriptor), 12))

    def lang(self, ptr):
        """English text of a 4-language name table."""
        return self.cstr(self.u32(ptr)).strip() if ptr else ''


def hexa(v):
    return '0x%08X' % v


def label_case(text):
    """A name as the game shows it (GRAND CHAMPION, HI-SCORE #1) in the maps' style (Grand Champion, Hi-Score #1)."""
    words = text.replace('_', ' ').split(' ')
    return ' '.join('-'.join(part[:1].upper() + part[1:].lower() for part in w.split('-')) for w in words)


def field(label, start, encoding, length=None, **more):
    f = {'label': label, 'start': hexa(start), 'encoding': encoding}
    if length is not None:
        f['length'] = length
    f.update(more)
    return f


def score_width(p):
    for n in p.syms:
        if n.startswith('_Z29sys_score_set_player_up_score'):
            return {'y': 8, 'x': 8, 'm': 4, 'j': 4, 'l': 4, 'i': 4}[n[-1]]
    raise SystemExit('the program has no sys_score_set_player_up_score')


def game_state(p, width):
    gs = {}
    up = p.find('player_up')
    ball = p.find('ball_in_play')
    players = p.find('total_players')
    credit = p.find('cd_default_credit_block_ram')
    gs['player_count'] = field('Player Count', players[0], 'int', players[1])
    gs['current_player'] = field('Player', up[0], 'int', up[1])
    gs['current_ball'] = field('Ball', ball[0], 'int', ball[1])
    gs['credits'] = field('Credits', credit[0] + 4, 'int', 1)
    gs['game_over'] = field('Game Over', p.sym('sys_gs_flags'), 'bool', 1, mask='0x10')
    for key, label, adj in (('free_play', 'Free Play', 'free_play'), ('ball_count', 'Balls Per Game', 'balls_per_game'),
                            ('max_credits', 'Maximum Credits', 'credit_limit')):
        a = p.find('ad_%s_adjustment_block_ram' % adj)
        if a:
            gs[key] = field(label, a[0], 'bool' if key == 'free_play' else 'int', 4)
    scores = p.find('player_scores_buffer') or p.find('player_scores')
    gs['scores'] = [field('Player %d' % (i + 1), scores[0] + i * width, 'int', width) for i in range(4)]
    return gs


def hstd(p, width):
    entries = []
    data, count, size = p.table('hstd_entry_table')
    for i in range(count):
        block, initials, name, f, unused, title = struct.unpack('<6I', p.read(data + i * size, 24))
        entries.append((block, p.lang(title)))
    tables = []
    data, count, size = p.table('hstd_table')
    for i in range(1, count):
        w = struct.unpack('<9I', p.read(data + i * size, 36))
        getter = p.name_at(w[1]) or ''
        lst = p.name_at(w[7])
        n = p.syms[lst][1] // 2 if lst else 0
        idx = [x for x in struct.unpack('<%dH' % n, p.read(w[7], n * 2)) if x] if n else []
        value = 'score' if 'score' in getter else 'counter'
        rows = []
        for k in idx:
            block, label = entries[k]
            rows.append({'label': label_case(label),
                         'initials': {'start': hexa(block), 'encoding': 'ch', 'length': 3, 'null': 'terminate'},
                         value: {'start': hexa(block + 0x18), 'encoding': 'int', 'length': width}})
        tables.append((p.lang(w[6]), rows))
    return tables


def adjustments(p):
    out = {}
    data, count, size = p.table('adjustment_table')
    for i in range(1, count):
        ptr, default, lo, hi, step = struct.unpack('<Iiiii', p.read(data + i * size, 20))
        names = p.u32(data + i * size + 0x18)
        label = p.lang(names) if names else ''
        if not ptr or not label or label == 'INVALID':
            continue
        out['%03d' % i] = field(label, ptr, 'int', 4)
    return {'Adjustments': out}


def audits(p):
    out = {}
    adata, acount, asize = p.table('audit_table')
    data, count, size = p.table('audit_display_table')
    for i in range(1, count):
        func, names, kind_index, flags = struct.unpack('<IIII', p.read(data + i * size, 16))
        label = p.lang(names)
        index = kind_index >> 16
        if func or not index or index >= acount or not label or label.startswith('HIDDEN'):
            continue
        block = p.u32(adata + index * asize)
        if block:
            out['%03d' % i] = field(label, block, 'int', 4)
    return {'Audits': out}


PLATFORM = {
    '_notes': [
        'Stern SPIKE 1: a static ARMv5TE Linux program on an ARM926-class CPU board. PinMAME runs it',
        'in its own memory (src/spike1), not as CPU 0, and reads it for a map through the driver',
        '(core_readMemoryHook). The game\'s variables are at the program\'s fixed addresses: its data',
        'and bss, from 0x8000 below the mmap area at 0x08000000. Settings, audits and high scores',
        'live there too, loaded from the game\'s NVRAM files at boot and saved as they change.',
        'These maps read that live memory. PinMAME\'s .nv file for a SPIKE 1 set is the machine\'s',
        'own NVRAM block (the game\'s files), not this memory, so an .nv parser cannot use them.'
    ],
    'cpu': 'ARM926',
    'endian': 'little',
    'memory_layout': [
        {'label': 'Program data', 'address': '0x00008000', 'size': '0x07FF8000', 'type': 'ram'}
    ]
}


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('program')
    ap.add_argument('roms', nargs='+')
    ap.add_argument('-o', '--output')
    ap.add_argument('--platform', help='also write the platform file here')
    ap.add_argument('--free-only', action='store_true', help='the sets are home models that always play free')
    a = ap.parse_args()
    p = Program(a.program)
    width = score_width(p)
    tables = hstd(p, width)
    m = {
        '_notes': [
            'Generated by PinMAME\'s scripts/spike1/spike1_memmap.py from the game program\'s own',
            'tables and symbols. Adjustments are keyed by the game\'s adjustment table index, audits by',
            'its audit display table index (the service menu\'s order); values of list-type',
            'adjustments are their index in the list.'
        ],
        '_fileformat': 0.9,
        '_metadata': {
            'version': 1,
            'license': 'Open Data Commons Open Database License (ODbL) v1.0',
            'platform': 'stern-spike1',
            'roms': a.roms
        },
        'game_state': game_state(p, width),
        'high_scores': tables[0][1] if tables else [],
        'mode_champions': [row for title, rows in tables[1:] for row in rows],
        'adjustments': adjustments(p),
        'audits': audits(p)
    }
    if a.free_only:
        m['_metadata']['free_only'] = a.roms
    text = json.dumps(m, indent=2) + '\n'
    if a.output:
        open(a.output, 'w', newline='\n').write(text)
    else:
        sys.stdout.write(text)
    if a.platform:
        open(a.platform, 'w', newline='\n').write(json.dumps(PLATFORM, indent=2) + '\n')


if __name__ == '__main__':
    main()
