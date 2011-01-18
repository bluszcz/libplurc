/* OpenSSL headers */
#include "openssl/bio.h"
#include "openssl/ssl.h"
#include "openssl/err.h"

/* Size definitions */
#define BUFRECVSIZE 1024
#define RESPONSE_SIZE 40960
#define REQUEST_SIZE 1024
#define SESSION_SIZE 512
#define MAX_MESSAGE_LENGTH 140
#define API_KEY_LENGTH 64

/* structures */
typedef struct plurk_struct {
	char recvbuf[BUFRECVSIZE];
	char reply[RESPONSE_SIZE];
	char *header, *body;
	char url[REQUEST_SIZE];
	char session[SESSION_SIZE];
	char encmsg[MAX_MESSAGE_LENGTH * 3];
	char apikey[API_KEY_LENGTH];
	unsigned int sestate;		/* if session has been initialized */

	unsigned int sstate;   		/* if SSL has been initialized */
	SSL_CTX *ctx;
	BIO *bio;
} PLURK;

/* Public Plurk APIs */
PLURK *plurk_open(const char *key);
void plurk_close(PLURK *ph);
int plurk_login(PLURK *ph, const char *username, const char *password);
int plurk_logout(PLURK *ph);
int plurk_add(PLURK *ph, const char *content, const char *qualifier);
int plurk_resps_get(PLURK *ph, const char *plurk_id, const char *from_responses);
int plurk_oprofile_get(PLURK *ph);
int plurk_pprofile_get(PLURK *ph, const char *user_id);
int plurk_pprofile_get_byint(PLURK *ph, long long int user_id);
int plurk_resps_radd(PLURK *ph, const char *plurk_id, const char *content, const char *qualifier);

