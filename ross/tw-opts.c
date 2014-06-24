#include <ctype.h>
#include <ross.h>

static const char ross_options[] = ROSS_OPTION_LIST;
static const char *program;
static const tw_optdef *all_groups[10];

static void need_argument(const tw_optdef *def) NORETURN;
static int is_empty(const tw_optdef *def);

static const tw_optdef *opt_groups[10];
static unsigned int opt_index = 0;

void
tw_opt_add(const tw_optdef *options)
{
	if(!options || !options->type || is_empty(options))
		return;

	opt_groups[opt_index++] = options;
	opt_groups[opt_index] = NULL;
}

#if 0
static void
print_options(FILE * f)
{
	const char *s = ross_options;
	int first = 1;

	while (s) {
		const char *hd = strstr(s, "-DROSS_");
		const char *u;
		if (!hd)
			break;
		hd += strlen("-DROSS_");

		if (first)
			first = 0;

		s = hd;
		while (*s && isupper(*s))
			s++;
		u = *s == '_' ? ++s : NULL;
		while (*s && !isspace(*s))
			s++;

		if (u)
			fprintf(f, "%s,", u);
		else
			fprintf(stderr, "%s,", hd);
	}
}
#endif

static void
show_options(void)
{
	const char *s = ross_options;
	int first = 1;

	while (s) {
		const char *hd = strstr(s, "-DROSS_");
		const char *u;
		if (!hd)
			break;
		hd += strlen("-DROSS_");

		if (first) {
			fprintf(stderr, "\nROSS Kernel Build Options:\n");
			first = 0;
		}

		s = hd;
		while (*s && isupper(*s))
			s++;
		u = *s == '_' ? ++s : NULL;
		while (*s && !isspace(*s))
			s++;

		fputs("  ", stderr);
		if (u)
			fprintf(stderr, "%.*s=%.*s", (int) (u - hd - 1), hd, (int) (s - u), u);
		else
			fprintf(stderr, "%.*s", (int) (s - hd), hd);
		fputc('\n', stderr);
	}
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
			case TWOPTTYPE_UINT:
				pos += fprintf(stderr, "=n");
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

				case TWOPTTYPE_UINT:
					fprintf(stderr, " (default %u)", *((unsigned int*)def->value));
					break;

				case TWOPTTYPE_STIME:
					fprintf(stderr, " (default %.2f)", *((tw_stime*)def->value));
					break;

				case TWOPTTYPE_CHAR:
					fprintf(stderr, " (default %s)", (char *) def->value);
					break;

				default:
					break;
				}
			}

			fputc('\n', stderr);
			cnt++;
		}
	}

	show_options();
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

				case TWOPTTYPE_UINT:
					fprintf(f, "%u,", *((unsigned int*)def->value));
					break;

				case TWOPTTYPE_STIME:
					fprintf(f, "%.2f,", *((tw_stime*)def->value));
					break;

				case TWOPTTYPE_CHAR:
					fprintf(f, "%s,", (char *)def->value);
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

static void
apply_opt(const tw_optdef *def, const char *value)
{
	switch (def->type)
	{
	case TWOPTTYPE_ULONG:
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
		case TWOPTTYPE_UINT:
			*((unsigned int*)def->value) = (unsigned int)v;
			break;
		default:
			tw_error(TW_LOC, "Option type not supported here.");
		}
		break;
	}

	case TWOPTTYPE_STIME:
	{
		tw_stime v;
		char *end;

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

		//*((char **)def->value) = tw_calloc(TW_LOC, "string arg", strlen(value) + 1, 1);
		strcpy((char*)def->value, value);
		break;
	}

	case TWOPTTYPE_SHOWHELP:
		if (tw_ismaster())
			show_help();
		tw_net_stop();
		exit(0);
		break;

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
			tw_error(TW_LOC, "Too many tw_optdef arrays.");
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

void des_print_model_name(void) {
  if (opt_groups != NULL && opt_groups[0] != NULL) {
    fprintf(desTraceFile, "%s", opt_groups[0]->help);
  }
}
