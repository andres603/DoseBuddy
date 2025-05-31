#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>
#include <WiFiS3.h>
#include <AccelStepper.h>

// WiFi credentials
char ssid[] = "user";        // Replace with your network SSID
char pass[] = "pass";    // Replace with your network password
int status = WL_IDLE_STATUS;
WiFiServer server(80);
// String currentStatus = ""; // Remove initial empty declaration, see definition below

// LCD setup
LiquidCrystal_I2C lcd(0x27, 16, 2);
RTC_DS3231 rtc;

// Stepper motor setup
#define motorPin1  8
#define motorPin2  9
#define motorPin3  10
#define motorPin4  11
#define MotorInterfaceType 8
AccelStepper stepper = AccelStepper(MotorInterfaceType, motorPin1, motorPin3, motorPin2, motorPin4);

// Time slots and hardware pins
int slotHours[6] = {0, 0, 0, 0, 0, 0};
int slotMinutes[6] = {0, 0, 0, 0, 0, 0};
int slotSeconds[6] = {0, 0, 0, 0, 0, 0};
const int led = 2;
const int button = 5;

// Button/LED states
int buttonState = 0;
unsigned long ledOnTime = 0; // 0 means LED sequence is inactive
bool ledSteady = false;
unsigned long blinkInterval = 500;
unsigned long lastBlinkTime = 0;
bool ledState = LOW;

// New variable for "Pills Taken" status persistence
unsigned long pillsTakenStartTime = 0; // 0 means state is not active or duration passed

typedef struct minMax_t {
  int minimum;
  int maximum;
};

// --- Define Status Constants ---
const String PILL_READY_STATUS = "Pills Ready";
const String IDLE_STATUS = "Idle";
const String PILLS_TAKEN_STATUS = "Pills Taken";
const String WIFI_ERROR_STATUS = "WiFi Error";
const String OVERRIDE_READY_STATUS = "Override Ready";

// --- Global variable holding the device's current state description ---
String currentStatus = IDLE_STATUS;

