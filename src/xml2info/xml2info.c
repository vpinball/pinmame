/** \file
 *
 * MAME XML to INFO converter.
 *
 * This file is Public Domain.
 */

/****************************************************************************/
/* libexpat */

#if 1
/* Include the internal copy of the libexpat library */
#include "xmlrole.c"
#include "xmltok.c"
#include "xmlparse.c"
#else
/* Use the system libexpat library */
#include <expat.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

/****************************************************************************/
/* Output format */

/* Indentation */
#define INDENT "\t"

/* Output format configuration
	L1 first level
	L2 second level
	B begin a list of items
	E end a list of items
	P begin an item
	N end an item
*/

/* Output unformatted */
/*
#define L1B "("
#define L1P " "
#define L1N ""
#define L1E ")"
#define L2B "("
#define L2P " "
#define L2N ""
#define L2E ")"
*/

/* Output on one level */
#define L1B " (\n"
#define L1P INDENT
#define L1N "\n"
#define L1E ")\n\n"
#define L2B " ("
#define L2P " "
#define L2N ""
#define L2E " )"

/* Output on two levels */
/*
#define L1B " (\n"
#define L1P INDENT
#define L1N "\n"
#define L1E ")\n\n"
#define L2B " (\n"
#define L2P INDENT INDENT
#define L2N "\n"
#define L2E INDENT ")"
*/

/****************************************************************************/
/* Container */

/**
 * Initialize a container of strings.
 */
void container_alloc(char*** map, unsigned* mac, unsigned* max)
{
	*map = 0;
	*mac = 0;
	*max = 0;
}

/**
 * Deinitialize a container of strings.
 */
void container_free(char*** map, unsigned* mac, unsigned* max)
{
	unsigned i;

	for(i=0;i<*mac;++i)
		free((*map)[i]);
	free(*map);
}

/**
 * Add a element in a container of strings.
 * \return
 *  - ==0 on success
 *  - !=0 on error
 */
int container_add(char*** map, unsigned* mac, unsigned* max, const char* s, unsigned len)
{
	if (*mac >= *mac) {
		*max = 256 + *mac * 2;
		*map = realloc(*map, *max * sizeof(char*));
		if (!*map) {
			return -1;
		}
	}

	(*map)[*mac] = malloc(len + 1);
	if (!(*map)[*mac]) {
		return -1;
	}

	memcpy((*map)[*mac], s, len);
	(*map)[*mac][len] = 0;

	*mac += 1;

	return 0;
}

/**
 * Compare elements in the container.
 */
int container_compare(const void* a, const void* b)
{
	const char* ar = *(const char**)a;
	const char* br = *(const char**)b;
	return strcmp(ar, br);
}

/****************************************************************************/
/* Parser */

/**
 * Max depth checked.
 */
#define DEPTH_MAX 5

enum token_t {
	token_open,
	token_close,
	token_data
};

struct state_t;

typedef void (process_t)(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes);

/**
 * State for every depth level.
 */
struct level_t {
	const char* tag; /**< Tag name. */
	char* data; /**< Accumulative data. */
	unsigned len; /**< Length of the data. */
	process_t* process; /**< Processing function. */
	int flag; /**< Generic flag used for internal information. */
};

/**
 * Global parsing state.
 */
struct state_t {
	XML_Parser parser; /**< Parser. */
	int depth; /**< Current depth. */
	struct level_t level[DEPTH_MAX]; /**< Level state. */
	FILE* os; /**< Output file. */
	int error; /**< Error flag. */
	int dip_default; /**< Dipswitch default attrib. */
	unsigned game_mac; /**< Collection of game name. Number of stored element. */
	unsigned game_max; /**< Collection of game name. Number of allocated element. */
	char** game_map; /**< Collection of game name. Element vector. */
	unsigned reference_mac; /**< Collection of reference of game name. */
	unsigned reference_max; /**< Collection of reference of game name. Number of allocated element. */
	char** reference_map; /**< Collection of reference of game name. Element vector. */
};

/****************************************************************************/
/* Processing */

