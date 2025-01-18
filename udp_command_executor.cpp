#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

class UdpCommandExecutor {
public:
    // Constructor allows specifying port, delimiter, end-marker, and log file
    UdpCommandExecutor(
        int port = 7755,
        const std::string& delimiter = "::",
        const std::string& endMarker = "#END#",
        const std::string& logFilePath = "/tmp/command_executor.log"
    )
    : port_(port),
      delimiter_(delimiter),
      endMarker_(endMarker),
      logFilePath_(logFilePath),
      sockfd_(-1),
      lastTimestampProcessed_(-1) // Initialize with -1 as a sentinel
    {
    }

    // Destructor: close socket if open
    ~UdpCommandExecutor() {
        if (sockfd_ >= 0) {
            close(sockfd_);
        }
    }

    // Parse a received message to extract (timestamp, command).
    // Returns true if parsing is successful, false otherwise.
    bool parseMessage(const std::string &message,
                      long long &outTimestamp,
                      std::string &outCommand) const
    {
        // Find the delimiter (e.g. "::")
        size_t delimiterPos = message.find(delimiter_);
        if (delimiterPos == std::string::npos) {
            return false;
        }

        // Extract timestamp substring
        std::string timestampStr = message.substr(0, delimiterPos);

        // After the delimiter, find the end-marker (e.g. "#END#")
        size_t commandStartPos = delimiterPos + delimiter_.size();
        size_t endMarkerPos = message.find(endMarker_, commandStartPos);
        if (endMarkerPos == std::string::npos) {
            return false;
        }

        // Extract the command substring
        std::string commandStr = message.substr(commandStartPos,
                                                endMarkerPos - commandStartPos);

        // Convert timestamp to long long
        try {
            outTimestamp = std::stoll(timestampStr);
        } catch (...) {
            return false;
        }

        // Trim trailing newlines/spaces from command
        outCommand = trim(commandStr);
        return true;
    }

    // Execute a command in the shell and return the output
    std::string executeCommand(const std::string &cmd) const {
        std::string output;
        FILE *pipe = popen(cmd.c_str(), "r");
        if (!pipe) {
            return "[Error opening pipe]";
        }
        char buffer[256];
        while (!feof(pipe)) {
            if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                output += buffer;
            }
        }
        pclose(pipe);
        return output;
    }

    // Log the command and its output
    void logCommand(long long timestampVal, 
                    const std::string &command, 
                    const std::string &output) const
    {
        std::ofstream logFile(logFilePath_, std::ios::app);
        if (logFile.is_open()) {
            logFile << "=====\n";
            logFile << "Timestamp: " << timestampVal << "\n";
            logFile << "Command: " << command << "\n";
            logFile << "Output:\n" << output << "\n";
            logFile << "=====\n";
        }
    }

    // Main method to listen for UDP commands and process them
    bool run() {
        // Create socket
        sockfd_ = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd_ < 0) {
            std::cerr << "Error: Cannot create socket\n";
            return false;
        }

        // Bind socket to the specified port
        sockaddr_in serverAddr;
        std::memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(port_);

        if (bind(sockfd_, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            std::cerr << "Error: Cannot bind socket to port " << port_ << "\n";
            close(sockfd_);
            sockfd_ = -1;
            return false;
        }

        std::cout << "Listening for UDP packets on port " << port_ << " ...\n";

        // Buffer to store incoming data
        const int BUF_SIZE = 1024;
        char buffer[BUF_SIZE];

        // Main loop
        while (true) {
            sockaddr_in clientAddr;
            socklen_t addrLen = sizeof(clientAddr);
            std::memset(buffer, 0, BUF_SIZE);

            ssize_t recvLen = recvfrom(sockfd_, buffer, BUF_SIZE - 1, 0,
                                       (struct sockaddr*)&clientAddr, &addrLen);
            if (recvLen < 0) {
                std::cerr << "Error: recvfrom failed\n";
                break;
            }

            std::string receivedData(buffer, recvLen);

            long long timestampVal;
            std::string commandStr;
            bool parsed = parseMessage(receivedData, timestampVal, commandStr);
            if (!parsed) {
                // Invalid format, ignore
                continue;
            }

            // If new timestamp <= last processed, ignore this command
            // (since timestamps are strictly increasing)
            if (timestampVal <= lastTimestampProcessed_) {
                continue;
            }

            // Update the last processed timestamp
            lastTimestampProcessed_ = timestampVal;

            // Execute the command
            std::string resultOutput = executeCommand(commandStr);

            // Log the command and its output
            logCommand(timestampVal, commandStr, resultOutput);
        }

        close(sockfd_);
        sockfd_ = -1;
        return true;
    }

private:
    int port_;
    std::string delimiter_;
    std::string endMarker_;
    std::string logFilePath_;
    int sockfd_;

    // Now we only keep one last processed timestamp
    long long lastTimestampProcessed_;

    // Helper function to trim trailing newlines/spaces
    std::string trim(const std::string &s) const {
        std::string result = s;
        while (!result.empty() && 
              (result.back() == '\n' || result.back() == '\r' || result.back() == ' ')) 
        {
            result.pop_back();
        }
        return result;
    }
};

int main(int argc, char *argv[]) {
    int port = 7755;
    if (argc > 1) {
        port = std::atoi(argv[1]);
        if (port <= 0) {
            std::cerr << "Invalid port. Using default 7755.\n";
            port = 7755;
        }
    }

    UdpCommandExecutor executor(
        port,         // Port
        "::",         // Delimiter
        "#END#",      // End marker
        "/tmp/command_executor.log"
    );

    if (!executor.run()) {
        std::cerr << "Failed to run UDP command executor.\n";
        return 1;
    }

    return 0;
}
