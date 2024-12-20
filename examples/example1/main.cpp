
/* Modified Preferences from
 * SPDX-License-Identifier: MIT
 * Copyright(c) 2022 Lincoln Lavoie <lincoln.lavoie@gmail.com>
 * https://github.com/lylavoie/PersistSettings
 */

#include <Arduino.h>
#include <OlonConfig.h>

struct ConfigObject {
  // All values should be initialized to their default values.  This
  // object typedef is used to reset the object to the default values
  // if the settings can not be read from the persistent storage, an
  // error is detected.
  uint8_t setting1 = 1;
  bool    b        = true;
};

// Construct PersistSettings object with the template of our
// configuration object, and provide the expected version of
// the configuration.
Olon::Config<ConfigObject> conf("main");

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("Test config.");
  Serial.println("Wait for key press...");
  while (!Serial.available()) {
  }
  Serial.println("Start.");

  if (!conf.read()) {
    Serial.println("Found no valid configuration. Restoring to defaults...");
    // conf.writeDefaults();
  } else {
    conf.data.setting1++;
    conf.data.b = !conf.data.b;
    // Save the updated Config to LittleFS.
    if (!conf.write()) {
      Serial.println("Error writting to configuration file");
    }
  }

  // Log a result for the user.  This value should increase
  // each time the platform is reset.
  Serial.printf("Setting1 Value: %d bool value %d\r\n", conf.data.setting1, conf.data.b);

}

void loop() {
  // NO-OP
}