void print_item(FILE* os, const char* s, unsigned len)
{
	unsigned i;

	for(i=0;i<len;++i) {
		if (isspace(s[i])) {
			fprintf(os, "_");
		} else {
			switch (s[i]) {
			case '(' :
			case ')' :
			case '"' :
				fprintf(os, "_");
				break;
			default:
				fprintf(os, "%c", s[i]);
			}
		}
	}
}

void print_string(FILE* os, const char* s, unsigned len)
{
	unsigned i;
	fprintf(os, "\"");
	for(i=0;i<len;++i) {
		switch (s[i]) {
		case '\a' : fprintf(os, "\\a"); break;
		case '\b' : fprintf(os, "\\b"); break;
		case '\f' : fprintf(os, "\\f"); break;
		case '\n' : fprintf(os, "\\n"); break;
		case '\r' : fprintf(os, "\\r"); break;
		case '\t' : fprintf(os, "\\t"); break;
		case '\v' : fprintf(os, "\\v"); break;
		case '\\' : fprintf(os, "\\\\"); break;
		case '\"' : fprintf(os, "\\\""); break;
		default:
			if (s[i]>=' ' && s[i]<='~')
				fprintf(os, "%c", s[i]);
			else
				fprintf(os, "\\x%02x", (unsigned)(unsigned char)s[i]);
		}
	}
	fprintf(os, "\"");
}

void process_error(struct state_t* state, const char* tag, const char* msg)
{
	fprintf(stderr, "%d:%s:%s\n", XML_GetCurrentLineNumber(state->parser), tag, msg);
	state->error = 1;
}

void process_game(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_open) {
		unsigned i;
		if (!attributes) {
			process_error(state, state->level[1].tag, "missing runnable attrib");
			return;
		}
		for(i=0;attributes[i];i+=2)
			if (strcmp(attributes[i],"runnable")==0)
				break;
		if (!attributes[i]) {
			process_error(state, state->level[1].tag, "missing runnable attrib");
			return;
		}
		if (strcmp(attributes[i+1],"no")==0)
			fprintf(state->os, "%s" L1B, "resource");
		else
			fprintf(state->os, "%s" L1B, state->level[1].tag);
		state->level[1].flag = 0;
	} else if (t == token_close) {
		fprintf(state->os, L1E);
		if (!state->level[1].flag) {
			process_error(state, state->level[1].tag, "missing name attrib");
			return;
		}
	}
}

void process_string2(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_data) {
		fprintf(state->os, L1P "%s ", state->level[2].tag);
		print_string(state->os, s, len);
		fprintf(state->os, L1N);
	}
}

void process_item2(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_data) {
		fprintf(state->os, L1P "%s ", state->level[2].tag);
		if (len == 0) {
			process_error(state, state->level[2].tag, "empty token");
			return;
		}
		print_item(state->os, s, len);
		fprintf(state->os, L1N);
	}
}

void process_gamename(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_data) {
		if (container_add(&state->game_map, &state->game_mac, &state->game_max, s, len) != 0) {
			process_error(state, state->level[2].tag, "low memory");
			return;
		}
		state->level[1].flag = 1;
		process_item2(state, t, s, len, attributes);
	}
}

void process_reference(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_data) {
		if (container_add(&state->reference_map, &state->reference_mac, &state->reference_max, s, len) != 0) {
			process_error(state, state->level[2].tag, "low memory");
			return;
		}
		process_item2(state, t, s, len, attributes);
	}
}

void process_set2(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_open) {
		fprintf(state->os, L1P "%s" L2B, state->level[2].tag);
	} else if (t == token_close) {
		fprintf(state->os, L2E L1N);
	}
}

void process_string3(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_data) {
		fprintf(state->os, L2P "%s ", state->level[3].tag);
		print_string(state->os, s, len);
		fprintf(state->os, "%s", L2N);
	}
}

void process_item3(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_data) {
		fprintf(state->os, L2P "%s ", state->level[3].tag);
		if (len == 0) {
			process_error(state, state->level[3].tag, "empty token");
			return;
		}
		print_item(state->os, s, len);
		fprintf(state->os, "%s", L2N);
	}
}

