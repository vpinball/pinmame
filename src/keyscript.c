// license:BSD-3-Clause
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "driver.h"
#include "input.h"
#include "inptport.h"
#include "keyscript.h"

#define KEYSCRIPT_MAXHELD 32

enum { KS_DOWN, KS_UP, KS_TAP, KS_MARK, KS_QUIT };

typedef struct {
  int       frame;
  int       action;
  int       hold;                        /* KS_TAP only */
  InputCode code[KEYSCRIPT_MAXHELD];     /* CODE_NONE terminated */
  char      text[64];                    /* KS_MARK only */
} keyscript_tEvent;

static struct {
  int              loaded;      /* 0 = not tried, 1 = active, -1 = off */
  int              frame;
  keyscript_tEvent *ev;
  int              nEv, nextEv;
  struct { InputCode code; int until; } held[KEYSCRIPT_MAXHELD]; /* until < 0 = forever */
  int              nHeld;
  char             marksName[512];
} ks;

/* "KEYCODE_A,KEYCODE_B" -> codes.  One seq_set_string()/seq_get_1() call per
   token, never batched: seq_set_string's own sequence limit is SEQ_MAX (16),
   so a single call for the whole line would silently cap out there. This way
   the per-line cap below is the only cap. Commas are turned into spaces and
   the words split by hand, since seq_set_string already uses strtok() on its
   own copy of its argument and nesting a second strtok() around it would
   corrupt both scans. */
