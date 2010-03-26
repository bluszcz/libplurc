#define PLURK_HOST "www.plurk.com"
#define PLURK_PORT "80"
#define HTTP_REQUEST_AUTH	"GET /admin/ HTTP/1.0\r\n\r\n"
#define HTTP_REQUEST_INIT "GET /config.h HTTP/1.0\r\n\r\n"
#define HTTP_REQUEST_FINISH "GET / HTTP/1.0\r\n\r\n"

#define HTTP_REQUEST_LOGIN "GET /API/Users/login%s HTTP/1.0\r\nHost: %s\r\nCookie: %s\r\n\r\n"
#define HTTP_REQUEST_LOGOUT "GET /API/Users/logout%s HTTP/1.0\r\nHost: %s\r\nCookie: %s\r\n\r\n"
#define HTTP_REQUEST_ADD "GET /API/Timeline/plurkAdd%s HTTP/1.0\r\nHost: %s\r\nCookie: %s\r\n\r\n"
#define HTTP_REQUEST_RESPS_GET "GET /API/Responses/get%s HTTP/1.0\r\nHost: %s\r\nCookie: %s\r\n\r\n"
#define HTTP_REQUEST_RESPS_RADD "GET /API/Responses/responseAdd%s HTTP/1.0\r\nHost: %s\r\nCookie: %s\r\n\r\n"
#define HTTP_REQUEST_OPROFILE_GET "GET /API/Profile/getOwnProfile%s HTTP/1.0\r\nHost: %s\r\nCookie: %s\r\n\r\n"
#define HTTP_REQUEST_PPROFILE_GET "GET /API/Profile/getPublicProfile%s HTTP/1.0\r\nHost: %s\r\nCookie: %s\r\n\r\n"


#define BUFRECVSIZE 1024
#define REQUEST_SIZE 1024
#define SESSION_SIZE 512
#define RESPONSE_SIZE 40960
