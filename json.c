/*
 * Json parser implementation
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
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>

#include "json.h"

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
#define DEFAULT_KVNR 20

static unsigned int hexvaltbl[] = {
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0, /*   0 to   9 */
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0, /*  10 to  19 */
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0, /*  20 to  29 */
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0, /*  30 to  39 */
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  1, /*  40 to  49 */
	 2,  3,  4,  5,  6,  7,  8,  9,  0,  0, /*  50 to  59 */
	 0,  0,  0,  0,  0, 10, 11, 12, 13, 14, /*  60 to  69 */
	15,  0,  0,  0,  0,  0,  0,  0,  0,  0, /*  70 to  79 */
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0, /*  80 to  89 */
	 0,  0,  0,  0,  0,  0,  0, 10, 11, 12, /*  90 to  99 */
	13, 14, 15,  0,  0,  0,  0,  0,  0,  0, /* 100 to 109 */
};

static int ishexdigit(char ch)
{
	return (ch >= '0' && ch <= '9') ||
		(ch >= 'a' && ch <= 'z') ||
		(ch >= 'A' && ch <= 'Z');
}

static int encode_utf8(char *dest, unsigned int ucval)
{
	int dlen;
	unsigned int exbytenr;
	unsigned char *dp = (unsigned char *)dest;

	if(ucval < 0x80u) {
		if (dest)
			*dest = (char)ucval;
		return 1;
	}

	/* Calculate the number of extra byte(s) */
	for(exbytenr = 1; exbytenr < 5; ++exbytenr)
		if(ucval < (1u << (exbytenr * 5 + 6)))
			break;
	dlen = 1 + exbytenr;

	if (!dest)
		return dlen;

	/* Write first byte value */
	*dp = (((1u << (exbytenr + 2)) - 2) << (6 - exbytenr)) |
		((ucval >> (6 * exbytenr)) & ((1u << (6 - exbytenr)) - 1));

	/* Write extra bytes */
	++dp;
	while (exbytenr--) {
		*dp = 0x80u | ((ucval >> (6 * exbytenr)) & 0x3Fu);
		++dp;
	}

	return dlen;
}

static inline unsigned int hexstrtoval(const char *hexstr)
{
	const unsigned char *hs = (const unsigned char *)hexstr;

	return  ((hexvaltbl[hs[0]] << 12) & 0xF000u) |
		((hexvaltbl[hs[1]] <<  8) & 0x0F00u) |
		((hexvaltbl[hs[2]] <<  4) & 0x00F0u) |
		((hexvaltbl[hs[3]]      ) & 0x000Fu);
}

static const char *decode_excape_uvalue(unsigned int *ucvalp, const char *src)
{
	unsigned int ucval, ucval2;
	char errorch = ' ';

	ucval = hexstrtoval(src);
	src += 4;

	/* Get unicode rang U+10000 ... U+10FFFF */
	if(ucval >= 0xD800u && ucval <= 0xDFFFu) {
		/* Expect high 10 bits encode and low 10 bits input */
		if(ucval >= 0xDC00u || src[0] != '\\' || src[1] != 'u' ||
		   !ishexdigit(src[2]) || !ishexdigit(src[3]) ||
		   !ishexdigit(src[4]) || !ishexdigit(src[5])) {
			*ucvalp = errorch;
			return src;
		}
		ucval2 = hexstrtoval(src + 2);
		/* Expect low 10 bits encode */
		if(ucval2 < 0xDC00u || ucval2 >= 0xE000u) {
			*ucvalp = errorch;
			return src;
		}
		*ucvalp = ((ucval - 0xD800u) << 10) | (ucval2 - 0xDC00u);
		return src + 6;
	} else {
		*ucvalp = ucval;
		return src;
	}
}

