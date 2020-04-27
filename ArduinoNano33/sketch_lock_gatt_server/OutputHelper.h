/**
 * @file    OutputHelper.h
 *
 * @brief   Provides the OutputHelper class, used as a wrapper around outputs.
 *
 * @author  Kris Dunning (ippie52@gmail.com)
 * @date    2020
 */

/**
 * Class used to make handling output signals easier.
 */
class OutputHelper
{
public:
    /**
     * @brief   Constructor - Takes the pin and the state handler.
     *
     * @param   pin     The input pin to monitor.
     * @param   state   The initial state of the pin.
     */
    OutputHelper(const int pin, const int state=LOW)
    : pin(pin)
    {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, state ? HIGH : LOW);
    }

    /**
     * @brief   Gets the state of the input from the last poll as an integer.
     *
     * @return  The current state of the input.
     */
    operator int() const
    {
        return digitalRead(pin);
    }

    /**
     * @brief   Assignment operator for easily writing the value of the output.
     *
     * @return  Reference to this OutputHelper object.
     */
    OutputHelper &operator=(const int value)
    {
        digitalWrite(pin, value ? HIGH : LOW);
    }

private:
    /// @brief  The input pin to monitor
    const int pin;
};
