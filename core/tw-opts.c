#include <ctype.h>
#include <ross.h>
#include <string.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) ( sizeof((a)) / sizeof((a)[0]) )
#endif

static const char *program;
static const tw_optdef *all_groups[10];

static void need_argument(const tw_optdef *def) NORETURN;
static int is_empty(const tw_optdef *def);
static void parse_args_file(const char* file_name, int* argc_p, char ***argv_p);

static const tw_optdef *opt_groups[10];
static unsigned int opt_index = 0;

// We don't allow recursive invocations of the `args-file` option
static uint args_file_depth = 0;

void
tw_opt_add(const tw_optdef *options)
{
	if(!options || !options->type || is_empty(options))
		return;

	opt_groups[opt_index++] = options;
	opt_groups[opt_index] = NULL;
}

static void
show_help(void)
{
	const tw_optdef **group = all_groups;
	unsigned cnt = 0;

	fprintf(stderr, "usage: %s [options] [-- [args]]\n", program);
	fputc('\n', stderr);

	for (; *group; group++)
	{
		const tw_optdef *def = *group;
		for (; def->type; def++)
		{
			int pos = 0;

			if (def->type == TWOPTTYPE_GROUP)
			{
				if (cnt)
					fputc('\n', stderr);
				fprintf(stderr, "%s:\n", def->help);
				cnt++;
				continue;
			}

			pos += fprintf(stderr, "  --%s", def->name);
			switch (def->type)
			{
			case TWOPTTYPE_ULONG:
			case TWOPTTYPE_ULONGLONG:
			case TWOPTTYPE_UINT:
				pos += fprintf(stderr, "=n");
				break;

			case TWOPTTYPE_DOUBLE:
				pos += fprintf(stderr, "=dbl");
                                break;

			case TWOPTTYPE_STIME:
				pos += fprintf(stderr, "=ts");
				break;

			case TWOPTTYPE_CHAR:
				pos += fprintf(stderr, "=str");
				break;

			default:
				break;
			}

			if (def->help)
			{
				int col = 22;
				int pad = col - pos;
				if (pad > 0)
					fprintf(stderr, "%*s", col - pos, "");
				else {
					fputc('\n', stderr);
					fprintf(stderr, "%*s", col, "");
				}
				fputs("  ", stderr);
				fputs(def->help, stderr);
			}

			if (def->value)
			{
				switch (def->type)
				{
				case TWOPTTYPE_ULONG:
					fprintf(stderr, " (default %lu)", *((unsigned long*)def->value));
					break;

				case TWOPTTYPE_ULONGLONG:
					fprintf(stderr, " (default %llu)", *((unsigned long long*)def->value));
					break;

				case TWOPTTYPE_UINT:
					fprintf(stderr, " (default %u)", *((unsigned int*)def->value));
					break;

				case TWOPTTYPE_DOUBLE:
					fprintf(stderr, " (default %.2f)", *((double*)def->value));
                                        break;

				case TWOPTTYPE_STIME:
					fprintf(stderr, " (default %.2f)", *((tw_stime*)def->value));
					break;

				case TWOPTTYPE_CHAR:
					fprintf(stderr, " (default %s)", (char *) def->value);
					break;

                                case TWOPTTYPE_FLAG:
                                    if((*(unsigned int*)def->value) == 0) {
                                        fprintf(stderr, " (default off)");
                                    } else {
                                        fprintf(stderr, " (default on)");
                                    }
                                    break;

				default:
					break;
				}
			}

			fputc('\n', stderr);
			cnt++;
		}
	}

        // CMake used to pass options by command line flags
	fprintf(stderr, "ROSS CMake Configuration Options:\n");
        fprintf(stderr, "  (See build-dir/core/config.h)\n");
}

void tw_opt_settings(FILE *outfile) {
    const tw_optdef **group = all_groups;
    unsigned cnt = 0;

    for (; *group; group++){
        const tw_optdef *def = *group;
        for (; def->type; def++){
            int pos = 0;

            if (def->type == TWOPTTYPE_GROUP){
                if (cnt)
                    fputc('\n', outfile);
                fprintf(outfile, "%s:\n", def->help);
                cnt++;
                continue;
            }

            pos += fprintf(outfile, "  --%s", def->name);

            if (def->value) {
                int col = 20;
                int pad = col - pos;
                if (pad > 0) {
                    fprintf(outfile, "%*s", col - pos, "");
                } else {
                    fputc('\n', outfile);
                    fprintf(outfile, "%*s", col, "");
                }
                fputs("  ", outfile);
            }

            if (def->value){
                switch (def->type){
                case TWOPTTYPE_ULONG:
                    fprintf(outfile, "%lu", *((unsigned long*)def->value));
                    break;

                case TWOPTTYPE_ULONGLONG:
                    fprintf(outfile, "%llu", *((unsigned long long*)def->value));
                    break;

                case TWOPTTYPE_UINT:
                    fprintf(outfile, "%u", *((unsigned int*)def->value));
                    break;

                case TWOPTTYPE_DOUBLE:
                    fprintf(outfile, "%.2f", *((double*)def->value));
                    break;

                case TWOPTTYPE_STIME:
                    fprintf(outfile, "%.2f", *((tw_stime*)def->value));
                    break;

                case TWOPTTYPE_CHAR:
                    fprintf(outfile, "%s", (char *) def->value);
                    break;

                case TWOPTTYPE_FLAG:
                    if((*(unsigned int*)def->value) == 0) {
                        fprintf(outfile, "off");
                    } else {
                        fprintf(outfile, "on");
                    }
                    break;

                default:
                    break;
                }
            }

            fputc('\n', outfile);
            cnt++;
        }
    }
}

