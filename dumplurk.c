#include <time.h>
#include <string.h>

#include "libplurc.h"
#include "json.h"
#include "config.h"

static char *monstr[12] = {
	"Jan", "Feb", "Mar",
	"Apr", "May", "Jun",
	"Jul", "Aug", "Sep",
	"Oct", "Nov", "Dec"
};

static void strtotm(struct tm *p_tm, const char *ptimestr)
{
	int m;
	char *ptr, *eptr;

	if (strlen(ptimestr) < 26)
		return;

	for (m = 0; m < 12; ++m) {
		ptr = strstr(ptimestr, monstr[m]);
		if (ptr != NULL) {
			p_tm->tm_mon = m;
			break;
		}
	}

	ptr += 4;
	p_tm->tm_year = strtol(ptr, &eptr, 10) - 1900;
	ptr = eptr + 1;
	p_tm->tm_hour = strtol(ptr, &eptr, 10);
	ptr = eptr + 1;
	p_tm->tm_min = strtol(ptr, &eptr, 10);
	ptr = eptr + 1;
	p_tm->tm_sec = strtol(ptr, &eptr, 10);

	ptr = strstr(ptimestr, ",");
	ptr += 2;
	p_tm->tm_mday = strtol(ptr, &eptr, 10);
}

static void print_timestr(char *timestr, const struct tm *p_tm)
{
	sprintf(timestr, "%4d-%02d-%02dT%02d:%02d:%02d",
			p_tm->tm_year + 1900, p_tm->tm_mon + 1, p_tm->tm_mday,
			p_tm->tm_hour, p_tm->tm_min, p_tm->tm_sec);
}

int main(int argc __attribute__((unused)),
	 char **argv __attribute__((unused)))
{
	int rc = -1;
	int dumpnum = 500;
	int i;
	PLURK *ph;
	JSON_OBJ *jo;
	const JSON_OBJ *plurk;
	const JSON_ARRAY *plurks;
	const char *posted;
	char timestr[32];
	time_t now;
	struct tm tmnow;

	time(&now);
	/* Get new plurks at any timezone */
	now += 86400;
	localtime_r(&now, &tmnow);
	print_timestr(timestr, &tmnow);

	ph = plurk_open(PLURK_KEY);
	if (!ph)
		goto err_out;
	if (plurk_login(ph, PLURK_USERNAME, PLURK_PASSWORD))
		goto err_plurk_close_out;
	while (dumpnum > 0) {
		if (plurk_timeline_getplurks(ph, timestr, 50))
			goto err_plurk_logout_out;
		jo = json_create_obj(ph->body);
		if (!jo)
			goto err_plurk_logout_out;
		if (json_get_array(jo, &plurks, "plurks"))
			goto err_plurk_logout_out;
		dumpnum -= plurks->size;

		json_array_foreach_object(plurks, &plurk, i) {
			if (json_get_string(plurk, &posted, "posted"))
				break;
			strtotm(&tmnow, posted);
			print_timestr(timestr, &tmnow);
		}
		json_print_obj(jo, 0);
		json_free_obj(jo);
	}

	rc = 0;
err_plurk_logout_out:
	plurk_logout(ph);
err_plurk_close_out:
	plurk_close(ph);
err_out:
	return rc;
}


