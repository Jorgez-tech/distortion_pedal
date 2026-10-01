#include <catch2/catch_session.hpp>
#include <JuceHeader.h>

int main (int argc, char* argv[])
{
    // Inicializar el subsistema GUI/MessageManager de JUCE de forma headless
    juce::ScopedJuceInitialiser_GUI juceInit;

    // Ejecutar la suite Catch2
    int result = Catch::Session().run (argc, argv);

    return result;
}
