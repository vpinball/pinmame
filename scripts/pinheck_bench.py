#!/usr/bin/env python3
"""Speed benchmark for the pinHeck games (Domino's, Rob Zombie's Spookshow, The Jetsons, America's Most Haunted).

Runs fixed, repeatable workloads on one or more builds of PinMAME.exe or sdl3pinmame, alternating the builds so that
load on the host affects all of them alike, and prints the speed per workload, the gain over the first build and the
geometric mean. Speed is emulated time / host time from the workload's mark frame on, read from PINHECK_TIME_LOG.

The builds need the pinHeck test hooks, which are off by default:
  MSVC:        set _CL_=/DPINHECK_TEST_HOOKS before building
  GCC, Clang:  cmake ... -DCMAKE_C_FLAGS=-DPINHECK_TEST_HOOKS
The rompath must hold pinheck.zip and the game zips; the set names default to the current ones (--sets changes them).

  pinheck_bench.py bench  --rompath ROMS [--runs 2] [--modes default,single] [--workloads attract,video,play]
                          [--games dominos,rzspook,jetsons,amh] NAME=EXE ...
  pinheck_bench.py check  --rompath ROMS NAME=EXE NAME=EXE
  pinheck_bench.py train  --rompath ROMS [--collect DIR] EXE

bench   workloads per game: attract (2400 frames, timed from frame 600), video (UART1 commands that play video
        clips, 2400 frames from 960) and play (coin, start, plunge and a shot every 1.5 s, the ball never drains;
        not for Domino's). Mode single sets PINHECK_THREADS=0 (no Propeller worker thread).
check   runs Domino's attract with each build and compares UART1, frame and NVRAM output, which must be identical.
train   runs attract and video of all games, as training for a PINMAME_PGO=GEN build; --collect moves MSVC's .pgc
        files from the executable's directory to DIR (PINMAME_PGO_DIR). With Clang, merge the .profraw files with
        llvm-profdata afterwards.

Each run starts from a copy of an NVRAM made by a 1200 frame boot in service mode, with the clock set to a fixed
time (PINHECK_RTC), runs unthrottled without sound, and its files are kept in --workdir.
"""
import argparse
import hashlib
import math
import os
import shutil
import subprocess
import sys
import tempfile
import time

GAMES = ["dominos", "rzspook", "jetsons", "amh"]
SETS = {"dominos": "dominos_006", "rzspook": "rzspook_026", "jetsons": "jetsons_004", "amh": "amh_023"}
RTC = "1790683200"

VIDEO = {
    "dominos": (10, "[E96000]~[V00AT9]~[F00ZM0]~[F00BWI]~[F00IF0]~[F00NBI]~~~~~~~~~~[F00NBI]~~~~[F00IF0]"),
    "rzspook": (10, "[E96000]~[V00AT9]~[F00ZM0]~[F00ZAA]~[F00ZAB]~[F00ZAC]~~~~~~~~~~[F00ZAD]~~~~[F00ZAE]"),
    "jetsons": (13, "[E96000]~[V00WZA]~[F00ZM0]~[F00G00]~[F00J00]~[F00NAA]~~~~~~~~~~[F00SAU]~~~~[F00H00]"),
    "amh": (10, "[E96000]~[V00D01]~[F00ZB1]~[F00AD2]~[F00B0O]~[F00CBG]~~~~~~~~~~[F00D1A]~~~~[F00EVG]"),
}

