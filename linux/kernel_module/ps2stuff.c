/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America
       	  
       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef __KERNEL__
#  define __KERNEL__
#endif
#ifndef MODULE
#  define MODULE
#endif

#include <linux/config.h>
#include <linux/module.h>

#include <linux/kernel.h>   /* printk() */
#include <linux/malloc.h>   /* kmalloc() */
#include <linux/fs.h>       /* everything... */
#include <linux/errno.h>    /* error codes */
#include <linux/types.h>    /* size_t */
#include <asm/page.h>
#include <asm/uaccess.h>
#include <asm/pgtable.h>    /* cache control */
#include <asm/addrspace.h>
#include <asm/io.h>	    /* virt_to_bus */

#include "ps2stuff_kmodule.h"

/********************************************
 * macros
 */

#ifdef _DEBUG
#  define dprintf( _msg, _args... ) printk( "ps2stuff:  " _msg "\n", ## _args );
#else
#  define dprintf( _msg, _args... )
#endif

#define wprintf( _msg, _args... ) printk( KERN_WARNING "ps2stuff:  " _msg "\n", ## _args );
#define eprintf( _msg, _args... ) printk( KERN_ERR "ps2stuff:  " _msg "\n", ## _args );

/********************************************
 * local data
 */

/* temporarily... */
static int ps2stuff_major = 60;

/* stuff to keep track of memory mapped regions of dma memory */
#define MAX_MMAPPED_REGIONS 160

typedef struct {
      unsigned int user_start, user_end, phys_start;
} pgl_mmapped_region_t;
	
static int num_mmapped_regions = 0;
static pgl_mmapped_region_t mmapped_regions[MAX_MMAPPED_REGIONS];

/********************************************
 * allocate/free dma memory
 */

static int
add_mmapped_region( unsigned int pa, unsigned int va, unsigned int byte_len )
{
   if ( num_mmapped_regions == MAX_MMAPPED_REGIONS ) {
      eprintf("too many mem areas allocated");
      return 0;
   }

   mmapped_regions[num_mmapped_regions].user_start = va;
   mmapped_regions[num_mmapped_regions].user_end = va + byte_len;
   mmapped_regions[num_mmapped_regions].phys_start = pa;
   num_mmapped_regions++;

   return 1;
}

static unsigned int
lookup_user_addr( unsigned int addr )
{
   int i;
   unsigned int phys_addr = 0;

   for ( i = 0; i < num_mmapped_regions; i++ ) {
      if ( mmapped_regions[i].phys_start != 0
	   && mmapped_regions[i].user_start <= addr
	   && addr < mmapped_regions[i].user_end )
	 break;
   }

   if ( i < num_mmapped_regions ) {
      // found
      phys_addr = mmapped_regions[i].phys_start +
	 ( addr - mmapped_regions[i].user_start );

/*        dprintf("found virtual addr 0x%08x->0x%08x in entry: 0x%08x - 0x%08x -> 0x%08x", */
/*  	      addr, phys_addr, */
/*  	      mmapped_regions[i].user_start, */
/*  	      mmapped_regions[i].user_end, */
/*  	      mmapped_regions[i].phys_start ); */
   }

   return phys_addr;
}

void
remove_mmapped_region( unsigned int phys_addr )
{
   int i;
   for ( i = 0; i < num_mmapped_regions; i++ ) {
      if ( mmapped_regions[i].phys_start == phys_addr ) {
	 mmapped_regions[i].phys_start = 0;
	 mmapped_regions[i].user_start = 0;
	 mmapped_regions[i].user_end = 0;
	 break;
      }
   }

   if ( i == num_mmapped_regions ) {
     wprintf("could not find region starting at pa 0x%08x to remove!",
	     phys_addr );
   }

   i = num_mmapped_regions;
   while ( i > 0 && mmapped_regions[i-1].phys_start == 0 )
      num_mmapped_regions--;
}

/********************************************
 * vma operations
 */

void ps2stuff_vma_open(struct vm_area_struct *vma);
void ps2stuff_vma_close(struct vm_area_struct *vma);
void ps2stuff_vma_unmap(struct vm_area_struct *vma,
			unsigned long addr, size_t len);

static struct vm_operations_struct ps2stuff_remap_vm_ops = {
    open:  ps2stuff_vma_open,
    close: ps2stuff_vma_close,
    unmap: ps2stuff_vma_unmap,
};

#define VMA_OFFSET(vma)  ((vma)->vm_offset)

