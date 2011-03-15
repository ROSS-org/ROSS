#include <stdlib.h>
#include <stdio.h>

struct var
{
	const char *name;
	const char *value;
	const char *help;
};

const struct var values[] = {
	{ "cc", CC, "C compiler used by ROSS" },
	{ "cflags", CFLAGS, "C flags used by ROSS" },
	{ "ldflags", LDFLAGS, "Linker flags used by ROSS" },
	{ NULL, NULL }
};

static int
usage(void)
{
	unsigned i;
	fputs("usage: ross-config option\n", stderr);
	fputc('\n', stderr);
	fputs("Recognized options:\n", stderr);
	for (i = 0; values[i].name; i++)
		fprintf(stderr,
			"    --%-15s %s\n",
			values[i].name,
			values[i].help);
	return 1;
}

int
main(int argc, char **argv)
{
	const char *name;
	unsigned i;

	if (argc != 2)
		return usage();

	name = argv[1];
	if (!strncmp(name, "--", 2))
		name += 2;
	if (!*name)
		return usage();

	for (i = 0; values[i].name; i++) {
		if (!strcmp(name, values[i].name)) {
			fputs(values[i].value, stdout);
			fputc('\n', stdout);
			return 0;
		}
	}

	return usage();
}
