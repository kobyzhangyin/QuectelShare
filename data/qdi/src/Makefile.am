AM_CFLAGS = -Wundef \
        -Wstrict-prototypes \
        -Wno-trigraphs \
        -g -O0 \
        -fno-inline \
        -fno-short-enums \
        -fpic \
        $(QMICLIENTHELPER_CFLAGS) \
        $(QMIFRAMEWORK_CFLAGS) \
        $(QMI_CFLAGS)

#refer to Androik.mk for staging header files
AM_CPPFLAGS = -D__packed__= \
        -DIMAGE_APPS_PROC \
        -DFEATURE_Q_NO_SELF_QPTR \
        -DFEATURE_DSUTILS_OFFTARGET \
        -DQDI_OFFTARGET \
        -DFEATURE_DS_LINUX_NO_RPC \
        -DFEATURE_DATA_LINUX_LE \
        -DFEATURE_DATA_SOFTAP_V6 \
        -DFEATURE_LINUX \
        -DFEATURE_DS_SVLTE1 \
        -DFEATURE_NO_REX \
        $(QMICLIENTHELPER_CFLAGS) \
        $(QMIFRAMEWORK_CFLAGS) \
        $(QMI_CFLAGS) \
        $(DSUTILS_CFLAGS) \
        $(DIAG_CFLAGS) \
        -I../inc \
        -I../src \
        -I$(topdir)/dsutils/inc \
        -I../../dsi_netctrl/inc

libqdi_c_sources = qdi.c \
                   qdi_netlink.c \
                   qdi_qmi_wds.c

requiredlibs = -lpthread

library_includedir = $(pkgincludedir)

lib_LTLIBRARIES = libqdi.la
libqdi_la_CC = @CC@
libqdi_la_SOURCES = $(libqdi_c_sources)

if USE_GLIB
  libqdi_la_CFLAGS  = -DUSE_GLIB $(AM_CFLAGS) @GLIB_CFLAGS@
  libqdi_la_LDFLAGS  = $(DSUTILS_LIBS) $(QMI_LIBS) $(QMIFRAMEWORK_LIBS) \
                       $(QMICLIENTHELPER_LIBS) -lpthread -lqmi @GLIB_LIBS@
else
  libqdi_la_CFLAGS = $(AM_CFLAGS) -fPIC
  libqdi_la_LDFLAGS = -lpthread -lqmi $(DSUTILS_LIBS) $(QMI_LIBS) \
                      $(QMIFRAMEWORK_LIBS) $(QMICLIENTHELPER_LIBS)
endif

if IS_EXTERNAL_AP
  libqdi_la_LDFLAGS += -lqmi_cci
endif

libqdi_la_CPPFLAGS = $(AM_CPPFLAGS)
libqdi_la_LIBADD = $(requiredlibs)