// (sendHTML function remains identical)
void sendHTML(WiFiClient client, const String& message = "", bool isError = false) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-type:text/html");
    client.println("Connection: close");
    client.println();

    client.println("<!DOCTYPE html>");
    client.println("<html lang='en'>");
    client.println("<head>");
    client.println("<meta charset='UTF-8'>");
    client.println("<meta name='viewport' content='width=device-width, initial-scale=1.0'>");
    client.println("<title>DoseBuddy Pill Dispenser</title>");
    client.println("<style>");
    // --- Root CSS Variables ---
    client.println("  :root {");
    client.println("    --bg-color: #ffffff;");
    client.println("    --card-bg: #f8f9fa;");
    client.println("    --text-color: #343a40;");
    client.println("    --muted-text-color: #6c757d;");
    client.println("    --border-color: #e9ecef;");
    client.println("    --primary-color: #6c757d;");
    client.println("    --primary-text: #ffffff;");
    client.println("    --input-bg: #ffffff;");
    client.println("    --success-bg: #d4edda;");
    client.println("    --success-border: #c3e6cb;");
    client.println("    --success-text: #155724;");
    client.println("    --error-bg: #f8d7da;");
    client.println("    --error-border: #f5c6cb;");
    client.println("    --error-text: #721c24;");
    client.println("  }");
    // --- Styles --- (unchnaged)
    client.println("  body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Helvetica Neue', Arial, sans-serif; margin: 0; padding: 15px; background-color: var(--bg-color); color: var(--text-color); line-height: 1.5; font-size: 15px; }");
    client.println("  .container { max-width: 650px; margin: 15px auto; padding: 20px; background-color: var(--card-bg); border-radius: 6px; border: 1px solid var(--border-color); }");
    client.println("  h1, h2 { color: var(--text-color); margin-top: 0; margin-bottom: 0.8em; text-align: center; font-weight: 500; }");
    client.println("  h1 { font-size: 1.6em; margin-bottom: 0.6em; }");
    client.println("  h2 { font-size: 1.3em; margin-top: 1.5em; border-top: 1px solid var(--border-color); padding-top: 1em; text-align: left; }");
    client.println("  p.instructions { font-size: 0.9em; text-align: center; color: var(--muted-text-color); margin-bottom: 1.5em; }");
    client.println("  form { display: flex; flex-direction: column; gap: 15px; }");
    client.println("  .slots-container { display: grid; grid-template-columns: repeat(auto-fit, minmax(260px, 1fr)); gap: 15px; }");
    client.println("  .slot { padding: 12px; border: 1px solid var(--border-color); border-radius: 4px; background-color: var(--bg-color); }");
    client.println("  .slot strong { display: block; margin-bottom: 8px; color: var(--text-color); font-weight: 500; display: flex; align-items: center; }");
    client.println("  .slot strong .icon { margin-right: 8px; font-style: normal; font-size: 1.1em; line-height: 1; }");
    client.println("  .time-inputs { display: flex; gap: 8px; align-items: center; flex-wrap: wrap; }");
    client.println("  .input-group { display: flex; flex-direction: column; flex-grow: 1; }");
    client.println("  label { font-size: 0.8em; margin-bottom: 2px; color: var(--muted-text-color); }");
    client.println("  input[type='number'] { width: 100%; box-sizing: border-box; padding: 6px 8px; border: 1px solid var(--border-color); border-radius: 3px; background-color: var(--input-bg); text-align: center; font-size: 0.9em; -moz-appearance: textfield; }");
    client.println("  input[type='number']::-webkit-outer-spin-button, input[type='number']::-webkit-inner-spin-button { -webkit-appearance: none; margin: 0; }");
    client.println("  input[type='submit'] { padding: 10px 20px; background-color: var(--primary-color); color: var(--primary-text); border: none; border-radius: 4px; cursor: pointer; font-size: 0.95em; transition: background-color 0.2s ease; align-self: center; margin-top: 8px; }");
    client.println("  input[type='submit']:hover { background-color: #5a6268; }");
    client.println("  .message-area { padding: 12px; margin-bottom: 15px; border: 1px solid transparent; border-radius: 4px; text-align: center; font-weight: bold; }");
    client.println("  .message-area.success { color: var(--success-text); background-color: var(--success-bg); border-color: var(--success-border); }");
    client.println("  .message-area.error { color: var(--error-text); background-color: var(--error-bg); border-color: var(--error-border); }");
    client.println("  footer { margin-top: 25px; padding-top: 15px; border-top: 1px solid var(--border-color); text-align: center; font-size: 0.85em; color: var(--muted-text-color); }");
    client.println("  footer p { margin: 5px 0; }");
    client.println("</style>");
    client.println("</head>");
    client.println("<body>");
    client.println("<div class='container'>");

    client.println("<h1>DoseBuddy Pill Dispenser</h1>");
    client.println("<p class='instructions'>Welcome to your DoseBuddy scheduler! Use this page to track your medication times for each slot below.</p>");

    if (message != "") {
        client.print("<div class='message-area ");
        client.print(isError ? "error" : "success");
        client.print("'>");
        client.print(message);
        client.println("</div>");
    }

    client.println("<form method='GET' action='/'>");
    client.println("<div class='slots-container'>");

    const char* pill_icon_entity = "💊";

    for (int i = 0; i < 6; i++) {
        client.print("<div class='slot'><strong><span class='icon'>");
        client.print(pill_icon_entity);
        client.print("</span>Slot ");
        client.print(i + 1);
        client.println("</strong>");
        client.println("<div class='time-inputs'>");
        client.print("<div class='input-group'><label for='slot"); client.print(i); client.print("h'>Hrs (0-23)</label><input type='number' name='slot"); client.print(i); client.print("h' id='slot"); client.print(i); client.print("h' value='"); client.print(slotHours[i]); client.print("' min='0' max='23' step='1' required></div>");
        client.print("<div class='input-group'><label for='slot"); client.print(i); client.print("m'>Min (0-59)</label><input type='number' name='slot"); client.print(i); client.print("m' id='slot"); client.print(i); client.print("m' value='"); client.print(slotMinutes[i]); client.print("' min='0' max='59' step='1' required></div>");
        client.print("<div class='input-group'><label for='slot"); client.print(i); client.print("s'>Sec (0-59)</label><input type='number' name='slot"); client.print(i); client.print("s' id='slot"); client.print(i); client.print("s' value='"); client.print(slotSeconds[i]); client.print("' min='0' max='59' step='1' required></div>");
        client.println("</div>"); // Close time-inputs
        client.println("</div>"); // Close slot
    }

    client.println("</div>"); // Close slots-container
    client.println("<input type='submit' value='Update Schedule'>");
    client.println("</form>");

    client.print("<h2>Current Status: ");
    client.print(currentStatus);
    client.println("</h2>");

    client.println("<footer>");
    client.println("<p>By <a href=\"https://melvinmokhtari.com/\">Melvin Mokhtari</a>, <a href=\"#_REPLACE_WITH_MADELINE_URL\">Madeline Rao</a>, <a href=\"#_REPLACE_WITH_NNAMDI_URL\">Nnamdi Nnake</a>, & <a href=\"#_REPLACE_WITH_EDWARD_URL\">Edward Chu</a></p>");
    client.println("<p>Contact: <a href=\"mailto:melvim1@mcmaster.ca\">melvim1@mcmaster.ca</a></p>");
    client.println("<p>© 2025 DoseBuddy</p>");
    client.println("</footer>");

    client.println("</div>"); // Close container

    client.println("<script>");
    client.print("  const serverStatus = '"); client.print(currentStatus); client.println("';");
    client.print("  const alertTriggerStatus = '"); client.print(PILL_READY_STATUS); client.println("';");
    client.println("  if (serverStatus === alertTriggerStatus) {");
    client.println("    alert('Reminder: Your pills are ready. Please take your medication.');");
    client.println("  }");
    client.println("</script>");

    client.println("</body></html>");
}