static void keyscript_parseKeys(const char *s, InputCode *out) {
  char buf[400];                          /* matches the callers' rest/keys buffers */
  int i = 0, n = 0;
  strncpy(buf, s, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = 0;
  for (i = 0; buf[i]; i++)
    if (buf[i] == ',') buf[i] = ' ';

  for (i = 0; buf[i]; ) {
    char tok[64];
    int t = 0;
    InputSeq seq;
    InputCode code;
    while (buf[i] == ' ' || buf[i] == '\t') i++;
    if (!buf[i]) break;
    while (buf[i] && buf[i] != ' ' && buf[i] != '\t') {
      if (t < (int)sizeof(tok) - 1) tok[t++] = buf[i];
      i++;
    }
    tok[t] = 0;
    seq_set_string(&seq, tok);
    code = seq_get_1(&seq);
    if (code == CODE_NONE) continue;
    if (code == CODE_OR || code == CODE_NOT) {
      logerror("key_script: '%s' is an operator, not a key, skipped\n", tok);
      continue;
    }
    if (n >= KEYSCRIPT_MAXHELD - 1) {
      logerror("key_script: more than %d keys on one line, rest dropped\n", KEYSCRIPT_MAXHELD - 1);
      break;
    }
    out[n++] = code;
  }
  out[n] = CODE_NONE;
}

static void keyscript_load(void) {
  FILE *f;
  char line[512];
  int cap = 64;

  ks.loaded = -1;
  if (!pmoptions.key_script || !pmoptions.key_script[0]) return;
  if (!(f = fopen(pmoptions.key_script, "r"))) {
    logerror("key_script: cannot open %s\n", pmoptions.key_script);
    return;
  }
  snprintf(ks.marksName, sizeof(ks.marksName), "%s.marks", pmoptions.key_script);
  remove(ks.marksName);
  ks.ev = (keyscript_tEvent *)malloc(cap * sizeof(keyscript_tEvent));
  if (!ks.ev) {
    logerror("key_script: out of memory loading %s\n", pmoptions.key_script);
    fclose(f);
    return;
  }

  while (fgets(line, sizeof(line), f)) {
    keyscript_tEvent e;
    char verb[16], rest[400];
    int nread;
    char *hash = strchr(line, '#');
    if (hash) *hash = 0;
    memset(&e, 0, sizeof(e));
    e.code[0] = CODE_NONE;
    verb[0] = 0;
    rest[0] = 0;
    nread = sscanf(line, "%d %15s %399[^\n]", &e.frame, verb, rest);
    if (nread < 2) continue;
    if      (!strcmp(verb, "down")) {
      if (nread < 3) { logerror("key_script: 'down' with no keys, line skipped\n"); continue; }
      e.action = KS_DOWN; keyscript_parseKeys(rest, e.code);
    }
    else if (!strcmp(verb, "up"))   {
      if (nread < 3) { logerror("key_script: 'up' with no keys, line skipped\n"); continue; }
      e.action = KS_UP;   keyscript_parseKeys(rest, e.code);
    }
    else if (!strcmp(verb, "tap"))  {
      char keys[380];
      keys[0] = 0;
      if (nread < 3 || sscanf(rest, "%d %379[^\n]", &e.hold, keys) != 2) {
        logerror("key_script: 'tap' with no count/keys, line skipped\n"); continue;
      }
      e.action = KS_TAP; keyscript_parseKeys(keys, e.code);
    }
    else if (!strcmp(verb, "mark")) {
      if (nread < 3) { logerror("key_script: 'mark' with no text, line skipped\n"); continue; }
      e.action = KS_MARK;
      if (strlen(rest) >= sizeof(e.text))
        logerror("key_script: 'mark' text longer than %d bytes, truncated\n", (int)sizeof(e.text) - 1);
      strncpy(e.text, rest, sizeof(e.text) - 1);
    }
    else if (!strcmp(verb, "quit"))   e.action = KS_QUIT;
    else { logerror("key_script: unknown verb '%s'\n", verb); continue; }
    if (ks.nEv == cap) {
      keyscript_tEvent *grown = (keyscript_tEvent *)realloc(ks.ev, cap * 2 * sizeof(keyscript_tEvent));
      if (!grown) {
        logerror("key_script: out of memory, %d events loaded, rest of %s dropped\n", ks.nEv, pmoptions.key_script);
        break;
      }
      cap *= 2;
      ks.ev = grown;
    }
    ks.ev[ks.nEv++] = e;
  }
  fclose(f);

  /* stable sort by frame: file order still decides same-frame ties */
  { int i;
    for (i = 1; i < ks.nEv; i++) {
      keyscript_tEvent key = ks.ev[i];
      int j = i - 1;
      while (j >= 0 && ks.ev[j].frame > key.frame) { ks.ev[j + 1] = ks.ev[j]; j--; }
      ks.ev[j + 1] = key;
    }
  }

  ks.loaded = 1;
  logerror("key_script: %d events from %s\n", ks.nEv, pmoptions.key_script);
}

static void keyscript_hold(InputCode code, int until) {
  int i;
  for (i = 0; i < ks.nHeld; i++)
    if (ks.held[i].code == code) { ks.held[i].until = until; return; }
  if (ks.nHeld < KEYSCRIPT_MAXHELD) {
    ks.held[ks.nHeld].code  = code;
    ks.held[ks.nHeld].until = until;
    ks.nHeld++;
  } else
    logerror("key_script: more than %d keys held at once, code %d dropped\n", KEYSCRIPT_MAXHELD, (int)code);
}

static void keyscript_release(InputCode code) {
  int i;
  for (i = 0; i < ks.nHeld; i++)
    if (ks.held[i].code == code) { ks.held[i] = ks.held[--ks.nHeld]; return; }
}

void keyscript_tick(void) {
  int i;
  if (!ks.loaded) keyscript_load();
  if (ks.loaded < 0) return;

  ks.frame++;

  /* expire timed holds first, so a tap of 1 frame is one frame long */
  for (i = 0; i < ks.nHeld; )
    if (ks.held[i].until >= 0 && ks.frame > ks.held[i].until)
      ks.held[i] = ks.held[--ks.nHeld];
    else
      i++;

  while (ks.nextEv < ks.nEv && ks.ev[ks.nextEv].frame <= ks.frame) {
    const keyscript_tEvent *e = &ks.ev[ks.nextEv++];
    const InputCode *c;
    switch (e->action) {
      case KS_DOWN: for (c = e->code; *c != CODE_NONE; c++) keyscript_hold(*c, -1); break;
      case KS_UP:   for (c = e->code; *c != CODE_NONE; c++) keyscript_release(*c);  break;
      case KS_TAP:  for (c = e->code; *c != CODE_NONE; c++)
                      keyscript_hold(*c, ks.frame + (e->hold > 0 ? e->hold : 1) - 1);
                    break;
      case KS_MARK: { FILE *m = fopen(ks.marksName, "a");
                      if (m) { fprintf(m, "%d %u %s\n", ks.frame,
                                       (unsigned)(timer_get_time() * 1000.0), e->text);
                               fclose(m); } }
                    break;
      case KS_QUIT: keyscript_hold(KEYCODE_ESC, -1); break;
    }
  }
}

int keyscript_pressed(InputCode code) {
  int i;
  if (ks.loaded <= 0) return 0;
  for (i = 0; i < ks.nHeld; i++)
    if (ks.held[i].code == code) return 1;
  return 0;
}