static int get_string(JPS *jps)
{
	int slen;
	char *sp;
	const char *p = jps->left;
	unsigned int ucval;

	if (*p != '\"')
		return Tkn_Error;
	++p;

	/* Calculate length and syntax check */
	slen = 0;
	while (*p != '\"') {
		if (!*p)
			return Tkn_Error;
		if (*p == '\\') {
			++p;
			switch (*p) {
			case '\"':
			case '\\':
			case '/':
			case 'b':
			case 'f':
			case 'n':
			case 'r':
			case 't':
				++p;
				++slen;
				break;
			case 'u':
				++p;
				if (ishexdigit(p[0]) && ishexdigit(p[1]) &&
				    ishexdigit(p[2]) && ishexdigit(p[3])) {
					p = decode_excape_uvalue(&ucval, p);
					slen += encode_utf8(NULL, ucval);
				} else {
					return Tkn_Error;
				}
				break;
			default:
				return Tkn_Error;
			}
			continue;
		}
		++p;
		++slen;
	}

	/* Allocate and copy string */
	jps->string = malloc(slen + 1);
	if (!jps->string)
		return Tkn_Error;
	p = jps->left + 1;
	sp = jps->string;
	while (*p != '\"') {
		if (*p == '\\') {
			++p;
			switch (*p) {
			case '"':
				*sp = '"';
				++sp; ++p;
				break;
			case '\\':
				*sp = '\\';
				++sp; ++p;
				break;
			case '/':
				*sp = '/';
				++sp; ++p;
				break;
			case 'b':
				*sp = '\b';
				++sp; ++p;
				break;
			case 'f':
				*sp = '\f';
				++sp; ++p;
				break;
			case 'n':
				*sp = '\n';
				++sp; ++p;
				break;
			case 'r':
				*sp = '\r';
				++sp; ++p;
				break;
			case 't':
				*sp = '\t';
				++sp; ++p;
				break;
			case 'u':
				p = decode_excape_uvalue(&ucval, p + 1);
				sp += encode_utf8(sp, ucval);
				break;
			default:
				return Tkn_Error;
			}
			continue;
		}
		*sp = *p;
		++sp;
		++p;
	}
	*sp = '\0';
	jps->left = ++p;
	return Tkn_String;
}

static int get_number(JPS *jps)
{
	const char *p = jps->left, *ep, *ip;
	int sign = 1, step;
	int exp = 0, expdir = 1;
	int isfloating = 0;

	if (*p == '-') {
		sign = -1;
		++p;
	}

	if (!isdigit(*p))
		return Tkn_Error;

	ep = p;
	while (isdigit(*ep))
		++ep;
	jps->number = 0;
	step = 1;
	ip = ep;
	while (--ip >= p) {
		jps->number += (*ip - '0') * step;
		step *= 10;
	}
	p = ep;

	if (*p == '.') {
		isfloating = 1;
		++p;
		if (!isdigit(*p))
			return Tkn_Error;
		step = 10;
		while (isdigit(*p)) {
			jps->number += (double)(*p - '0') / (double)step;
			++p;
		}
	}

	if (*p == 'e' || *p == 'E') {
		++p;
		if (*p == '+') {
			++p;
		} else if (*p == '-') {
			expdir = -1;
			++p;
		} else if (!isdigit(*p)) {
			return Tkn_Error;
		}

		ep = p;
		while (isdigit(*ep))
			++ep;
		exp = 0;
		step = 1;
		ip = ep;
		while (--ip >= p) {
			exp += (*ip - '0') * step;
			step *= 10;
		}
		p = ep;
	}
	jps->left = p;

	if (exp) {
		exp *= expdir;
		jps->number *= pow((double)10, (double)exp);
	}
	jps->number *= sign;
	if (isfloating)
		return Tkn_Floating;
	else
		return Tkn_Integer;
}

