/*
	plurc or gtfo
*/

#include <ctype.h>

/* OpenSSL headers */

#include "openssl/bio.h"
#include "openssl/ssl.h"
#include "openssl/err.h"

/* OpenSSL cert */

#define SSL_CERT_FILE SSL_CERT_PATH"/Equifax_Secure_Plurc_CA.crt"

/* libplurc api commands */

#include "api.h"

/* globals */

static char reply[RESPONSE_SIZE];
static char session[SESSION_SIZE];
static unsigned int sestate=0;						/* if session has been initialized */

static unsigned int sstate=0;   				/* if SSL has been initialized */
static SSL_CTX * ctx = NULL;
static BIO * bio = NULL;

/* this one is used to build GET part with parameters */
struct plurk_key_value
{
	char key[129];
	char value[513];
};

/* this one represents HTTP Response - splitted for headers and body */
struct plurk_response
{
	char *headers;
	char *body;
};

char *urlencode(char *content)
{
	/* simple urlencode implementation TODO: make it better */
	char *ptr = content;
	char *encoded = malloc(421);
	char *penc = encoded;

	while (*ptr)
	{
		if (*ptr==' ')
			*penc++ = '+';
		else if  (*ptr=='/')
		{
			memcpy(penc, "%2f", strlen("%2f"));
			penc+=3;
		}
		else if (*ptr=='#')
		{
			memcpy(penc, "%2f", strlen("%23"));
			penc+=3;
		}
		else
			*penc++ = *ptr;		
		ptr++;
	};
	*penc = '\0';
	
	return encoded;
};

char *getheader(const char *headers, char *header)
{
	char *value = NULL;
	value = malloc(SESSION_SIZE);
	memset(value, '\0', SESSION_SIZE);

	unsigned int c;				/* counters */
	unsigned int d;				/* counters */
	unsigned int isizes = strlen(headers);
	unsigned int isize = strlen(header);
	for (c=0;c<isizes;c++)
	{
		if (tolower(headers[c])==tolower(header[0]))
		{	
			c++;
			for(d=1;(d<isize && c <= isizes);d++) /* checks if c is not bigger  */
																						/* than whole headers strem   */
			{
				if (tolower(headers[c])!=tolower(header[d]))
				{
					break;
				};
				c++;			
			};
			if (d==isize) 
			{	
				c += 2; 		/* skip ': ' */
				d = 0; 			/* reset counter */
			  while((c<isizes) && (headers[c] != 10))
				{
					memset(&value[d], headers[c], sizeof(headers[c]));
					c++;
					d++;
				};
				return value;
			};
		};
	};
	return value;
};


struct plurk_response fetchpage(char *url, const unsigned int secure)
{
	/*
		url - taken from function plurk_request
		secure - (0, plain),(1,over ssl)
	*/
	ssize_t bytessent;
	unsigned int rsize =0;
	unsigned int check_header = 1;
	struct plurk_response response;
	char *eoh = NULL;											/* end of headers */
	char buf[BUFRECVSIZE];
  memset(&buf, '\0', BUFRECVSIZE);

	if (secure)
	{
		SSL_library_init();
		SSL_load_error_strings();
		if (!sstate)
		{
			ctx = SSL_CTX_new(SSLv23_client_method());
			sstate = 1;
		};

		if (ctx==NULL)
		{
			ERR_print_errors_fp(stderr);
			exit(EXIT_FAILURE);
		};

		SSL * ssl=NULL;

		if(! SSL_CTX_load_verify_locations(ctx, "/etc/ssl/certs/Certum_Root_CA.pem", NULL))
		{
			ERR_print_errors_fp(stderr);
			exit(EXIT_FAILURE);
		}

		bio = BIO_new_ssl_connect(ctx);
		BIO_get_ssl(bio, & ssl);
		SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);