void process_num3(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_data) {
		unsigned i;
		for(i=0;i<len;++i) {
			if (!isdigit(s[i])) {
				process_error(state, state->level[3].tag, "integer number expected");
				return;
			}
		}
		process_item3(state, t, s, len, attributes);
	}
}

void process_hex3(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_data) {
		unsigned i;
		for(i=0;i<len;++i) {
			if (!isxdigit(s[i])) {
				process_error(state, state->level[3].tag, "hex number expected");
				return;
			}
		}
		process_item3(state, t, s, len, attributes);
	}
}

void process_float3(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_data) {
		unsigned i;
		for(i=0;i<len;++i) {
			if (!isdigit(s[i]) && s[i] != '.') {
				process_error(state, state->level[3].tag, "float number expected");
				return;
			}
		}
		process_item3(state, t, s, len, attributes);
	}
}

/* Convert gamelist:game:sample:name -> game:sample */
void process_samplename(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_data) {
		fprintf(state->os, L1P "%s ", state->level[2].tag);
		if (len == 0) {
			process_error(state, state->level[2].tag, "empty token");
			return;
		}
		print_item(state->os, s, len);
		fprintf(state->os, L1N);
	}
}

/* Convert gamelist:game:rom:offset -> game:rom:offs */
void process_romoffset(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_data) {
		state->level[3].tag = "offs";
		process_item3(state, t, s, len, attributes);
	}
}

/* Convert gamelist:game:rom:status=baddump|nodump|good -> game:rom:flags=baddump|nodump */
void process_romstatus(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_data) {
		if (strncmp("baddump", s, len) == 0) {
			fprintf(state->os, L2P "flags baddump" L2N);
		} else if (strncmp("nodump", s, len) == 0) {
			fprintf(state->os, L2P "flags nodump" L2N);
		}
	}
}

/* Convert gamelist:game:rom:dispose=yes -> game:rom:flags=dispose */
void process_romdispose(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_data) {
		if (strncmp("yes", s, len) == 0) {
			fprintf(state->os, L2P "flags dispose" L2N);
		}
	}
}

/* Convert gamelist:game:rom:soundonly=yes -> game:rom:flags=soundonly */
void process_romsoundonly(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_data) {
		if (strncmp("yes", s, len) == 0) {
			fprintf(state->os, L2P "flags soundonly" L2N);
		}
	}
}

/* Convert gamelist:game:chip:audio=yes -> game:rom:flags=audio */
void process_chipsoundonly(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_data) {
		if (strncmp("yes", s, len) == 0) {
			fprintf(state->os, L2P "flags audio" L2N);
		}
	}
}

/* Remove gamelist:game:input:tilt=no */
void process_inputtilt(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_data) {
		if (strncmp("yes", s, len) == 0) {
			fprintf(state->os, L2P "tilt yes" L2N);
		}
	}
}

/* Remove gamelist:game:input:service=no */
void process_inputservice(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_data) {
		if (strncmp("yes", s, len) == 0) {
			fprintf(state->os, L2P "service yes" L2N);
		}
	}
}

/* Convert gamelist:game:video:refresh -> game:video:freq */
void process_videorefresh(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_data) {
		state->level[3].tag = "freq";
		process_float3(state, t, s, len, attributes);
	}
}

/* Convert gamelist:game:video:width -> game:video:x */
void process_videowidth(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_data) {
		state->level[3].tag = "x";
		process_num3(state, t, s, len, attributes);
	}
}

/* Convert gamelist:game:video:height -> game:video:y */
void process_videoheight(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_data) {
		state->level[3].tag = "y";
		process_num3(state, t, s, len, attributes);
	}
}