// (updateSlotsFromParams remains identical)
void updateSlotsFromParams(String params) {
  int fromIndex = 0;
  while (fromIndex < params.length()) {
    int ampPos = params.indexOf('&', fromIndex);
    String param = ampPos == -1 ? params.substring(fromIndex) : params.substring(fromIndex, ampPos);
    int equalPos = param.indexOf('=');
    if (equalPos != -1) {
      String key = param.substring(0, equalPos);
      String value = param.substring(equalPos + 1);

      if (key.startsWith("slot")) {
        int slotIndex = key.charAt(4) - '0';
        char type = key.charAt(5);
        int val = value.toInt();

        if (slotIndex >= 0 && slotIndex < 6) {
          if (type == 'h' && val >= 0 && val <= 23) slotHours[slotIndex] = val;
          else if (type == 'm' && val >= 0 && val <= 59) slotMinutes[slotIndex] = val;
          else if (type == 's' && val >= 0 && val <= 59) slotSeconds[slotIndex] = val;
        }
      }
    }
    fromIndex = ampPos == -1 ? params.length() : ampPos + 1;
  }
}

// (checkInput, updateRTC remain identical)
bool checkInput(const int value, const minMax_t minMax) {
  if ((value >= minMax.minimum) && (value <= minMax.maximum)) return true;
  Serial.print(value); Serial.print(" is out of range "); Serial.print(minMax.minimum); Serial.print(" - "); Serial.println(minMax.maximum);
  return false;
}

void updateRTC() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Edit Mode...");
  const char txt[6][15] = { "year [4-digit]", "month [1~12]", "day [1~31]", "hours [0~23]", "minutes [0~59]", "seconds [0~59]" };
  const minMax_t minMax[] = { {2000, 9999}, {1, 12}, {1, 31}, {0, 23}, {0, 59}, {0, 59} };
  String str = "";
  long newDate[6];
  DateTime newDateTime;
  while (1) {
    while (Serial.available()) Serial.read();
    for (int i = 0; i < 6; i++) {
      while (1) {
        Serial.print("Enter "); Serial.print(txt[i]); Serial.print(" (or -1 to abort) : ");
        while (!Serial.available()) { ; }
        str = Serial.readString();
        if ((str == "-1") || (str == "-1\n") || (str == "-1\r") || (str == "-1\r\n")) {
          Serial.println("\nABORTED");
          lcd.clear(); return;
        }
        newDate[i] = str.toInt();
        if (checkInput(newDate[i], minMax[i])) break;
      }
      Serial.println(newDate[i]);
    }
    newDateTime = DateTime(newDate[0], newDate[1], newDate[2], newDate[3], newDate[4], newDate[5]);
    if (newDateTime.isValid()) break;
    Serial.println("Date/time entered was invalid, please try again.");
  }
  rtc.adjust(newDateTime);
  Serial.println("RTC Updated!");
  lcd.clear();
}

