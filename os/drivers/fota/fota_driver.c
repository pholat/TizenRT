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

/************************************************************************************
 * Included Files
 ************************************************************************************/

#include <tinyara/config.h>

#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>
#include <debug.h>
#include <tinyara/arch.h>
#include <tinyara/fs/fs.h>
#include <tinyara/fs/ioctl.h>
#include <tinyara/fota/fota.h>
#include "partition_table.h"

/************************************************************************************
 * Definitions
 ************************************************************************************/

/************************************************************************************
 * Private Types
 ************************************************************************************/

/************************************************************************************
 * Private Function Prototypes
 ************************************************************************************/
const char* fota_turbo="/dev/fotachar";

static int fota_open(FAR struct file *filep);
static int fota_close(FAR struct file *filep);
static ssize_t fota_read(FAR struct file *filep, FAR char *buffer, size_t buflen);
static ssize_t fota_write(FAR struct file *filep, FAR const char *buffer, size_t buflen);
static int fota_ioctl(FAR struct file *filep, int cmd, unsigned long arg);

/************************************************************************************
 * Private Variables
 ************************************************************************************/

static const struct file_operations g_fotaops = {
	fota_open,					/* open */
	fota_close,					/* close */
	fota_read,					/* read */
	fota_write,					/* write */
	0,							/* seek */
	fota_ioctl					/* ioctl */
};

static sem_t g_fota_open_sem;
static int fota_fd;
static bool g_fota_dev_written = false;
static int foo_set_partition(uint32_t part_id);

/************************************************************************************
 * Private Functions
 ************************************************************************************/
/************************************************************************************
 * Name: fota_write
 ************************************************************************************/
static ssize_t fota_write(FAR struct file *filep, FAR const char *buffer, size_t buflen)
{
	FAR struct inode *inode = filep->f_inode;
	FAR fota_dev_t *dev = inode->i_private;
	int ret = ERROR;

	if (!fota_fd>0) {
		dbg(" device is not opened, ret = %d\n", ret);
		set_errno(EBADF);
		return ret;
	}

	ret = dev->fota_write(buffer, buflen);
	if (ret == OK && !g_fota_dev_written) {
		g_fota_dev_written = true;
	}

	return ret;
}

/************************************************************************************
 * Name: fota_read
 ************************************************************************************/
static ssize_t fota_read(FAR struct file *filep, FAR char *buffer, size_t buflen)
{
	FAR struct inode *inode = filep->f_inode;
	FAR fota_dev_t *dev = inode->i_private;
	int ret = ERROR;

	if (!fota_fd) {
		dbg(" device is not opened, ret = %d\n", ret);
		set_errno(EBADF);
		return ret;
	}

	ret = dev->fota_read(buffer, buflen);
	return ret;
}

/************************************************************************************
 * Name: fota_ioctl
 ************************************************************************************/
static int fota_ioctl(FAR struct file *filep, int cmd, unsigned long arg)
{
	FAR struct inode *inode = filep->f_inode;
	FAR fota_dev_t *dev = inode->i_private;
	int ret = ERROR;

	if (!fota_fd) {
		dbg(" device is not opened, ret = %d\n", ret);
		set_errno(EBADF);
		return ret;
	}

	switch (cmd) {
	case FOTA_GET_PART:
		if (dev->fota_get_partition) {
			ret = dev->fota_get_partition();
		}
		break;
	case FOTA_SET_PART:
		if (dev->fota_set_partition) {
			ret = dev->fota_set_partition((uint32_t)arg);
		}
		break;
	case FOTA_SET_BIN:
		if (dev->fota_set_binary) {
			ret = dev->fota_set_binary((uint32_t)arg);
		}
		break;
	case FOTA_SET_PARAM:
		if (dev->fota_set_bootparam) {
			ret = dev->fota_set_bootparam(FOTA_BOOTPARAM_TYPE_BINSTATE);
			ret = dev->fota_set_bootparam(FOTA_BOOTPARAM_TYPE_BOOTMODE);
		}
		break;
	default:
		set_errno(EINVAL);
		break;
	}

	return ret;
}

