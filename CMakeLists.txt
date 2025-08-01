cmake_minimum_required(VERSION 3.13)
project(
  aeolus
  VERSION 0.10.4
  LANGUAGES CXX)
add_executable(aeolus)
target_compile_definitions(aeolus PRIVATE VERSION="${CMAKE_PROJECT_VERSION}")
target_compile_options(aeolus PRIVATE -Wno-deprecated-declarations -Wno-constant-conversion)

target_sources(
  aeolus
  PRIVATE source/main.cc
          source/audio.cc
          source/model.cc
          source/slave.cc
          source/addsynth.cc
          source/scales.cc
          source/reverb.cc
          source/asection.cc
          source/division.cc
          source/rankwave.cc
          source/rngen.cc
          source/exp2ap.cc
          source/lfqueue.cc)

include(GNUInstallDirs)
find_package(PkgConfig REQUIRED)

if(APPLE)
  set(ENV{PKG_CONFIG_PATH} "/opt/X11/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
  target_compile_definitions(aeolus PRIVATE STATIC_UI)
elseif(UNIX)
  # Linux uses separate MIDI thread with ALSA sequencer (imidi.cc), while macOS
  # uses JACK MIDI which is in audio.cc
  target_sources(aeolus PRIVATE source/imidi.cc)

  # ALSA
  pkg_check_modules(ALSA REQUIRED alsa)
  pkg_check_modules(ZITA_ALSA_PCMI REQUIRED zita-alsa-pcmi)
  target_include_directories(aeolus PRIVATE SYSTEM ${ALSA_INCLUDE_DIRS}
                                            ${ZITA_ALSA_PCMI_INCLUDE_DIRS})
  target_link_libraries(aeolus PRIVATE ${ALSA_LIBRARIES}
                                       ${ZITA_ALSA_PCMI_LIBRARIES} rt)

endif()

# clthreads
find_library(CLTHREADS_LIB clthreads REQUIRED)
target_link_libraries(aeolus PRIVATE ${CLTHREADS_LIB} pthread ${CMAKE_DL_LIBS})

# JACK
pkg_check_modules(JACK REQUIRED jack)
target_compile_definitions(aeolus PRIVATE HAVE_JACK)
target_include_directories(aeolus PRIVATE ${JACK_INCLUDE_DIRS})
target_link_libraries(aeolus PRIVATE ${JACK_LDFLAGS})

# X11 && clxclient (optional, for GUI)
pkg_check_modules(X11 x11)
pkg_check_modules(XFT Xft)
if(X11_FOUND)
  find_library(CLXCLIENT_LIB clxclient)
  if(CLXCLIENT_LIB)
    target_sources(
      aeolus
      PRIVATE source/styles.cc
              source/mainwin.cc
              source/midiwin.cc
              source/audiowin.cc
              source/instrwin.cc
              source/editwin.cc
              source/midimatrix.cc
              source/multislider.cc
              source/functionwin.cc
              source/xiface.cc)
    target_compile_definitions(aeolus PRIVATE HAVE_XIFACE)
    target_include_directories(aeolus PRIVATE ${X11_INCLUDE_DIRS} ${XFT_INCLUDE_DIRS})
    target_link_libraries(aeolus PRIVATE ${X11_LDFLAGS} ${XFT_LIBRARIES} ${CLXCLIENT_LIB})
  endif()
endif()

# readline (optional, for TUI)
find_library(READLINE_LIB readline)
if(READLINE_LIB)
  target_compile_definitions(aeolus PRIVATE HAVE_TIFACE)
  target_sources(aeolus PRIVATE source/tiface.cc)
  target_link_libraries(aeolus PRIVATE ${READLINE_LIB})
endif()

install(TARGETS aeolus)
install(
  CODE "message(STATUS \"You will need to provide Aeolus a 'stops' directory - see README and https://kokkinizita.linuxaudio.org/linuxaudio/downloads/index.html\")"
)
