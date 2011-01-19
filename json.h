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

typedef struct json_hash_item {
	const JSON_KV *kvptr;
	struct json_hash_item *next;
} JSON_HASH;

#define KEY_HASH_SIZE 32
typedef struct json_object {
	int kvnr;	/* Key value pair number */
	int akvnr;	/* Allocated KV space number */
	JSON_HASH keyhash[KEY_HASH_SIZE];
	JSON_KV *kv;
} JSON_OBJ;

JSON_OBJ *json_create_obj(const char *str);
void json_free_obj(JSON_OBJ *jo);
void json_print_array(JSON_ARRAY *jo, int indent);
void json_print_obj(JSON_OBJ *jo, int indent);
int json_get_integer(const JSON_OBJ *jo, long long int *val, const char *key);
int json_get_floating(const JSON_OBJ *jo, double *val, const char *key);
int json_get_string(const JSON_OBJ *jo, const char **val, const char *key);
int json_get_boolean(const JSON_OBJ *jo, int *val, const char *key);
int json_get_array(const JSON_OBJ *jo, const JSON_ARRAY **val, const char *key);
int json_get_object(const JSON_OBJ *jo, const JSON_OBJ **val, const char *key);

int json_array_checktype(const JSON_ARRAY *ja, const char *typestr);
int json_array_get_val(const JSON_ARRAY *ja, void *val, int aidx);
#define quote(v) #v
#define dummy_assign(type, value_ptr) \
	{ type dummy __attribute__((unused)) = value_ptr; }
#define _json_array_foreach(const_array_ptr, value_ptr, type, aidx) \
	for (aidx = 0; \
	     json_array_checktype(const_array_ptr, quote(type)) && \
	     json_array_get_val(const_array_ptr, (void *)value_ptr, aidx); \
	     ++aidx)
#define _json_array_foreach_typecheck(const_array_ptr, value_ptr, type, aidx) \
	dummy_assign(json_##type##_type, value_ptr) \
	_json_array_foreach(const_array_ptr, value_ptr, json_##type##_type, aidx)

#define json_integer_type	long long int *
#define json_floating_type	double *
#define json_string_type	const char **
#define json_boolean_type	int *
#define json_array_type		const JSON_ARRAY **
#define json_object_type	const JSON_OBJ **

#define json_array_foreach_integer(const_array_ptr, value_ptr, aidx) \
	_json_array_foreach_typecheck(const_array_ptr, value_ptr, integer, aidx)
#define json_array_foreach_floating(const_array_ptr, value_ptr, aidx) \
	_json_array_foreach_typecheck(const_array_ptr, value_ptr, floating, aidx)
#define json_array_foreach_string(const_array_ptr, value_ptr, aidx) \
	_json_array_foreach_typecheck(const_array_ptr, value_ptr, string, aidx)
#define json_array_foreach_boolean(const_array_ptr, value_ptr, aidx) \
	_json_array_foreach_typecheck(const_array_ptr, value_ptr, boolean, aidx)
#define json_array_foreach_array(const_array_ptr, value_ptr, aidx) \
	_json_array_foreach_typecheck(const_array_ptr, value_ptr, array, aidx)
#define json_array_foreach_object(const_array_ptr, value_ptr, aidx) \
	_json_array_foreach_typecheck(const_array_ptr, value_ptr, object, aidx)