/* Detect Convert gamelist:game:dipswitch:dipvalue:default */
void process_dipswitchdipvalue(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_open) {
		unsigned i;
		if (!attributes) {
			process_error(state, state->level[2].tag, "missing default attrib");
			return;
		}
		for(i=0;attributes[i];i+=2)
			if (strcmp(attributes[i], "default")==0)
				break;
		if (!attributes[i]) {
			process_error(state, state->level[2].tag, "missing default attrib");
			return;
		}
		state->dip_default = strcmp(attributes[i+1],"yes")==0;
	} else if (t == token_close) {
		state->dip_default = 0;
	}
}

/* Convert gamelist:game:dipswitch:dipvalue:name -> game:dipswitch:entry */
void process_dipswitchdipvaluename(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_data) {
		fprintf(state->os, L2P "entry ");
		print_string(state->os, s, len);
		fprintf(state->os, "%s", L2N);
		if (state->dip_default) {
			fprintf(state->os, L2P "default ");
			print_string(state->os, s, len);
			fprintf(state->os, "%s", L2N);
		}
	}
}

/**
 * Conversion table.
 * Any element/attribute not in this table is ignored.
 */
struct conversion_t {
	unsigned depth;
	const char* name[DEPTH_MAX];
	process_t* process;
} CONV[] = {
	{ 1, { "mame", "game", 0, 0, 0 }, process_game },
	{ 2, { "mame", "game", "name", 0, 0 }, process_gamename },
	{ 2, { "mame", "game", "description", 0, 0 }, process_string2 },
	{ 2, { "mame", "game", "year", 0, 0 }, process_item2 },
	{ 2, { "mame", "game", "history", 0, 0 }, process_string2 },
	{ 2, { "mame", "game", "manufacturer", 0, 0 }, process_string2 },
	{ 2, { "mame", "game", "disk", 0, 0 }, process_set2 },
	{ 3, { "mame", "game", "disk", "name", 0 }, process_item3 },
	{ 3, { "mame", "game", "disk", "md5", 0 }, process_hex3 },
	{ 3, { "mame", "game", "disk", "sha1", 0 }, process_hex3 },
	{ 3, { "mame", "game", "disk", "region", 0 }, process_item3 },
	{ 3, { "mame", "game", "disk", "index", 0 }, process_num3 },
	{ 2, { "mame", "game", "cloneof", 0, 0 }, process_reference },
	{ 2, { "mame", "game", "romof", 0, 0 }, process_reference },
	{ 2, { "mame", "game", "rom", 0, 0 }, process_set2 },
	{ 3, { "mame", "game", "rom", "name", 0 }, process_item3 },
	{ 3, { "mame", "game", "rom", "merge", 0 }, process_item3 },
	{ 3, { "mame", "game", "rom", "size", 0 }, process_num3 },
	{ 3, { "mame", "game", "rom", "crc", 0 }, process_hex3 },
	{ 3, { "mame", "game", "rom", "md5", 0 }, process_hex3 },
	{ 3, { "mame", "game", "rom", "sha1", 0 }, process_hex3 },
	{ 3, { "mame", "game", "rom", "region", 0 }, process_item3 },
	{ 3, { "mame", "game", "rom", "offset", 0 }, process_romoffset },
	{ 3, { "mame", "game", "rom", "status", 0 }, process_romstatus },
	{ 3, { "mame", "game", "rom", "dispose", 0 }, process_romdispose },
	{ 3, { "mame", "game", "rom", "soundonly", 0 }, process_romsoundonly },
	{ 2, { "mame", "game", "sampleof", 0, 0 }, process_reference },
	{ 3, { "mame", "game", "sample", "name", 0 }, process_samplename },
	{ 2, { "mame", "game", "chip", 0, 0 }, process_set2 },
	{ 3, { "mame", "game", "chip", "type", 0 }, process_item3 },
	{ 3, { "mame", "game", "chip", "name", 0 }, process_item3 },
	{ 3, { "mame", "game", "chip", "soundonly", 0 }, process_chipsoundonly },
	{ 3, { "mame", "game", "chip", "clock", 0 }, process_num3 },
	{ 2, { "mame", "game", "video", 0, 0 }, process_set2 },
	{ 3, { "mame", "game", "video", "screen", 0 }, process_item3 },
	{ 3, { "mame", "game", "video", "orientation", 0 }, process_item3 },
	{ 3, { "mame", "game", "video", "width", 0 }, process_videowidth },
	{ 3, { "mame", "game", "video", "height", 0 }, process_videoheight },
	{ 3, { "mame", "game", "video", "aspectx", 0 }, process_num3 },
	{ 3, { "mame", "game", "video", "aspecty", 0 }, process_num3 },
	{ 3, { "mame", "game", "video", "refresh", 0 }, process_videorefresh },
	{ 2, { "mame", "game", "sound", 0, 0 }, process_set2 },
	{ 3, { "mame", "game", "sound", "channels", 0 }, process_num3 },
	{ 2, { "mame", "game", "input", 0, 0 }, process_set2 },
	{ 3, { "mame", "game", "input", "players", 0 }, process_num3 },
	{ 3, { "mame", "game", "input", "control", 0 }, process_item3 },
	{ 3, { "mame", "game", "input", "buttons", 0 }, process_num3 },
	{ 3, { "mame", "game", "input", "coins", 0 }, process_num3 },
	{ 3, { "mame", "game", "input", "tilt", 0 }, process_inputtilt },
	{ 3, { "mame", "game", "input", "service", 0 }, process_inputservice },
	{ 2, { "mame", "game", "dipswitch", 0, 0 }, process_set2 },
	{ 3, { "mame", "game", "dipswitch", "name", 0 }, process_string3 },
	{ 3, { "mame", "game", "dipswitch", "dipvalue", 0 }, process_dipswitchdipvalue },
	{ 4, { "mame", "game", "dipswitch", "dipvalue", "name" }, process_dipswitchdipvaluename },
	{ 2, { "mame", "game", "driver", 0, 0 }, process_set2 },
	{ 3, { "mame", "game", "driver", "status", 0 }, process_item3 },
	{ 3, { "mame", "game", "driver", "color", 0 }, process_item3 },
	{ 3, { "mame", "game", "driver", "sound", 0 }, process_item3 },
	{ 3, { "mame", "game", "driver", "palettesize", 0 }, process_num3 },
	{ 0, { 0, 0, 0, 0, 0 }, 0 }
};

