/*
 ***************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology 5th Rd.
 * Science-based Industrial Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright 2002, Ralink Technology, Inc.
 *
 * All rights reserved. Ralink's source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of Ralink Tech. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************

 Module Name:
 rt_proc.c

 Abstract:
 Create and register proc file system for ralink device

 Revision History:
 Who         When            What
 --------    ----------      ----------------------------------------------
 */

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>

#include "rt_config.h"

int wl_proc_init(void);
int wl_proc_exit(void);

#ifdef CONFIG_RALINK_RT2880
#define PROCREG_DIR             "rt2880"
#endif /* CONFIG_RALINK_RT2880 */

#ifdef CONFIG_RALINK_RT3052
#define PROCREG_DIR             "rt3052"
#endif /* CONFIG_RALINK_RT3052 */

#ifdef CONFIG_RALINK_RT2883
#define PROCREG_DIR             "rt2883"
#endif /* CONFIG_RALINK_RT2883 */

#ifdef CONFIG_RALINK_RT3883
#define PROCREG_DIR             "rt3883"
#endif /* CONFIG_RALINK_RT3883 */

#ifdef CONFIG_RALINK_RT5350
#define PROCREG_DIR             "rt5350"
#endif /* CONFIG_RALINK_RT5350 */

#ifndef PROCREG_DIR
#define PROCREG_DIR             "rt2880"
#endif /* PROCREG_DIR */

#ifdef CONFIG_PROC_FS




static struct proc_dir_entry *entry_wl_beacon_mac = NULL;
int ProbeRssi[MAX_MACLIST_LENGTH];
UCHAR GLOBAL_AddrLocal[MAX_MACLIST_LENGTH][MAC_ADDR_LEN];

//struct mutex mac_list_table_lock;


index_t mac_list_index;
index_t *cur_index = NULL;

static int InitIndexTListSize(index_t *index, int size)
{
	int i;
	index_t *head = NULL,
	*move = NULL,
	*new = NULL;
	head = index;
	move = index;
	new = NULL;

	i = size;
	if ( i < 1 ) {
		printk("Init Index size error, size should be great than 1");
		return -1;
	}
	if (!move) {
		printk("Init Index size error, index is NULL");
		return -1;
	}
	head->index = 0;
	head->next = index;

	for (i = 1; i < size; i++) {
		new = (index_t*)vmalloc(sizeof(index_t));
		if(!new) {
			printk("Init index_t size vmalloc error");
			return -1;
		}
		new->index = i;
		new->next = head;
		move->next = new;
		move = new;
	}

	return 0;
}

static void DestoryIndexTList(index_t *index)
{
	index_t *move = NULL,
	*head = NULL,
	*p = NULL;

	head = index;
	move = head->next;

	while(move != head) {
		p = move->next;
		vfree(move);
		move = p;
	}
}

static int maclist_proc_show(struct seq_file *seq, void *v)
{
	index_t *item = NULL,
	*head = NULL;
	head = &mac_list_index;
	item = head;

	do {
		if ( ProbeRssi[item->index] < 0){
			seq_printf(seq,"[%d] %02x:%02x:%02x:%02x:%02x:%02x\n",
					ProbeRssi[item->index],
					GLOBAL_AddrLocal[item->index][0],
					GLOBAL_AddrLocal[item->index][1],
					GLOBAL_AddrLocal[item->index][2],
					GLOBAL_AddrLocal[item->index][3],
					GLOBAL_AddrLocal[item->index][4],
					GLOBAL_AddrLocal[item->index][5]);
/*
			printk(KERN_ERR "show: [%d] %02x:%02x:%02x:%02x:%02x:%02x\n",
					ProbeRssi[item->index],
					GLOBAL_AddrLocal[item->index][0],
					GLOBAL_AddrLocal[item->index][1],
					GLOBAL_AddrLocal[item->index][2],
					GLOBAL_AddrLocal[item->index][3],
					GLOBAL_AddrLocal[item->index][4],
					GLOBAL_AddrLocal[item->index][5]);
*/
			ProbeRssi[item->index] = 0;
			memset(GLOBAL_AddrLocal[item->index], 0, MAC_ADDR_LEN);
		}
		item = item->next;

	}while(item != head);

	return 0;
}

static int maclist_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file,maclist_proc_show,inode->i_private);
}

static ssize_t maclist_proc_write(struct file *file, const char *buffer, size_t len, loff_t *off)
{
	return 0;
}

static const struct file_operations maclist_proc_fops = {
	.owner = THIS_MODULE,
	.open = maclist_proc_open,
	.write = maclist_proc_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

int wl_proc_init(void)
{

	if (InitIndexTListSize(&mac_list_index, MAX_MACLIST_LENGTH) != 0)
	return -ENOMEM;

//	mutex_init(&mac_list_table_lock);

	cur_index = &mac_list_index;

	entry_wl_beacon_mac = proc_create_data("mac_probe_info", 0444, NULL, &maclist_proc_fops, GLOBAL_AddrLocal);

	if(!entry_wl_beacon_mac) {
		DestoryIndexTList(&mac_list_index);
		return -1;
	}

	return 0;
}

int wl_proc_exit(void)
{
	remove_proc_entry("mac_probe_info", NULL);
	DestoryIndexTList(&mac_list_index);
	return 0;
}
#endif /* CONFIG_PROC_FS */



