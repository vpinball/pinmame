#include "altsound_csv_parser.hpp"

#include <memory>
#include <string.h>
#include <unistd.h>

#ifdef __cplusplus
  extern "C" {
#endif

#include <dirent.h>

#ifdef __cplusplus
  }
#endif

#define CSV_MAX_LINE_LENGTH 512
#define CSV_SUCCESS 0
#define CSV_ERROR_NO_SUCH_FIELD -1
#define CSV_ERROR_HEADER_NOT_FOUND -2
#define CSV_ERROR_NO_MORE_RECORDS -3
#define CSV_ERROR_FIELD_INDEX_OUT_OF_RANGE -4
#define CSV_ERROR_FILE_NOT_FOUND -5
#define CSV_ERROR_LINE_FORMAT -6

// Support for lookup-based altsound (CSV)
const char* path_table = "\\altsound.csv";

AltsoundCsvParser::AltsoundCsvParser(char *gname_in)
: reader(NULL),
  g_szGameName(gname_in),
  filename(NULL)
//  path_main(path_main_in)
//  cvpmd(cvpmd_in)
{
	char* lpHelp = cvpmd;
	char* lpSlash = NULL;
	size_t BASE_PATH_LEN;
	int result = 0; // Assume success at start

	// Get path to VPinMAME
	HINSTANCE hInst;
#ifndef _WIN64
	hInst = GetModuleHandle("VPinMAME.dll");
#else
	hInst = GetModuleHandle("VPinMAME64.dll");
#endif
	GetModuleFileName(hInst, cvpmd, sizeof(cvpmd));

	// Get rid of "VPinMAME.dll" from path name
	while (*lpHelp) {
		if (*lpHelp == '\\')
			lpSlash = lpHelp;
		lpHelp++;
	}
	if (lpSlash)
		*lpSlash = '\0';

	// Determine base path length
	BASE_PATH_LEN = strlen(cvpmd) + strlen(path_main) + strlen(g_szGameName) + 1;

	size_t PATH_LEN;
	char* PATH;

	// Build path to CSV file
	PATH_LEN = BASE_PATH_LEN + strlen(path_table) + 1;
	PATH = (char*)malloc(PATH_LEN);
	strcpy_s(PATH, PATH_LEN, cvpmd);
	strcat_s(PATH, PATH_LEN, path_main);
	strcat_s(PATH, PATH_LEN, g_szGameName);
	strcat_s(PATH, PATH_LEN, path_table);
	filename = PATH;
}

