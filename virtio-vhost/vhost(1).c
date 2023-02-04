




#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <unistd.h>

#include <sys/socket.h>
#include <sys/un.h>

#include <stdint.h>
#include <stddef.h>

#include <pthread.h>
#include <fcntl.h>

#include <sys/ioctl.h>
#include <sys/mman.h>


#include <linux/if.h>
#include <linux/if_tun.h>

//typedef uint8_t u8;




//virtio-v1.1-cs.pdf. page 74
struct virtio_net_hdr {

#define VIRTIO_NET_HDR_F_NEEDS_CSUM 1    /**< Use csum_start,csum_offset*/
	uint8_t flags;
#define VIRTIO_NET_HDR_GSO_NONE     0    /**< Not a GSO frame */
#define VIRTIO_NET_HDR_GSO_TCPV4    1    /**< GSO frame, IPv4 TCP (TSO) */
#define VIRTIO_NET_HDR_GSO_UDP      3    /**< GSO frame, IPv4 UDP (UFO) */
#define VIRTIO_NET_HDR_GSO_TCPV6    4    /**< GSO frame, IPv6 TCP */
#define VIRTIO_NET_HDR_GSO_ECN      0x80 /**< TCP has ECN set */
	uint8_t gso_type;
	uint16_t hdr_len;     /**< Ethernet + IP + tcp/udp hdrs */
	uint16_t gso_size;    /**< Bytes to append to hdr_len per frame */
	uint16_t csum_start;  /**< Position to start checksumming from */
	uint16_t csum_offset; /**< Offset after that to place checksum */
	uint16_t num_buffers;
};

// virtio-v1.1-cs.pdf. page 21
struct virtq_desc {
	uint64_t addr;
	uint32_t len;
#define VIRTQ_DESC_F_NEXT 1
#define VIRTQ_DESC_F_WRITE 2
#define VIRTQ_DESC_F_INDIRECT 4
	uint16_t flags;
	uint16_t next;
	
};

struct virtq_avail {
#define VIRTQ_AVAIL_F_NO_INTERRUPT 1
	uint16_t flags;
	uint16_t idx;
	uint16_t ring[0];
	uint16_t used_event; /* Only if VIRTIO_F_EVENT_IDX */ 

};

/* le32 is used here for ids for padding reasons. */
struct virtq_used_elem {
	/* Index of start of used descriptor chain. */
	uint32_t id;
	/* Total length of the descriptor chain which was used (written to) */
	uint32_t len;
};

struct virtq_used {
#define VIRTQ_USED_F_NO_NOTIFY 1
	uint16_t flags;
	uint16_t idx;
	struct virtq_used_elem ring[0];
	uint16_t avail_event; /* Only if VIRTIO_F_EVENT_IDX */
};


struct virtqueue {

	struct virtq_desc *desc;
	struct virtq_avail *avail;
	struct virtq_used *used;

	int kickfd;
	int callfd;

	uint32_t num;

};

// vpp , dpdk , virtio -->
// ---> virtio

#define VIRTIO_NET_F_CSUM					0
#define VIRTIO_NET_F_GUEST_CSUM				1
#define VIRTIO_NET_F_CTRL_GUEST_OFFLOADS	2
#define VIRTIO_NET_F_MTU					3
#define VIRTIO_NET_F_MAC					5
#define VIRTIO_NET_F_GUEST_TSO4				7
#define VIRTIO_NET_F_GUEST_TSO6				8

#define VIRTIO_NET_F_GUEST_ECN				9
#define VIRTIO_NET_F_GUEST_UFO				10
#define VIRTIO_NET_F_HOST_TSO4				11
#define VIRTIO_NET_F_HOST_TSO6				12
#define VIRTIO_NET_F_HOST_ECN 				13
#define VIRTIO_NET_F_HOST_UFO				14
#define VIRTIO_NET_F_MRG_RXBUF				15
#define VIRTIO_NET_F_STATUS					16

