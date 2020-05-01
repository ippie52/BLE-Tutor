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

#include <string.h>
// The Arduino Nano 33 IoT/BLE does not come with any EEPROM, so SRAM must be
// used. The FlashStorage library provides this. As with the ArduinoBLE library,
// go to "Tools" > "Manage Libraries" then search for FlashStorage.
#include <FlashStorage.h>

#include "ClassInputHelper.h"
#include "Common.h"
#include "OutputHelper.h"

/**
 * Constants and definitions
 */

/// @brief  The number of unlock events to store
#define MAX_UNLOCK_TIMES        (10)

/// @brief  The default time that the lock is opened in milliseconds
#define DEFAULT_UNLOCK_TIME_MS  (5000)

/// @brief  The time for the multi-purpose button to be pressed before allowing
///         the secret code to be updated.
#define SECRET_CODE_DELAY_MS    (5000)

/// @brief  The time for the multi-purpose nutton to be pressed before sending
///         a full log message to the client.
#define FULL_LOG_DURATION_MS    (2000)

/// @brief  The maximum length of the secret code. This is limited to the number
///         of characters available in a standard write message.
static const int SECRET_CODE_MAX_LENGTH = MAX_PACKET_LENGTH;

/// @brief  The default string to unlock the device
#define DEFAULT_SECRET_CODE     "BLE-Tutor"

/**
 * Structures, type definitions and enumerations
 */

/*******************************************************************************
 * @brief   Provides all of the available lock states
 */
enum LockState
{
    Locked = 1,
    Unlocked = 2,
    ManuallyOverridden = (Unlocked + 0x10),
    UpdateSecretCode = 0x20,
};

/*******************************************************************************
 * @brief   Structure to hold the secret code string. This is required to store
 *          the value in flash memory.
 */
struct Secret
{
    /// @brief  The storage buffer for the secret code, stored in flash.
    char code[SECRET_CODE_MAX_LENGTH];
};

/**
 * @brief  Callback function pointer definition for when the lock state changes
 */
typedef void (*LockStateChangedCallback)(const LockState state);

/**
 * @brief  Callback function pointer definition for log messages
 */
typedef void (*LogMessageCallback)(const bool full);

/**
 * Globals
 */
/// @brief  Create a storage variable for reading and writing the secret code.
FlashStorage(secret_code, Secret);


/**
 * Class used to monitor and update the state of the lock
 */
class Lock
{
public:
    /***************************************************************************
     * @brief   Constructor - Sets up the lock and the relevant inputs and
     *          outputs
     *
     * @param   lockLed         The pin to output the locked state
     * @param   unlockLed       The pin to output the unlocked state
     * @param   override        The pin to monitor for manual override
     * @param   mpPin           The multi-purpose pin input
     * @param   stateCallback   The callback handler for the lock state change
     * @param   duration        The lock duration in milliseconds
     */
    Lock(
        const int lockLed,
        const int unlockLed,
        const int override,
        const int mpPin,
        LockStateChangedCallback stateCallback,
        LogMessageCallback logCallback,
        const int duration = DEFAULT_UNLOCK_TIME_MS
    )
    : lockedLed(lockLed, HIGH)
    , unlockedLed(unlockLed)
    , overridePin(override, this, &Lock::manualOverrideHandler, nullptr, 0L)
    , multipurposePin(mpPin, this, &Lock::mpPinPressed, &Lock::mpPinTimeout, SECRET_CODE_DELAY_MS)
    , stateCallback(stateCallback)
    , logCallback(logCallback)
    , unlockedDurationMs(duration)
    , lastUnlocked(0L)
    , unlockIndex(0)
    , lockState(LockState::Locked)
    {
        memset(unlockTimes, 0, sizeof(long) * MAX_UNLOCK_TIMES);
    }

    /***************************************************************************
     * @brief   Unlocks the lock if the secret message is correct.
     *
     * @param   message     The secret message to check
     */
    bool unlockWithMessage(const char *message)
    {
        Secret secret = secret_code.read();
        if ((lockState & LockState::UpdateSecretCode))
        {
            const int len = strlen(message);
            memcpy(secret.code, message, len);
            secret.code[len] = '\0';
            secret_code.write(secret);
            Serial.println(
                String("Secret code has been updated to \"") +
                secret.code +
                "\""
            );
        }
        else if ((lockState & LockState::Locked) != 0)
        {
            if (strcmp(message, secret.code) == 0)
            {
                unlock();
            }
        }
        return !isLocked();
    }

    /***************************************************************************
     * @brief   Locks the lock
     */
    void lock()
    {
        Serial.println("Locking.");
        lastUnlocked = 0;
        unlockedLed = LOW;
        lockedLed = HIGH;
        updateLockState(LockState::Locked);
    }

    /***************************************************************************
     * @brief   Polls the lock for any changes and locks if the delay has
     *          expired.
     */
    void poll()
    {
        overridePin.poll();
        multipurposePin.poll();
        // If not manually overridden, check for the time-out.
        if (!overridePin && !isLocked())
        {
            const long delta = millis() - lastUnlocked;
            if (delta >= unlockedDurationMs)
            {
                lock();
            }
        }
    }

    /***************************************************************************
     * @brief   Returns the lock state of the lock
     *
     * @return  The lock state, true if locked.
     */
    bool isLocked() const
    {
        return (lockState & LockState::Locked) != 0;
    }

    /***************************************************************************
     * @brief   Converts the lock to a bool type, indicating whether it's locked
     *
     * @return  The lock state, true if locked.
     */
    operator bool() const
    {
        return (lockState & LockState::Locked) != 0;
    }

    /***************************************************************************
     * @brief   Gets the current lock state of the lock
     */
    LockState getLockState() const
    {
        return lockState;
    }

