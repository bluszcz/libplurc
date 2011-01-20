#include <ctype.h>
#include <string.h>
#include <strings.h>

/* libplurc api commands */
#include "libplurc.h"
#include "api.h"

/* OpenSSL cert */
#define SSL_CERT_FILE SSL_CERT_PATH"/Equifax_Secure_Plurc_CA.crt"

static char radix16[16] =
{
	'0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
};

static int urlencode(char *encoded, const char *content)
{
	const char *ptr = content;
	char *penc = encoded;
	int cc;

	for( ; *ptr ; ++ptr) {
		if(isalnum(*ptr) ||
			*ptr == '.' || *ptr == '_')
			*penc++ = *ptr;
		else {
			cc = *ptr;
			*penc++ = '%';
			*penc++ = radix16[(cc >> 4) & 0xF];
			*penc++ = radix16[(cc & 0xF)];
		}
	}
	*penc = '\0';

	return penc - encoded;
}

static int getheader(const char *headers, const char *header, char *buffer)
{
	const char *ptr = headers, *eptr;
	int len;

	*buffer = '\0';
	while ((ptr = strstr(ptr, "\r\n")) != NULL) {
		ptr += 2;
		if (!strncasecmp(ptr, header, strlen(header))) {
			ptr += strlen(header);
			if (strncmp(ptr, ": ", 2))
				continue;
			ptr += 2;
			eptr = strstr(ptr, "\r\n");
			if (eptr)
				len = eptr - ptr;
			else
				len = strlen(ptr);
			memcpy(buffer, ptr, len);
			buffer[len] = '\0';
			return 0;
		}
	}
	return -1;
}

static void fetchpage(PLURK *ph, const char *url, int secure)
{
	/*
		url - taken from function plurk_request
		secure - (0, plain),(1,over ssl)
	*/
	ssize_t total;
	int rc;
	unsigned long offset;
	char *eoh;		/* end of headers */

	ph->recvbuf[0] = '\0';
	if (secure)
	{
		SSL_library_init();
		SSL_load_error_strings();
		if (!ph->sstate)
		{
			ph->ctx = SSL_CTX_new(SSLv23_client_method());
			ph->sstate = 1;
		}

		if (!ph->ctx)
		{
			ERR_print_errors_fp(stderr);
			exit(EXIT_FAILURE);
		}

		SSL * ssl = NULL;

		if (!SSL_CTX_load_verify_locations(ph->ctx, "/etc/ssl/certs/Certum_Root_CA.pem", NULL))
		{
			ERR_print_errors_fp(stderr);
			exit(EXIT_FAILURE);
		}

		ph->bio = BIO_new_ssl_connect(ph->ctx);
		BIO_get_ssl(ph->bio, &ssl);
		SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);

		BIO_set_conn_hostname(ph->bio, PLURK_HOST ":" SSL_PORT);
	} else {
		ph->bio = BIO_new_connect(PLURK_HOST ":" HTTP_PORT);
	}

	if(ph->bio == NULL) {
		/* Handle the failure */
		ERR_print_errors_fp(stderr);
		exit(EXIT_FAILURE);
	}

	if(BIO_do_connect(ph->bio) <= 0) {
		/* Handle failed connection */
		ERR_print_errors_fp(stderr);
		exit(EXIT_FAILURE);
	}


	total = strlen(url);
	offset = 0;
	while (total > 0) {
		rc = BIO_write(ph->bio, url + offset, total);
		if (rc > 0) {
			total -= rc;
			offset += rc;
		} else {
			fprintf(stderr, "BIO_write error\n");
			exit(EXIT_FAILURE);
		}
	}

	offset = 0;
	while((rc = BIO_read(ph->bio, ph->recvbuf, BUFRECVSIZE-1)) && (rc > 0)) {
		memcpy(ph->reply + offset, ph->recvbuf, rc);
		offset += rc;
	}
	ph->reply[offset] = '\0';

	eoh = strstr(ph->reply, "\r\n\r\n");
	if ( eoh != NULL ) {
		*eoh = '\0';
		ph->header = ph->reply;
		ph->body = eoh + 4;
	} else {
		ph->header = NULL;
		ph->body = NULL;
	}

	BIO_free_all(ph->bio);
}

static char *plurk_encode_kv(char *buf, const char *key, const char *value)
{
	int rc;

	rc = urlencode(buf, key);
	buf += rc;
	memcpy(buf, "=", 2);
	++buf;
	rc = urlencode(buf, value);
	buf += rc;

	return buf;
}

