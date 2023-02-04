


#include "spdk/stdinc.h"
#include "spdk/nvme.h"
#include "spdk/vmd.h"
#include "spdk/nvme_zns.h"

#include "spdk/env.h"
#include "spdk/string.h"
#include "spdk/log.h"




struct ctrlr_entry {
	struct spdk_nvme_ctrlr *ctrlr;
	TAILQ_ENTRY(ctrlr_entry) link;
	char name[1024];
};

struct ns_entry {

	struct spdk_nvme_ctrlr *ctrlr;
	struct spdk_nvme_ns *ns;

	TAILQ_ENTRY(ns_entry) link;
	struct spdk_nvme_qpair *qpair;

};

struct spdk_nvme_sequence {

	struct ns_entry *ns_entry;
	char *buf;

	unsigned using_cmb_io;
	int is_completed;

};


static TAILQ_HEAD(, ctrlr_entry) g_controllers = TAILQ_HEAD_INITIALIZER(g_controllers);
static TAILQ_HEAD(, ns_entry) g_namespaces = TAILQ_HEAD_INITIALIZER(g_namespaces);

static struct spdk_nvme_transport_id g_trid = {};

static bool g_vmd = false;


static void spdk_nvme_usage(const char *program_name)
{
	printf("%s [options]", program_name);
	printf("\t\n");
	printf("options:\n");
	printf("\t[-d DPDK huge memory size in MB]\n");
	printf("\t[-g use single file descriptor for DPDK memory segments]\n");
	printf("\t[-i shared memory group ID]\n");
	printf("\t[-r remote NVMe over Fabrics target address]\n");
	printf("\t[-V enumerate VMD]\n");
#ifdef DEBUG
	printf("\t[-L enable debug logging]\n");
#else
	printf("\t[-L enable debug logging (flag disabled, must reconfigure with --enable-debug)\n");
#endif
}


static int spdk_nvme_parse_args(int argc, char **argv, 
		struct spdk_env_opts *env_opts) {

	spdk_nvme_trid_populate_transport(&g_trid, SPDK_NVME_TRANSPORT_PCIE);
	snprintf(g_trid.subnqn, sizeof(g_trid.subnqn), "%s", SPDK_NVMF_DISCOVERY_NQN);

	int op, rc;
	while ((op = getopt(argc, argv, "d:gi:r:L:V")) != -1) {

		switch (op) {

		case 'V':
			g_vmd = true;
			break;

		case 'i':
			env_opts->shm_id = spdk_strtol(optarg, 10);
			if (env_opts->shm_id < 0) {
				fprintf(stderr, "Invalid shared memory Id\n");
				return env_opts->shm_id;
			}
			break;

		case 'g':
			env_opts->hugepage_single_segments = true;
			break;

		case 'r':
			if (spdk_nvme_transport_id_parse(&g_trid, optarg) != 0) {
				fprintf(stderr, "Error parsing transport address\n");
				return 1;
			}
			break;

		case 'd':
			env_opts->mem_size = spdk_strtol(optarg, 10);
			if (env_opts->mem_size < 0) {
				fprintf(stderr, "Invalid DPDK memory size\n");
				return env_opts->mem_size;
			}
			break;

		case 'L':
			rc = spdk_log_set_flag(optarg);
			if (rc < 0) {
				fprintf(stderr, "unknown flag\n");
				spdk_nvme_usage(argv[0]);
				exit(EXIT_FAILURE);
			}
			break;
		default:
			spdk_nvme_usage(argv[0]);
			return 1;
		}

	}

	return 0;
}


static void spdk_nvme_register_ns(struct spdk_nvme_ctrlr *ctrlr, struct spdk_nvme_ns *ns) {

	if (!spdk_nvme_ns_is_active(ns)) {
		return ;
	}

	struct ns_entry *entry = malloc(sizeof(struct ns_entry));
	if (entry == NULL) {
		perror("ns_entry malloc");
		exit(1);
	}

	entry->ctrlr = ctrlr;
	entry->ns = ns;

	TAILQ_INSERT_TAIL(&g_namespaces, entry, link);

	printf("  Namespace ID: %d size: %juGB\n", spdk_nvme_ns_get_id(ns),
	       spdk_nvme_ns_get_size(ns) / (1024*1024*1024));
}


static bool _spdk_nvme_probe_cb(void *cb_ctx, const struct spdk_nvme_transport_id *trid,
				   struct spdk_nvme_ctrlr_opts *opts) {


	printf("Attaching to %s\n", trid->traddr);

	return true;

}


