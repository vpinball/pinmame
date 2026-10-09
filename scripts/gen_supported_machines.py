#!/usr/bin/env python3
"""Generate SUPPORTED_MACHINES.md from the Open Pinball Database and the drivers.

  scripts/gen_supported_machines.py [--refresh] [--check] [--self-test]

Two inputs, joined on the machine title.

OPDB (opdb-v2.json) supplies the machines.  Its `type` field is on the
`machine` entries, not the `machineGroup` they hang under, so a group counts
as solid state if any member does.  One group is one row.

src/wpc/**/*.c supplies the drivers.  Every CORE_GAMEDEF / CORE_CLONEDEF
variant is a ROM set and the macro's last argument is its status: 0 is clean,
GAME_IMPERFECT_SOUND and friends are partial, GAME_NOT_WORKING is broken.  A
machine takes the status of its worst ROM set.

Parsing notes:

  * several driver files hold non-UTF-8 bytes in game titles, which makes grep
    skip them as binary and undercount by ~650 ROM sets; read with
    errors="replace";
  * a clone's parent is not always another machine.  Gottlieb System 80 games
    are clones of a BIOS set (gts80a), so matching considers every ROM set;
  * sam.c and sam_original.c define the same games behind #ifdef SAM_ORIGINAL.
    Only sam.c is built.

Name matching runs in tiers -- exact, alias, variant, containment, fuzzy,
family, unique name -- guarded by a year window and, from the third tier, the
manufacturer.  ROM sets that match nothing are listed on the page.
"""
import argparse, datetime, difflib, json, os, re, sys, textwrap, unicodedata
import urllib.request
from collections import defaultdict

OPDB_URL = "https://mp-data.sfo3.cdn.digitaloceanspaces.com/opdb-v2.json"
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
WPC = os.path.join(ROOT, "src", "wpc")
OUTPUT = os.path.join(ROOT, "SUPPORTED_MACHINES.md")
CACHE = os.path.join(
    os.environ.get("XDG_CACHE_HOME") or os.path.expanduser("~/.cache"),
    "pinmame", "opdb-v2.json")

SKIP_FILES = {"sam_original.c"}          # #ifdef SAM_ORIGINAL twin of sam.c
# sims/template holds simulator skeletons.  Their WPC_GAMEDEF() is undefined
# and not built.
SKIP_DIRS = {"template"}

# Displays a CPU drives.  Picks which member of an OPDB group describes the
# row; it does not decide whether a machine belongs on the page.
SS_DISPLAYS = ("alphanumeric", "dmd", "lcd", "cga")

# Reels before this year are electromechanical.  From 1980 on they are a retro
# choice on electronic hardware (Whoa Nellie, King of Diamonds).
REELS_SS_FROM = 1980

# OPDB display fields contradicted by the drivers.  Keyed by OPDB name.
# opdb_titles() fails on an entry that stops matching or stops changing anything.
DISPLAY_FIXES = {
    "Krull": "alphanumeric",                        # Gottlieb System 80A segments
    "Rob Zombie's Spookshow International": "dmd",  # PinHeck, 128 x 32 dots
    "The Jetsons": "dmd",                           # PinHeck, 128 x 64 dots
        # Mondialmatic's other machines of the same year and hardware -- Hell,
        # Hell's Queen, The Best Jump -- are alphanumeric in OPDB.
    "Jolly Joker": "alphanumeric",
    "Sisters": "alphanumeric",
    "Big Dryvers": "alphanumeric",
}

# Electromechanical machines that OPDB also carries as a solid state entry,
# duplicating the "em" entry at the same name and year.  Williams started
# solid state with Hot Tip in 1977, Recel two years after Criterium 75.
# opdb_titles() fails on an entry that stops matching.
NOT_SOLID_STATE = {
    "Aztec",           # Williams, 1976; OPDB's "Aztec (SS)" is not a machine
    "Criterium 75",    # Recel, 1975
}
EOL_YEARS = 10                           # "end of life" cut-off, in years
YEAR_SLACK = 3                           # how far PinMAME and OPDB may disagree

# Game definition macro -> argument count, from src/wpc/core.h and
# src/wpc/pinheck.h.  parse_drivers() raises on a mismatch; self_test() fails
# on a wrapper macro missing from this table.
ARITY = {"CORE_GAMEDEF": 7, "CORE_GAMEDEFNV": 6, "CORE_GAMEDEFNVR90": 6,
         "CORE_CLONEDEF": 8, "CORE_CLONEDEFNV": 7,
                  # Clone of the `pinheck` BIOS, so no parent among these sets.
         "PINHECK_GAMEDEF": 7}

# Driver flags, from src/driver.h and src/wpc/vpintf.h.  resolve_flags() raises
# on a flag not listed here.
FLAGS_OK = {"0", "GAME_USES_CHIMES", "GAME_NOCRC", "GAME_NO_COCKTAIL"}
FLAGS_PARTIAL = {
    "GAME_IMPERFECT_SOUND": "imperfect sound",
    "GAME_NO_SOUND": "no sound",
    "GAME_IMPERFECT_GRAPHICS": "imperfect graphics",
    "GAME_IMPERFECT_COLORS": "imperfect colours",
    "GAME_WRONG_COLORS": "wrong colours",
    "GAME_UNEMULATED_PROTECTION": "unemulated protection",
}
FLAG_BROKEN = "GAME_NOT_WORKING"

