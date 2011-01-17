/*
 * Json parser interface
 *
 * Copyright 2011 Guo-Fu Tseng <cooldavid@cooldavid.org>
 *
 * Author: Guo-Fu Tseng <cooldavid@cooldavid.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#undef CONVERT_TO_UTF8
typedef struct json_parse_state {
	const char *buf, *left;
	union {
		double number;
		char *string;
	};
} JPS;

enum json_tokens {
	Tkn_Error,
	Tkn_LCurlyBrackets,
	Tkn_RCurlyBrackets,
	Tkn_Colon,
	Tkn_Comma,
	Tkn_LSquareBrackets,
	Tkn_RSquareBrackets,
	Tkn_String,
	Tkn_True,
	Tkn_False,
	Tkn_Null,
	Tkn_Integer,
	Tkn_Floating,
};

enum json_value_type {
	Val_Null,
	Val_Integer,
	Val_Floating,
	Val_String,
	Val_Boolean,
	Val_Array,
	Val_Object,
};

#define DEFAULT_ARNR 20
struct json_value;
typedef struct json_array {
	int size;
	int asize;
	int type;
	struct json_value *values;
} JSON_ARRAY;

struct json_object;
typedef struct json_value {
	union {
		long long integer;
		double floating;
		char *string;
		int boolean;
		struct json_array *array;
		struct json_object *object;
	};
} JSON_VAL;

typedef struct json_key_val {
	char *key;
	int type;
	JSON_VAL value;
} JSON_KV;

#define DEFAULT_KVNR 20
typedef struct json_object {
	int kvnr;	/* Key value pair number */
	int akvnr;	/* Allocated KV space number */
	JSON_KV *kv;
} JSON_OBJ;

JSON_OBJ *create_json_obj(const char *str);
void free_json_obj(JSON_OBJ *jo);
void print_json_array(JSON_ARRAY *jo, int indent);
void print_json_obj(JSON_OBJ *jo, int indent);
