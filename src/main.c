/*
 * TCHunt-ng - reveal (possibly) encrypted files stored on a filesystem.
 * Copyright (C) 2016, 2017  CodeWard.org
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <magic.h>
#include <unistd.h>

#include "strolldir.h"
#include "test_magic.h"
#include "test_chidist.h"

#define TCHUNTNG_VERSION "1.0"

static void
usage (const char *p)
{
	fprintf (stdout, "Usage: %s [options] <file>\n\n\
Options:\n\
 -r  recursively traverse a directory\n\
 -v  show version information\n", p);
}

static void
version_info (void)
{
	fprintf (stdout, "TCHunt-ng %s\n", TCHUNTNG_VERSION);
}

#define EXIT_NOTFOUND 2

static int
scan_dir (const char *p, const char *dirname)
{
	stroller_t dir;
	struct stroller_flist_path *path_iter;
	const struct stroller_flist *files;
	struct testmagic *testmagic;
	int exitno, has_file;

	exitno = EXIT_SUCCESS;
	testmagic = NULL;
	has_file = 0;

	if ( strolldir_open (&dir, dirname) != 0 ){
		fprintf (stderr, "%s: %s\n", p, strerror (errno));
		exitno = EXIT_FAILURE;
		goto cleanup;
	}

	testmagic = testmagic_init ();

	if ( testmagic == NULL ){
		fprintf (stderr, "%s: %s\n", p, testmagic_error (testmagic));
		exitno = EXIT_FAILURE;
		goto cleanup;
	}

	do {
		if ( strolldir_scan (&dir) != 0 ){
			fprintf (stderr, "%s: '%s': %s\n", p, strolldir_getdir (&dir), strerror (errno));

			// Terminate if the error is something severe rather than access
			// permissions violation.
			if ( errno != EACCES ){
				exitno = EXIT_FAILURE;
				goto cleanup;
			}
		}

		files = strolldir_getfiles (&dir);

		for ( path_iter = files->head; path_iter != NULL; path_iter = path_iter->next ){

			switch ( testmagic_test (testmagic, path_iter->path) ){
				case -1:
					fprintf (stderr, "%s: '%s': %s\n", p, path_iter->path, testmagic_error (testmagic));
					exitno = EXIT_FAILURE;
					goto cleanup;

				case 0:
					// Read a next file...
					continue;

				case 1:
					// Follow up with other tests...
					break;

				case 2:
					goto test_success;
			}

			switch ( testchidist_x2 (path_iter->path) ){
				case -1:
					fprintf (stderr, "%s: '%s': %s\n", p, path_iter->path, strerror (errno));
					exitno = EXIT_FAILURE;
					goto cleanup;

				case 0:
					// Read a next file...
					continue;

				case 1:
					break;
			}

test_success:
			has_file = 1;
			fprintf (stdout, "%s\n", path_iter->path);
		}

	} while ( strolldir_nextdir (&dir) );

	if ( ! has_file )
		exitno = EXIT_NOTFOUND;

cleanup:
	strolldir_close (&dir);
	testmagic_free (testmagic);

	return exitno;
}

static int
scan_file (const char *p, const char *filename)
{
	struct testmagic *testmagic;
	int exitno;

	exitno = EXIT_SUCCESS;
	testmagic = testmagic_init ();

	if ( testmagic == NULL ){
		fprintf (stderr, "%s: %s\n", p, testmagic_error (testmagic));
		exitno = EXIT_FAILURE;
		goto cleanup;
	}

	switch ( testmagic_test (testmagic, filename) ){
		case -1:
			fprintf (stderr, "%s: '%s': %s\n", p, filename, testmagic_error (testmagic));
			exitno = EXIT_FAILURE;
			goto cleanup;

		case 0:
			exitno = EXIT_NOTFOUND;
			goto cleanup;

		case 1:
			// Follow up with other tests...
			break;

		case 2:
			goto test_success;
	}

	switch ( testchidist_x2 (filename) ){
		case -1:
			fprintf (stderr, "%s: '%s': %s\n", p, filename, strerror (errno));
			exitno = EXIT_FAILURE;
			goto cleanup;

		case 0:
			exitno = EXIT_NOTFOUND;
			goto cleanup;

		case 1:
			break;
	}

test_success:
	fprintf (stdout, "%s\n", filename);

cleanup:
	testmagic_free (testmagic);
	
	return exitno;
}

int
main (int argc, char *argv[])
{
	struct stat file_stat;
	int exitno, c, recursive, is_dir;

	exitno = EXIT_SUCCESS;
	recursive = 0;
	is_dir = 0;

	while ( (c = getopt (argc, argv, "rv")) != -1 ){
		switch ( c ){
			case 'r':
				recursive = 1;
				break;

			case 'v':
				version_info ();
				exitno = EXIT_SUCCESS;
				return exitno;

			case '?':
				exitno = EXIT_FAILURE;
				return exitno;
		}
	}

	if ( (argc - optind) == 0 ){
		usage (argv[0]);
		exitno = EXIT_FAILURE;
		return exitno;
	}

	if ( stat (argv[optind], &file_stat) == -1 ){
		fprintf (stderr, "%s: %s: %s\n", argv[0], argv[optind], strerror (errno));
		exitno = EXIT_FAILURE;
		return exitno;
	}

	switch ( file_stat.st_mode & S_IFMT ){
		case S_IFDIR:
			is_dir = 1;
			break;
	}

	if ( is_dir ){
		if ( recursive ){
			exitno = scan_dir (argv[0], argv[optind]);
		} else {
			fprintf (stderr, "%s: omitting directory '%s'\n", argv[0], argv[optind]);
			exitno = EXIT_FAILURE;
		}
	} else {
		exitno = scan_file (argv[0], argv[optind]);
	}

	return exitno;
}

