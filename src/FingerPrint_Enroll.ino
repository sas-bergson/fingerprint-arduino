#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>

/**
 * @file FingerPrint_Enroll.ino
 * @brief Command-driven fingerprint firmware for host-controlled workflows.
 *
 * Protocol (line-based, newline terminated):
 * @code
 * CMD|<cmd_code>|<arg>                        Host command
 * EVT|<cmd_code>|<event_code>|<message>        Firmware async event
 * RSP|<cmd_code>|<status_code>|<payload>       Firmware terminal response
 * TPL|<index>|<hex bytes>                     Template packet
 * @endcode
 *
 * Command codes:
 * - 10: Read board status
 * - 20: Read fingerprint module status
 * - 30: Read/store fingerprint and export template bytes
 */

SoftwareSerial mySerial(2, 3);                                 ///< SoftwareSerial link to fingerprint sensor (RX=2, TX=3)
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial); ///< Adafruit fingerprint sensor driver

static const int CMD_BOARD_STATUS = 10;      ///< Command: read board status
static const int CMD_FP_STATUS = 20;         ///< Command: read fingerprint module status
static const int CMD_READ_STORE_EXPORT = 30; ///< Command: read/store fingerprint and export template
static const int CMD_CAPTURE_IMAGE = 40;     ///< Command: capture fingerprint image

static const int STS_OK = 0;                     ///< Status: success
static const int STS_ERR = 1;                    ///< Status: generic error
static const int STS_INVALID_CMD = 2;            ///< Status: unrecognised command code
static const int STS_SENSOR_NOT_FOUND = 10;      ///< Status: sensor offline or not found
static const int STS_TIMEOUT = 11;               ///< Status: operation timed out
static const int STS_ENROLL_FAIL = 12;           ///< Status: enroll or read flow failed
static const uint16_t SENSOR_MAX_WIRE_LEN = 300; ///< Maximum supported sensor wire packet length (bytes)
static const int IMAGE_WIDTH = 256;              ///< Fingerprint image width in pixels
static const int IMAGE_HEIGHT = 288;             ///< Fingerprint image height in pixels

static bool sensorReady = false;        ///< True when the fingerprint sensor is online
static uint32_t sensorUartBaud = 57600; ///< Active sensor UART baud rate

/**
 * @brief Send one protocol event line.
 */
void sendEvent(int cmd, int code, const String &message)
{
    Serial.print("EVT|");
    Serial.print(cmd);
    Serial.print('|');
    Serial.print(code);
    Serial.print('|');
    Serial.println(message);
}

/**
 * @brief Send one protocol response line.
 */
void sendResponse(int cmd, int status, const String &payload)
{
    Serial.print("RSP|");
    Serial.print(cmd);
    Serial.print('|');
    Serial.print(status);
    Serial.print('|');
    Serial.println(payload);
}

/**
 * @brief Convert one byte to uppercase 2-char hex string.
 */
String byteToHex(uint8_t value)
{
    char buffer[3];
    snprintf(buffer, sizeof(buffer), "%02X", value);
    return String(buffer);
}

/**
 * @brief Send board status including firmware metadata.
 */
void handleBoardStatus()
{
    String payload = "fw=fingerprint-host-proto-v1;serial=230400;sensor_uart=" + String(sensorUartBaud) + ";board=arduino-uno";
    sendResponse(CMD_BOARD_STATUS, STS_OK, payload);
}

/**
 * @brief Send fingerprint module status and key module metadata.
 */
void handleFingerprintStatus()
{
    if (!sensorReady)
    {
        sendResponse(CMD_FP_STATUS, STS_SENSOR_NOT_FOUND, "sensor=offline");
        return;
    }

    finger.getParameters();
    finger.getTemplateCount();

    String payload = "sensor=online";
    payload += ";capacity=" + String(finger.capacity);
    payload += ";security=" + String(finger.security_level);
    payload += ";packet_len=" + String(finger.packet_len);
    payload += ";baud=" + String(finger.baud_rate);
    payload += ";stored=" + String(finger.templateCount);

    sendResponse(CMD_FP_STATUS, STS_OK, payload);
}