// --- MINIMAL CHANGE: updateLCD function ---
// Remove the conditional update based on time change to force update every loop.
// Use "HH" for 24hr format.
// Add padding to ensure line clearing.
void updateLCD() {
  //static DateTime lastDisplayedTime; // Removed this tracker
  DateTime rtcTime = rtc.now();

  char dateBuffer[] = "DD-MMM-YYYY DDD";
  //char timeBuffer[] = "hh:mm:ss AP"; //12 hour with AP
  char timeBuffer[] = "hh:mm:ss"; //24 hour

  // Always update Line 0 in each loop iteration
  //if (lastDisplayedTime.unixtime() != rtcTime.unixtime()) { // Removed condition

  lcd.setCursor(0, 0);
  lcd.print("Time: ");
  lcd.setCursor(5, 0);
  lcd.print(rtcTime.toString(timeBuffer));
}


void setup() {
  stepper.setMaxSpeed(1000);
  stepper.setAcceleration(500);

  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  pinMode(led, OUTPUT);
  pinMode(button, INPUT_PULLUP);

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    lcd.print("RTC Error!");
    while (1);
  }
  if (rtc.lostPower()) {
    Serial.println("RTC lost power, setting time...");
    lcd.clear();
    lcd.print("RTC Power Loss");
    delay(1000);
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  lcd.clear();
  lcd.print("Connecting WiFi");
  Serial.print("Connecting to "); Serial.println(ssid);
  status = WiFi.begin(ssid, pass);
  int attempts = 0;
  while (status != WL_CONNECTED && attempts < 20) {
      delay(500); Serial.print(".");
      lcd.setCursor(attempts % 16, 1); lcd.print(".");
      status = WiFi.status(); attempts++;
  }

  lcd.clear();
  if (WiFi.status() == WL_CONNECTED) {
    server.begin();
    Serial.println("Connected!"); Serial.print("IP: "); Serial.println(WiFi.localIP());
    lcd.setCursor(0, 0); lcd.print("DoseBuddy Online");
    lcd.setCursor(0, 1); lcd.print(WiFi.localIP());
    delay(5000);
  } else {
    Serial.println("WiFi Failed");
    lcd.setCursor(0, 0); lcd.print("WiFi Failed!");
    lcd.setCursor(0, 1); lcd.print("                ");
    currentStatus = WIFI_ERROR_STATUS;
    delay(3000);
  }

  lcd.clear();
  currentStatus = IDLE_STATUS;
  updateLCD(); // Initial time display
  lcd.setCursor(0, 1); // Initial status display
  lcd.print("Status: Idle    ");
}


