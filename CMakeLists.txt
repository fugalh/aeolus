cmake_minimum_required(VERSION 3.13)
project(
  aeolus
  VERSION 0.10.4
  LANGUAGES CXX)
add_executable(aeolus)
target_compile_definitions(aeolus PRIVATE VERSION="${CMAKE_PROJECT_VERSION}")
target_compile_options(aeolus PRIVATE -Wno-deprecated-declarations -Wno-constant-conversion)
target_compile_features(aeolus PRIVATE cxx_std_20)

target_sources(
  aeolus
  PRIVATE source/main.cc
          source/audio_backend.cc
          source/audio_factory.cc
          source/midi_backend.cc
          source/model.cc
          source/slave.cc
          source/midi_processor.cc
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
  # Linux uses separate MIDI thread with ALSA sequencer (alsa_midi.cc), while macOS
  # uses JACK MIDI which is in audio.cc
  target_sources(aeolus PRIVATE source/alsa_audio.cc source/alsa_midi.cc)

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
target_sources(aeolus PRIVATE source/jack_audio.cc)

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

# Optional unit tests
option(BUILD_TESTS "Build unit tests" OFF)
if(BUILD_TESTS)
  enable_testing()
  
  # Find GTest
  pkg_check_modules(GTEST REQUIRED gtest)
  
  # Create test executable
  add_executable(aeolus_test)
  
  # Add test sources
  target_sources(aeolus_test PRIVATE
      tests/test_main.cc
      tests/test_audio_backend.cc
      tests/test_midi_processor.cc
  )
  
  # Add Aeolus source files needed for testing (without main.cc)
  target_sources(aeolus_test PRIVATE
      source/audio_backend.cc
      source/midi_processor.cc
      source/midi_backend.cc
      source/lfqueue.cc
      source/asection.cc
      source/division.cc
      source/reverb.cc
      source/addsynth.cc
      source/scales.cc
      source/rankwave.cc
      source/rngen.cc
      source/exp2ap.cc
  )
  
  # Configure test target
  target_include_directories(aeolus_test PRIVATE 
      ${GTEST_INCLUDE_DIRS}
      source
  )
  
  target_link_libraries(aeolus_test PRIVATE 
      ${GTEST_LDFLAGS}
      ${CLTHREADS_LIB} 
      pthread
  )
  
  target_compile_definitions(aeolus_test PRIVATE VERSION="test")
  target_compile_options(aeolus_test PRIVATE -Wno-deprecated-declarations -Wno-constant-conversion)
  target_compile_features(aeolus_test PRIVATE cxx_std_20)
  
  # Add test
  add_test(NAME aeolus_unit_tests COMMAND aeolus_test)
endif()

install(TARGETS aeolus)
install(
  CODE "message(STATUS \"You will need to provide Aeolus a 'stops' directory - see README and https://kokkinizita.linuxaudio.org/linuxaudio/downloads/index.html\")"
)