/**
 * @brief Wait until a finger is present and image acquisition succeeds.
 */
/**
 * @brief Wait for fingerprint image with intermediate feedback events.
 *
 * Provides deterministic finger detection with clear user feedback:
 * 1. Sends "waiting" events every 500ms so user knows system is listening
 * 2. Detects when finger is first placed (within 3 seconds ideally)
 * 3. Once detected, waits for minimum hold time (800ms) for good quality
 * 4. Terminates on success, error, or 10-second timeout
 *
 * This replaces the silent polling approach with active feedback, ensuring
 * users know exactly when to place/hold the finger and when capture is complete.
 *
 * @param cmdCode Command code for event reporting
 * @return FINGERPRINT_OK on success, error code otherwise
 */
int waitForFingerImageWithFeedback(int cmdCode)
{
    const unsigned long totalTimeoutMs = 10000UL;   // 10 second total timeout
    const unsigned long minHoldTimeMs = 800UL;      // Finger must be present for 800ms
    const unsigned long feedbackIntervalMs = 500UL; // Send feedback every 500ms

    unsigned long startTime = millis();
    unsigned long lastFeedbackTime = 0;
    unsigned long fingerDetectedTime = 0; // When finger first detected (0 = not yet)

    while (true)
    {
        unsigned long now = millis();
        unsigned long elapsedMs = now - startTime;

        // Check total timeout
        if (elapsedMs > totalTimeoutMs)
        {
            sendEvent(cmdCode, 901, "timeout: no finger detected within 10 seconds");
            return STS_TIMEOUT;
        }

        // Send feedback every 500ms so user knows system is waiting
        if ((now - lastFeedbackTime) >= feedbackIntervalMs)
        {
            if (fingerDetectedTime == 0)
            {
                // Still waiting for finger
                unsigned long waitedSeconds = elapsedMs / 1000;
                String msg = "waiting for finger (";
                msg += String(waitedSeconds);
                msg += "s)";
                sendEvent(cmdCode, 902, msg);
            }
            else
            {
                // Finger detected, show hold time
                unsigned long holdTimeMs = now - fingerDetectedTime;
                unsigned long holdTimeNeededMs = (minHoldTimeMs > holdTimeMs) ? (minHoldTimeMs - holdTimeMs) : 0;
                if (holdTimeNeededMs > 0)
                {
                    String msg = "finger detected, hold for ";
                    msg += String(holdTimeNeededMs);
                    msg += "ms";
                    sendEvent(cmdCode, 903, msg);
                }
            }
            lastFeedbackTime = now;
        }

        // Try to capture image
        int p = finger.getImage();

        if (p == FINGERPRINT_OK)
        {
            // Finger detected and image captured successfully!
            if (fingerDetectedTime == 0)
            {
                // First detection
                fingerDetectedTime = millis();
            }

            // Check if finger has been held long enough
            unsigned long holdTimeMs = millis() - fingerDetectedTime;
            if (holdTimeMs >= minHoldTimeMs)
            {
                // Good quality capture achieved
                sendEvent(cmdCode, 904, "fingerprint captured successfully");
                return FINGERPRINT_OK;
            }

            // Finger detected but not held long enough yet, continue polling
            delay(50);
            continue;
        }

        // If image collection fails, reset detection timer
        if (p == FINGERPRINT_IMAGEFAIL)
        {
            if (fingerDetectedTime != 0)
            {
                sendEvent(cmdCode, 905, "image quality poor, retrying");
                fingerDetectedTime = 0; // Reset hold time counter
            }
            delay(50);
            continue;
        }

        // Communication error
        if (p == FINGERPRINT_PACKETRECIEVEERR)
        {
            sendEvent(cmdCode, 906, "communication error with sensor");
            return p;
        }

        // No finger currently detected
        if (p == FINGERPRINT_NOFINGER)
        {
            if (fingerDetectedTime != 0)
            {
                // Finger was being held but now removed before hold time complete
                sendEvent(cmdCode, 907, "finger removed too early, place again");
                fingerDetectedTime = 0; // Reset
            }
            delay(50);
            continue;
        }

        // Other error
        delay(50);
    }
}

