// license:BSD-3-Clause

/***************************************************************************
 * PinMAME Remote Debugger - HTTP API handlers                             *
 * See api_handler.h; endpoint reference is served at /api/doc.            *
 *                                                                         *
 * Parameter conventions (also documented in README.md):                   *
 *   addr, val, bank, pattern, data : hexadecimal (optional 0x/$ prefix)   *
 *   size, lines, before, cpu, sw, idx, len, mode, ignore : decimal        *
 * All handlers run on the HTTP server thread and take the debugger lock   *
 * (directly or via remote_debug_* functions) around emulator access.      *
 ***************************************************************************/
#ifdef REMOTE_DEBUG

#include "api_handler.h"
#include "remote_debug.h"
#include "http_server.h"
#include "driver.h"
#include "mame.h"
#include "wpc/core.h"
#include "wpc/wpc.h"
#include "cpuintrf.h"
#include "cpu/m6809/m6809.h"
#include "cpu/adsp2100/adsp2100.h"
#include "memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Provided by src/wpc/wpc.c / src/wpc/core.c */
extern int wpc_get_bank(void);
extern void core_get_dmd_data(int layout_idx, float **pixels, int *width, int *height);
extern UINT8 *wpc_ram;

/* ------------------------------------------------------------------ */
/* Small helpers                                                      */
/* ------------------------------------------------------------------ */

/* Decode %XX and '+' URL escapes; dst holds at most dstlen bytes. */
static void url_decode(char *dst, int dstlen, const char *src)
{
	int o = 0;
	while (*src && o < dstlen - 1) {
		if (src[0] == '%' && isxdigit((unsigned char)src[1]) && isxdigit((unsigned char)src[2])) {
			char hex[3];
			hex[0] = src[1];
			hex[1] = src[2];
			hex[2] = 0;
			dst[o++] = (char)strtoul(hex, NULL, 16);
			src += 3;
		}
		else if (*src == '+') {
			dst[o++] = ' ';
			src++;
		}
		else {
			dst[o++] = *src++;
		}
	}
	dst[o] = 0;
}

/* Extract the (URL-decoded) value of key from a query string.
 * Matches keys exactly (start of string or after '&'). dest is set to
 * the empty string when the key is absent. */
static void get_query_param(const char *query, const char *key, char *dest, int max_len)
{
	size_t klen = strlen(key);
	const char *p = query;
	dest[0] = 0;
	while (p && *p) {
		if (strncmp(p, key, klen) == 0 && p[klen] == '=') {
			const char *v = p + klen + 1;
			const char *end = strchr(v, '&');
			char raw[512];
			size_t n = end ? (size_t)(end - v) : strlen(v);
			if (n >= sizeof(raw))
				n = sizeof(raw) - 1;
			memcpy(raw, v, n);
			raw[n] = 0;
			url_decode(dest, max_len, raw);
			return;
		}
		p = strchr(p, '&');
		if (p)
			p++;
	}
}

/* Parse a hexadecimal number (optional 0x/0X/$ prefix). */
static UINT32 parse_hex(const char *str)
{
	if (!str || !*str)
		return 0;
	if (str[0] == '$')
		return (UINT32)strtoul(str + 1, NULL, 16);
	if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X'))
		return (UINT32)strtoul(str + 2, NULL, 16);
	return (UINT32)strtoul(str, NULL, 16);
}

/* Parse a decimal number; a 0x/$ prefix switches to hexadecimal. */
static int parse_int(const char *str)
{
	if (!str || !*str)
		return 0;
	if (str[0] == '$')
		return (int)strtoul(str + 1, NULL, 16);
	if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X'))
		return (int)strtoul(str + 2, NULL, 16);
	return atoi(str);
}

/* Parse a hex byte string ("DEADBEEF") into bytes; returns the count. */
static int parse_hex_bytes(const char *str, UINT8 *out, int maxlen)
{
	int n = 0;
	size_t len = strlen(str);
	size_t i;
	for (i = 0; i + 1 < len && n < maxlen; i += 2) {
		char hex[3];
		if (!isxdigit((unsigned char)str[i]) || !isxdigit((unsigned char)str[i + 1]))
			break;
		hex[0] = str[i];
		hex[1] = str[i + 1];
		hex[2] = 0;
		out[n++] = (UINT8)strtoul(hex, NULL, 16);
	}
	return n;
}

/* Respond with a malloc()ed copy of json and the given HTTP status. */
static void respond_json(http_response_t *resp, int status, const char *json)
{
	resp->body = strdup(json);
	resp->len = resp->body ? (int)strlen(resp->body) : 0;
	resp->status = status;
	strcpy(resp->content_type, "application/json");
}

static void respond_ok(http_response_t *resp)
{
	respond_json(resp, 200, "{\"status\": \"ok\"}");
}

static void respond_error(http_response_t *resp, int status, const char *message)
{
	char buf[256];
	char esc[160];
	snprintf(buf, sizeof(buf), "{\"status\": \"error\", \"message\": \"%s\"}",
	         remote_debug_json_escape(esc, (int)sizeof(esc), message));
	respond_json(resp, status, buf);
}

/* Take a buffer produced by a remote_debug_get_* function. */
static void respond_owned(http_response_t *resp, char *body, int len, const char *ctype)
{
	if (body) {
		resp->body = body;
		resp->len = len;
		resp->status = 200;
		strcpy(resp->content_type, ctype);
	}
	else {
		respond_error(resp, 500, "out of memory");
	}
}

/* Resolve a register name or numeric id for /api/debugger/state/write.
 * Understands M6809 and ADSP2100 family names. Returns -1 if unknown. */
static int resolve_register_id(int cpu_idx, const char *name)
{
	static const struct { const char *name; int id; } m6809_regs[] = {
		{"PC", M6809_PC}, {"S", M6809_S}, {"SP", M6809_S},
		{"CC", M6809_CC}, {"FLAGS", M6809_CC}, {"A", M6809_A},
		{"B", M6809_B}, {"U", M6809_U}, {"X", M6809_X},
		{"Y", M6809_Y}, {"DP", M6809_DP}
	};
	static const struct { const char *name; int id; } adsp_regs[] = {
		{"PC", ADSP2100_PC}, {"AX0", ADSP2100_AX0}, {"AX1", ADSP2100_AX1},
		{"AY0", ADSP2100_AY0}, {"AY1", ADSP2100_AY1}, {"AR", ADSP2100_AR},
		{"AF", ADSP2100_AF}, {"MX0", ADSP2100_MX0}, {"MX1", ADSP2100_MX1},
		{"MY0", ADSP2100_MY0}, {"MY1", ADSP2100_MY1}, {"MR0", ADSP2100_MR0},
		{"MR1", ADSP2100_MR1}, {"MR2", ADSP2100_MR2}, {"MF", ADSP2100_MF},
		{"SI", ADSP2100_SI}, {"SE", ADSP2100_SE}, {"SB", ADSP2100_SB},
		{"SR0", ADSP2100_SR0}, {"SR1", ADSP2100_SR1}, {"CNTR", ADSP2100_CNTR},
		{"ASTAT", ADSP2100_ASTAT}, {"MSTAT", ADSP2100_MSTAT},
		{"SSTAT", ADSP2100_SSTAT}, {"IMASK", ADSP2100_IMASK},
		{"ICNTL", ADSP2100_ICNTL}
	};
	int cpu_type;
	size_t i;

	if (name[0] == '\0')
		return -1;
	if (isdigit((unsigned char)name[0]))
		return atoi(name);

	cpu_type = CPU_M6809;
	remote_debug_lock();
	if (Machine && cpu_idx >= 0 && cpu_idx < cpu_gettotalcpu())
		cpu_type = Machine->drv->cpu[cpu_idx].cpu_type;
	remote_debug_unlock();

	if (cpu_type == CPU_ADSP2105) {
		for (i = 0; i < sizeof(adsp_regs) / sizeof(adsp_regs[0]); i++) {
			if (strcasecmp(name, adsp_regs[i].name) == 0)
				return adsp_regs[i].id;
		}
	}
	else {
		for (i = 0; i < sizeof(m6809_regs) / sizeof(m6809_regs[0]); i++) {
			if (strcasecmp(name, m6809_regs[i].name) == 0)
				return m6809_regs[i].id;
		}
	}
	return -1;
}

