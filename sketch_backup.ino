
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <math.h>
#include <stdlib.h>

// LCD configuration
LiquidCrystal_I2C lcd(0x27, 16, 2);

const int SDA_PIN = 21;
const int SCL_PIN = 22;

// Battery thresholds
const float MIN_VOLTAGE = 9.6;
const float MAX_VOLTAGE = 12.6;
const float MAX_TEMPERATURE = 45.0;
const float MAX_CURRENT = 5.0;

const int MAX_LOG_ENTRIES = 10;

enum FaultCode {
  LOW_VOLTAGE = 1,
  HIGH_VOLTAGE,
  HIGH_TEMPERATURE,
  HIGH_CURRENT
};

struct FaultEvent {
  unsigned long elapsedMs;
  unsigned int reportNumber;
  FaultCode code;
};

FaultEvent faultLog[MAX_LOG_ENTRIES];
unsigned long faultCounts[4] = {0, 0, 0, 0};

int logIndex = 0;
int logCount = 0;
int inputStage = 0;

unsigned int reportNumber = 0;
unsigned int warningReports = 0;

float voltage = 0;
float current = 0;
float temperature = 0;

bool lastWarning = false;

String inputLine = "";

// LCD page management
int lcdPage = 0;
unsigned long lastPageChange = 0;
const unsigned long PAGE_INTERVAL = 3000;

const char* prompts[] = {
  "Enter battery voltage (V), then press Enter:",
  "Enter battery current (A), then press Enter:",
  "Enter battery temperature (C), then press Enter:"
};

void showPrompt() {
  Serial.println();
  Serial.println(prompts[inputStage]);
}

const char* faultName(FaultCode code) {
  switch (code) {
    case LOW_VOLTAGE: return "LOW VOLTAGE";
    case HIGH_VOLTAGE: return "HIGH VOLTAGE";
    case HIGH_TEMPERATURE: return "HIGH TEMPERATURE";
    case HIGH_CURRENT: return "HIGH CURRENT";
    default: return "UNKNOWN";
  }
}

void logFault(FaultCode code) {
  faultLog[logIndex] = {millis(), reportNumber, code};

  logIndex = (logIndex + 1) % MAX_LOG_ENTRIES;

  if (logCount < MAX_LOG_ENTRIES) {
    logCount++;
  }

  faultCounts[(int)code - 1]++;

  Serial.print("FAULT CODE ");
  Serial.print((int)code);
  Serial.print(": ");
  Serial.println(faultName(code));
}

float estimateSOC(float v) {
  float soc = ((v - 9.0) / 3.6) * 100.0;

  if (soc > 100.0) soc = 100.0;
  if (soc < 0.0) soc = 0.0;

  return soc;
}

// LCD page 0: voltage, SOC, status and counters
// LCD page 1: current, temperature, status and counters
void updateLCD(bool forceUpdate = false) {
  unsigned long now = millis();

  if (!forceUpdate &&
      now - lastPageChange >= PAGE_INTERVAL) {
    lcdPage = 1 - lcdPage;
    lastPageChange = now;
    forceUpdate = true;
  }

  if (!forceUpdate) return;

  lcd.clear();
  lcd.setCursor(0, 0);

  if (lcdPage == 0) {
    lcd.print("V:");
    lcd.print(voltage, 2);
    lcd.print(" SOC:");
    lcd.print(estimateSOC(voltage), 0);
    lcd.print("%");
  } else {
    lcd.print("I:");
    lcd.print(current, 2);
    lcd.print("A T:");
    lcd.print(temperature, 0);
    lcd.print("C");
  }

  lcd.setCursor(0, 1);

  if (lastWarning) {
    lcd.print("WARNING");
  } else {
    lcd.print("NORMAL");
  }

  lcd.print(" R:");
  lcd.print(reportNumber);
  lcd.print(" W:");
  lcd.print(warningReports);
}

