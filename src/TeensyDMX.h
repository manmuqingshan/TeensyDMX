/*
 * Library for doing DMX on a Teensy. Note that transmit and receive can't
 * be done on the same serial port.
 *
 * (c) 2017 Shawn Silverman
 */

/*
  Links:
  https://github.com/jimparis/DmxReceiver
  https://forum.pjrc.com/threads/19662-Arduinoesque-overriding-of-core-functionality?highlight=teensydmx
  https://www.holidaycoro.com/v/vspfiles/downloads/UnderstandingDMX.pdf
  https://www.pjrc.com/teensy/K20P64M72SF1RM.pdf
*/

#ifndef QINDESIGN_TEENSYDMX_H_
#define QINDESIGN_TEENSYDMX_H_

#include <Arduino.h>

// C++ includes
#include <cstdint>

namespace qindesign {
namespace teensydmx {

void uart0_rx_status_isr();
void uart0_rx_error_isr();
void uart0_tx_status_isr();

void uart1_rx_status_isr();
void uart1_rx_error_isr();
void uart1_tx_status_isr();

void uart2_rx_status_isr();
void uart2_rx_error_isr();
void uart2_tx_status_isr();

// The maximum size of a DMX packet, including the start code.
constexpr int kMaxDMXPacketSize = 513;

// The minimnum size of a DMX packet, including the start code.
constexpr int kMinDMXPacketSize = 25;

// TeensyDMX implements either a receiver or transmitter on one of
// hardware serial ports 1-3.
class TeensyDMX {
 public:
  // Creates a new DMX receiver or transmitter using the given hardware UART.
  TeensyDMX(HardwareSerial &uart)
      : uart_(uart),
        began_(false),
        packetCount_(0) {}

  // TeensyDMX is neither copyable nor movable.
  TeensyDMX(const TeensyDMX&) = delete;
  TeensyDMX& operator=(const TeensyDMX&) = delete;

  virtual ~TeensyDMX() = default;

  // Sets up the system for receiving or transmitting DMX on the specified
  // serial port.
  virtual void begin() = 0;

  // Tells the system to stop receiving or transmitting DMX. Call this
  // to clean up.
  virtual void end() = 0;

  // Returns the total number of packets received or transmitted since
  // the reciever was started.
  unsigned int packetCount() const {
    return packetCount_;
  }

 protected:
  // Returns the index given a serial port, or -1 if the serial port is
  // not supported.
  static int serialIndex(HardwareSerial &uart) {
    if (&uart == &Serial1) {
      return 0;
    }
    if (&uart == &Serial2) {
      return 1;
    }
    if (&uart == &Serial3) {
      return 2;
    }
    return -1;
  }

  HardwareSerial &uart_;  // TODO(shawn): Should this be volatile?

  // Tracks whether the system has been configured. Subclasses must manage
  // this state.
  bool began_;

  // The number of packets sent or received. Subclasses must manage this.
  volatile unsigned int packetCount_;
};

// ---------------------------------------------------------------------------
//  Receiver
// ---------------------------------------------------------------------------

// A DMX receiver.
class Receiver final : public TeensyDMX {
 public:
  Receiver(HardwareSerial &uart)
      : TeensyDMX(uart),
        buf1_{0},
        buf2_{0},
        activeBuf_(buf1_),
        inactiveBuf_(buf2_),
        activeBufIndex_(0),
        packetSize_(0),
        first_(true) {}

  // Destructs Receiver. This calls end().
  ~Receiver() override {
    end();
  }

  void begin() override;

  void end() override;

  // Reads all or part of a packet into buf. This does nothing if len
  // is negative or zero, or if startChannel is negative or beyond
  // kMaxDMXPacketSize, and only reads up to the end of the packet
  // if startChannel + len would go past the end.
  //
  // This will return the number of bytes actually read into buf, -1 if there
  // is no packet available since the last call to this function, or zero if
  // len is negative or zero, or if the requested data is outside the range
  // of the recieved packet. Differentiating between -1 and zero allows the
  // caller to determine whether there was no packet received or a packet
  // was received and did not contain the requested data.
  //
  // The values starting at startChannel will be stored starting at index
  // zero in buf. buf must have a size of at least len bytes.
  int readPacket(uint8_t *buf, int startChannel, int len);