/**
* init struct and open file
*/
//DAR_TODO add logging to CSV parsing functions
bool AltsoundCsvParser::parse(PinSamples* psd)
{
	reader = (CsvReader*)malloc(sizeof(CsvReader));

	reader->f = fopen(filename, "r");
	if (reader->f == NULL) {
		free(reader);
		return false;
	}

	reader->delimiter = ',';
	reader->n_header_fields = 0;
	reader->n_fields = 0;

	// File opened, read header
	if (csv_read_header() != CSV_SUCCESS) {
		LOG(("%sfailed to read CSV header\n", indent));
		free(reader);
		reader = NULL;
	
		LOG(("%sEND: PARSE_ALTSOUND_CSV\n", indent)); //DAR_DEBUG
		return false;
	}
	
	int colID, colCHANNEL, colDUCK, colGAIN, colLOOP, colSTOP, colFNAME;
	LOG(("%snum_headers: %d\n", indent, reader->n_header_fields));
	
	colID = csv_get_colnumber_for_field("ID");
	colCHANNEL = csv_get_colnumber_for_field("CHANNEL");
	colDUCK = csv_get_colnumber_for_field("DUCK");
	colGAIN = csv_get_colnumber_for_field("GAIN");
	colLOOP = csv_get_colnumber_for_field("LOOP");
	colSTOP = csv_get_colnumber_for_field("STOP");
	colFNAME = csv_get_colnumber_for_field("FNAME");
	
	size_t pos = ftell(reader->f);
	
	while (csv_read_record() == CSV_SUCCESS) {
		int val;
		csv_get_hex_field(colID, &val);
		csv_get_int_field(colCHANNEL, &val);
		csv_get_int_field(colDUCK, &val);
		csv_get_int_field(colGAIN, &val);
		csv_get_int_field(colLOOP, &val);
		csv_get_int_field(colSTOP, &val);
		psd->num_files++;
	}
	
	if (psd->num_files > 0) {
		psd->ID = (int*)malloc(psd->num_files * sizeof(int));
		psd->files_with_subpath = (char**)malloc(psd->num_files * sizeof(char*));
		psd->channel = (signed char*)malloc(psd->num_files * sizeof(signed char));
		psd->gain = (float*)malloc(psd->num_files * sizeof(float));
		psd->ducking = (float*)malloc(psd->num_files * sizeof(float));
		psd->loop = (unsigned char*)malloc(psd->num_files * sizeof(unsigned char));
		psd->stop = (unsigned char*)malloc(psd->num_files * sizeof(unsigned char));
	}
	else {
		// DAR@20230516 It seems we should be cleaning up and getting out
		//              here.
		psd->files_with_subpath = NULL;
	}
	
	fseek(reader->f, pos, SEEK_SET);
	
	for (int i = 0; i < psd->num_files; ++i) {
		char filePath[4096];
		char tmpPath[4096];
	
		int val = 0;
		const int err = csv_read_record();
		if (err != CSV_SUCCESS) {
			LOG(("%sFAILED TO READ CSV RECORD %d\n", indent, i));
			psd->num_files = i;
			return false;
//			break;
		}
	
		csv_get_hex_field(colID, &val);
		psd->ID[i] = val;
		val = 0;
		psd->channel[i] = csv_get_int_field(colCHANNEL, &val) ? -1 : val;
		val = 0;
		csv_get_int_field(colDUCK, &val);
		psd->ducking[i] = val > 100 ? 1.0f : val < 0 ? -1.0f : val < 0 ? -1.0f : (float)val / 100.f;
		val = 0;
		csv_get_int_field(colGAIN, &val);
		psd->gain[i] = val > 100 ? 1.0f : (float)val / 100.f;
		val = 0;
		csv_get_int_field(colLOOP, &val);
		psd->loop[i] = val;
		val = 0;
		csv_get_int_field(colSTOP, &val);
		psd->stop[i] = val;
	
		strcpy_s(filePath, sizeof(filePath), cvpmd);
		strcat_s(filePath, sizeof(filePath), path_main);
		strcat_s(filePath, sizeof(filePath), g_szGameName);
		strcat_s(filePath, sizeof(filePath), "\\");
		strcat_s(filePath, sizeof(filePath), reader->fields[colFNAME]);
		GetFullPathName(filePath, sizeof(filePath), tmpPath, NULL);
		psd->files_with_subpath[i] = (char*)malloc(strlen(tmpPath) + 1);
		strcpy(psd->files_with_subpath[i], tmpPath);
	
		LOG(("%sID = %d, CHANNEL = %d, DUCK = %.2f, GAIN = %.2f, LOOP = %d, STOP = %d, FNAME = '%s'\n" \
			, indent, psd->ID[i], psd->channel[i], psd->ducking[i], psd->gain[i], psd->loop[i] \
			, psd->stop[i], psd->files_with_subpath[i]));
	}
	
	csv_close();
	LOG(("%sFOUND %d SAMPLES\n", indent, psd->num_files));
	
	// Clean up allocated memory
    //DAR_TODO

	return true;
}

/**
* trim field buffer from end to start
*/
void AltsoundCsvParser::trim(const char* const start, char* end) {
	while (end > start) {
		end--;
		if (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n') {
			*end = 0;
		}
		else {
			break;
		}
	}
}

/* Get a line of text from a file, discarding any end-of-line characters */
int AltsoundCsvParser::fgetline(char* const buff, const int nchars, FILE* const file)
{
	int length;

	if (fgets(buff, nchars, file) == NULL)
		return -1;
	if (buff[0] == '\r')
		memmove(buff, buff + 1, nchars - 1);

	length = (int)strlen(buff);
	while (length && (buff[length - 1] == '\r' || buff[length - 1] == '\n'))
		length--;
	buff[length] = 0;

	return length;
}