static int get_token(JPS *jps)
{
	const char *p = jps->left;

	while (*p && isspace(*p))
		++p;

	/* Check for special characters */
	switch (*p) {
	case '{':
		jps->left = ++p;
		return Tkn_LCurlyBrackets;
	case '}':
		jps->left = ++p;
		return Tkn_RCurlyBrackets;
	case ':':
		jps->left = ++p;
		return Tkn_Colon;
	case ',':
		jps->left = ++p;
		return Tkn_Comma;
	case '[':
		jps->left = ++p;
		return Tkn_LSquareBrackets;
	case ']':
		jps->left = ++p;
		return Tkn_RSquareBrackets;
	case '\"':
		jps->left = p;
		return get_string(jps);
	case '\0':
		return Tkn_Error;
	}

	/* Check for special tokens */
	if (!strncasecmp(p, "true", 4)){
		jps->left = p + 4;
		return Tkn_True;
	} else if (!strncasecmp(p, "false", 5)) {
		jps->left = p + 5;
		return Tkn_False;
	} else if (!strncasecmp(p, "null", 4)) {
		jps->left = p + 4;
		return Tkn_Null;
	} else if (*p == '-' || isdigit(*p)) {
		jps->left = p;
		return get_number(jps);
	}

	return Tkn_Error;
}

static int peek_token(JPS *jps) {
	const char *p;
	int tkn;

	p = jps->left;
	tkn = get_token(jps);
	if (tkn == Tkn_String) {
		free(jps->string);
		jps->string = NULL;
	}
	jps->left = p;

	return tkn;
}

static int expect(int tkn, JPS *jps)
{
	int token;
	token = get_token(jps);
	if (tkn != token) {
		fprintf(stderr, "\texpect %d got %d\n", tkn, token);
		return 1;
	} else {
		return 0;
	}
}

static JSON_OBJ *alloc_json_obj(void)
{
	JSON_OBJ *jo;

	jo = calloc(1, sizeof(JSON_OBJ));
	if (!jo)
		return NULL;
	jo->kv = calloc(1, DEFAULT_KVNR * sizeof(JSON_KV));
	if (!jo->kv) {
		free(jo);
		return NULL;
	}
	jo->akvnr = DEFAULT_KVNR;

	return jo;
}

static void json_free_hash(JSON_HASH *hash)
{
	int i;
	JSON_HASH *hp;

	for (i = 0; i < KEY_HASH_SIZE; ++i) {
		hp = hash[i].next;
		while (hp) {
			hash[i].next = hp->next;
			free(hp);
			hp = hash[i].next;
		}
	}
	memset(hash, 0, KEY_HASH_SIZE * sizeof(JSON_HASH));
}

static unsigned int json_hashkey(const char *strkey)
{
	int klen = strlen(strkey), i;
	unsigned int hashkey = 0xCDCDCDCD;

	for (i = 0; i < klen; ++i) {
		if (i & 1)
			hashkey += strkey[i];
		else
			hashkey *= strkey[i];
	}
	hashkey %= KEY_HASH_SIZE;
	return hashkey;
}

static void json_insert_hash(const JSON_KV *jkv, JSON_HASH *hash)
{
	JSON_HASH *hp;

	hp = hash + json_hashkey(jkv->key);
	while (hp->next)
		hp = hp->next;
	if (hp->kvptr) {
		hp->next = calloc(1, sizeof(JSON_HASH));
		hp = hp->next;
	}
	hp->kvptr = jkv;
}

static JSON_KV *json_obj_next_kv(JSON_OBJ *jo)
{
	int i;
	if (jo->kvnr == jo->akvnr) {
		jo->akvnr <<= 1;
		json_free_hash(jo->keyhash);
		jo->kv = realloc(jo->kv, jo->akvnr * sizeof(JSON_KV));
		if (!jo->kv)
			return NULL;
	}
	for (i = 0; i < jo->kvnr; ++i)
		json_insert_hash(jo->kv + i, jo->keyhash);
	++jo->kvnr;
	return jo->kv + (jo->kvnr - 1);
}

static JSON_ARRAY *alloc_json_array(void)
{
	JSON_ARRAY *ja;

	ja = calloc(1, sizeof(JSON_ARRAY));
	if (!ja)
		return NULL;
	ja->values = calloc(1, DEFAULT_ARNR * sizeof(JSON_VAL));
	if (!ja->values) {
		free(ja);
		return NULL;
	}
	ja->asize = DEFAULT_ARNR;

	return ja;
}

