#include <iostream>
#include <fstream>
#include <sstream>
#include <zlib.h>

#define CHUNK_SIZE 1024

void compressAndSaveToFile(const char* inputFile, const char* compressedFile) {
    std::ifstream file(inputFile, std::ios::binary);
    if (!file) {
        std::cerr << "Error opening file " << inputFile << std::endl;
        return;
    }

    // Read the file into a stringstream
    std::ostringstream fileStream;
    fileStream << file.rdbuf();
    std::string fileContents = fileStream.str();
    file.close();

    // Compress the file contents using zlib
    z_stream zlibStream;
    zlibStream.zalloc = Z_NULL;
    zlibStream.zfree = Z_NULL;
    zlibStream.opaque = Z_NULL;

    if (deflateInit(&zlibStream, Z_DEFAULT_COMPRESSION) != Z_OK) {
        std::cerr << "Error initializing zlib" << std::endl;
        return;
    }

    zlibStream.avail_in = fileContents.size();
    zlibStream.next_in = (Bytef*)fileContents.data();

    std::ofstream compressedFileStream(compressedFile, std::ios::binary);
    if (!compressedFileStream) {
        std::cerr << "Error opening file " << compressedFile << " for writing" << std::endl;
        deflateEnd(&zlibStream);
        return;
    }

    char compressedBuffer[CHUNK_SIZE];

    do {
        zlibStream.avail_out = CHUNK_SIZE;
        zlibStream.next_out = (Bytef*)compressedBuffer;

        if (deflate(&zlibStream, Z_FINISH) == Z_STREAM_ERROR) {
            std::cerr << "Error compressing data" << std::endl;
            deflateEnd(&zlibStream);
            return;
        }

        compressedFileStream.write(compressedBuffer, CHUNK_SIZE - zlibStream.avail_out);

    } while (zlibStream.avail_out == 0);

    deflateEnd(&zlibStream);
    compressedFileStream.close();
}

void decompressAndSaveToFile(const char* compressedFile, const char* decompressedFile) {
    std::ifstream compressedFileStream(compressedFile, std::ios::binary);
    if (!compressedFileStream) {
        std::cerr << "Error opening file " << compressedFile << std::endl;
        return;
    }

    // Read the compressed file into a stringstream
    std::ostringstream compressedStream;
    compressedStream << compressedFileStream.rdbuf();
    std::string compressedData = compressedStream.str();
    compressedFileStream.close();

    // Decompress the data using zlib
    z_stream zlibStream;
    zlibStream.zalloc = Z_NULL;
    zlibStream.zfree = Z_NULL;
    zlibStream.opaque = Z_NULL;

    if (inflateInit(&zlibStream) != Z_OK) {
        std::cerr << "Error initializing zlib" << std::endl;
        return;
    }

    zlibStream.avail_in = compressedData.size();
    zlibStream.next_in = (Bytef*)compressedData.data();

    std::ofstream decompressedFileStream(decompressedFile, std::ios::binary);
    if (!decompressedFileStream) {
        std::cerr << "Error opening file " << decompressedFile << " for writing" << std::endl;
        inflateEnd(&zlibStream);
        return;
    }

    char decompressedBuffer[CHUNK_SIZE];

    do {
        zlibStream.avail_out = CHUNK_SIZE;
        zlibStream.next_out = (Bytef*)decompressedBuffer;

        if (inflate(&zlibStream, Z_NO_FLUSH) == Z_STREAM_ERROR) {
            std::cerr << "Error decompressing data" << std::endl;
            inflateEnd(&zlibStream);
            return;
        }

        decompressedFileStream.write(decompressedBuffer, CHUNK_SIZE - zlibStream.avail_out);

    } while (zlibStream.avail_out == 0);

    inflateEnd(&zlibStream);
    decompressedFileStream.close();
}

int main() {
    // const char* inputFile = "3d.obj";
    const char* compressedFile = "3dcomp";
    const char* decompressedFile = "3d_decompressed.obj";

    // // Compress the original file
    // compressAndSaveToFile(inputFile, compressedFile);

    

    // Decompress the compressed file
    decompressAndSaveToFile(compressedFile, decompressedFile);

    return 0;
}
