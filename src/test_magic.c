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
#if 0
#include <stdio.h> // FIXME
#endif
#include <stdlib.h>
#include <string.h>
#include <magic.h>
#include <errno.h>

#include "test_magic.h"

#define MAGIC_FLAGS (MAGIC_NO_CHECK_APPTYPE | MAGIC_SYMLINK | MAGIC_NO_CHECK_COMPRESS | MAGIC_NO_CHECK_ELF | MAGIC_NO_CHECK_FORTRAN | MAGIC_NO_CHECK_TAR | MAGIC_NO_CHECK_TOKENS | MAGIC_NO_CHECK_TROFF)

struct testmagic*
testmagic_init (void)
{
	struct testmagic *new_test;

	new_test = calloc (1, sizeof (struct testmagic));

	if ( new_test == NULL )
		return NULL;

	new_test->magic_res = magic_open (MAGIC_FLAGS);

	if ( new_test->magic_res == NULL ){
		free (new_test);
		return NULL;
	}

	if ( magic_load (new_test->magic_res, NULL) == -1 ){
		free (new_test);
		magic_close (new_test->magic_res);
		return NULL;
	}

	return new_test;
}

int
testmagic_test (struct testmagic *testmagic, const char *file)
{
	const char *ftype;
	int match;

	ftype = magic_file (testmagic->magic_res, file);

	if ( ftype == NULL )
		return -1;

#if 0
	fprintf (stderr, "%s : %s\n", file, ftype);
#endif

	match = 0;

	if ( strcmp (ftype, "data") == 0 )
		match = 1;
	else if ( strncmp (ftype, "PGP", 3) == 0 )
		match = 2;
	else if ( strncmp (ftype, "GPG", 3) == 0 )
		match = 2;

	return match;
}

void
testmagic_free (struct testmagic *testmagic)
{
	if ( testmagic == NULL )
		return;

	magic_close (testmagic->magic_res);
	free (testmagic);
}

const char*
testmagic_error (struct testmagic *testmagic)
{
	if ( errno != 0 )
		return strerror (errno);

	return magic_error (testmagic->magic_res);
}