/**
 * @brief Backward compatible wrapper using improved algorithm.
 *
 * For compatibility, this calls the improved version internally.
 */
int waitForFingerImage(int cmdCode)
{
    return waitForFingerImageWithFeedback(cmdCode);
}

/**
 * @brief Map capture result code to short text label.
 */
String captureErrorLabel(int code)
{
    if (code == STS_TIMEOUT)
    {
        return "timeout";
    }
    if (code == FINGERPRINT_PACKETRECIEVEERR)
    {
        return "comm_error";
    }
    if (code == FINGERPRINT_IMAGEFAIL)
    {
        return "image_fail";
    }
    if (code == FINGERPRINT_NOFINGER)
    {
        return "no_finger";
    }
    return "capture_error";
}

/**
 * @brief Wait until no finger is present with progress feedback.
 *
 * Sends periodic feedback so user knows system is monitoring finger removal.
 * Useful after capture to ensure user removes finger before next operation.
 *
 * @param timeoutMs Maximum time to wait for finger removal
 * @return true if finger was successfully removed, false on timeout
 */
bool waitUntilNoFingerWithFeedback(int timeoutMs)
{
    unsigned long startTime = millis();
    unsigned long lastFeedbackTime = 0;
    const unsigned long feedbackIntervalMs = 500UL;

    while (true)
    {
        unsigned long now = millis();
        unsigned long elapsedMs = now - startTime;

        // Check timeout
        if (elapsedMs > static_cast<unsigned long>(timeoutMs))
        {
            sendEvent(CMD_READ_STORE_EXPORT, 910, "timeout: finger not removed");
            return false;
        }

        // Send feedback every 500ms
        if ((now - lastFeedbackTime) >= feedbackIntervalMs)
        {
            String msg = "remove finger (";
            msg += String((timeoutMs - elapsedMs) / 1000);
            msg += "s timeout)";
            sendEvent(CMD_READ_STORE_EXPORT, 911, msg);
            lastFeedbackTime = now;
        }

        // Check if finger is gone
        int p = finger.getImage();
        if (p == FINGERPRINT_NOFINGER)
        {
            sendEvent(CMD_READ_STORE_EXPORT, 912, "finger removed");
            return true;
        }

        delay(80);
    }
}

/**
 * @brief Backward compatible wrapper using improved algorithm.
 */
bool waitUntilNoFinger(int timeoutMs)
{
    return waitUntilNoFingerWithFeedback(timeoutMs);
}

/**
 * @brief Read one byte from the sensor UART with timeout.
 */
bool readSensorByte(uint8_t &out, uint16_t timeoutMs)
{
    unsigned long start = millis();
    while ((millis() - start) < timeoutMs)
    {
        if (mySerial.available() > 0)
        {
            out = static_cast<uint8_t>(mySerial.read());
            return true;
        }
        delay(1);
    }
    return false;
}

/**
 * @brief Try sensor communication at a specific UART baud.
 */
bool trySensorBaud(uint32_t baud)
{
    mySerial.begin(baud);
    finger.begin(baud);
    delay(80);
    if (finger.verifyPassword())
    {
        sensorUartBaud = baud;
        return true;
    }
    return false;
}

/**
 * @brief Initialize sensor UART link, preferring a stable baud for SoftwareSerial.
 */