static JSON_VAL *json_array_next_val(JSON_ARRAY *ja)
{
	if (ja->size == ja->asize) {
		ja->asize <<= 1;
		ja->values = realloc(ja->values, ja->asize * sizeof(JSON_VAL));
		if (!ja->values)
			return NULL;
	}
	++ja->size;
	return ja->values + (ja->size - 1);
}

static int json_value(int *type, JSON_VAL *value, JPS *jps);
static int json_array(JSON_ARRAY *array, JPS *jps)
{
	int token, vtype;
	JSON_VAL *val;

	if (expect(Tkn_LSquareBrackets, jps))
		return -1;

	/* Handle empty array */
	token = peek_token(jps);
	if (token == Tkn_RSquareBrackets)
		return expect(Tkn_RSquareBrackets, jps);

	/* Process each element */
	while (1) {
		val = json_array_next_val(array);
		if (!val)
			return -1;
		if (json_value(&vtype, val, jps))
			return -1;
		/*
		 * FIXME: Check for type consistency for each element
		 *        in the array
		 */
		array->type = vtype;

		token = peek_token(jps);
		if (token == Tkn_Comma)
			expect(Tkn_Comma, jps);
		else if (token == Tkn_Error)
			return -1;
		else
			break;
	};

	if (expect(Tkn_RSquareBrackets, jps))
		return -1;

	return 0;
}

static int json_obj(JSON_OBJ *jo, JPS *jps);
static int json_value(int *type, JSON_VAL *value, JPS *jps)
{
	int token;

	token = peek_token(jps);
	switch (token) {
	case Tkn_String:
		get_token(jps);
		*type = Val_String;
		value->string = jps->string;
		jps->string = NULL;
		return 0;
	case Tkn_Integer:
		get_token(jps);
		*type = Val_Integer;
		value->integer = (long long)jps->number;
		return 0;
	case Tkn_Floating:
		get_token(jps);
		*type = Val_Floating;
		value->floating = jps->number;
		return 0;
	case Tkn_LCurlyBrackets:
		*type = Val_Object;
		value->object = alloc_json_obj();
		return json_obj(value->object, jps);
	case Tkn_LSquareBrackets:
		*type = Val_Array;
		value->array = alloc_json_array();
		return json_array(value->array, jps);
	case Tkn_True:
		get_token(jps);
		*type = Val_Boolean;
		value->boolean = 1;
		return 0;
	case Tkn_False:
		get_token(jps);
		*type = Val_Boolean;
		value->boolean = 0;
		return 0;
	case Tkn_Null:
		get_token(jps);
		*type = Val_Null;
		return 0;
	default:
		return -1;
	}
}

static int json_kv(JSON_OBJ *jo, JPS *jps)
{
	JSON_KV *jkv;

	/* Get key */
	if (expect(Tkn_String, jps))
		return -1;
	jkv = json_obj_next_kv(jo);
	if (!jkv)
		return -1;
	jkv->key = jps->string;
	jps->string = NULL;
	json_insert_hash(jkv, jo->keyhash);

	/* Get Separator */
	if (expect(Tkn_Colon, jps))
		return -1;

	/* Get Value */
	if (json_value(&jkv->type, &jkv->value, jps))
		return -1;

	return 0;
}

static int json_kv_list(JSON_OBJ *jo, JPS *jps)
{
	int token;

	if (json_kv(jo, jps))
		return -1;

	token = peek_token(jps);
	while (token == Tkn_Comma) {
		expect(Tkn_Comma, jps);
		if (json_kv(jo, jps))
			return -1;
		token = peek_token(jps);
	}
	if (token == Tkn_Error)
		return -1;

	return 0;
}

static int json_obj(JSON_OBJ *jo, JPS *jps)
{
	int token;

	if (expect(Tkn_LCurlyBrackets, jps))
		return -1;

	/* Handle empty object */
	token = peek_token(jps);
	if (token == Tkn_RCurlyBrackets) {
		if(expect(Tkn_RCurlyBrackets, jps))
			return -1;
		return 0;
	}

	/* Process object KV elements */
	if (json_kv_list(jo, jps))
		return -1;

	if (expect(Tkn_RCurlyBrackets, jps))
		return -1;
	return 0;
}