void
tw_opt_print(void)
{
	FILE *f = g_tw_csv;
	const tw_optdef **group = all_groups;

	if(!tw_ismaster() || NULL == f)
		return;

	for (; *group; group++)
	{
		const tw_optdef *def = *group;
		for (; def->type; def++)
		{
			if (def->type == TWOPTTYPE_GROUP ||
				(def->name && 0 == strcmp(def->name, "help")))
				continue;

			if (def->value)
			{
				switch (def->type)
				{
				case TWOPTTYPE_ULONG:
					fprintf(f, "%lu,", *((unsigned long*)def->value));
					break;

				case TWOPTTYPE_ULONGLONG:
					fprintf(f, "%llu,", *((unsigned long long*)def->value));
					break;

				case TWOPTTYPE_UINT:
					fprintf(f, "%u,", *((unsigned int*)def->value));
					break;

				case TWOPTTYPE_DOUBLE:
					fprintf(f, "%.2f,", *((double*)def->value));
                                        break;

				case TWOPTTYPE_STIME:
					fprintf(f, "%.2f,", *((tw_stime*)def->value));
					break;

				case TWOPTTYPE_CHAR:
					fprintf(f, "%s,", (char *)def->value);
					break;

                                case TWOPTTYPE_FLAG:
                                    fprintf(f, "%s,", (char *)def->name);
                                    break;

				default:
					break;
				}
			} else
				fprintf(f, "undefined,");
		}
	}

	//print_options(f);
}

static void
need_argument(const tw_optdef *def)
{
	if (tw_ismaster())
		fprintf(stderr,
			"%s: option --%s requires a valid argument\n",
			program, def->name);
	tw_net_stop();
	exit(1);
}

// Trims left spaces and returns NULL if the string is empty or has a comment (starts with #)
char *ltrim(char *s)
{
    while(isspace(*s)) s++;
    if (*s == '\0' || *s == '#') { return NULL; }
    return s;
}

// Returns the next argument and leaves `line` pointer at the end of the argument, so that it can be consumed further
char *next_argument(char **line) {
    char * to_ret = *line;
    // consuming characters until we have traversed a full argument
    // An argument doesn't have any spaces on it
    while(!(isspace(**line) || **line == '\0' || **line == '#')) {
        (*line)++;
    }

    switch (**line) {
        case '\0':
            break;
        case '#':  // we don't care about the rest of the line
            **line = '\0';
            break;
        default:  // the next character is a space, so there might be more arguments in the same line
            **line = '\0';
            (*line)++;
    }

    return to_ret;
}

static void
apply_opt(const tw_optdef *def, const char *value)
{
	switch (def->type)
	{
	case TWOPTTYPE_ULONG:
	case TWOPTTYPE_ULONGLONG:
	case TWOPTTYPE_UINT:
	{
		unsigned long v;
		char *end;

		if (!value)
			need_argument(def);
		v = strtoul(value, &end, 10);

		if (*end)
			need_argument(def);
		switch (def->type)
		{
		case TWOPTTYPE_ULONG:
			*((unsigned long*)def->value) = v;
			break;
		case TWOPTTYPE_ULONGLONG:
			*((unsigned long long*)def->value) = v;
			break;

		case TWOPTTYPE_UINT:
			*((unsigned int*)def->value) = (unsigned int)v;
			break;
		default:
			tw_error(TW_LOC, "Option type not supported here.");
		}
		break;
	}

        case TWOPTTYPE_DOUBLE:
	{
		double v;
		char *end;

		if (!value)
			need_argument(def);
		v = strtod(value, &end);
		if (*end)
			need_argument(def);
		*((double*)def->value) = v;
		break;
	}

	case TWOPTTYPE_STIME:
	{
		tw_stime v;
		char *end;

                tw_warning(TW_LOC, "Option type stime (TWOPT_STIME) is deprecated. Please use double (TWOPT_DOUBLE).");

		if (!value)
			need_argument(def);
		v = strtod(value, &end);
		if (*end)
			need_argument(def);
		*((tw_stime*)def->value) = v;
		break;
	}

	case TWOPTTYPE_CHAR:
	{
		if (!value)
			need_argument(def);

		// *((char **)def->value) = tw_calloc(TW_LOC, "string arg", strlen(value) + 1, 1);
		strcpy((char *) def->value, value);
		break;
	}

        case TWOPTTYPE_FLAG:
                *((unsigned int*)def->value) = 1;
                break;

	case TWOPTTYPE_SHOWHELP:
		if (tw_ismaster())
			show_help();
		tw_net_stop();
		exit(0);
		break;

	case TWOPTTYPE_ARGSFILE:
	{
		if (args_file_depth) {
			tw_error(TW_LOC, "--args-file cannot be invoked inside an args-file.");
		}
		args_file_depth++;

		int argc_parsed = 1;
		char** argv_parsed = (char**)malloc(sizeof(char*));

		// Copying program name into first slot of argv
		size_t prog_name_len = strlen(program);
		argv_parsed[0] = malloc(prog_name_len + 3); // 1 for `\0`, and 2 for "./"
		strcpy(argv_parsed[0], "./");
		strcat(argv_parsed[0], program);
		argv_parsed[0][prog_name_len+2] = '\0';

		parse_args_file(value, &argc_parsed, &argv_parsed);

		// Print out the arguments passed through the args-file
		if (tw_ismaster()) {
			printf("Arguments passed through --args-file:\n");
			for (int i = 1; i < argc_parsed; i++) {
				printf("%s\n", argv_parsed[i]);
			}
			printf("\n");
		}

		// Recursive call of args-file (it is not allowed to go recursively more than once, thanks to `args_file_depth`)
		tw_opt_parse(&argc_parsed, &argv_parsed);
		for (size_t i = 0; i < argc_parsed; i++) {
			free(argv_parsed[0]);
		}
		free(argv_parsed);

		args_file_depth--;
		break;
	}

	default:
		tw_error(TW_LOC, "Option type not supported here.");
	}
}