/* ------------------------------------------------------------------ */
/* Classic single-line commands (MAME debugger style)                 */
/* ------------------------------------------------------------------ */

static void handle_classic_command(const char *cmd_line)
{
	char clean[256];
	char *p, *cmd;

	strncpy(clean, cmd_line, sizeof(clean) - 1);
	clean[sizeof(clean) - 1] = 0;
	for (p = clean; *p; p++)
		*p = (char)toupper((unsigned char)*p);

	cmd = strtok(clean, " ,");
	if (!cmd)
		return;
	if (strcmp(cmd, "BP") == 0) {
		char *as = strtok(NULL, " ,");
		if (as) {
			char *colon = strchr(as, ':');
			if (colon) {
				*colon = 0;
				remote_debug_breakpoint_add_banked(parse_hex(colon + 1), (int)parse_hex(as));
			}
			else {
				remote_debug_breakpoint_add(parse_hex(as));
			}
		}
	}
	else if (strcmp(cmd, "BC") == 0) {
		remote_debug_breakpoint_clear();
	}
	else if (strcmp(cmd, "WP") == 0) {
		char *as = strtok(NULL, " ,");
		char *sz = strtok(NULL, " ,");
		char *md = strtok(NULL, " ,");
		if (as) {
			int mode = REMOTE_DEBUG_WP_RW;
			int len = sz ? (int)parse_hex(sz) : 1;
			int bank = -1;
			char *colon = strchr(as, ':');   /* optional bank:addr */
			if (colon) {
				*colon = 0;
				bank = (int)parse_hex(as);
				as = colon + 1;
			}
			if (md) {
				if (strstr(md, "RW"))
					mode = REMOTE_DEBUG_WP_RW;
				else if (strchr(md, 'R'))
					mode = REMOTE_DEBUG_WP_READ;
				else if (strchr(md, 'W'))
					mode = REMOTE_DEBUG_WP_WRITE;
			}
			remote_debug_watchpoint_add(parse_hex(as), len, mode, bank,
			                            REMOTE_DEBUG_COND_NONE, 0);
		}
	}
	else if (strcmp(cmd, "WC") == 0) {
		remote_debug_watchpoint_clear();
	}
	else if (strcmp(cmd, "G") == 0) {
		remote_debug_set_paused(0);
	}
	else if (strcmp(cmd, "S") == 0) {
		remote_debug_step();
	}
	else if (strcmp(cmd, "F") == 0) {
		char *as = strtok(NULL, " ,");
		char *sz = strtok(NULL, " ,");
		char *vl = strtok(NULL, " ,");
		if (as && sz && vl)
			remote_debug_memory_fill(0, parse_hex(as), (int)parse_hex(sz), (UINT8)parse_hex(vl));
	}
	else if (strcmp(cmd, "QUIT") == 0) {
		remote_debug_quit();
	}
	else if (strcmp(cmd, "HELP") == 0) {
		remote_debug_add_message("Commands: BP [bank:]addr, BC, WP addr[,len[,type]], WC, G, S, F addr,len,val, QUIT (all values hex)");
	}
	else {
		char err[128];
		snprintf(err, sizeof(err), "Unknown command: %.80s", cmd);
		remote_debug_add_message(err);
	}
}

/* ------------------------------------------------------------------ */
/* Handlers                                                           */
/* ------------------------------------------------------------------ */

static void handle_api_info(const http_request_t *req, http_response_t *resp)
{
	char *buffer;
	(void)req;
	buffer = malloc(8192);
	if (!buffer) {
		respond_error(resp, 500, "out of memory");
		return;
	}
	remote_debug_lock();
	if (Machine && Machine->gamedrv) {
		int bank = wpc_get_bank();
		char lamp_hex[CORE_MAXLAMPCOL * 2 + 1];
		char sw_hex[CORE_MAXSWCOL * 2 + 1];
		char seg_hex[CORE_SEGCOUNT * 4 + 1];
		char desc_esc[256];
		int i, ded, len;
		for (i = 0; i < CORE_MAXLAMPCOL; i++)
			sprintf(lamp_hex + i * 2, "%02X", coreGlobals.lampMatrix[i]);
		for (i = 0; i < CORE_MAXSWCOL; i++)
			sprintf(sw_hex + i * 2, "%02X", coreGlobals.swMatrix[i]);
		for (i = 0; i < CORE_SEGCOUNT; i++)
			sprintf(seg_hex + i * 4, "%04X", coreGlobals.segments[i].w);
		ded = wpc_data ? wpc_data[0] : 0;
		len = snprintf(buffer, 8192,
			"{\"game\": \"%s\", \"description\": \"%s\", \"manufacturer\": \"%s\", "
			"\"year\": \"%s\", \"paused\": %d, \"wpc_bank\": %d, \"lamps\": \"%s\", "
			"\"switches\": \"%s\", \"segments\": \"%s\", \"dedicated\": %d, "
			"\"solenoids\": %u, \"solenoids2\": %u}",
			Machine->gamedrv->name,
			remote_debug_json_escape(desc_esc, (int)sizeof(desc_esc), Machine->gamedrv->description),
			Machine->gamedrv->manufacturer, Machine->gamedrv->year,
			remote_debug_is_paused(), bank, lamp_hex, sw_hex, seg_hex, ded,
			coreGlobals.solenoids, coreGlobals.solenoids2);
		remote_debug_unlock();
		resp->body = buffer;
		resp->len = (len > 0 && len < 8192) ? len : (int)strlen(buffer);
		resp->status = 200;
		strcpy(resp->content_type, "application/json");
	}
	else {
		remote_debug_unlock();
		free(buffer);
		respond_json(resp, 503, "{\"status\": \"initializing\"}");
	}
}

static void handle_api_events(const http_request_t *req, http_response_t *resp)
{
	(void)req;
	resp->sse = 1;
	resp->status = 200;
}

static void handle_api_dmd_info(const http_request_t *req, http_response_t *resp)
{
	float *px;
	int w, h;
	(void)req;
	remote_debug_lock();
	core_get_dmd_data(0, &px, &w, &h);
	remote_debug_unlock();
	if (px && w > 0 && h > 0) {
		char res[128];
		snprintf(res, sizeof(res), "{\"width\": %d, \"height\": %d}", w, h);
		respond_json(resp, 200, res);
	}
	else {
		respond_error(resp, 404, "DMD not initialized");
	}
}

static void handle_api_dmd_raw(const http_request_t *req, http_response_t *resp)
{
	char *body = NULL;
	int len = 0;
	(void)req;
	remote_debug_get_dmd_screenshot(&body, &len, resp->content_type);
	if (body)
		respond_owned(resp, body, len, resp->content_type);
	else
		respond_error(resp, 404, "DMD not initialized");
}

static void handle_api_dmd_pnm(const http_request_t *req, http_response_t *resp)
{
	char *body = NULL;
	int len = 0;
	(void)req;
	remote_debug_get_dmd_pnm(&body, &len, resp->content_type);
	if (body)
		respond_owned(resp, body, len, resp->content_type);
	else
		respond_error(resp, 404, "DMD not initialized");
}

static void handle_api_screenshot_info(const http_request_t *req, http_response_t *resp)
{
	int w, h;
	char res[128];
	(void)req;
	remote_debug_get_screenshot_info(&w, &h);
	snprintf(res, sizeof(res), "{\"width\": %d, \"height\": %d}", w, h);
	respond_json(resp, 200, res);
}

static void handle_api_screenshot_raw(const http_request_t *req, http_response_t *resp)
{
	char *body = NULL;
	int len = 0;
	(void)req;
	remote_debug_get_raw_screenshot(&body, &len);
	if (body)
		respond_owned(resp, body, len, "application/octet-stream");
	else
		respond_error(resp, 404, "no display captured yet");
}

static void handle_api_screenshot_pnm(const http_request_t *req, http_response_t *resp)
{
	char *body = NULL;
	int len = 0;
	(void)req;
	remote_debug_get_screenshot(&body, &len, resp->content_type);
	if (body)
		respond_owned(resp, body, len, resp->content_type);
	else
		respond_error(resp, 404, "no display captured yet");
}