# Not pinball: bingos, shuffle alleys, bowlers, gun games, redemption pieces.
# Kept out of the matching -- Williams' "Topaz (Shuffle)" is not Inder's
# "Topaz".  The Video/Pinball Combos (Baby Pac-Man, Caveman) are pinball.
NON_PINBALL = re.compile(r"\((?:Bingo|Shuffle|Bowler|Gun ?game|Redemption)\)", re.I)

# OPDB name -> the PinMAME title it means, for names no tier bridges.  Both
# sides are normalised before comparison.
ALIASES = {
    "Disney TRON: Legacy": "TRON: Legacy",
    "James Cameron's Avatar": "Avatar",
    "Frank Thomas' Big Hurt": "Big Hurt",
    "Dr. Dude And His Excellent Ray": "Dr. Dude",
    "The Bally Game Show": "Game Show, The",
    "Maverick": "Maverick, The Movie",
    "Cirsa Sport 2000": "Sport 2000",
    "Pinball Lizard": "(Pinball) Lizard",
}

DISPLAYS = {"dmd": "DMD", "alphanumeric": "Alphanumeric", "lights": "Lights",
            "lcd": "LCD", "reels": "Reels", "cga": "CGA"}

ARTICLES = ("the", "a", "an", "le", "la", "les", "el", "los", "il")
# Dropped when comparing manufacturers.
MANUF_NOISE = {"and", "co", "coin", "inc", "sa", "s", "ltd", "the", "games",
               "game", "electronics", "enterprises", "pinball", "amusements",
               "amusement", "international", "industries", "corp", "company",
               "manufacturing", "matic", "play", "gmbh", "srl", "bv", "ag"}


# ---------------------------------------------------------------- OPDB input

def fetch_opdb(cache, refresh, url):
    """Download only when the cache is missing or --refresh is given."""
    if not refresh and os.path.exists(cache):
        with open(cache, encoding="utf-8") as f:
            return json.load(f), datetime.date.fromtimestamp(os.path.getmtime(cache))
    os.makedirs(os.path.dirname(cache) or ".", exist_ok=True)
    sys.stderr.write("downloading %s\n" % url)
    with urllib.request.urlopen(url, timeout=120) as r:
        raw = r.read()
    tmp = cache + ".tmp"
    with open(tmp, "wb") as f:
        f.write(raw)
    os.replace(tmp, cache)
    return json.loads(raw.decode("utf-8")), datetime.date.today()


def opdb_titles(doc):
    """-> [title dicts], one per machineGroup that has a solid state member."""
    groups, meta = defaultdict(list), {}
    for e in doc["entries"]:
        if e.get("entryType") == "machineGroup":
            meta[e["opdbId"]] = e
        elif e.get("entryType") == "machine":
            groups[e.get("opdbGroup") or e["opdbId"]].append(e)

    titles, fixed, excluded = [], set(), set()
    for gid, members in groups.items():
        ss = [m for m in members if m.get("type") == "ss"]
        if not ss:
            continue
        group = meta.get(gid)
        # Describe the row from members with a CPU-driven display.  OPDB groups a
        # machine with its re-themes and reissues across both eras: "El Dorado" holds
        # Gottlieb's 1975 electromechanical original and its 1984 solid state
        # "El Dorado City of Gold".  Reels-only groups still count; see SS_DISPLAYS.
        pick = [m for m in ss if m.get("display") in SS_DISPLAYS] or ss
        # A member dated well before its own group is a data error: Mali's "Big Rig"
        # is 1971 among five siblings from 1977.  Discounted only while another
        # candidate remains.
        if group and group.get("year"):
            sane = [m for m in pick
                    if not m.get("year") or m["year"] >= group["year"] - YEAR_SLACK]
            pick = sane or pick
        # Earliest such member; remakes inherit it.
        dated = [m for m in pick if m.get("year")]
        first = min(dated, key=lambda m: m["year"]) if dated else pick[0]
        names = {m["name"] for m in ss if m.get("name")}
        if group and group.get("name"):
            names.add(group["name"])
        for m in ss:
            for extra in (m.get("commonName"), m.get("shortName")):
                if extra:
                    names.add(extra)
        display = first.get("display") or ""
        for n in names:
            if DISPLAY_FIXES.get(n, display) != display:
                display = DISPLAY_FIXES[n]
                fixed.add(n)
        known_em = names & NOT_SOLID_STATE
        if known_em:
            excluded |= known_em
            continue
        if display == "reels" and (first.get("year") or 0) < REELS_SS_FROM:
            continue
        # The group name is usually the better label ("Flash" over its "Storm"
        # re-theme), except when the group is named for a machine of another era.
        name = (group or first).get("name") or first["name"]
        if group and group.get("year") and first.get("year") and \
                abs(group["year"] - first["year"]) > YEAR_SLACK:
            name = first.get("name") or name
        titles.append({
            "id": gid,
            "name": name,
            "year": first.get("year"),
            "years": {m["year"] for m in pick if m.get("year")},
            "manufacturer": (first.get("manufacturer") or {}).get("name") or "",
            "display": display,
            "ipdb": first.get("ipdbId"),
            "names": names,
            "sets": [],
            "tier": None,
        })
    stale = set(DISPLAY_FIXES) - fixed
    if stale:
        raise SystemExit("DISPLAY_FIXES entries that no longer match an OPDB "
                         "machine or no longer change anything: %s" % sorted(stale))
    stale = NOT_SOLID_STATE - excluded
    if stale:
        raise SystemExit("NOT_SOLID_STATE entries that no longer match an OPDB "
                         "machine: %s" % sorted(stale))
    titles.sort(key=lambda t: (t["year"] or 9999, t["name"].lower()))
    return titles


# ------------------------------------------------------------ driver parsing

