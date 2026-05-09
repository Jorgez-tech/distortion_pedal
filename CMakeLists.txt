cmake_minimum_required(VERSION 3.20)
project(DistortX VERSION 1.0.0)

# Descargamos JUCE directamente usando FetchContent (Versión 7)
include(FetchContent)
FetchContent_Declare(
    juce
    URL https://github.com/juce-framework/JUCE/archive/refs/tags/7.0.9.zip
)
FetchContent_MakeAvailable(juce)

# Declaración y Configuración del Plugin
juce_add_plugin(DistortX
    COMPANY_NAME "JZ Audio"
    PLUGIN_MANUFACTURER_CODE Jzau
    PLUGIN_CODE Dist
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "DistortX"
)

# Versión de C++
target_compile_features(DistortX PUBLIC cxx_std_17)

# Deshabilitar compatibilidad con VST2 para evitar errores de compilación
target_compile_definitions(DistortX PUBLIC JUCE_VST3_CAN_REPLACE_VST2=0)

# Incluimos los módulos básicos de JUCE necesarios para Audio y UI, y explícitamente el módulo DSP
target_link_libraries(DistortX
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Archivos de código fuente (que vamos a crear)
target_sources(DistortX
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
)

# Generar archivo JuceHeader.h para el target DistortX
juce_generate_juce_header(DistortX)