void ps2stuff_vma_open(struct vm_area_struct *vma)
{
  MOD_INC_USE_COUNT;
}

void ps2stuff_vma_close(struct vm_area_struct *vma)
{
  MOD_DEC_USE_COUNT;
}

void ps2stuff_vma_unmap(struct vm_area_struct *vma,
		       unsigned long addr, size_t len)
{
   int *mem;

   int i;
   unsigned int phys_addr = 0;

   for ( i = 0; i < num_mmapped_regions; i++ ) {
      if ( mmapped_regions[i].phys_start != 0
	   && mmapped_regions[i].user_start <= addr
	   && addr < mmapped_regions[i].user_end )
	 break;
   }

   if ( i < num_mmapped_regions ) {
      // found
      phys_addr = mmapped_regions[i].phys_start +
	 ( addr - mmapped_regions[i].user_start );

/*        dprintf("found virtual addr 0x%08x->0x%08x in entry: 0x%08x - 0x%08x -> 0x%08x", */
/*  	      addr, phys_addr, */
/*  	      mmapped_regions[i].user_start, */
/*  	      mmapped_regions[i].user_end, */
/*  	      mmapped_regions[i].phys_start ); */
   }

   if ( addr != mmapped_regions[i].user_start ) {
     dprintf("trying to free from middle of memory block!");
     dprintf("dumping all entries (va is 0x%08x):", addr);
     for ( i = 0; i < num_mmapped_regions; i++ ) {
       if ( mmapped_regions[i].phys_start != 0
	    && mmapped_regions[i].user_start <= addr
	    && addr < mmapped_regions[i].user_end )
	 dprintf("m ");
       dprintf("0x%08x 0x%08x 0x%08x",
	       mmapped_regions[i].phys_start,
	       mmapped_regions[i].user_start,
	       mmapped_regions[i].user_end );
     }
   }

   phys_addr |= (unsigned int)0x80000000;

   bigphysarea_free_pages( phys_addr );
   dprintf("unmapped mem at 0x%08x (0x%08x) with length %d",
	   phys_addr, addr, len );

   remove_mmapped_region( (unsigned int)0x0fffffff & phys_addr );
}

/********************************************
 * file operations
 */

int ps2stuff_open (struct inode *inode, struct file *filp);
int ps2stuff_release(struct inode *inode, struct file *filp);
int ps2stuff_mmap(struct file *filp, struct vm_area_struct *vma);
int ps2stuff_nopage_mmap(struct file *filp, struct vm_area_struct *vma);
int ps2stuff_ioctl( struct inode *inode, struct file *filp,
		   unsigned int cmd, unsigned long arg);

struct file_operations ps2stuff_remap_ops = {
    open:    ps2stuff_open,
    release: ps2stuff_release,
    mmap:    ps2stuff_mmap,
    ioctl:   ps2stuff_ioctl,
};

int ps2stuff_open (struct inode *inode, struct file *filp)
{
    filp->f_op = &ps2stuff_remap_ops;
    MOD_INC_USE_COUNT;
    return 0;
}

int ps2stuff_release(struct inode *inode, struct file *filp)
{
    MOD_DEC_USE_COUNT;
    return 0;
}

int ps2stuff_mmap(struct file *filp, struct vm_area_struct *vma)
{
   unsigned long requested_size, adjusted_pg_size;
   unsigned long new_mem;

   if ( vma->vm_offset == 2 ) {
     // map timers

    /* tell linux not to swap out this memory */
     vma->vm_flags |= VM_IO;

      pgprot_val(vma->vm_page_prot) = (pgprot_val(vma->vm_page_prot)
				       & ~_CACHE_MASK) | _CACHE_UNCACHED;
     if (remap_page_range(vma->vm_start,
			  0x10000000,
			  4096 * 2,
			  vma->vm_page_prot)) {
	eprintf("failed to remap timers");
       return -EAGAIN;
     }
     
     dprintf("mapped timers to 0x%08x",
	    vma->vm_start );

     return 0;
   }

   /* get some memory from bigphysarea */
    requested_size = vma->vm_end-vma->vm_start;
    adjusted_pg_size = (requested_size + PAGE_SIZE-1) >> PAGE_SHIFT;
    new_mem = bigphysarea_alloc_pages( adjusted_pg_size,
				       1, /* page alignment */
				       GFP_KERNEL );

    if ( new_mem == 0 ) {
       eprintf("bigphysarea_alloc returned 0!");
    }

    flush_cache_all();

    dprintf("mapping %d pages -- pa = 0x%08x, va = 0x%08x",
	   adjusted_pg_size,
	   new_mem,
	   vma->vm_start );

    if ( new_mem == 0 )
       return -ENOMEM;

    /* tell linux not to swap out this memory */
    vma->vm_flags |= VM_IO;

    if ( vma->vm_offset == 1 ) {
      dprintf("allocating uncached");
      /* access as uncached */
      pgprot_val(vma->vm_page_prot) = (pgprot_val(vma->vm_page_prot)
				       & ~_CACHE_MASK) | _CACHE_UNCACHED;
    }

    if (remap_page_range(vma->vm_start,
			 __pa(new_mem),
			 adjusted_pg_size << PAGE_SHIFT,
			 vma->vm_page_prot))
        return -EAGAIN;

    /* store physical addr as the "offset" into this device */
    // vma->vm_offset = new_mem;
    vma->vm_offset = 0;


    /* add region to list */
    add_mmapped_region( __pa(new_mem),
			vma->vm_start,
			adjusted_pg_size << PAGE_SHIFT );

    vma->vm_ops = &ps2stuff_remap_vm_ops;
    ps2stuff_vma_open(vma);
    return 0;
}

