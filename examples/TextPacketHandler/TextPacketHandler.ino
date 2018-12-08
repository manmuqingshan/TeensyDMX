/*
 * TextPacketHandler shows how to implement a Responder to make use of
 * Text Packets.
 *
 * (c) 2018 Shawn Silverman
 */

#include <TeensyDMX.h>
#include "TextPacketHandler.h"

namespace teensydmx = ::qindesign::teensydmx;

// Create the DMX receiver on Serial1.
teensydmx::Receiver dmxRx{Serial1};

// Holds a TextPacketHandler instance.
TextPacketHandler textPacketHandler;

void setup() {
  // Initialize the serial port
  Serial.begin(115200);
  while (!Serial && millis() < 4000) {
    // Wait for initialization to complete or a time limit
  }
  Serial.println("Starting.");

  // Set up the responder
  // In this case, the handler accepts several packet types
  // This is just for illustration purposes
  for (uint8_t sc : TextPacketHandler::kStartCodes) {
    dmxRx.addResponder(sc, &textPacketHandler);
  }

  dmxRx.begin();
}

void loop() {
}

// Receives a NUL-terminated string plus its length.
//
// If charsPerLine is zero then it should be ignored.
void setText(uint8_t page, uint8_t charsPerLine, const char *text, int len) {
  Serial.printf("Page %d:", page);

  // Print everything on one line if charsPerLine is zero
  if (charsPerLine == 0) {
    Serial.printf(" %s\n", text);
    return;
  }

  Serial.println();
  while (len >= charsPerLine) {
    Serial.printf("%.*s\n", charsPerLine, text);
    len -= charsPerLine;
    text = &text[charsPerLine];
  }
  if (len > 0) {
    Serial.printf("%s\n", text);
  }
}

// Receives a NUL-terminated UTF-8 string plus its length.
//
// If charsPerLine is zero then it should be ignored.
void setUTF8Text(uint8_t page, uint8_t charsPerLine,
                 const char *text, int len) {
  // TODO: Implement this, as an exercise to the reader
}