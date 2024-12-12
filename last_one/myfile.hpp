#ifndef MY_FILE_HPP
#define MY_FILE_HPP

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <memory>
#include <stdexcept>
#include <iomanip>  // 包含 setw 和 setfill

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/evp.h>
// #include <openssl/sha.h>  // 引入 OpenSSL 的 SHA-256 支援
#include <portaudio.h>
#include <sndfile.h>

using namespace std;

#define CHUNK_SIZE 100

#define FRAME_SIZE 100
#define SAMPLE_RATE 44100
#define CHANNELS 1

#define MAX_FILE_SIZE 1024 * 1024 * 10 // 10 MB

namespace FileProcessing{

class FileReader {
    public:
        //! Default implementation for every function
        virtual ~FileReader() = default;
        virtual bool openSendFile(const string& filePath) { 
            throw runtime_error("Not implemented for this FileReader type"); 
        }
        virtual bool openReceiveFile(const string& filePath) { 
            throw runtime_error("Not implemented for this FileReader type"); 
        }
        virtual void process_send(SSL* ssl_client) {
            throw runtime_error("Not implemented for this FileReader type");
        }
        virtual void process_receive_server(SSL* ssl_client) {
            throw runtime_error("Not implemented for this FileReader type");
        }
        virtual void receiveAndSendFile(SSL* ssl_giver, SSL* ssl_target, const string& filename, const string& username) {
            throw runtime_error("Not implemented for this FileReader type");
        }
        virtual void storeFileContentForClient(vector<vector<string>>& fileBuffer) { 
            throw runtime_error("Not implemented for this FileReader type"); 
        }
        virtual void calculateFileHash(uint8_t status) { 
            throw runtime_error("Not implemented for this FileReader type"); 
        }
        virtual string getHash(uint8_t status) { 
            return ""; 
        }
        virtual void close() {}

        virtual bool readAudioFile(const string& filePath) {
            throw runtime_error("Not implemented for this FileReader type");
        }

        virtual bool initAudio() {
            throw runtime_error("Not implemented for this FileReader type");
        }
        virtual void closeAudio() {
            throw runtime_error("Not implemented for this FileReader type");
        }

        virtual void saveAudioFile(const string& filePath) {
            throw runtime_error("Not implemented for this FileReader type");
        }
};

class TextFileReader : public FileReader {
    private:
        ifstream file;
        ofstream receiveFile;
        string fileHash;
        string fileHashReceived;
    public:
        //TODO Calculate the file hash.
        void calculateFileHash(uint8_t status) override;
        string getHash(uint8_t status) override;
        //! For file transfer within client-server
        //TODO filePath Path to the file.
        bool openSendFile(const string& filePath) override; 
        bool openReceiveFile(const string& filePath) override;

        //TODO This function defines how the file content will be handled for sending. 
        void process_send(SSL* ssl_client) override;
        //TODO This function defines how the file content will be handled for receiving.
        void process_receive_server(SSL* ssl_client) override;
        
        //! For file transfer within client-client
        //TODO This function defines how the file will be handled for receiving and sending by server.
        void receiveAndSendFile(SSL* ssl_giver, SSL* ssl_target, const string& filename, const string& username) override;

        //TODO Store the file content for client.
        void storeFileContentForClient(vector<vector<string>>& fileBuffer) override;

        //TODO Closes the file if it is open.
        void close() override;
};

// Add new class for audio handling
class AudioFileReader : public FileReader {
    private:
        PaStream* stream;
        vector<float> audioBuffer;
        SNDFILE* sndFile;
        SF_INFO sfInfo;
        
        // ssize_t totalAudioSize;
        // string audioFileName;
        // string targetUserName;
        
        static int paCallback(const void* inputBuffer, void* outputBuffer,
                            unsigned long framesPerBuffer,
                            const PaStreamCallbackTimeInfo* timeInfo,
                            PaStreamCallbackFlags statusFlags,
                            void* userData);

    public:
        void process_send(SSL* ssl_conn) override;
        void process_receive_server(SSL* ssl_conn) override;
        
        // New methods for audio handling
        bool initAudio() override;
        void closeAudio() override;
        bool readAudioFile(const string& filePath) override;
        void saveAudioFile(const string& filePath) override;
        void startPlayback();
        void stopPlayback();
        void addAudioData(const char* data, size_t size);
};

// A unique_ptr to a FileReader instance. To insure the memory operation.
unique_ptr<FileReader> createFileReader(const string& fileType);

} // namespace FileProcessing

const string FILE_HEADER = "FILE:"; // 設定固定的標頭

bool sendFile(const string& filePath, unique_ptr<FileProcessing::FileReader>& fileReader, SSL* ssl_client);
bool receiveFileServer(const string& filePath, unique_ptr<FileProcessing::FileReader>& fileReader, SSL* ssl_client);
bool reTransferFile(unique_ptr<FileProcessing::FileReader>& fileReader, SSL* ssl_giver, SSL* ssl_target, const string& filename, const string& username);

bool sendAudioFile(const string& filePath, unique_ptr<FileProcessing::FileReader>& fileReader, SSL* ssl_conn);
bool receiveAndSaveAudioFileServer(const string& filePath, unique_ptr<FileProcessing::FileReader>& fileReader, SSL* ssl_conn);
#endif