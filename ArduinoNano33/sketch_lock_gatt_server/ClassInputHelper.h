/**
 * @file    ClassInputHelper.h
 *
 * @brief   Provides the ClassInputHelper class, used to handle input de-bounce
 *          and other common and tedious things. This is for use within a class
 *          accepting class methods.
 *
 * @author  Kris Dunning (ippie52@gmail.com)
 * @date    2020
 */
#pragma once

#include "InputHelper.h"

/**
 * Class used to make handling input signals easier within a class.
 */
template<class T = void>
class ClassInputHelper : public InputHelper
{
public:
    /**
     * @brief   Constructor - Takes the pin and the state handler.
     *
     * @param   pin         The input pin to monitor.
     * @param   object      The object that the callback belongs to.
     * @param   callback    The member function callback handler for state changes.
     */
    ClassInputHelper(const int pin, T *object, void (T::*callback)(const int, const int, const long))
    : InputHelper(pin, nullptr)
    , object(object)
    , memberFunc(callback)
    {

    }

    virtual void signalCallback(const int pin, const int state, const long duration)
    {
        if (object != nullptr && memberFunc != nullptr)
        {
            (object->*memberFunc)(pin, state, duration);
        }
    }

private:
    /// @brief  The template object to signal
    T *object;
    /// @brief  The member function of the template object
    void (T::*memberFunc)(const int, const int, const long);
};
