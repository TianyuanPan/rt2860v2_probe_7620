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

#include <linux/seq_file.h>

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



spinlock_t mac_table_lock;

index_t mac_table_index;
index_t *cur_index = NULL;

mac_signal_t procfs_mac_table_info[TABLE_MAX_LEN];

static struct proc_dir_entry *mac_table_entry;


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

/*
 *  s:     almost always ignored
 *  pos:   integer position indicateing where to start
 *         need not be a byte position
 */
static void *seq_seq_start(struct seq_file *s, loff_t *pos)
{
//    PDEBUG("position is %d/n", *pos);
    if (*pos >= TABLE_MAX_LEN)
        return NULL;
    return  procfs_mac_table_info + *pos;
}

/*
 *  v:       is the iterator as returned from previous call to start or next
 *  return:  NULL means nothing left
 */
static void *seq_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
//    PDEBUG("next: %d/n", *pos);
    (*pos) = ++(*pos);
    if (*pos >= TABLE_MAX_LEN)
        return NULL;
    return  procfs_mac_table_info + *pos;
}

static void seq_seq_stop(struct seq_file *s, void *v)
{
    /* Actually, there's nothing to do here */
	return;
}

static int seq_seq_show(struct seq_file *seq, void *v)
{
	mac_signal_t *ptr = (mac_signal_t *)v;

	LOCK_MAC_TABLE();

	if( ptr->c_signal < 0) {


			seq_printf(seq, "[%d] %02x:%02x:%02x:%02x:%02x:%02x\n",
					ptr->c_signal,
					ptr->c_mac[0],
					ptr->c_mac[1],
					ptr->c_mac[2],
					ptr->c_mac[3],
					ptr->c_mac[4],
					ptr->c_mac[5]);

			ptr->c_signal = 0;
			memset(ptr->c_mac, 0, MAC_ADDR_LEN);
	}

	UNLOCK_MAC_TABLE();
    return 0;
}
/*
 * Tie the sequence operators up.
 */
static struct seq_operations seq_seq_ops = {
    .start = seq_seq_start,
    .next  = seq_seq_next,
    .stop  = seq_seq_stop,
    .show  = seq_seq_show
};

/*
 * Now to implement the /proc file we need only make an open
 * method which sets up the sequence operators.
 */
static int seq_proc_open(struct inode *inode, struct file *file)
{
    return seq_open(file, &seq_seq_ops);
}

static size_t proc_mac_table_write(struct file *file, const char __user *buffer,
		                         size_t count, loff_t *ppos)
{
	return 0;
}

static const struct file_operations procfs_mac_table_info_fops =
{
		.owner = THIS_MODULE,
		.open = seq_proc_open,
		.read = seq_read,
		.write = proc_mac_table_write,
		.llseek = seq_lseek,
		.release = seq_release,
};



int wl_proc_init(void)
{
	spin_lock_init(&mac_table_lock);

	if (InitIndexTListSize(&mac_table_index, TABLE_MAX_LEN) != 0)
	return -ENOMEM;

	cur_index = &mac_table_index;

	mac_table_entry = proc_create_data(PROOC_ENTRY_NAME, 0444, NULL, &procfs_mac_table_info_fops, procfs_mac_table_info);

	if(!mac_table_entry) {
		printk(KERN_ERR "proc_mac_init_ERR: proc_create_data mac_probe_info error!!!!\n");
		DestoryIndexTList(&mac_table_index);
		return -1;
	}

	return 0;
}

int wl_proc_exit(void)
{
	if (mac_table_entry){
	  remove_proc_entry(PROOC_ENTRY_NAME, NULL);
	  DestoryIndexTList(&mac_table_index);
	}
	return 0;
}

#endif /* CONFIG_PROC_FS */



