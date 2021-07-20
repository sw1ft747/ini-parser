# ini-parser
Parser of .ini files. Written on C, compatibility with C++

# Methods
You can use two methods to parse .ini file, using a callback function (handler) in real time or fill data in hash table and after parsing file you can call functions to read data from filled hash table

### Callback function

First you need declare a function to handle calls from function `ini_parse_handler`, here you will compare sections and parameters

Signature:
```cpp
void MyFuncHandler(const char *pszSection, const char *pszKey, const char *pszValue);
```

Now you can call function to parse .ini file

```cpp
int ini_parse_handler(const char *filename, iniHandlerFn handler);
```

Example:
```cpp
void MyFuncHandler(const char *pszSection, const char *pszKey, const char *pszValue)
{
	INI_INIT_PARAMETERS(pszSection, pszKey);

	struct ini_datatype datatype;

	if (INI_GET_PARAMETER("SETTINGS", "Port"))
	{
		INI_FIELDTYPE_INT(datatype);
		
		if (ini_read_string(pszValue, &datatype))
		{
			printf("SETTINGS: Port = %d\n", datatype.m_int);
		}
	}
}

int main()
{
	int success = ini_parse_handler("test.ini", MyHandlerFunc);
	
	if ( !success )
	{
		if (ini_get_last_error() == INI_MISSING_FILE)
			printf("Missing file test.ini to parse\n");
		else
			printf("Syntax error: %s in line %d\n", ini_get_last_error_msg(), ini_get_last_line());
	}

	return 0;
}
```

### Filling hash table
First you need to declare hash table struct called `ini_data`

After, call function to parse .ini file

```cpp
int ini_parse_data(const char *filename, struct ini_data *data);
```

Example from my [project](https://github.com/r47t/suspend-process/blob/main/main.cpp#L111 "project")

Example:
```cpp
int main()
{
	ini_data data;

	if ( !ini_parse_data("test.ini", &data) )
	{
		if (ini_get_last_error() == INI_MISSING_FILE)
			printf("Missing file test.ini to parse\n");
		else
			printf("Syntax error: %s in line %d\n", ini_get_last_error_msg(), ini_get_last_line());

		return 1;
	}

	ini_datatype datatype;
	INI_FIELDTYPE_UINT32(datatype);

	if (ini_read_data(&data, "CONTROLS", "Button", &datatype))
	{
		printf("CONTROLS: Button = %X\n", data.m_uint32);
	}

	ini_free_data(&data, 0);
	return 0;
}
```

Do not forget to free memory in hash table

```cpp
void ini_free_data(struct ini_data *data, int is_allocated);
```

# Reading data from .ini file
There's two function to read data: directly from string and from hash table

```cpp
int ini_read_string(const char *value, struct ini_datatype *datatype);
int ini_read_data(struct ini_data *data, const char *section, const char *key, struct ini_datatype *datatype);
```

I added structure to choose what type of data to read and write it union inside

```cpp
// What types to read
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

struct ini_datatype
{
	ini_field_type_t fieldtype;
	int radix; // Radix when read INI_FIELD_UINT32 or INI_FIELD_UINT64

	// Output will be here
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

// Useful macros to choose types to read
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
```

*Note: when you read string it will be allocated, so don't forget to free memory if it will be needed via function free()*