		BIO_set_conn_hostname(bio, "www.plurk.com:443");
	} 
	else
	{
		bio = BIO_new_connect("www.plurk.com:80");
	};


	if(bio == NULL)
	{
			/* Handle the failure */
			ERR_print_errors_fp(stderr);
			exit(EXIT_FAILURE);
	}

	if(BIO_do_connect(bio) <= 0)
	{
		/* Handle failed connection */
		ERR_print_errors_fp(stderr);
		exit(EXIT_FAILURE);
	}

	bytessent = BIO_write(bio, url, strlen(url));

	while((rsize = BIO_read(bio, buf, BUFRECVSIZE-1)) && (rsize!=0))
	{
		strncat(reply, buf, rsize);
	};

	eoh = strstr(reply, "\r\n\r\n");
	
	if ( eoh !=NULL)
	{
		check_header = 0;
		*eoh = '\0';
		response.headers = reply;
		response.body = eoh + 4;
	} 
	else
	{
		response.headers = "";
		response.body = "";
	};

	BIO_free_all(bio);
	return response;
}	

/* this functions must be called before exit */
void plurc_close()
{
	if (sstate)
	{
		SSL_CTX_free(ctx);
		sstate = 0;
	};
}


char *get_kv(int counter, char *url_part, struct plurk_key_value plurk_object)
{
	/*
		This function helps building proper url for request
	*/
	if (counter==0) 
	{
		strncpy(&url_part[strlen(url_part)], "?", sizeof("?"));
	} 
	else
	{
		strncpy(&url_part[strlen(url_part)], "&", sizeof("&"));
	};	
	strncpy(&url_part[strlen(url_part)], plurk_object.key, sizeof(plurk_object.key));
	strncpy(&url_part[strlen(url_part)], "=", sizeof("="));
	strncpy(&url_part[strlen(url_part)], plurk_object.value, sizeof(plurk_object.value));
	return url_part;
};

int set_session(char *tsession)
{
	if (!sestate)
	{
		strncpy(session, tsession, strlen(tsession));
		sestate = 1;
		return 1;
	}
	return 0;
}

char *plurk_request(struct plurk_key_value *data_to_pass, unsigned int data_size, char* base_url)
{	
	/*
		This function generates whole request which is send to server
		url_part is used as request in function fetchpage
	*/
	
	unsigned int counter;
	struct plurk_key_value plurk_object;
	static char url_part[REQUEST_SIZE];
	static char url[REQUEST_SIZE]; /* needed bigger size */
	memcpy(url_part, "\0", sizeof("\0"));

	/* iteration over plurk objects */
	for (counter=0;counter<data_size;counter++) 
	{
		plurk_object = data_to_pass[counter];
		get_kv(counter, url_part, plurk_object);
	};

  snprintf(url, REQUEST_SIZE, base_url, url_part, PLURK_HOST, session);
	return url;	
};

char *plurk_login(char *username, char *password, char *key)
{
	/* returns session */
	struct plurk_key_value plurk_datas[3];
	struct plurk_response response;
	char *url_part;

	strncpy(plurk_datas[0].key, "api_key", sizeof("api_key"));
	strncpy(plurk_datas[0].value, key, strlen(key)+1);
	strncpy(plurk_datas[1].key, "username", sizeof("username"));
	strncpy(plurk_datas[1].value,username, strlen(username)+1);
	strncpy(plurk_datas[2].key,"password", sizeof("password"));
	strncpy(plurk_datas[2].value,password,strlen(password)+1);

	url_part = plurk_request(plurk_datas, 3, HTTP_REQUEST_LOGIN);
	response = fetchpage(url_part, 1);

	return(getheader(response.headers, "set-cookie"));
	
};

char *plurk_logout(char *key)
{
	
	struct plurk_key_value plurk_datas[1];
	struct plurk_response response;
	char *url_part;	

	strncpy(plurk_datas[0].key, "api_key", sizeof("api_key"));
	strncpy(plurk_datas[0].value, key, strlen(key)+1);

	url_part = plurk_request(plurk_datas, 1, HTTP_REQUEST_LOGOUT);
	response = fetchpage(url_part, 0);

	return response.body;
};