  // Gets the value for one channel. The start code can be read at
  // channel zero.
  //
  // If the channel is out of range then this will return zero.
  uint8_t get(int channel) const {
    if (channel <= 0 || kMaxDMXPacketSize <= channel) {
      return 0;
    }
    return inactiveBuf_[channel];
  }

 private:
  // Fills the buffer from the UART and then completes the packet from
  // immediately before the break. This reads up to a maximum of
  // kMaxDMXPacketSize bytes and ignores anything after that until
  // the next break.
  //
  // This will be called from an ISR.
  void completePacket();

  // Resets the packet on a framing error.
  // This will be called from an ISR.
  void resetPacket();

  // Receives a byte.
  // This will be called from an ISR.
  void receiveByte(uint8_t b);

  uint8_t buf1_[kMaxDMXPacketSize];
  uint8_t buf2_[kMaxDMXPacketSize];
  uint8_t *activeBuf_;
  volatile uint8_t *inactiveBuf_;
  int activeBufIndex_;

  // The size of the last received packet.
  volatile int packetSize_;

  // The current read technique is to fill the buffer after a break is
  // detected, but the break indicates a packet start, not a packet end.
  // Therefore, we're always one behind, and so the first break must not
  // cause a valid packet collection.
  bool first_;

  // These error ISR's need to access private functions
  friend void uart0_rx_status_isr();
  friend void uart0_rx_error_isr();
  friend void uart1_rx_status_isr();
  friend void uart1_rx_error_isr();
  friend void uart2_rx_status_isr();
  friend void uart2_rx_error_isr();
};

// ---------------------------------------------------------------------------
//  Sender
// ---------------------------------------------------------------------------

// A DMX transmitter.
class Sender final : public TeensyDMX {
 public:
  Sender(HardwareSerial &uart)
      : TeensyDMX(uart),
        state_(XmitStates::kIdle),
        outputBuf_{0},
        outputBufIndex_(0),
        packetSize_(kMaxDMXPacketSize) {}

  // Destructs Sender. This calls end().
  ~Sender() override {
    end();
  }

  void begin() override;

  void end() override;

  // Sets the transmit packet size, in number of channels plus the start code.
  // This does nothing if the size is outside the range,
  // [kMinDMXPacketSize, kMaxDMXPacketSize].
  //
  // The default is kMaxDMXPacketSize.
  void setPacketSize(int size) {
    if (kMinDMXPacketSize <= size && size <= kMaxDMXPacketSize) {
      packetSize_ = size;
    }
  }

  // Sets a channel's value. Channel zero represents the start code.
  // The start code should really be zero, but it can be changed here.
  //
  // If the channel is out of range then the call is ignored.
  void set(int channel, uint8_t value) {
    if (0 <= channel && channel < kMaxDMXPacketSize) {
      outputBuf_[channel] = value;
    }
  }

  // Sets the values for a range of channels.
  //
  // This does nothing if any part of the channel range is not in the
  // range [0, kMaxDMXPacketSize-1].
  void set(int startChannel, const uint8_t *values, int len);

 private:
   // State that tracks what to transmit and when.
   enum XmitStates {
     kBreak,  // Need to transmit a break
     kData,   // Need to transmit data
     kIdle,   // The end of data for one packet has been reached
   };

  // Completes a sent packet. This increments the packet count, resets the
  // output buffer index, and sets the state to Idle.
  //
  // This will be called from an ISR.
  void completePacket();

  // Keeps track of what we're transmitting.
  volatile XmitStates state_;

  volatile uint8_t outputBuf_[kMaxDMXPacketSize];
  int outputBufIndex_;

  // The size of the packet to be sent.
  volatile int packetSize_;

  // These error ISR's need to access private functions
  friend void uart0_tx_status_isr();
  friend void uart1_tx_status_isr();
  friend void uart2_tx_status_isr();
};

}  // namespace teensydmx
}  // namespace qindesign

#endif  // QINDESIGN_TEENSYDMX_H_
