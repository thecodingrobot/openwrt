/*
 * drv_optic_config.h - build configuration for the GPON optic driver
 * (kernel module flavour).  This mirrors what the Lantiq UGW SDK
 * "configure" script would have generated for a production ONU build.
 */
#ifndef _GPON_OPTIC_CONFIG_H
#define _GPON_OPTIC_CONFIG_H

/* kernel-module build (vs. user-space simulation library) */
#define ENABLE_LINUX            1

/* /proc/driver/optic/{version,status,...} entries */
#define INCLUDE_PROCFS_SUPPORT  1

/* kernel-CLI hook (requires the Lantiq 'cli' framework module) */
/* #undef INCLUDE_CLI_SUPPORT */

/* userspace simulation / remote-ONU variants - not for the kernel module */
/* #undef OPTIC_SIMULATION */
/* #undef OPTIC_LIBRARY */
/* #undef INCLUDE_REMOTE_ONLY_ONU */
/* #undef INCLUDE_REMOTE_ONU */
/* #undef EVENT_LOGGER_DEBUG */

/* std_defs.h only typedefs ulong_t when HAVE_ULONG_T is unset */
/* #undef HAVE_ULONG_T */

#endif /* _GPON_OPTIC_CONFIG_H */