/**
 * Identify the specified element/attribute.
 */
struct conversion_t* identify(unsigned depth, const struct level_t* level)
{
	unsigned i, j;

	if (depth < DEPTH_MAX) {
		for(i=0;CONV[i].name[0];++i) {
			if (CONV[i].depth != depth)
				continue;
			for(j=0;j<=depth;++j) {
				if (strcmp(level[j].tag, CONV[i].name[j]) != 0)
					break;
			}
			if (j > depth)
				break;
		}
		if (CONV[i].name[0])
			return &CONV[i];
	}

	return 0;
}

/**
 * End Handler for the Expat parser.
 */
void end_handler(void* data, const XML_Char* name)
{
	struct state_t* state = (struct state_t*)data;

	if (state->depth < DEPTH_MAX) {
		if (state->error == 0) {
			if (state->level[state->depth].process) {
				state->level[state->depth].process(state, token_data, state->level[state->depth].data, state->level[state->depth].len, 0);
				state->level[state->depth].process(state, token_close, 0, 0, 0);
			}
		}
		free(state->level[state->depth].data);
	}

	--state->depth;
}

/**
 * Data Handler for the Expat parser.
 */
void data_handler(void* data, const XML_Char* s, int len)
{
	struct state_t* state = (struct state_t*)data;

	if (state->depth < DEPTH_MAX) {
		if (state->error == 0) {
			/* accumulate the data */
			unsigned new_len = state->level[state->depth].len + len;
			state->level[state->depth].data = realloc(state->level[state->depth].data, new_len);
			if (!state->level[state->depth].data) {
				process_error(state, state->level[state->depth].tag, "low memory");
				return;
			}
			memcpy(state->level[state->depth].data + state->level[state->depth].len, s, len);
			state->level[state->depth].len += len;
		}
	}
}

/**
 * Start Handler for the Expat parser.
 */
