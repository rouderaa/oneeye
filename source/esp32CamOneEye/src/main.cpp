#include <Arduino.h>
#include <WiFi.h>
#include <esp_system.h>
#include "speedservo.hpp"
#include "remotechannel.hpp"

const char *ssid = "ESP32_TelnetAP";
const char *password = "12345678";

WiFiServer telnetServer(23);
WiFiClient telnetClient;
RemoteChannel remoteChannel(24);
#define MAX_USER_INPUT 256
#define LED 33
#define LED_ON 0
#define LED_OFF 1
#define NRSERVOS 4
// Servo numbers
#define EH 0
#define EV 1
#define LU 2
#define LD 3

static int selectedServo = 0;
static SpeedServo *servos[NRSERVOS];

extern "C"
{
    int yywrap();
    int yylex();
    char *set_input_string(const char *str);

    void handleLedOn()
    {
        Serial.println("LED ON");
        digitalWrite(LED, LED_ON);
    }
    void handleLedOff()
    {
        Serial.println("LED OFF");
        digitalWrite(LED, LED_OFF);
    }
    void handleHelp()
    {
        telnetClient.println("led [on|off]");
        telnetClient.println("select [eh|ev|lu|ld]");
        telnetClient.println("set <servoValue>");
        telnetClient.println("move <servoValue>");
        telnetClient.println("help");
        telnetClient.println("reset");
        telnetClient.println("sweep");
        telnetClient.println("center");
        // telnetClient.println("%XXXXXXXX");
    }
    void handleSelect(int servoKind)
    {
        telnetClient.print("select: ");
        telnetClient.println(servoKind);
        selectedServo = servoKind;
    }
    void handleSet(int servoValue)
    {
        Serial.print("set ");
        Serial.println(servoValue);

        if ((servoValue >= 0) && (servoValue <= 255))
        {
            if ((selectedServo >= 0) && (selectedServo < NRSERVOS))
            {
                telnetClient.printf(
                    "set[%d]=%d (%s) pin=%d\n",
                    selectedServo, servoValue,
                    servos[selectedServo]->getName(),
                    servos[selectedServo]->getPin());
                servos[selectedServo]->set(servoValue);
            }
        }
        else
        {
            telnetClient.println("Error: servo value out of range.");
        }
    }
    void handleMove(int servoValue)
    {
        Serial.print("move ");
        Serial.println(servoValue);

        if ((servoValue >= 0) && (servoValue <= 255))
        {
            if ((selectedServo >= 0) && (selectedServo < NRSERVOS))
            {
                telnetClient.printf("move[%d]=%d\n", selectedServo, servoValue);
                int value = servos[selectedServo]->move(servoValue);
                telnetClient.printf("value=%d\n", value);
            }
        }
        else
        {
            telnetClient.println("Error: servo value out of range.");
        }
    }
    /**
     * Converts two hexadecimal characters to their integer value
     * @param hexChars Pointer to two hex characters (0-9, A-F, a-f)
     * @return Integer value of the hexadecimal pair, or -1 if invalid characters
     */
    int hexPairToInt(const char *hexChars)
    {
        if (hexChars == NULL)
        {
            return -1;
        }

        int result = 0;

        // Process both characters using the same logic
        for (int i = 0; i < 2; i++)
        {
            char c = hexChars[i];
            int val;

            if (c >= '0' && c <= '9')
            {
                val = c - '0';
            }
            else if (c >= 'A' && c <= 'F')
            {
                val = c - 'A' + 10;
            }
            else if (c >= 'a' && c <= 'f')
            {
                val = c - 'a' + 10;
            }
            else
            {
                return -1; // Invalid character
            }

            // For first character, shift left by 4, for second just add
            result = (i == 0) ? (val << 4) : (result | val);
        }

        return result;
    }
    void handleMoveServos(char *message)
    {
        // Serial.printf("handleMoveServo: %s\n", message);
        servos[EH]->move(hexPairToInt(&message[1]));
        servos[EV]->move(hexPairToInt(&message[3]));
        int v = hexPairToInt(&message[7]);
        if (v < 0)
            v = 0;
        if (v > 128)
            v = 128;
        servos[LU]->move((int) ((v/128.0)*155.0));
        servos[LD]->move((int) ((v/128.0)*255.0));
    }
    void handleReset()
    {
        esp_restart();
    }
    void handleSweep()
    {
        // Sweep servo a couple of times
        for (int sweepCount = 0; sweepCount < 1; sweepCount++)
        {
            for (int servoValue = 0; servoValue < 180; servoValue = servoValue + 10)
            {
                // telnetClient.printf("servo[%d]=%d\n", selectedServo, servoValue);
                if (!servos[selectedServo]->set(servoValue))
                    telnetClient.printf("sweep value [%d] failed\n", servoValue);
                delay(100); // ms
            }
            for (int servoValue = 179; servoValue >= 0; servoValue = servoValue - 10)
            {
                // telnetClient.printf("servo[%d]=%d\n", selectedServo, servoValue);
                if (!servos[selectedServo]->set(servoValue))
                    telnetClient.printf("sweep value [%d] failed\n", servoValue);
                delay(19); // ms
            }
        }
    }
    void handleCenter()
    {
        for (int servoNr = 0; servoNr < NRSERVOS; servoNr++)
        {
            servos[servoNr]->set(128);
            delay(200); // give power supply a break
        }
    }
    void handlePose(char *string)
    {
        telnetClient.printf("Got hex value: %s\n", string);
        free(string);
    }
}
void attachServo(int servoNum, int pin)
{
    if (servos[servoNum]->attach(pin) == 0)
    {
        Serial.printf(
            "Error attaching servo %d to pin %d\n",
            servoNum, pin);
    }
}

