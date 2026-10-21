#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>

namespace SilKitPy {

// Infer the wire DLC without silently padding or truncating the payload.
inline std::uint16_t InferCanDlc(std::size_t size, bool fd, bool xl)
{
    if (xl) {
        if (size < 1 || size > 2048)
            throw std::invalid_argument("CAN XL payload must contain 1 to 2048 bytes");
        return static_cast<std::uint16_t>(size - 1);
    }
    if (fd) {
        constexpr std::size_t lengths[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64};
        for (std::uint16_t dlc = 0; dlc < 16; ++dlc) {
            if (size == lengths[dlc])
                return dlc;
        }
        throw std::invalid_argument("CAN FD payload length must be 0..8, 12, 16, 20, 24, 32, 48 or 64 bytes; pad the payload explicitly");
    }
    if (size > 8)
        throw std::invalid_argument("Classic CAN payload must contain at most 8 bytes");
    return static_cast<std::uint16_t>(size);
}

} // namespace SilKitPy
