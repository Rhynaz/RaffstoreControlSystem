idf_component_register(
    INCLUDE_DIRS "include"
    REQUIRES "esp_wifi"
)

target_compile_options(${COMPONENT_LIB} INTERFACE -Wno-unknown-pragmas)
