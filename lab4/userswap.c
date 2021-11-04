#include "userswap.h"

#include <sys/mman.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>

// for pages
#define DIRTY 1
#define CLEAN 0
#define RESIDENT 1
#define NONRESIDENT 0

typedef struct PAGE
{
	void *addr;
	int page_num;
	int global_page_num;
	int swap_page_num;
	int dirty;
	int resident;
	struct PAGE *next;
	struct PAGE *prev;
} page_t;

typedef struct ALLOC
{
	void *start_addr;
	size_t size;
	int page_count;
	int max_pages;
	page_t *pages;
	struct ALLOC *next;
	struct ALLOC *prev;
} alloc_t;

typedef struct SWAP_LOCATIONS
{
	page_t *page_ptr;
	int swap_index;
	bool free;
	struct SWAP_LOCATIONS *next;
} swap_loc_t;

struct sigaction oldact; // orig sigsegv handler
alloc_t *head = NULL;		 // list of allocations
int page_size;					 // 4096
int total_pages = 0;		 // global num of pages spawned (for FIFO, never decremented to ensure unique vals)
size_t lorm = 8626176;	 // default limit
size_t cur_rm = 0;			 // amt of bytes taken up so far
int total_evicted = 0;	 // total no. of evicted pages so far

// swap file stuff
FILE *swap_file;						// swap file
char *swap_file_name;				// name of swap file (process pid)
int swap_index = 0;					// just a global count
swap_loc_t *swap_locations; // locations in swap file

void *sig_addr_to_page_addr(void *sig_addr, alloc_t *alloc)
{
	int page_num = ((uintptr_t)sig_addr - (uintptr_t)alloc->start_addr) / page_size;
	void *page_addr = alloc->start_addr + (page_num * page_size);
	return page_addr;
}

page_t *create_page(void *start_addr, int page_num)
{
	page_t *new_page = malloc(sizeof(page_t));
	new_page->addr = start_addr;
	new_page->page_num = page_num;
	new_page->global_page_num = total_pages;
	new_page->swap_page_num = -1;
	new_page->dirty = CLEAN;
	new_page->resident = RESIDENT;
	new_page->next = NULL;
	new_page->prev = NULL;
	return new_page;
}

page_t *find_page(void *sig_addr, alloc_t *alloc)
{
	void *page_addr = sig_addr_to_page_addr(sig_addr, alloc);
	page_t *cur = alloc->pages;
	if (cur == NULL)
		return NULL;
	else
	{
		while (cur != NULL)
		{
			if (cur->addr == page_addr)
				return cur;
			else
				cur = cur->next;
		}
		return NULL;
	}
}

void add_page(page_t *page, alloc_t *alloc)
{
	alloc->page_count++;
	total_pages++;
	page_t *cur = alloc->pages;
	page->next = NULL;
	if (cur == NULL)
	{
		alloc->pages = page;
		page->prev = NULL;
	}
	else
	{
		while (cur->next != NULL)
			cur = cur->next;
		cur->next = page;
		page->prev = cur;
	}
}

void remove_page(page_t *page, alloc_t *alloc)
{
	if (alloc->pages == NULL)
		return;
	else
	{
		alloc->page_count--;

		if (alloc->pages == page)
			alloc->pages = alloc->pages->next;
		if (page->next != NULL)
			page->next->prev = page->prev;
		if (page->prev != NULL)
			page->prev->next = page->next;
	}
}

void move_page_to_back(page_t *page, alloc_t *alloc)
{
	remove_page(page, alloc);
	add_page(page, alloc);
	page->global_page_num = total_pages;
}

page_t *find_first_resident_page(alloc_t *alloc)
{
	page_t *cur = alloc->pages;
	while (cur != NULL && cur->resident != RESIDENT)
		cur = cur->next;
	return cur;
}

swap_loc_t *create_swap_loc()
{
	swap_loc_t *new_loc = (swap_loc_t *)malloc(sizeof(swap_loc_t));
	new_loc->free = true;
	new_loc->next = NULL;
	new_loc->swap_index = swap_index;
	swap_index++;

	if (swap_locations == NULL)
		swap_locations = new_loc;
	else
	{
		swap_loc_t *cur = swap_locations;
		while (cur->next != NULL)
			cur = cur->next;
		cur->next = new_loc;
	}

	return new_loc;
}

swap_loc_t *get_free_swap_loc()
{
	swap_loc_t *cur = swap_locations;
	while (cur != NULL && !cur->free)
		cur = cur->next;

	if (cur == NULL)
		cur = create_swap_loc();

	cur->free = false;
	return cur;
}