static void handle_api_debugger_command(const http_request_t *req, http_response_t *resp)
{
	char cmd_buf[256];
	get_query_param(req->query, "cmd", cmd_buf, (int)sizeof(cmd_buf));
	if (!cmd_buf[0]) {
		respond_error(resp, 400, "missing parameter: cmd");
		return;
	}
	handle_classic_command(cmd_buf);
	respond_ok(resp);
}

static void handle_api_debugger_control_runto(const http_request_t *req, http_response_t *resp)
{
	char addr_buf[32], bank_buf[32];
	int bank;
	get_query_param(req->query, "addr", addr_buf, (int)sizeof(addr_buf));
	get_query_param(req->query, "bank", bank_buf, (int)sizeof(bank_buf));
	if (!addr_buf[0]) {
		respond_error(resp, 400, "missing parameter: addr");
		return;
	}
	bank = bank_buf[0] ? (int)parse_hex(bank_buf) : -1;
	remote_debug_run_to(parse_hex(addr_buf), bank);
	respond_ok(resp);
}

static void handle_api_debugger_control(const http_request_t *req, http_response_t *resp)
{
	char cmd_buf[32];
	get_query_param(req->query, "cmd", cmd_buf, (int)sizeof(cmd_buf));
	if (strcmp(cmd_buf, "pause") == 0)
		remote_debug_set_paused(1);
	else if (strcmp(cmd_buf, "resume") == 0)
		remote_debug_set_paused(0);
	else if (strcmp(cmd_buf, "step") == 0)
		remote_debug_step();
	else if (strcmp(cmd_buf, "stepover") == 0)
		remote_debug_step_over();
	else if (strcmp(cmd_buf, "stepout") == 0)
		remote_debug_step_out();
	else if (strcmp(cmd_buf, "exit") == 0)
		remote_debug_quit();
	else {
		respond_error(resp, 400, "cmd must be pause|resume|step|stepover|stepout|exit");
		return;
	}
	respond_ok(resp);
}

static void handle_api_debugger_messages(const http_request_t *req, http_response_t *resp)
{
	char *body = NULL;
	int len = 0;
	(void)req;
	remote_debug_get_messages(&body, &len);
	respond_owned(resp, body, len, "application/json");
}

static void handle_api_debugger_callstack(const http_request_t *req, http_response_t *resp)
{
	char *body = NULL;
	int len = 0;
	(void)req;
	remote_debug_get_callstack(&body, &len);
	respond_owned(resp, body, len, "application/json");
}

static void handle_api_debugger_trace(const http_request_t *req, http_response_t *resp)
{
	char cmd_buf[32], addr_buf[32], bank_buf[32];
	char *body = NULL;
	int len = 0;
	get_query_param(req->query, "cmd", cmd_buf, (int)sizeof(cmd_buf));
	if (strcmp(cmd_buf, "add") == 0) {
		int bank;
		get_query_param(req->query, "addr", addr_buf, (int)sizeof(addr_buf));
		get_query_param(req->query, "bank", bank_buf, (int)sizeof(bank_buf));
		if (!addr_buf[0]) {
			respond_error(resp, 400, "missing parameter: addr");
			return;
		}
		bank = bank_buf[0] ? (int)parse_hex(bank_buf) : -1;
		remote_debug_trace_add(parse_hex(addr_buf), bank);
	}
	else if (strcmp(cmd_buf, "clear") == 0) {
		remote_debug_trace_clear();
	}
	else if (cmd_buf[0]) {
		respond_error(resp, 400, "cmd must be add|clear (or omitted)");
		return;
	}
	remote_debug_get_trace(&body, &len);
	respond_owned(resp, body, len, "application/json");
}

static void handle_api_debugger_breakpoints(const http_request_t *req, http_response_t *resp)
{
	char cmd_buf[32], addr_buf[32], bank_buf[32], cond_buf[32], ignore_buf[32];
	get_query_param(req->query, "cmd", cmd_buf, (int)sizeof(cmd_buf));
	if (strcmp(cmd_buf, "add") == 0) {
		int bank;
		UINT32 ignore;
		get_query_param(req->query, "addr", addr_buf, (int)sizeof(addr_buf));
		get_query_param(req->query, "bank", bank_buf, (int)sizeof(bank_buf));
		get_query_param(req->query, "cond", cond_buf, (int)sizeof(cond_buf));
		get_query_param(req->query, "ignore", ignore_buf, (int)sizeof(ignore_buf));
		if (!addr_buf[0]) {
			respond_error(resp, 400, "missing parameter: addr");
			return;
		}
		bank = bank_buf[0] ? (int)parse_hex(bank_buf) : -1;
		ignore = ignore_buf[0] ? (UINT32)parse_int(ignore_buf) : 0;
		if (remote_debug_breakpoint_add_ex(parse_hex(addr_buf), bank, cond_buf, ignore) != 0) {
			respond_error(resp, 400, "bad condition or breakpoint table full");
			return;
		}
	}
	else if (strcmp(cmd_buf, "clear") == 0) {
		remote_debug_breakpoint_clear();
	}
	else {
		respond_error(resp, 400, "cmd must be add|clear");
		return;
	}
	respond_ok(resp);
}

static void handle_api_debugger_watchpoints(const http_request_t *req, http_response_t *resp)
{
	char cmd_buf[32], addr_buf[32], len_buf[32], mode_buf[32], bank_buf[32];
	char cond_buf[32], val_buf[32];
	get_query_param(req->query, "cmd", cmd_buf, (int)sizeof(cmd_buf));
	if (strcmp(cmd_buf, "add") == 0) {
		int len, mode, bank, cond_op = REMOTE_DEBUG_COND_NONE;
		UINT8 cond_val = 0;
		get_query_param(req->query, "addr", addr_buf, (int)sizeof(addr_buf));
		get_query_param(req->query, "len", len_buf, (int)sizeof(len_buf));
		get_query_param(req->query, "mode", mode_buf, (int)sizeof(mode_buf));
		get_query_param(req->query, "bank", bank_buf, (int)sizeof(bank_buf));
		get_query_param(req->query, "cond", cond_buf, (int)sizeof(cond_buf));
		get_query_param(req->query, "val", val_buf, (int)sizeof(val_buf));
		if (!addr_buf[0]) {
			respond_error(resp, 400, "missing parameter: addr");
			return;
		}
		len = len_buf[0] ? parse_int(len_buf) : 1;
		mode = mode_buf[0] ? parse_int(mode_buf) : REMOTE_DEBUG_WP_RW;
		bank = bank_buf[0] ? (int)parse_hex(bank_buf) : -1;
		if (mode < REMOTE_DEBUG_WP_READ || mode > REMOTE_DEBUG_WP_RW) {
			respond_error(resp, 400, "mode must be 1 (read), 2 (write) or 3 (rw)");
			return;
		}
		if (cond_buf[0]) {
			if (strcmp(cond_buf, "eq") == 0) cond_op = REMOTE_DEBUG_COND_EQ;
			else if (strcmp(cond_buf, "ne") == 0) cond_op = REMOTE_DEBUG_COND_NE;
			else if (strcmp(cond_buf, "lt") == 0) cond_op = REMOTE_DEBUG_COND_LT;
			else if (strcmp(cond_buf, "gt") == 0) cond_op = REMOTE_DEBUG_COND_GT;
			else if (strcmp(cond_buf, "le") == 0) cond_op = REMOTE_DEBUG_COND_LE;
			else if (strcmp(cond_buf, "ge") == 0) cond_op = REMOTE_DEBUG_COND_GE;
			else {
				respond_error(resp, 400, "cond must be eq|ne|lt|gt|le|ge (with val=HEX)");
				return;
			}
			cond_val = (UINT8)parse_hex(val_buf);
		}
		remote_debug_watchpoint_add(parse_hex(addr_buf), len, mode, bank, cond_op, cond_val);
	}
	else if (strcmp(cmd_buf, "clear") == 0) {
		remote_debug_watchpoint_clear();
	}
	else {
		respond_error(resp, 400, "cmd must be add|clear");
		return;
	}
	respond_ok(resp);
}

