
#include "spdk/env.h"
#include "spdk/nvme.h"
#include "spdk/stdinc.h"
#include "spdk/vmd.h"

#include <stdint.h>
#include <stdio.h>

static void write_callback(void* arg, const struct spdk_nvme_cpl* completion)
{
    int* finished = (int*)arg;
    *finished = 1;
    printf("write finished!\n");
}

static void read_callback(void* arg, const struct spdk_nvme_cpl* completion)
{
    int* finished = (int*)arg;
    *finished = 1;
    printf("read finished!\n");
}

static void do_write(struct spdk_nvme_ctrlr* ctrlr, struct spdk_nvme_ns* ns)
{
    int finished = 0;
    printf("1\n");
    struct spdk_nvme_qpair* qpair = spdk_nvme_ctrlr_alloc_io_qpair(ctrlr, NULL, 0);
    if (qpair != NULL) {
        printf("2\n");
    }

	size_t sz;
    char* wbuf = (char*)spdk_nvme_ctrlr_map_cmb(ctrlr, &sz); // 4KB
    if (wbuf != NULL) {
        printf("3.1\n");
    } else {
        printf("3.2\n");
        wbuf = (char*)spdk_zmalloc(0x1000, 0x1000, NULL, SPDK_ENV_SOCKET_ID_ANY, SPDK_MALLOC_DMA);
    }
    strcpy(wbuf, "hello world, hello world, hello world.");
    printf("4\n");
    int rc = spdk_nvme_ns_cmd_write(ns, qpair, wbuf, 0, 1, write_callback, (void*)&finished, 0);
	if (rc != 0) {
		fprintf(stderr, "starting write I/O failed\n");
		exit(1);
	}

	while (!finished) {
        spdk_nvme_qpair_process_completions(qpair, 0);
    }
    spdk_nvme_ctrlr_free_io_qpair(qpair);
}

static void do_read(struct spdk_nvme_ctrlr* ctrlr, struct spdk_nvme_ns* ns)
{
    printf("1\n");
    int finished = 0;
    struct spdk_nvme_qpair* qpair = spdk_nvme_ctrlr_alloc_io_qpair(ctrlr, NULL, 0);
    if (qpair != NULL) {
        printf("2\n");
    }

	size_t sz;
    char* rbuf = (char*)spdk_nvme_ctrlr_map_cmb(ctrlr, &sz); // 4KB
    if (rbuf != NULL) {
        printf("3.1\n");
    } else {
        printf("3.2\n");
        rbuf = (char*)spdk_zmalloc(0x1000, 0x1000, NULL, SPDK_ENV_SOCKET_ID_ANY, SPDK_MALLOC_DMA);
    }
    printf("4\n");
    int rc = spdk_nvme_ns_cmd_read(ns, qpair, rbuf, 0, 1, read_callback, (void*)&finished, 0);
	if (rc != 0) {
		fprintf(stderr, "starting read I/O failed\n");
		exit(1);
	}

	while (!finished) {
        spdk_nvme_qpair_process_completions(qpair, 0);
    }
    printf("%s\n", rbuf);
}

static bool fun1(void* cb_ctx, const struct spdk_nvme_transport_id* trid,
    struct spdk_nvme_ctrlr_opts* opts)
{
    printf("function1(%s)!\n", trid->traddr);

	return true;
}

static void fun2(void* cb_ctx, const struct spdk_nvme_transport_id* trid,
    struct spdk_nvme_ctrlr* ctrlr, const struct spdk_nvme_ctrlr_opts* opts)
{
    printf("function2(%s)!\n", trid->traddr);
    const struct spdk_nvme_ctrlr_data* cdata = spdk_nvme_ctrlr_get_data(ctrlr);
    printf("[%d-%d][%s-%s-%s]\n", cdata->vid, cdata->ssvid, cdata->sn, cdata->mn, cdata->fr);

    uint32_t num_ns = spdk_nvme_ctrlr_get_num_ns(ctrlr);
    printf("num namespace:%d\n", num_ns);

#if 0
    for (uint32_t i = 0; i <= num_ns; i++) {
        struct spdk_nvme_ns* ns = spdk_nvme_ctrlr_get_ns(ctrlr, i);
        uint64_t sz = spdk_nvme_ns_get_size(ns);
        printf("[%d][Size:%juGB]\n", i, sz / 1000000000);
        do_write(ctrlr, ns);
        do_read(ctrlr, ns);
    }
#else

	int nsid;
	for (nsid = spdk_nvme_ctrlr_get_first_active_ns(ctrlr); nsid != 0;
			nsid = spdk_nvme_ctrlr_get_next_active_ns(ctrlr, nsid)) {

		struct spdk_nvme_ns *ns = spdk_nvme_ctrlr_get_ns(ctrlr, nsid);
		if (ns == NULL) continue;

		uint64_t sz = spdk_nvme_ns_get_size(ns);
        printf("[%d][Size:%juGB]\n", nsid, sz / (1024 * 1024 * 1024));
        do_write(ctrlr, ns);
        do_read(ctrlr, ns);
		
	}

#endif

}

int main(int argc, char** argv)
{
    int res;
    struct spdk_env_opts opts;

    printf("Hello World!\n");

    spdk_env_opts_init(&opts);

    res = spdk_env_init(&opts);
    printf("spdk_env_init() = %d\n", res);

    res = spdk_vmd_init();
    printf("spdk_vmd_init() = %d\n", res);

    res = spdk_nvme_probe(NULL, NULL, fun1, fun2, NULL);
    printf("spdk_nvme_probe() = %d\n", res);

    return 0;
}











