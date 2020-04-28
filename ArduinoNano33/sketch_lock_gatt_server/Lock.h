/**
 * @file    Lock.h
 *
 * @brief   Provides the Lock class, which keeps track of the lock and the
 *          outputs associated with it.
 *
 * @author  Kris Dunning (ippie52@gmail.com)
 * @date    2020
 */
#pragma once

#include "OutputHelper.h"
#include "ClassInputHelper.h"

/**
 * Class used to monitor and update the state of the lock
 */
class Lock
{
public:
    /**
     * @brief   Constructor - Sets up the lock and the relevant inputs and outputs
     *
     * @param   locked      The pin to output the locked state
     * @param   unlocked    The pin to output the unlocked state
     * @param   override    The pin to monitor for manual override
     * @param   duration    The lock duration in milliseconds (default 4 seconds)
     */
    Lock(const int locked, const int unlocked, const int override, const int duration = 4000)
    : lockedLed(locked, HIGH)
    , unlockedLed(unlocked)
    , unlockedDurationMs(duration)
    , overridePin(override, this, &Lock::manualOverrideHandler)
    , lastUnlocked(0L)
    , locked(true)
    {
    }

    /**
     * @brief   Unlocks the lock
     */
    void unlock()
    {
        Serial.println("Unlocking.");
        lastUnlocked = millis();
        unlockedLed = HIGH;
        lockedLed = LOW;
        locked = false;
    }

    /**
     * @brief   Locks the lock
     */
    void lock()
    {
        Serial.println("Locking.");
        lastUnlocked = 0;
        unlockedLed = LOW;
        lockedLed = HIGH;
        locked = true;
    }

    /**
     * @brief   Handles the change in manual override state when the input is
     *          polled
     *
     * @param   pin         The pin on which the signal was triggered
     * @param   state       The new state of the switch
     * @param   durationMs  The duration of the last state in milliseconds
     */
    void manualOverrideHandler(const int pin, const int state, const long durationMs)
    {
        if (state)
        {
            Serial.println("Manual override triggered.");
            unlock();
        }
        else
        {
            // Don't lock immediately, allow the delay
            Serial.println("Manual override ended. Lock will be closed after delay.");
            lastUnlocked = millis();
        }
    }

    /**
     * @brief   Polls the lock for any changes and locks if the delay has
     *          expired.
     */
    void poll()
    {
        overridePin.poll();
        // If not manually overridden, check for the time-out.
        if (!overridePin && !locked)
        {
            const long delta = millis() - lastUnlocked;
            if (delta >= unlockedDurationMs)
            {
                lock();
            }
        }
    }

private:
    /// @brief  The locked LED output
    OutputHelper lockedLed;
    /// @brief  The unlocked LED output
    OutputHelper unlockedLed;
    /// @brief  The duration (in milliseconds for unlocking signals)
    const int unlockedDurationMs;
    /// @brief  The manual override input
    ClassInputHelper<Lock> overridePin;
    /// @brief  The time (in milliseconds since start) when last unlocked
    long lastUnlocked;
    /// @brief  The current lock state
    bool locked;
};