static void handle_api_debugger_points(const http_request_t *req, http_response_t *resp)
{
	char cmd_buf[32], type_buf[32], idx_buf[32];
	char *body = NULL;
	int len = 0;
	get_query_param(req->query, "cmd", cmd_buf, (int)sizeof(cmd_buf));
	if (cmd_buf[0]) {
		int idx;
		get_query_param(req->query, "type", type_buf, (int)sizeof(type_buf));
		get_query_param(req->query, "idx", idx_buf, (int)sizeof(idx_buf));
		idx = parse_int(idx_buf);
		if (strcmp(type_buf, "bp") == 0) {
			if (strcmp(cmd_buf, "toggle") == 0)
				remote_debug_breakpoint_toggle(idx);
			else if (strcmp(cmd_buf, "delete") == 0)
				remote_debug_breakpoint_delete(idx);
		}
		else {
			if (strcmp(cmd_buf, "toggle") == 0)
				remote_debug_watchpoint_toggle(idx);
			else if (strcmp(cmd_buf, "delete") == 0)
				remote_debug_watchpoint_delete(idx);
		}
	}
	remote_debug_get_points(&body, &len);
	respond_owned(resp, body, len, "application/json");
}

static void handle_api_debugger_memory_find(const http_request_t *req, http_response_t *resp)
{
	char addr_buf[32], size_buf[32], pat_buf[256], cpu_buf[32], bank_buf[32];
	UINT8 pattern[128];
	UINT32 addr, found = 0;
	int size, cpu, pat_len, bank;

	get_query_param(req->query, "addr", addr_buf, (int)sizeof(addr_buf));
	get_query_param(req->query, "size", size_buf, (int)sizeof(size_buf));
	get_query_param(req->query, "pattern", pat_buf, (int)sizeof(pat_buf));
	get_query_param(req->query, "cpu", cpu_buf, (int)sizeof(cpu_buf));
	get_query_param(req->query, "bank", bank_buf, (int)sizeof(bank_buf));
	addr = parse_hex(addr_buf);
	size = size_buf[0] ? parse_int(size_buf) : 0x2000;
	cpu = cpu_buf[0] ? parse_int(cpu_buf) : 0;
	bank = bank_buf[0] ? (int)parse_hex(bank_buf) : -1;
	pat_len = parse_hex_bytes(pat_buf, pattern, (int)sizeof(pattern));
	if (pat_len == 0) {
		respond_error(resp, 400, "missing or malformed parameter: pattern");
		return;
	}
	if (remote_debug_memory_find(cpu, addr, size, pattern, pat_len, &found, bank)) {
		char res[64];
		snprintf(res, sizeof(res), "{\"status\": \"ok\", \"found\": %u}", found);
		respond_json(resp, 200, res);
	}
	else {
		respond_json(resp, 200, "{\"status\": \"not_found\"}");
	}
}

static void handle_api_debugger_memory_fill(const http_request_t *req, http_response_t *resp)
{
	char addr_buf[32], size_buf[32], val_buf[32], cpu_buf[32];
	get_query_param(req->query, "addr", addr_buf, (int)sizeof(addr_buf));
	get_query_param(req->query, "size", size_buf, (int)sizeof(size_buf));
	get_query_param(req->query, "val", val_buf, (int)sizeof(val_buf));
	get_query_param(req->query, "cpu", cpu_buf, (int)sizeof(cpu_buf));
	if (!addr_buf[0] || !size_buf[0] || !val_buf[0]) {
		respond_error(resp, 400, "missing parameters: addr, size, val");
		return;
	}
	remote_debug_memory_fill(cpu_buf[0] ? parse_int(cpu_buf) : 0,
	                         parse_hex(addr_buf), parse_int(size_buf),
	                         (UINT8)parse_hex(val_buf));
	respond_ok(resp);
}

/* Disassemble one instruction; returns its size (at least 1). */
static unsigned dasm_one(char *buf, size_t buflen, UINT32 addr)
{
	unsigned size;
	(void)buflen;
	activecpu_set_op_base(addr);
	size = activecpu_dasm(buf, addr);
	return size ? size : 1;
}

/* Find a start address <= target so that forward disassembly from it hits
 * target exactly, yielding up to want_lines instructions before target.
 * Best-effort heuristic (6809 instructions are 1-5 bytes long). */
static UINT32 dasm_backtrack(UINT32 target, int want_lines)
{
	UINT32 best_start = target;
	int best_lines = 0;
	int max_back = want_lines * 5 + 4;
	int off;
	char buf[64];

	for (off = 1; off <= max_back; off++) {
		UINT32 a = target - (UINT32)off;
		int count = 0;
		while (a < target)
			a += dasm_one(buf, sizeof(buf), a);
		if (a != target)
			continue;
		a = target - (UINT32)off;
		count = 0;
		while (a < target) {
			a += dasm_one(buf, sizeof(buf), a);
			count++;
		}
		if (count > best_lines && count <= want_lines + 4) {
			best_lines = count;
			best_start = target - (UINT32)off;
			if (best_lines >= want_lines)
				break;
		}
	}
	/* skip surplus instructions so at most want_lines remain */
	while (best_lines > want_lines) {
		best_start += dasm_one(buf, sizeof(buf), best_start);
		best_lines--;
	}
	return best_start;
}

static void handle_api_debugger_dasm(const http_request_t *req, http_response_t *resp)
{
	char addr_buf[32], lines_buf[32], before_buf[32], cpu_buf[32], bank_buf[32];
	UINT32 addr, start;
	int lines, before, cpu_idx, bank;
	char *buf;
	char *ptr;

	get_query_param(req->query, "addr", addr_buf, (int)sizeof(addr_buf));
	get_query_param(req->query, "lines", lines_buf, (int)sizeof(lines_buf));
	get_query_param(req->query, "before", before_buf, (int)sizeof(before_buf));
	get_query_param(req->query, "cpu", cpu_buf, (int)sizeof(cpu_buf));
	get_query_param(req->query, "bank", bank_buf, (int)sizeof(bank_buf));
	addr = parse_hex(addr_buf);
	lines = lines_buf[0] ? parse_int(lines_buf) : 10;
	before = before_buf[0] ? parse_int(before_buf) : 0;
	cpu_idx = cpu_buf[0] ? parse_int(cpu_buf) : 0;
	bank = bank_buf[0] ? (int)parse_hex(bank_buf) : -1;
	if (lines < 1)
		lines = 1;
	if (lines > 100)
		lines = 100;
	if (before < 0)
		before = 0;
	if (before > 50)
		before = 50;

	buf = malloc((size_t)(lines + before) * 256 + 256);
	if (!buf) {
		respond_error(resp, 500, "out of memory");
		return;
	}
	ptr = buf;
	ptr += sprintf(ptr, "{\"cpu\": %d, \"bank\": %d, \"lines\": [", cpu_idx, bank);
	remote_debug_lock();
	if (Machine && cpu_idx >= 0 && cpu_idx < cpu_gettotalcpu()) {
		int old_bank = -1;
		int i, total;
		cpuintrf_push_context(cpu_idx);
		if (bank != -1 && Machine->drv->cpu[cpu_idx].cpu_type == CPU_M6809) {
			old_bank = wpc_get_bank();
			cpu_setbank(1, memory_region(WPC_ROMREGION) + bank * 0x4000);
		}
		start = addr;
		total = lines;
		if (before > 0) {
			char tmp[64];
			UINT32 a;
			start = dasm_backtrack(addr, before);
			/* count how many instructions the backtrack actually found */
			for (a = start; a < addr; total++)
				a += dasm_one(tmp, sizeof(tmp), a);
		}
		for (i = 0; i < total; i++) {
			char dasm_buf[64];
			char esc[160];
			unsigned size = dasm_one(dasm_buf, sizeof(dasm_buf), start);
			ptr += sprintf(ptr, "%s{\"addr\": %u, \"size\": %u, \"text\": \"%s\"}",
			               (i > 0) ? "," : "", start, size,
			               remote_debug_json_escape(esc, (int)sizeof(esc), dasm_buf));
			start += size;
		}
		if (old_bank != -1)
			cpu_setbank(1, memory_region(WPC_ROMREGION) + old_bank * 0x4000);
		cpuintrf_pop_context();
	}
	remote_debug_unlock();
	ptr += sprintf(ptr, "]}");
	resp->body = buf;
	resp->len = (int)(ptr - buf);
	resp->status = 200;
	strcpy(resp->content_type, "application/json");
}