static void
print_it( u32 *addr, int num_words )
{
   int i;
   for ( i = 0; i < num_words; i+=4 ) {
      dprintf("%p:  0x%08x 0x%08x 0x%08x 0x%08x",
	     addr, addr[0], addr[1], addr[2], addr[3] );
      addr += 4;
   }
}

/********************************************
 * dma-related
 */

#define DMAC_ENABLEW	((volatile u32*)KSEG1ADDR(0x1000f590))

#define DMAC_VIF1_CHCR	((volatile u32*)KSEG1ADDR(0x10009000))
#define DMAC_VIF1_MADR	((volatile u32*)KSEG1ADDR(0x10009010))
#define DMAC_VIF1_QWC	((volatile u32*)KSEG1ADDR(0x10009020))
#define DMAC_VIF1_TADR	((volatile u32*)KSEG1ADDR(0x10009030))

#define GIF_CTRL        ((volatile u32*)KSEG1ADDR(0x10003000))
#define GIF_STAT        ((volatile u32*)KSEG1ADDR(0x10003020))
#define GIF_TAG0        ((volatile u32*)KSEG1ADDR(0x10003040))
#define GIF_TAG1        ((volatile u32*)KSEG1ADDR(0x10003050))
#define GIF_TAG2        ((volatile u32*)KSEG1ADDR(0x10003060))
#define GIF_TAG3        ((volatile u32*)KSEG1ADDR(0x10003070))

#define VIF1_FBRST	((volatile u32*)KSEG1ADDR(0x10003c10))
#define VIF1_STAT       ((volatile u32*)KSEG1ADDR(0x10003c00))
#define VIF1_ERR        ((volatile u32*)KSEG1ADDR(0x10003c20))

#define VIF1_FIFO       ((volatile u32*)KSEG1ADDR(0x10005000))

