# Top-level CMakeLists.txt for the cirque-input-module.
#
# Wires three trees:
#   - drivers/         Cirque Pinnacle input driver (gated on CONFIG_INPUT)
#   - src/             Always-built runtime code (behaviors, events, processors)
#                      + Studio RPC + split relay (gated on the relevant Kconfig
#                      symbols).
#
# Symbols:
#   - CONFIG_ZMK_CIRQUE            parent menuconfig; enables this module
#   - CONFIG_ZMK_CIRQUE_STUDIO_RPC child of ZMK_CIRQUE; gates the Studio RPC
#                                  subsystem in src/studio/ and the nanopb
#                                  codegen for proto/tokyo2006/cirque/*.proto.
#   - CONFIG_ZMK_SPLIT_RELAY_EVENT upstream ZMK flag; gates the split-relay
#                                  helper in src/split/cirque_relay.c on both
#                                  split halves.

add_subdirectory(drivers)

zephyr_include_directories(include)

target_sources(app PRIVATE
  src/pointing_speed_math.c
  src/events/trackpad_status_changed.c
  src/behaviors/behavior_cirque_mode.c
  src/behaviors/behavior_drag_scroll.c
  src/behaviors/behavior_pointing_speed.c
  src/input_processor_drag_scroll.c
  src/input_processor_pointer_speed.c
)

# Runtime state get/set (src/studio/cirque_state.c). It is mutually dependent
# on the driver: the driver calls cirque_state_get_*/load_defaults, and
# cirque_state.c calls cirque_driver_get_state/apply_all. Both are therefore
# compiled together whenever the driver is (CONFIG_INPUT_PINNACLE2). Compiling
# it also whenever the Studio RPC is enabled lets the RPC link (and round-trip
# against a static fallback state) in device-less builds -- tests, or a split
# half whose trackpad is on the other hand. Compiling cirque_state.c on BOTH
# split roles lets the peripheral apply relayed state via cirque_state_set_*().
# Its cirque_settings_load_cb dependency is gated behind
# CONFIG_ZMK_CUSTOM_SETTINGS (central-only).
if(CONFIG_INPUT_PINNACLE2 OR CONFIG_ZMK_CIRQUE_STUDIO_RPC)
  target_sources(app PRIVATE src/studio/cirque_state.c)
endif()

if(CONFIG_ZMK_CIRQUE_STUDIO_RPC)
  # Studio RPC handler: src/studio/cirque_handler.c + nanopb codegen for
  # proto/tokyo2006/cirque/*.proto. Generated headers land under
  # ${CMAKE_CURRENT_BINARY_DIR}/proto and are exposed to `app` below.
  target_sources(app PRIVATE
    src/studio/cirque_handler.c
    src/studio/cirque_settings.c
  )

  list(APPEND CMAKE_MODULE_PATH ${ZEPHYR_BASE}/modules/nanopb)
  include(nanopb)
  set(NANOPB_GENERATE_CPP_APPEND_PATH TRUE)
  set(NANOPB_GENERATE_CPP_STANDALONE OFF)

  # Adding nanopb output directly to the `app` target causes duplicate-symbol
  # link issues, so we collect the generated proto sources into a private
  # library and link it into `app`. `add_dependencies` would only set build
  # ordering, leaving proto + nanopb runtime symbols undefined at link time;
  # `target_link_libraries` is required to actually pull the library's PRIVATE
  # generated sources (and any transitive deps it carries) into `app`.
  zephyr_library()
  file(GLOB_RECURSE PROTO_FILES ${CMAKE_CURRENT_SOURCE_DIR}/proto/*.proto)
  nanopb_generate_cpp(proto_srcs proto_hdrs RELPATH ${CMAKE_CURRENT_SOURCE_DIR} ${PROTO_FILES})
  target_include_directories(${ZEPHYR_CURRENT_LIBRARY} PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
  target_sources(${ZEPHYR_CURRENT_LIBRARY} PRIVATE ${proto_srcs} ${proto_hdrs})

  target_include_directories(app PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/proto)
  target_link_libraries(app PRIVATE ${ZEPHYR_CURRENT_LIBRARY})
endif()

if(CONFIG_ZMK_SPLIT_RELAY_EVENT AND CONFIG_INPUT_PINNACLE2)
  # Split-relay helper: compiled into both halves when split + relay events are
  # enabled AND the trackpad driver is present (cirque_relay.c references
  # cirque_state_get_*/set_* which only cirque_state.c defines; that file is
  # gated on CONFIG_INPUT_PINNACLE2 above). The central uses
  # cirque_relay_send_state() from the Studio RPC handler; the peripheral
  # receives the relayed carrier via the standard ZMK_RELAY_EVENT_HANDLE macro
  # and applies it.
  target_sources(app PRIVATE src/split/cirque_relay.c)
endif()