# play: frames, mark frame, UART1 send time, coin/start/launch taps, first shot frame, the shots (switch keys; none to a
# lock or scoop that holds the ball) one every 90 frames, and how many shots the second pass leaves off at the end
PLAY = {
    "rzspook": (4800, 1500, 10, [(600, 6, "5"), (660, 6, "1"), (960, 40, "SPACE")], 1200,
                ["B", "LCONTROL R", "RCONTROL R", "LCONTROL MINUS", "RCONTROL MINUS", "C", "LCONTROL L", "RCONTROL L", "H",
                 "J", "G", "E", "X", "A", "T", "LCONTROL B", "RCONTROL B", "LCONTROL N", "LCONTROL EQUALS", "RCONTROL EQUALS"],
                1),
    "jetsons": (5100, 1800, 13, [(900, 6, "5"), (960, 6, "1"), (1200, 6, "9")], 1500,
                ["B", "LCONTROL R", "RCONTROL R", "LCONTROL MINUS", "RCONTROL MINUS", "E", "C", "X", "G", "H", "J", "T", "Y",
                 "U", "LCONTROL B", "RCONTROL B", "LCONTROL L", "RCONTROL L", "L"], 0),
    "amh": (4800, 1500, 7, [(480, 6, "5"), (540, 6, "1"), (660, 40, "SPACE")], 900,
            ["B", "LCONTROL R", "RCONTROL R", "LCONTROL MINUS", "RCONTROL MINUS", "H", "V", "J", "W", "K", "M", "Z", "X",
             "C", "T", "Y", "U", "LCONTROL B", "RCONTROL B", "LCONTROL N", "RCONTROL N", "F", "S", "D"], 0),
}


def keys_line(frame, hold, key):
    return "%d tap %d %s" % (frame, hold, " ".join("KEYCODE_" + k for k in key.split()))


def workload(game, name):
    """frames, mark frame, UART1 send time and string, key script lines; None if the game has no such workload"""
    if name == "attract":
        return 2400, 600, None, None, []
    if name == "video":
        at, send = VIDEO[game]
        return 2400, 960, at, send, []
    if name == "play" and game in PLAY:
        frames, mark, at, start, first, shots, extra = PLAY[game]
        seq = shots + shots[:len(shots) - extra]
        keys = [keys_line(f, h, k) for f, h, k in start] + [keys_line(first + 90 * i, 1, k) for i, k in enumerate(seq)]
        return frames, mark, at, "[E97000]", keys
    return None


def exe_args(exe):
    if "sdl" in os.path.basename(exe).lower():
        return ["-headless"]
    return ["-window", "-skip_disclaimer", "-skip_gameinfo"]


def cpu_seconds(proc):
    """CPU time of a finished child: user + kernel"""
    if os.name == "nt":
        import ctypes
        from ctypes import wintypes
        t = [wintypes.FILETIME() for _ in range(4)]
        if ctypes.windll.kernel32.GetProcessTimes(wintypes.HANDLE(int(proc._handle)), *[ctypes.byref(x) for x in t]):
            return sum((x.dwHighDateTime << 32 | x.dwLowDateTime) for x in t[2:]) / 1e7
        return float("nan")
    return proc.cpu


def launch(exe, set_name, d, rompath, frames, env, keys=None):
    cmd = [exe, set_name, "-rompath", rompath, "-nvram_directory", "nvram", "-cfg_directory", "cfg", "-nothrottle",
           "-nosound", "-frames_to_run", str(frames)] + exe_args(exe)
    if keys is not None:
        with open(os.path.join(d, "keys.txt"), "w") as f:
            f.write("\n".join(keys) + "\n")
        cmd += ["-key_script", "keys.txt"]
    e = dict(os.environ, PINHECK_RTC=RTC, PINHECK_INSERVICE="6", PINHECK_TIME_LOG=os.path.join(d, "time.log"),
             PINHECK_UART1_LOG=os.path.join(d, "uart.log"), PINHECK_FRAME_LOG=os.path.join(d, "frames.bin"), **env)
    with open(os.path.join(d, "run.out"), "w") as out:
        if os.name == "nt":
            p = subprocess.Popen(cmd, cwd=d, env=e, stdout=out, stderr=subprocess.STDOUT)
            p.wait()
        else:
            import resource
            before = resource.getrusage(resource.RUSAGE_CHILDREN)
            p = subprocess.Popen(cmd, cwd=d, env=e, stdout=out, stderr=subprocess.STDOUT)
            p.wait()
            after = resource.getrusage(resource.RUSAGE_CHILDREN)
            p.cpu = after.ru_utime - before.ru_utime + after.ru_stime - before.ru_stime
    return p


