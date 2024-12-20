/*
 * This version uses preferences class to read write settings.
 * based on https://github.com/lylavoie/PersistSettings/blob/main/include/PersistSettings.h
*/

#ifndef __OLON_CONFIG__
  #define __OLON_CONFIG__

  #include <Arduino.h>
  #include <Preferences.h>

  #define OLON_CONFIG_VERSION "1.0.3"

namespace Olon {
template <class T>
class Config {
 public:
  T data;
  Config(String name);
  bool     read();
  bool     write();
  void     writeDefaults();
  bool     valid();
  void     deleteConfig();
  uint16_t CRC16(const uint8_t* data, size_t data_len);

 private:
  // Inline logging functions replacing macros
  void logDebug(const char* tag, const char* format, ...) {
    #ifdef OLON_DEBUG_CONFIG
      va_list args;
      va_start(args, format);
      char buffer[256]; // Adjust size as needed
      vsnprintf(buffer, sizeof(buffer), format, args);
      Serial.printf("\033[0;36m%6lu [D] [%s] %s\033[0m\n", millis(), tag, buffer);
      va_end(args);
    #endif
  }

  void logInfo(const char* tag, const char* format, ...) {
    #ifdef OLON_DEBUG_CONFIG
      va_list args;
      va_start(args, format);
      char buffer[256]; // Adjust size as needed
      vsnprintf(buffer, sizeof(buffer), format, args);
      Serial.printf("\033[0;32m%6lu [I] [%s] %s\033[0m\n", millis(), tag, buffer);
      va_end(args);
    #endif
  }
// Inline logging functions replacing macros
  void logWarning(const char* tag, const char* format, ...) {
    #ifdef OLON_DEBUG_CONFIG
      va_list args;
      va_start(args, format);
      char buffer[256]; // Adjust size as needed
      vsnprintf(buffer, sizeof(buffer), format, args);
      Serial.printf("\033[0;33m%6lu [W] [%s] %s\033[0m\n", millis(), tag, buffer);
      va_end(args);
    #endif
  }

  void logError(const char* tag, const char* format, ...) {
    #ifdef OLON_DEBUG_CONFIG
      va_list args;
      va_start(args, format);
      char buffer[256]; // Adjust size as needed
      vsnprintf(buffer, sizeof(buffer), format, args);
      Serial.printf("\033[0;31m%6lu [E] [%s] %s\033[0m\n", millis(), tag, buffer);
      va_end(args);
    #endif
  }

  void logBulk(const char* format, ...) {
    #ifdef OLON_DEBUG_CONFIG
      va_list args;
      va_start(args, format);
      char buffer[256]; // Adjust size as needed
      vsnprintf(buffer, sizeof(buffer), format, args);
      Serial.printf("%s", buffer);
      va_end(args);
    #endif
  }

  bool     _valid;
  String   _name;
  constexpr static const char* const TAG = "CONFIG";
  constexpr static const char* const configKey = "config";
};

} // namespace
#endif //__OLON_CONFIG__

// because the header uses template, the implementation must exist in same header file

// Constructor
template <class T>
Olon::Config<T>::Config(String name) {
  _name  = name;
  _valid = false;
}

// Initialize the settings object, read the config from the
// LittleFS storage, and validate data.  If the config object
// can not be read, an error is detected,
// the Config object will be reset to the default values.
template <class T>
bool Olon::Config<T>::read(void) {
  _valid = false;
  Preferences prefs;
  logDebug(TAG, "Reading configuration...");
  // Setup the preferences namespace, as read only
  if (!prefs.begin(_name.c_str(), true)) {
    logError(TAG, "Failed to open PersistSettings namespace.");
    writeDefaults();
    return false;
  }

  // Read the settings from the persistent storage.
  // The last 2 bytes are CRC16
  size_t configLen = prefs.getBytesLength(configKey);

  if (sizeof(T) + 2 != configLen) {
    logError(TAG, "Length read failure (read: %d, expected: %d).", configLen, sizeof(T) + 2);
    writeDefaults();
    return false;
  }

  byte memBytes[configLen];
  prefs.getBytes(configKey, memBytes, configLen);

  // Close the preferences
  prefs.end();

  uint16_t savedCRC = ((uint16_t)memBytes[configLen - 2]) |
                      ((uint16_t)memBytes[configLen - 1] << 8);

  uint16_t calculatedCRC = CRC16(memBytes, configLen - 2);

  // Check the CRC16
  if (savedCRC != calculatedCRC) {
    logError(TAG, "CRC16 check failed");
    writeDefaults();
    return false;
  }

  // All checks passed, valid config, copy the data to the object.
  memcpy(&data, memBytes, sizeof(T));

  // Indicate the config is valid
  _valid = true;
  logDebug(TAG, "Configuration read ok.");
  return true;
}

// Write the current values of the Config object to persistent storage.
template <class T>
bool Olon::Config<T>::write(void) {
  Preferences prefs;

  // Setup the preferences namespace, as write
  if (!prefs.begin(_name.c_str(), false)) {
    logError(TAG, "Error opening \"%s\" configuration", _name.c_str());
    return false;
  }

  // Save the config, as a byte array, with 2-byte CRC on the end.
  byte memBytes[sizeof(T) + 2];
  memcpy(memBytes, &data, sizeof(T));
  uint16_t crc            = CRC16(memBytes, sizeof(T));
  memBytes[sizeof(T)]     = static_cast<byte>((crc & 0xFF));
  memBytes[sizeof(T) + 1] = static_cast<byte>((crc >> 8) & 0xFF);
  size_t savedBytes = prefs.putBytes(configKey, memBytes, sizeof(T) + 2);

  // Close the preferences
  prefs.end();
  logDebug(TAG, "Saved %zu bytes to \"%s\" configuration", savedBytes, _name.c_str());
  return true;
}

// Indicates the Config data object is valid.
template <class T>
bool Olon::Config<T>::valid(void) {
  return _valid;
}

template <class T>
inline void Olon::Config<T>::deleteConfig() {
  Preferences prefs;
  if (!prefs.begin(_name.c_str(), false)) {
    logError(TAG, "Error opening \"%s\" configuration", _name.c_str());
    return;
  }
  logDebug(TAG, "Clearing \"%s\" configuration", _name.c_str());
  prefs.clear();
  prefs.end();
}

// Resets the Config object to the default value and provided during the
// construction of the PersistSettings object and writes those values to
// the persistent storage.
template <class T>
void Olon::Config<T>::writeDefaults(void) {
  T newData;
  data = newData;
  logDebug(TAG, "Saving default configuration");
  write();
  _valid = true;
}

// Calculate the CRC16 over the provided data object (byte array) and
// return the value.
template <class T>
uint16_t Olon::Config<T>::CRC16(const uint8_t* data, size_t data_len) {
  uint16_t crc = 0xFFFF;
  while (data_len--) {
    crc ^= *data++;
    for (uint8_t i = 0; i < 8; i++) {
      if (crc & 0x01) {
        crc >>= 1;
        crc ^= 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}