static void
wait_for_vif1()
{
   int count, times;

   for ( times = 0; times < 4000; times++ ) {
      // don't poll too often because accessing
      // the chcr register goes over the bus and
      // interrupts the dma transfer, slowing it down.
      for (count = 2000; count > 0; count--)
	 // I don't think the beta compiler adjusts the
	 // size of small loops to account for a known hardware bug.
	 // so, just to be on the safe side...
	 asm ("nop\n nop\n nop\n nop\n nop\n nop\n"
	      "nop\n nop\n nop\n nop\n nop\n nop\n"
	      : "+r" (times) );

      if (!(*DMAC_VIF1_CHCR & 0x0100))
	 break;
   }

   // have we timed out?
   if ( times == 2000 ) {
      unsigned int tag_bits, tadr, madr, gt0, gt1, gt2, gt3;
      u32 *madr_p, *tadr_p;
      unsigned int vif1_stat, vif1_err, gif_stat;
      unsigned int vif1_fifo[16*4];
      int i;

      eprintf("### vif1 dma transfer timed out! stopping.. ###");

      // disable interrupts
      cli();

      // suspend all dma transfers
      *DMAC_ENABLEW = 1 << 16;
      // stop dma to vif1
      *DMAC_VIF1_CHCR = 0;
      // forcebreak vif1
      *VIF1_FBRST = 1 << 1;
      // pause the GIF
      *GIF_CTRL =  1 << 3;


      // dma controller regs

      tag_bits = *DMAC_VIF1_CHCR;
      tag_bits = tag_bits >> 16;

      madr = *DMAC_VIF1_MADR;
      madr_p = (u32*)(madr | (unsigned int)0x80000000);

      tadr = *DMAC_VIF1_TADR;
      tadr_p = (u32*)(tadr | (unsigned int)0x80000000);

      // gif regs

      gt0 = *GIF_TAG0;
      gt1 = *GIF_TAG1;
      gt2 = *GIF_TAG2;
      gt3 = *GIF_TAG3;

      gif_stat = *GIF_STAT;

      // vif1 regs

      vif1_stat = *VIF1_STAT;
      vif1_err = *VIF1_ERR;
      for ( i = 0; i < 16*4; i++ )
	 vif1_fifo[i] = VIF1_FIFO[i];

      // reset vif1
      *VIF1_FBRST = 1 << 0;
      // make sure there are no stalls
      *VIF1_FBRST = 1 << 3;
      // reset the GIF
      *GIF_CTRL =  1 << 0;
      // re-enable dma transfers
      *DMAC_ENABLEW = 0 << 16;

      // re-enable interrupts
      sti();

      eprintf("tag_bits = 0x%04x", tag_bits );
      
      eprintf("madr = 0x%08x: 0x%08x 0x%08x 0x%08x 0x%08x",
	     madr,
	     ((u32*)madr_p)[0],
	     ((u32*)madr_p)[1],
	     ((u32*)madr_p)[2],
	     ((u32*)madr_p)[3] );
      eprintf("tadr = 0x%08x: 0x%08x 0x%08x 0x%08x 0x%08x",
	     tadr,
	     ((u32*)tadr_p)[0],
	     ((u32*)tadr_p)[1],
	     ((u32*)tadr_p)[2],
	     ((u32*)tadr_p)[3] );

      eprintf("giftag = 0x%08x 0x%08x 0x%08x 0x%08x",
	     gt0, gt1, gt2, gt3 );

      eprintf("vif1_stat: 0x%08x, vif1_err: 0x%08x, gif_stat: 0x%08x",
	     vif1_stat, vif1_err, gif_stat );

      eprintf("vif1_fifo:");
      print_it( vif1_fifo, 16*4 );
   }
}

static void
kick_chain( unsigned int addr )
{
   if ( addr & 15 )
      eprintf("kick_chain: address not aligned -- 0x%08x\n",
	     addr );

   wait_for_vif1();

   flush_cache_all();

   cli();
   *DMAC_VIF1_TADR = addr;
   *DMAC_VIF1_QWC = 0;
   *DMAC_VIF1_CHCR = 0x0145;
   sti();
}

/********************************************
 * the ps2stuff ioctl handler
 */

int ps2stuff_ioctl( struct inode *inode, struct file *filp,
		   unsigned int cmd, unsigned long arg)
{
   unsigned int phys_addr;
   unsigned int *temp;
   int ret = 0;

   switch (cmd) {
      case PS2STUFF_IOCTV1DMAK:
	 phys_addr = lookup_user_addr( arg );
	 kick_chain( phys_addr );
	 break;

      case PS2STUFF_IOCTV1DMAW:
	 wait_for_vif1();
	 break;

      case PS2STUFF_IOCQPHYSADDR:
	 ret = lookup_user_addr(arg);
	 break;

      case PS2STUFF_IOCTMEMRESET:
	 bigphysarea_free_all_pages();
	 num_mmapped_regions = 0;
	 *VIF1_ERR = 2;
	 break;

      default:
	 return -ENOTTY;
   }

   return ret;
}

/********************************************
 * module start/stop
 */

#ifndef SET_MODULE_OWNER
#  define SET_MODULE_OWNER(structure) /* nothing */
#endif

static int ps2stuff_init(void)
{
    int result;

    SET_MODULE_OWNER(&ps2stuff_remap_ops);

    result = register_chrdev(ps2stuff_major, "ps2stuff", &ps2stuff_remap_ops);
    if (result < 0)
    {
        eprintf("unable to get major %d", ps2stuff_major);
        return result;
    }
    if (ps2stuff_major == 0)
        ps2stuff_major = result;
    return 0;
}


static void ps2stuff_cleanup(void)
{
    unregister_chrdev(ps2stuff_major, "ps2stuff");
}


#ifndef module_init
#  define module_init(x)        int init_module(void) { return x(); }
#  define module_exit(x)        void cleanup_module(void) { x(); }
#endif

module_init(ps2stuff_init);
module_exit(ps2stuff_cleanup);
