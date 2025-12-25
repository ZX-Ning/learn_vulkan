#ifndef UTILS_HPP
#define UTILS_HPP

#include <filesystem>
#include <fstream>
#include <vector>

#define DISABLE_COPY(ClassName)                      \
public:                                              \
    ClassName(const ClassName&) = delete;            \
    ClassName& operator=(const ClassName&) = delete;

inline std::vector<char> readFile(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }
    size_t fileSize = std::filesystem::file_size(filePath);
    std::vector<char> buffer(fileSize);
    file.read(buffer.data(), fileSize);
    file.close();
    return buffer;
}

template <class T = int>
struct Size2D {
    static_assert(std::is_integral_v<T>);
    T width;
    T height;
    bool greaterThanZero() {
        return width > 0 && height > 0;
    }
    template <class U>
    operator Size2D<U>() {
        return {
            static_cast<U>(this->width),
            static_cast<U>(this->height)
        };
    }
};

#endif  // UTILS_HPP
