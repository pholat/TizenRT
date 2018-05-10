/****************************************************************************
 *
 * Copyright 2016 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/
/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <stdio.h>
#include <tinyara/config.h>
#ifdef CONFIG_TASH
#include <apps/shell/tash.h>
#endif
#include <apps/system/fota_hal.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: fota_sample
 *
 * Description:
 *      fota sample application
 ****************************************************************************/

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int fota_sample(int argc, char *argv[])
#endif
{
	int ret = ERROR;
	uint32_t part_id;
	fotahal_handle_t fotahal_handle;

	printf("***********************************************************\n");
	printf(" fota update is in progress !!!\n");

	/* Example call sequence to use fota hal */

	/* open fota hal, get the handle to work with it */
	if ((fotahal_handle = fotahal_open()) == NULL) {
		printf("%s : fotahal_open error\n", __func__);
		goto open_error;
	}

	/* Get the current running partition */
	part_id = fotahal_get_partition(fotahal_handle);
	if (part_id == -1) {
		printf("fotahal_get_partition error\n");
		goto part_error;
	}

	printf(" current running partition is  [ OTA%d ]\n", part_id);


	printf("Wait for socket\n");
	int socket_desc, client_sock, c, read_size;
	struct sockaddr_in server, client;
    // TODO make it so that there will be 4kB stack available for this thread ...
	static char client_message[4096];

	//Create socket
	socket_desc = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_desc == -1) {
		printf("Could not create socket");
		return -1;
	}
	printf("Socket created");

	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(12345);

	//Bind
	if (bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0) {
		//print the error message
		perror("bind failed. Error");
		close(socket_desc);
		return 1;
	}
	printf("bind done\n");

	//Listen
	if (listen(socket_desc, 3) < 0) {
		printf("listen failed\n");
		close(socket_desc);
		return 1;
	}
	//Accept and incoming connection
	printf("Waiting for incoming connections...\n");
	c = sizeof(struct sockaddr_in);

	//accept connection from an incoming client
	client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t *)&c);
	if (client_sock < 0) {
		perror("accept failed");
		close(socket_desc);
		return 1;
	}
	printf("Connection accepted\n");

	//Receive a message from client
	while ((read_size = recv(client_sock, client_message, 4096, 0)) > 0) {
        printf(".");
        fflush(stdout);
        if (fotahal_write(fotahal_handle, client_message, read_size) != FOTAHAL_RETURN_SUCCESS) {
            printf("%s : fotahal_write error\n", __func__);
            goto write_error;
        }
    }

	if (read_size == 0) {
		printf("client disconnected\n");
	} else if (read_size == -1) {
		printf("recv failed\n");
	}

	close(client_sock);
	close(socket_desc);

	if (fotahal_close(fotahal_handle) != FOTAHAL_RETURN_SUCCESS) {
		printf("%s : fotahal_close error\n", __func__);
		goto close_error;
	}

	printf(" fota update successfully downloaded !!!\n");
	printf(" Please reset your board to continue fota update !!!\n");
	printf("***********************************************************\n");
	ret = OK;

close_error:
write_error:
part_error:
	fotahal_close(fotahal_handle);
open_error:
	return ret;
}

