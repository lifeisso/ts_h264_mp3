#include <stdio.h>
#include <string.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */
#include <arpa/inet.h>

#if 0
#define SERV_ADDR	"224.0.1.1"
#define SERV_PORT	10002
int msd;
struct sockaddr_in      servaddr;
socklen_t servaddrlen;
void mutilcast_init(void)
{
	msd = socket(AF_INET, SOCK_DGRAM, 0);

	bzero(&servaddr, sizeof(servaddr));
        servaddr.sin_family      = AF_INET;
        servaddr.sin_addr.s_addr = inet_addr(SERV_ADDR);
        servaddr.sin_port        = htons(SERV_PORT);

	servaddrlen = sizeof(servaddr);
}
int mutilcast_send(void)
{
	char buf[188];

	int ret = read_ts_packet(buf);
	if(ret == 0){
		printf("new data...\n");
		return sendto(msd, buf, 188, 0, &servaddr, servaddrlen);
	}
}

#endif