bool initializeSensorLink()
{
    // First try the common default.
    if (trySensorBaud(57600))
    {
        sensorUartBaud = 57600;
        return true;
    }

    // Fallbacks in case the module is already configured differently.
    if (trySensorBaud(19200))
    {
        return true;
    }
    if (trySensorBaud(9600))
    {
        return true;
    }

    return false;
}

/**
 * @brief Read one full structured packet from sensor UART.
 * @param type           Output: packet type byte.
 * @param payload        Output: payload bytes (checksum excluded).
 * @param payloadLen     Output: number of payload bytes written.
 * @param payloadMax     Maximum capacity of the payload buffer.
 * @param stageError     Output: stage index where parsing failed (0 = success).
 * @param startTimeoutMs Timeout for the initial 0xEF01 sync (default 1500 ms).
 * @return true on success, false on timeout or malformed packet.
 */
bool readSensorStructuredPacket(uint8_t &type, uint8_t *payload, uint16_t &payloadLen, uint16_t payloadMax, int &stageError, uint16_t startTimeoutMs = 1500)
{
    payloadLen = 0;
    stageError = 0;

    uint8_t b = 0;
    const unsigned long syncStartMs = millis();
    // Sync to start code 0xEF01.
    while (true)
    {
        if ((millis() - syncStartMs) > static_cast<unsigned long>(startTimeoutMs))
        {
            stageError = 1;
            return false;
        }

        if (!readSensorByte(b, startTimeoutMs))
        {
            stageError = 1;
            return false;
        }
        if (b == 0xEF)
        {
            uint8_t b2 = 0;
            if (!readSensorByte(b2, 200))
            {
                stageError = 2;
                return false;
            }
            if (b2 == 0x01)
            {
                break;
            }
        }
    }

    // Address (4 bytes)
    for (int i = 0; i < 4; ++i)
    {
        if (!readSensorByte(b, 200))
        {
            stageError = 3;
            return false;
        }
    }

    // Packet type
    if (!readSensorByte(type, 200))
    {
        stageError = 4;
        return false;
    }

    // Wire length: payload + checksum(2)
    uint8_t lenHi = 0;
    uint8_t lenLo = 0;
    if (!readSensorByte(lenHi, 200) || !readSensorByte(lenLo, 200))
    {
        stageError = 5;
        return false;
    }
    const uint16_t wireLen = (static_cast<uint16_t>(lenHi) << 8) | lenLo;
    if (wireLen < 2 || wireLen > SENSOR_MAX_WIRE_LEN)
    {
        stageError = 6;
        return false;
    }

    // Read body bytes: payload + checksum(2)
    uint8_t body[SENSOR_MAX_WIRE_LEN];
    for (uint16_t i = 0; i < wireLen; ++i)
    {
        if (!readSensorByte(b, 3000))
        {
            stageError = 7;
            return false;
        }
        body[i] = b;
    }

    // Strip checksum from payload.
    payloadLen = wireLen - 2;
    if (payloadLen > payloadMax)
    {
        stageError = 8;
        return false;
    }

    for (uint16_t i = 0; i < payloadLen; ++i)
    {
        payload[i] = body[i];
    }

    return true;
}

/**
 * @brief Download model from sensor and stream bytes as TPL lines.
 * @return Number of bytes sent, or -1 on error.
 */
