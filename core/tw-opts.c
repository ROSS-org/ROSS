#include <ctype.h>
#include <ross.h>

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

void tw_opt_pretty_print(FILE *f, int help_flag) {
    const tw_optdef **group = all_groups;
    unsigned cnt = 0;

    if (help_flag) {
        fprintf(f, "usage: %s [options] [-- [args]]\n", program);
        fputc('\n', f);
    }

    for (; *group; group++){
        const tw_optdef *def = *group;
        for (; def->type; def++){
            int pos = 0;

            // Starting a new Opt Group
            if (def->type == TWOPTTYPE_GROUP){
                if (cnt)
                    fputc('\n', f);
                fprintf(f, "%s:\n", def->help);
                cnt++;
                continue;
            }

            // Print option name
            pos += fprintf(f, "  --%s", def->name);

            // If help_flag:
            //    print =type after name
            //    print help text (if specified)
            //    print default value (actually current value)
            // If no help_flag
            //    just print current value

            if (help_flag) {
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
            }

            int col = 22;
            int pad = col - pos;
            if (pad > 0)
                fprintf(f, "%*s", col - pos, "");
            else {
                fputc('\n', f);
                fprintf(f, "%*s", col, "");
            }
            fputs("  ", f);

            if (help_flag) {
                fputs(def->help, f);
                fprintf(f, " (default ");
            }

            if (def->value){
                switch (def->type){
                case TWOPTTYPE_ULONG:
                    fprintf(f, "%lu", *((unsigned long*)def->value));
                    break;

                case TWOPTTYPE_UINT:
                    fprintf(f, "%u", *((unsigned int*)def->value));
                    break;

                case TWOPTTYPE_STIME:
                    fprintf(f, "%.2f", *((tw_stime*)def->value));
                    break;

                case TWOPTTYPE_CHAR:
                    fprintf(f, "%s", (char *) def->value);
                    break;

                case TWOPTTYPE_FLAG:
                    if((*(unsigned int*)def->value) == 0) {
                        fprintf(f, "off");
                    } else {
                        fprintf(f, "on");
                    }
                    break;

                default:
                    break;
                }
            }

            if (help_flag) {
                fprintf(f, ")");
            }

            fputc('\n', f);
            cnt++;
        }
    }

    fprintf(f, "\nROSS CMake Configuration Options:\n");
    fprintf(f, "  (See build-dir/core/config.h)\n");
}

void tw_opt_csv_print() {
    // calculate a per-git-revision filename
    char fname[20];
    sprintf(fname, "ross-%.*s-clo.csv", 6, ROSS_VERSION);

    // calculate how many model/user added opt groups
    int user_opt_groups = opt_index - 3;

    // If file doesn't exist yet
    // create it and add header row
    FILE *f;
    if ((f = fopen(fname, "r")) == NULL) {
        f = fopen(fname, "w");
        // last 4 opt groups are from ROSS itself
        const tw_optdef **group = all_groups;
        int group_index = 0;
        for (; *group; group++){
            const tw_optdef *def = *group;
            for (; def->type; def++) {
                if (def->type == TWOPTTYPE_GROUP || (def->name && 0 == strcmp(def->name, "help"))) {
                    group_index++;
                    continue;
                }
                if (group_index < user_opt_groups) {
                    continue;
                }
                if (def->name) {
                    fprintf(f, "%s,", def->name);
                }
            }
        }
        fputc('\n', f);
    } else {
        f = fopen(fname, "a");
    }

    // put current option values into CSV file
    const tw_optdef **group = all_groups;

    int group_index = 0;
    for (; *group; group++)
    {
        const tw_optdef *def = *group;
        for (; def->type; def++)
        {
            if (def->type == TWOPTTYPE_GROUP || (def->name && 0 == strcmp(def->name, "help"))) {
                group_index++;
                continue;
            }

            if (group_index < user_opt_groups) {
                continue;
            }

            if (def->name && def->value)
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

                case TWOPTTYPE_FLAG:
                    if((*(unsigned int*)def->value) == 0) {
                        fprintf(f, "off,");
                    } else {
                        fprintf(f, "on,");
                    }
                    break;

                default:
                    break;
                }
            } else if (def->name) {
                fprintf(f, "undefined,");
            }
        }
    }
    fputc('\n', f);
    fclose(f);
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
        strcpy((char *) def->value, value);
        break;
    }

        case TWOPTTYPE_FLAG:
                *((unsigned int*)def->value) = 1;
                break;

    case TWOPTTYPE_SHOWHELP:
        if (tw_ismaster()) {
            tw_opt_pretty_print(stderr, 1);
        }
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
