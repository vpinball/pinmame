#!/usr/bin/env python3
"""Turn cirsa_sound_sweep.sh's captures into a per-element sound table.

  scripts/cirsa_sound_table.py CAPDIR [--game NAME]

Adjudication rules, kept the same as docs/findings/2026-09-04-switch-sounds.md
so the two are comparable:

  * pass 1 closes nothing and is the control; passes 2-5 are the stimulus;
  * a command counts for an element only if it lands in that element's slot in
    at least 2 of the 4 stimulus passes -- one hit is coincidence;
  * a command attributed to >= 8 unrelated elements inside one capture is
    background for that capture and is dropped (Mephisto re-queues 0x31 every
    ~3 s from a self-rescheduling task at 0x43CE; Sport 2000's equivalent is
    0x1F, "stop everything");
  * an element SOUNDS when the long-period capture attributes a command to it.
    That capture's pass period is 432 s against the others' 183.6 s, so a ROM
    timer cannot land in the same element's slot in both.  When only the
    1.7 s-slot captures attribute a command, the element is "short-only": that
    is where the coin chutes land, because a chute held past the ROM's
    stuck-coin window is refused.
"""
import argparse, collections, os, re, sys

CMD = re.compile(r"SW cmd f=(\d+) el=(-?\d+) cmd=([0-9A-F]{2})")
SLOT = re.compile(r"SW slot p=(\d+) s=(\d+) el=(-?\d+) ")
LVL = re.compile(r"SW lvl el=(\d+) seen=(\d) raw=([0-9A-F]{2})")
MINPASS = 2
ELEMS = 68


def load(path):
    """-> ({el: {cmd: passes}}, background set, {el: closures the ROM saw})"""
    raw = collections.defaultdict(lambda: collections.defaultdict(set))
    seen = collections.defaultdict(list)
    p = 0
    for line in open(path, errors="replace"):
        m = SLOT.search(line)
        if m:
            p = int(m.group(1))
            continue
        m = CMD.search(line)
        if m:
            el = int(m.group(2))
            if el >= 0 and p > 0:
                raw[el][m.group(3)].add(p)
            continue
        m = LVL.search(line)
        if m:
            seen[int(m.group(1))].append(int(m.group(2)))
    per = collections.defaultdict(dict)
    for el, cc in raw.items():
        for c, passes in cc.items():
            if len(passes) >= MINPASS:
                per[el][c] = len(passes)
    spread = collections.Counter()
    for el in per:
        for c in per[el]:
            spread[c] += 1
    bg = {c for c, n in spread.items() if n >= 8}
    for el in list(per):
        for c in list(per[el]):
            if c in bg:
                del per[el][c]
    return per, bg, seen


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("capdir")
    ap.add_argument("--game", action="append")
    a = ap.parse_args()
    games = a.game or ["sport2k", "mephisto", "mephist1"]
    for g in games:
        tags = sorted(
            (int(m.group(1)), m.group(0))
            for f in os.listdir(a.capdir)
            for m in [re.fullmatch(re.escape(g) + r"\.c(\d+)\.log", f)]
            if m)
        if not tags:
            continue
        caps = {ms: load(os.path.join(a.capdir, f)) for ms, f in tags}
        order = [ms for ms, _ in tags]
        slow = max(order)
        print(f"\n===== {g} =====")
        print("background (spread>=8): " +
              ", ".join(f"{ms}:{sorted(caps[ms][1])}" for ms in order))
        print(f"{'el':>3} | " + " | ".join(f"{ms:>14}" for ms in order) + " | verdict")
        sounds, short = [], []
        for el in range(ELEMS):
            cells = [",".join(f"{c}/{n}" for c, n in sorted(caps[ms][0].get(el, {}).items()))
                     or "-" for ms in order]
            if caps[slow][0].get(el):
                v = "SOUNDS"; sounds.append(el)
            elif any(caps[ms][0].get(el) for ms in order if ms != slow):
                v = "short-only"; short.append(el)
            else:
                v = "silent"
            print(f"{el:>3} | " + " | ".join(f"{c:>14}" for c in cells) + f" | {v}")
        print(f"-- SOUNDS ({len(sounds)}): {sounds}")
        print(f"-- short-only ({len(short)}): {short}")
        # stimulus check: every closure should show up in the ROM's own level
        # array, except the trough switches, which rest closed and are pulsed open.
        never = sorted(el for el, s in caps[order[0]][2].items() if not any(s))
        if caps[order[0]][2]:
            print(f"-- level array never read closed (trough is expected here): {never}")


if __name__ == "__main__":
    sys.exit(main())
