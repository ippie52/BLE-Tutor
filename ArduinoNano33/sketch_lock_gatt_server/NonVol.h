/**
 * @file    NonVol.h
 *
 * @brief   Provides the templated NonVol class, used to read and write values
 *          to EEPROM using a simplified interface.
 *
 * @author  Kris Dunning (ippie52@gmail.com)
 * @date    2020
 */

// @TODO Have a better means of identifying whether EEPROM available.
#if defined(ARDUINO_ARCH_SAMD)
#error TODO - Find alternative means of writing to and from SRAM
#include <FlashAsEEPROM.h>

#else
#include <EEPROM.h>
#endif // Board check

/**
 * Class to wrap around the EEPROM get/put functions, to make reading and
 * writing slightly easier.
 */
template<class T>
class NonVol
{
public:
    /**
     * @brief   Constructor - Takes the address to read/write in EEPROM
     *
     * @param   address     The EEPROM address to read and write
     */
    NonVol(const int address)
    : address(address)
    { }

    /**
     * @brief   Read functor - Gets the value currently stored in memory.
     *
     * @return  Reference to the value.
     */
    T operator() ()
    {
        return EEPROM.get(address, this->value);
    }

    /**
     * @brief   Cast conversion to object type T. This allows easy assignment
     *          to the template type, making this class transparent.
     *
     * @return  Reference to the underlying value.
     */
    operator T() const
    {
        return EEPROM.get(address, this->value);
    }

    /**
     * @brief   Assignment operator, assigns the template type value to memory
     *          and returns a reference to this NonVol object.
     *
     * @param   other   The other template value being assigned to this.
     *
     * @return  Reference to this object.
     */
    NonVol& operator=(T other)
    {
        this->value = other;
        EEPROM.put(address, other);
        return *this;
    }

    /**
     * @brief   Write functor - Writes the new value to memory
     *
     * @param   value   The template object value to be written to EEPROM.
     */
    void operator() (T value)
    {
        this->value = value;
        EEPROM.put(address, value);
    }

private:
    /// @brief  The address within EEPROM to read/write.
    const int address;
    /// @brief  The storage value for reading to/from.
    T value;
};
