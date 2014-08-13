AM_CFLAGS = -D_ANSC_LINUX
AM_CFLAGS += -D_ANSC_USER
AM_CFLAGS += -D_ANSC_LITTLE_ENDIAN_
AM_CFLAGS += -D_COSA_SIM_
AM_LDFLAGS = -L$(top_srcdir)/../CcspCommonLibrary/lib
AM_LDFLAGS += -lccsp_common

AM_CPPFLAGS = -Wall -Werror
ACLOCAL_AMFLAGS = -I m4
hardware_platform = i686-linux-gnu
bin_PROGRAMS = PsmSsp
PsmSsp_CPPFLAGS = -I/var/tmp/pc-rdkb/include/dbus-1.0 -I/var/tmp/pc-rdkb/include -I$(top_srcdir)/../CcspCommonLibrary/source/ccsp/custom -I$(top_srcdir)/../CcspCommonLibrary/source/ccsp/include -I$(top_srcdir)/../CcspCommonLibrary/source/debug_api/include -I$(top_srcdir)/../CcspCommonLibrary/source/cosa/include -I$(top_srcdir)/../CcspCommonLibrary/source/cosa/include/linux -I$(top_srcdir)/../CcspCommonLibrary/source/ccsp/components/include -I$(top_srcdir)/../CcspCommonLibrary/source/cosa/package/slap/include -I$(top_srcdir)/../CcspCommonLibrary/source/ccsp/components/common/MessageBusHelper/include -I$(top_srcdir)/../CcspCommonLibrary/source/util_api/http/include -I$(top_srcdir)/../CcspCommonLibrary/source/util_api/ansc/include -I$(top_srcdir)/../CcspCommonLibrary/source/util_api/tls/include -I$(top_srcdir)/../CcspCommonLibrary/source/util_api/web/include -I$(top_srcdir)/../CcspCommonLibrary/source/util_api/stun/include -I$(top_srcdir)/source/DslhManagementServer -I$(top_srcdir)/../CcspCommonLibrary/source/cosa/package/system/include -I$(top_srcdir)/source/include $(CPPFLAGS)
PsmSsp_SOURCES = Ssp/ssp_main.c Ssp/ssp_dbus.c Ssp/ssp_cfmif.c PsmFileLoader/psm_flo_interface.c PsmFileLoader/psm_flo_control.c PsmFileLoader/psm_flo_parse.c PsmFileLoader/psm_flo_operation.c PsmFileLoader/psm_flo_states.c PsmFileLoader/psm_flo_base.c PsmSysRegistry/psm_sysro_states.c PsmSysRegistry/psm_sysro_interface.c PsmSysRegistry/psm_sysro_storage.c PsmSysRegistry/psm_sysro_sysramif.c PsmSysRegistry/psm_sysro_base.c PsmSysRegistry/psm_sysro_cfmif.c PsmSysRegistry/psm_sysro_operation.c PsmSysRegistry/psm_sysro_control.c ../source-pc/ssp_HAL_apis.c

PsmSsp_LDFLAGS =  -L$(top_srcdir)/../CcspCommonLibrary/lib -lccsp_common
