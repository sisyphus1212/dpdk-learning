

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <arpa/inet.h>



// posix
// ./posix_server 192.168.0.29 8888
int main(int argc, char *argv[]) {

	int listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd == -1) {
		return -1;
	}

	struct sockaddr_in servaddr, clientaddr;
	memset(&servaddr, 0, sizeof(struct sockaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(8888);
	
	bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
	
	listen(listenfd, 10);

	socklen_t len = sizeof(clientaddr);
	int connfd = accept(listenfd, (struct sockaddr*)&clientaddr, &len);

	char buffer[1024] = {0};
	int rv = recv(connfd, buffer, 1024, 0);
	
	printf("Received from client length: %d , %s\n", rv, buffer);

	rv = send(connfd, buffer, rv, 0);

	close(connfd);

	close(listenfd);

	return 0;
	
}