static void handle_api_debugger_nvram_dump(const http_request_t *req, http_response_t *resp)
{
	(void)req;
	remote_debug_lock();
	if (Machine && Machine->drv && Machine->drv->nvram_handler && wpc_ram) {
		char *body = malloc(0x2000);
		if (body) {
			memcpy(body, wpc_ram, 0x2000);
			remote_debug_unlock();
			respond_owned(resp, body, 0x2000, "application/octet-stream");
			return;
		}
		remote_debug_unlock();
		respond_error(resp, 500, "out of memory");
		return;
	}
	remote_debug_unlock();
	respond_error(resp, 404, "no WPC NVRAM available");
}

static void handle_api_debugger_nvram(const http_request_t *req, http_response_t *resp)
{
	char cmd_buf[32];
	get_query_param(req->query, "cmd", cmd_buf, (int)sizeof(cmd_buf));
	if (strcmp(cmd_buf, "clear") != 0) {
		respond_error(resp, 400, "cmd must be clear");
		return;
	}
	remote_debug_lock();
	if (Machine && Machine->drv && Machine->drv->nvram_handler) {
		Machine->drv->nvram_handler(NULL, 0);
		remote_debug_unlock();
		respond_ok(resp);
	}
	else {
		remote_debug_unlock();
		respond_error(resp, 404, "no NVRAM handler");
	}
}

static void handle_api_debugger_memory_write(const http_request_t *req, http_response_t *resp)
{
	char addr_buf[32], val_buf[32], data_buf[512], cpu_buf[32];
	UINT32 addr;
	int cpu_idx;

	get_query_param(req->query, "addr", addr_buf, (int)sizeof(addr_buf));
	get_query_param(req->query, "val", val_buf, (int)sizeof(val_buf));
	get_query_param(req->query, "data", data_buf, (int)sizeof(data_buf));
	get_query_param(req->query, "cpu", cpu_buf, (int)sizeof(cpu_buf));
	if (!addr_buf[0] || (!val_buf[0] && !data_buf[0])) {
		respond_error(resp, 400, "missing parameters: addr and val or data");
		return;
	}
	addr = parse_hex(addr_buf);
	cpu_idx = cpu_buf[0] ? parse_int(cpu_buf) : 0;
	if (data_buf[0]) {
		UINT8 bytes[256];
		int n = parse_hex_bytes(data_buf, bytes, (int)sizeof(bytes));
		if (n == 0) {
			respond_error(resp, 400, "malformed parameter: data");
			return;
		}
		if (remote_debug_memory_write_block(cpu_idx, addr, bytes, n) != 0) {
			respond_error(resp, 400, "bad cpu index or machine not running");
			return;
		}
	}
	else {
		UINT8 val = (UINT8)parse_hex(val_buf);
		if (remote_debug_memory_write_block(cpu_idx, addr, &val, 1) != 0) {
			respond_error(resp, 400, "bad cpu index or machine not running");
			return;
		}
	}
	respond_ok(resp);
}

static void handle_api_debugger_state_write(const http_request_t *req, http_response_t *resp)
{
	char cpu_buf[32], reg_buf[32], val_buf[32];
	int cpu_idx, reg_id;

	get_query_param(req->query, "reg", reg_buf, (int)sizeof(reg_buf));
	get_query_param(req->query, "val", val_buf, (int)sizeof(val_buf));
	get_query_param(req->query, "cpu", cpu_buf, (int)sizeof(cpu_buf));
	if (!reg_buf[0] || !val_buf[0]) {
		respond_error(resp, 400, "missing parameters: reg, val");
		return;
	}
	cpu_idx = cpu_buf[0] ? parse_int(cpu_buf) : 0;
	reg_id = resolve_register_id(cpu_idx, reg_buf);
	if (reg_id < 0) {
		respond_error(resp, 400, "invalid register name or id");
		return;
	}
	remote_debug_set_register(cpu_idx, reg_id, parse_hex(val_buf));
	respond_ok(resp);
}

/* Append the register set of CPU i (context already pushed). */
static void append_cpu_registers(int i, char **p)
{
	int type = Machine->drv->cpu[i].cpu_type;
	*p += sprintf(*p, "\"type\": %d, \"pc\": %u, \"sp\": %u",
	              type, cpunum_get_reg(i, REG_PC), cpunum_get_reg(i, REG_SP));
	if (type == CPU_M6809) {
		*p += sprintf(*p,
			", \"a\": %u, \"b\": %u, \"x\": %u, \"y\": %u, \"u\": %u, \"dp\": %u, \"cc\": %u",
			cpunum_get_reg(i, M6809_A), cpunum_get_reg(i, M6809_B),
			cpunum_get_reg(i, M6809_X), cpunum_get_reg(i, M6809_Y),
			cpunum_get_reg(i, M6809_U), cpunum_get_reg(i, M6809_DP),
			cpunum_get_reg(i, M6809_CC));
	}
	else if (type == CPU_ADSP2105) {
		*p += sprintf(*p,
			", \"ax0\": %u, \"ax1\": %u, \"ay0\": %u, \"ay1\": %u, \"ar\": %u, \"cntr\": %u, \"astat\": %u",
			cpunum_get_reg(i, ADSP2100_AX0), cpunum_get_reg(i, ADSP2100_AX1),
			cpunum_get_reg(i, ADSP2100_AY0), cpunum_get_reg(i, ADSP2100_AY1),
			cpunum_get_reg(i, ADSP2100_AR), cpunum_get_reg(i, ADSP2100_CNTR),
			cpunum_get_reg(i, ADSP2100_ASTAT));
	}
}

static void handle_api_debugger_state(const http_request_t *req, http_response_t *resp)
{
	char *buf;
	char *ptr;
	(void)req;
	buf = malloc(8192);
	if (!buf) {
		respond_error(resp, 500, "out of memory");
		return;
	}
	ptr = buf;
	ptr += sprintf(ptr, "{\"cpus\": [");
	remote_debug_lock();
	if (Machine) {
		int i;
		for (i = 0; i < cpu_gettotalcpu(); i++) {
			if (i > 0)
				ptr += sprintf(ptr, ",");
			ptr += sprintf(ptr, "{\"id\": %d, ", i);
			cpuintrf_push_context(i);
			append_cpu_registers(i, &ptr);
			cpuintrf_pop_context();
			ptr += sprintf(ptr, "}");
		}
	}
	remote_debug_unlock();
	ptr += sprintf(ptr, "]}");
	resp->body = buf;
	resp->len = (int)(ptr - buf);
	resp->status = 200;
	strcpy(resp->content_type, "application/json");
}

static void handle_api_debugger_memory(const http_request_t *req, http_response_t *resp)
{
	char addr_buf[32], size_buf[32], cpu_buf[32], bank_buf[32];
	UINT32 addr;
	int size, cpu_idx, bank;
	char *buf;
	char *ptr;

	get_query_param(req->query, "addr", addr_buf, (int)sizeof(addr_buf));
	get_query_param(req->query, "size", size_buf, (int)sizeof(size_buf));
	get_query_param(req->query, "cpu", cpu_buf, (int)sizeof(cpu_buf));
	get_query_param(req->query, "bank", bank_buf, (int)sizeof(bank_buf));
	addr = parse_hex(addr_buf);
	size = size_buf[0] ? parse_int(size_buf) : 16;
	cpu_idx = cpu_buf[0] ? parse_int(cpu_buf) : 0;
	bank = bank_buf[0] ? (int)parse_hex(bank_buf) : -1;
	if (size < 1)
		size = 1;
	if (size > 2048)
		size = 2048;

	buf = malloc((size_t)size * 4 + 256);
	if (!buf) {
		respond_error(resp, 500, "out of memory");
		return;
	}
	ptr = buf;
	ptr += sprintf(ptr, "{\"addr\": %u, \"cpu\": %d, \"bank\": %d, \"data\": [", addr, cpu_idx, bank);
	remote_debug_lock();
	if (Machine && cpu_idx >= 0 && cpu_idx < cpu_gettotalcpu()) {
		int i;
		for (i = 0; i < size; i++) {
			ptr += sprintf(ptr, "%s%u", (i > 0) ? "," : "",
			               (unsigned)remote_debug_read_byte(cpu_idx, addr + (UINT32)i, bank));
		}
	}
	remote_debug_unlock();
	ptr += sprintf(ptr, "]}");
	resp->body = buf;
	resp->len = (int)(ptr - buf);
	resp->status = 200;
	strcpy(resp->content_type, "application/json");
}

