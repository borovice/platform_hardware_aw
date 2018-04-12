/*
 * Copyright (C) 2010 ARM Limited. All rights reserved.
 *
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef GRALLOC_PRIV_H_
#define GRALLOC_PRIV_H_

#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <linux/fb.h>
#include <sys/types.h>
#include <unistd.h>
#include <linux/ion.h>
#include <sys/ioctl.h>

#include <hardware/gralloc.h>
#include <cutils/native_handle.h>
#include <alloc_device.h>
#include <utils/Log.h>

#include <cutils/properties.h>

#ifdef MALI_600
#define GRALLOC_ARM_UMP_MODULE 0
#define GRALLOC_ARM_DMA_BUF_MODULE 1
#else

/* NOTE:
 * If your framebuffer device driver is integrated with UMP, you will have to
 * change this IOCTL definition to reflect your integration with the framebuffer
 * device.
 * Expected return value is a UMP secure id backing your framebuffer device memory.
 */

/*#define IOCTL_GET_FB_UMP_SECURE_ID    _IOR('F', 311, unsigned int)*/
#define GRALLOC_ARM_UMP_MODULE 0
#define GRALLOC_ARM_DMA_BUF_MODULE 1

/* NOTE:
 * If your framebuffer device driver is integrated with dma_buf, you will have to
 * change this IOCTL definition to reflect your integration with the framebuffer
 * device.
 * Expected return value is a structure filled with a file descriptor
 * backing your framebuffer device memory.
 */
#if GRALLOC_ARM_DMA_BUF_MODULE
struct fb_dmabuf_export
{
	__u32 fd;
	__u32 flags;
};
/*#define FBIOGET_DMABUF    _IOR('F', 0x21, struct fb_dmabuf_export)*/
typedef int ion_user_handle_t;
#define ION_INVALID_HANDLE 0

#endif /* GRALLOC_ARM_DMA_BUF_MODULE */


#endif

/* the max string size of GRALLOC_HARDWARE_GPU0 & GRALLOC_HARDWARE_FB0
 * 8 is big enough for "gpu0" & "fb0" currently
 */
#define MALI_GRALLOC_HARDWARE_MAX_STR_LEN 8

/*
 *Numbers of buffers for page flipping (at least 2).
 *NUM_FRAMEBUFFER_SURFACE_BUFFERS should be equal to NUM_FB_BUFFERS
 *in android/device/softwinner/<board-name>-common/BoardConfigCommon.mk.
 */
#define NUM_FB_BUFFERS 3

#if GRALLOC_ARM_UMP_MODULE
#include <ump/ump.h>
#endif

#define MALI_IGNORE(x) (void)x
typedef enum
{
	MALI_YUV_NO_INFO,
	MALI_YUV_BT601_NARROW,
	MALI_YUV_BT601_WIDE,
	MALI_YUV_BT709_NARROW,
	MALI_YUV_BT709_WIDE,
} mali_gralloc_yuv_info;

struct private_handle_t;

struct private_module_t
{
	gralloc_module_t base;

	private_handle_t *framebuffer;
	uint32_t flags;
	uint32_t numBuffers;
	uint32_t bufferMask;
	pthread_mutex_t lock;
	buffer_handle_t currentBuffer;
	int ion_client;

	struct fb_var_screeninfo info;
	struct fb_fix_screeninfo finfo;
	float xdpi;
	float ydpi;
	float fps;

	enum
	{
		// flag to indicate we'll post this buffer
		PRIV_USAGE_LOCKED_FOR_POST = 0x80000000
	};

	/* default constructor */
	private_module_t();
};

#ifdef __cplusplus
struct private_handle_t : public native_handle
{
#else
struct private_handle_t
{
	struct native_handle nativeHandle;
#endif

	enum
	{
		PRIV_FLAGS_FRAMEBUFFER = 0x00000001,
		PRIV_FLAGS_USES_UMP    = 0x00000002,
		PRIV_FLAGS_USES_ION    = 0x00000004,

		/* This flag is used to tell hwcomposer if the current buffer is physical continous, zero means not physical continous */
		PRIV_FLAGS_USES_CONFIG = 0x00000008,
	};

	enum
	{
		LOCK_STATE_WRITE     =   1 << 31,
		LOCK_STATE_MAPPED    =   1 << 30,
		LOCK_STATE_UNREGISTERED  =   1 << 29,
		LOCK_STATE_READ_MASK =   0x3FFFFFFF
	};

	// ints
#if GRALLOC_ARM_DMA_BUF_MODULE
	/*shared file descriptor for dma_buf sharing*/
	int     share_fd;
#endif
	int     magic;
	int     flags;
	int     usage;
	int     size;
	int     width;
	int     height;
	int     format;
	int     stride;
	int     aw_byte_align[3];
	union
	{
		void   *base;
		uint64_t padding;
	};
	int     lockState;
	int     writeOwner;
	int     pid;

	mali_gralloc_yuv_info yuv_info;

	// Following members are for UMP memory only
#if GRALLOC_ARM_UMP_MODULE
	int     ump_id;
	int     ump_mem_handle;
#endif

	// Following members is for framebuffer only
	int     fd;
	int     offset;

#if GRALLOC_ARM_DMA_BUF_MODULE
	ion_user_handle_t ion_hnd;
#endif