static void _spdk_nvme_attach_cb(void *cb_ctx, const struct spdk_nvme_transport_id *trid,
				    struct spdk_nvme_ctrlr *ctrlr,
				    const struct spdk_nvme_ctrlr_opts *opts) {

	struct ctrlr_entry *entry = malloc(sizeof(struct ctrlr_entry));
	if (entry == NULL) {
		perror("ctrlr entry malloc");
		exit(1);
	}

	printf("Attach to %s\n", trid->traddr);
	
	const struct spdk_nvme_ctrlr_data *cdata = spdk_nvme_ctrlr_get_data(ctrlr);

	snprintf(entry->name, sizeof(entry->name), "%-20.20s (%-20.20s)", cdata->mn, cdata->sn);

	entry->ctrlr = ctrlr;
	TAILQ_INSERT_TAIL(&g_controllers, entry, link);

	int nsid;
	for (nsid = spdk_nvme_ctrlr_get_first_active_ns(ctrlr); nsid != 0;
			nsid = spdk_nvme_ctrlr_get_next_active_ns(ctrlr, nsid)) {

		struct spdk_nvme_ns *ns = spdk_nvme_ctrlr_get_ns(ctrlr, nsid);
		if (ns == NULL) continue;

		spdk_nvme_register_ns(ctrlr, ns);
	}

}

static void spdk_nvme_reset_zone_complete(void *arg, 
	const struct spdk_nvme_cpl *completion) {

	struct spdk_nvme_sequence *sequence = arg;

	sequence->is_completed = 1;

	if (spdk_nvme_cpl_is_error(completion)) {

		spdk_nvme_qpair_print_completion(sequence->ns_entry->qpair, 
			(struct spdk_nvme_cpl*)completion);

		fprintf(stderr, "I/O error status: %s\n", spdk_nvme_cpl_get_status_string(&completion->status));
		fprintf(stderr, "Reset zone I/O failed, aborting run\n");

		sequence->is_completed = 2;
		exit(1);
		
	}

}

static void spdk_nvme_read_complete(void *ctx, const struct spdk_nvme_cpl *cpl) {

	struct spdk_nvme_sequence *sequence = ctx;

	sequence->is_completed = 1;

	if (spdk_nvme_cpl_is_error(cpl)) {
		spdk_nvme_qpair_print_completion(sequence->ns_entry->qpair, (struct spdk_nvme_cpl*)cpl);
		fprintf(stderr, "I/O error status: %s\n", spdk_nvme_cpl_get_status_string(&cpl->status));
		fprintf(stderr, "Read I/O failed, aborting run\n");
		sequence->is_completed = 2;
		exit(1);
	}

	printf("[%s:%d] --> %s", __func__, __LINE__, sequence->buf);
	spdk_free(sequence->buf);
}


static void spdk_nvme_write_complete(void *ctx, const struct spdk_nvme_cpl *cpl) {

	
	struct spdk_nvme_sequence *sequence = ctx;
	struct ns_entry *ns_entry = sequence->ns_entry;

	if (spdk_nvme_cpl_is_error(cpl)) {

		spdk_nvme_qpair_print_completion(sequence->ns_entry->qpair, (struct spdk_nvme_cpl *)cpl);
		fprintf(stderr, "I/O error status: %s\n", spdk_nvme_cpl_get_status_string(&cpl->status));
		fprintf(stderr, "Write I/O failed, aborting run\n");

		sequence->is_completed = 2;
		exit(1);
	}

	sequence->buf = spdk_zmalloc(0x1000, 0x1000, NULL, SPDK_ENV_SOCKET_ID_ANY, SPDK_MALLOC_DMA);
	int rc = spdk_nvme_ns_cmd_read(ns_entry->ns, ns_entry->qpair, sequence->buf, 
		0, 1, spdk_nvme_read_complete, (void*)sequence, 0);
	if (rc != 0) {
		fprintf(stderr, "starting read I/O failed\n");
		exit(1);
	}

}



static void spdk_nvme_reset_zone_and_wait_for_completion(struct spdk_nvme_sequence *sequence) {

	if (spdk_nvme_zns_reset_zone(sequence->ns_entry->ns, sequence->ns_entry->qpair,
		0, false, spdk_nvme_reset_zone_complete, sequence)) {

		fprintf(stderr, "starting reset zone I/O failed\n");
		exit(1);

	}

	while (!sequence->is_completed) {
		spdk_nvme_qpair_process_completions(sequence->ns_entry->qpair, 0);		
	}

	sequence->is_completed = 0;

}