def strip_comments(src):
    """Blank out C comments, keeping newlines and string literals.

    Eight CORE_CLONEDEFs are commented out.
    """
    out, i, n = [], 0, len(src)
    while i < n:
        c = src[i]
        if c in '"\'':
            out.append(c)
            i += 1
            while i < n:
                if src[i] == "\\":
                    out.append(src[i:i + 2])
                    i += 2
                    continue
                out.append(src[i])
                i += 1
                if src[i - 1] == c:
                    break
            continue
        if c == "/" and i + 1 < n:
            if src[i + 1] == "/":
                j = src.find("\n", i)
                i = n if j < 0 else j
                continue
            if src[i + 1] == "*":
                j = src.find("*/", i + 2)
                j = n if j < 0 else j + 2
                out.append("\n" * src.count("\n", i, j))
                i = j
                continue
        out.append(c)
        i += 1
    return "".join(out)


DEFINE = re.compile(r"^[ \t]*#[ \t]*define[ \t]+([A-Za-z_]\w*)[ \t]+(\S.*?)[ \t]*$")
COND = re.compile(r"^[ \t]*#[ \t]*(ifdef|ifndef|if|else|elif|endif)\b[ \t]*(.*)$")


def local_defines(src):
    """-> {macro: replacement} for the release build (MAME_DEBUG undefined).

    zacgames.c's SOUNDFLAG and playgames.c's GAME_STATUS are 0 under
    MAME_DEBUG and GAME_IMPERFECT_SOUND otherwise.
    """
    defs, stack = {}, []
    for line in src.splitlines():
        m = COND.match(line)
        if m:
            kw, rest = m.group(1), m.group(2)
            if kw in ("ifdef", "ifndef", "if"):
                debug = "MAME_DEBUG" in rest
                stack.append([debug, not (debug and kw == "ifdef")])
            elif kw in ("else", "elif") and stack:
                if stack[-1][0]:
                    stack[-1][1] = not stack[-1][1]
            elif kw == "endif" and stack:
                stack.pop()
            continue
        if all(live for _, live in stack):
            m = DEFINE.match(line)
            if m:
                defs[m.group(1)] = m.group(2)
    return defs


def split_args(s):
    """Top-level comma split that respects strings and nested parentheses."""
    parts, depth, quote, cur, i = [], 0, False, "", 0
    while i < len(s):
        c = s[i]
        if quote:
            if c == "\\":
                cur += s[i:i + 2]
                i += 2
                continue
            if c == '"':
                quote = False
            cur += c
        elif c == '"':
            quote = True
            cur += c
        elif c == "(":
            depth += 1
            cur += c
        elif c == ")":
            depth -= 1
            cur += c
        elif c == "," and depth == 0:
            parts.append(cur.strip())
            cur = ""
        else:
            cur += c
        i += 1
    parts.append(cur.strip())
    return parts


def close_paren(src, i):
    """Index just past the ')' closing the call whose '(' was at i-1."""
    depth, quote = 1, False
    while depth:
        c = src[i]
        if quote:
            if c == "\\":
                i += 2
                continue
            if c == '"':
                quote = False
        elif c == '"':
            quote = True
        elif c == "(":
            depth += 1
        elif c == ")":
            depth -= 1
        i += 1
    return i


MACRO = re.compile(r"\b(%s)\s*\("
                   % "|".join(sorted(ARITY, key=len, reverse=True)))
# Anything shaped like a game definition macro; policed by self_test().
ANY_MACRO = re.compile(r"\b(\w*(?:GAMEDEF|CLONEDEF)\w*)\s*\(")


def resolve_flags(expr, defs, where):
    """-> (status, [note]) for a macro's flag argument."""
    notes, status, seen = [], "working", set()
    todo = [t.strip() for t in expr.split("|")]
    while todo:
        tok = todo.pop()
        if not tok or tok in seen:
            continue
        seen.add(tok)
        if tok in defs:
            todo += [t.strip() for t in defs[tok].split("|")]
            continue
        if tok in FLAGS_OK or re.fullmatch(r"0[xX]?[0-9a-fA-F]*", tok):
            continue
        if tok == FLAG_BROKEN:
            status = "broken"
        elif tok in FLAGS_PARTIAL:
            notes.append(FLAGS_PARTIAL[tok])
            if status == "working":
                status = "partial"
        else:
            raise SystemExit("%s: unknown driver flag %r -- classify it in "
                             "FLAGS_OK / FLAGS_PARTIAL" % (where, tok))
    return ("broken" if status == "broken" else status), sorted(set(notes))


