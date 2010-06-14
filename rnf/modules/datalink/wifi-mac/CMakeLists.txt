IF(WITH_WIFI_MAC)
INCLUDE_DIRECTORIES(${ROSSNET_SOURCE_DIR}/kernel)
INCLUDE_DIRECTORIES(${ROSS_TOP_SOURCE_DIR}/ross)
SET(wifi_mac_srcs wifi-mac-global.c 
wifi-mac-model.c  
wifi-mac-xml.c 
wifi-mac-helper.c   
wifi-mac-pe.c     
)

#FIND_PACKAGE(LibXml2 REQUIRED)

INCLUDE_DIRECTORIES(${LIBXML2_INCLUDE_DIR})

ADD_LIBRARY(wifi_mac ${wifi_mac_srcs})
ENDIF(WITH_WIFI_MAC)
