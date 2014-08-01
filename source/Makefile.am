AM_CFLAGS = 

AM_CPPFLAGS = -Wall -Werror

AM_LDFLAGS = -lm -lccsp_common

ACLOCAL_AMFLAGS = -I m4

#hardware_platform = $(MACHINE)
hardware_platform = i686-linux-gnu

#lib_LTLIBRARIES = 
bin_PROGRAMS = PsmSsp

#libhalcm_ladir = $(top_builddir)/source/.libs

PsmSsp_CPPFLAGS = -I$(top_builddir)/source/include $(CPPFLAGS)

PsmSsp_SOURCES = PsmFileLoader/psm_flo_base.c
		PsmFileLoader/psm_flo_control.c
		PsmFileLoader/psm_flo_global.h
		PsmFileLoader/psm_flo_interface.c
		PsmFileLoader/psm_flo_internal_api.h
		PsmFileLoader/psm_flo_operation.c
		PsmFileLoader/psm_flo_parse.c
		PsmFileLoader/psm_flo_states.c
		PsmSysRegistry/psm_sysro_base.c
		PsmSysRegistry/psm_sysro_cfmif.c
		PsmSysRegistry/psm_sysro_control.c
		PsmSysRegistry/psm_sysro_global.h
		PsmSysRegistry/psm_sysro_interface.c
		PsmSysRegistry/psm_sysro_internal_api.h
		PsmSysRegistry/psm_sysro_operation.c
		PsmSysRegistry/psm_sysro_states.c
		PsmSysRegistry/psm_sysro_storage.c
		PsmSysRegistry/psm_sysro_sysramif.c
		Ssp/ssp_cfmif.c
		Ssp/ssp_dbus.c
		Ssp/ssp_global.h
		Ssp/ssp_main.c

#libhalcm_la_LIBADD = 
