/**
 * @file    InputHelper.h
 *
 * @brief   Provides the InputHelper class, used to handle input de-bounce and
 *          other common and tedious things.
 *
 * @author  Kris Dunning (ippie52@gmail.com)
 * @date    2020
 */


// Provides the function pointer type definition for the input state handler.
typedef void InputToggleCallback(
    const int pin,
    const int newState,
    const long lastChange
);


/**
 * Class used to make handling input signals easier.
 */
class InputHelper
{
public:
    /**
     * @brief   Constructor - Takes the pin and the state handler.
     *
     * @param   pin         The input pin to monitor.
     * @param   callback    The callback handler for state changes.
     */
    InputHelper(const int pin, InputToggleCallback *callback)
    : pin(pin)
    , callback(callback)
    , lastState(digitalRead(pin))
    , lastChangeMs(millis())
    {
        pinMode(pin, INPUT);
    }

    /**
     * @brief   Polls event, to be called in the loop() function. Checks the
     *          current state of the input and signals the callback if the state
     *          has changed since the last poll.
     */
    void poll()
    {
        // Read twice with short delay to remove any button bounce
        const long currentTimeMs = millis();
        const int a = digitalRead(pin);
        delay(10);
        const int b = digitalRead(pin);
        if ((a == b) && (a != lastState))
        {
            if (callback != nullptr)
            {
                callback(pin, a, currentTimeMs - lastChangeMs);
            }
            lastState = a;
            lastChangeMs = currentTimeMs;
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


private:
    /// @brief  The input pin to monitor
    const int pin;
    /// @brief  The callback for handling state changes
    InputToggleCallback *callback;
    /// @brief  The last state of this input.
    int lastState;
    /// @brief  The last state change time in milliseconds
    long lastChangeMs;
};
