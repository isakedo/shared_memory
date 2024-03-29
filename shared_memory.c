
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/ioctl.h>
#include <time.h>
#include <fcntl.h>
#include <string.h>

/* Shutdown message ID */
#define SHUTDOWN_MSG	0xEF56A55A
#define PING		0xEF56A559
#define WRITE_NOW	0xEF56A558

#define MEM_SIZE (sizeof(unsigned long)*1024)

struct _payload {
	unsigned int message;
        unsigned long size;
        off_t data;
};

static int fd, fdsm;

struct _payload *i_payload;
struct _payload *r_payload;


#define RPMSG_GET_KFIFO_SIZE 1
#define RPMSG_GET_AVAIL_DATA_SIZE 2
#define RPMSG_GET_FREE_SPACE 3

#define RPMSG_HEADER_LEN 16
#define MAX_RPMSG_BUFF_SIZE (512 - RPMSG_HEADER_LEN)
#define PAYLOAD_MIN_SIZE	1
#define PAYLOAD_MAX_SIZE	(MAX_RPMSG_BUFF_SIZE - 24)
#define NUM_PAYLOADS		(PAYLOAD_MAX_SIZE/PAYLOAD_MIN_SIZE)



int main(int argc, char *argv[])
{
	int flag = 1;
	int shutdown_msg = SHUTDOWN_MSG;
	int ping_msg = PING;
	int writenow_msg = WRITE_NOW;
	int cmd, ret;
	int size, bytes_rcvd, bytes_sent;
	int opt;
	char *rpmsg_dev="/dev/rpmsg0";
	char *shared_mem="/dev/sharedmemory";
	while ((opt = getopt(argc, argv, "d:")) != -1) {
		switch (opt) {
		case 'd':
			rpmsg_dev = optarg;
			break;
		default:
			printf("getopt return unsupported option: -%c\n",opt);
			break;
		}
	}
	printf("\r\n Shared memory test start \r\n");

	printf("\r\n Open rpmsg dev! \r\n");

	fd = open(rpmsg_dev, O_RDWR | O_NONBLOCK);

	if (fd < 0) {
		perror("Failed to open rpmsg file /dev/rpmsg0.");
		return -1;
	}


	fdsm = open(shared_mem, O_RDWR);//prueba abrir el módulo

	if (fdsm < 0) {
		perror("Failed to open /dev/sahredmemory.");
		return -1;
	}

	off_t address = ioctl(fdsm,0,0);

	printf("\r\n Query internal info .. \r\n");

	ioctl(fd, RPMSG_GET_KFIFO_SIZE, &size);

	printf(" rpmsg kernel fifo size = %u \r\n", size);

	ioctl(fd, RPMSG_GET_FREE_SPACE, &size);

	printf(" rpmsg kernel fifo free space = %u \r\n", size);

	i_payload = (struct _payload *)malloc(sizeof(unsigned int) +
			sizeof(unsigned long) + PAYLOAD_MAX_SIZE);
	r_payload = (struct _payload *)malloc(sizeof(unsigned int) +
			sizeof(unsigned long) + PAYLOAD_MAX_SIZE);

	if (i_payload == 0 || r_payload == 0) {
		printf("ERROR: Failed to allocate memory for payload.\n");
		return -1;
	}

	while (flag == 1) {
		printf("\r\n **************************************** \r\n");
		printf(" Please enter command and press enter key\r\n");
		printf(" **************************************** \r\n");
		printf(" 1 - Bare metal writes one 4B data\r\n");
		printf(" 2 - Linux writes one 4B data\r\n");
		printf(" 3 - Read one 4B data\r\n");
		printf(" 4 - Send data to remote core, retrieve the echo");
		printf(" and validate its integrity .. \r\n");
		printf(" 5 - Quit this application .. \r\n");
		printf(" CMD>");
		ret = scanf("%d", &cmd);
		if (!ret) {
			while (1) {
				if (getchar() == '\n')
					break;
			}

			printf("\r\n invalid command\r\n");
			continue;
		}

		if (cmd == 1) {
			
			i_payload->message = writenow_msg;
			i_payload->data = address;
			i_payload->size = sizeof(unsigned int) + sizeof(unsigned long) + 
				sizeof(i_payload->data);
			write(fd,i_payload,i_payload->size);

			printf("\r\n one 4B data writen by bare metal \r\n");

		} else if (cmd == 2) {

			char buf[] = "1357"; 
			write(fdsm,buf,sizeof(buf));

			printf("\r\n one 4B data writen by bare metal \r\n");

		} else if (cmd == 3) {

			char buf[4];
			read(fdsm,buf,sizeof(buf));

			printf("\r\nDATA: %x \r\n",buf[0]);
			printf("DATA: %x \r\n",buf[1]);
			printf("DATA: %x \r\n",buf[2]);
			printf("DATA: %x \r\n",buf[3]);

		} else if (cmd == 4) {

			i_payload->message = ping_msg;
			i_payload->data = "Echo";
			i_payload->size = sizeof(unsigned int) + sizeof(unsigned long) + 
				sizeof(i_payload->data);

			bytes_sent = write(fd,i_payload,i_payload->size);

			if (bytes_sent <= 0) {
				printf("\r\n Error sending data");
				printf(" .. \r\n");
				break;
			}

			printf("echo test: sent : %s\n", i_payload->data);

			bytes_rcvd = read(fd, r_payload,(sizeof(unsigned int) +
					sizeof(unsigned long)) + PAYLOAD_MAX_SIZE);
			while (bytes_rcvd <= 0) {
				usleep(10000);
				bytes_rcvd = read(fd, r_payload, (sizeof(unsigned int) +
					sizeof(unsigned long)) + PAYLOAD_MAX_SIZE);
			}

			printf("echo test: received : %s\n", r_payload->data);

		} else if (cmd == 5) {
			flag = 0;
			/* Send shutdown message to remote */
			i_payload->message = shutdown_msg;
			i_payload->size = sizeof(unsigned int) + sizeof(unsigned long);
			write(fd,i_payload,i_payload->size);
			sleep(1);
			printf("\r\n Quitting application .. \r\n");
			printf("  Shared memory test end \r\n");
		} else {
			printf("\r\n invalid command! \r\n");
		}
	}

	free(i_payload);
	free(r_payload);

	close(fd);
	close(fdsm);
	return 0;
}




