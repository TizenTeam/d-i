/*
 * anna's not nearly apt, but for the Debian installer, it will do. 
 *
 * anna is Copyright (C) 2000 by Joey Hess, under the terms of the GPL.
 * Apologetically dedicated to my sister, Anna.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <cdebconf/debconfclient.h>
#include "anna.h"

static int
is_installed(struct package_t *p, struct linkedlist_t *installed)
{
	struct package_t *q;
	struct version_t pv, qv;

	/* If we don't understand the version number, we play safe
	 * and assume we should install it */
	if (p->version == NULL || !di_parse_version(&pv, p->version))
		return 0;
	q = di_pkg_find(installed, p->package);
	if (q == NULL || q->version == NULL || !di_parse_version(&qv, q->version))
		return 0;
	return (di_compare_version(&pv, &qv) <= 0);
}

/*
 * This function takes a linked list of available packages, decides which
 * are worth installing, creates a linked list of those, and returns it.
 *
 * - Don't install packages that are already installed
 * - Ask for which packages with priority below standard to install
 */
struct linkedlist_t *select_packages (struct linkedlist_t *packages) {
	struct list_node *node, *next, *prev = NULL;
	struct package_t *p;
	struct linkedlist_t *status_p, *lowpri_p;
	FILE *fp;

	fp = fopen(STATUS_FILE, "r");
	if (fp == NULL)
		return NULL;
	status_p = di_pkg_parse(fp);
	fclose(fp);

	lowpri_p = (struct linkedlist_t *)malloc(sizeof(struct linkedlist_t));
	lowpri_p->head = lowpri_p->tail = NULL;

	/* Resolve the dependency pointers */
	di_pkg_resolve_deps(packages);

        for (node = packages->head; node != NULL; node = next)
	{
		next = node->next;
		p = (struct package_t *)node->data;
		if (di_pkg_is_virtual(p))
			continue;
		if (is_installed(p, status_p)) {
                        if (prev)
                                prev->next = next;
                        else
                                packages->head = next;
                        continue;
                } else if (p->priority < standard) {
			/* Unlink these packages temporarily */
			if (prev)
				prev->next = next;
			else
				packages->head = next;
			node->next = NULL;
			if (lowpri_p->tail == NULL)
				lowpri_p->head = lowpri_p->tail = node;
			else {
				lowpri_p->tail->next = node;
				lowpri_p->tail = node;
			}
			continue;
		}
                prev = node;
        }
	if (lowpri_p->head != NULL) {
		struct debconfclient *debconf;
		char *choices;
		char *tmp;
		size_t choices_size = 1;

		choices = malloc(choices_size);
		choices[0] = '\0';
		for (node = lowpri_p->head; node != NULL; node = node->next) {
			p = (struct package_t *)node->data;
			choices_size += strlen(p->package) + 2 + strlen(p->description) + 2;
			choices = realloc(choices, choices_size);
			strcat(choices, p->package);
			strcat(choices, ": ");
			strcat(choices, p->description);
			strcat(choices, ", ");
		}
		if (choices_size >= 3)
			choices[choices_size-3] = '\0';
		debconf = debconfclient_new();
		debconf->command(debconf, "FSET", ANNA_CHOOSE_LOWPRI_PACKAGES,
				"seen", "false", NULL);
		debconf->command(debconf, "SUBST", ANNA_CHOOSE_LOWPRI_PACKAGES,
				"CHOICES", choices, NULL);
		debconf->command(debconf, "INPUT medium", ANNA_CHOOSE_LOWPRI_PACKAGES,
				NULL);
		debconf->command(debconf, "GO", NULL);
		free(choices);
		debconf->command(debconf, "GET", ANNA_CHOOSE_LOWPRI_PACKAGES, NULL);
		if (debconf->value != NULL) {
			/* This is probably not the best way to do it,
			 * but I'm feeling lazy. Feel free to improve it. */
			for (node = lowpri_p->head; node != NULL; node = next) {
				next = node->next;
				p = (struct package_t *)node->data;
				asprintf(&tmp, "%s: %s", p->package, p->description);
				if (tmp != NULL && strstr(debconf->value, tmp) != NULL) {
					node->next = packages->head;
					packages->head = node;
				}
				free(tmp);
			}
		}
	}

	/* Finally, pull in needed dependencies */
	return di_pkg_toposort_list(packages);
}

/* Calls udpkg to unpack a package. */
int unpack_package (char *pkgfile) {
	char *command;

	asprintf(&command, "%s %s", DPKG_UNPACK_COMMAND, pkgfile);
	return ! system(command);
}

/* check whether the md5sum of file matches sum.  if they don't,
 * return 0.
 */

int md5sum(char* sum, char *file) {
        int io[2];
        int pid;
        char line[1024];
	/* Trivially true if the Packages file doesn't have md5sum lines */
	if (sum == NULL)
		return 1;
        pipe(io);
        pid = fork();
        if (pid == 0) {
                /* child */
                dup2(io[1],1);
                execl("/usr/bin/md5sum","/usr/bin/md5sum",file,0);
        }
        wait(NULL);
        read(io[0],&line,1023);
        line[1023] = '\0';
        if (strlen(line) < 32) {
                /* not a success, return */
                return 0;
        }
        line[32] = '\0';
        /* line now contains just the md5sum */
        return ! strcmp(line,sum);
}

/*
 * Retrives and unpacks each package in the linked list in turn.
 * 
 * Returns false on failure, and aborts the operation.
 */
int install_packages (struct linkedlist_t *packages) {
	struct list_node *node;
	struct package_t *p;
	char *f, *fp, *dest_file;
	char *emsg;
	int ret = 1;

	for (node = packages->head; node != NULL; node = node->next) {
		p = (struct package_t *)node->data;
		if (p->filename) {
			/*
			 * Come up with a destination filename.. let's use
			 * the filename we're getting, minus any path
			 * component.
			 */
			for(f = fp = p->filename; *fp != 0; fp++)
				if (*fp == '/')
					f = ++fp;
			asprintf(&dest_file, "%s/%s", DOWNLOAD_DIR, f);

			if (! get_package(p, dest_file)) {
				asprintf(&emsg, "anna: error getting %s!\n",
					p->filename);
				di_log(emsg);
				free(emsg);
				ret = 0;
				break;
			} else if (! md5sum(p->md5sum, dest_file)) {
				asprintf(&emsg, "anna: md5sum mismatch on %s!\n",
					p->filename);
				di_log(emsg);
				free(emsg);
				unlink(dest_file);
				ret = 0;
				break;
			} else if (! unpack_package(dest_file)) {
				unlink(dest_file);
				ret = 0;
				break;
			}
			unlink(dest_file);
			free(dest_file);
		}
	}

	cleanup();

	return ret;
}

int main (int argc, char **argv) {
	/* Tell udpkg to shut up. */
	setenv("UDPKG_QUIET", "y", 1);
	
	return ! install_packages(select_packages(get_packages()));
}
