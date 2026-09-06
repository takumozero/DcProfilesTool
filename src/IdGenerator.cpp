#include "IdGenerator.h"

#include <random>
#include <sstream>
#include <iomanip>

std::string IdGenerator::generate()
{
    static std::random_device randomDevice;
    static std::mt19937 generator(randomDevice());

    std::uniform_int_distribution<unsigned int> distribution(
        0x00000000,
        0xFFFFFFFF
    );

    const unsigned int value = distribution(generator);

    std::stringstream stream;

    stream << std::uppercase
        << std::hex
        << std::setw(8)
        << std::setfill('0')
        << value;

    return stream.str();
}