int streamTemplateBytes(int fingerId, int &errorCode)
{
    errorCode = 0;

    while (mySerial.available() > 0)
    {
        mySerial.read();
    }

    // Ensure the just-stored model is loaded in CharBuffer1 before upload.
    uint8_t p = finger.loadModel(static_cast<uint16_t>(fingerId));
    if (p != FINGERPRINT_OK)
    {
        errorCode = 90 + p;
        return -1;
    }

    p = FINGERPRINT_PACKETRECIEVEERR;
    for (int attempt = 0; attempt < 3; ++attempt)
    {
        p = finger.getModel();
        if (p == FINGERPRINT_OK)
        {
            break;
        }
        delay(120);
    }
    if (p != FINGERPRINT_OK)
    {
        errorCode = 100 + p;
        return -1;
    }

    delay(20);

    int totalBytes = 0;

    int badFrames = 0;
    int packetIdx = 0;
    while (packetIdx < 20)
    {
        uint8_t packetType = 0;
        uint8_t payload[256];
        uint16_t payloadLen = 0;
        int stageError = 0;

        if (!readSensorStructuredPacket(packetType, payload, payloadLen, sizeof(payload), stageError))
        {
            ++badFrames;
            if (badFrames <= 6)
            {
                continue;
            }
            errorCode = 400 + stageError;
            return -1;
        }
        badFrames = 0;

        if (packetType != FINGERPRINT_DATAPACKET && packetType != FINGERPRINT_ENDDATAPACKET)
        {
            errorCode = 420 + packetType;
            return -1;
        }

        Serial.print("TPL|");
        Serial.print(packetIdx);
        Serial.print('|');
        for (uint16_t i = 0; i < payloadLen; ++i)
        {
            if (i > 0)
            {
                Serial.write(' ');
            }
            if (payload[i] < 16)
            {
                Serial.write('0');
            }
            Serial.print(payload[i], HEX);
            ++totalBytes;
        }
        Serial.println();
        ++packetIdx;

        if (packetType == FINGERPRINT_ENDDATAPACKET)
        {
            break;
        }
    }

    if (totalBytes <= 0)
    {
        errorCode = 320;
        return -1;
    }

    return totalBytes;
}

/**
 * @brief Send upload-image command packet directly to sensor.
 * @return Sensor ACK code.
 */
uint8_t requestImageUpload()
{
    uint8_t data[] = {0x0A};
    Adafruit_Fingerprint_Packet cmd(FINGERPRINT_COMMANDPACKET, sizeof(data), data);
    finger.writeStructuredPacket(cmd);

    Adafruit_Fingerprint_Packet ack(FINGERPRINT_ACKPACKET, 0, data);
    const uint8_t rsp = finger.getStructuredPacket(&ack, 2000);
    if (rsp != FINGERPRINT_OK)
    {
        return rsp;
    }
    if (ack.type != FINGERPRINT_ACKPACKET)
    {
        return FINGERPRINT_BADPACKET;
    }
    return ack.data[0];
}

/**
 * @brief Reconfigure sensor UART baud on demand.
 */
bool setSensorBaudRuntime(uint32_t targetBaud)
{
    uint8_t baudCode = 0;
    if (targetBaud == 9600)
    {
        baudCode = FINGERPRINT_BAUDRATE_9600;
    }
    else if (targetBaud == 19200)
    {
        baudCode = FINGERPRINT_BAUDRATE_19200;
    }
    else if (targetBaud == 57600)
    {
        baudCode = FINGERPRINT_BAUDRATE_57600;
    }
    else
    {
        return false;
    }

    if (finger.setBaudRate(baudCode) != FINGERPRINT_OK)
    {
        return false;
    }
    delay(120);
    return trySensorBaud(targetBaud);
}

/**
 * @brief Stream image data packets as IMG lines.
 * @return Number of bytes sent, or -1 on error.
 */