static void handle_api_input(const http_request_t *req, http_response_t *resp)
{
	char sw_buf[32], val_buf[32], pulse_buf[32];
	get_query_param(req->query, "sw", sw_buf, (int)sizeof(sw_buf));
	get_query_param(req->query, "val", val_buf, (int)sizeof(val_buf));
	get_query_param(req->query, "pulse", pulse_buf, (int)sizeof(pulse_buf));
	if (!sw_buf[0] || !val_buf[0]) {
		respond_error(resp, 400, "missing parameters: sw, val");
		return;
	}
	if (remote_debug_set_switch(parse_int(sw_buf), parse_int(val_buf),
	                            pulse_buf[0] ? parse_int(pulse_buf) : 0) == 0)
		respond_ok(resp);
	else
		respond_error(resp, 503, "core not initialized");
}

static void handle_api_switches(const http_request_t *req, http_response_t *resp)
{
	char *body = NULL;
	int len = 0;
	(void)req;
	remote_debug_get_switches(&body, &len);
	respond_owned(resp, body, len, "application/json");
}

static void handle_api_lamps(const http_request_t *req, http_response_t *resp)
{
	char *body = NULL;
	int len = 0;
	(void)req;
	remote_debug_get_lamps(&body, &len);
	respond_owned(resp, body, len, "application/json");
}

static void handle_api_solenoids(const http_request_t *req, http_response_t *resp)
{
	char *body = NULL;
	int len = 0;
	(void)req;
	remote_debug_get_solenoids(&body, &len);
	respond_owned(resp, body, len, "application/json");
}

/* Parse a monitor object type name; -1 if unknown. */
static int parse_obj_type(const char *s)
{
	if (strcmp(s, "sw") == 0 || strcmp(s, "switch") == 0) return REMOTE_DEBUG_OBJ_SWITCH;
	if (strcmp(s, "lamp") == 0)                            return REMOTE_DEBUG_OBJ_LAMP;
	if (strcmp(s, "sol") == 0 || strcmp(s, "solenoid") == 0) return REMOTE_DEBUG_OBJ_SOL;
	return -1;
}

static void handle_api_monitor(const http_request_t *req, http_response_t *resp)
{
	char cmd_buf[32], type_buf[32], id_buf[32];
	char *body = NULL;
	int len = 0;
	get_query_param(req->query, "cmd", cmd_buf, (int)sizeof(cmd_buf));
	if (strcmp(cmd_buf, "add") == 0) {
		int type;
		char brk_buf[32];
		get_query_param(req->query, "type", type_buf, (int)sizeof(type_buf));
		get_query_param(req->query, "id", id_buf, (int)sizeof(id_buf));
		get_query_param(req->query, "break", brk_buf, (int)sizeof(brk_buf));
		type = parse_obj_type(type_buf);
		if (type < 0 || !id_buf[0]) {
			respond_error(resp, 400, "need type=sw|lamp|sol and id");
			return;
		}
		if (remote_debug_monitor_add(type, parse_int(id_buf),
		                             (brk_buf[0] && brk_buf[0] != '0')) != 0) {
			respond_error(resp, 400, "monitor table full");
			return;
		}
	}
	else if (strcmp(cmd_buf, "clear") == 0) {
		remote_debug_monitor_clear();
	}
	else if (cmd_buf[0]) {
		respond_error(resp, 400, "cmd must be add|clear (or omitted)");
		return;
	}
	remote_debug_get_monitors(&body, &len);
	respond_owned(resp, body, len, "application/json");
}

static void handle_api_monitor_log(const http_request_t *req, http_response_t *resp)
{
	char cmd_buf[32];
	char *body = NULL;
	int len = 0;
	get_query_param(req->query, "cmd", cmd_buf, (int)sizeof(cmd_buf));
	if (strcmp(cmd_buf, "clear") == 0)
		remote_debug_action_log_clear();
	remote_debug_get_action_log(&body, &len);
	respond_owned(resp, body, len, "application/json");
}

static void handle_api_debugger_instrument(const http_request_t *req, http_response_t *resp)
{
	char cmd_buf[32], addr_buf[32], bank_buf[32];
	char *body = NULL;
	int len = 0;
	get_query_param(req->query, "cmd", cmd_buf, (int)sizeof(cmd_buf));
	if (strcmp(cmd_buf, "add") == 0) {
		int bank;
		get_query_param(req->query, "addr", addr_buf, (int)sizeof(addr_buf));
		get_query_param(req->query, "bank", bank_buf, (int)sizeof(bank_buf));
		if (!addr_buf[0]) {
			respond_error(resp, 400, "missing parameter: addr");
			return;
		}
		bank = bank_buf[0] ? (int)parse_hex(bank_buf) : -1;
		if (remote_debug_instrument_add(parse_hex(addr_buf), bank) != 0) {
			respond_error(resp, 400, "instrumentation table full");
			return;
		}
	}
	else if (strcmp(cmd_buf, "clear") == 0) {
		remote_debug_instrument_clear();
	}
	else if (cmd_buf[0]) {
		respond_error(resp, 400, "cmd must be add|clear (or omitted)");
		return;
	}
	remote_debug_get_instrumentation(&body, &len);
	respond_owned(resp, body, len, "application/json");
}

static void handle_api_debugger_scan(const http_request_t *req, http_response_t *resp)
{
	char cmd_buf[32], addr_buf[32], size_buf[32], cpu_buf[32], op_buf[32], val_buf[32];
	char *body = NULL;
	int len = 0;
	get_query_param(req->query, "cmd", cmd_buf, (int)sizeof(cmd_buf));
	if (strcmp(cmd_buf, "new") == 0) {
		int cpu, size;
		get_query_param(req->query, "addr", addr_buf, (int)sizeof(addr_buf));
		get_query_param(req->query, "size", size_buf, (int)sizeof(size_buf));
		get_query_param(req->query, "cpu", cpu_buf, (int)sizeof(cpu_buf));
		cpu = cpu_buf[0] ? parse_int(cpu_buf) : 0;
		size = size_buf[0] ? parse_int(size_buf) : 0x2000;
		if (remote_debug_scan_new(cpu, parse_hex(addr_buf), size) < 0) {
			respond_error(resp, 400, "bad scan region (size 1..8192, valid cpu)");
			return;
		}
	}
	else if (strcmp(cmd_buf, "filter") == 0) {
		static const struct { const char *name; int op; } ops[] = {
			{"eq", REMOTE_DEBUG_SCAN_EQ}, {"ne", REMOTE_DEBUG_SCAN_NE},
			{"changed", REMOTE_DEBUG_SCAN_CHANGED}, {"unchanged", REMOTE_DEBUG_SCAN_UNCHANGED},
			{"inc", REMOTE_DEBUG_SCAN_INC}, {"dec", REMOTE_DEBUG_SCAN_DEC}
		};
		int op = -1;
		size_t i;
		get_query_param(req->query, "op", op_buf, (int)sizeof(op_buf));
		get_query_param(req->query, "val", val_buf, (int)sizeof(val_buf));
		for (i = 0; i < sizeof(ops) / sizeof(ops[0]); i++)
			if (strcmp(op_buf, ops[i].name) == 0)
				op = ops[i].op;
		if (op < 0) {
			respond_error(resp, 400, "op must be eq|ne|changed|unchanged|inc|dec");
			return;
		}
		if (remote_debug_scan_filter(op, (UINT8)parse_hex(val_buf)) < 0) {
			respond_error(resp, 400, "no active scan; run cmd=new first");
			return;
		}
	}
	else if (cmd_buf[0]) {
		respond_error(resp, 400, "cmd must be new|filter (or omitted)");
		return;
	}
	remote_debug_get_scan(&body, &len);
	respond_owned(resp, body, len, "application/json");
}

