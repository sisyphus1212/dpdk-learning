

#include "loadbalancer.h"
#include <vnet/plugin/plugin.h>


int loadbalancer_set_targets(loadbalancer_main_t *lb, u32 sw_if_index_source, 
		u32 *sw_if_index_targets) {

	u32 *sw_if_index;

	debug("debug:[%s:%s:%d] loadbalancer_node.index:%d\n", __FILE__, __func__, __LINE__, loadbalancer_node.index);
	loadbalancer_runtime_t *rt = vlib_node_get_runtime_data(lb->vlib_main, loadbalancer_node.index);

	debug("debug:[%s:%s:%d] sw_if_index_source:%d\n", __FILE__, __func__, __LINE__, sw_if_index_source);
	
	rt->sw_if_index_source = sw_if_index_source;
	vec_foreach(sw_if_index, sw_if_index_targets) {
		vec_add1(rt->sw_if_target, *sw_if_index);
	}
	return 0;
}


static clib_error_t *set_lb_target_command_fn (vlib_main_t * vm,
                                     unformat_input_t * input,
                                     vlib_cli_command_t * cmd) {
	//flowtable_main_t *fm = &flowtable_main;
	loadbalancer_main_t *lb = &loadbalancer_main;
	loadbalancer_runtime_t *rt = vlib_node_get_runtime_data(vm, loadbalancer_node.index);
	//loadbalancer_main_t *lb = rt->lbm;
	
	u32 sw_if_index_source = ~0;
	u32 sw_if_index = ~0;
	u32 *sw_if_index_targets = rt->sw_if_target;
	
	int source = 1;

	debug("debug:[%s:%s:%d] input: %s, vnet_main: %p\n", __FILE__, __func__, __LINE__, 
		input->buffer, lb->vnet_main);

	while (unformat_check_input(input) != UNFORMAT_END_OF_INPUT) {
		
		if (unformat(input, "to")) {
			source = 0;
		} else if (unformat(input, "%U", unformat_vnet_sw_interface, 
			lb->vnet_main, &sw_if_index)) {
			debug("debug:[%s:%s:%d] sw_if_index:%d\n", __FILE__, __func__, __LINE__, sw_if_index);
			if (source) {
				sw_if_index_source = sw_if_index;
			} else {
				vec_add1(sw_if_index_targets, sw_if_index);
			} 
		} else break;

		debug("[%s:%s:%d]: token: %s\n", __FILE__, __func__, __LINE__, input->buffer);
	} 

	if (sw_if_index == ~0) {
		return clib_error_return(0, "No Source Interface specified");
	}

	if (vec_len(sw_if_index_targets) <= 0) {
		return clib_error_return(0, "No Target Interface specified");
	}


	int rv = loadbalancer_set_targets(lb, sw_if_index_source, sw_if_index_targets);
	if (rv) {
		return clib_error_return(0, "set interface loadbalancer return %d", rv);
	}
	return 0;
}


VLIB_CLI_COMMAND(set_interface_loadbalanced_command) = {
	.path = "set interface loadbalanced",
	.short_help = "set interface loadbalanced <interface> to <interfaces-list>",
	.function = set_lb_target_command_fn,
};