void json_free_obj(JSON_OBJ *jo);
static void json_free_array(JSON_ARRAY *ja)
{
	int i;

	for (i = 0; i < ja->size; ++i) {
		if (ja->type == Val_String) {
			free(ja->values[i].string);
		} else if (ja->type == Val_Object) {
			json_free_obj(ja->values[i].object);
		} else if (ja->type == Val_Array) {
			json_free_array(ja->values[i].array);
		} else {
			break;
		}
	}
	free(ja->values);
	free(ja);
}

void json_free_obj(JSON_OBJ *jo)
{
	int i;

	if (!jo)
		return;

	for (i = 0; i < jo->kvnr; ++i) {
		if (jo->kv[i].type == Val_String) {
			free(jo->kv[i].value.string);
		} else if (jo->kv[i].type == Val_Object) {
			json_free_obj(jo->kv[i].value.object);
		} else if (jo->kv[i].type == Val_Array) {
			json_free_array(jo->kv[i].value.array);
		}
		free(jo->kv[i].key);
	}
	json_free_hash(jo->keyhash);
	free(jo->kv);
	free(jo);
}

JSON_OBJ *json_create_obj(const char *str)
{
	JPS jps;
	JSON_OBJ *root;

	jps.buf = jps.left = str;
	root = alloc_json_obj();
	if (!root)
		return NULL;
	if(json_obj(root, &jps)) {
		fprintf(stderr, "Parse error near: \"%20.20s\"\n", jps.left);
		json_free_obj(root);
		return NULL;
	}

	return root;
}

static void iprintf(int indent, const char *fmt, ...)
{
	va_list ap;

	while (indent-- > 0)
		printf(" ");

	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
}

void json_print_array(JSON_ARRAY *ja, int indent)
{
	int i;

	iprintf(indent, "Array:\n");
	indent += 6;
	for (i = 0; i < ja->size; ++i) {
		switch (ja->type) {
		case Val_Null:
			iprintf(indent, "Null\n");
			break;
		case Val_Floating:
			iprintf(indent, "%lf\n", ja->values[i].floating);
			break;
		case Val_Integer:
			iprintf(indent, "%lld\n", ja->values[i].integer);
			break;
		case Val_String:
			iprintf(indent, "\"%s\"\n", ja->values[i].string);
			break;
		case Val_Boolean:
			if (ja->values[i].boolean)
				iprintf(indent, "True\n");
			else
				iprintf(indent, "False\n");
			break;
		case Val_Array:
			json_print_array(ja->values[i].array, indent);
			break;
		case Val_Object:
			json_print_obj(ja->values[i].object, indent);
			break;
		}
	}
	indent -= 6;
}

void json_print_obj(JSON_OBJ *jo, int indent)
{
	int i;

	iprintf(indent, "Object:\n");
	indent += 7;
	for (i = 0; i < jo->kvnr; ++i) {
		iprintf(indent, "%s -> ", jo->kv[i].key);
		switch (jo->kv[i].type) {
		case Val_Null:
			printf("Null\n");
			break;
		case Val_Floating:
			printf("%lf\n", jo->kv[i].value.floating);
			break;
		case Val_Integer:
			printf("%lld\n", jo->kv[i].value.integer);
			break;
		case Val_String:
			printf("\"%s\"\n", jo->kv[i].value.string);
			break;
		case Val_Boolean:
			if (jo->kv[i].value.boolean)
				printf("True\n");
			else
				printf("False\n");
			break;
		case Val_Array:
			indent += strlen(jo->kv[i].key) + 4;
			printf("\n");
			json_print_array(jo->kv[i].value.array, indent);
			indent -= strlen(jo->kv[i].key) + 4;
			break;
		case Val_Object:
			indent += strlen(jo->kv[i].key) + 4;
			printf("\n");
			json_print_obj(jo->kv[i].value.object, indent);
			indent -= strlen(jo->kv[i].key) + 4;
			break;
		}
	}
	indent -= 7;
}