#define VIRTIO_NET_F_CTRL_VQ				17
#define VIRTIO_NET_F_CTRL_RX				18
#define VIRTIO_NET_F_CTRL_VLAN				19
#define VIRTIO_NET_F_GUEST_ANNOUNCE			21

#define VIRTIO_NET_F_MQ						22
#define VIRTIO_NET_F_CTRL_MAC_ADDR			23

#define VIRTIO_F_VERSION_1					32


#define VIRTIO_NET_F_RSC_EXT				61
#define VIRTIO_NET_F_STANDBY				62

#if 0
#define VHOST_SUPPORTED_FEATURES 			\
		(1ULL << VIRTIO_NET_F_CSUM)			|	\
		(1ULL << VIRTIO_NET_F_GUEST_CSUM)|	\
		(1ULL << VIRTIO_NET_F_CTRL_GUEST_OFFLOADS)|	\
		(1ULL << VIRTIO_NET_F_MTU)				|	\
		(1ULL << VIRTIO_NET_F_MAC)				|	\
		(1ULL << VIRTIO_NET_F_GUEST_TSO4)		|	\
		(1ULL << VIRTIO_NET_F_GUEST_TSO6)		|	\
		(1ULL << VIRTIO_NET_F_GUEST_ECN)		|	\
		(1ULL << VIRTIO_NET_F_GUEST_UFO)		|	\
		(1ULL << VIRTIO_NET_F_HOST_TSO4)		|	\
		(1ULL << VIRTIO_NET_F_HOST_TSO6)		|	\
		(1ULL << VIRTIO_NET_F_HOST_ECN)			|	\
		(1ULL << VIRTIO_NET_F_HOST_UFO)			|	\
		(1ULL << VIRTIO_NET_F_MRG_RXBUF)		|	\
		(1ULL << VIRTIO_NET_F_STATUS)			|	\
		(1ULL << VIRTIO_NET_F_CTRL_VQ)			|	\
		(1ULL << VIRTIO_NET_F_CTRL_RX)			|	\
		(1ULL << VIRTIO_NET_F_CTRL_VLAN)		|	\
		(1ULL << VIRTIO_NET_F_GUEST_ANNOUNCE)	|	\
		(1ULL << VIRTIO_NET_F_MQ)				|	\
		(1ULL << VIRTIO_NET_F_CTRL_MAC_ADDR)	|	\
		(1ULL << VIRTIO_NET_F_RSC_EXT)			|	\
		(1ULL << VIRTIO_NET_F_STANDBY)			

#else

#define VHOST_SUPPORTED_FEATURES				\
		((1ULL << VIRTIO_F_VERSION_1) | \
		 (1ULL << VIRTIO_NET_F_GUEST_CSUM) | \
		 (1ULL << VIRTIO_NET_F_GUEST_TSO4) | \
		 (1ULL << VIRTIO_NET_F_GUEST_TSO6))


#endif

#define VIRTIO_MAX_REGION		8


uint64_t vhost_supported_featrues = (VHOST_SUPPORTED_FEATURES);

#define ROUNDON(x, y)  (x & (~(y - 1)))
#define ROUNDUP(x, y) (((x)+(y)-1) & (~((y)-1)))


// ---> vhost