def parse_drivers(wpc=WPC):
    """-> [ROM set dicts] for every CORE_*DEF* in src/wpc/**/*.c."""
    sets = []
    for dirpath, dirs, files in os.walk(wpc):
        dirs[:] = [d for d in dirs if d not in SKIP_DIRS]
        for fn in sorted(files):
            if not fn.endswith(".c") or fn in SKIP_FILES:
                continue
            path = os.path.join(dirpath, fn)
            rel = os.path.relpath(path, ROOT).replace(os.sep, "/")
            with open(path, encoding="utf-8", errors="replace") as f:
                src = strip_comments(f.read())
            defs = local_defines(src)
            # sims/<system>/full/ is a complete table simulator, prelim/ a partial
            # one.  sims/sleic and sims/pinheck sit outside that split, so the
            # simulator is reported without a tier.
            sim = ("" if "/sims/" not in rel else
                   "full" if "/full/" in rel else
                   "preliminary" if "/prelim/" in rel else "yes")
            for m in MACRO.finditer(src):
                end = close_paren(src, m.end())
                kind = m.group(1)
                args = split_args(src[m.end():end - 1])
                if len(args) != ARITY[kind]:
                    raise SystemExit(
                        "%s:%d: %s takes %d arguments, found %d (%r)"
                        % (rel, src.count("\n", 0, m.start()) + 1, kind,
                           ARITY[kind], len(args), args))
                if kind in ("CORE_GAMEDEF", "PINHECK_GAMEDEF"):
                    rom, parent, rest = "%s_%s" % (args[0], args[1]), None, args[2:]
                elif kind == "CORE_CLONEDEF":
                    rom = "%s_%s" % (args[0], args[1])
                    parent, rest = "%s_%s" % (args[0], args[2]), args[3:]
                elif kind == "CORE_CLONEDEFNV":
                    rom, parent, rest = args[0], args[1], args[2:]
                else:
                    rom, parent, rest = args[0], None, args[1:]
                title, year, manuf, _machine, flag = rest
                where = "%s:%d" % (rel, src.count("\n", 0, m.start()) + 1)
                status, notes = resolve_flags(flag, defs, where)
                sets.append({
                    "rom": rom,
                    "parent": parent,
                    "title": title.strip().strip('"'),
                    "year": int(year) if year.isdigit() else None,
                    "manufacturer": manuf.strip().strip('"'),
                    "status": status,
                    "notes": notes,
                    "file": rel,
                    "sim": sim,
                    "pinball": not NON_PINBALL.search(title),
                })
    if not sets:
        raise SystemExit("no game definitions found under %s" % wpc)
    return sets


def root_years(sets):
    """-> {rom: {candidate years}}, adding the parent's year to each clone.

    Free-play and MOD re-releases carry their own year: Freedom's "Free Play+"
    set is dated 2019 for a 1976 machine.
    """
    by_rom = {s["rom"]: s for s in sets}
    out = {}
    for s in sets:
        years, cur, hops = set(), s, 0
        while cur and hops < 8:
            if cur["year"]:
                years.add(cur["year"])
            cur, hops = by_rom.get(cur["parent"] or ""), hops + 1
        out[s["rom"]] = years
    return out


# ----------------------------------------------------------------- matching

def fold(name):
    """Lower-case, de-accent, drop bracketed revisions, move trailing articles."""
    n = unicodedata.normalize("NFKD", name)
    n = "".join(c for c in n if not unicodedata.combining(c))
    n = n.lower().replace("&", " and ").replace("+", " plus ")
    n = re.sub(r"\s*\([^)]*\)", " ", n)
    n = re.sub(r"\s*\[[^\]]*\]", " ", n)
    n = re.sub(r"\s+", " ", n).strip(" ,.-")
    # The drivers move a trailing owner to the back as they do articles:
    # "Frankenstein, Mary Shelley's", "Tommy Pinball Wizard, The Who's".
    m = re.match(r"^(.+),\s*((?:the\s+)?[\w.' \u2019-]*[\w.]['\u2019]s?)$", n)
    if m and m.group(2).rstrip("s").endswith(("'", "\u2019")):
        n = "%s %s" % (m.group(2), m.group(1))
    m = re.match(r"^(.*),\s*(%s)$" % "|".join(ARTICLES), n)
    if m:
        n = "%s %s" % (m.group(2), m.group(1))
    return re.sub(r"^(?:%s)\s+" % "|".join(ARTICLES), "", n).strip()


def squash(name):
    return re.sub(r"[^a-z0-9]", "", fold(name))


TRAILING = re.compile(r"\s+(pinball machine|pinball|shuffle|bowler|redemption)$")
POSSESSIVE = re.compile(r"^(?:\w+[.']?\s+){0,2}\w+['\u2019]s?\s+(.+)$")


def variants(name):
    """-> extra keys for a title: subtitle halves, dropped owner, trimmed tail."""
    base, out = fold(name), set()

    def add(x):
        x = squash(x)
        # Short keys collide ("quest", "tommy").
        if len(x) >= 8:
            out.add(x)

    add(TRAILING.sub("", base))
    for sep in (":", " - ", " \u2013 ", " / "):
        if sep in base:
            head, _, tail = base.partition(sep)
            add(head)
            add(tail)
    m = POSSESSIVE.match(base)
    if m and (len(m.group(1).split()) >= 2 or len(m.group(1)) >= 8):
        add(m.group(1))
    out.discard(squash(base))
    return out


def manuf_tokens(name):
    n = unicodedata.normalize("NFKD", name.lower())
    n = "".join(c for c in n if not unicodedata.combining(c))
    n = re.sub(r"\([^)]*\)", " ", n)
    return {w for w in re.split(r"[^a-z0-9]+", n) if w and w not in MANUF_NOISE}


def manuf_ok(a, b):
    """Compatible unless both sides name a company and they share no word."""
    ta, tb = manuf_tokens(a), manuf_tokens(b)
    return not ta or not tb or bool(ta & tb)


def year_gap(title, years):
    """Smallest distance between a ROM set's candidate years and the title's."""
    if not years or not title["years"]:
        return 0
    return min(abs(y - t) for y in years for t in title["years"])


