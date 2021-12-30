#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>

#define GET_WIDTH(x) (*reinterpret_cast<unsigned int *>((x)))
#define GET_HEIGHT(x) (*reinterpret_cast<unsigned int *>((x) + 4))

static const unsigned char pngHeader[8] = {
    0x89, 0x50, 0x4E, 0x47,
    0x0D, 0x0A, 0x1A, 0x0A
};

static const unsigned char jpgHeader[2] = {
    0xFF, 0xD8
};

struct SOF {
    unsigned short height;
    unsigned short width;
    unsigned short segmentSize;
    unsigned char bitsPerPixel;
    unsigned char segmentIdentifier;
};

template<typename T>
T reverseOrder(T value) {
    T result{};

    for (size_t i = 0; i < sizeof(value); ++i) {
        *(reinterpret_cast<unsigned char *>(&result) + i) =
            *(reinterpret_cast<unsigned char *>(&value) + sizeof(value) - i - 1);
    }

    return result;
}

void InitializeSOF(SOF &sof, void *data) {
    sof.segmentSize = reverseOrder(*reinterpret_cast<unsigned short *>(data));
    data = reinterpret_cast<char *>(data) + 1;
    sof.bitsPerPixel = *(reinterpret_cast<unsigned char *>(data));
    data = reinterpret_cast<char *>(data) + 2;
    sof.height = reverseOrder(*reinterpret_cast<unsigned short *>(data));
    data = reinterpret_cast<char *>(data) + 2;
    sof.width = reverseOrder(*reinterpret_cast<unsigned short *>(data));
}

SOF findSOF0Marker(std::ifstream &stream) {
    static const unsigned char SOF0[3] = {
        0xFF, 0xC0, 0xC2
    };

    char buffer[8] = { 0 };
    char byte{};

    std::vector<SOF> segments;

    while (!stream.eof()) {
        stream.read(&byte, sizeof(byte));

        if (static_cast<unsigned char>(byte) == SOF0[0]) {
            do {
                stream.read(&byte, sizeof(byte));
            } while (static_cast<unsigned char>(byte) == SOF0[0]);

            if (static_cast<unsigned char>(byte) == SOF0[2] ||
                static_cast<unsigned char>(byte) == SOF0[1]) {

                SOF sof;
                stream.read(buffer, 7);
                InitializeSOF(sof, buffer);
                sof.segmentIdentifier = byte;

                segments.push_back(sof);
            }
            else {
                 stream.read(buffer, 2);
                 unsigned short skip =
                    reverseOrder(*reinterpret_cast<unsigned short *>(buffer)) - 2;
                 stream.seekg(skip, std::ios_base::cur);
            }
        }
    }

    return segments.size() == 0 ? SOF{} : segments.back();
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        std::cerr << "Enter file name!" << std::endl;
        return 1;
    }

    char header[8];
    char chunkHeader[8];

    for (decltype(argc) i = 1; i < argc; ++i) {
        std::ifstream image(argv[i]);

        image.read(header, sizeof(header));

        if (std::memcmp(header, pngHeader, sizeof(pngHeader)) == 0)
        {
            image.read(chunkHeader, sizeof(chunkHeader));

            unsigned int dataSize = reverseOrder(*reinterpret_cast<unsigned int *>(chunkHeader));

            char *chunkData = new char[dataSize];

            image.read(chunkData, dataSize);

            std::cout << argv[i] << "\n\t";

            std::cout << "Image width: " << reverseOrder(GET_WIDTH(chunkData)) << "\n\t";
            std::cout << "Image height: " << reverseOrder(GET_HEIGHT(chunkData)) << std::endl;

            delete[] chunkData;
        }
        else if (std::memcmp(header, jpgHeader, sizeof(jpgHeader)) == 0) {
            image.seekg(-6, std::ios_base::cur);

            auto sof = findSOF0Marker(image);

            std::cout << argv[i] << "\n\t";
            std::cout << "Image width: " << sof.width << "\n\t";
            std::cout << "Image height: " << sof.height << std::endl;
        }
        else {
            std::cerr << "Unrecognized image format!" << std::endl;
            continue;
        }
    }

    return 0;
}