enum {
	VHOST_USER_NONE = 0,
	VHOST_USER_GET_FEATURES = 1,
	VHOST_USER_SET_FEATURES = 2,
	VHOST_USER_SET_OWNER = 3,
	VHOST_USER_RESET_OWNER = 4,
	VHOST_USER_SET_MEM_TABLE = 5,
	VHOST_USER_SET_LOG_BASE = 6,
	VHOST_USER_SET_LOG_FD = 7,
	VHOST_USER_SET_VRING_NUM = 8,
	VHOST_USER_SET_VRING_ADDR = 9,
	VHOST_USER_SET_VRING_BASE = 10,
	VHOST_USER_GET_VRING_BASE = 11,
	VHOST_USER_SET_VRING_KICK = 12,
	VHOST_USER_SET_VRING_CALL = 13,
	VHOST_USER_SET_VRING_ERR = 14,
	VHOST_USER_GET_PROTOCOL_FEATURES = 15,
	VHOST_USER_SET_PROTOCOL_FEATURES = 16,
	VHOST_USER_GET_QUEUE_NUM = 17,
	VHOST_USER_SET_VRING_ENABLE = 18,
	VHOST_USER_SEND_RARP = 19,
	VHOST_USER_NET_SET_MTU = 20,

	VHOST_USER_MAX = VHOST_USER_NET_SET_MTU,
	
};


#define VHOST_USER_VERSION_MASK		0x3
#define VHOST_USER_REPLY_MASK		0x1 << 2
#define VHOST_USER_VERSION			0x1


#define MAX_MULTI_QUEUE				256			


struct vhost_user_region {
	uint64_t guest_address;
	uint64_t size;
	uint64_t user_address;
	uint64_t mmap_offset;
};

struct vhost_user_mem {

	uint32_t nregions;
	uint32_t padding;

	struct vhost_user_region regions[VIRTIO_MAX_REGION];

};


struct vhost_vring_state {
	uint32_t index;
	uint32_t num;
};


struct vhost_vring_address {
	uint32_t index;
	uint32_t flags;
	uint32_t size;
	uint64_t desc;
	uint64_t used;
	uint64_t avail;
	uint64_t log;
};

struct vhost_user_msg {
	uint32_t request;  // 1
	uint32_t flags;		// 
	uint32_t size;

	union {

		uint64_t num;
		struct vhost_user_mem memory;
		struct vhost_vring_state state;
		struct vhost_vring_address addr;
		//uint64_t unset[VIRTIO_MAX_REGION * 2];
	
	};

	int fds[VIRTIO_MAX_REGION];
} __attribute__((packed));

pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;


struct virtio_dev {  // backend, 
	struct virtqueue vq[MAX_MULTI_QUEUE];
	struct vhost_user_mem *mem;
};

struct virtio_dev *virtiodev = NULL;


