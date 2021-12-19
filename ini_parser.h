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

#ifndef INI_PARSER_H
#define INI_PARSER_H

#ifdef __cplusplus
extern "C" {
#endif

//-----------------------------------------------------------------------------
// Size in bytes of buffer when read .ini file
//-----------------------------------------------------------------------------

#define INI_BUFFER_LENGTH 256

//-----------------------------------------------------------------------------
// Size of hash table
//-----------------------------------------------------------------------------

#define INI_HASH_TABLE_SIZE 64

//-----------------------------------------------------------------------------
// Prefixes of comments
//-----------------------------------------------------------------------------

#define INI_COMMENT_PREFIX ";#"

//-----------------------------------------------------------------------------
// Prefix of section
//-----------------------------------------------------------------------------

#define INI_SECTION_PREFIX '['

//-----------------------------------------------------------------------------
// Postfix of section
//-----------------------------------------------------------------------------

#define INI_SECTION_POSTFIX ']'

//-----------------------------------------------------------------------------
// Delimiter of key and value
//-----------------------------------------------------------------------------

#define INI_PARAMETER_DELIMITER "="

//-----------------------------------------------------------------------------
// Helper macros
//-----------------------------------------------------------------------------

#define INI_INIT_PARAMETERS(__sect, __key) const char *__section__ = __sect; const char *__key__ = __key
#define INI_GET_PARAMETER(__sect, __key) (!strcmp(__key, __key__) && !strcmp(__sect, __section__))

#define INI_FIELDTYPE_INT(datatype) datatype.fieldtype = INI_FIELD_INTEGER
#define INI_FIELDTYPE_INT64(datatype) datatype.fieldtype = INI_FIELD_INT64
#define INI_FIELDTYPE_FLOAT(datatype) datatype.fieldtype = INI_FIELD_FLOAT
#define INI_FIELDTYPE_DOUBLE(datatype) datatype.fieldtype = INI_FIELD_DOUBLE
#define INI_FIELDTYPE_BYTE(datatype) datatype.fieldtype = INI_FIELD_BYTE
#define INI_FIELDTYPE_CHAR(datatype) datatype.fieldtype = INI_FIELD_CHAR
#define INI_FIELDTYPE_CSTRING(datatype) datatype.fieldtype = INI_FIELD_CSTRING
#define INI_FIELDTYPE_UINT32(datatype, basis) datatype.fieldtype = INI_FIELD_UINT32; datatype.radix = ((basis < 0) ? 0 : (basis > 16) ? 16 : basis)
#define INI_FIELDTYPE_UINT64(datatype, basis) datatype.fieldtype = INI_FIELD_UINT64; datatype.radix = ((basis < 0) ? 0 : (basis > 16) ? 16 : basis)
#ifdef __cplusplus
#define INI_FIELDTYPE_BOOL(datatype) datatype.fieldtype = INI_FIELD_BOOL
#endif

//-----------------------------------------------------------------------------
// Signature of function handler
//-----------------------------------------------------------------------------

typedef void (*iniHandlerFn)(const char *pszSection, const char *pszKey, const char *pszValue);

//-----------------------------------------------------------------------------
// Error codes
//-----------------------------------------------------------------------------

enum ini_error_code
{
	INI_MISSING_FILE = -2,
	INI_NO_ERROR = -1,
	INI_ERROR_SECTION_START_ID,
	INI_ERROR_SECTION_END_ID,
	INI_ERROR_SECTION_EMPTY,
	INI_ERROR_KEY_EMPTY,
	INI_ERROR_VALUE_EMPTY
};

//-----------------------------------------------------------------------------
// Field type to read
//-----------------------------------------------------------------------------

typedef enum ini_field_type
{
	INI_FIELD_INTEGER = 0,
	INI_FIELD_INT64,
	INI_FIELD_FLOAT,
	INI_FIELD_DOUBLE,
	INI_FIELD_BYTE,
	INI_FIELD_CHAR,
	INI_FIELD_CSTRING,
	INI_FIELD_UINT32,
	INI_FIELD_UINT64,
	INI_FIELD_BOOL
} ini_field_type_t;

//-----------------------------------------------------------------------------
// Structure for reading data types
//-----------------------------------------------------------------------------

struct ini_datatype
{
	ini_field_type_t fieldtype;
	int radix;

	union
	{
		int					m_int;
		signed long long	m_int64;
		float				m_float;
		double				m_double;
		unsigned char		m_byte;
		char				m_char;
		const char			*m_pszString;
		unsigned int		m_uint32;
		unsigned long long	m_uint64;
	#ifdef __cplusplus
		bool				m_bool;
	#else
		unsigned char		m_bool;
	#endif
	};
};

//-----------------------------------------------------------------------------
// Structure of entry used in hash table
//-----------------------------------------------------------------------------

struct ini_entry
{
	struct ini_entry *next;

	const char *key;
	const char *value;

	const char *section;
};

//-----------------------------------------------------------------------------
// Hash table
//-----------------------------------------------------------------------------

struct ini_data
{
	struct ini_entry *entries[INI_HASH_TABLE_SIZE];
};

//-----------------------------------------------------------------------------
// Purpose: get last error message
//-----------------------------------------------------------------------------

const char *ini_get_last_error_msg();

//-----------------------------------------------------------------------------
// Purpose: get last error code
//-----------------------------------------------------------------------------

int ini_get_last_error();

//-----------------------------------------------------------------------------
// Purpose: get last line where an error has occurred
//-----------------------------------------------------------------------------

int ini_get_last_line();

//-----------------------------------------------------------------------------
// Purpose: read data from string
//
// Params:
// @value - string to read
// @datatype - field type to read
// @fieldtype - directly read data type (-1 - ignore)
//
// Return value: 1 - success, 0 - failed to read data
//-----------------------------------------------------------------------------

int ini_read_string(const char *value, struct ini_datatype *datatype, int fieldtype);

//-----------------------------------------------------------------------------
// Purpose: read data from filled hash table
//
// Params:
// @data - pointer to hash table
// @section - name of section
// @key - name of parameter
// @datatype - field type to read
// @fieldtype - directly read data type (-1 - ignore)
//
// Return value: 1 - success, 0 - failed to read data
//-----------------------------------------------------------------------------

int ini_read_data(struct ini_data *data, const char *section, const char *key, struct ini_datatype *datatype, int fieldtype);

//-----------------------------------------------------------------------------
// Purpose: save data from .ini file in hash table
//
// Params:
// @filename - directory of file
// @data - pointer to hash table
//
// Return value: 1 - success, 0 - failed to parse file
//-----------------------------------------------------------------------------

int ini_parse_data(const char *filename, struct ini_data *data);

//-----------------------------------------------------------------------------
// Purpose: call a callback when parse .ini file
//
// Params:
// @filename - directory of file
// @handler - pointer to function handler
//
// Return value: 1 - success, 0 - failed to parse file
//-----------------------------------------------------------------------------

int ini_parse_handler(const char *filename, iniHandlerFn handler);

//-----------------------------------------------------------------------------
// Purpose: free allocated memory for hash table
//
// Params:
// @data - pointer to hash table
// @is_allocated - hash table was allocated
//-----------------------------------------------------------------------------

void ini_free_data(struct ini_data *data, int is_allocated);

//-----------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif // INI_PARSER_H