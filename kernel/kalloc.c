// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct  {
  struct spinlock lock;
  // every page's ref_count, indexed by physical address divided by 4096
  int ref_counts[(PHYSTOP-KERNBASE) / PGSIZE];
} page_ref;

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

void
kinit()
{
  initlock(&page_ref.lock, "page_ref");
  for (int i = 0; i < (PHYSTOP-KERNBASE) / PGSIZE; i++)
    page_ref.ref_counts[i] = 1;

  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  if (decr_ref((uint64)pa) == 0) {
    // Fill with junk to catch dangling refs.
    memset(pa, 1, PGSIZE);

    r = (struct run*)pa;

    acquire(&kmem.lock);
    r->next = kmem.freelist;
    kmem.freelist = r;
    release(&kmem.lock);
  }

}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r)
  {
    memset((char*)r, 5, PGSIZE); // fill with junk
    if (incr_ref((uint64)r) != 1) panic("kalloc(): ref count wrong");
  }
  return (void*)r;
}

// return ref count after decrease
int
decr_ref(uint64 pa)
{
  int count;
  int index = PA2REFINDEX(pa);
  acquire(&page_ref.lock);
  page_ref.ref_counts[index]--;
  count = page_ref.ref_counts[index];
  release(&page_ref.lock);
  return count;
}

// return ref count after increase
int
incr_ref(uint64 pa)
{
  int count;
  int index = PA2REFINDEX(pa);
  acquire(&page_ref.lock);
  page_ref.ref_counts[index]++;
  count = page_ref.ref_counts[index];
  release(&page_ref.lock);
  return count;
}
