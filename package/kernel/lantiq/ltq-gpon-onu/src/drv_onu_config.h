/*
 * drv_onu_config.h - build configuration for the GPON ONU driver
 * (kernel module flavour).  This mirrors what the Lantiq UGW SDK
 * "configure" script would have generated for a production ONU build
 * (see the pristine drv_onu_config.h.in for the full option set).
 *
 * NOTE: this file only controls kernel-side compilation switches; the
 * userspace ABI lives in include/ and is NOT affected by this header.
 */
#ifndef _GPON_ONU_CONFIG_H
#define _GPON_ONU_CONFIG_H

/* kernel-module build (vs. user-space simulation library) */
#define ENABLE_LINUX            1

/* /proc/driver/onu/{version,status,...} entries */
#define INCLUDE_PROCFS_SUPPORT  1

/* kernel-CLI hook (requires the Lantiq 'cli' framework module) */
/* #undef INCLUDE_CLI_SUPPORT */

/* CLI dump support (ioctl trace, needs CLI framework) */
/* #undef INCLUDE_CLI_DUMP_SUPPORT */

/* remote-DTI ONU variants - not for the local kernel module */
/* #undef INCLUDE_REMOTE_ONLY_ONU */
/* #undef INCLUDE_REMOTE_ONU */

/* event logger (el_log_macros.h of the UGW SDK) */
/* #undef EVENT_LOGGER_DEBUG */

/* userspace simulation / library variants - not for the kernel module */
/* #undef ONU_SIMULATION */
/* #undef ONU_LIBRARY */

/* export the extra debug symbol set (gpe_table_*, tmu_* helpers) */
/* #undef ONU_DBG_EXPORTS */

/* TOD output on ASC1 (needs Falcon BSP pinmux/lantiq_soc.h - platform
 * hook, see PORTING-ONU.md) */
/* #undef ONU_TOD_ASC1 */

/* SCE/COP debug support */
/* #undef INCLUDE_SCE_DEBUG */
/* #undef INCLUDE_COP_DEBUG */

/* Selected device type: left at the driver default (PSB98010, the
 * Falcon-G variant used on the GPON SFP sticks) */
/* #undef ONU_DEVICE_PSB980xx */

/* std_defs.h only typedefs ulong_t when HAVE_ULONG_T is unset */
/* #undef HAVE_ULONG_T */

#endif /* _GPON_ONU_CONFIG_H */