static const JSON_KV *json_get_kv(const JSON_OBJ *jo, const char *key)
{
	const JSON_HASH *hp;

	hp = jo->keyhash + json_hashkey(key);
	do {
		if (hp->kvptr && !strcmp(hp->kvptr->key, key))
			return hp->kvptr;
		hp = hp->next;
	} while (hp);

	return NULL;
}

int json_get_integer(const JSON_OBJ *jo, long long int *val, const char *key)
{
	const JSON_KV *jkvp = json_get_kv(jo, key);
	if (!jkvp)
		return -1;

	if (jkvp->type != Val_Integer)
		return -1;

	*val = jkvp->value.integer;
	return 0;
}

int json_get_floating(const JSON_OBJ *jo, double *val, const char *key)
{
	const JSON_KV *jkvp = json_get_kv(jo, key);
	if (!jkvp)
		return -1;

	if (jkvp->type != Val_Floating)
		return -1;

	*val = jkvp->value.floating;
	return 0;
}

int json_get_string(const JSON_OBJ *jo, const char **val, const char *key)
{
	const JSON_KV *jkvp = json_get_kv(jo, key);
	if (!jkvp)
		return -1;

	if (jkvp->type != Val_String)
		return -1;

	*val = jkvp->value.string;
	return 0;
}

int json_get_boolean(const JSON_OBJ *jo, int *val, const char *key)
{
	const JSON_KV *jkvp = json_get_kv(jo, key);
	if (!jkvp)
		return -1;

	if (jkvp->type != Val_Boolean)
		return -1;

	*val = jkvp->value.boolean;
	return 0;
}

int json_get_array(const JSON_OBJ *jo, const JSON_ARRAY **val, const char *key)
{
	const JSON_KV *jkvp = json_get_kv(jo, key);
	if (!jkvp)
		return -1;

	if (jkvp->type != Val_Array)
		return -1;

	*val = jkvp->value.array;
	return 0;
}

int json_get_object(const JSON_OBJ *jo, const JSON_OBJ **val, const char *key)
{
	const JSON_KV *jkvp = json_get_kv(jo, key);
	if (!jkvp)
		return -1;

	if (jkvp->type != Val_Object)
		return -1;

	*val = jkvp->value.object;
	return 0;
}

int json_array_checktype(const JSON_ARRAY *ja, const char *typestr)
{
	if (!strcmp(typestr, "long long int") && ja->type == Val_Integer)
		return 1;
	if (!strcmp(typestr, "double") && ja->type == Val_Floating)
		return 1;
	if (!strcmp(typestr, "const char *") && ja->type == Val_String)
		return 1;
	if (!strcmp(typestr, "int") && ja->type == Val_Boolean)
		return 1;
	if (!strcmp(typestr, "const JSON_ARRAY *") && ja->type == Val_Array)
		return 1;
	if (!strcmp(typestr, "const JSON_OBJ *") && ja->type == Val_Object)
		return 1;
	return 0;
}

int json_array_get_val(const JSON_ARRAY *ja, void *val, int aidx)
{
	if (aidx >= ja->size)
		return 0;

	switch (ja->type) {
	case Val_Integer:
		{
		long long int *ival = (long long int *)val;
		*ival = ja->values[aidx].integer;
		}
		break;
	case Val_Floating:
		{
		double *fval = (double *)val;
		*fval = ja->values[aidx].floating;
		}
		break;
	case Val_String:
		{
		const char **sval = (const char **)val;
		*sval = ja->values[aidx].string;
		}
		break;
	case Val_Boolean:
		{
		int *bval = (int *)val;
		*bval = ja->values[aidx].boolean;
		}
		break;
	case Val_Array:
		{
		const JSON_ARRAY **aval = (const JSON_ARRAY **)val;
		*aval = ja->values[aidx].array;
		}
		break;
	case Val_Object:
		{
		const JSON_OBJ **oval = (const JSON_OBJ **)val;
		*oval = ja->values[aidx].object;
		}
		break;
	}

	return 1;
}

