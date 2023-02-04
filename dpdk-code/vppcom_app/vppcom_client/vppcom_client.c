


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <arpa/inet.h>


#include <vcl/vppcom.h>


// vppcom
// ./vppcom_client 192.168.0.29 8888
int main(int argc, char *argv[]) {

	int rv = 0;

	printf("vppcom server: %d\n", argc);

	if (argc < 3) return -1;

	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(struct sockaddr_in ));
	servaddr.sin_family = AF_INET;
	//servaddr.sin_addr
	inet_pton(AF_INET, argv[1], &servaddr.sin_addr);
	servaddr.sin_port = atoi(argv[2]);
	

	rv = vppcom_app_create("vppcom_server\n");
	if (rv) {
		printf("vppcom_app_create\n");
		return -1;
	}

	int connfd = vppcom_session_create(VPPCOM_PROTO_TCP, 0);
	if (connfd < 0) {
		return -1;
	}

	printf("vppcom_session_create: %s, %s\n", argv[1], argv[2]);

	vppcom_endpt_t endpt;
	memset(&endpt, 0, sizeof(endpt));
	endpt.is_ip4 = 1;
	endpt.ip = (uint8_t*)&servaddr.sin_addr; 
	endpt.port = htons(servaddr.sin_port); //
	
	rv = vppcom_session_connect(connfd, &endpt);
	if (rv < 0) {
		printf("vppcom_session_bind failed\n");
		return -1;
	}

	char *str = "vppcomclient\n";
	rv = vppcom_session_write(connfd, str, strlen(str));
	if (rv < 0) {
		printf("vppcom_session_accept failed\n");
		return -1;
	}
	
	char buffer[1024] = {0};
	rv = vppcom_session_read(connfd, buffer, sizeof(buffer));
	if (rv < 0) {
		printf("vppcom_session_accept failed\n");
		return -1;
	}
	
	printf("Received from client length: %d , %s\n", rv, buffer);

	

	vppcom_session_close(connfd);


	vppcom_app_destroy();

	return 0;
	
}







