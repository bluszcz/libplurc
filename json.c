#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <iconv.h>

#include "json.h"

static char hexvaltbl[] = {
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

static int get_string(JPS *jps)
{
	int slen;
	char *sp, u16buf[2], *inbuf;
	const char *p = jps->left;
	iconv_t u16to8;
	size_t inlen, outlen;

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
					p += 4;
					slen += 3;
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
				++p;
				if (ishexdigit(p[0]) && ishexdigit(p[1]) &&
				    ishexdigit(p[2]) && ishexdigit(p[3])) {
					u16buf[0] = (hexvaltbl[(int)p[2]] << 4) | hexvaltbl[(int)p[3]];
					u16buf[1] = (hexvaltbl[(int)p[0]] << 4) | hexvaltbl[(int)p[1]];
					inbuf = u16buf;
					inlen = 2;
					outlen = 3;
					u16to8 = iconv_open("UTF-8", "UTF-16");
					iconv(u16to8, &inbuf, &inlen, &sp, &outlen);
					iconv_close(u16to8);
					p += 4;
				} else {
					return Tkn_Error;
				}
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

	jo = malloc(sizeof(JSON_OBJ));
	if (!jo)
		return NULL;
	memset(jo, 0, sizeof(JSON_OBJ));
	jo->kv = malloc(DEFAULT_KVNR * sizeof(JSON_KV));
	if (!jo->kv) {
		free(jo);
		return NULL;
	}
	memset(jo->kv, 0, DEFAULT_KVNR * sizeof(JSON_KV));
	jo->akvnr = DEFAULT_KVNR;

	return jo;
}

static JSON_KV *json_obj_next_kv(JSON_OBJ *jo)
{
	if (jo->kvnr == jo->akvnr) {
		jo->akvnr <<= 1;
		jo->kv = realloc(jo->kv, jo->akvnr * sizeof(JSON_KV));
		if (!jo->kv)
			return NULL;
	}
	++jo->kvnr;
	return jo->kv + (jo->kvnr - 1);
}

static JSON_ARRAY *alloc_json_array(void)
{
	JSON_ARRAY *ja;

	ja = malloc(sizeof(JSON_ARRAY));
	if (!ja)
		return NULL;
	memset(ja, 0, sizeof(JSON_ARRAY));
	ja->values = malloc(DEFAULT_ARNR * sizeof(JSON_VAL));
	if (!ja->values) {
		free(ja);
		return NULL;
	}
	memset(ja->values, 0, DEFAULT_ARNR * sizeof(JSON_VAL));
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
		if (token != Tkn_Comma)
			break;
		expect(Tkn_Comma, jps);
	};

	if (expect(Tkn_RSquareBrackets, jps))
		return -1;

	return 0;
}

static JSON_OBJ *json_obj(JSON_OBJ *jo, JPS *jps);
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
		if (json_obj(value->object, jps))
			return 0;
		else
			return -1;
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

static JSON_OBJ *json_kv(JSON_OBJ *jo, JPS *jps)
{
	JSON_KV *jkv;

	/* Get key */
	if (expect(Tkn_String, jps))
		return NULL;
	jkv = json_obj_next_kv(jo);
	if (!jkv)
		return NULL;
	jkv->key = jps->string;
	jps->string = NULL;


	/* Get Separator */
	if (expect(Tkn_Colon, jps))
		return NULL;

	/* Get Value */
	if (json_value(&jkv->type, &jkv->value, jps))
		return NULL;

	return jo;
}

static JSON_OBJ *json_kv_list(JSON_OBJ *jo, JPS *jps)
{
	int token;

	jo = json_kv(jo, jps);
	if (!jo)
		return NULL;

	token = peek_token(jps);
	while (token == Tkn_Comma) {
		expect(Tkn_Comma, jps);
		jo = json_kv(jo, jps);
		if (!jo)
			return NULL;
		token = peek_token(jps);
	}
	if (token == Tkn_Error)
		return NULL;

	return jo;
}

static JSON_OBJ *json_obj(JSON_OBJ *jo, JPS *jps)
{
	if (expect(Tkn_LCurlyBrackets, jps))
		return NULL;
	jo = json_kv_list(jo, jps);
	if (!jo)
		return NULL;
	if (expect(Tkn_RCurlyBrackets, jps))
		return NULL;
	return jo;
}

static void json_free_obj(JSON_OBJ *jo);
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

static void json_free_obj(JSON_OBJ *jo)
{
	int i;

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
	free(jo->kv);
	free(jo);
}

JSON_OBJ *create_json_obj(const char *str)
{
	JPS jps;
	JSON_OBJ *jo, *root;

	jps.buf = jps.left = str;
	root = alloc_json_obj();
	if (!root)
		return NULL;
	jo = json_obj(root, &jps);
	if (!jo) {
		fprintf(stderr, "Parse error near: \"%20.20s\"\n", jps.left);
		json_free_obj(root);
		return NULL;
	}

	return root;
}

void free_json_obj(JSON_OBJ *jo)
{
	if (!jo)
		return;
	json_free_obj(jo);
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

void print_json_array(JSON_ARRAY *ja, int indent)
{
	int i;

	iprintf(indent, "Array:\n");
	indent += 6;
	for (i = 0; i < ja->size; ++i) {
		switch (ja->type) {
		case Val_Null:
			printf("Null\n");
			break;
		case Val_Floating:
			printf("%lf\n", ja->values[i].floating);
			break;
		case Val_Integer:
			printf("%lld\n", ja->values[i].integer);
			break;
		case Val_String:
			printf("\"%s\"\n", ja->values[i].string);
			break;
		case Val_Boolean:
			if (ja->values[i].boolean)
				printf("True\n");
			else
				printf("False\n");
			break;
		case Val_Array:
			printf("\n");
			print_json_array(ja->values[i].array, indent);
			break;
		case Val_Object:
			printf("\n");
			print_json_obj(ja->values[i].object, indent);
			break;
		}
	}
	indent -= 6;
}

void print_json_obj(JSON_OBJ *jo, int indent)
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
			print_json_array(jo->kv[i].value.array, indent);
			indent -= strlen(jo->kv[i].key) + 4;
			break;
		case Val_Object:
			indent += strlen(jo->kv[i].key) + 4;
			printf("\n");
			print_json_obj(jo->kv[i].value.object, indent);
			indent -= strlen(jo->kv[i].key) + 4;
			break;
		}
	}
	indent -= 7;
}

