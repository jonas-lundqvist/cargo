//
// The MIT License (MIT)
//
// Copyright (c) 2015 Joakim Soderberg <joakim.soderberg@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "cargo.h"

static char *og1_1 = "default 1 1";

static char *og2_1 = "default 2 1";
static char *module2_login;
static char *module2_password;

static char *global_1 = "default global 1";
static char *global_2 = "default global 2";

typedef enum cmd_s
{
	INVALID_COMMAND,
	COMMAND1,
	COMMAND2,
	COMMAND3
} cmd_t;

static cmd_t str_to_cmd(const char *str)
{
	if (!strcmp(str, "command1")) return COMMAND1;
	if (!strcmp(str, "command2")) return COMMAND2;
	if (!strcmp(str, "command3")) return COMMAND3;

	return INVALID_COMMAND;
}

int add_global_opts(cargo_t cargo)
{
	int ret = 0;
	ret |= cargo_add_option(cargo, 0, "--opt1", "String used for global option 1", "s", &global_1);
	ret |= cargo_add_option(cargo, 0, "--opt2", "String used for global option 2", "s", &global_2);
	return ret;
}

int add_module1_opts(cargo_t cargo)
{
	int ret = 0;
	ret |= cargo_add_group(cargo, 0, "module1", "Module 1 options", NULL);
	ret |= cargo_add_option(cargo, 0, "<module1> --og1_1", "String used for option 1 1", "s", &og1_1);
	return ret;
}

int subcommand1(int argc, char **argv)
{
	int ret = 0;
	cargo_t mod1;

	if (cargo_init(&mod1, 0, argv[0]))
	{
		fprintf(stderr, "Failed to init command line parsing\n");
		return -1;
	}

	ret |= add_global_opts(mod1);
	ret |= add_module1_opts(mod1);
	assert(ret == 0);

	return ret;
}

int subcommand2(int argc, char **argv)
{
	int ret = 0;
	cargo_t mod2;

	if (cargo_init(&mod2, 0, argv[0]))
	{
		fprintf(stderr, "Failed to init command line parsing\n");
		return -1;
	}

	ret |= add_global_opts(mod2);

	// Module 1.
	ret |= add_module1_opts(mod2);

	// Module 2.
	ret |= cargo_add_group(mod2, 0, "module2", "Module 2 options", NULL);
	ret |= cargo_add_option(mod2, 0, "<module2> --og2_1", "String used for option 1 1", "s", &og1_1);
	assert(ret == 0);

	return ret;
}

static int parse_command_cb(cargo_t ctx, void *user, char *optname, int argc, char **argv)
{
	cmd_t *cmd = (cmd_t *)user;
	assert(cmd);

	*cmd = (cmd_t)str_to_cmd(argv[0]);

	if (*cmd == INVALID_COMMAND)
	{
		fprintf(stderr, "Invalid command \"%s\"\n", argv[0]);
		return -1;
	}
	return 1;
}

int main(int argc, char **argv)
{
	cargo_t cargo;
	int ret = 0;
	size_t i;
	char **strs = NULL;
	size_t str_count = 0;
	int verbose = 0;
	cmd_t cmd = INVALID_COMMAND;

	if (cargo_init(&cargo, 0, argv[0]))
	{
		fprintf(stderr, "Failed to init command line parsing\n");
		return -1;
	}

	//
	// Commands, one required, but only one at a time.
	//
	ret |= cargo_add_mutex_group(cargo,
			CARGO_MUTEXGRP_ONE_REQUIRED |
			CARGO_MUTEXGRP_GROUP_USAGE,
			"cmds", "Commands", NULL);
	ret |= cargo_mutex_group_set_metavar(cargo, "cmds", "COMMAND");

	ret |= cargo_add_option(cargo, CARGO_OPT_NOT_REQUIRED, "<!cmds> command1",
			"Silly example command", "c", parse_command_cb, &cmd);

	ret |= cargo_add_option(cargo, 0, "args",
			"Some more args", "[s]*", &strs, &str_count);


	// This is just a dummy, it will not parse anything that is left to command1
	// but still show up in usage and so on.
	ret |= cargo_add_option(cargo, CARGO_OPT_NOT_REQUIRED, "<!cmds> command2",
			"Another silly example command", "c0", NULL, NULL);

	ret |= cargo_add_option(cargo, CARGO_OPT_NOT_REQUIRED, "<!cmds> command3",
			"The third silly command", "D");

	//
	// Some other arguments.
	//

	// Allow -vvv or multiple --verbosity and -v are counted to raise the count.
	ret |= cargo_add_option(cargo, CARGO_OPT_NOT_REQUIRED, "--verbose -v",
			"Verbosity", "b!", &verbose);

	assert(ret == 0);

	if (cargo_parse(cargo, 0, 1, argc, argv)) return -1;

	switch (cmd)
	{
		default:
		case INVALID_COMMAND: printf("Invalid command\n"); break;
		case COMMAND1: printf("Command 1\n"); break;
		case COMMAND2: printf("Command 2\n"); break;
		case COMMAND3: printf("Command 3\n"); break;
	}

	if (verbose)
	{
		printf("Got %lu extra arguments\n", str_count);
	}

	if (verbose >= 2)
	{
		for (i = 0; i < str_count; i++)
		{
			printf("%s\n", strs[i]);
		}
	}

	cargo_destroy(&cargo);
	return 0;
}
