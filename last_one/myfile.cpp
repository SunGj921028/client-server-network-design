#include "myfile.hpp"

// Open the file for sending
bool FileProcessing::TextFileReader::openSendFile(const string& filePath) {
    file.open(filePath, ios::binary | ios::in);
    if (!file.is_open()) {
        throw runtime_error("Failed to open text file: " + filePath + ". File may not exist or be inaccessible.");
        return false;
    }else{ return true;}
}

// Open the file for receiving
bool FileProcessing::TextFileReader::openReceiveFile(const string& filePath) {
    receiveFile.open(filePath, ios::binary | ios::out);
    if (!receiveFile.is_open()) {
        throw runtime_error("Failed to open text file: " + filePath + ". File may not exist or be inaccessible.");
        return false;
    }else{ return true;}
}

// TODO ACK 機制（After sending a chunk, wait for the server to acknowledge the receipt of the chunk before sending the next chunk.）
void FileProcessing::TextFileReader::process_send(SSL* ssl_client) {
    char buffer[CHUNK_SIZE] = {0};
    int countChunk = 0;
    cout << "-----------------------------------------------------------------\n";
    while (1) {
        memset(buffer, 0, CHUNK_SIZE);
        file.read(buffer, CHUNK_SIZE);
        streamsize bytesRead = file.gcount(); // 實際讀取的字節數
        if (bytesRead <= 0) { break; }

        const char* bufferPtr = buffer;
        cout << "Sending " << ++countChunk << " chunk of " << bytesRead << " bytes\n";
        while(bytesRead > 0){
            ssize_t bytesSent = SSL_write(ssl_client, bufferPtr, bytesRead);
            if (bytesSent <= 0) {
                int ssl_error = SSL_get_error(ssl_client, bytesSent);
                if (ssl_error == SSL_ERROR_WANT_READ || ssl_error == SSL_ERROR_WANT_WRITE) {
                    continue; // 暫時無法讀取，繼續重試
                }else if(ssl_error == SSL_ERROR_SYSCALL){
                    throw runtime_error("SSL_write failed due to system error: " + string(strerror(errno)));
                }else if(ssl_error == SSL_ERROR_SSL){
                    throw runtime_error("SSL_write failed due to SSL protocol error.");
                }else{
                    throw runtime_error("SSL_write encountered unknown error.");
                }
            }
            bytesRead -= bytesSent;
            bufferPtr += bytesSent;
        }
    }
    SSL_write(ssl_client, "EOF", 3);
    cout << "-----------------------------------------------------------------\n";
}

void FileProcessing::TextFileReader::process_receive_server(SSL* ssl_client) {
    ssize_t totalBytesReceived = 0;
    char buffer[CHUNK_SIZE] = {0};
    int countChunk = 0;
    cout << "-----------------------------------------------------------------\n";
    cout << "Receiving...\n";
    while (1) {
        memset(buffer, 0, CHUNK_SIZE);
        ssize_t bytesRead = SSL_read(ssl_client, buffer, CHUNK_SIZE);
        if (bytesRead <= 0) {
            int ssl_error = SSL_get_error(ssl_client, bytesRead);
            if (ssl_error == SSL_ERROR_WANT_READ || ssl_error == SSL_ERROR_WANT_WRITE) {
                continue; // 暫時無法讀取，繼續重試
            }else if (ssl_error == SSL_ERROR_ZERO_RETURN) {
                // Clean shutdown
                //? Means receive file successfully and end
                break;
            }else if(ssl_error == SSL_ERROR_SYSCALL){
                throw runtime_error("SSL_read failed due to system error: " + string(strerror(errno)));
            }else if(ssl_error == SSL_ERROR_SSL){
                throw runtime_error("SSL_read failed due to SSL protocol error.");
            }else{
                throw runtime_error("SSL_read encountered unknown error.");
            }
        }
        //! EOF 
        if(string(buffer, bytesRead).find("EOF") != string::npos){
            break;
        }
        totalBytesReceived += bytesRead;
        if(totalBytesReceived > MAX_FILE_SIZE){
            throw runtime_error("File size exceeds the maximum limit of " + to_string(MAX_FILE_SIZE) + " bytes.");
        }
        // Write the received data to file
        receiveFile.write(buffer, bytesRead);
        if (!receiveFile) {
            throw runtime_error("Failed to write to file");
        }
        cout << "Received " << ++countChunk << " chunk of " << bytesRead << " bytes\n";
    }
    cout << "\nFile reception complete.\n";
    cout << "-----------------------------------------------------------------\n";
}