// alloc tap
int tun_alloc(char *dev)
{
    struct ifreq ifr;
    int fd, err;
    memset(&ifr, 0, sizeof(ifr));

    if( (fd = open("/dev/net/tun", O_RDWR)) < 0 )
        return -1;

	ifr.ifr_flags = IFF_TAP | IFF_NO_PI;;
    if( *dev )
        memcpy(ifr.ifr_name, dev, strlen(dev));

    if( (err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0 ){
        close(fd);
        return err;
    }

	return fd;
}



int vhost_user_set_owner(void) {

}


/*
+------------------+  +------------------+  +------------------+ <->
| Virtual-Machine  |  | Virtual-Machine  |  | Virtual-Machine  |  |
+------------------+  +------------------+  +------------------+  |
|     qemu-kvm     |  |     qemu-kvm     |  |     qemu-kvm     |  |
+------------------+  +------------------+  +------------------+  | User Space
----------------------------------------------------------------  |
|              Host Virtual Address Space	(HVA)			   |  |
================================================================ <->
|															   |  |
|                          Host OS				               |  |
|														       |  | Kernel
----------------------------------------------------------------  |
|              Host Physical Address Space  (HPA)              |  |
---------------------------------------------------------------- <->
*/


/*
+-----------------------------------+
|  Guest Virtual Address (GVA)      |
+-----------------------------------+
|            Guest OS               |
+-----------------------------------+
|  Guest Physical Address (GPA)     |
+-----------------------------------+
=====================================
+-----------------------------------+
|   Host Virtual Address (HVA)      |
+-----------------------------------+
|            Host OS                |
+-----------------------------------+
|   Host Physical Address (HPA)     |
+-----------------------------------+

+-----------------------------------+
|  guest adress
|  user address
|  size
|  mmap_offset


--> region->mmap_offset = mmap_addr + msg->mmap_offset - regions->guest_addr

--> gpa_to_hva()
--> hva = addr + region->mmap_offset

--> gva_to_hva()
--> hva = addr + region->mmap_offset + region->guest_addr - region->user_addr

*/

uint64_t gpa_to_hva(struct virtio_dev *dev, uint64_t gpa_addr) {

	int i = 0;

	struct vhost_user_region *region;

	for (i = 0;i < dev->mem->nregions;i ++) {

		region = &dev->mem->regions[i];
		if (gpa_addr <= region->user_address + region->size &&
			gpa_addr >= region->user_address)
			return gpa_addr + region->mmap_offset;

	}

	return 0;
}

uint64_t gva_to_hva(struct virtio_dev *dev, uint64_t gva_addr) {

	int i = 0;

	struct vhost_user_region *region;

	for (i = 0;i < dev->mem->nregions;i ++) {

		if (gva_addr <= region->user_address + region->size &&
			gva_addr >= region->user_address) 
			return gva_addr + region->mmap_offset + 
				region->guest_address- region->user_address;
		
	}

	return 0;

}


// msg--> 
// dev->mem
int vhost_user_set_mem_table(struct virtio_dev *dev, struct vhost_user_msg *msg) {

	if (!dev->mem) {
		dev->mem = (struct vhost_user_mem*)malloc(sizeof(struct vhost_user_mem));
		memset(dev->mem, 0, sizeof(struct vhost_user_mem));
	}

	struct vhost_user_mem *memory = &msg->memory;  // --> 

	dev->mem->nregions = memory->nregions; // 2

	printf("memory->nregions: %d\n", memory->nregions);

	int i = 0;
	for (i = 0;i < memory->nregions;i ++) {

		
		memcpy(&dev->mem->regions[i], &memory->regions[i], sizeof(struct vhost_user_region));
#if 1

		printf("fd: %d, size: %lx\n", msg->fds[i], dev->mem->regions[i].size);
		printf("mmap_offset: %lx\n", memory->regions[i].mmap_offset);


		size_t size = dev->mem->regions[i].size + dev->mem->regions[i].mmap_offset;
		size = ROUNDUP(size, 2 << 20);

		void *mmap_addr = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED,
				msg->fds[i], 0);

		dev->mem->regions[i].mmap_offset = (uint64_t)mmap_addr + memory->regions[i].mmap_offset
				- memory->regions[i].guest_address;

#else		
		void *mmap_addr = mmap(NULL, dev->mem->regions[i].size, PROT_READ|PROT_WRITE, MAP_SHARED,
				msg->fds[i], 0);

		dev->mem->regions[i].mmap_offset = (uint64_t)mmap_addr + memory->regions[i].mmap_offset
				- memory->regions[i].guest_address;
#endif
	}

	return 0;	
}



