/**
 * @file    InputHelper.h
 *
 * @brief   Provides the InputHelper class, used to handle input de-bounce and
 *          other common and tedious things.
 *
 * @author  Kris Dunning (ippie52@gmail.com)
 * @date    2020
 */
#pragma once

/// @brief  Provides the function pointer type definition for the input state handler.
typedef void (*InputToggleCallback)(
    const int pin,
    const int newState,
    const long lastChange
);

/// @brief  Provides function pointer type definition for the input
typedef void (*InputTimeoutCallback)(const int pin, const long durationMs);

/**
 * Class used to make handling input signals easier.
 */
class InputHelper
{
public:
    /**
     * @brief   Constructor - Takes the pin and the state handler.
     *
     * @param   pin                 The input pin to monitor.
     * @param   toggle_callback     The static callback handler for state
     *                              changes.
     * @param   timeout_callback    The static callback handler for when an
     *                              input is triggered for a particular length
     *                              of time.
     */
    InputHelper(
        const int pin,
        InputToggleCallback toggle_callback,
        InputTimeoutCallback timeout_callback=nullptr,
        const long timeout_duration_ms=10000
    )
    : pin(pin)
    , toggle_callback(toggle_callback)
    , lastState(digitalRead(pin))
    , lastChangeMs(millis())
    , timeout_callback(timeout_callback)
    , timeout_duration_ms(timeout_duration_ms)
    , trigger_timeout(true)
    {
        pinMode(pin, INPUT);
    }

    /**
     * @brief   Polls event, to be called in the loop() function. Checks the
     *          current state of the input and signals the toggle callback if
     *          the state has changed since the last poll.
     */
    void poll()
    {
        // Read twice with short delay to remove any button bounce
        const long currentTimeMs = millis();
        const int a = digitalRead(pin);
        delay(10);
        const int b = digitalRead(pin);
        if (a == b)
        {
            const long duration = currentTimeMs - lastChangeMs;
            if (a != lastState)
            {
                signalToggleCallback(pin, a, duration);

                lastState = a;
                lastChangeMs = currentTimeMs;
                // If we're now toggled off, reset the time-out trigger
                if (lastState)
                {
                    trigger_timeout = true;
                }
            }
            else if (trigger_timeout &&
                lastState &&
                (duration >= timeout_duration_ms))
            {
                trigger_timeout = false;
                signalTimeoutCallback(pin, duration);
            }
        }
    }

    /**
     * @brief   Signals the toggle callback handler
     *
     * @param   pin         The input pin
     * @param   state       The new state of the input
     * @param   duration    The duration of the last state (in milliseconds)
     */
    virtual void signalToggleCallback(const int pin, const int state, const long duration)
    {
        if (toggle_callback != nullptr)
        {
            toggle_callback(pin, state, duration);
        }
    }

    /**
     * @brief   Signals the time-out callback handler
     *
     * @param   pin         The input pin that triggered this event
     * @param   duration    The time in milliseconds that the pin was set high
     */
    virtual void signalTimeoutCallback(const int pin, const long duration)
    {
        if (timeout_callback != nullptr)
        {
            timeout_callback(pin, duration);
        }
    }

    /**
     * @brief   Gets the current state as an integer.
     *
     * @return  The current state of the input.
     */
    operator int() const
    {
        return lastState;
    }


protected:
    /// @brief  The input pin to monitor
    const int pin;
    /// @brief  The toggle callback for handling state changes
    InputToggleCallback toggle_callback;
    /// @brief  The last state of this input.
    int lastState;
    /// @brief  The last state change time in milliseconds
    long lastChangeMs;
    /// @brief  The time-out callback for handling button presses of a certain
    ///         duration or longer
    InputTimeoutCallback timeout_callback;
    /// @brief  The time-out callback duration.
    long timeout_duration_ms;
    /// @brief  Indicates whether to send the time-out callback
    bool trigger_timeout;
};

