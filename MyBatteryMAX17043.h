//
// Created by Thibault PLET on 19/05/2020.
//

#ifndef COM_OSTERES_AUTOMATION_ARDUINO_COMPONENT_MYSENSOR_MYBATTERYMAX17043_H
#define COM_OSTERES_AUTOMATION_ARDUINO_COMPONENT_MYSENSOR_MYBATTERYMAX17043_H

#include <Wire.h>
#include <MySensors.h>
#include <SparkFunMAX17043.h>
#include <ArduinoProperty.h>

class MyBatteryMAX17043 {
public:
    /**
     * Constructor
     *
     * @param intervalSend      Delay to send data and if data value changed, in milliseconds.
     * @param intervalSendForce Delay to send data and even if data value not changed, in milliseconds.
     * @param limit             Battery level limit: below this value, enter in deep sleep mode
     * @param pinInterrupt      Pin for interrupt when deep sleep mode
     */
    MyBatteryMAX17043(
        unsigned long intervalSend = 60000U, // 1 min
        unsigned long intervalSendForce = 180000U, // 3 min
        float limit = 0, // %
        unsigned int pinInterrupt = 3 // D3
    ) {
        this->intervalSend = new DataBuffer(intervalSend);
        this->intervalSendForce = new DataBuffer(intervalSendForce);
        this->levelLimit = limit;
        this->pinInterrupt = pinInterrupt;
        this->enable = true;
    }

    /**
     * Destructor
     */
    ~MyBatteryMAX17043()
    {
        delete this->intervalSend;
        delete this->intervalSendForce;
    }

    /**
     * Presentation (for MySensor)
     */
    void presentation()
    {
        if (this->isEnable()) {
            // Nothing to do
        }
    }

    /**
     * Setup
     */
    void setup()
    {
        if (this->isEnable()) {
            this->battery.begin();
            this->battery.quickStart();

        }
    }

    /**
     * Receive (for MySensor)
     */
    void receive(const MyMessage &message)
    {
        if (this->isEnable()) {
            // Nothing to do
        }
    }

    /**
     * Process
     */
    void loop()
    {
        if (this->isEnable()) {
            // Execute only if interval outdated
            if (this->intervalSend->isOutdated() || this->intervalSendForce->isOutdated()) {
                this->process();
            }
        }
    }

    /**
     * Move increment forward to the future
     */
    void bufferMoveForward(unsigned long increment)
    {
        this->intervalSend->moveForward(increment);
        this->intervalSendForce->moveForward(increment);
    }

    /**
     * Send battery level to gateway
     */
    void sendLevel()
    {
        #ifdef MY_DEBUG
        this->sendLog(String("Send battery lvl to gtw").c_str());
        #endif

        sendBatteryLevel(this->level, true);

        this->lastLevel = this->level;
    }

    /**
     * Set feature enable or not
     */
    void setEnable(bool enable)
    {
        this->enable = enable;
    }

    /**
     * Flag for enable feature
     */
    bool isEnable()
    {
        return this->enable;
    }

    /**
     * Flag to indicate if data has been sent from last loop
     */
    bool isDataSent()
    {
        return this->dataSent;
    }

    /**
     * Set levelLimitSecurityCptMax
     */
    void setLevelLimitSecurityCptMax(unsigned int levelLimitSecurityCptMax)
    {
        this->levelLimitSecurityCptMax = levelLimitSecurityCptMax;
    }

    /**
     * Get interval send
     */
    DataBuffer * getIntervalSend()
    {
        return this->intervalSend;
    }

    /**
     * Get interval send force
     */
    DataBuffer * getIntervalSendForce()
    {
        return this->intervalSendForce;
    }

protected:

    void process()
    {
        this->dataSent = false;
        this->level = this->battery.getSOC();

        // Send to gateway
        if (
            this->intervalSendForce->isOutdated() || 
            this->intervalSend->isOutdated() && this->level != this->lastLevel
        ) {
            this->sendLevel();
            this->dataSent = true;
        }

        // Reset interval
        if (this->intervalSend->isOutdated()) {
            this->intervalSend->reset();
        }
        if (this->intervalSendForce->isOutdated()) {
            this->intervalSendForce->reset();
        }

        // If level lower than limit, increment counter to check several time
        if (this->level < this->levelLimit) {
            this->levelLimitSecurityCpt++;
        } else {
            this->levelLimitSecurityCpt = 0;
        }

        // If battery low confirmed: Enter in deep sleep mode
        if (this->levelLimitSecurityCpt >= this->levelLimitSecurityCptMax) {
            this->levelLimitSecurityCpt = 0;
            this->level = 0;
            this->sendLevel();

            // Enter in deep sleep mode
            #ifdef MY_DEBUG
            this->sendLog(String("Deep sleep mode!").c_str());
            #endif
            sleep(digitalPinToInterrupt(this->pinInterrupt), CHANGE, 0);
            // Here: code not executed
        }
    }

    /**
     * Send log
     *
     * @param char * message Log message (max 25 bytes). To confirm: 10 char max
     */
    void sendLog(const char * message)
    {
      MyMessage msg;
      msg.sender = getNodeId();
      msg.destination = GATEWAY_ADDRESS;
      msg.sensor = NODE_SENSOR_ID;
      msg.type = I_LOG_MESSAGE;
      mSetCommand(msg, C_INTERNAL);
      mSetRequestEcho(msg, true);
      mSetEcho(msg, false);

      msg.set(message);

      _sendRoute(msg);
    }

    /*
     * Enable feature or not
     */
    bool enable = true;
    
    /**
     * Interval to send battery level to gateway if data changed
     */
    DataBuffer * intervalSend;

    /*
     * Interval to send battery level to gateway, even if data not changed
     */
    DataBuffer * intervalSendForce;

    /**
     * Battery level value
     */
    float level;

    /**
     * Last battery level value
     */
    float lastLevel;

    /**
     * Battery level limit. Below this value, enter in deep sleep mode
     */
    float levelLimit;

    /**
     * Counter to check battery level limit several time (for security check)
     */
    unsigned int levelLimitSecurityCpt = 0;

    /**
     * Max time which battery level limit considered as true, and enter deep sleep mode
     */
    unsigned int levelLimitSecurityCptMax = 3;

    /**
     * Battery gauge
     */
    MAX17043 battery;

    /**
     * Flag to indicate if data has been sent to gateway from last loop
     */
    bool dataSent;

    /**
     * Pin for interrupt when deep sleep mode (@see attachInterrupt method)
     * Set pin value (will be set to digitalPinToInterrupt method)
     */
    unsigned int pinInterrupt = 3; // D3
};

#endif //COM_OSTERES_AUTOMATION_ARDUINO_COMPONENT_MYSENSOR_MYBATTERYMAX17043_H