	long long aw_buf_id;

#if GRALLOC_ARM_DMA_BUF_MODULE
#define GRALLOC_ARM_NUM_FDS 1
#else
#define GRALLOC_ARM_NUM_FDS 0
#endif

#ifdef __cplusplus
	static const int sNumFds = GRALLOC_ARM_NUM_FDS;
	static const int sMagic = 0x3141592;

#if GRALLOC_ARM_UMP_MODULE
	private_handle_t(int flags, int usage, int size, void *base, int lock_state, ump_secure_id secure_id, ump_handle handle):
#if GRALLOC_ARM_DMA_BUF_MODULE
		share_fd(-1),
#endif
		magic(sMagic),
		flags(flags),
		usage(usage),
		size(size),
		width(0),
		height(0),
		format(0),
		stride(0),
		base(base),
		lockState(lock_state),
		writeOwner(0),
		pid(getpid()),
		yuv_info(MALI_YUV_NO_INFO),
		ump_id((int)secure_id),
		ump_mem_handle((int)handle),
		fd(0),
		offset(0)
#if GRALLOC_ARM_DMA_BUF_MODULE
		,
		ion_hnd(ION_INVALID_HANDLE)
#endif

	{
		version = sizeof(native_handle);
		numFds = sNumFds;
		numInts = (sizeof(private_handle_t) - sizeof(native_handle)) / sizeof(int) - sNumFds;
	}
#endif

#if GRALLOC_ARM_DMA_BUF_MODULE
	private_handle_t(int flags, int usage, int size, void *base, int lock_state):
		share_fd(-1),
		magic(sMagic),
		flags(flags),
		usage(usage),
		size(size),
		width(0),
		height(0),
		format(0),
		stride(0),
		base(base),
		lockState(lock_state),
		writeOwner(0),
		pid(getpid()),
		yuv_info(MALI_YUV_NO_INFO),
#if GRALLOC_ARM_UMP_MODULE
		ump_id((int)UMP_INVALID_SECURE_ID),
		ump_mem_handle((int)UMP_INVALID_MEMORY_HANDLE),
#endif
		fd(0),
		offset(0),
		ion_hnd(ION_INVALID_HANDLE)

	{
		version = sizeof(native_handle);
		numFds = sNumFds;
		numInts = (sizeof(private_handle_t) - sizeof(native_handle)) / sizeof(int) - sNumFds;
	}

#endif

	private_handle_t(int flags, int usage, int size, void *base, int lock_state, int fb_file, int fb_offset):
#if GRALLOC_ARM_DMA_BUF_MODULE
		share_fd(-1),
#endif
		magic(sMagic),
		flags(flags),
		usage(usage),
		size(size),
		width(0),
		height(0),
		format(0),
		stride(0),
		base(base),
		lockState(lock_state),
		writeOwner(0),
		pid(getpid()),
		yuv_info(MALI_YUV_NO_INFO),
#if GRALLOC_ARM_UMP_MODULE
		ump_id((int)UMP_INVALID_SECURE_ID),
		ump_mem_handle((int)UMP_INVALID_MEMORY_HANDLE),
#endif
		fd(fb_file),
		offset(fb_offset)
#if GRALLOC_ARM_DMA_BUF_MODULE
		,
		ion_hnd(ION_INVALID_HANDLE)
#endif

	{
		version = sizeof(native_handle);
		numFds = sNumFds;
		numInts = (sizeof(private_handle_t) - sizeof(native_handle)) / sizeof(int) - sNumFds;
	}

	~private_handle_t()
	{
		magic = 0;
	}

	bool usesPhysicallyContiguousMemory()
	{
		return (flags & PRIV_FLAGS_FRAMEBUFFER) ? true : false;
	}

	static int validate(const native_handle *h)
	{
		const private_handle_t *hnd = (const private_handle_t *)h;

		if (!h || h->version != sizeof(native_handle) || h->numFds != sNumFds ||
		        h->numInts != (sizeof(private_handle_t) - sizeof(native_handle)) / sizeof(int) - sNumFds ||
		        hnd->magic != sMagic)
		{
			return -EINVAL;
		}

		return 0;
	}

	static private_handle_t *dynamicCast(const native_handle *in)
	{
		if (validate(in) == 0)
		{
			return (private_handle_t *) in;
		}

		return NULL;
	}
#endif
};

typedef struct
{
	int dram_size;  /* MB */

	/*
	 * For allwinner platforms, there are just two secure levels:
	 * 1: the system is with secure os.
	 * 3: the system is without secure os.
	 */
	int secure_level;

	/* This flag is used to flush cache on ARMv7 */
	bool ion_flush_cache_range;

	/* If CARVEOUT HEAP is available, this should be true. */
	bool carveout_enable;

	/*
	 * ro.kernel.iomem.type = 0xaf10 -> IOMMU is enabled
	 * ro.kernel.iomem.type = 0xfa01 -> IOMMU is disabled
	 */
	bool iommu_enabled;
}
aw_mem_info_data;

typedef struct {
    long start;
    long end;
}sunxi_cache_range;

int aw_flush_cache(int ion_client, void* start_vaddr, int shared_fd, size_t size);

static inline bool get_gralloc_debug(void)
{
	char gralloc_debug[PROPERTY_VALUE_MAX] = {0};

	property_get("debug.gralloc.showmsg", gralloc_debug, "0");
	if(atoi(gralloc_debug))
		return true;

	return false;
}

#endif /* GRALLOC_PRIV_H_ */
