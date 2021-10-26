#include "userswap.h"

#include <sys/mman.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>

typedef struct ALLOCATION
{
	void *start_addr;
	size_t size;
	struct ALLOCATION *next;
} alloc_t;

alloc_t *head = NULL;

void page_fault_handler(siginfo_t *info)
{
	// printf("address: %p (si),\naddress: %p (si)\n", &info->si_addr, info->si_addr);
	alloc_t *cur = head;
	while (cur->start_addr != info->si_addr)
		cur = cur->next;
	// printf("address: %p (si),\naddress: %p (cur),\nsize: %zu\n", info->si_addr, cur->start_addr, cur->size);
	mprotect(info->si_addr, cur->size, PROT_READ);
}

void segv_handler(int sig, siginfo_t *info, void *ucontext)
{
	// printf("Attempt to access memory at address %p\n", info->si_addr);
	page_fault_handler(info);
}

void install_segv_handler()
{
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_SIGINFO;
	act.sa_sigaction = &segv_handler;
	if (sigaction(SIGSEGV, &act, NULL) == -1)
	{
		perror("sigaction");
		exit(EXIT_FAILURE);
	}
}

void userswap_set_size(size_t size)
{
}

void *userswap_alloc(size_t size)
{
	if (size == 0)
		return NULL;
	install_segv_handler();
	long rounded_size = (size - 1) / 4096 * 4096 + 4096;
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

	munmap(mem, cur->size);
}

void *userswap_map(int fd, size_t size)
{
	return NULL;
}