void start_handler(void* data, const XML_Char* name, const XML_Char** attributes)
{
	struct state_t* state = (struct state_t*)data;
	struct conversion_t* c;
	unsigned i;

	++state->depth;

	if (state->depth < DEPTH_MAX) {
		state->level[state->depth].tag = name;
		state->level[state->depth].data = 0;
		state->level[state->depth].len = 0;

		if (state->error == 0) {
			c = identify(state->depth, state->level);
			if (c) {
				state->level[state->depth].process = c->process;
				state->level[state->depth].process(state, token_open, 0, 0, attributes);
			} else {
				state->level[state->depth].process = 0;
			}

			for(i=0;attributes[i];i+=2) {
				const char* null_atts[1] = { 0 };
				start_handler(data, attributes[i], null_atts);
				data_handler(data, attributes[i+1], strlen(attributes[i+1]));
				end_handler(data, attributes[i]);
			}
		} else {
			state->level[state->depth].process = 0;
		}
	}
}

/**
 * Check the correctness of the gathered information.
 */
int check(struct state_t* state)
{
	unsigned i;

	/* sort the container */
	qsort(state->game_map, state->game_mac, sizeof(char*), container_compare);

	/* check for duplicate */
	for(i=1;i<state->game_mac;++i) {
		if (strcmp(state->game_map[i-1], state->game_map[i]) == 0) {
			fprintf(stderr, "::duplicate game %s\n", state->game_map[i]);
			return -1;
		}
	}

	/* check the presence of any reference */
	for(i=0;i<state->reference_mac;++i) {
		void* p = bsearch(&state->reference_map[i], state->game_map, state->game_mac, sizeof(char*), container_compare);
		if (!p) {
			fprintf(stderr, "::reference to a missing game %s\n", state->reference_map[i]);
			return -1;
		}
	}

	return 0;
}

/**
 * Convert the XML input to INFO output.
 */
int process(FILE* is, FILE* os)
{
	struct state_t state;
	char buf[4096];

	state.parser = XML_ParserCreate(NULL);
	if (!state.parser) {
		fprintf(stderr, "Couldn't allocate memory for parser\n");
		return -1;
	}

	state.depth = -1;
	state.os = os;
	state.error = 0;
	container_alloc(&state.game_map, &state.game_mac, &state.game_max);
	container_alloc(&state.reference_map, &state.reference_mac, &state.reference_max);

	XML_SetUserData(state.parser, &state);
	XML_SetElementHandler(state.parser, start_handler, end_handler);
	XML_SetCharacterDataHandler(state.parser, data_handler);

	while (1) {
		int done;
		int len;

		len = fread(buf, 1, sizeof(buf), is);
		if (ferror(is)) {
			process_error(&state, "", "read error");
			break;
		}

		done = feof(is);

		if (XML_Parse(state.parser, buf, len, done) == XML_STATUS_ERROR) {
			process_error(&state, "", XML_ErrorString(XML_GetErrorCode(state.parser)));
			break;
		}

		if (done)
			break;
	}

	XML_ParserFree(state.parser);

	if (state.error) {
		return -1;
	}

	if (check(&state) != 0) {
		return -1;
	}

	container_free(&state.game_map, &state.game_mac, &state.game_max);
	container_free(&state.reference_map, &state.reference_mac, &state.reference_max);

	return 0;
}

int main(int argc, char* argv[])
{
	int i;

	for(i=1;i<argc;++i) {
		if (strcmp(argv[i],"-version")==0) {
			printf("xml2info 1.0\n");
			printf("libexpat %d.%d.%d\n", XML_MAJOR_VERSION, XML_MINOR_VERSION, XML_MICRO_VERSION);
			exit(EXIT_SUCCESS);
		} else {
			fprintf(stderr, "MAME XML to INFO converter\n");
			fprintf(stderr, "Syntax: xml2info [-version] < input.xml > output.lst\n");
			exit(EXIT_SUCCESS);
		}
	}

	if (process(stdin, stdout) != 0) {
		exit(EXIT_FAILURE);
	}

	return EXIT_SUCCESS;
}