def match(titles, sets):
    """Assign ROM sets to titles tier by tier; later tiers only see leftovers."""
    years = root_years(sets)
    by_rom = {s["rom"]: s for s in sets}

    def root_of(s):
        """The oldest ancestor inside the driver set: one machine's ROM family."""
        cur, hops = s, 0
        while cur["parent"] in by_rom and hops < 8:
            cur, hops = by_rom[cur["parent"]], hops + 1
        return cur["rom"]

    keys, varkeys = defaultdict(list), defaultdict(list)
    for s in sets:
        keys[squash(s["title"])].append(s)
        for v in variants(s["title"]):
            varkeys[v].append(s)

    alias = {squash(k): squash(v) for k, v in ALIASES.items()}
    for t in titles:
        t["_keys"] = {squash(n) for n in t["names"] if squash(n)}
        t["_vars"] = set().union(*(variants(n) for n in t["names"])) if t["names"] else set()
        for k in list(t["_keys"]):
            if k in alias:
                t["_keys"].add(alias[k])

    free = {s["rom"]: s for s in sets}

    def propose(tier, pick):
        """Run one tier: collect proposals, give each set to its best claimant."""
        bids = defaultdict(list)
        for t in titles:
            for s in pick(t):
                if s["rom"] not in free:
                    continue
                same_maker = manuf_ok(t["manufacturer"], s["manufacturer"])
                # With no year on either side only the name is left, and names repeat:
                # Rowamet's "Heavy Metal" is not Stern's, a WPC homebrew "Rush" is not
                # Stern's.  Require the manufacturer to agree.
                if not (years[s["rom"]] and t["years"]) and not same_maker:
                    continue
                gap = year_gap(t, years[s["rom"]])
                if gap > YEAR_SLACK:
                    continue
                if tier >= 3 and not same_maker:
                    continue
                pen = 0 if same_maker else 1
                bids[s["rom"]].append(((gap, pen, t["name"], t["id"]), t))
        for rom, cands in bids.items():
            best = min(cands, key=lambda c: c[0])[1]
            best["sets"].append(free.pop(rom))
            if best["tier"] is None:
                best["tier"] = tier

    propose(1, lambda t: [s for k in t["_keys"] for s in keys.get(k, ())])
    propose(2, lambda t: [s for k in t["_keys"] | t["_vars"]
                          for s in list(varkeys.get(k, ())) + list(keys.get(k, ()))])
    propose(3, lambda t: contained(t, free))
    all_keys = list(keys)
    propose(4, lambda t: fuzzy(t, keys, all_keys))

    # Revisions outside the year window that are the same machine: rfm_160 is
    # dated 2003 for a 1999 machine and the MODs run to 2024.  Restricted to a
    # family an earlier tier already matched -- Allied Leisure's games are all
    # clones of one `allied` set without being the same machine.
    owners = defaultdict(set)
    for t in titles:
        for s in t["sets"]:
            owners[root_of(s)].add(t["id"])
    by_id = {t["id"]: t for t in titles}
    for rom, s in list(free.items()):
        names = {squash(s["title"])} | variants(s["title"])
        for tid in owners.get(root_of(s), ()):
            t = by_id[tid]
            # Inside a family the title name is often a prefix of the set name:
            # "AC/DC Limited Edition" against "AC/DC".
            if names & (t["_keys"] | t["_vars"]) or any(
                    len(k) >= 4 and n.startswith(k)
                    for k in t["_keys"] for n in names):
                t["sets"].append(free.pop(rom))
                break

    # A name unique among all solid state machines, with a matching maker, even
    # when the years disagree: "Ali (7-digit conversion Free Play rev. 76)" is
    # dated 2023 for a 1980 machine and is a root set.  Uniqueness is the guard;
    # repeated names (three "Star Wars") still need the year.
    claims = defaultdict(set)
    for t in titles:
        for k in t["_keys"]:
            claims[k].add(t["id"])
    for rom, s in list(free.items()):
        holders = claims.get(squash(s["title"])) or ()
        if len(holders) != 1:
            continue
        t = by_id[next(iter(holders))]
        if manuf_ok(t["manufacturer"], s["manufacturer"]):
            t["sets"].append(free.pop(rom))

    # A re-theme keeps its own maker, so a mismatch alone is not wrong: Playboy's
    # family includes Arkon's "Sexy Girl".  But a family that already belongs to a
    # title matching the set's maker is the better home: Data East's 2016-dated
    # Star Wars MODs otherwise land on Stern's 2017 Star Wars.
    homes = defaultdict(list)
    for t in titles:
        for s in t["sets"]:
            homes[root_of(s)].append(t)
    for t in titles:
        for s in list(t["sets"]):
            if manuf_ok(t["manufacturer"], s["manufacturer"]):
                continue
            for other in homes[root_of(s)]:
                if other is not t and manuf_ok(other["manufacturer"], s["manufacturer"]):
                    t["sets"].remove(s)
                    other["sets"].append(s)
                    break

    for t in titles:
        t["sets"].sort(key=lambda s: (["working", "partial", "broken"].index(s["status"]),
                                      s["year"] or 9999, s["rom"]))
        for k in ("_keys", "_vars"):
            t.pop(k, None)
    return [s for s in sets if s["rom"] in free]


def contained(title, free):
    """One name sitting inside the other: 'Bride of Pinbot' in 'Machine: ...'."""
    out = []
    for k in title["_keys"]:
        if len(k) < 8:
            continue
        for s in free.values():
            sk = squash(s["title"])
            if len(sk) >= 8 and (k in sk or sk in k):
                out.append(s)
    return out


def fuzzy(title, keys, all_keys):
    out = []
    for k in title["_keys"]:
        if len(k) < 8:
            continue
        for near in difflib.get_close_matches(k, all_keys, n=2, cutoff=0.88):
            out += keys[near]
    return out


