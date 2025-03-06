// remotechannel.hpp

#ifndef REMOTECHANNEL_HPP
#define REMOTECHANNEL_HPP

#include <Arduino.h>
#include <WiFi.h>
#include <array>
#include <queue>

extern "C" {
    extern void handleMoveServos(char *message);
}

class RemoteChannel {
public:
    enum class Status {
        OK,
        NOT_LISTENING,
        INVALID_FORMAT,
        NO_CLIENT,
        TIMEOUT,
        SERVER_ERROR
    };

    static constexpr size_t MESSAGE_LENGTH = 11;  // '%' + 10 hex chars
    static constexpr unsigned long DEFAULT_TIMEOUT = 1000; // 1 second
    static constexpr unsigned long KEEPALIVE_TIMEOUT = 5000000; 
    
    /**
     * @brief Construct a new Remote Channel object
     * @param listenPort Port number to listen on
     */
    explicit RemoteChannel(int listenPort)
        : server(listenPort)
        , port(listenPort)
        , receivedChars(0)
        , isListening(false)
        , lastError(Status::OK)
        , timeout(DEFAULT_TIMEOUT)
        , lastClientActivity(0)
        , currentState(State::IDLE) {
        buffer.fill('\0');
    }

    /**
     * @brief Start the server
     * @return true if server started successfully
     */
    bool begin() {
        server.begin();

        isListening = true;
        log("Server started on IP: " + WiFi.softAPIP().toString() + " on port " + String(port));
        return true;
    }

    /**
     * @brief Stop the server
     */
    void end() {
        log("disconnect2");
        disconnectClient();
        server.stop();
        isListening = false;
        clearMessageQueue();
    }

    /**
     * @brief Check if server is currently listening
     * @return true if server is active
     */
    bool isServerActive() const {
        return isListening;
    }

    /**
     * @brief Get the last error status
     * @return Status code of the last error
     */
    Status getLastError() const {
        return lastError;
    }

    /**
     * @brief Check if there are messages available
     * @return true if messages are in the queue
     */
    bool hasMessages() const {
        return !messageQueue.empty();
    }

    /**
     * @brief Get the next message from the queue
     * @return String containing the message, or empty string if no messages
     */
    String getNextMessage() {
        if (messageQueue.empty()) {
            return "";
        }
        
        String message = messageQueue.front();
        messageQueue.pop();
        return message;
    }

    /**
     * @brief Process incoming connections and data
     */
    void tick() {
        switch (currentState) {
            case State::IDLE:
                currentState = State::LISTENING_STATE;
                break;

            case State::LISTENING_STATE:
                if (!isListening) {
                    lastError = Status::NOT_LISTENING;
                    currentState = State::IDLE;
                    return;
                }
                
                // Accept new client if none connected
                if (!currentClient.connected()) {
                    currentClient = server.available();
                    if (currentClient.connected()) {
                        lastClientActivity = millis();
                        log("New client connected");
                        currentState = State::CLIENT_CONNECTED_STATE;
                    }
                } else {
                    currentState = State::PROCESSING_STATE;
                }
                break;

            case State::CLIENT_CONNECTED_STATE:
                // Check for client timeout
                if ((millis() - lastClientActivity) > KEEPALIVE_TIMEOUT) {
                    log("disconnect3");
                    disconnectClient();
                    log("Client timeout");
                    currentState = State::IDLE;
                    lastError = Status::TIMEOUT;
                } else if (currentClient.available()) {
                    currentState = State::PROCESSING_STATE;
                }
                break;

            case State::PROCESSING_STATE:
                processClientData();
                break;
        }
    }

private:
    enum class State { 
        IDLE, 
        LISTENING_STATE, 
        CLIENT_CONNECTED_STATE, 
        PROCESSING_STATE 
    };

    WiFiServer server;
    WiFiClient currentClient;
    int port;
    std::array<char, MESSAGE_LENGTH + 1> buffer;  // +1 for null terminator
    uint8_t receivedChars;
    bool isListening;
    Status lastError;
    unsigned long timeout;
    unsigned long lastClientActivity;
    std::queue<String> messageQueue;
    State currentState;

    /**
     * @brief Process data from the connected client
     */
    void processClientData() {
        if (!currentClient.connected()) {
            currentState = State::LISTENING_STATE;
            return;
        }

        unsigned long startTime = millis();
        bool dataProcessed = false;

        while (currentClient.available() && (millis() - startTime < timeout)) {
            dataProcessed = true;
            lastClientActivity = millis();
            char c = currentClient.read();
            
            // If we're at the start, wait for '%'
            if (receivedChars == 0) {
                if (c == '%') {
                    buffer[0] = c;
                    receivedChars = 1;
                }
                continue;
            }
            
            // We're receiving the 10 hex characters
            if (receivedChars < MESSAGE_LENGTH) {
                if (!isHexChar(c)) {
                    resetState();
                    lastError = Status::INVALID_FORMAT;
                    break;
                }
                buffer[receivedChars++] = c;
                
                // Check if we've received all characters
                if (receivedChars == MESSAGE_LENGTH) {
                    buffer[MESSAGE_LENGTH] = '\0';
                    messageQueue.push(String(buffer.data()));
                    // log("Message received: " + String(buffer.data()));
                    handleMoveServos(buffer.data());
                    resetState();
                    lastError = Status::OK;
                    break;
                }
            }
        }

        // Always transition back to CLIENT_CONNECTED_STATE regardless of data being processed
        // This removes the timeout disconnection when no data arrives
        currentState = State::CLIENT_CONNECTED_STATE;
    }

    /**
     * @brief Check if character is a valid hexadecimal digit
     * @param c Character to check
     * @return true if character is hex digit
     */
    bool isHexChar(char c) const {
        return (c >= '0' && c <= '9') || 
               (c >= 'A' && c <= 'F') || 
               (c >= 'a' && c <= 'f');
    }

    /**
     * @brief Reset the processing state
     */
    void resetState() {
        receivedChars = 0;
        buffer.fill('\0');
    }

    /**
     * @brief Clear the message queue
     */
    void clearMessageQueue() {
        while (!messageQueue.empty()) {
            messageQueue.pop();
        }
    }

    /**
     * @brief Disconnect the current client
     */
    void disconnectClient() {
        if (currentClient.connected()) {
            currentClient.stop();
            log("Client disconnected");
        }
        resetState();
    }

    /**
     * @brief Log a message to Serial
     * @param message Message to log
     */
    void log(const String& message) {
        Serial.println("RemoteChannel: " + message);
    }
};

#endif // REMOTECHANNEL_HPP