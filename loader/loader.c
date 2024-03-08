/*
 * Loader Implementation
 *
 * 2022, Operating Systems
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <math.h>
#include <signal.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

#include "exec_parser.h"

static so_exec_t *exec;
static int fd;

static void *default_handler;

static int page_size = 4096;

static void segv_handler(int signum, siginfo_t *info, void *context)
{
	char *pfault_addr = (char *) info->si_addr;

	for (int i = 0; i < exec->segments_no; i++) {
		// find out if the pfault is in one of the segments

		so_seg_t *current_seg = exec->segments + i;
		char *current_seg_addr = (char *) current_seg->vaddr;
		int current_seg_fs = current_seg->file_size;
		int current_seg_ms = current_seg->mem_size;

		if (pfault_addr >= current_seg_addr && pfault_addr <= current_seg_addr + current_seg_ms - 1) {
			// pfault found inside current seg => find out what caused it:
			// 1. invalid acces (no permision/s) => segfault default action

			// allocating space for the current segment data (if needed)
			if (current_seg->data == NULL) {
				int current_seg_pages = ceil(current_seg_ms / page_size);

				current_seg->data = (void *) calloc(current_seg_pages, 1);
				if (current_seg->data == NULL) {
					perror("calloc failed1");
				exit(EXIT_FAILURE);
			}
			}

			int page_n = (pfault_addr - current_seg_addr) / page_size;

			if (((char *)(current_seg->data))[page_n] == 1) {
				// check custom data field of the current seg to see if the page
				// that gave the pfault was prev mapped (if it was => invalid acces)
				signal(SIGSEGV, default_handler);
				raise(SIGSEGV);
			}

			// 2. page not mapped yet => map the page in memory & copy data
			else {
				int current_seg_offs = current_seg->offset;
				int current_seg_perm = current_seg->perm;

				char *m = (char *) mmap (current_seg_addr + page_n * page_size, page_size,
				PROT_WRITE, MAP_FIXED | MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

				if (m == MAP_FAILED) {
					perror("mmap failed1");
					exit(EXIT_FAILURE);
				} else {
					// marking the page as 'mapped' in custom data field
					((char *)(current_seg->data))[page_n] = 1;
				}

				// check if there is data to copy in the page
				int data_size = current_seg_fs - page_n * page_size;

				if (data_size > 0) {
					// reposition the file offset for read
					lseek(fd, current_seg_offs + page_n * page_size, SEEK_SET);

					// read data from file into mapped page
					int read_size = data_size > page_size ? page_size : data_size;
					int r = read(fd, m, read_size);

					if (r == -1) {
						perror("read failed1");
						exit(EXIT_FAILURE);
					}

					// zero the rest of the page if needed
					if (read_size < page_size) {
						int zero_size;

						zero_size = page_size - read_size > current_seg_ms - page_n * page_size - read_size ?
						current_seg_ms - page_n * page_size - read_size : page_size - read_size;

						memset(current_seg_addr + page_n * page_size + read_size, 0,
						zero_size);
					}
				} else {
					// zero the page
					int zero_size = current_seg_ms - page_n * page_size;

					if (zero_size > page_size)
						zero_size = page_size;

					memset(current_seg_addr + page_n * page_size, 0, zero_size);
				}

				// giving segment permissions to the page
				int mp = mprotect(m, page_size, current_seg_perm);

				if (mp == -1) {
					perror("mprotect failed1");
					exit(EXIT_FAILURE);
				}

				return;
			}
		}
	}

	// pfault not found inside any of the segments => segfault default action
	signal(SIGSEGV, default_handler);
	raise(SIGSEGV);
}

int so_init_loader(void)
{
	default_handler = signal(SIGSEGV, NULL);

	int rc;
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_sigaction = segv_handler;
	sa.sa_flags = SA_SIGINFO;
	rc = sigaction(SIGSEGV, &sa, NULL);
	if (rc < 0) {
		perror("sigaction");
		return -1;
	}
	return 0;
}

int so_execute(char *path, char *argv[])
{
	fd = open(path, O_RDONLY);
	if (fd < 0) {
		perror("open failed1");
		return -1;
	}

	exec = so_parse_exec(path);
	if (!exec)
		return -1;

	so_start_exec(exec, argv);

	int c = close(fd);

	if (c == -1) {
		perror("close failed1");
		return -1;
	}

	return 0;
}
