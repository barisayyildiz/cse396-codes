#include <iostream>
#include <fstream>
#include <zlib.h>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 12345
#define CHUNK_SIZE 1024
#define BUFFER_SIZE 1024

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

    int result;
    do {
        zlibStream.avail_out = CHUNK_SIZE;
        zlibStream.next_out = (Bytef*)decompressedBuffer;

        result = inflate(&zlibStream, Z_FINISH);

        if (result == Z_NEED_DICT || result == Z_DATA_ERROR || result == Z_MEM_ERROR || result == Z_BUF_ERROR) {
            if (result == Z_BUF_ERROR && zlibStream.avail_out == 0) {
                // Buffer full, write decompressed data to the file
                int decompressedBytes = CHUNK_SIZE - zlibStream.avail_out;
                decompressedFileStream.write(decompressedBuffer, decompressedBytes);
            } else {
                std::cerr << "Error decompressing data: " << zError(result) << std::endl;
                inflateEnd(&zlibStream);
                return;
            }
        } else {
            // Write decompressed data to the file
            int decompressedBytes = CHUNK_SIZE - zlibStream.avail_out;
            decompressedFileStream.write(decompressedBuffer, decompressedBytes);
        }

    } while (result != Z_STREAM_END);

    // Complete the decompression process
    inflateEnd(&zlibStream);
    decompressedFileStream.close();
}


int main() {
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        std::cerr << "Error creating socket" << std::endl;
        return -1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr) <= 0) {
        std::cerr << "Error setting server IP" << std::endl;
        return -1;
    }

    // Connect to the server
    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        std::cerr << "Error connecting to server" << std::endl;
        return -1;
    }

    // Receive and decompress data in chunks
    char buffer[CHUNK_SIZE];
    int bytesRead;

    std::ofstream decompressedFileStream("3dcomp", std::ios::binary);

    do {
        bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);

        if (bytesRead > 0) {
            // Process the received chunk (e.g., decompress and save to file)
            // Note: You need to implement the appropriate logic for handling the received data
            // For example, you can append the received chunk to a buffer or write it to a file
            decompressedFileStream.write(buffer, bytesRead);
            
            // Optionally, send an acknowledgment to the server
            send(clientSocket, "ACK", 3, 0);
        }
    } while (bytesRead > 0);

    decompressAndSaveToFile("3dcomp", "3d_decompressed.obj");

    // Close the socket
    close(clientSocket);

    return 0;
}