long streamImageBytes(int &errorCode)
{
    errorCode = 0;

    while (mySerial.available() > 0)
    {
        mySerial.read();
    }

    uint8_t p = requestImageUpload();
    if (p != FINGERPRINT_OK)
    {
        errorCode = 500 + p;
        return -1;
    }

    uint32_t totalBytes = 0;
    int packetIdx = 0;
    int badFrames = 0;
    bool uploadRetried = false;
    bool sawEndPacket = false;
    bool streamBaudFallbackTried = false;
    // UART upload image mode provides 4-bit grayscale packed as 2 pixels per byte.
    // Validate row count from received bytes so both partial-height and full-height
    // module streams can be accepted without mode-specific constants.
    const uint32_t packedRowBytes = static_cast<uint32_t>(IMAGE_WIDTH) / 2U;

    // Some modules stream small payload chunks (e.g. 32 bytes), which can
    // require over 2000 packets for a full 256x288 image.
    static const int IMAGE_MAX_PACKETS = 2600;

    int initialSyncRetries = 0;

    while (packetIdx < IMAGE_MAX_PACKETS)
    {
        uint8_t packetType = 0;
        uint8_t payload[256];
        uint16_t payloadLen = 0;
        int stageError = 0;

        const uint16_t startTimeout = (packetIdx == 0) ? 6000 : 1500;

        if (!readSensorStructuredPacket(packetType, payload, payloadLen, sizeof(payload), stageError, startTimeout))
        {
            if (packetIdx == 0 && stageError == 1 && !uploadRetried)
            {
                uploadRetried = true;
                while (mySerial.available() > 0)
                {
                    mySerial.read();
                }
                const uint8_t retryP = requestImageUpload();
                if (retryP != FINGERPRINT_OK)
                {
                    errorCode = 700 + retryP;
                    return -1;
                }
                delay(50);
                continue;
            }

            if (packetIdx == 0 && stageError == 1)
            {
                ++initialSyncRetries;
                if (initialSyncRetries >= 3)
                {
                    errorCode = 601;
                    return -1;
                }
                continue;
            }

            ++badFrames;
            if (badFrames > 12 && !streamBaudFallbackTried && sensorUartBaud == 57600)
            {
                streamBaudFallbackTried = true;
                if (setSensorBaudRuntime(19200))
                {
                    while (mySerial.available() > 0)
                    {
                        mySerial.read();
                    }
                    const uint8_t retryP = requestImageUpload();
                    if (retryP == FINGERPRINT_OK)
                    {
                        badFrames = 0;
                        packetIdx = 0;
                        totalBytes = 0;
                        sawEndPacket = false;
                        continue;
                    }
                }
            }
            if (badFrames <= 40)
            {
                continue;
            }
            errorCode = 600 + stageError;
            return -1;
        }
        badFrames = 0;

        if (packetType != FINGERPRINT_DATAPACKET && packetType != FINGERPRINT_ENDDATAPACKET)
        {
            errorCode = 620 + packetType;
            return -1;
        }

        // Stream packet bytes directly to avoid large temporary String allocations on AVR.
        for (uint16_t i = 0; i < payloadLen; ++i)
        {
            ++totalBytes;
        }

        Serial.print("IMB|");
        Serial.print(packetIdx);
        Serial.print('|');
        Serial.println(payloadLen);
        Serial.write(payload, payloadLen);
        Serial.write('\n');
        ++packetIdx;

        if (packetType == FINGERPRINT_ENDDATAPACKET)
        {
            if (packedRowBytes > 0)
            {
                const uint32_t receivedRows = totalBytes / packedRowBytes;
                if (receivedRows >= 200U && receivedRows <= static_cast<uint32_t>(IMAGE_HEIGHT))
                {
                    sawEndPacket = true;
                    break;
                }
            }

            // Treat unexpected early end packets as bad frames and continue.
            ++badFrames;
            if (badFrames > 40)
            {
                errorCode = 632;
                return -1;
            }
        }
    }

    if (totalBytes <= 0)
    {
        errorCode = 630;
        return -1;
    }

    if (!sawEndPacket)
    {
        errorCode = 631;
        return -1;
    }

    if (packedRowBytes == 0)
    {
        errorCode = 634;
        return -1;
    }

    const uint32_t receivedRows = totalBytes / packedRowBytes;
    if (receivedRows < 200U || receivedRows > static_cast<uint32_t>(IMAGE_HEIGHT))
    {
        errorCode = 633;
        return -1;
    }

    return static_cast<long>(totalBytes);
}

/**
 * @brief Capture image and export raw grayscale bytes to host.
 */
