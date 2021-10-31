#include "userswap.h"

#include <sys/mman.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <stdint.h>
#include <sys/time.h>

#define DIRTY 1
#define CLEAN 0
#define REFERENCED 1
#define NONREFERENCED 0

typedef struct PAGE
{
	void *addr;
	int page_num;
	int global_page_num;
	int dirty;
	int referenced;
	struct PAGE *next;
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

struct sigaction oldact;
alloc_t *head = NULL;
int page_size;
int total_pages = 0;

size_t lorm = 8626176; // default
size_t cur_rm = 0;		 // amt of bytes taken up so far

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
	new_page->referenced = REFERENCED;
	new_page->dirty = CLEAN;
	new_page->next = NULL;
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
	if (cur == NULL)
		alloc->pages = page;
	else
	{
		while (cur->next != NULL)
			cur = cur->next;
		cur->next = page;
	}
}

void page_fault_handler(siginfo_t *info, alloc_t *alloc)
{
	// printf("address: %p (si),\naddress: %p (si)\n", &info->si_addr, info->si_addr);
	page_t *page = find_page(info->si_addr, alloc);
	if (page != NULL)
	{
		// printf("writing\n");
		// write
		// page->dirty = DIRTY;
		if (mprotect(page->addr, page_size, PROT_READ | PROT_WRITE) == -1)
		{
			perror("mprotect_rw");
			exit(EXIT_FAILURE);
		}
		return;
	}
	// if (alloc->page_count < alloc->max_pages && cur_rm < lorm)
	if (alloc->page_count < alloc->max_pages)
	{
		// printf("create page\n");
		// create page
		void *addr = sig_addr_to_page_addr(info->si_addr, alloc);
		page = create_page(addr, alloc->page_count);
		add_page(page, alloc);
		cur_rm += page_size;

		if (mprotect(addr, page_size, PROT_READ) == -1)
		{
			perror("mprotect_r");
			exit(EXIT_FAILURE);
		}
	}
	else
	{
		printf("evict\n");
		// evict
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
		// printf("memory not found\n");
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
	// size_t old_lorm = lorm;
	// lorm = size;
	// if (cur_rm > size)
	// {
	// }
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
		perror("userswap_alloc");
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

	// printf("alloc_size = %zu,\naddress: %p,\naddress: %p\n", rounded_size, ptr, alloc->start_addr);
	return ptr;
}

void userswap_free(void *mem)
{
	alloc_t *cur = head;
	while (cur->start_addr != mem)
		cur = cur->next;
	// printf("address: %p\naddress: %p\nfree size %zu\n", mem, cur->start_addr, cur->size);
	page_t *cur_page = cur->pages;
	while (cur_page != NULL)
	{
		page_t *temp = cur_page;
		cur_page = cur_page->next;
		free(temp);
	}

	alloc_t *temp = cur;
	if (head == temp)
		head = temp->next;
	if (temp->next != NULL)
		temp->next->prev = temp->prev;
	if (temp->prev != NULL)
		temp->prev->next = temp->next;

	munmap(mem, temp->size);
	free(temp);
}

void *userswap_map(int fd, size_t size)
{
	return NULL;
}