void setupServos()
{
    // Allocate timers for all servos
    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    ESP32PWM::allocateTimer(3);

    // Create servo instances
    servos[0] = new SpeedServo("EH", 110, 150, 12);
    servos[1] = new SpeedServo("EV", 130, 160, 13);
    servos[2] = new SpeedServo("LU", 70, 127, 14);
    servos[3] = new SpeedServo("LD", 60, 90, 15);
}

void setup()
{
    Serial.begin(115200, SERIAL_8N1, -1, 16);
    Serial.println("Initializing....");
    boolean result = WiFi.softAP(ssid, password);
    if (result == true)
    {
        Serial.println("Accesspoint Ready");
        Serial.print("IP Address: ");
        Serial.println(WiFi.softAPIP());
        telnetServer.begin();
        telnetServer.setNoDelay(true);
    }
    else
    {
        Serial.println("Accesspoint Failed!");
    }
    // red status led
    pinMode(LED, OUTPUT);
    digitalWrite(LED, LED_OFF);

    setupServos();

    remoteChannel.begin();
}

static bool showIp = true;
static const char *welcome = "Welcome to project OneEye";
static const char *prompt = "ESP32> ";

void loop()
{
    remoteChannel.tick();

    if (telnetServer.hasClient())
    {
        // Handle incoming connection
        telnetClient = telnetServer.accept();
        showIp = false;
        Serial.println(welcome);
        telnetClient.println(welcome);
        telnetClient.print(prompt);

        // Debug, show address of each servo instance
        /*
        for(int index =0; index < NRSERVOS; index++) {
            telnetClient.printf("adr servo[%d]=%d\n", index, servos[index]);
        }
        */
    }

    // Check for client disconnections
    if (!telnetClient || !telnetClient.connected())
    {
        // Reset the telnetClient object to its original state
        telnetClient.stop();
        showIp = true;
    }

    if (showIp)
    {
        Serial.print("IP Address: ");
        Serial.println(WiFi.softAPIP());
        delay(1000); // ms
    }

    if (telnetClient && telnetClient.connected())
    {
        static char inputBuffer[MAX_USER_INPUT];
        static int inputIndex = 0;

        while (telnetClient.available())
        {
            char c = telnetClient.read();
            if (c == '\n' || c == '\r')
            {
                if (inputIndex > 0)
                {
                    inputBuffer[inputIndex] = '\0';
                    char *parseError = set_input_string(inputBuffer);
                    telnetClient.println(parseError);
                    inputIndex = 0;
                    telnetClient.print(prompt);
                }
                else
                {
                    // Empty line, just show prompt again
                    telnetClient.print(prompt);
                }
            }
            else if (inputIndex < MAX_USER_INPUT - 1)
            {
                inputBuffer[inputIndex++] = c;
            }
        }
    }
}