static int plurk_request(PLURK *ph, const char *base_url, int is_ssl,
			 int argn, ...)
{
	char *ptr = ph->url, *key, *value;
	va_list ap;
	int i;

	/* Build first line of request */
	sprintf(ptr, "GET %s", base_url);
	ptr += strlen(ptr);
	va_start(ap, argn);
	for (i = 0; i < argn; ++i) {
		if (i == 0)
			memcpy(ptr, "?", 2);
		else
			memcpy(ptr, "&", 2);
		++ptr;
		key = va_arg(ap, char *);
		value = va_arg(ap, char *);
		ptr = plurk_encode_kv(ptr, key, value);
	}
	va_end(ap);
	sprintf(ptr, " HTTP/1.0\r\n");
	ptr += strlen(ptr);

	/* Add HTTP host header */
	sprintf(ptr, "Host: %s\r\n", PLURK_HOST);
	ptr += strlen(ptr);

	/* Add Cookie header if exist */
	if (ph->sestate) {
		sprintf(ptr, "Cookie: %s\r\n", ph->session);
		ptr += strlen(ptr);
	}

	/* Finalize header */
	sprintf(ptr, "\r\n");

	fetchpage(ph, ph->url, is_ssl);
	return 0;
}

/*
 * Public APIs
 */
/* Allocate buffers for an API session */
PLURK *plurk_open(const char *apikey)
{
	PLURK *ph;

	ph = malloc(sizeof(PLURK));
	if (ph) {
		memset(ph, 0, sizeof(PLURK));
		strcpy(ph->apikey, apikey);
	}

	return ph;
}

/* this functions must be called in pair of each plurk_open */
void plurk_close(PLURK *ph)
{
	if (!ph)
		return;

	if (ph->sstate)
	{
		SSL_CTX_free(ph->ctx);
		ph->sstate = 0;
	};
	free(ph);
}

int plurk_login(PLURK *ph, const char *username, const char *password)
{
	int rc;

	rc = plurk_request(ph, "/API/Users/login", IS_SSL, 3,
			"api_key", ph->apikey,
			"username", username,
			"password", password);
	if (rc)
		return rc;

	rc = getheader(ph->header, "set-cookie", ph->session);
	if (!rc)
		ph->sestate = 1;
	return rc;
}

int plurk_logout(PLURK *ph)
{
	int rc;

	if (!ph->sestate)
		return -1;

	rc = plurk_request(ph, "/API/Users/logout", NOT_SSL, 1,
				"api_key", ph->apikey);
	if (!rc)
		ph->sestate = 0;
	return rc;
}

int plurk_add(PLURK *ph, const char *content, const char *qualifier)
{
	int rc;

	if (!ph->sestate)
		return -1;

	rc = plurk_request(ph, "/API/Timeline/plurkAdd", NOT_SSL, 3,
			"api_key", ph->apikey,
			"content", content,
			"qualifier", qualifier);
	return rc;
}

int plurk_oprofile_get(PLURK *ph)
{
	int rc;

	if (!ph->sestate)
		return -1;

	rc = plurk_request(ph, "/API/Profile/getOwnProfile", NOT_SSL, 1,
			"api_key", ph->apikey);
	return rc;
}

int plurk_pprofile_get(PLURK *ph, const char *user_id)
{
	int rc;

	rc = plurk_request(ph, "/API/Profile/getPublicProfile", NOT_SSL, 2,
			"api_key", ph->apikey,
			"user_id", user_id);
	return rc;
}

int plurk_pprofile_get_byint(PLURK *ph, long long int user_id)
{
	int rc;
	char buf[20];

	sprintf(buf, "%lld", user_id);
	rc = plurk_request(ph, "/API/Profile/getPublicProfile", NOT_SSL, 2,
			"api_key", ph->apikey,
			"user_id", buf);
	return rc;
}

int plurk_resps_get(PLURK *ph, const char *plurk_id, const char *from_response)
{
	int rc;

	rc = plurk_request(ph, "/API/Responses/get", NOT_SSL, 3,
			"api_key", ph->apikey,
			"plurk_id", plurk_id,
			"from_response", from_response);
	return rc;
}

int plurk_resps_radd(PLURK *ph, const char *plurk_id,
			const char *content, const char *qualifier)
{
	int rc;

	if (!ph->sestate)
		return -1;

	rc = plurk_request(ph, "/API/Responses/responseAdd", NOT_SSL, 4,
			"api_key", ph->apikey,
			"plurk_id", plurk_id,
			"content", content,
			"qualifier", qualifier);
	return rc;
}

int plurk_polling_getplurks(PLURK *ph, const char *offset, int limit)
{
	int rc;
	char numstr[20];

	if (!ph->sestate)
		return -1;

	sprintf(numstr, "%d", limit);
	rc = plurk_request(ph, "/API/Polling/getPlurks", NOT_SSL, 3,
			"api_key", ph->apikey,
			"offset", offset,
			"limit", numstr);
	return rc;
}

int plurk_timeline_getplurks(PLURK *ph, const char *offset, int limit)
{
	int rc;
	char numstr[20];

	if (!ph->sestate)
		return -1;

	sprintf(numstr, "%d", limit);
	rc = plurk_request(ph, "/API/Timeline/getPlurks", NOT_SSL, 3,
			"api_key", ph->apikey,
			"offset", offset,
			"limit", numstr);
	return rc;
}

