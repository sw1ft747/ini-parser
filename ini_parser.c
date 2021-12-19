/** Parser of .ini files
*
*	Copyright (c) 2021 Sw1ft
*
*	Permission is hereby granted, free of charge, to any person obtaining a copy
*	of this software and associated documentation files (the "Software"), to deal
*	in the Software without restriction, including without limitation the rights
*	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*	copies of the Software, and to permit persons to whom the Software is
*	furnished to do so, subject to the following conditions:
*
*	The above copyright notice and this permission notice shall be included in all
*	copies or substantial portions of the Software.
*
*	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
*	SOFTWARE.
*/

#pragma warning(disable : 6001) /* Using "uninitialized" memory blah blah */

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#ifndef _CRT_NONSTDC_NO_DEPRECATE
#define _CRT_NONSTDC_NO_DEPRECATE
#endif

//-----------------------------------------------------------------------------

#include "ini_parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//-----------------------------------------------------------------------------

#define INI_STRIP_CHARS (" \t\n")

#define INI_STRIP_CHARS_LEN (sizeof(INI_STRIP_CHARS) - 1)
#define INI_COMMENT_PREFIX_LEN (sizeof(INI_COMMENT_PREFIX) - 1)

//-----------------------------------------------------------------------------

typedef enum
{
	PARSE_DATA = 0,
	PARSE_HANDLER
} parse_type_t;

//-----------------------------------------------------------------------------

static int s_ini_last_error_code = INI_NO_ERROR;
static int s_last_line = -1;

//-----------------------------------------------------------------------------

const char *ini_error_messages[] =
{
	"expected identifier of section",
	"expected end-of-section identifier",
	"section name is empty",
	"parameter name is empty",
	"value of parameter is empty"
};

//-----------------------------------------------------------------------------
// Primitive hash function
//-----------------------------------------------------------------------------

static int ini_get_hash(const char *str, size_t length)
{
	int hash = 0;

	for (size_t i = 0; i < length; ++i)
		hash += str[i];

	return hash;
}

//-----------------------------------------------------------------------------
// Purpose: add entry in the hash table
//-----------------------------------------------------------------------------

static void ini_add_entry(struct ini_data *data, struct ini_entry *entry)
{
	int element = ini_get_hash(entry->key, strlen(entry->key)) % INI_HASH_TABLE_SIZE;

	struct ini_entry *ent = data->entries[element];

	if (ent)
	{
		data->entries[element] = entry;
		entry->next = ent;
	}
	else
	{
		data->entries[element] = entry;
	}
}

//-----------------------------------------------------------------------------
// Purpose: check if the character contains at least one character in the array
//-----------------------------------------------------------------------------