def base_nvram(work, name, exe, game, set_name, rompath):
    d = os.path.join(work, "base", name, game)
    if os.path.isfile(os.path.join(d, "nvram", set_name + ".nv")):
        return os.path.join(d, "nvram")
    shutil.rmtree(d, ignore_errors=True)
    os.makedirs(os.path.join(d, "nvram"))
    os.makedirs(os.path.join(d, "cfg"))
    p = launch(exe, set_name, d, rompath, 1200, {})
    if p.returncode != 0 or not os.path.isfile(os.path.join(d, "nvram", set_name + ".nv")):
        sys.exit("first boot of %s with %s failed (exit %s), see %s" % (set_name, exe, p.returncode, d))
    return os.path.join(d, "nvram")


def run(work, tag, name, exe, game, set_name, rompath, wl, env):
    frames, mark, at, send, keys = wl
    d = os.path.join(work, "runs", tag)
    shutil.rmtree(d, ignore_errors=True)
    os.makedirs(os.path.join(d, "cfg"))
    shutil.copytree(base_nvram(work, name, exe, game, set_name, rompath), os.path.join(d, "nvram"))
    e = dict(env)
    if send:
        e.update(PINHECK_UART1_SEND=send, PINHECK_UART1_SEND_AT=str(at), PINHECK_UART1_SEND_GAP="1")
    t0 = time.monotonic()
    p = launch(exe, set_name, d, rompath, frames, e, keys if keys else None)
    wall = time.monotonic() - t0
    if p.returncode != 0:
        sys.exit("%s: %s exited %s, see %s" % (tag, exe, p.returncode, d))
    pts = []
    with open(os.path.join(d, "time.log")) as f:
        for line in f:
            v = line.split()
            if len(v) >= 2 and float(v[0]) >= mark / 60.0:
                pts.append((float(v[0]), float(v[1])))
    if len(pts) < 2:
        sys.exit("%s: no timed window in %s" % (tag, os.path.join(d, "time.log")))
    emulated = (frames - 1) / 60.0
    return (pts[-1][0] - pts[0][0]) / (pts[-1][1] - pts[0][1]), cpu_seconds(p) / emulated, wall, d


def builds(specs):
    out = []
    for s in specs:
        name, sep, exe = s.partition("=")
        if not sep:
            name, exe = os.path.basename(os.path.dirname(os.path.abspath(s))) or s, s
        if not os.path.isfile(exe):
            sys.exit("no executable %s" % exe)
        out.append((name, os.path.abspath(exe)))
    return out


def set_names(arg):
    sets = dict(SETS)
    for item in filter(None, (arg or "").split(",")):
        game, _, set_name = item.partition("=")
        if game not in sets:
            sys.exit("unknown game %s in --sets" % game)
        sets[game] = set_name
    return sets


def cmd_bench(a):
    bs, sets = builds(a.builds), set_names(a.sets)
    modes = [("default", {}), ("single", {"PINHECK_THREADS": "0"})]
    modes = [m for m in modes if m[0] in a.modes.split(",")]
    res = {}
    for game in a.games.split(","):
        for w in a.workloads.split(","):
            wl = workload(game, w)
            if not wl:
                continue
            for mode, env in modes:
                for i in range(a.runs):
                    for name, exe in bs:
                        tag = "%s-%s-%s-%s-%d" % (game, w, mode, name, i + 1)
                        sp, cpu, wall, _ = run(a.workdir, tag, name, exe, game, sets[game], a.rompath, wl, env)
                        res.setdefault((game, w, mode, name), []).append((sp, cpu))
                        print("%-40s %.3fx  %.2f CPU-s per emulated s  %.0f s" % (tag, sp, cpu, wall), flush=True)
    names = [n for n, _ in bs]
    for mode, _ in modes:
        print("\n%s (%s)\n" % (mode, "worker thread on" if mode == "default" else "PINHECK_THREADS=0"))
        print("| workload | " + " | ".join(names) + "".join(" | %s vs %s" % (n, names[0]) for n in names[1:]) + " |")
        print("|---" * (2 * len(names)) + "|")
        ratios = {n: [] for n in names[1:]}
        for game in a.games.split(","):
            for w in a.workloads.split(","):
                r = {n: res.get((game, w, mode, n)) for n in names}
                if not r[names[0]]:
                    continue
                sp = {n: sum(x[0] for x in v) / len(v) for n, v in r.items()}
                cpu = {n: sum(x[1] for x in v) / len(v) for n, v in r.items()}
                cells = ["%.3fx (%.2f)" % (sp[n], cpu[n]) for n in names]
                for n in names[1:]:
                    ratios[n].append(sp[n] / sp[names[0]])
                print("| %s %s | %s%s |" % (game, w, " | ".join(cells),
                                           "".join(" | %+.1f%%" % (100 * (sp[n] / sp[names[0]] - 1)) for n in names[1:])))
        if len(names) > 1:
            geo = ["%+.1f%%" % (100 * (math.prod(v) ** (1.0 / len(v)) - 1)) for v in ratios.values()]
            print("| geometric mean |" + " |" * len(names) + " " + " | ".join(geo) + " |")
    print("\nspeed: emulated / host time from the mark frame; (n): CPU seconds per emulated second")


