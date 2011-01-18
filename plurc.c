/*
	plurc or gtfo
*/


#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "libplurc.h"
#include "json.h"

#include "config.h"	/* configuration file */

int main(int argc, char **argv)
{
	int rc = 0;
	PLURK *ph;
	ph = plurk_open(PLURK_KEY);
	JSON_OBJ *jo, *jo2;
	long long int friends, favorer;
	const char *privacy;
	double karma;
	const JSON_OBJ *user_info, *plurk;
	const JSON_ARRAY *plurk_array, *favorers;
	const char *plurk_msg, *nickname;

	if (argc==3) {
		if (!strcmp(argv[1], "add")) {
			/*
				example of adding new Plurks
			*/
			printf("You are gonna add new plurk with content:\n %s\n", argv[2]);
			if (plurk_login(ph, PLURK_USERNAME, PLURK_PASSWORD)) {
				fprintf(stderr, "Plurk login error\n");
				rc = -1;
				goto out;
			}
			if (!plurk_add(ph, argv[2], "says")) {
				jo = json_create_obj(ph->body);
				if (jo) {
					json_print_obj(jo, 0);
					json_free_obj(jo);
				}
			} else {
				fprintf(stderr, "Plurk add error\n");
				rc = -1;
				goto out;
			}
			if (!plurk_logout(ph)) {
				jo = json_create_obj(ph->body);
				if (jo) {
					json_print_obj(jo, 0);
					json_free_obj(jo);
				}
			} else {
				fprintf(stderr, "Plurk logout error\n");
				rc = -1;
				goto out;
			}
		} else if (!strcmp(argv[1], "resps_get")) {
			/*
				exeample of fetching responses
			*/
			printf("You wanna plurk responses %s\n", argv[2]);
			if (!plurk_resps_get(ph, argv[2], "0")) {
				jo = json_create_obj(ph->body);
				if (jo) {
					json_print_obj(jo, 0);
					json_free_obj(jo);
				}
			} else {
				fprintf(stderr, "Plurk get response error\n");
				rc = -1;
				goto out;
			}
		} else if (!strcmp(argv[1], "pprofile_get")) {
			/*
				example of fetching public profile
			*/
			printf("You wanna public profile get for %s\n", argv[2]);
			if (!plurk_pprofile_get(ph, argv[2])) {
				jo = json_create_obj(ph->body);
				if (!jo)
					goto out;
				json_print_obj(jo, 0);
				if (!json_get_integer(jo, &friends, "friends_count"))
					printf("Have %lld friends\n", friends);
				if (!json_get_string(jo, &privacy, "privacy"))
					printf("Plurk privacy setting is: \"%s\"\n", privacy);
				if (!json_get_object(jo, &user_info, "user_info") &&
				    !json_get_floating(user_info, &karma, "karma")) {
					printf("Karma is: %lf\n", karma);
				}
				if (json_get_array(jo, &plurk_array, "plurks")) {
					json_free_obj(jo);
					goto out;
				}
				printf("Recent plurks:\n");
				int i;
				json_array_foreach_object(plurk_array, &plurk, i) {
					if (!json_get_string(plurk, &plurk_msg, "content_raw"))
						printf("\t%s\n", plurk_msg);
					if (json_get_array(plurk, &favorers, "favorers")) {
						printf("\n");
						continue;
					}
					int j;
					printf("\tFavorers:");
					json_array_foreach_integer(favorers, &favorer, j) {
						printf(" %lld", favorer);
						if (plurk_pprofile_get_byint(ph, favorer))
							continue;
						jo2 = json_create_obj(ph->body);
						if (!jo2)
							continue;
						if (json_get_object(jo2, &user_info, "user_info"))
							continue;
						if (json_get_string(user_info, &nickname, "nick_name"))
							continue;
						printf("(%s)", nickname);
					}
					printf("\n\n");
				}
				json_free_obj(jo);
			} else {
				fprintf(stderr, "Plurk get response error\n");
				rc = -1;
				goto out;
			}
		}
	} else if (argc==4) {
		if (!strcmp(argv[1], "resps_radd")) {
			/*
				example of adding responses to plurk
			*/
			printf("You wanna add response for plurk %s\n", argv[2]);
			if (plurk_login(ph, PLURK_USERNAME, PLURK_PASSWORD)) {
				fprintf(stderr, "Plurk login error\n");
				rc = -1;
				goto out;
			}

			if (!plurk_resps_radd(ph, argv[2], argv[3], "says")) {
				jo = json_create_obj(ph->body);
				if (jo) {
					json_print_obj(jo, 0);
					json_free_obj(jo);
				}
			} else {
				fprintf(stderr, "Plurk add error\n");
				rc = -1;
				goto out;
			}

			if (!plurk_logout(ph)) {
				jo = json_create_obj(ph->body);
				if (jo) {
					json_print_obj(jo, 0);
					json_free_obj(jo);
				}
			} else {
				fprintf(stderr, "Plurk logout error\n");
				rc = -1;
				goto out;
			}

		}
	} else if (argc==2) {
		if (!strcmp(argv[1], "oprofile_get")) {
			/*
				example of fetching own profile data
			*/
			printf("You wanna your own profile\n");
			if (plurk_login(ph, PLURK_USERNAME, PLURK_PASSWORD)) {
				fprintf(stderr, "Plurk login error\n");
				rc = -1;
				goto out;
			}

			if (!plurk_oprofile_get(ph)) {
				jo = json_create_obj(ph->body);
				if (jo) {
					json_print_obj(jo, 0);
					if (!json_get_integer(jo, &friends, "friends_count"))
						printf("Have %lld friends\n", friends);
					if (!json_get_string(jo, &privacy, "privacy"))
						printf("Plurk privacy setting is: \"%s\"\n", privacy);
					json_free_obj(jo);
				}
			} else {
				fprintf(stderr, "Plurk add error\n");
				rc = -1;
				goto out;
			}

			if (!plurk_logout(ph)) {
				jo = json_create_obj(ph->body);
				if (jo) {
					json_print_obj(jo, 0);
					json_free_obj(jo);
				}
			} else {
				fprintf(stderr, "Plurk logout error\n");
				rc = -1;
				goto out;
			}
		};

	} else {
		printf("Only supported uses now are: \n");
		printf(" %s add message\n", argv[0]);
		printf(" %s resps_get plurk_id\n", argv[0]);
		printf(" %s pprofile_get user_id\n", argv[0]);
		printf(" %s oprofile_get\n", argv[0]);
		printf(" %s resps_radd plurk_id response\n", argv[0]);
	}

out:
	plurk_close(ph);	/* call it to make some cleanups */
	return rc;
}


