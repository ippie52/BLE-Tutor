/**
 * @file    ClassInputHelper.h
 *
 * @brief   Provides the ClassInputHelper class, used to handle input de-bounce
 *          and other common and tedious things. This is for use within a class
 *          accepting class methods, rather than its parent, that only accepts
 *          unbound functions.
 *
 * @author  Kris Dunning (ippie52@gmail.com)
 * @date    2020
 */
#pragma once

#include "InputHelper.h"

/*******************************************************************************
 * Class used to make handling input signals easier within a class.
 */
template<class T = void>
class ClassInputHelper : public InputHelper
{
public:
    /***************************************************************************
     * @brief   Constructor - Takes the pin and the state handler.
     *
     * @param   pin             The input pin to monitor.
     * @param   object          The object that the callback belongs to.
     * @param   toggle_callback The member function callback handler for state
     *                          changes to the input.
     * @param   timeout_callback The member function callback handler for inputs
     *                          set high for a prolonged period of time.
     * @param   timeout_duration_ms The number of milliseconds to wait after a
     *                          signal is set high before triggering the
     *                          timeout_callback.
     */
    ClassInputHelper(
        const int pin,
        T *object,
        void (T::*toggle_callback)(const int, const int, const long),
        void (T::*timeout_callback)(const int, const long),
        const long timeout_duration_ms)
    : InputHelper(pin, nullptr, nullptr, timeout_duration_ms)
    , object(object)
    , toggleMethod(toggle_callback)
    , timeoutMethod(timeout_callback)
    {
    }

    /***************************************************************************
     * @brief   Signals the toggle callback handler
     *
     * @param   pin         The input pin
     * @param   state       The new state of the input
     * @param   duration    The duration of the last state (in milliseconds)
     */
    virtual void signalToggleCallback(const int pin, const int state, const long duration)
    {
        if (object != nullptr && toggleMethod != nullptr)
        {
            (object->*toggleMethod)(pin, state, duration);
        }
    }

    /***************************************************************************
     * @brief   Signals the time-out callback handler
     *
     * @param   pin         The input pin that triggered this event
     * @param   duration    The time in milliseconds that the pin was set high
     */
    virtual void signalTimeoutCallback(const int pin, const long duration)
    {
        if (object != nullptr && timeoutMethod != nullptr)
        {
            (object->*timeoutMethod)(pin, duration);
        }
    }

private:
    /// @brief  The template object to signal
    T *object;
    /// @brief  The member function of the template object for toggle callbacks
    void (T::*toggleMethod)(const int, const int, const long);
    /// @brief  The member function of the template object for time-out callbacks
    void (T::*timeoutMethod)(const int, const long);
};