def digest(path):
    if not os.path.isfile(path):
        return "missing"
    with open(path, "rb") as f:
        return hashlib.sha1(f.read()).hexdigest()[:12]


def cmd_check(a):
    bs, sets = builds(a.builds), set_names(a.sets)
    s = sets["dominos"]
    dirs = [run(a.workdir, "check-" + n, n, exe, "dominos", s, a.rompath, workload("dominos", "attract"), {})[3]
            for n, exe in bs]
    same = True
    for f in ["uart.log", "frames.bin", os.path.join("nvram", s + ".nv")]:
        h = [digest(os.path.join(d, f)) for d in dirs]
        ok = len(set(h)) == 1 and h[0] != "missing"
        same &= ok
        print("%-20s %s  %s" % (f, "identical" if ok else "DIFFERENT", "  ".join(h)))
    sys.exit(0 if same else 1)


def cmd_train(a):
    (name, exe), = builds([a.exe])
    sets = set_names(a.sets)
    for game in GAMES:
        for w in ("attract", "video"):
            sp, _, wall, _ = run(a.workdir, "train-%s-%s" % (game, w), name, exe, game, sets[game], a.rompath,
                                 workload(game, w), {})
            print("train %s %s: %.3fx, %.0f s" % (game, w, sp, wall), flush=True)
    if a.collect:
        os.makedirs(a.collect, exist_ok=True)
        pgc = [f for f in os.listdir(os.path.dirname(exe)) if f.lower().endswith(".pgc")]
        for f in pgc:
            shutil.move(os.path.join(os.path.dirname(exe), f), os.path.join(a.collect, f))
        print("%d .pgc files moved to %s" % (len(pgc), a.collect))


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    sub = ap.add_subparsers(dest="cmd", required=True)
    for c in ("bench", "check", "train"):
        p = sub.add_parser(c)
        p.add_argument("--rompath", required=True)
        p.add_argument("--sets", help="game=set,... e.g. dominos=dominos,rzspook=rzspook,jetsons=jetsons,amh=amh")
        p.add_argument("--workdir", default=os.path.join(tempfile.gettempdir(), "pinheck_bench"))
        if c == "train":
            p.add_argument("--collect", help="move MSVC .pgc files here after training")
            p.add_argument("exe")
        else:
            p.add_argument("builds", nargs="+", metavar="NAME=EXE")
        if c == "bench":
            p.add_argument("--runs", type=int, default=2)
            p.add_argument("--modes", default="default,single")
            p.add_argument("--workloads", default="attract,video,play")
            p.add_argument("--games", default=",".join(GAMES))
    a = ap.parse_args()
    a.rompath = os.path.abspath(a.rompath)
    a.workdir = os.path.abspath(a.workdir)
    {"bench": cmd_bench, "check": cmd_check, "train": cmd_train}[a.cmd](a)


if __name__ == "__main__":
    main()
