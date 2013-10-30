SUBSYSNAME := phasta/phUtil
MODULENAME := phastaIO
NODEP := 1
NOSHARED = 1

dirs := .

ifeq ($(ARCHOS), )
    ARCHOS := $(shell $(DEVROOT)/Util/buildUtil/getarch)
endif

include $(DEVROOT)/Util/buildUtil/make.common