void showFaultLog() {
  Serial.println();
  Serial.println("--- RECENT FAULT LOG ---");

  if (logCount == 0) {
    Serial.println("No faults recorded.");
  } else {
    int start =
      (logIndex - logCount + MAX_LOG_ENTRIES)
      % MAX_LOG_ENTRIES;

    for (int i = 0; i < logCount; i++) {
      int idx = (start + i) % MAX_LOG_ENTRIES;

      Serial.print("Report ");
      Serial.print(faultLog[idx].reportNumber);
      Serial.print(" | Time(ms): ");
      Serial.print(faultLog[idx].elapsedMs);
      Serial.print(" | ");
      Serial.println(faultName(faultLog[idx].code));
    }
  }

  Serial.println("--- FAULT COUNTS ---");

  for (int i = 0; i < 4; i++) {
    Serial.print(faultName((FaultCode)(i + 1)));
    Serial.print(": ");
    Serial.println(faultCounts[i]);
  }
}

void checkBattery() {
  bool warning = false;

  Serial.println();
  Serial.println("===== BATTERY REPORT =====");

  Serial.print("Report number: ");
  Serial.println(reportNumber);

  Serial.print("Voltage: ");
  Serial.print(voltage, 2);
  Serial.println(" V");

  Serial.print("Current: ");
  Serial.print(current, 2);
  Serial.println(" A");

  Serial.print("Temperature: ");
  Serial.print(temperature, 1);
  Serial.println(" C");

  Serial.print("Estimated SOC: ");
  Serial.print(estimateSOC(voltage), 1);
  Serial.println(" %");

  if (voltage < MIN_VOLTAGE) {
    logFault(LOW_VOLTAGE);
    warning = true;
  }

  if (voltage > MAX_VOLTAGE) {
    logFault(HIGH_VOLTAGE);
    warning = true;
  }

  if (temperature > MAX_TEMPERATURE) {
    logFault(HIGH_TEMPERATURE);
    warning = true;
  }

  if (current > MAX_CURRENT) {
    logFault(HIGH_CURRENT);
    warning = true;
  }

  lastWarning = warning;

  // Count reports, not individual fault events
  if (warning) {
    warningReports++;
  }

  Serial.print("SYSTEM STATUS: ");
  Serial.println(warning ? "WARNING" : "NORMAL");

  Serial.print("Total reports: ");
  Serial.println(reportNumber);

  Serial.print("Reports with warnings: ");
  Serial.println(warningReports);

  showFaultLog();

  Serial.println("==========================");

  // Show the latest report immediately on the LCD
  lcdPage = 0;
  lastPageChange = millis();
  updateLCD(true);
}

void processInput(String text) {
  text.trim();

  if (text.length() == 0) return;

  char* endPtr;
  float value = strtof(text.c_str(), &endPtr);

  if (endPtr == text.c_str() ||
      *endPtr != '\0' ||
      !isfinite(value)) {
    Serial.println("Invalid number. Please try again.");
    showPrompt();
    return;
  }

  float minimum = 0.0;
  float maximum = 20.0;

  if (inputStage == 2) {
    minimum = -40.0;
    maximum = 125.0;
  }

  if (value < minimum || value > maximum) {
    Serial.println("Value outside permitted input range.");
    showPrompt();
    return;
  }

  if (inputStage == 0) {
    voltage = value;
    inputStage = 1;
    showPrompt();
    return;
  }

  if (inputStage == 1) {
    current = value;
    inputStage = 2;
    showPrompt();
    return;
  }

  temperature = value;
  reportNumber++;

  checkBattery();

  inputStage = 0;
  showPrompt();
}

void setup() {
  Serial.begin(115200);

  Wire.begin(SDA_PIN, SCL_PIN);

  lcd.init();
  lcd.backlight();
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("ESP32 Battery");
  lcd.setCursor(0, 1);
  lcd.print("Monitor Ready");

  Serial.println("INTERACTIVE ESP32 BATTERY MONITOR");
  Serial.println("LCD and report counters enabled.");
  Serial.println("Enter one value at a time.");

  delay(1000);

  voltage = 0;
  current = 0;
  temperature = 0;
  lastWarning = false;

  updateLCD(true);
  showPrompt();
}

void loop() {
  // Accept serial input without blocking
  while (Serial.available() > 0) {
    char c = (char)Serial.read();

    if (c == '\n' || c == '\r') {
      if (inputLine.length() > 0) {
        processInput(inputLine);
        inputLine = "";
      }
    } else if (c >= 32 && c <= 126) {
      if (inputLine.length() < 30) {
        inputLine += c;
      } else {
        inputLine = "";
        Serial.println(
          "Input too long. Please enter a number."
        );
        showPrompt();
      }
    }
  }

  // Alternate LCD pages every three seconds
  updateLCD();
}