void handleCaptureImage()
{
    if (!sensorReady)
    {
        sendResponse(CMD_CAPTURE_IMAGE, STS_SENSOR_NOT_FOUND, "sensor=offline");
        return;
    }

    sendEvent(CMD_CAPTURE_IMAGE, 100, "place finger");
    int p = waitForFingerImage(CMD_CAPTURE_IMAGE);
    if (p != FINGERPRINT_OK)
    {
        const int status = (p == STS_TIMEOUT) ? STS_TIMEOUT : STS_ENROLL_FAIL;
        String payload = "capture_image_failed:" + captureErrorLabel(p);
        payload += ";code=" + String(p);
        sendResponse(CMD_CAPTURE_IMAGE, status, payload);
        return;
    }

    sendEvent(CMD_CAPTURE_IMAGE, 200, "streaming image");
    int streamErrorCode = 0;
    const long byteCount = streamImageBytes(streamErrorCode);
    if (byteCount < 0)
    {
        String payload = "image_stream_failed";
        payload += ";code=" + String(streamErrorCode);
        sendResponse(CMD_CAPTURE_IMAGE, STS_ERR, payload);
        return;
    }

    uint16_t reportedHeight = IMAGE_HEIGHT;
    if (byteCount > 0)
    {
        const uint32_t pixelsTransferred = static_cast<uint32_t>(byteCount) * 2U;
        const uint32_t computedHeight = pixelsTransferred / static_cast<uint32_t>(IMAGE_WIDTH);
        if (computedHeight > 0 && computedHeight <= static_cast<uint32_t>(IMAGE_HEIGHT))
        {
            reportedHeight = static_cast<uint16_t>(computedHeight);
        }
    }

    String payload = "bytes=" + String(byteCount);
    payload += ";width=" + String(IMAGE_WIDTH);
    payload += ";height=" + String(reportedHeight);
    payload += ";full_height=" + String(IMAGE_HEIGHT);
    payload += ";format=gray4_packed";
    sendResponse(CMD_CAPTURE_IMAGE, STS_OK, payload);
}

/**
 * @brief Execute read/store/export fingerprint flow.
 */
void handleReadStoreExport(int fingerId)
{
    if (!sensorReady)
    {
        sendResponse(CMD_READ_STORE_EXPORT, STS_SENSOR_NOT_FOUND, "sensor=offline");
        return;
    }

    if (fingerId < 1 || fingerId > 127)
    {
        sendResponse(CMD_READ_STORE_EXPORT, STS_ERR, "invalid_id");
        return;
    }

    sendEvent(CMD_READ_STORE_EXPORT, 100, "place finger");
    int p = waitForFingerImage(CMD_READ_STORE_EXPORT);
    if (p != FINGERPRINT_OK)
    {
        const int status = (p == STS_TIMEOUT) ? STS_TIMEOUT : STS_ENROLL_FAIL;
        String payload = "capture_1_failed:" + captureErrorLabel(p);
        payload += ";code=" + String(p);
        sendResponse(CMD_READ_STORE_EXPORT, status, payload);
        return;
    }

    p = finger.image2Tz(1);
    if (p != FINGERPRINT_OK)
    {
        sendResponse(CMD_READ_STORE_EXPORT, STS_ENROLL_FAIL, "image2tz_1_failed");
        return;
    }

    sendEvent(CMD_READ_STORE_EXPORT, 110, "remove finger");
    if (!waitUntilNoFinger(8000))
    {
        sendResponse(CMD_READ_STORE_EXPORT, STS_TIMEOUT, "remove_timeout");
        return;
    }

    sendEvent(CMD_READ_STORE_EXPORT, 120, "place same finger again");
    p = waitForFingerImage(CMD_READ_STORE_EXPORT);
    if (p != FINGERPRINT_OK)
    {
        const int status = (p == STS_TIMEOUT) ? STS_TIMEOUT : STS_ENROLL_FAIL;
        String payload = "capture_2_failed:" + captureErrorLabel(p);
        payload += ";code=" + String(p);
        sendResponse(CMD_READ_STORE_EXPORT, status, payload);
        return;
    }

    p = finger.image2Tz(2);
    if (p != FINGERPRINT_OK)
    {
        sendResponse(CMD_READ_STORE_EXPORT, STS_ENROLL_FAIL, "image2tz_2_failed");
        return;
    }

    p = finger.createModel();
    if (p != FINGERPRINT_OK)
    {
        sendResponse(CMD_READ_STORE_EXPORT, STS_ENROLL_FAIL, "create_model_failed");
        return;
    }

    p = finger.storeModel(fingerId);
    if (p != FINGERPRINT_OK)
    {
        sendResponse(CMD_READ_STORE_EXPORT, STS_ENROLL_FAIL, "store_model_failed");
        return;
    }

    sendEvent(CMD_READ_STORE_EXPORT, 200, "streaming template");
    int streamErrorCode = 0;
    const int byteCount = streamTemplateBytes(fingerId, streamErrorCode);
    if (byteCount < 0)
    {
        String payload = "template_stream_failed";
        payload += ";code=" + String(streamErrorCode);
        sendResponse(CMD_READ_STORE_EXPORT, STS_ERR, payload);
        return;
    }

    String payload = "id=" + String(fingerId);
    payload += ";bytes=" + String(byteCount);
    payload += ";stored=1";
    sendResponse(CMD_READ_STORE_EXPORT, STS_OK, payload);
}