static void spdk_nvme(void) {

	struct ns_entry *ns_entry;
	struct spdk_nvme_sequence sequence;
	size_t sz;

	TAILQ_FOREACH(ns_entry, &g_namespaces, link) {

		ns_entry->qpair = spdk_nvme_ctrlr_alloc_io_qpair(ns_entry->ctrlr, NULL, 0);
		if (ns_entry->qpair == NULL) {
			printf("Error: spdk_nvme_ctrlr_alloc_io_qpair() failed\n");
			return ;
		}

		sequence.using_cmb_io = 1;
		sequence.buf = spdk_nvme_ctrlr_map_cmb(ns_entry->ctrlr, &sz);
		if (sequence.buf == NULL || sz < 0x1000) {

			sequence.using_cmb_io = 0;
			sequence.buf = spdk_zmalloc(0x1000, 0x1000, NULL, 
				SPDK_ENV_SOCKET_ID_ANY, SPDK_MALLOC_DMA);
			
		}

		if (sequence.buf == NULL) {

			printf("Error: write buffer allocation failed\n");
			return ;
			
		}

		if (sequence.using_cmb_io) {

			printf("INFO: using controller memory buffer for IO\n");
 
		} else {

			printf("INFO: using host memory buffer for IO\n");

		}
		sequence.is_completed = 0;
		sequence.ns_entry = ns_entry;

		if (spdk_nvme_ns_get_csi(ns_entry->ns) == SPDK_NVME_CSI_ZNS) {
			spdk_nvme_reset_zone_and_wait_for_completion(&sequence);
		}

		snprintf(sequence.buf, 0x1000, "%s", "spdk nvme\n");

		int rc = spdk_nvme_ns_cmd_write(ns_entry->ns, ns_entry->qpair, sequence.buf,
			0, 1, spdk_nvme_write_complete, &sequence, 0);
		if (rc != 0) {
			fprintf(stderr, "starting write I/O failed\n");
			exit(1);
		}

		while(!sequence.is_completed) {
			spdk_nvme_qpair_process_completions(ns_entry->qpair, 0);
		}

		spdk_nvme_ctrlr_free_io_qpair(ns_entry->qpair);
	}

}

static void cleanup(void) {

	struct ns_entry *ns_entry, *tmp_ns_entry;
	struct ctrlr_entry *ctrlr_entry, *tmp_ctrlr_entry;

	struct spdk_nvme_detach_ctx *detach_ctx = NULL;

	TAILQ_FOREACH_SAFE(ns_entry, &g_namespaces, link, tmp_ns_entry) {

		TAILQ_REMOVE(&g_namespaces, ns_entry, link);
		free(ns_entry);
		
	}

	printf("[%s:%d] --> cleanup\n", __func__, __LINE__);

	TAILQ_FOREACH_SAFE(ctrlr_entry, &g_controllers, link, tmp_ctrlr_entry) {

		TAILQ_REMOVE(&g_controllers, ctrlr_entry, link);
		spdk_nvme_detach_async(ctrlr_entry->ctrlr, &detach_ctx);
		free(ctrlr_entry);

	}

	if (detach_ctx) {

		spdk_nvme_detach_poll(detach_ctx);

	}

}

int main(int argc, char *argv[]) {

	struct spdk_env_opts opts;

	spdk_env_opts_init(&opts);
	int rc = spdk_nvme_parse_args(argc, argv, &opts);
	if (rc != 0) return rc;

	opts.name = "spdk_nvme";
	if (spdk_env_init(&opts) < 0) {
		fprintf(stderr, "Unable to initiablize SPDK env\n");
		return 1;
	}

	printf("Initiazing NVMe Controllers\n");

	if (g_vmd && spdk_vmd_init()) {
		fprintf(stderr, "Failed to initialize VMD. Some NVMe devices can be unavailable.\n");
	}

	rc = spdk_nvme_probe(&g_trid, NULL, _spdk_nvme_probe_cb, _spdk_nvme_attach_cb, NULL);
	if (rc != 0) {
		goto exit;
	}

	if (TAILQ_EMPTY(&g_controllers)) {
		fprintf(stderr, "no NVMe controllers found\n");
		rc = 1;
		goto exit;
	}

	spdk_nvme();
	//cleanup();

	if (g_vmd) {
		printf("[%s:%d] --> spdk_vmd_fini\n", __func__, __LINE__);
		spdk_vmd_fini();
	}
	

exit:
	cleanup();
	spdk_env_fini();

	printf("[%s:%d] --> spdk_env_fini\n", __func__, __LINE__);
	
	return rc;
}


