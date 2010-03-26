/*
	plurc or gtfo
*/


#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "libplurc.h"

#include "config.h"	/* configuration file */

int main(int argc, char **argv)
{
	char *lsession;		/* variable to keep the session */

	if (argc==3)
	{
		if (strcmp(argv[1],"add")==0)
		{	
			/* 
				example of adding new Plurks 
			*/
			printf("You are gonna add new plurk with content:\n %s\n", argv[2]);
			lsession = plurk_login(PLURK_USERNAME, PLURK_PASSWORD, PLURK_KEY);
			set_session(lsession); /* call it to set global session for libplurc */
			free(lsession);
			printf("%s \n", plurk_add(argv[2], PLURK_KEY));

			printf("%s \n", plurk_logout(PLURK_KEY));		/* it's better to logout */
			plurc_close();											/* call it to make some cleanups */
		}
		else if (strcmp(argv[1],"resps_get")==0)
		{
			/* 
				exeample of fetching responses 
			*/
			printf("You wanna plurk responses %s\n", argv[2]);
			printf("%s \n", plurk_resps_get(argv[2], "0", PLURK_KEY));
			plurc_close();	/* call it at the end of using libplurc */
		}
		else if (strcmp(argv[1],"pprofile_get")==0)
		{
			/*
				example of fetching public profile
			*/
			printf("You wanna public profile get for %s\n", argv[2]);
			printf("%s \n", plurk_pprofile_get(argv[2], PLURK_KEY));
			plurc_close();	/* call it at the end of using libplurc */
	
		}
		
	}
	else if (argc==4)
	{
		if (strcmp(argv[1],"resps_radd")==0)
		{
			/*
				example of adding responses to plurk
			*/
			printf("You wanna add response for plurk %s\n", argv[2]);
			lsession = plurk_login(PLURK_USERNAME, PLURK_PASSWORD, PLURK_KEY);
			set_session(lsession); /* call it to set global session for libplurc */
			free(lsession);
			printf("%s \n", plurk_resps_radd(argv[2], argv[3], PLURK_KEY));
			printf("%s \n", plurk_logout(PLURK_KEY));		/* it's better to logout */
			plurc_close();											/* call it to make some cleanups */
		}
	}

	else if (argc==2)
	{
		if (strcmp(argv[1],"oprofile_get")==0)
		{
			/*
				example of fetching own profile data
			*/
			printf("You wanna your own profile\n");
			lsession = plurk_login(PLURK_USERNAME, PLURK_PASSWORD, PLURK_KEY);
			set_session(lsession); /* call it to set global session for libplurc */
			free(lsession);
			printf("%s \n", plurk_oprofile_get(PLURK_KEY));
			printf("%s \n", plurk_logout(PLURK_KEY));		/* it's better to logout */
			plurc_close();											/* call it to make some cleanups */
		};

	}
	else
	{
		printf("Only supported uses now are: \n");
		printf(" %s add message\n", argv[0]);
		printf(" %s resps_get plurk_id\n", argv[0]);
		printf(" %s pprofile_get user_id\n", argv[0]);
		printf(" %s oprofile_get\n", argv[0]);
		printf(" %s resp_add plurk_id response\n", argv[0]);
	};
		return 0;
}


