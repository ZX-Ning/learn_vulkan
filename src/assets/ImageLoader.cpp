#include "ImageLoader.hpp"

#include <limits>
#include <stdexcept>

const Image Image::readImage(std::span<const uint8_t> imgFileData) {
    if (imgFileData.empty() ||
        imgFileData.size() > std::numeric_limits<int>::max()) {
        throw std::runtime_error("Unsupported image file size");
    }
    Image image;
    uint8_t* data = stbi_load_from_memory(
        imgFileData.data(),
        static_cast<int>(imgFileData.size()),
        &image.width,
        &image.height,
        &image.channels,
        4
    );
    if (data == nullptr) {
        throw std::runtime_error("Can not read image file.");
    }
    image.channels = 4;
    image.data = ImageDataWrapper(data);
    return image;
}

std::span<const uint8_t> Image::rawData() const {
    return {data.get(), static_cast<std::size_t>(width) * height * channels};
}
