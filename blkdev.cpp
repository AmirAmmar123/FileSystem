#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>
#include "blkdev.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdexcept>
#include <errno.h>

BlockDeviceSimulator::BlockDeviceSimulator(std::string fname) {

	// if file doesn't exist returns -1, create it
	if (access(fname.c_str(), F_OK) == -1) {
		fd = open(fname.c_str(), O_CREAT | O_RDWR | O_EXCL, 0664);
		if (fd == -1)
			throw std::runtime_error(
				std::string("open-create failed: ") + strerror(errno));

		// The lseek function is used to reposition the offset of the file descriptor
		if (lseek(fd, DEVICE_SIZE-1, SEEK_SET) == -1)
			throw std::runtime_error("Could not seek");
		/* A null byte is written at the last offset to ensure the file size is set to DEVICE_SIZE.
		   By ensuring the file size using lseek and writing a null byte,
		   the code prepares the file for use as a block device simulator with a fixed size,
		   allowing subsequent operations to access a predictable and appropriately sized file.
		   Parameters Explanation:
			 fd: The file descriptor of the open file.
			 DEVICE_SIZE-1: The offset is set to DEVICE_SIZE-1.
			      This means moving the offset to the position just before the desired device size.
		    SEEK_SET: The whence parameter SEEK_SET means that the offset is set to DEVICE_SIZE-1 bytes from the beginning of the file.
		 */
		::write(fd, "\0", 1);
	} else {
		fd = open(fname.c_str(), O_RDWR);
		if (fd == -1) {
			throw std::runtime_error(
				std::string("open failed: ") + strerror(errno));
		}
	}
	/* The mmap function creates a new mapping in the virtual address space of the calling process.
	 * It allows a file or a device to be accessed as if it were part of the process's memory,
	 * enabling efficient file I/O operations.
	 * void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
	 *     addr: The starting address for the new mapping. NULL indicates that the kernel chooses the address.
		   length: The length of the mapping (in bytes).
		   prot: Memory protection flags.
		   flags: Flags that determine the nature of the mapping.
		   fd: File descriptor of the file to be mapped.
		   offset: Offset in the file where the mapping start

		The mmap function is used to map the file into memory so that it can be accessed directly through a pointer,
		allowing efficient read and write operations as if it were a block of memory.
	 */

	filemap = (unsigned char *)mmap(NULL, DEVICE_SIZE, PROT_READ | PROT_WRITE,
				        MAP_SHARED, fd, 0);

	// filemap is a pointer to the memory-mapped region of the file. When a file is memory-mapped
	if (filemap == (unsigned char *)-1)
		throw std::runtime_error(strerror(errno));
}

BlockDeviceSimulator::~BlockDeviceSimulator() {
	munmap(filemap, DEVICE_SIZE);
	close(fd);

	/*
	* munmap(filemap, DEVICE_SIZE):

			munmap is used to unmap the memory region that was previously mapped by mmap.
			filemap is the pointer to the mapped region.
			DEVICE_SIZE is the size of the mapped region.
			This call releases the memory mapping, ensuring that any changes are flushed to the file and the memory is freed.

	   close(fd):
			Closes the file descriptor fd.
			This call ensures that the file is properly closed and that any pending changes are written to disk.
	 */
}

void BlockDeviceSimulator::read(int addr, int size, char *ans) {
	memcpy(ans, filemap + addr, size);
	/* memcpy(ans, filemap + addr, size) copies size bytes from the memory-mapped file starting at filemap + addr into the
	   buffer ans.
	   This function allows efficient reading of data from the mapped file region.
	*/

}

void BlockDeviceSimulator::write(int addr, int size, const char *data) {
	memcpy(filemap + addr, data, size);
	/*
	* memcpy(filemap + addr, data, size) copies size bytes from the buffer
	* data into the memory-mapped file starting at filemap + addr.
	  This function allows efficient writing of data to the mapped file region.
	*/
}