int vhost_user_msg_handler(int connfd, struct vhost_user_msg *msg) {

	switch (msg->request) {

	case VHOST_USER_GET_FEATURES:
		printf("get features : 0x%lx\n", vhost_supported_featrues);

		msg->num = vhost_supported_featrues;
		msg->size = sizeof(vhost_supported_featrues);

		msg->flags &= ~VHOST_USER_VERSION_MASK;
		msg->flags |= VHOST_USER_VERSION;
		msg->flags |= VHOST_USER_REPLY_MASK;

		size_t count = offsetof(struct vhost_user_msg, num) + msg->size;

		send(connfd, msg, count, 0);
		
		break;
		
	case VHOST_USER_SET_FEATURES:

		vhost_supported_featrues = msg->num;
	
		printf("set features: 0x%lx\n", vhost_supported_featrues);
		break;
	case VHOST_USER_SET_OWNER: //
		printf("set owner : %d\n", msg->fds[0]);

		virtiodev = (struct virtio_dev *)malloc(sizeof(struct virtio_dev));
		memset(virtiodev, 0, sizeof(struct virtio_dev));

	
		pthread_mutex_lock(&mtx);
		pthread_cond_signal(&cond);
		pthread_mutex_unlock(&mtx);
		//
		// vhost_net
		
		break;
	case VHOST_USER_RESET_OWNER:
		printf("reset owner\n");
		break;
	case VHOST_USER_SET_MEM_TABLE: // 72
		printf("set memtable\n");
		// backend --> virtio_dev
		// qemu --> vhost_user_msg
		vhost_user_set_mem_table(virtiodev, msg);
		
		break;
	case VHOST_USER_SET_LOG_BASE:
		printf("log base\n");
		break;
	case VHOST_USER_SET_LOG_FD:
		printf("log fd\n");
		break;
	case VHOST_USER_SET_VRING_NUM:
		printf("set vring num: %d\n", msg->state.num);
		virtiodev->vq[msg->state.index].num = msg->state.num;
		
		break;
	case VHOST_USER_SET_VRING_ADDR: { // msg --> 
		printf("set vring addr\n");

		struct virtqueue *vq = &virtiodev->vq[msg->state.index];
		vq->desc = (struct virtq_desc*)gva_to_hva(virtiodev, msg->addr.desc);
		vq->avail = (struct virtq_avail*)gva_to_hva(virtiodev, msg->addr.avail);
		vq->used = (struct virtq_used*)gva_to_hva(virtiodev, msg->addr.used);

		printf("index: %d\n", msg->state.index);
		printf("gva desc: %lx, avail: %lx, used: %lx\n",
			msg->addr.desc, msg->addr.avail, msg->addr.used);
		printf("hva desc: %p, avail: %p, used: %p\n", 
			vq->desc, vq->avail, vq->used);
			
		
		break;
	}
	case VHOST_USER_SET_VRING_BASE: {
		printf("set vring base\n");

		
		break;
	}
	case VHOST_USER_GET_VRING_BASE:
		printf("get vring base\n");

		
		
		break;
	case VHOST_USER_SET_VRING_KICK: {
		
		int index = msg->num & 0x00ff; //

		int fd = 0;
		if (msg->num & 0x100) { // 
			fd = -1;
		} else {
			fd = msg->fds[0];
		}

		
        printf("set vring kick : %d, index: %d\n", msg->fds[0], index);

		if (virtiodev->vq[index].kickfd > 0) {
			close(virtiodev->vq[index].kickfd);
		}

		virtiodev->vq[index].kickfd = fd;
		
		break;
	}
	case VHOST_USER_SET_VRING_CALL: {

		int index = msg->num & 0x00ff; //

		int fd = 0;
		if (msg->num & 0x100) { // 
			fd = -1;
		} else {
			fd = msg->fds[0];
		}

		
		printf("set vring call: %d, index: %d\n", msg->fds[0], index);

		if (virtiodev->vq[index].callfd > 0) {
			close(virtiodev->vq[index].callfd);
		}

		virtiodev->vq[index].callfd = fd;
		
		break;
	}
	case VHOST_USER_SET_VRING_ERR:
		printf("set vring err\n");
		break;
	case VHOST_USER_GET_PROTOCOL_FEATURES:
		printf("get protocol features\n");
		break;
	case VHOST_USER_SET_PROTOCOL_FEATURES:
		printf("set protocol features\n");
		break;
	case VHOST_USER_GET_QUEUE_NUM:
		printf("get queue num\n");
		break;
	case VHOST_USER_SET_VRING_ENABLE:
		printf("get vring enable\n");
		break;
	case VHOST_USER_SEND_RARP:
		printf("send rarp\n");
		break;
	case VHOST_USER_NET_SET_MTU:
		printf("net set mtu\n");
		break;

	}

}



