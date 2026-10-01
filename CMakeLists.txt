cmake_minimum_required(VERSION 3.20)
project(DistortX VERSION 1.0.0)

# Descargamos JUCE directamente usando FetchContent (Versión 7)
include(FetchContent)
FetchContent_Declare(
    juce
    URL https://github.com/juce-framework/JUCE/archive/refs/tags/7.0.9.zip
    DOWNLOAD_EXTRACT_TIMESTAMP true
)
FetchContent_MakeAvailable(juce)

# Declaración y Configuración del Plugin
juce_add_plugin(DistortX
    COMPANY_NAME "JZ Audio"
    BUNDLE_ID "com.jzaudio.distortx"
    PLUGIN_MANUFACTURER_CODE Jzau
    PLUGIN_CODE Dist
    # AU es exclusivo de macOS (CMK-01); se activa condicionalmente para evitar
    # declaraciones de formato inválidas en builds de Windows.
    FORMATS VST3 $<$<PLATFORM_ID:Darwin>:AU> Standalone
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

# Archivos de código fuente del plugin
target_sources(DistortX
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/DistortXLookAndFeel.cpp
        Source/DistortXLookAndFeel.h
)

# Generar archivo JuceHeader.h para el target DistortX
juce_generate_juce_header(DistortX)

# ==============================================================================
# Suite de Pruebas Unitarias Automatizadas (Fase 5 - Catch2 v3)
# ==============================================================================
option(BUILD_TESTING "Habilitar compilacion de suite de pruebas unitarias" ON)

if(BUILD_TESTING)
    FetchContent_Declare(
        Catch2
        URL https://github.com/catchorg/Catch2/archive/refs/tags/v3.5.2.zip
        DOWNLOAD_EXTRACT_TIMESTAMP true
    )
    FetchContent_MakeAvailable(Catch2)

    juce_add_console_app(DistortX_Tests
        PRODUCT_NAME "DistortX_Tests"
    )

    target_sources(DistortX_Tests
        PRIVATE
            Tests/MainTestRunner.cpp
            Tests/DSPCurvesTests.cpp
            Tests/FilterStabilityTests.cpp
            Tests/LatencyPDCTests.cpp
            Tests/RealtimeStressTests.cpp
            Tests/APVTSStateTests.cpp
            Source/PluginProcessor.cpp
            Source/PluginProcessor.h
            Source/PluginEditor.cpp
            Source/PluginEditor.h
            Source/DistortXLookAndFeel.cpp
            Source/DistortXLookAndFeel.h
    )

    target_compile_features(DistortX_Tests PRIVATE cxx_std_17)
    target_compile_definitions(DistortX_Tests PRIVATE
        JUCE_VST3_CAN_REPLACE_VST2=0
        JucePlugin_Name="DistortX"
    )

    target_link_libraries(DistortX_Tests
        PRIVATE
            Catch2::Catch2
            juce::juce_audio_utils
            juce::juce_dsp
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags
    )

    juce_generate_juce_header(DistortX_Tests)

    include(CTest)
    include(Catch)
    catch_discover_tests(DistortX_Tests)
endif()
