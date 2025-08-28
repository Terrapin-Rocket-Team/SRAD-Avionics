/* Teensy 4.1 “Quick FC” — matches rocket_tool/serial_link.py
   Protocol:
     HELLO\n  -> OK\n
     LIST\n   -> <id1,id2,...>\n
     GET <n>\n -> See envelope:
       BEGIN <n>\n
       FILE <name> <size>\n
       <raw bytes...>
       [repeat FILE blocks...]
       END\n
*/

#include <Arduino.h>
#include <SD.h>

// ---------- Config ----------
static const unsigned long STARTUP_WAIT_MS = 5000;  // wait for host
static const uint32_t BAUD = 115200;
static const size_t CMD_BUFSZ = 128;
static const size_t IO_BUFSZ  = 1024;  // file streaming chunk

// If your card is large/fast you can bump IO_BUFSZ (e.g., 4096)
static char cmdBuf[CMD_BUFSZ];

// ---------- Helpers ----------
static bool startsWith(const char* s, const char* pref) {
  while (*pref) {
    if (*s++ != *pref++) return false;
  }
  return true;
}

static void trimCRLF(char* s) {
  size_t n = strlen(s);
  while (n > 0 && (s[n-1] == '\r' || s[n-1] == '\n')) {
    s[--n] = '\0';
  }
}

static int parseIntAfter(const char* s, const char* after) {
  // e.g., s="GET 149", after="GET " -> returns 149 or -1 on failure
  const char* p = strstr(s, after);
  if (!p) return -1;
  p += strlen(after);
  while (*p == ' ') p++;
  if (!isdigit(*p) && *p != '-') return -1;
  return atoi(p);
}

// Extract leading integer before first underscore in a filename like "149_FlightData.csv"
static int idFromFilename(const char* name) {
  // Find first '_' and ensure all characters before are digits
  const char* us = strchr(name, '_');
  if (!us) return -1;
  int id = 0;
  const char* p = name;
  if (!isdigit(*p) && *p != '-') return -1;
  bool neg = false;
  if (*p == '-') { neg = true; p++; }
  if (!isdigit(*p)) return -1;
  while (p < us) {
    if (!isdigit(*p)) return -1;
    id = id * 10 + (*p - '0');
    p++;
  }
  return neg ? -id : id;
}

static void sendFile(const char* path, const char* printableName) {
  File f = SD.open(path, FILE_READ);
  if (!f) return;  // silently skip if missing

  // Header
  Serial.print("FILE ");
  Serial.print(printableName);
  Serial.print(" ");
  Serial.println((uint32_t)f.size());

  static uint8_t buf[IO_BUFSZ];
  while (true) {
    int n = f.read(buf, sizeof(buf));
    if (n <= 0) break;
    Serial.write(buf, n);
  }
  f.close();
}

// LIST: scan root for * _FlightData.csv, collect unique IDs, print CSV then \n
static void handleLIST() {
  // Teensy SD root iteration
  File dir = SD.open("/");
  if (!dir) { Serial.println(); return; }

  // Store up to 256 IDs (adjust if you expect more)
  const size_t MAX_IDS = 256;
  int ids[MAX_IDS];
  size_t count = 0;

  while (true) {
    File ent = dir.openNextFile();
    if (!ent) break;
    if (!ent.isDirectory()) {
      const char* nm = ent.name();  // short 8.3 or long name; Teensy returns long
      // We only consider files ending with _FlightData.csv
      const char* suffix = "_FlightData.csv";
      size_t ln = strlen(nm), ls = strlen(suffix);
      if (ln > ls && strcmp(nm + (ln - ls), suffix) == 0) {
        int id = idFromFilename(nm);
        if (id >= 0) {
          // de-dup
          bool seen = false;
          for (size_t i = 0; i < count; ++i) if (ids[i] == id) { seen = true; break; }
          if (!seen && count < MAX_IDS) {
            ids[count++] = id;
          }
        }
      }
    }
    ent.close();
  }
  dir.close();

  // Print CSV list
  for (size_t i = 0; i < count; ++i) {
    Serial.print(ids[i]);
    if (i + 1 < count) Serial.print(",");
  }
  Serial.println();
}

static void handleGET(int id) {
  if (id < 0) {
    Serial.println("END");  // no-op but terminate cleanly
    return;
  }
  Serial.print("BEGIN ");
  Serial.println(id);

  // Build file names expected on the SD root
  //  <id>_FlightData.csv
  //  <id>_Log.txt
  //  <id>_PreFlightData.csv
  char path[64];
  char name[64];

  snprintf(path, sizeof(path), "/%d_FlightData.csv", id);
  snprintf(name, sizeof(name), "%d_FlightData.csv", id);
  sendFile(path, name);

  snprintf(path, sizeof(path), "/%d_Log.txt", id);
  snprintf(name, sizeof(name), "%d_Log.txt", id);
  sendFile(path, name);

  snprintf(path, sizeof(path), "/%d_PreFlightData.csv", id);
  snprintf(name, sizeof(name), "%d_PreFlightData.csv", id);
  sendFile(path, name);

  Serial.println("END");
}

// ---------- Arduino ----------
void setup() {
  Serial.begin(BAUD);
  unsigned long t0 = millis();
  while (!Serial && (millis() - t0 < STARTUP_WAIT_MS)) {
    // wait briefly for host to open the port
  }

  // Teensy 4.1 built-in SD
  if (!SD.begin(BUILTIN_SDCARD)) {
    // If SD fails to mount, we still answer HELLO/LIST/GET (LIST empty, GET END)
  }
}

void loop() {
  // Read one line command (ending with '\n'), ignore empty lines
  if (Serial.available()) {
    size_t n = Serial.readBytesUntil('\n', cmdBuf, CMD_BUFSZ - 1);
    cmdBuf[n] = '\0';
    trimCRLF(cmdBuf);
    if (cmdBuf[0] == '\0') {
      return;
    }

    // Commands
    if (strcmp(cmdBuf, "HELLO") == 0) {
      Serial.println("OK");
      return;
    }

    if (strcmp(cmdBuf, "LIST") == 0) {
      handleLIST();
      return;
    }

    if (startsWith(cmdBuf, "GET")) {
      int id = parseIntAfter(cmdBuf, "GET");
      handleGET(id);
      return;
    }

    // Unknown -> minimal feedback line
    Serial.print("ERR unknown: ");
    Serial.println(cmdBuf);
  }
}
