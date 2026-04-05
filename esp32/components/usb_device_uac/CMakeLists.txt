# USB Device UAC only supports targets with USB OTG hardware: esp32s2, esp32s3, esp32p4
# Return early for unsupported targets to avoid build errors
if(NOT IDF_TARGET STREQUAL "esp32s2" AND NOT IDF_TARGET STREQUAL "esp32s3" AND NOT IDF_TARGET STREQUAL "esp32p4")
    idf_component_register()
    return()
endif()

set(priv_requires esp_timer esp_hw_support) # esp_hw_support provides usb_new_phy (USB PHY API)

# Determine whether tinyusb is fetched from component registry or from local path
idf_build_get_property(build_components BUILD_COMPONENTS)
if(tinyusb IN_LIST build_components)
    set(tinyusb_name tinyusb) # Local component
else()
    set(tinyusb_name espressif__tinyusb) # Managed component
endif()
list(APPEND priv_requires ${tinyusb_name})

# Add USB descriptors to component sources so they get compiled and linked
set(component_srcs "usb_device_uac.c" "tusb/usb_descriptors.c")

idf_component_register(SRCS ${component_srcs}
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES ${priv_requires})

# Note: Symbol forcing is done at project level in main/CMakeLists.txt
# Component-level linker flags don't propagate to the final executable

# Configure tinyusb to use our tusb_config.h and descriptors
if(NOT DEFINED CONFIG_USB_DEVICE_UAC_AS_PART OR NOT CONFIG_USB_DEVICE_UAC_AS_PART)
    idf_component_get_property(tusb_lib ${tinyusb_name} COMPONENT_LIB)

    target_include_directories(${tusb_lib} BEFORE PRIVATE 
        "${CMAKE_CURRENT_LIST_DIR}/tusb"
        "${CMAKE_CURRENT_LIST_DIR}/tusb_uac"
    )
    target_compile_options(${tusb_lib} PRIVATE 
        "-I${CMAKE_CURRENT_LIST_DIR}/tusb"
        "-I${CMAKE_CURRENT_LIST_DIR}/tusb_uac"
    )
    target_include_directories(${COMPONENT_LIB} PRIVATE 
        "${CMAKE_CURRENT_LIST_DIR}/tusb"
        "${CMAKE_CURRENT_LIST_DIR}/tusb_uac"
    )
endif()

include(package_manager)
cu_pkg_define_version(${CMAKE_CURRENT_LIST_DIR})