static void handle_api_debugger_savestate(const http_request_t *req, http_response_t *resp)
{
	char cmd_buf[32], slot_buf[32];
	char *body = NULL;
	int len = 0;
	get_query_param(req->query, "cmd", cmd_buf, (int)sizeof(cmd_buf));
	get_query_param(req->query, "slot", slot_buf, (int)sizeof(slot_buf));
	if (strcmp(cmd_buf, "save") == 0) {
		if (!slot_buf[0]) { respond_error(resp, 400, "missing parameter: slot"); return; }
		if (remote_debug_savestate_save(slot_buf) != 0) {
			respond_error(resp, 500, "cannot save (no RAM or all slots full)");
			return;
		}
		respond_ok(resp);
		return;
	}
	if (strcmp(cmd_buf, "load") == 0) {
		if (!slot_buf[0]) { respond_error(resp, 400, "missing parameter: slot"); return; }
		if (remote_debug_savestate_load(slot_buf) != 0) {
			respond_error(resp, 404, "unknown slot");
			return;
		}
		respond_ok(resp);
		return;
	}
	if (strcmp(cmd_buf, "delete") == 0) {
		remote_debug_savestate_delete(slot_buf);
		respond_ok(resp);
		return;
	}
	if (cmd_buf[0]) {
		respond_error(resp, 400, "cmd must be save|load|delete (or omitted to list)");
		return;
	}
	remote_debug_get_savestates(&body, &len);
	respond_owned(resp, body, len, "application/json");
}

static void handle_api_debugger_dmdrec(const http_request_t *req, http_response_t *resp)
{
	char cmd_buf[32];
	char *body = NULL;
	int len = 0;
	get_query_param(req->query, "cmd", cmd_buf, (int)sizeof(cmd_buf));
	if (strcmp(cmd_buf, "start") == 0)
		remote_debug_dmdrec_start();
	else if (strcmp(cmd_buf, "stop") == 0)
		remote_debug_dmdrec_stop();
	else if (strcmp(cmd_buf, "clear") == 0)
		remote_debug_dmdrec_clear();
	else if (cmd_buf[0]) {
		respond_error(resp, 400, "cmd must be start|stop|clear (or omitted)");
		return;
	}
	remote_debug_get_dmdrec_info(&body, &len);
	respond_owned(resp, body, len, "application/json");
}

static void handle_api_debugger_dmdrec_data(const http_request_t *req, http_response_t *resp)
{
	char *body = NULL;
	int len = 0;
	(void)req;
	remote_debug_get_dmdrec_data(&body, &len);
	if (body)
		respond_owned(resp, body, len, "application/octet-stream");
	else
		respond_error(resp, 404, "no frames recorded");
}

static void handle_api_debugger_exectrace(const http_request_t *req, http_response_t *resp)
{
	char cmd_buf[32];
	char *body = NULL;
	int len = 0;
	get_query_param(req->query, "cmd", cmd_buf, (int)sizeof(cmd_buf));
	if (strcmp(cmd_buf, "start") == 0)
		remote_debug_exectrace_start();
	else if (strcmp(cmd_buf, "stop") == 0)
		remote_debug_exectrace_stop();
	else if (strcmp(cmd_buf, "clear") == 0)
		remote_debug_exectrace_clear();
	else if (cmd_buf[0]) {
		respond_error(resp, 400, "cmd must be start|stop|clear (or omitted)");
		return;
	}
	remote_debug_get_exectrace(&body, &len);
	respond_owned(resp, body, len, "application/json");
}

static void handle_api_debugger_coverage(const http_request_t *req, http_response_t *resp)
{
	char cmd_buf[32], addr_buf[32], size_buf[32], bank_buf[32];
	char *body = NULL;
	int len = 0;
	get_query_param(req->query, "cmd", cmd_buf, (int)sizeof(cmd_buf));
	if (strcmp(cmd_buf, "start") == 0)
		remote_debug_coverage_start();
	else if (strcmp(cmd_buf, "stop") == 0)
		remote_debug_coverage_stop();
	else if (strcmp(cmd_buf, "clear") == 0)
		remote_debug_coverage_clear();
	else if (cmd_buf[0]) {
		respond_error(resp, 400, "cmd must be start|stop|clear (or omitted)");
		return;
	}
	/* a region query returns the per-address executed flags */
	get_query_param(req->query, "addr", addr_buf, (int)sizeof(addr_buf));
	if (addr_buf[0]) {
		int size, bank;
		get_query_param(req->query, "size", size_buf, (int)sizeof(size_buf));
		get_query_param(req->query, "bank", bank_buf, (int)sizeof(bank_buf));
		size = size_buf[0] ? parse_int(size_buf) : 256;
		if (size < 1) size = 1;
		if (size > 4096) size = 4096;
		bank = bank_buf[0] ? (int)parse_hex(bank_buf) : -1;
		remote_debug_get_coverage_region(&body, &len, parse_hex(addr_buf), size, bank);
	}
	else {
		remote_debug_get_coverage_info(&body, &len);
	}
	respond_owned(resp, body, len, "application/json");
}

static void handle_api_debugger_tracepoints(const http_request_t *req, http_response_t *resp)
{
	char cmd_buf[32], addr_buf[32], bank_buf[32];
	char *body = NULL;
	int len = 0;
	get_query_param(req->query, "cmd", cmd_buf, (int)sizeof(cmd_buf));
	if (strcmp(cmd_buf, "add") == 0) {
		int bank;
		get_query_param(req->query, "addr", addr_buf, (int)sizeof(addr_buf));
		get_query_param(req->query, "bank", bank_buf, (int)sizeof(bank_buf));
		if (!addr_buf[0]) {
			respond_error(resp, 400, "missing parameter: addr");
			return;
		}
		bank = bank_buf[0] ? (int)parse_hex(bank_buf) : -1;
		if (remote_debug_tracepoint_add(parse_hex(addr_buf), bank) != 0) {
			respond_error(resp, 400, "tracepoint table full");
			return;
		}
	}
	else if (strcmp(cmd_buf, "clear") == 0) {
		remote_debug_tracepoint_clear();
	}
	else if (cmd_buf[0]) {
		respond_error(resp, 400, "cmd must be add|clear (or omitted)");
		return;
	}
	remote_debug_get_tracepoints(&body, &len);
	respond_owned(resp, body, len, "application/json");
}

static void handle_api_debugger_savestate_diff(const http_request_t *req, http_response_t *resp)
{
	char a_buf[32], b_buf[32];
	char *body = NULL;
	int len = 0;
	get_query_param(req->query, "a", a_buf, (int)sizeof(a_buf));
	get_query_param(req->query, "b", b_buf, (int)sizeof(b_buf));
	if (!a_buf[0]) {
		respond_error(resp, 400, "missing parameter: a (slot name)");
		return;
	}
	remote_debug_savestate_diff(&body, &len, a_buf, b_buf);
	respond_owned(resp, body, len, "application/json");
}

static void handle_ui(const http_request_t *req, http_response_t *resp)
{
/* ui_html.h is generated from ui.html by the makefile; it contains the
 * HTML content as a C array plus its length. */
#include "remote_debug/ui_html.h"
	(void)req;
	resp->body = malloc(src_remote_debug_ui_html_len);
	if (!resp->body) {
		respond_error(resp, 500, "out of memory");
		return;
	}
	memcpy(resp->body, src_remote_debug_ui_html, src_remote_debug_ui_html_len);
	resp->len = (int)src_remote_debug_ui_html_len;
	resp->status = 200;
	strcpy(resp->content_type, "text/html");
}

static void handle_api_doc(const http_request_t *req, http_response_t *resp);

/* ------------------------------------------------------------------ */
/* Routing                                                            */
/* ------------------------------------------------------------------ */