void set_swap_loc_free(int swap_index)
{
	if (swap_locations == NULL)
		return;
	else
	{
		swap_loc_t *cur = swap_locations;
		while (cur != NULL && cur->swap_index != swap_index)
			cur = cur->next;

		if (cur != NULL && cur->swap_index == swap_index)
		{
			cur->free = true;
			cur->page_ptr = NULL;
		}
	}
}

void page_fault_handler(siginfo_t *info, alloc_t *alloc)
{
	page_t *page = find_page(info->si_addr, alloc);
	if (page != NULL && page->resident == RESIDENT && page->dirty == CLEAN)
	{
		// write
		page->dirty = DIRTY;
		if (mprotect(page->addr, page_size, PROT_READ | PROT_WRITE) == -1)
		{
			perror("page_fault_handler_mprotect_rw");
			exit(EXIT_FAILURE);
		}
	}
	else if (page != NULL && page->resident == NONRESIDENT)
	{
		// previously evicted into swap file
		page->resident = RESIDENT;
		int swap_offset = page->swap_page_num * page_size;
		if (mprotect(page->addr, page_size, PROT_READ | PROT_WRITE) == -1)
		{
			perror("page_fault_handler_mprotect_r");
			exit(EXIT_FAILURE);
		}
		swap_file = fopen(swap_file_name, "r+");
		if (lseek(fileno(swap_file), swap_offset, SEEK_SET) == -1)
		{
			perror("page_fault_handler_lseek_old");
			exit(EXIT_FAILURE);
		}
		if (read(fileno(swap_file), page->addr, page_size) == -1)
		{
			printf("%p bad\n", page->addr);
			perror("page_fault_handler_read_old");
			exit(EXIT_FAILURE);
		}
		// update the swap location to free
		set_swap_loc_free(page->swap_page_num);

		// shift current page to the end of alloc page list
		move_page_to_back(page, alloc);

		if (mprotect(page->addr, page_size, PROT_READ) == -1)
		{
			perror("page_fault_handler_mprotect_r");
			exit(EXIT_FAILURE);
		}
	}
	else if (alloc->page_count < alloc->max_pages && cur_rm < lorm)
	{
		// create page
		void *addr = sig_addr_to_page_addr(info->si_addr, alloc);
		page = create_page(addr, alloc->page_count);
		add_page(page, alloc);
		cur_rm += page_size;

		if (mprotect(addr, page_size, PROT_READ) == -1)
		{
			perror("page_fault_handler_mprotect_r");
			exit(EXIT_FAILURE);
		}
	}
	else
	{
		// evict
		page_t *oldest = find_first_resident_page(alloc);
		if (oldest->dirty == DIRTY)
		{
			// set the swap index in page
			int swap_offset = swap_index * page_size;
			oldest->swap_page_num = swap_index;

			// set page in the swap locs
			// get free loc
			swap_loc_t *swap_loc = get_free_swap_loc();
			swap_loc->page_ptr = oldest;

			// write to swap file
			swap_file = fopen(swap_file_name, "r+");
			if (lseek(fileno(swap_file), swap_offset, SEEK_SET) == -1)
			{
				perror("page_fault_handler_lseek");
				exit(EXIT_FAILURE);
			}
			if (write(fileno(swap_file), oldest->addr, page_size) == -1)
			{
				perror("page_fault_handler_write");
				exit(EXIT_FAILURE);
			}
			oldest->dirty = CLEAN;
		}
		oldest->resident = NONRESIDENT;
		// madvise on the page
		madvise(oldest->addr, page_size, MADV_DONTNEED);
		// mprotect NONE
		if (mprotect(oldest->addr, page_size, PROT_NONE) == -1)
		{
			perror("page_fault_handler_mprotect_evict");
			exit(EXIT_FAILURE);
		}
		cur_rm -= page_size;

		// create page
		void *addr = sig_addr_to_page_addr(info->si_addr, alloc);
		page = create_page(addr, alloc->page_count);
		add_page(page, alloc);
		cur_rm += page_size;

		if (mprotect(addr, page_size, PROT_READ) == -1)
		{
			perror("page_fault_handler_mprotect_r");
			exit(EXIT_FAILURE);
		}
	}
}

void segv_handler(int sig, siginfo_t *info, void *ucontext)
{
	alloc_t *cur = head;
	while (cur != NULL)
	{
		if (info->si_addr >= cur->start_addr && info->si_addr <= cur->start_addr + cur->size)
		{
			page_fault_handler(info, cur);
			return;
		}
		else
			cur = cur->next;
	}
	if (cur == NULL)
	{
		sigaction(SIGSEGV, &oldact, NULL);
		return;
	}
}