    /***************************************************************************
     * @brief   Gets the time-stamp of the unlock "offset" times ago
     *
     * @param   offset  The offset into the history of unlocks
     *
     * @return  The time since power on that the unlock occurred. Zero if no
     *          unlock data available for the given offset.
     */
    long getUnlockTime(const int offset = 0) const
    {
        int index = unlockIndex - offset + MAX_UNLOCK_TIMES;
        index = index % MAX_UNLOCK_TIMES;
        long value = 0;
        if ((unsigned int)index < MAX_UNLOCK_TIMES)
        {
            value = unlockTimes[index];
        }

        return value;
    }

    /***************************************************************************
     * @brief   Initialises the Lock, by checking and setting the secret code
     */
    static void initialise()
    {
        // Get the secret code from flash, or assign the default value
        Secret secret = secret_code.read();
        if (strlen(secret.code) == 0)
        {
            Serial.println(
                String("Secret code not found - Setting default: ") +
                DEFAULT_SECRET_CODE
            );
            sprintf(secret.code, DEFAULT_SECRET_CODE);
            secret_code.write(secret);
        }
    }

    /*******************************************************************************
     * @brief   Prints the start-up information to the serial port.
     */
    static void printStartInfo()
    {
        Serial.print("Secret code to unlock remotely: \"");
        Secret secret = secret_code.read();
        Serial.print(secret.code);
        Serial.println("\"");

        Serial.println("To update the secret code:");
        Serial.println("1. Connect to the device via Bluetooth.");
        Serial.println("2. Press and hold the log button until the lock LED flashes.");
        Serial.println("3. When the LED stops flashing, write the new value.");
        Serial.println("4. Await status confirmation.");
    }

private:

    /***************************************************************************
     * @brief   Unlocks the lock
     *
     * @param   manualOverride  Indicates whether the lock is manually
     *          overridden. Default, not manually overridden.
     */
    void unlock(const bool manualOverride=false)
    {
        // We only want to record this one first unlock attempt
        if (isLocked())
        {
            // Increment first to make obtaining the current index easier.
            unlockIndex = (unlockIndex + 1) % MAX_UNLOCK_TIMES;
            unlockTimes[unlockIndex] = millis();
        }

        Serial.println("Unlocking.");
        lastUnlocked = millis();
        unlockedLed = HIGH;
        lockedLed = LOW;
        updateLockState(manualOverride ? LockState::ManuallyOverridden : LockState::Unlocked);
    }

    /***************************************************************************
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
            unlock(true);
        }
        else
        {
            // Don't lock immediately, allow the delay
            Serial.println("Manual override ended. Lock will be closed after delay.");
            lastUnlocked = millis();
        }
    }

    /***************************************************************************
     * @brief   Handler for when the multi-purpose button is pressed.
     *
     * @param   pin             The input pin triggering the handler.
     * @param   state           The new state of the input.
     * @param   durationMs      The duration of the press/release in milliseconds.
     */
    void mpPinPressed(const int pin, const int state, const long durationMs)
    {
        // If the button is pressed for more than this time, a full log is sent,
        // line by line.
        Serial.print("Log button has been ");
        Serial.println(state ? "pressed." : "released.");
        if (!state)
        {
            if (durationMs >= SECRET_CODE_DELAY_MS)
            {
                // Ignore this situation - it'll be handled by the time-out handler
            }
            else if (durationMs >= FULL_LOG_DURATION_MS)
            {
                if (logCallback != nullptr)
                {
                    logCallback(true);
                }
            }
            else
            {
                if (logCallback != nullptr)
                {
                    logCallback(false);
                }
            }
        }
    }

    /***************************************************************************
     * @brief   Handler for when the multi-purpose button is pressed for a
     *          certain length of time.
     *
     * @param   pin         The input pin that triggered the handler
     * @param   durationMs  The duration that the button was pressed for
     */
    void mpPinTimeout(const int pin, const long durationMs)
    {
        if (durationMs >= SECRET_CODE_DELAY_MS)
        {
            // Flash the connected LED then continue.
            const int originalState = lockedLed;
            for (int i = 0; i < 10; ++i)
            {
                lockedLed = !lockedLed;
                delay(lockedLed ? 100 : 200);
            }
            lockedLed = originalState;
            updateLockState((LockState)(LockState::UpdateSecretCode | lockState));
        }
        else
        {
            Serial.println("The expected duration has not been met.");
        }
    }

    /***************************************************************************
     * @brief   Updates the current lock state, then signals any callbacks.
     *
     * @param   newState    The new lock state
     */
    void updateLockState(const LockState newState)
    {
        lockState = newState;
        if (stateCallback != nullptr)
        {
            stateCallback(lockState);
        }
    }

    /// @brief  The locked LED output
    OutputHelper lockedLed;
    /// @brief  The unlocked LED output
    OutputHelper unlockedLed;
    /// @brief  The manual override input
    ClassInputHelper<Lock> overridePin;
    /// @brief  The multi-purpose request button input
    ClassInputHelper<Lock> multipurposePin;
    /// @brief  The callback handler for state changes
    LockStateChangedCallback stateCallback;
    /// @brief  The callback handler for log messages
    LogMessageCallback logCallback;
    /// @brief  The duration (in milliseconds for unlocking signals)
    const int unlockedDurationMs;
    /// @brief  The time (in milliseconds since start) when last unlocked
    long lastUnlocked;
    /// @brief  The unlock times buffer, stores the last few unlock times
    long unlockTimes[MAX_UNLOCK_TIMES];
    /// @brief  The current unlock time index
    int unlockIndex;
    /// @brief  The current state of the lock
    LockState lockState;
};
