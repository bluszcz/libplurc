#include <time.h>

#include "libplurc.h"
#include "json.h"
#include "config.h"

int main(int argc __attribute__((unused)),
	 char **argv __attribute__((unused)))
{
	int rc = -1;
	int dumpnum = 500;
	PLURK *ph;
	JSON_OBJ *jo;

	ph = plurk_open(PLURK_KEY);
	if (!ph)
		goto err_out;
	if (plurk_login(ph, PLURK_USERNAME, PLURK_PASSWORD))
		goto err_plurk_close_out;
	while (dumpnum > 0) {
		if (plurk_timeline_getplurks(ph, "2011-01-01T00:00:00", 50))
			goto err_plurk_logout_out;
		jo = json_create_obj(ph->body);
		if (!jo)
			goto err_plurk_logout_out;
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


