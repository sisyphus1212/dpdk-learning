

#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <fcntl.h>

#include <linux/if.h>
#include <linux/if_tun.h>

#include <sys/poll.h>
#include <sys/ioctl.h>


// alloc tap
int tun_alloc(char *dev)
{
    struct ifreq ifr;
    int fd, err;
    memset(&ifr, 0, sizeof(ifr));

    if( (fd = open("/dev/net/tun", O_RDWR)) < 0 )
        return -1;

	ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
    if( *dev )
        memcpy(ifr.ifr_name, dev, strlen(dev));

    if( (err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0 ){
        close(fd);
        return err;
    }

	return fd;
}


int main() {


    int fd = tun_alloc("vnet0");


    struct pollfd pfd = {0};
	pfd.fd = fd;
	pfd.events = POLLIN | POLLOUT;

	printf("vhost_user_vnet_start --> \n");
	while (1) { // fd --> qemu , eth0. 

		int ret = poll(&pfd, 1, -1);
		if (ret < 0) {
			usleep(1); 
			continue;
		}

		//rx
		if (pfd.revents & POLLIN) {
			char buffer[1024] = {0};
			int np = read(fd, buffer, 1024);
			if (np > 0) {
				printf("buffer: %d\n", np);
			}
		}
		

		usleep(1);
	}


}

	
	
	