static int ini_contains_chars(char ch, const char *chars, size_t length)
{
	for (size_t i = 0; i < length; ++i)
	{
		if (chars[i] == ch)
			return 1;
	}

	return 0;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static char *ini_lstrip(char *str)
{
	while (*str && ini_contains_chars(*str, INI_STRIP_CHARS, INI_STRIP_CHARS_LEN))
		++str;

	return str;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static void ini_rstrip(char *str)
{
	char *end = str + strlen(str) - 1;

	if (end < str)
		return;

	while (end >= str && ini_contains_chars(*end, INI_STRIP_CHARS, INI_STRIP_CHARS_LEN))
	{
		*end = '\0';
		--end;
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static char *ini_strip(char *str)
{
	char *result = ini_lstrip(str);
	ini_rstrip(result);
	return result;
}

//-----------------------------------------------------------------------------
// Purpose: set NUL-terminator where comment section begins
//-----------------------------------------------------------------------------

static void ini_remove_comment(char *str)
{
	while (*str && !ini_contains_chars(*str, INI_COMMENT_PREFIX, INI_COMMENT_PREFIX_LEN))
		++str;

	if (*str)
		*str = '\0';
}

//-----------------------------------------------------------------------------
// Purpose: get last error message
//-----------------------------------------------------------------------------

const char *ini_get_last_error_msg()
{
	if (s_ini_last_error_code <= INI_NO_ERROR)
		return NULL;

	return ini_error_messages[s_ini_last_error_code];
}

//-----------------------------------------------------------------------------
// Purpose: get last error code
//-----------------------------------------------------------------------------

int ini_get_last_error()
{
	return s_ini_last_error_code;
}

//-----------------------------------------------------------------------------
// Purpose: get last line where an error has occurred
//-----------------------------------------------------------------------------

int ini_get_last_line()
{
	return s_last_line;
}

//-----------------------------------------------------------------------------
// Purpose: read data from string
//-----------------------------------------------------------------------------

int ini_read_string(const char *value, struct ini_datatype *datatype, int fieldtype)
{
	switch (fieldtype != -1 ? fieldtype : datatype->fieldtype)
	{
	case INI_FIELD_INTEGER:
		datatype->m_int = atol(value);
		break;

	case INI_FIELD_INT64:
		datatype->m_int64 = atoll(value);
		break;

	case INI_FIELD_FLOAT:
		datatype->m_float = strtof(value, NULL);
		break;

	case INI_FIELD_DOUBLE:
		datatype->m_double = strtod(value, NULL);
		break;

	case INI_FIELD_BYTE:
		datatype->m_byte = (unsigned char)(atol(value) & 0xFF);
		break;

	case INI_FIELD_CHAR:
		datatype->m_char = (char)(atoi(value) & 0xFF);
		break;

	case INI_FIELD_CSTRING:
		datatype->m_pszString = strdup(value);
		break;

	case INI_FIELD_UINT32:
		datatype->m_uint32 = strtoul(value, NULL, datatype->radix);
		break;

	case INI_FIELD_UINT64:
		datatype->m_uint64 = strtoull(value, NULL, datatype->radix);
		break;

	case INI_FIELD_BOOL:
		datatype->m_bool = (!strcmp(value, "false") || !strcmp(value, "0")) ? 0 : 1;
		break;

	default:
		return 0;
	}

	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: read data from filled hash table
//-----------------------------------------------------------------------------

int ini_read_data(struct ini_data *data, const char *section, const char *key, struct ini_datatype *datatype, int fieldtype)
{
	const char *value = NULL;

	int element = ini_get_hash(key, strlen(key)) % INI_HASH_TABLE_SIZE;
	struct ini_entry *entry = data->entries[element];

	while (entry != NULL)
	{
		if (!strcmp(key, entry->key) && !strcmp(section, entry->section))
		{
			value = entry->value;
			break;
		}

		entry = entry->next;
	}

	if (!value)
		return 0;

	return ini_read_string(value, datatype, fieldtype);
}

//-----------------------------------------------------------------------------
// Purpose: main function for parsing .ini files
//-----------------------------------------------------------------------------

int ini_parse(const char *filename, parse_type_t type, struct ini_data *data, iniHandlerFn handler)
{
	FILE *file = fopen(filename, "r");

	if (file)
	{
		// Zero memory
		if (type == PARSE_DATA)
			memset(data, 0, sizeof(struct ini_data));

		int line = 0;
		int parsingSection = 1;

		static int bufferSize = INI_BUFFER_LENGTH;
		static char *pszFileBuffer = NULL;

		if (!pszFileBuffer)
			pszFileBuffer = malloc(bufferSize);

		long int endpos;
		fseek(file, 0, SEEK_END);
		endpos = ftell(file);
		rewind(file);

		const char *pszSection = NULL;

		// Read line by line
		while (fgets(pszFileBuffer, bufferSize, file))
		{
			size_t length = strlen(pszFileBuffer);

			// Increase buffer size
			while (pszFileBuffer[length - 1] != '\n' && ftell(file) != endpos)
			{
				bufferSize *= 2;

				void *realloc_mem = realloc(pszFileBuffer, bufferSize);

				if (!realloc_mem)
				{
					fclose(file);
					return 0;
				}

				pszFileBuffer = realloc_mem;
				fgets(pszFileBuffer + length, bufferSize - length, file);

				length = strlen(pszFileBuffer);
			}

			++line;

			// Strip string from spaces and comments
			char *str = ini_lstrip(pszFileBuffer);
			ini_remove_comment(str);
			ini_rstrip(str);

			// Nothing here, skip
			if (!*str)
				continue;

			// Parsing section
			if (parsingSection)
			{
			PARSE_SECTION:
				if (*str == INI_SECTION_PREFIX)
				{
					char *end = str + strlen(str) - 1;

					// Exclude prefix of section
					++str;

					if (*end == INI_SECTION_POSTFIX)
					{
						// Exclude postfix of section
						*end = '\0';

						// Strip
						str = ini_strip(str);

						if (*str)
						{
							// Next try to parse parameters
							parsingSection = 0;

							// Free previous name of section
							if (pszSection)
								free((void *)pszSection);
							
							// Save name of section
							pszSection = strdup(str);
						}
						else
						{
							s_ini_last_error_code = INI_ERROR_SECTION_EMPTY;
							s_last_line = line;
							return 0;
						}
					}
					else
					{
						s_ini_last_error_code = INI_ERROR_SECTION_END_ID;
						s_last_line = line;
						return 0;
					}
				}
				else
				{
					s_ini_last_error_code = INI_ERROR_SECTION_START_ID;
					s_last_line = line;
					return 0;
				}
			}
			else
			{
				// Found prefix of section, start parsing a new one
				if (*str == INI_SECTION_PREFIX)
				{
					parsingSection = 1;
					goto PARSE_SECTION;
				}

				// Split parameter
				char *key = strtok(str, INI_PARAMETER_DELIMITER);

				if (!key || !*key)
				{
					s_ini_last_error_code = INI_ERROR_KEY_EMPTY;
					s_last_line = line;
					return 0;
				}

				char *value = strtok(NULL, INI_PARAMETER_DELIMITER);

				if (!value || !*value)
				{
					s_ini_last_error_code = INI_ERROR_VALUE_EMPTY;
					s_last_line = line;
					return 0;
				}

				// Strip
				ini_rstrip(key);
				value = ini_strip(value);

				if (type == PARSE_DATA)
				{
					// Fill our hash table
					struct ini_entry *entry = calloc(1, sizeof(struct ini_entry));

					entry->key = strdup(key);
					entry->value = strdup(value);
					entry->section = strdup(pszSection);

					ini_add_entry(data, entry);
				}
				else if (type == PARSE_HANDLER)
				{
					// Call our callback
					handler(pszSection, key, value);
				}
			}
		}

		// Free allocated memory
		if (pszSection)
			free((void *)pszSection);

		fclose(file);

		s_ini_last_error_code = INI_NO_ERROR;
		s_last_line = -1;

		return 1;
	}
	else
	{
		s_ini_last_error_code = INI_MISSING_FILE;
		return 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: wrapper to save .ini data in hash table
//-----------------------------------------------------------------------------

int ini_parse_data(const char *filename, struct ini_data *data)
{
	return ini_parse(filename, PARSE_DATA, data, NULL);
}

//-----------------------------------------------------------------------------
// Purpose: wrapper to call a callback when parse .ini file
//-----------------------------------------------------------------------------

int ini_parse_handler(const char *filename, iniHandlerFn handler)
{
	return ini_parse(filename, PARSE_HANDLER, NULL, handler);
}

//-----------------------------------------------------------------------------
// Purpose: free allocated memory from hash table
//-----------------------------------------------------------------------------

void ini_free_data(struct ini_data *data, int is_allocated)
{
	for (size_t i = 0; i < INI_HASH_TABLE_SIZE; ++i)
	{
		struct ini_entry *entry = data->entries[i];

		while (entry != NULL)
		{
			struct ini_entry *prev = entry;
			entry = entry->next;

			free((void *)prev->key);
			free((void *)prev->value);
			free((void *)prev->section);
			free(prev);
		}
	}

	if (is_allocated)
		free(data);
}
