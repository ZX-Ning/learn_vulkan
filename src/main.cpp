#include <print>
#include <string_view>

#include "App.hpp"
#include "ComputeTest.hpp"

int main(int argc, char* argv[]) {
    if (argc > 2) {
        std::println(
            stderr, "Usage: {} [--compute|model.gltf|model.glb]", argv[0]
        );
        return 1;
    }
    try {
        if (argc == 2 && std::string_view(argv[1]) == "--compute") {
            return runComputeTest();
        }
        else {
            App app(
                !IS_RELEASE,
                argc == 2 ? std::filesystem::path(argv[1])
                          : std::filesystem::path{}
            );
            app.run();
        }
    }
    catch (const std::exception& e) {
        std::println(stderr, "Error: {}", e.what());
        return 1;
    }
    std::println("Done.");

    return 0;
}
