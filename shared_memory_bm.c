/*
 * Copyright (c) 2014, Mentor Graphics Corporation
 * All rights reserved.
 *
 * Copyright (C) 2015 Xilinx, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of Mentor Graphics Corporation nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/**************************************************************************************
* This is a sample demonstration application that showcases usage of rpmsg
* This application is meant to run on the remote CPU running bare-metal code.
* It echoes back data that was sent to it by the master core.
*
* The application calls init_system which defines a shared memory region in
* MPU settings for the communication between master and remote using
* zynqMP_r5_map_mem_region API,it also initializes interrupt controller
* GIC and register the interrupt service routine for IPI using
* zynqMP_r5_gic_initialize API.
*
* Echo test calls the remoteproc_resource_init API to create the
* virtio/RPMsg devices required for IPC with the master context.
* Invocation of this API causes remoteproc on the bare-metal to use the
* rpmsg name service announcement feature to advertise the rpmsg channels
* served by the application.
*
* The master receives the advertisement messages and performs the following tasks:
* 	1. Invokes the channel created callback registered by the master application
* 	2. Responds to remote context with a name service acknowledgement message
* After the acknowledgement is received from master, remoteproc on the bare-metal
* invokes the RPMsg channel-created callback registered by the remote application.
* The RPMsg channel is established at this point. All RPMsg APIs can be used subsequently
* on both sides for run time communications between the master and remote software contexts.
*
* Upon running the master application to send data to remote core, master will
* generate the payload and send to remote (bare-metal) by informing the bare-metal with
* an IPI, the remote will send the data back by master and master will perform a check
* whether the same data is received. Once the application is ran and task by the
* bare-metal application is done, master needs to properly shut down the remote
* processor
*
* To shut down the remote processor, the following steps are performed:
* 	1. The master application sends an application-specific shut-down message
* 	   to the remote context
* 	2. This bare-metal application cleans up application resources,
* 	   sends a shut-down acknowledge to master, and invokes remoteproc_resource_deinit
* 	   API to de-initialize remoteproc on the bare-metal side.
* 	3. On receiving the shut-down acknowledge message, the master application invokes
* 	   the remoteproc_shutdown API to shut down the remote processor and de-initialize
* 	   remoteproc using remoteproc_deinit on its side.
*
**************************************************************************************/

#include "xil_printf.h"
#include "rsc_table.h"

#define SHUTDOWN_MSG	0xEF56A55A
#define PING			0xEF56A559
#define WRITE_NOW		0xEF56A558

struct _payload {
		unsigned int message;
        unsigned long size;
        off_t data;
};

/* Global functions and variables */
extern const struct remote_resource_table resources;
extern int init_system(void);
extern void cleanup_system(void);

extern struct rproc_info_plat_local proc_table;

/* Local variables */
static struct rpmsg_endpoint *rp_ept;
static struct remote_proc *proc = NULL;
static struct rsc_table_info rsc_info;
static int evt_chnl_deleted = 0;

/*-----------------------------	------------------------------------------------*
 *  RPMSG callbacks setup by remoteproc_resource_init()
 *-----------------------------------------------------------------------------*/

static void rpmsg_read_cb(struct rpmsg_channel *rp_chnl, void *data, int len,
                void * priv, unsigned long src)
{
	(void)priv;
	(void)src;

	struct _payload *r_payload = data;

	if (r_payload->message == WRITE_NOW) {

		char* address = r_payload->data;
		char buf[] = "2468";
		memcpy(address,buf,sizeof(buf));

		return;
	}
	else if (r_payload->message == PING) {

		/* send back the data */
		rpmsg_send(rp_chnl, data, len);

		return;
	}
	/* On reception of a shutdown we signal the application to terminate */
	else if (r_payload->message == SHUTDOWN_MSG) {
		evt_chnl_deleted = 1;
		return;
	}

}

static void rpmsg_channel_created(struct rpmsg_channel *rp_chnl)
{
	rp_ept = rpmsg_create_ept(rp_chnl, rpmsg_read_cb, RPMSG_NULL,
				RPMSG_ADDR_ANY);
}

static void rpmsg_channel_deleted(struct rpmsg_channel *rp_chnl)
{
	(void)rp_chnl;

	rpmsg_destroy_ept(rp_ept);
	rp_ept = NULL;
}


/*-----------------------------------------------------------------------------*
 *  Application entry point
 *-----------------------------------------------------------------------------*/
int main(void)
{
	int status = 0;

	/* Initialize HW and SW components/objects */
	init_system();

	/* Resource table needs to be provided to remoteproc_resource_init() */
	rsc_info.rsc_tab = (struct resource_table *)&resources;
	rsc_info.size = sizeof(resources);

	/* Initialize OpenAMP framework */
	xil_printf("Initializing OpenAMP...\n");
	status = remoteproc_resource_init(&rsc_info, &proc_table,
				rpmsg_channel_created,
				rpmsg_channel_deleted, rpmsg_read_cb,
				&proc, 0);
	if (RPROC_SUCCESS != status) {
		xil_printf("Error: initializing OpenAMP framework\n");
	} else {
		xil_printf("Waiting for events...\n");
		do {
			hil_poll(proc->proc, 0);
		} while (!evt_chnl_deleted);

		/* disable interrupts and free resources */
		xil_printf("Stopping OpenAMP...\n");
		remoteproc_resource_deinit(proc);
	}

	cleanup_system();

	/* We shall never exit main, just wait forever */
	while (1) {
		__asm__("wfi\n\t");
	}

	/* suppress compilation warnings*/
	return 0;
}