static const api_route_t api_routes[] = {
	{"/api/info", handle_api_info,
	 "game info, pause state, lamp/switch/segment matrices, solenoids"},
	{"/api/events", handle_api_events,
	 "Server-Sent-Events stream of debugger events (halt, resume, step, message)"},
	{"/api/dmd/info", handle_api_dmd_info, "DMD dimensions"},
	{"/api/dmd/raw", handle_api_dmd_raw, "DMD frame, 1 luminance byte per pixel"},
	{"/api/dmd/pnm", handle_api_dmd_pnm, "DMD frame as PGM (P5) image"},
	{"/api/screenshot/info", handle_api_screenshot_info, "screen dimensions"},
	{"/api/screenshot/raw", handle_api_screenshot_raw, "screen as raw RGB24"},
	{"/api/screenshot/pnm", handle_api_screenshot_pnm, "screen as PPM (P6) image"},
	{"/api/screenshot", handle_api_screenshot_pnm, "alias of /api/screenshot/pnm"},
	{"/api/debugger/control", handle_api_debugger_control,
	 "?cmd=pause|resume|step|stepover|stepout|exit"},
	{"/api/debugger/control/runto", handle_api_debugger_control_runto,
	 "?addr=HEX[&bank=HEX] - resume until addr is reached (bank: 0x4000-0x7FFF only)"},
	{"/api/debugger/state", handle_api_debugger_state, "registers of all CPUs"},
	{"/api/debugger/state/write", handle_api_debugger_state_write,
	 "?reg=NAME|ID&val=HEX[&cpu=N] - set a register (M6809/ADSP names)"},
	{"/api/debugger/dasm", handle_api_debugger_dasm,
	 "?addr=HEX[&lines=N][&before=N][&cpu=N][&bank=HEX] - disassemble"},
	{"/api/debugger/callstack", handle_api_debugger_callstack,
	 "callstack with register context"},
	{"/api/debugger/messages", handle_api_debugger_messages, "message log"},
	{"/api/debugger/command", handle_api_debugger_command,
	 "?cmd=... - classic command (BP/BC/WP/WC/G/S/F/QUIT/HELP)"},
	{"/api/debugger/breakpoints", handle_api_debugger_breakpoints,
	 "?cmd=add&addr=HEX[&bank=HEX][&cond=REG==HEX][&ignore=N] | ?cmd=clear"},
	{"/api/debugger/watchpoints", handle_api_debugger_watchpoints,
	 "?cmd=add&addr=HEX[&len=N][&mode=1|2|3][&bank=HEX][&cond=eq|ne|lt|gt|le|ge&val=HEX] | ?cmd=clear"},
	{"/api/debugger/points", handle_api_debugger_points,
	 "list all points; ?cmd=toggle|delete&type=bp|wp&idx=N to modify"},
	{"/api/debugger/memory", handle_api_debugger_memory,
	 "?addr=HEX[&size=N][&cpu=N][&bank=HEX] - read memory (bank: 0x4000-0x7FFF ROM)"},
	{"/api/debugger/memory/write", handle_api_debugger_memory_write,
	 "?addr=HEX&val=HEX | ?addr=HEX&data=HEXBYTES [&cpu=N] - write memory"},
	{"/api/debugger/memory/fill", handle_api_debugger_memory_fill,
	 "?addr=HEX&size=N&val=HEX[&cpu=N] - fill memory"},
	{"/api/debugger/memory/find", handle_api_debugger_memory_find,
	 "?pattern=HEXBYTES[&addr=HEX][&size=N][&cpu=N][&bank=HEX] - search memory"},
	{"/api/debugger/trace", handle_api_debugger_trace,
	 "memory access trace; ?cmd=add&addr=HEX[&bank=HEX] | ?cmd=clear"},
	{"/api/debugger/nvram", handle_api_debugger_nvram, "?cmd=clear - wipe NVRAM"},
	{"/api/debugger/nvram/dump", handle_api_debugger_nvram_dump,
	 "raw 8KB WPC CMOS RAM dump"},
	{"/api/debugger/instrument", handle_api_debugger_instrument,
	 "PC hit counting; ?cmd=add&addr=HEX[&bank=HEX] | ?cmd=clear | list"},
	{"/api/debugger/exectrace", handle_api_debugger_exectrace,
	 "instruction trace ring; ?cmd=start|stop|clear | list (last N executed)"},
	{"/api/debugger/coverage", handle_api_debugger_coverage,
	 "code coverage; ?cmd=start|stop|clear | summary | ?addr=HEX[&size=N][&bank=HEX] region"},
	{"/api/debugger/tracepoints", handle_api_debugger_tracepoints,
	 "log-and-continue points; ?cmd=add&addr=HEX[&bank=HEX] | ?cmd=clear | list+log"},
	{"/api/debugger/scan", handle_api_debugger_scan,
	 "value scan; ?cmd=new&addr=HEX[&size=N][&cpu=N] | ?cmd=filter&op=eq|ne|changed|unchanged|inc|dec[&val=HEX]"},
	{"/api/switches", handle_api_switches,
	 "all switches with number, matrix position, state and name"},
	{"/api/lamps", handle_api_lamps, "all lamps with number, matrix position, state"},
	{"/api/solenoids", handle_api_solenoids, "all solenoids with number and state"},
	{"/api/monitor", handle_api_monitor,
	 "object monitoring; ?cmd=add&type=sw|lamp|sol&id=N[&break=1] | ?cmd=clear | list"},
	{"/api/monitor/log", handle_api_monitor_log,
	 "recorded state-change action log; ?cmd=clear to reset"},
	{"/api/debugger/savestate", handle_api_debugger_savestate,
	 "checkpoint WPC RAM + main CPU regs; ?cmd=save|load|delete&slot=NAME | list"},
	{"/api/debugger/savestate/diff", handle_api_debugger_savestate_diff,
	 "?a=SLOT[&b=SLOT] - diff two save slots' RAM (b omitted = live RAM)"},
	{"/api/debugger/dmdrec", handle_api_debugger_dmdrec,
	 "DMD frame recorder; ?cmd=start|stop|clear | status"},
	{"/api/debugger/dmdrec/data", handle_api_debugger_dmdrec_data,
	 "packed binary of recorded DMD frames (u32 count,w,h; per frame u32 t + w*h bytes)"},
	{"/api/input", handle_api_input,
	 "?sw=N&val=0|1[&pulse=MS] - set a switch, optionally as a timed pulse"},
	{"/ui", handle_ui, "the web UI"},
	{"/api/doc", handle_api_doc, "this document"},
};

static void handle_api_doc(const http_request_t *req, http_response_t *resp)
{
	char *buf = malloc(8192);
	char *ptr;
	size_t i;
	(void)req;
	if (!buf) {
		respond_error(resp, 500, "out of memory");
		return;
	}
	ptr = buf;
	ptr += sprintf(ptr,
		"PinMAME Remote Debugger API\n"
		"===========================\n"
		"All endpoints use GET. Parameter conventions:\n"
		"  addr, val, bank, pattern, data: hex (optional 0x/$ prefix)\n"
		"  size, lines, before, cpu, sw, idx, len, mode, ignore: decimal\n\n");
	for (i = 0; i < sizeof(api_routes) / sizeof(api_routes[0]); i++) {
		ptr += snprintf(ptr, (size_t)(8192 - (ptr - buf)), "%-32s %s\n",
		                api_routes[i].path, api_routes[i].doc);
		if (ptr - buf > 8192 - 160)
			break;
	}
	resp->body = buf;
	resp->len = (int)(ptr - buf);
	resp->status = 200;
	strcpy(resp->content_type, "text/plain");
}

void api_handler(const http_request_t *req, http_response_t *resp)
{
	size_t i;
	if (!remote_debug_ready) {
		respond_json(resp, 503, "{\"status\": \"not_ready\"}");
		return;
	}
	for (i = 0; i < sizeof(api_routes) / sizeof(api_routes[0]); i++) {
		if (strcmp(req->path, api_routes[i].path) == 0) {
			api_routes[i].handler(req, resp);
			return;
		}
	}
	respond_error(resp, 404, "not found; see /api/doc");
}

#endif /* REMOTE_DEBUG */