void install_segv_handler()
{
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_SIGINFO;
	act.sa_sigaction = &segv_handler;
	if (sigaction(SIGSEGV, &act, &oldact) == -1)
	{
		perror("sigaction");
		exit(EXIT_FAILURE);
	}
}

void userswap_set_size(size_t size)
{
	ulong rounded_size = (size - 1) / page_size * page_size + page_size; // round off to nearest page size
	if (cur_rm > rounded_size)
	{
		while (cur_rm > rounded_size)
		{
			// evict
			alloc_t *alloc = head;
			page_t *cur_oldest = find_first_resident_page(alloc);
			int oldest_page_num = cur_oldest->global_page_num;

			while (alloc->next != NULL)
			{
				if (alloc->pages->global_page_num < oldest_page_num)
				{
					oldest_page_num = alloc->pages->global_page_num;
					cur_oldest = alloc->pages;
				}
				alloc = alloc->next;
			}

			if (cur_oldest->dirty == DIRTY)
			{
				// set the swap index in page
				int swap_offset = swap_index * page_size;
				cur_oldest->swap_page_num = swap_index;

				// set page in the swap locs
				// get free loc
				swap_loc_t *swap_loc = get_free_swap_loc();
				swap_loc->page_ptr = cur_oldest;

				// write to swap file
				swap_file = fopen(swap_file_name, "r+");
				if (lseek(fileno(swap_file), swap_offset, SEEK_SET) == -1)
				{
					perror("userswap_set_size_lseek");
					exit(EXIT_FAILURE);
				}
				if (write(fileno(swap_file), cur_oldest->addr, page_size) == -1)
				{
					perror("userswap_set_size_write");
					exit(EXIT_FAILURE);
				}
			}
			// madvise on the page
			madvise(cur_oldest->addr, page_size, MADV_DONTNEED);
			// mprotect NONE
			if (mprotect(cur_oldest->addr, page_size, PROT_NONE) == -1)
			{
				perror("userswap_set_size_mprotect_evict");
				exit(EXIT_FAILURE);
			}
			cur_rm -= page_size;
		}
	}
	lorm = rounded_size;
}

void *userswap_alloc(size_t size)
{
	if (size == 0)
		return NULL;

	install_segv_handler();
	page_size = sysconf(_SC_PAGESIZE);
	long rounded_size = (size - 1) / page_size * page_size + page_size; // round off to nearest page size
	void *ptr = mmap(NULL, rounded_size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (ptr == MAP_FAILED)
	{
		perror("userswap_alloc_mmap");
		exit(EXIT_FAILURE);
	}

	int pid = getpid();
	swap_file_name = malloc(14);
	sprintf(swap_file_name, "%d.swap", pid);
	swap_file = fopen(swap_file_name, "w+");
	if (ftruncate(fileno(swap_file), rounded_size) == -1)
	{
		perror("userswap_alloc_ftruncate");
		exit(EXIT_FAILURE);
	}

	alloc_t *alloc = (alloc_t *)malloc(sizeof(alloc_t));
	alloc->start_addr = ptr;
	alloc->size = rounded_size;
	alloc->next = NULL;
	alloc->page_count = 0;
	alloc->max_pages = rounded_size / page_size;

	if (head == NULL)
		head = alloc;
	else
	{
		alloc_t *cur = head;
		while (cur->next != NULL)
			cur = cur->next;
		cur->next = alloc;
	}

	return ptr;
}

void userswap_free(void *mem)
{
	alloc_t *cur = head;
	while (cur->start_addr != mem)
		cur = cur->next;

	page_t *cur_page = cur->pages;
	while (cur_page != NULL)
	{
		page_t *temp = cur_page;
		cur_page = cur_page->next;
		free(temp);
		temp = NULL;
	}

	swap_loc_t *cur_swap_loc = swap_locations;
	while (cur_swap_loc != NULL)
	{
		if (cur_swap_loc->page_ptr == NULL)
			cur_swap_loc->free = true;

		cur_swap_loc = cur_swap_loc->next;
	}

	if (head == cur)
		head = cur->next;
	if (cur->next != NULL)
		cur->next->prev = cur->prev;
	if (cur->prev != NULL)
		cur->prev->next = cur->next;

	munmap(mem, cur->size);
	free(cur);
	cur = NULL;
}

void *userswap_map(int fd, size_t size)
{
	return NULL;
}