def verdict(title):
    """-> the bucket for a machine that has at least one ROM set.

    "Fully working" requires every ROM set to be clean, so no row in that
    table carries a caveat.  A machine whose own ROMs are clean can land in
    "partially working" on a regional variant or a prototype; note_for()
    names it.
    """
    statuses = {s["status"] for s in title["sets"]}
    if statuses == {"broken"}:
        return "broken"
    if statuses == {"working"}:
        return "working"
    return "partial"


def note_for(title):
    """-> the Notes cell: what is wrong, in which revisions, and what is not."""
    bad = [s for s in title["sets"] if s["notes"] or s["status"] == "broken"]
    if not bad:
        return ""
    labels = sorted({n for s in bad for n in s["notes"]})
    if any(s["status"] == "broken" for s in bad):
        labels.append("does not run")
    clean = len(title["sets"]) - len(bad)
    if not clean:
        return ", ".join(labels)
    named = ", ".join("`%s`" % s["rom"] for s in bad[:3])
    if len(bad) > 3:
        named += " and %d more" % (len(bad) - 3)
    return "%s in %s; %d other revision%s clean" % (
        ", ".join(labels), named, clean, "" if clean == 1 else "s")


# ------------------------------------------------------------------- output

BUCKETS = [
    ("working", "Fully working",
     "Every known ROM set runs, and the driver flags nothing wrong with any "
     "of them."),
    ("partial", "Partially working",
     "At least one ROM set is flagged as wrong, missing or not running — "
     "most often the sound hardware. The machine itself usually plays. Where "
     "only some revisions are affected the note names them and says how many "
     "others are clean, which is often the real story: a regional variant or "
     "a prototype is at fault while the production ROMs are fine."),
    ("broken", "Emulated but not working",
     "A driver exists and the ROMs are known, but every ROM set is marked "
     "`GAME_NOT_WORKING`: it does not boot or does not play."),
    ("absent", "Not emulated",
     "No driver. These machines are old enough to be in scope, so they are "
     "the candidates for future work."),
    ("recent", "Not emulated \u2014 released recently",
     "No driver, and not expected to get one soon: see the note on "
     "end-of-life machines above."),
]


# Markdown-active inside a table cell.  A pipe ends the cell; the rest can open
# emphasis, a code span, a link or raw HTML.  Machine names contain them
# (Q*Bert's Quest).
ACTIVE = re.compile(r"([\\`*_\[\]<>~|])")


def wrap(lines, width=90):
    """Hard-wrap prose; markdown rejoins it.

    Tables, headings and fenced code are left alone.
    """
    out, fenced = [], False
    for line in lines:
        if line.startswith("```"):
            fenced = not fenced
        if fenced or len(line) <= width or line[:1] in "|# ":
            out.append(line)
            continue
        out.extend(textwrap.wrap(line, width, break_long_words=False,
                                 break_on_hyphens=False))
    return out


def table(w, headers, aligns, rows):
    """Write a GFM table, padded so the pipes line up in the source.

    A parser trims the cells, so the padding does not affect rendering.
    """
    width = [len(h) for h in headers]
    for row in rows:
        for i, cell in enumerate(row):
            width[i] = max(width[i], len(cell))
    width = [max(n, 3) for n in width]

    def line(cells):
        return "| %s |" % " | ".join(
            c.rjust(width[i]) if aligns[i] == ">" else c.ljust(width[i])
            for i, c in enumerate(cells))

    w(line(headers))
    w("| %s |" % " | ".join(
        "-" * (width[i] - 1) + ":" if a == ">" else "-" * width[i]
        for i, a in enumerate(aligns)))
    for row in rows:
        w(line(row))


def esc(s):
    return ACTIVE.sub(r"\\\1", str(s)).replace("\n", " ")


def rom_cell(sets, limit=3):
    names = ["`%s`" % s["rom"] for s in sets]
    if len(names) <= limit:
        return ", ".join(names)
    return "%s + %d more" % (", ".join(names[:limit]), len(names) - limit)