char *plurk_add(char *content, char *key)
{
	
	struct plurk_key_value plurk_datas[3];
	struct plurk_response response;
	char *url_part;	
	char *econtent	= urlencode(content);

	strncpy(plurk_datas[0].key, "api_key", sizeof("api_key"));
	strncpy(plurk_datas[0].value, key, strlen(key)+1);
	strncpy(plurk_datas[1].key, "content", sizeof("content"));
	strncpy(plurk_datas[1].value, econtent, strlen(econtent)+1);
	strncpy(plurk_datas[2].key, "qualifier", sizeof("qualifier"));
	strncpy(plurk_datas[2].value, ":", sizeof(":"));

	url_part = plurk_request(plurk_datas, 3, HTTP_REQUEST_ADD);
	response = fetchpage(url_part, 0);
	free(econtent);

	return response.body;
};

char *plurk_resps_get(char *plurk_id, char *from_response, char *key)
{
	
	struct plurk_key_value plurk_datas[1];
	struct plurk_response response;
	char *url_part;	

	strncpy(plurk_datas[0].key, "api_key", sizeof("api_key"));
	strncpy(plurk_datas[0].value, key, strlen(key)+1);
	strncpy(plurk_datas[1].key, "plurk_id", sizeof("plurk_id"));
	strncpy(plurk_datas[1].value, plurk_id, strlen(plurk_id)+1);
	strncpy(plurk_datas[2].key, "from_response", sizeof("from_response"));
	strncpy(plurk_datas[2].value, from_response, strlen(from_response)+1);

	url_part = plurk_request(plurk_datas, 3, HTTP_REQUEST_RESPS_GET);
	response = fetchpage(url_part, 0);

	return response.body;
}

char *plurk_oprofile_get(char *key)
{
	struct plurk_key_value plurk_datas[1];
	struct plurk_response response;
	char *url_part;	

	strncpy(plurk_datas[0].key, "api_key", sizeof("api_key"));
	strncpy(plurk_datas[0].value, key, strlen(key)+1);

	url_part = plurk_request(plurk_datas, 1, HTTP_REQUEST_OPROFILE_GET);
	response = fetchpage(url_part, 0);
	return response.body;
}

char *plurk_pprofile_get(char *user_id, char *key)
{
	struct plurk_key_value plurk_datas[2];
	struct plurk_response response;
	char *url_part;	

	strncpy(plurk_datas[0].key, "api_key", sizeof("api_key"));
	strncpy(plurk_datas[0].value, key, strlen(key)+1);
	strncpy(plurk_datas[1].key, "user_id", sizeof("user_id"));
	strncpy(plurk_datas[1].value, user_id, strlen(user_id)+1);

	url_part = plurk_request(plurk_datas, 2, HTTP_REQUEST_PPROFILE_GET);
	response = fetchpage(url_part, 0);
	return response.body;

}

char *plurk_resps_radd(char *plurk_id, char *content, char *key)
{
	
	struct plurk_key_value plurk_datas[4];
	struct plurk_response response;
	char *url_part;	
	char *econtent	= urlencode(content);

	strncpy(plurk_datas[0].key, "api_key", sizeof("api_key"));
	strncpy(plurk_datas[0].value, key, strlen(key)+1);
	strncpy(plurk_datas[1].key, "content", sizeof("content"));
	strncpy(plurk_datas[1].value, econtent, strlen(econtent)+1);
	strncpy(plurk_datas[2].key, "qualifier", sizeof("qualifier"));
	strncpy(plurk_datas[2].value, ":", sizeof(":"));
	strncpy(plurk_datas[3].key, "plurk_id", sizeof("plurk_id"));
	strncpy(plurk_datas[3].value, plurk_id, strlen(plurk_id)+1);

	url_part = plurk_request(plurk_datas, 4, HTTP_REQUEST_RESPS_RADD);
	response = fetchpage(url_part, 0);
	free(econtent);

	return response.body;
};

