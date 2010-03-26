/* libplurc.h */

/* structures */
struct plurk_key_value;
struct plurk_response;

/* internal API */
char *urlencode(char *content);
char *getheader(const char *headers, char *header);
struct plurk_response fetchpage(char *url, const int secure);
void plurc_close(void);
char *get_kv(int counter, char *url_part, struct plurk_key_value plurk_object);
char *plurk_request(struct plurk_key_value *data_to_pass, int data_size, char* base_url);
int set_session(char *tsession);

/* Plurk.com API */
char *plurk_login(char *username, char *password, char *key);
char *plurk_add(char *content, char *key);
char *plurk_logout(char *key);
char *plurk_resps_get(char *plurk_id, char *from_responses, char *key);
char *plurk_oprofile_get(char *key);
char *plurk_pprofile_get(char *user_id, char *key);
char *plurk_resps_radd(char *plurk_id, char *content, char *key);