void * vhost_user_vnet_start(void *arg) {

	printf("vhost_user_vnet_start --> \n");

	pthread_mutex_lock(&mtx);
	while (virtiodev == NULL) {
		pthread_cond_wait(&cond, &mtx);
	}

	pthread_mutex_unlock(&mtx);
	
	int fd = tun_alloc("vnet0");
	if (fd < 0) {
		perror("tap");
	}

	printf("vhost_user_vnet_start --> \n");
	while (1) { // fd --> qemu , eth0. 

		//tx
		

		//rx

		usleep(1);
	}
	
	

}



int main(int argc, char **argv) {

	if (argc < 2) return -1;

	int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);

	struct sockaddr_un sa;
	memset(&sa, 0, sizeof(struct sockaddr_un));
	sa.sun_family = AF_UNIX;
	sprintf(sa.sun_path, "%s", argv[1]);
	int rc = bind(sockfd, (struct sockaddr*)&sa, sizeof(struct sockaddr_un));
	if (rc < 0) return -2;

	listen(sockfd, 1024);

	pthread_t tid;
	pthread_create(&tid, NULL, vhost_user_vnet_start, NULL);

	printf("/tmp/vhost.sock --> accept\n");
	//
	int clientfd = accept(sockfd, 0, 0);

	printf("/tmp/vhost.sock --> accept clientfd: %d\n", clientfd);

	size_t hdrsz = offsetof(struct vhost_user_msg, num);

#if 0
	while (1) {

		char buffer[512] = {0};

		int rlen = recv(clientfd, buffer, hdrsz, 0);
		if (rlen == 0) {
			close(clientfd);
			printf("vm exit!\n");
			break;
		} else if (rlen > 0) {
			printf("rlen: %d\n", rlen);

			struct vhost_user_msg *msg = (struct vhost_user_msg*)buffer;
			printf("request: %d, flags: %d, size: %d\n", 
					msg->request, msg->flags, msg->size);
			
			if (msg->size > 0) {
				int rc = recv(clientfd, buffer+hdrsz, msg->size, 0);
				if (rc != msg->size)  {
					perror("recv");
				}
			}
			
			vhost_user_msg_handler(clientfd, msg);
			
		
		}

	}
#else

	while (1) {

		struct vhost_user_msg msg = {0};
		
		struct iovec iov;
		iov.iov_base = &msg;
		iov.iov_len = hdrsz;

		struct msghdr msgh;
		size_t fdsize = sizeof(msg.fds);
		char control[CMSG_SPACE(fdsize)];
		
		memset(&msgh, 0, sizeof(struct msghdr));
		msgh.msg_iov = &iov;
		msgh.msg_iovlen = 1;
		msgh.msg_control = control;  // 
		msgh.msg_controllen = sizeof(control);
		

		int rc = recvmsg(clientfd, &msgh, 0);
		if (rc <= 0) {
			perror("recvmsg");
			break;
		}

		if (msgh.msg_flags & (MSG_TRUNC | MSG_CTRUNC)) {
			break;
		}

		struct cmsghdr *cmsg;
		for (cmsg = CMSG_FIRSTHDR(&msgh);cmsg != NULL;cmsg = CMSG_NXTHDR(&msgh, cmsg)) {

			if (cmsg->cmsg_level == SOL_SOCKET && (cmsg->cmsg_type == SCM_RIGHTS)) {
				//printf("fds: %d\n", CMSG_DATA(cmsg));
				memcpy(msg.fds, CMSG_DATA(cmsg), fdsize);
			}
		
		}
		
		
		printf("request: %d, flags: %d, size: %d\n", 
					msg.request, msg.flags, msg.size);
		
		if (msg.size > 0) {
			int rc = recv(clientfd, &msg.num, msg.size, 0);
			if (rc != msg.size)  {
				perror("recv");
			}
		}
			
		vhost_user_msg_handler(clientfd, &msg);
	}
	

#endif
}