static void
match_opt(const char *arg)
{
	const char *eq = strchr(arg + 2, '=');
	const tw_optdef **group = all_groups;

	for (; *group; group++)
	{
		const tw_optdef *def = *group;
		for (; def->type; def++)
		{
			if (!def->name || def->type == TWOPTTYPE_GROUP)
				continue;
			if (!eq && !strcmp(def->name, arg + 2))
			{
				apply_opt(def, NULL);
				return;
			}
			else if (eq && !strncmp(def->name, arg + 2, eq - arg - 2))
			{
				apply_opt(def, eq + 1);
				return;
			}
		}
	}

	if (tw_ismaster())
		fprintf(stderr,
			"%s: option '%s' not recognized; see --help for details\n",
			program, arg);
	tw_net_stop();
	exit(1);
}

static const tw_optdef basic[] = {
	{ TWOPTTYPE_SHOWHELP, "help", "show this message", NULL },
	TWOPT_END()
};

static int is_empty(const tw_optdef *def)
{
	for (; def->type; def++) {
		if (def->type == TWOPTTYPE_GROUP)
			continue;
		return 0;
	}
	return 1;
}

static void
parse_args_file(const char* file_name, int* argc_p, char ***argv_p)
{
	FILE* file;
	char* line = NULL;
	char* argument = NULL;
	size_t len = 0;
	ssize_t read;
	int argc = *argc_p;
	char** argv = *argv_p;

	file = fopen(file_name, "r");

	if (file == NULL) {
		tw_error(TW_LOC, "Invalid file path!");
	}

	while ((read = getline(&line, &len, file)) != -1) {
		char * start_line = line;
		line = ltrim(line);
		// Retrieve all arguments from the line
		while (line && (argument = next_argument(&line))) {
			argc++;
			argv = (char**)realloc(argv, sizeof(char*)*argc);

			// Saving argument in memory
			size_t argument_len = strlen(argument);
			argv[argc-1] = (char*)malloc(sizeof(char)*(argument_len+1));
			strcpy(argv[argc-1], argument);

			line = ltrim(line);

			// checking for disallowed `--` argument
			if(strcmp(argument, "--")==0) {
				tw_error(TW_LOC, "Argument `--' is invalid inside of args-file.");
			}
		}
		free(start_line);
	}
	if (line) { free(line); }

	*argc_p = argc;
	*argv_p = argv;
}

void
tw_opt_parse(int *argc_p, char ***argv_p)
{
	int argc = *argc_p;
	char **argv = *argv_p;
	unsigned i;

	program = strrchr(argv[0], '/');
	if (program)
		program++;
	else
		program = argv[0];

	for (i = 0; opt_groups[i]; i++)
	{
		if (!(opt_groups[i])->type || is_empty(opt_groups[i]))
			continue;
		if (i >= ARRAY_SIZE(all_groups))
		{
			tw_error(TW_LOC, "Too many tw_optdef arrays.");
		}
		all_groups[i] = opt_groups[i];
	}
	all_groups[i++] = basic;
	all_groups[i] = NULL;

	while (argc > 1)
	{
		const char *s = argv[1];
		if (strncmp(s, "--", 2))
		  {
		    printf("Warning: found ill-formated argument: %s, stopping arg parsing here!! \n", s );
		    break;
		  }
		if (strcmp(s, "--"))
			match_opt(s);

		argc--;
		memmove(argv + 1, argv + 2, (argc - 1) * sizeof(*argv));

		if (!strcmp(s, "--"))
			break;
	}

	*argc_p = argc;
	*argv_p = argv;
}