void loop() {
  DateTime rtcTime = rtc.now();
  buttonState = digitalRead(button);
  updateLCD(); // **** THIS NOW RUNS EVERY LOOP, UPDATING LINE 0 ****

  // --- Status Management & Persistence ---
  if (currentStatus == PILLS_TAKEN_STATUS && pillsTakenStartTime != 0) {
    if (millis() - pillsTakenStartTime >= 60000) {
        currentStatus = IDLE_STATUS;
        pillsTakenStartTime = 0;
        lcd.setCursor(0, 1);
        lcd.print("Status: Idle    ");
    }
  }
  else if (ledOnTime == 0 && currentStatus == PILL_READY_STATUS) {
      // If LED sequence stopped but status is still Ready, means it timed out or button wasn't handled?
      // Change to IDLE. Button press now handles transition to PILLS_TAKEN.
      currentStatus = IDLE_STATUS;
      lcd.setCursor(0, 1);
      lcd.print("Status: Idle    ");
  }

  // --- LED Control Logic (Unchanged) ---
  if (ledSteady) {
    if (millis() - ledOnTime >= 60000) {
      ledSteady = false; lastBlinkTime = millis(); ledState = HIGH; digitalWrite(led, ledState);
    } else { digitalWrite(led, HIGH); }
  } else if (ledOnTime != 0) {
    if (millis() - lastBlinkTime >= blinkInterval) {
      lastBlinkTime = millis(); ledState = !ledState; digitalWrite(led, ledState);
    }
  } else {
    digitalWrite(led, LOW); ledState = LOW;
  }

  // --- Check Time Slots ---
  static unsigned long lastSlotCheckSecond = 0;
  if (rtcTime.second() != lastSlotCheckSecond) {
      lastSlotCheckSecond = rtcTime.second();
      for (int slot = 0; slot < 6; slot++) {
          if (rtcTime.hour() == slotHours[slot] &&
              rtcTime.minute() == slotMinutes[slot] &&
              rtcTime.second() == slotSeconds[slot]) {

              if (currentStatus == IDLE_STATUS && ledOnTime == 0) {
                  // --- MINIMAL CHANGE: Remove temporary message on line 0 ---
                  // It was conflicting with constant time updates.
                  // Rely on Line 1 status from rotatePills().
                  /*
                  lcd.setCursor(0, 0);
                  lcd.print("Slot "); lcd.print(slot + 1); lcd.print(" Triggered");
                  delay(1500);
                  */
                  rotatePills();
                  break;
              }
          }
      }
  }

  // --- Button Handling (Unchanged) ---
  static int lastButtonState = HIGH;
  if (buttonState == LOW && lastButtonState == HIGH) {
    if (ledOnTime != 0) {
      lcd.setCursor(0, 1); lcd.print("Status: PillsTkn");
      digitalWrite(led, LOW);
      currentStatus = PILLS_TAKEN_STATUS;
      ledOnTime = 0; ledSteady = false; ledState = LOW; lastBlinkTime = 0;
      pillsTakenStartTime = millis();
      delay(50);
    }
  }
  lastButtonState = buttonState;

  // --- Serial Input Handling ---
  if (Serial.available()) {
    char input = Serial.read();
    if (input == 'u') {
        updateRTC(); // Clears LCD itself
        // updateLCD() will run at start of next loop to show time
        // Update line 1 status immediately
        lcd.setCursor(0, 1);
        if (currentStatus == IDLE_STATUS) { lcd.print("Status: Idle    "); }
        else if (currentStatus == PILLS_TAKEN_STATUS) { lcd.print("Status: PillsTkn"); }
        else if (currentStatus == PILL_READY_STATUS) { lcd.print("Status:PillsRdy!"); }
        else { lcd.print("Status: ???     ");} // Handle other potential states if needed
    }
  }

  // --- Web Server Handling (Unchanged) ---
  WiFiClient client = server.available();
  if (client) {
    String currentLine = ""; String requestPath = ""; bool isGetRequest = false;
    String feedbackMessage = ""; bool isFeedbackError = false;
    unsigned long clientStartTime = millis();
    while (client.connected() && (millis() - clientStartTime < 2000)) {
      if (client.available()) {
        char c = client.read();
        if (c == '\n') {
          if (currentLine.startsWith("GET /")) {
            isGetRequest = true;
            int firstSpace = currentLine.indexOf(' '); int secondSpace = currentLine.indexOf(' ', firstSpace + 1);
            if (firstSpace != -1 && secondSpace != -1) {
              requestPath = currentLine.substring(firstSpace + 1, secondSpace);
              int qMark = requestPath.indexOf('?');
              if (qMark != -1) {
                String params = requestPath.substring(qMark + 1);
                updateSlotsFromParams(params);
                feedbackMessage = "Schedule updated!"; isFeedbackError = false;
              } } }
          if (currentLine.length() == 0 && isGetRequest) { sendHTML(client, feedbackMessage, isFeedbackError); break; }
          else if (currentLine.length() == 0 && !isGetRequest) { break; }
          currentLine = "";
        } else if (c != '\r') { currentLine += c; }
        clientStartTime = millis();
      } }
    client.stop();
  }

} // End loop()


// --- MINIMAL CHANGE: Rotate Pills function ---
// Ensure status message on Line 1 is consistent
void rotatePills() {
  stepper.enableOutputs();
  stepper.setCurrentPosition(0);
  stepper.moveTo(4096);
  stepper.setMaxSpeed(1000);
  stepper.setAcceleration(500);
  while (stepper.distanceToGo() != 0) { stepper.run(); }
  stepper.disableOutputs();

  lcd.setCursor(0, 1); // Write status on the second line
  lcd.print("Status:PillsRdy!"); // Keep shortened status
  currentStatus = PILL_READY_STATUS;

  digitalWrite(led, HIGH); ledOnTime = millis(); ledSteady = true;
  lastBlinkTime = 0; ledState = HIGH;
}

// (rotateOver function remains unchanged)
void rotateOver() {
  stepper.enableOutputs();
  stepper.setCurrentPosition(0);
  stepper.moveTo(4096);
  stepper.setMaxSpeed(1000);
  stepper.setAcceleration(500);
  while(stepper.distanceToGo() != 0) { stepper.run(); }
  stepper.disableOutputs();

  lcd.setCursor(0, 1); lcd.print("Status: Override");
  digitalWrite(led, HIGH);
  // currentStatus = OVERRIDE_READY_STATUS; // Example
  ledOnTime = millis(); ledSteady = true;
}