/************************************************************************************
 * Name: fota_close
 *
 * Description:
 *   This routine is called when the fota driver closed
 ************************************************************************************/
static int fota_close(FAR struct file *filep)
{
	FAR struct inode *inode = filep->f_inode;
	FAR fota_dev_t *dev = inode->i_private;

	if (!fota_fd) {
		dbg(" device is not opened, ret = %d\n", ERROR);
		set_errno(EBADF);
		return ERROR;
	}

	if (g_fota_dev_written) {
		/* Lets flush, if anything buffered */
		dev->fota_write_flush();
		g_fota_dev_written = false;
	}
    close(fota_fd);
    bchdev_unregister(fota_turbo);
    fota_fd=0;

	sem_post(&g_fota_open_sem);
	return OK;
}

/************************************************************************************
 * Name: fota_open
 *
 * Description:
 *  fota driver open
 ************************************************************************************/
static int fota_open(FAR struct file *filep)
{
	int ret;
    char mntdev[16]={0};
    int minor=0;
	while (sem_wait(&g_fota_open_sem) != OK) ;
    if( foo_set_partition(minor) ) {
        printf("error %d\n", __LINE__);
        return -1;
    }
    minor=get_ota_partition();
    snprintf(mntdev, 16, "/dev/mtdblock%d", minor);
    if ( (ret = bchdev_register(mntdev,fota_turbo,false))!=0 ) {
        printf("error register %d\n",ret);
    }
    if ( !((fota_fd=open( fota_turbo, O_RDWR))>0) ) {
        printf("error: %s :%d:%d:%s\n",mntdev, __LINE__, fota_fd, strerror(errno) );
        ret=fota_fd;
    } else {
        ret =OK;
    }
	return ret;
}

/************************************************************************************
 * Public Functions
 ************************************************************************************/

/************************************************************************************
 * Name: fota_register
 *
 * Description:
 *   Register fota driver
 *
 ************************************************************************************/


int fota_register(FAR fota_dev_t *dev)
{
	const char *path = "/dev/fota";

	sem_init(&g_fota_open_sem, 0, 1);

	dbg("Registering %s\n", path);

	if (!dev) {
		return ERROR;
	}

	return register_driver(path, &g_fotaops, 0666, dev);
}

/// check address space in which we are
static int foo_get_partition(void)
{
    int retval=-1; 
    int i=0;
    for (; i < PART_MAP_SIZE; ++i)
    {
        if ( ((void*)&i > (void*)g_partition_map[i].start) && ((void*)&i < (void*)g_partition_map[i].end) )
            retval=1;
            break;
    }
    return retval;
}

int get_ota_partition(void)
{
    int i=0,retval=-1;
    for (; i < PART_MAP_SIZE; ++i)
    {
        if ( strcmp( "ota", g_partition_map[i].name) == 0 ) {
            retval=i;
            break;
        }
    }
    return retval;
}

/// in part_id returns ota partition num
static int foo_set_partition(uint32_t part_id)
{
    return 0;
}

static int foo_set_binary(uint32_t bin_id)
{
    // for ftl not needed
    return 0;
}

static int foo_set_bootparam(fota_bootparam_type_t bootparam_type)
{
    // bootloader does that
    return 0;
}

static int foo_read(FAR const char *buffer, size_t buflen)
{
    int ret=-1;
    return ret;
}

static int foo_write(FAR const char *buffer, size_t buflen)
{
    return write(fota_fd,buffer,buflen);
}

static int foo_write_flush(void) 
{
    if ( fota_fd > 0 ) {
        fflush(fota_fd);
        return 0;
    } else {
        return -1;
    }
}

static int foo_erase(void)
{
    return -1;
}

static fota_dev_t g_fotadevice = {
    .fota_get_partition=foo_get_partition,
    .fota_set_partition=foo_set_partition,
    .fota_set_binary=foo_set_binary,
    .fota_set_bootparam=foo_set_bootparam,
    .fota_read=foo_read,
    .fota_write=foo_write,
    .fota_write_flush=foo_write_flush,
    .fota_erase=foo_erase,
    .mtd=0,
};

void up_fotainitialize(void) {
	
    (void)fota_register(&g_fotadevice);
}
