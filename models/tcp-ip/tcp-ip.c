#include <tcp-ip.h>
#include <tcp.h>
#include <ip.h>

int
main(int argc, char ** argv, char ** env)
{
	tcp_main(argc, argv, env);
	ip_main(argc, argv, env);

	tcp_md_final();
	ip_md_final();

	return 0;
}
