/* xcrun - clone of apple's xcode xcrun utility
 *
 * Copyright (c) 2013, Brian McKenzie <mckenzba@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *
 *  3. Neither the name of the organization nor the names of its contributors may
 *     be used to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#define TOOL_VERSION "0.0.1"
#define DARWINSDK_CFG ".darwinsdk.dat"

/**
 * @func usage -- Print helpful information about this program.
 * @prog -- name of this program
 */
static void usage(char *prog)
{
	fprintf(stderr, "Usage: %s <program>\n", prog);
	exit(1);
}

/**
 * @func get_sdk_path -- retrieve current sdk path
 * @return: string of current path on success, NULL string on failure
 */
static char *get_sdk_path(void)
{
	FILE *fp = NULL;
	char devpath[PATH_MAX - 1];
	char *pathtocfg = NULL;
	char *darwincfg_path = NULL;
	char *value = NULL;

	if ((value = getenv("DEVELOPER_DIR")) != NULL)
		return value;

	memset(devpath, 0, sizeof(devpath));

	if ((pathtocfg = getenv("HOME")) == NULL) {
		fprintf(stderr, "xcode-select: error: failed to read HOME variable.\n");
		return NULL;
	}

	darwincfg_path = (char *)malloc((strlen(pathtocfg) + sizeof(DARWINSDK_CFG)));

	strcat(pathtocfg, "/");
	strcat(darwincfg_path, strcat(pathtocfg, DARWINSDK_CFG));

	if ((fp = fopen(darwincfg_path, "r")) != NULL) {
		fseek(fp, SEEK_SET, 0);
		fread(devpath, (PATH_MAX), 1, fp);
		value = devpath;
		fclose(fp);
	} else {
		fprintf(stderr, "xcode-select: error: unable to read configuration file. (errno=%s)\n", strerror(errno));
		return NULL;
	}

	free(darwincfg_path);

	return value;
}

/**
 * @func call_command - Execute new process to replace this one.
 * @arg cmd -- program's absolute path
 * @arg argv -- arguments to be passed to new process
 * @return: -1 on error, otherwise no return
 */
static int call_command(const char *cmd, char *argv[])
{
	return execv(cmd, argv);
}

/**
 * @func find_command - Search for a program.
 * @arg name -- name of program
 * @arg argv -- arguments to be passed if program found
 * @return: NULL on failed search or execution
 */
static char *find_command(const char *name, char *argv[])
{
	char *cmd = NULL;		/* command's absolute path */
	char *env_path = NULL;		/* contents of PATH env variable */
	char *absl_path = NULL;		/* path entry in PATH env variable */
	char *this_path = NULL;		/* the path that might point to the program */
	char delimiter[2] = ":";	/* delimiter for path enteries in PATH env variable */

	/* Read our PATH environment variable. */
	if ((env_path = getenv("PATH")) != NULL)
		absl_path = strtok(env_path, delimiter);
	else {
		fprintf(stderr, "xcrun: error: failed to read PATH variable.\n");
		return NULL;
	}

	/* Search each path entry in PATH until we find our program. */
	while (absl_path != NULL) {
		this_path = (char *)malloc((PATH_MAX - 1));

		/* Construct our program's absolute path. */
		strncpy(this_path, absl_path, strlen(absl_path));
		cmd = strncat(strcat(this_path, "/"), name, strlen(name));

		/* Does it exist? Is it an executable? */
		if (access(cmd, X_OK) == 0) {
			call_command(cmd, argv);
			/* NOREACH */
			fprintf(stderr, "xcrun: error: can't exec \'%s\' (errno=%s)\n", cmd, strerror(errno));
			return NULL;
		}

		/* If not, move onto the next entry.. */
		absl_path = strtok(NULL, delimiter);
	}

	/* We have searched PATH, but we haven't found our program yet. Try looking at the SDK folder */
	this_path = (char *)malloc((PATH_MAX - 1));
	if ((this_path = get_sdk_path()) != NULL) {
		cmd = strncat(strcat(this_path, "/bin/"), name, strlen(name));
		/* Does it exist? Is it an executable? */
		if (access(cmd, X_OK) == 0) {
			call_command(cmd, argv);
			/* NOREACH */
			fprintf(stderr, "xcrun: error: can't exec \'%s\' (errno=%s)\n", cmd, strerror(errno));
			return NULL;
		}
	}

	/* We have searched everywhere, but we haven't found our program. State why. */
	fprintf(stderr, "xcrun: error: can't exec \'%s\' (errno=%s)\n", name, strerror(errno));

	/* Search has failed, return NULL. */
	return NULL;
}

int main(int argc, char *argv[])
{
	int retval = 1;

	/* Print usage if no argument is supplied. */
	if (argc < 2)
		usage(argv[0]);

	/* Search for program. */
	if (find_command(argv[1], ++argv) != NULL)
		retval = 0;
	else {
		fprintf(stderr, "xcrun: error: failed to execute command \'%s\'. aborting.\n", argv[0]);
	}

	return retval;
}