void FileProcessing::TextFileReader::receiveAndSendFile(SSL* ssl_giver, SSL* ssl_target, const string& filename, const string& username) {
    ssize_t totalBytesReceived = 0;
    char buffer[CHUNK_SIZE] = {0};
    int countChunk = 0;
    cout << "\n-----------------------------------------------------------------\n";
    cout << "Receiving for re-sending to specific client...\n";
    vector<vector<char>> chunks;

    // 初始化 SHA-256 context
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (mdctx == nullptr) {
        throw runtime_error("Failed to create EVP_MD_CTX.");
    }
    if (EVP_DigestInit_ex(mdctx, EVP_sha256(), nullptr) != 1) {
        EVP_MD_CTX_free(mdctx);
        throw runtime_error("Failed to initialize SHA-256.");
    }

    while (1) {
        memset(buffer, 0, CHUNK_SIZE);
        ssize_t bytesRead = SSL_read(ssl_giver, buffer, CHUNK_SIZE);
        if (bytesRead <= 0) {
            int ssl_error = SSL_get_error(ssl_giver, bytesRead);
            if (ssl_error == SSL_ERROR_WANT_READ || ssl_error == SSL_ERROR_WANT_WRITE) {
                continue; // 暫時無法讀取，繼續重試
            }else if (ssl_error == SSL_ERROR_ZERO_RETURN) {
                // Clean shutdown
                //? Means receive file successfully and connection end
                break;
            }else if(ssl_error == SSL_ERROR_SYSCALL){
                throw runtime_error("SSL_read failed due to system error: " + string(strerror(errno)));
            }else if(ssl_error == SSL_ERROR_SSL){
                throw runtime_error("SSL_read failed due to SSL protocol error.");
            }else{
                throw runtime_error("SSL_read encountered unknown error.");
            }
        }
        //! EOF 
        if(string(buffer, bytesRead).find("EOF") != string::npos){
            break;
        }

        // 同時計算 hash 和儲存數據
        if (EVP_DigestUpdate(mdctx, buffer, bytesRead) != 1) {
            EVP_MD_CTX_free(mdctx);
            throw runtime_error("Failed to update SHA-256 hash.");
        }

        // 將讀取到的資料加入暫時緩衝
        chunks.emplace_back(buffer, buffer + bytesRead);
        totalBytesReceived += bytesRead;
        if(totalBytesReceived > MAX_FILE_SIZE){
            memset(buffer, 0, CHUNK_SIZE);
            chunks.clear();
            EVP_MD_CTX_free(mdctx); //! Free the SHA-256 context
            throw runtime_error("File size exceeds the maximum limit of " + to_string(MAX_FILE_SIZE) + " bytes.");
        }
    }
    cout << "Receive complete. File total size: " << totalBytesReceived << " bytes\n";

    // 完成 hash 計算
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len = 0;
    if (EVP_DigestFinal_ex(mdctx, hash, &hash_len) != 1) {
        EVP_MD_CTX_free(mdctx);
        throw runtime_error("Failed to finalize SHA-256 hash.");
    }
    EVP_MD_CTX_free(mdctx);

    // 將 hash 轉換為十六進制字串
    std::ostringstream oss;
    for (unsigned int i = 0; i < hash_len; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    string hashValueOfRetransmitFile = oss.str();
    cout << "Hash value of the re-sent file is: " << hashValueOfRetransmitFile << endl << endl;

    string chunkHeader = "INFO:" + to_string(totalBytesReceived) + " bytes | " + filename + " | " + hashValueOfRetransmitFile + " | " + username;
    if(SSL_write(ssl_target, chunkHeader.c_str(), chunkHeader.length()) <= 0){
        throw runtime_error("Failed to send chunk header to target client.");
    }

    // Send file content
    for (const auto& chunk : chunks) {
        // 构建带有前缀的缓冲区
        vector<char> prefixedChunk(FILE_HEADER.begin(), FILE_HEADER.end());
        prefixedChunk.insert(prefixedChunk.end(), chunk.begin(), chunk.end());
        ssize_t bytesSent = SSL_write(ssl_target, prefixedChunk.data(), prefixedChunk.size());

        if (bytesSent <= 0) {
            int ssl_error = SSL_get_error(ssl_target, bytesSent);
            if (ssl_error == SSL_ERROR_WANT_READ || ssl_error == SSL_ERROR_WANT_WRITE) {
                continue;
            } else {
                throw runtime_error("SSL_write failed: unexpected error.");
            }
        }

        cout << "Sent chunk of size: " << bytesSent - FILE_HEADER.size() << " bytes\n";
    }
    // Send EOF to target client
    SSL_write(ssl_target, "EOF", 3);
    cout << "\nFile reception and re-sending complete.\n";
    cout << "-----------------------------------------------------------------\n";
}

void FileProcessing::TextFileReader::storeFileContentForClient(vector<vector<string>>& fileBuffer) {
    //! Write the file content to the file
    for (const auto& chunk : fileBuffer) {
        for (const auto& str : chunk) {
            // Write the string content to the file
            receiveFile.write(str.c_str(), str.length());
            if (!receiveFile) {
                throw runtime_error("Failed to write chunk to file");
            }
            cout << "Received chunk of size: " << str.length() << " bytes\n";
        }
    }
    receiveFile.flush();  // Ensure all data is written to disk
}

void FileProcessing::TextFileReader::close() {
    if (file.is_open()) {
        file.close();
    }
    if (receiveFile.is_open()) {
        receiveFile.close();
    }
}

void FileProcessing::TextFileReader::calculateFileHash(uint8_t status) {
    //! Calculate the hash of the file being sent
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len = 0;
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (mdctx == nullptr) {
        throw runtime_error("Failed to create EVP_MD_CTX.");
    }
    // 初始化 SHA-256
    if (EVP_DigestInit_ex(mdctx, EVP_sha256(), nullptr) != 1) {
        EVP_MD_CTX_free(mdctx);
        throw runtime_error("Failed to initialize SHA-256.");
    }
    char buffer[1024];
    file.seekg(0);  // 回到檔案開頭
    while (file.read(buffer, sizeof(buffer))) {
        if (EVP_DigestUpdate(mdctx, buffer, file.gcount()) != 1) {
            EVP_MD_CTX_free(mdctx);
            throw runtime_error("Failed to update SHA-256 hash.");
        }
    }
    // 處理剩餘的字節
    if (EVP_DigestUpdate(mdctx, buffer, file.gcount()) != 1) {
        EVP_MD_CTX_free(mdctx);
        throw runtime_error("Failed to update SHA-256 hash.");
    }

    // 完成哈希計算
    if (EVP_DigestFinal_ex(mdctx, hash, &hash_len) != 1) {
        EVP_MD_CTX_free(mdctx);
        throw runtime_error("Failed to finalize SHA-256 hash.");
    }

    EVP_MD_CTX_free(mdctx);

    // 將哈希值轉換為十六進制字串
    std::ostringstream oss;
    for (unsigned int i = 0; i < hash_len; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    if(status == 0){
        //! Store the hash of the file being sent
        fileHash = oss.str();
    }else if(status == 1){
        //! Store the hash of the file being received
        fileHashReceived = oss.str();
    }
    
}

string FileProcessing::TextFileReader::getHash(uint8_t status) {
    if(status == 0){ return fileHash;}
    else { return fileHashReceived;}
}

unique_ptr<FileProcessing::FileReader> FileProcessing::createFileReader(const string& fileType) {
    if (fileType == "txt" || fileType == "md") {
        return make_unique<FileProcessing::TextFileReader>();
    } else if (fileType == "wav" || fileType == "mp3") {
        return make_unique<FileProcessing::AudioFileReader>();
    }
    // 支援未來更多檔案類型
    throw runtime_error("Unsupported file type: " + fileType);
}

bool sendFile(const string& filePath, unique_ptr<FileProcessing::FileReader>& fileReader, SSL* ssl_client){
    try {
        //! Calculate the hash of the file
        fileReader->openSendFile(filePath);
        fileReader->calculateFileHash(0);
        fileReader->close();
        //! Send the file
        fileReader->openSendFile(filePath);
        fileReader->process_send(ssl_client);
        fileReader->close();
        return true;
    } catch (const exception& e) {
        cerr << "Error during file sending: " << e.what() << endl;
        return false;
    }
}

bool receiveFileServer(const string& filePath, unique_ptr<FileProcessing::FileReader>& fileReader, SSL* ssl_client){
    try {
        fileReader->openReceiveFile(filePath);
        fileReader->process_receive_server(ssl_client);
        fileReader->close();
        return true;
    }catch (const exception& e) {
        cerr << "Error during file receiving: " << e.what() << endl;
        return false;
    }
}

bool reTransferFile(unique_ptr<FileProcessing::FileReader>& fileReader, SSL* ssl_giver, SSL* ssl_target, const string& filename, const string& username){
    try {
        fileReader->receiveAndSendFile(ssl_giver, ssl_target, filename, username);
        return true;
    }catch (const exception& e) {
        cerr << "Error during file re-sending: " << e.what() << endl;
        return false;
    }
}

void FileProcessing::AudioFileReader::closeAudio() {
    stopPlayback();
    if (stream) {
        Pa_CloseStream(stream);
    }
    Pa_Terminate();
    if (sndFile) {
        sf_close(sndFile);
    }
}

bool FileProcessing::AudioFileReader::initAudio() {
    PaError err = Pa_Initialize();
    memset(&sfInfo, 0, sizeof(sfInfo));
    if (err != paNoError) {
        cerr << "PortAudio error: " << Pa_GetErrorText(err) << endl;
        return false;
    }

    // Open audio stream
    err = Pa_OpenDefaultStream(&stream,
                             0,          // no input channels
                             CHANNELS,   // stereo output
                             paFloat32,  // sample format
                             SAMPLE_RATE,
                             FRAME_SIZE, // frames per buffer
                             paCallback,
                             this);      // pointer to this instance
    
    if (err != paNoError) {
        cerr << "PortAudio error: " << Pa_GetErrorText(err) << endl;
        return false;
    }
    
    return true;
}

int FileProcessing::AudioFileReader::paCallback(const void* inputBuffer, void* outputBuffer,
                                              unsigned long framesPerBuffer,
                                              const PaStreamCallbackTimeInfo* timeInfo,
                                              PaStreamCallbackFlags statusFlags,
                                              void* userData) {
    AudioFileReader* reader = static_cast<AudioFileReader*>(userData);
    float* out = static_cast<float*>(outputBuffer);
    
    // Copy data from audioBuffer to output
    size_t framesToPlay = min(framesPerBuffer * CHANNELS, 
                             reader->audioBuffer.size());
    
    if (framesToPlay == 0) {
        // No data available, output silence
        memset(out, 0, framesPerBuffer * CHANNELS * sizeof(float));
        return paContinue;
    }
    
    // Copy available data
    memcpy(out, reader->audioBuffer.data(), 
           framesToPlay * sizeof(float));
    
    // Remove played data from buffer
    reader->audioBuffer.erase(
        reader->audioBuffer.begin(),
        reader->audioBuffer.begin() + framesToPlay
    );
    
    return paContinue;
}

void FileProcessing::AudioFileReader::startPlayback() {
    if (stream) {
        PaError err = Pa_StartStream(stream);
        if (err != paNoError) {
            cerr << "PortAudio error: " << Pa_GetErrorText(err) << endl;
        }
    }
}

void FileProcessing::AudioFileReader::stopPlayback() {
    if (stream) {
        Pa_StopStream(stream);
    }
}

void FileProcessing::AudioFileReader::addAudioData(const char* data, size_t size) {
    // Convert incoming audio data to float samples
    const float* samples = reinterpret_cast<const float*>(data);
    size_t numSamples = size / sizeof(float);
    
    // Add to buffer
    audioBuffer.insert(audioBuffer.end(), samples, samples + numSamples);
}

void FileProcessing::AudioFileReader::saveAudioFile(const string& filePath){
    // 設置音頻文件參數
    SF_INFO sfInfo;
    sfInfo.samplerate = SAMPLE_RATE;
    sfInfo.channels = CHANNELS;
    sfInfo.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT; // 使用浮點格式

    // 打開音頻文件
    SNDFILE* outFile = sf_open(filePath.c_str(), SFM_WRITE, &sfInfo);
    if (!outFile) {
        throw std::runtime_error("Failed to open audio file for writing: " + 
                                 std::string(sf_strerror(nullptr)));
    }

    // 寫入音頻數據
    sf_count_t written = sf_write_float(outFile, audioBuffer.data(), audioBuffer.size());
    if (written != static_cast<sf_count_t>(audioBuffer.size())) {
        sf_close(outFile);
        throw std::runtime_error("Failed to write all audio data to file");
    }

    cout << "Audio file successfully saved at: " << filePath << endl;

    // 關閉文件
    sf_close(outFile);
}

void FileProcessing::AudioFileReader::process_receive_server(SSL* ssl_client) {
    char buffer[FRAME_SIZE] = {0};
    
    cout << "-----------------------------------------------------------------\n";
    cout << "Receiving audio file and start streaming...\n";

    startPlayback();
    
    while (1) {
        memset(buffer, 0, FRAME_SIZE);
        ssize_t bytesRead = SSL_read(ssl_client, buffer, FRAME_SIZE);
        
        if (bytesRead <= 0) {
            int ssl_error = SSL_get_error(ssl_client, bytesRead);
            if (ssl_error == SSL_ERROR_WANT_READ || ssl_error == SSL_ERROR_WANT_WRITE) {
                continue; // 暫時無法讀取，繼續重試
            }else if (ssl_error == SSL_ERROR_ZERO_RETURN) {
                // Clean shutdown
                //? Means receive file successfully and end
                break;
            }else if(ssl_error == SSL_ERROR_SYSCALL){
                throw runtime_error("SSL_read failed due to system error: " + string(strerror(errno)));
            }else if(ssl_error == SSL_ERROR_SSL){
                throw runtime_error("SSL_read failed due to SSL protocol error.");
            }else{
                throw runtime_error("SSL_read encountered unknown error.");
            }
        }

        string message(buffer, bytesRead);
        if(message.find("EOF") != string::npos){break;}

        // Process audio data directly
        addAudioData(buffer, bytesRead);
        cout << "Received audio frame of " << bytesRead << " bytes\n";
    }
    
    cout << "\nAudio stream complete.\n";
    cout << "-----------------------------------------------------------------\n";
}

bool FileProcessing::AudioFileReader::readAudioFile(const string& filePath) {
    sndFile = sf_open(filePath.c_str(), SFM_READ, &sfInfo);
    if (!sndFile) {
        cerr << "Error opening sound file: " << sf_strerror(nullptr) << endl;
        return false;
    }

    // Read the entire file into our buffer
    vector<float> tempBuffer(FRAME_SIZE * sfInfo.channels);
    
    while (true) {
        sf_count_t numRead = sf_readf_float(sndFile, tempBuffer.data(), FRAME_SIZE);
        if (numRead <= 0) break; //! EOF
        
        // Add the read data to our main buffer
        audioBuffer.insert(audioBuffer.end(), 
                         tempBuffer.begin(), 
                         tempBuffer.begin() + numRead * sfInfo.channels);
    }

    return true;
}

void FileProcessing::AudioFileReader::process_send(SSL* ssl_conn) {
    size_t totalSize = audioBuffer.size() * sizeof(float);

    // Send audio data in frames
    cout << "-----------------------------------------------------------------\n";
    size_t bytesSent = 0;
    while (bytesSent < totalSize) {
        size_t remainingBytes = totalSize - bytesSent;
        size_t bytesToSend = min(remainingBytes, static_cast<size_t>(FRAME_SIZE));
        
        // Add audio data to frame
        const char* dataPtr = reinterpret_cast<const char*>(audioBuffer.data()) + bytesSent;
        ssize_t result = SSL_write(ssl_conn, dataPtr, bytesToSend);
        
        if (result <= 0) {
            int ssl_error = SSL_get_error(ssl_conn, result);
            if (ssl_error == SSL_ERROR_WANT_WRITE || 
                ssl_error == SSL_ERROR_WANT_READ) {
                continue;
            }
            throw runtime_error("SSL write error");
        }
        
        bytesSent += bytesToSend;
        cout << "Sent audio frame of " << bytesToSend << " bytes\n";
    }
    cout << "-----------------------------------------------------------------\n";
    
    // Send EOF marker
    SSL_write(ssl_conn, "EOF", 3);
}

bool sendAudioFile(const string& filePath, unique_ptr<FileProcessing::FileReader>& fileReader, SSL* ssl_conn){
    try {
        if(!fileReader->readAudioFile(filePath)){
            return false;
        }
        fileReader->process_send(ssl_conn);
        return true;
    } catch (const exception& e) {
        cerr << "Error during audio sending: " << e.what() << endl;
        return false;
    }
}

bool receiveAndSaveAudioFileServer(const string& filePath, unique_ptr<FileProcessing::FileReader>& fileReader, SSL* ssl_conn){
    try {
        fileReader->initAudio();  // Initialize audio for playback
        fileReader->process_receive_server(ssl_conn);
        //! Switch file name of path to .wav
        string newFilePath = filePath.substr(0, filePath.find_last_of('.')) + ".wav";
        fileReader->saveAudioFile(newFilePath);
        fileReader->closeAudio();
        return true;
    }catch (const exception& e) {
        cerr << "Error during file receiving: " << e.what() << endl;
        return false;
    }
}