int AltsoundCsvParser::parse_line(char* line, const int header) {
	char* p = line;
	char* d = NULL;
	char* f = NULL;
	int capacity = 0;
	int field_number = 0;
	int enclosed_in_quotes = 0;
	int escaped = 0;
	int allocField = 1;
	int justAfterDelim = 1; 		// if set skip whitespace
	char** fields = header ? reader->header_fields : reader->fields;

	while (*p) {
		// realloc field array
		if (field_number == capacity) {
			int i;
			const int size = (capacity + 10) * sizeof(char*);
			fields = capacity == 0 ? (char**)malloc(size) : (char**)realloc(fields, size);
			capacity += 10;
			for (i = field_number; i < capacity; i++) fields[i] = NULL;
		}
		// allocate field
		if (allocField) {
			allocField = 0;
			fields[field_number] = _strdup(p);
			f = d = fields[field_number];
		}
		if (enclosed_in_quotes) {
			if (*p == '"' && !escaped)
				enclosed_in_quotes = 0;
			else if (*p == '\\' && !escaped)
				escaped = 1;
			else {
				if (justAfterDelim && (*p == ' ' || *p == '\t'))
					justAfterDelim = 0;
				else {
					*d++ = *p;	// copy char to target
					escaped = 0;
					justAfterDelim = 0;
				}
			}
		}
		else { // not in quotes
			if (*p == '"' && !escaped)
				enclosed_in_quotes = 1;
			else if (*p == reader->delimiter && !escaped) {
				// terminate current field
				*d = 0;
				trim(f, d);
				// next field
				field_number++;
				allocField = 1;
				justAfterDelim = 1;
			}
			else if (*p == '\\' && !escaped)
				escaped = 1;
			else {
				if (justAfterDelim && (*p == ' ' || *p == '\t'))
					justAfterDelim = 0;
				else {
					*d++ = *p;	// copy char to target
					escaped = 0;
					justAfterDelim = 0;
				}
			}
		}

		p++;
	}

	if (!d || !f)
		return CSV_ERROR_LINE_FORMAT;

	*d = 0;
	trim(f, d);

	if (enclosed_in_quotes) return CSV_ERROR_LINE_FORMAT; // quote still open
	if (escaped) return CSV_ERROR_LINE_FORMAT; // esc still open

	if (header) {
		reader->header_fields = fields;
		reader->n_header_fields = field_number + 1;
	}
	else {
		reader->fields = fields;
		reader->n_fields = field_number + 1;
	}

	return CSV_SUCCESS;
}

int AltsoundCsvParser::csv_read_header() {
	char buf[CSV_MAX_LINE_LENGTH];
	const int len = fgetline(buf, CSV_MAX_LINE_LENGTH, reader->f);
	if (len < 0)
		return CSV_ERROR_HEADER_NOT_FOUND;

	// parse line and look for headers
	return parse_line(buf, 1);
}

int AltsoundCsvParser::csv_get_colnumber_for_field(const char* fieldname) {
	int i;
	for (i = 0; i < reader->n_header_fields; i++)
		if (strcmp(reader->header_fields[i], fieldname) == 0)
			return i;

	return CSV_ERROR_NO_SUCH_FIELD;
}

void AltsoundCsvParser::free_record() {
	int i;
	for (i = 0; i < reader->n_fields; i++)
		free(reader->fields[i]);

	if (reader->n_fields) free(reader->fields);
}

int AltsoundCsvParser::csv_get_int_field(const int field_index, int* pValue) {
	if (field_index >= 0 && field_index < reader->n_fields) {
		if (sscanf(reader->fields[field_index], "%d", pValue) == 1) return 0;
		return CSV_ERROR_LINE_FORMAT;
	}

	return CSV_ERROR_FIELD_INDEX_OUT_OF_RANGE;
}

int AltsoundCsvParser::csv_get_hex_field(const int field_index, int* pValue) {
	if (field_index >= 0 && field_index < reader->n_fields) {
		if (sscanf(reader->fields[field_index], "0x%x", pValue) == 1) return 0;
		return CSV_ERROR_LINE_FORMAT;
	}

	return CSV_ERROR_FIELD_INDEX_OUT_OF_RANGE;
}

int AltsoundCsvParser::csv_get_float_field(const int field_index, float* pValue) {
	if (field_index >= 0 && field_index < reader->n_fields) {
		if (sscanf(reader->fields[field_index], "%f", pValue) == 1) return 0;
		return CSV_ERROR_LINE_FORMAT;
	}

	return CSV_ERROR_FIELD_INDEX_OUT_OF_RANGE;
}

int AltsoundCsvParser::csv_get_str_field(const int field_index, char** pValue) {
	if (field_index >= 0 && field_index < reader->n_fields) {
		*pValue = reader->fields[field_index];
		return 0;
	}

	return CSV_ERROR_FIELD_INDEX_OUT_OF_RANGE;
}

void AltsoundCsvParser::csv_close() {
	if (reader) {
		int i;
		free_record();
		for (i = 0; i < reader->n_header_fields; i++) {
			if (reader->header_fields[i]) free(reader->header_fields[i]);
		}
		if (reader->n_header_fields > 0) free(reader->header_fields);
		if (reader->f) fclose(reader->f);
		free(reader);
		reader = NULL;
	}
}

int AltsoundCsvParser::csv_read_record() {
	char buf[CSV_MAX_LINE_LENGTH];
	const int len = fgetline(buf, CSV_MAX_LINE_LENGTH, reader->f);
	if (len < 0)
		return CSV_ERROR_NO_MORE_RECORDS;

	free_record();
	// parse line and look for data
	return parse_line(buf, 0);
}