def render(titles, orphans, other, stamp, cutoff, eol_years):
    counts = defaultdict(int)
    for t in titles:
        counts[t["bucket"]] += 1
    total = len(titles)
    today = datetime.date.today().isoformat()

    L = []
    w = L.append
    w("# Solid state machines in PinMAME")
    w("")
    w("*Generated by `scripts/gen_supported_machines.py`. Do not edit by hand.*")
    w("")
    w("Every solid state pinball machine known to the "
      "[Open Pinball Database](https://opdb.org/), matched against the drivers "
      "in `src/wpc`, so you can see at a glance what PinMAME emulates, what it "
      "emulates imperfectly, and what it does not emulate at all.")
    w("")
    w("## Scope: solid state, and end of life")
    w("")
    w("**PinMAME emulates solid state machines only.** A solid state machine "
      "runs its rules on a CPU reading a ROM, which is the thing PinMAME "
      "emulates. Electromechanical machines keep their rules in relays, "
      "steppers and motors, with no code to run, so they are out of scope and "
      "are not listed here. A handful of early solid state machines still use "
      "chimes instead of a sound board; those are in scope and are listed.")
    w("")
    w("A few machines score on mechanical reels rather than on a "
      "display. From %d on that is a deliberate retro choice on "
      "electronic hardware, so those are listed; earlier reels machines "
      "are electromechanical and are not. Where a machine has members of "
      "both kinds, as Gottlieb's El Dorado has its 1975 "
      "electromechanical original and its 1984 solid state *El Dorado "
      "City of Gold*, the row is the solid state one." % REELS_SS_FROM)
    w("")
    w("**PinMAME only emulates machines that are considered end of life.** "
      "Work starts on a machine once it is no longer commercially current. In "
      "practice this means a machine released less than %d years ago is "
      "unlikely to be emulated, so everything from **%d onwards** is listed "
      "separately below as not expected rather than as missing." % (eol_years, cutoff))
    w("")
    w("## Summary")
    w("")
    rows = [[label, str(counts[key]),
             "%.1f%%" % (100.0 * counts[key] / total if total else 0)]
            for key, label, _ in BUCKETS]
    rows.append(["**Total**", "**%d**" % total, ""])
    table(w, ["Status", "Machines", "Share"], ["<", ">", ">"], rows)
    w("")

    w("### By decade")
    w("")
    decades = defaultdict(lambda: defaultdict(int))
    for t in titles:
        d = (t["year"] // 10 * 10) if t["year"] else 0
        decades[d][t["bucket"]] += 1
        decades[d]["total"] += 1
    table(w, ["Decade", "Total", "Fully working", "Partial", "Not working",
              "Not emulated"], ["<", ">", ">", ">", ">", ">"],
          [["%ds" % d if d else "unknown", str(decades[d]["total"]),
            str(decades[d]["working"]), str(decades[d]["partial"]),
            str(decades[d]["broken"]),
            str(decades[d]["absent"] + decades[d]["recent"])]
           for d in sorted(decades)])
    w("")

    for key, label, blurb in BUCKETS:
        rows = [t for t in titles if t["bucket"] == key]
        w("## %s (%d)" % (label, len(rows)))
        w("")
        w(blurb)
        w("")
        if not rows:
            w("_None._")
            w("")
            continue
        if key in ("absent", "recent"):
            table(w, ["Machine", "Year", "Manufacturer", "Display"],
                  ["<", ">", "<", "<"],
                  [[esc(t["name"]), str(t["year"] or ""), esc(t["manufacturer"]),
                    DISPLAYS.get(t["display"], t["display"])] for t in rows])
        else:
            table(w, ["Machine", "Year", "Manufacturer", "Display",
                      "PinMAME ROM sets", "Simulator", "Notes"],
                  ["<", ">", "<", "<", "<", "<", "<"],
                  [[esc(t["name"]), str(t["year"] or ""), esc(t["manufacturer"]),
                    DISPLAYS.get(t["display"], t["display"]), rom_cell(t["sets"]),
                    next((s["sim"] for s in t["sets"] if s["sim"]), ""),
                    note_for(t)] for t in rows])
        w("")

    w("## Appendix: driver entries with no machine")
    w("")
    w("### ROM sets that matched nothing (%d)" % len(orphans))
    w("")
    w("Driver entries that matched no solid state OPDB title. Most are "
      "prototypes, test fixtures and regional machines OPDB does not carry. "
      "The rest are matching failures: add them to `ALIASES` in "
      "`scripts/gen_supported_machines.py` to fix a row above.")
    w("")
    table(w, ["ROM set", "Title", "Year", "Manufacturer", "Driver"],
          ["<", "<", ">", "<", "<"],
          [["`%s`" % s["rom"], esc(s["title"]), str(s["year"] or ""),
            esc(s["manufacturer"]), "`%s`" % s["file"]]
           for s in sorted(orphans, key=lambda s: (s["year"] or 9999,
                                                   s["title"].lower()))])
    w("")

    w("### Driver entries that are not pinball machines (%d)" % len(other))
    w("")
    w("Bingos, shuffle alleys, bowlers, gun games and redemption pieces. "
      "PinMAME drives them, but they are not pinball machines, so they "
      "take no part in the matching.")
    w("")
    table(w, ["ROM set", "Title", "Year", "Manufacturer", "Driver"],
          ["<", "<", ">", "<", "<"],
          [["`%s`" % s["rom"], esc(s["title"]), str(s["year"] or ""),
            esc(s["manufacturer"]), "`%s`" % s["file"]]
           for s in sorted(other, key=lambda s: (s["year"] or 9999,
                                                 s["title"].lower()))])
    w("")

    w("## How this page is built")
    w("")
    w("```")
    w("scripts/gen_supported_machines.py")
    w("```")
    w("")
    w("The script reads a cached copy of OPDB and re-downloads only with "
      "`--refresh`; `--check` fails if this file is out of date. Machine data "
      "comes from the OPDB snapshot of **%s**; driver data from `src/wpc` at "
      "the commit this file was generated from, on **%s**." % (stamp, today))
    w("")
    w("A machine is one OPDB *group*, so a title and its remakes are one row, "
      "dated and credited to the original. Its status is that of the *worst* "
      "of its ROM sets, so \"fully working\" means every revision is clean and "
      "no row in that table carries a caveat; when only some revisions are "
      "affected, the note says which. The "
      "simulator column reports PinMAME's optional table simulation "
      "(`src/wpc/sims`), which lets a machine be played from the keyboard. It "
      "is independent of emulation accuracy, and its absence says nothing "
      "about how well the ROM runs under Visual Pinball. `full` and "
      "`preliminary` are the driver tree's own split; `yes` is a simulator "
      "that sits outside it and so is not classified either way.")
    w("")
    w("Machines are matched to drivers by name, with a year and manufacturer "
      "guard. This is not exact: a machine listed as not emulated may have a "
      "driver under a name the matcher missed. Check the ROM set list above "
      "before concluding a machine is absent. Also take a look at https://www.vpforums.org/index.php?showtopic=47 "
      "for a list of pinball (or related) machines and prototypes that are not emulated yet.")
    return "\n".join(wrap(L)) + "\n"


# ------------------------------------------------------------------ plumbing

def build(args):
    doc, stamp = fetch_opdb(args.cache, args.refresh, args.url)
    titles = opdb_titles(doc)
    sets = parse_drivers()
    other = [s for s in sets if not s["pinball"]]
    orphans = match(titles, [s for s in sets if s["pinball"]])
    cutoff = datetime.date.today().year - args.eol_years
    for t in titles:
        t["bucket"] = ("recent" if not t["sets"] and (t["year"] or 0) >= cutoff
                       else "absent" if not t["sets"] else verdict(t))
    page = render(titles, orphans, other, stamp, cutoff, args.eol_years)
    return titles, sets, orphans, page


def self_test():
    """Parse-level checks; arity and flag drift already raise."""
    assert squash("Addams Family, The (L-5)") == "addamsfamily"
    assert squash("Pin-Bot (L-5)") == "pinbot"
    assert squash("Dr. Dude") == "drdude"
    assert "brideofpinbot" in variants("Machine: Bride of Pinbot, The (L-2)")
    assert manuf_ok("Bally / Oliver", "Bally") and manuf_ok("Alvin G", "Alvin G. & Co")
    assert not manuf_ok("Williams", "Data East")
    assert strip_comments('a; // CORE_GAMEDEF(x)\nb;') == 'a; \nb;'
    assert strip_comments('x("//not a comment"); /* CORE_GAMEDEF(y) */') == \
        'x("//not a comment"); '

    sets = parse_drivers()
    by_rom = {s["rom"]: s for s in sets}
    assert len(sets) > 2900, len(sets)
    assert "sam_original.c" not in {s["file"].split("/")[-1] for s in sets}
    assert by_rom["mm_109"]["status"] == "working"
    assert any(s["status"] == "broken" for s in sets)
    assert any(s["status"] == "partial" for s in sets)
    assert by_rom["mm_109"]["sim"] == "full"
    # Every set defined under sims/ has a simulator, including the ones outside
    # the full/prelim split (sims/sleic, sims/pinheck).
    assert all(s["sim"] for s in sets if "/sims/" in s["file"])
    assert by_rom["iomoon"]["sim"] == "yes" and by_rom["jetsons_004"]["sim"] == "yes"
    assert not by_rom["topaz_l1"]["pinball"] and by_rom["topazi"]["pinball"]

    # PinHeck is defined through a wrapper macro.
    assert {"amh_023", "dominos_006", "rzspook_026", "jetsons_004"} <= set(by_rom)
    assert by_rom["jetsons_004"]["title"] == "Jetsons, The"

    # No game definition macro may exist that ARITY does not know about.
    # WPC_GAMEDEF() is an undefined placeholder in sims/template, kept out by
    # SKIP_DIRS.
    seen = set()
    for dirpath, dirs, files in os.walk(WPC):
        for fn in files:
            if fn.endswith((".c", ".h")):
                with open(os.path.join(dirpath, fn), encoding="utf-8",
                          errors="replace") as f:
                    seen |= set(ANY_MACRO.findall(f.read()))
    unknown = seen - set(ARITY) - {"WPC_GAMEDEF"}
    assert not unknown, "game definition macro(s) the parser ignores: %s" % sorted(unknown)
    # the commented-out CORE_CLONEDEFs must not be picked up
    assert "tom_20" not in by_rom and "dm_h7" not in by_rom

    # Each alias must still name a real driver title and still be doing work.
    titles = {squash(s["title"]) for s in sets}
    for s in sets:
        titles |= variants(s["title"])
    for opdb_name, pinmame_name in ALIASES.items():
        assert squash(opdb_name) != squash(pinmame_name), \
            "alias %r is a no-op" % opdb_name
        assert squash(pinmame_name) in titles, \
            "alias %r points at %r, which no driver defines" % (opdb_name, pinmame_name)
    print("parse ok: %d ROM sets, %d drivers" % (sets.__len__(),
                                                 len({s["file"] for s in sets})))
    return 0


def main(argv=None):
    p = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    p.add_argument("-o", "--output", default=OUTPUT, help="markdown to write")
    p.add_argument("--cache", default=CACHE, help="where the OPDB copy lives")
    p.add_argument("--refresh", action="store_true",
                   help="re-download OPDB instead of using the cache")
    p.add_argument("--url", default=OPDB_URL)
    p.add_argument("--eol-years", type=int, default=EOL_YEARS,
                   help="machines newer than this are listed as not expected")
    p.add_argument("--check", action="store_true",
                   help="exit 1 if the output file is out of date")
    p.add_argument("--self-test", action="store_true")
    p.add_argument("--stdout", action="store_true")
    args = p.parse_args(argv)

    if args.self_test:
        return self_test()

    titles, sets, orphans, page = build(args)
    if args.stdout:
        sys.stdout.write(page)
        return 0
    if args.check:
        old = open(args.output, encoding="utf-8").read() if os.path.exists(args.output) else ""
        if old != page:
            sys.stderr.write("%s is out of date; run scripts/%s\n"
                             % (os.path.relpath(args.output, ROOT),
                                os.path.basename(__file__)))
            return 1
        print("%s is up to date" % os.path.relpath(args.output, ROOT))
        return 0
    with open(args.output, "w", encoding="utf-8") as f:
        f.write(page)
    buckets = defaultdict(int)
    for t in titles:
        buckets[t["bucket"]] += 1
    sys.stderr.write(
        "%s: %d machines (%d working, %d partial, %d broken, %d absent, "
        "%d recent) from %d ROM sets, %d unmatched\n"
        % (os.path.relpath(args.output, ROOT), len(titles), buckets["working"],
           buckets["partial"], buckets["broken"], buckets["absent"],
           buckets["recent"], len(sets), len(orphans)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