/**
 * @brief Parse and dispatch one host command line.
 */
void processCommandLine(char *line)
{
    char *token = strtok(line, "|");
    if (token == NULL || strcmp(token, "CMD") != 0)
    {
        sendResponse(0, STS_INVALID_CMD, "invalid_prefix");
        return;
    }

    char *cmdToken = strtok(NULL, "|");
    char *argToken = strtok(NULL, "|");
    if (cmdToken == NULL)
    {
        sendResponse(0, STS_INVALID_CMD, "missing_cmd");
        return;
    }

    const int cmd = atoi(cmdToken);
    const int arg = (argToken == NULL) ? 0 : atoi(argToken);

    switch (cmd)
    {
    case CMD_BOARD_STATUS:
        handleBoardStatus();
        break;
    case CMD_FP_STATUS:
        handleFingerprintStatus();
        break;
    case CMD_READ_STORE_EXPORT:
        handleReadStoreExport(arg);
        break;
    case CMD_CAPTURE_IMAGE:
        handleCaptureImage();
        break;
    default:
        sendResponse(cmd, STS_INVALID_CMD, "unknown_command");
        break;
    }
}

/**
 * @brief Arduino setup: initialise host serial and sensor UART link.
 *
 * Starts the host UART at 230400 baud, attempts sensor baud negotiation
 * (57600 → 19200 → 9600), and sends an initial firmware-ready event.
 */
void setup()
{
    Serial.begin(230400);
    while (!Serial)
    {
        delay(1);
    }

    sensorReady = initializeSensorLink();

    if (sensorReady)
    {
        sendEvent(0, 1, "firmware_ready;sensor=online");
    }
    else
    {
        sendEvent(0, 2, "firmware_ready;sensor=offline");
    }
}

/**
 * @brief Arduino main loop: read and dispatch one host command per iteration.
 *
 * Blocks on `Serial.readBytesUntil('\n')` and forwards the resulting
 * null-terminated line to processCommandLine().
 */
void loop()
{
    static char commandBuffer[96];

    if (Serial.available() > 0)
    {
        size_t readCount = Serial.readBytesUntil('\n', commandBuffer, sizeof(commandBuffer) - 1);
        commandBuffer[readCount] = '\0';

        if (readCount == 0)
        {
            return;
        }

        processCommandLine(commandBuffer);
    }
}
