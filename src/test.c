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
#include <string.h>
#include <errno.h>
#include <utime.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "test_magic.h"
#include "test_entropy.h"
#include "testxcode.h"
#include "test.h"

int
tests_init (struct test_ctl *test_ctl, int flags)
{
	int mod_flgs;

	mod_flgs = TESTMAGIC_FLAGS;

	if ( flags & TESTFLG_RESTOREATIME )
		mod_flgs |= MAGIC_PRESERVE_ATIME;

	test_ctl->flags = flags;

	if ( testmagic_init (&(test_ctl->testmagic_res), mod_flgs) == -1 )
		return TESTX_ERROR;

	return TESTX_SUCCESS;
}

/* Minimal size of a TrueCrypt file. */
#define TCRYPT_SIZE_MIN 19456

int
tests_test_file (struct test_ctl *test_ctl, const char *path, struct stat *fstat)
{
	FILE *file;
	const char *file_class;
	unsigned char buff[TENTROPY_MAXLEN];
	struct utimbuf timebuff;
	size_t buff_len;
	int ret;

	ret = TESTX_SUCCESS;

	if ( test_ctl->flags & TESTFLG_RESTOREATIME ){
		timebuff.actime = fstat->st_atim.tv_sec;
		timebuff.modtime = fstat->st_mtim.tv_sec;
	}

	file = fopen (path, "rb");

	if ( file == NULL ){
		test_ctl->errmsg = strerror (errno);
		ret = TESTX_ERROR;
		goto egress;
	}

	buff_len = fread (buff, 1, sizeof (buff), file);

	if ( ferror (file) ){
		test_ctl->errmsg = strerror (errno);
		ret = TESTX_ERROR;
	}

	fclose (file);

	/* XXX
	 * Make sure this condition always precedes the errflag check below!
	 * This is because fopen(3) modifies atime and we want to make sure it is
	 * set back to its original value before the function terminates in case of
	 * fread(3) error.
	 * XXX
	 */
	if ( test_ctl->flags & TESTFLG_RESTOREATIME )
		utime (path, &timebuff);

	if ( ret == TESTX_ERROR )
		goto egress;

	fprintf (stderr, "read %zu\n", buff_len);

	/* If TCHunt compatibility mode is enabled, check the file's size. */
	if ( test_ctl->flags & TESTFLG_TESTCOMPAT ){
		if ( fstat->st_size < TCRYPT_SIZE_MIN || (fstat->st_size % 512) != 0 ){
			ret = TESTX_ENORESULT;
			goto egress;
		}
	}

	ret = testmagic_test_buffer (&(test_ctl->testmagic_res), buff, buff_len, &file_class);

	switch ( ret ){
		case TESTX_ERROR:
			test_ctl->errmsg = testmagic_error (&(test_ctl->testmagic_res));
			goto egress;

		case TESTX_ENORESULT:
			goto egress;

		case TESTX_SUCCESS:
			goto egress;

		case TESTX_CONTINUE:
			ret = TESTX_SUCCESS;
			break;
	}

	ret = testentropy_x2_buffer (buff, buff_len);

	switch ( ret ){
		case TESTX_ERROR:
			test_ctl->errmsg = strerror (errno);
			goto egress;

		case TESTX_ENORESULT:
			goto egress;

		case TESTX_SUCCESS:
			goto egress;

		case TESTX_CONTINUE:
			ret = TESTX_SUCCESS;
			break;
	}

egress:
	return ret;
}

void
tests_free (struct test_ctl *test_ctl)
{
	if ( test_ctl == NULL )
		return;

	testmagic_free (&(test_ctl->testmagic_res));
}

