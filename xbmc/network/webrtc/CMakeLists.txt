set(HEADERS IDataStream.h
            RTCPeerConnection.h)

if(NOT ENABLE_STATIC_LIBS)
  core_add_library(network_webrtc)
endif()
