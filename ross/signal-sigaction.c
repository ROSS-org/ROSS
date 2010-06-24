#include <ross.h>
#include <signal.h>

void
tw_register_signals(void)
{
#ifndef BLUE_GENE
	struct sigaction	siga;

	sigemptyset(&siga.sa_mask);
	siga.sa_flags = 0;

	siga.sa_handler = (void(*)(int))tw_sigsegv;
	sigaction(SIGSEGV, &siga, NULL);
	sigaction(SIGBUS, &siga, NULL);
	sigaction(SIGINT, &siga, NULL);

	siga.sa_handler = (void(*)(int))tw_sigterm;
	sigaction(SIGTERM, &siga, NULL);
#endif
}
