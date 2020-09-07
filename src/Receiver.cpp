// This file is part of the TeensyDMX library.
// (c) 2017-2020 Shawn Silverman

#include "TeensyDMX.h"

// C++ includes
#include <algorithm>
#include <utility>

// Project includes
#include "Responder.h"
#include "uart_routine_defines.h"

namespace qindesign {
namespace teensydmx {

constexpr uint32_t kSlotsBaud    = 250000;                // 4us
constexpr uint32_t kSlotsFormat  = SERIAL_8N2;            // 9:2
constexpr uint32_t kBitTime      = 1000000 / kSlotsBaud;  // In microseconds
constexpr uint32_t kCharTime     = 11 * kBitTime;         // In microseconds
constexpr uint32_t kMinBreakTime = 88;                    // In microseconds
constexpr uint32_t kMinMABTime   = 8;                     // In microseconds

// Routines:
// 1. RX ISR routines, and
// 2. Routines that do raw transmit.
//    These don't affect the transmitter.
#if defined(HAS_KINETISK_UART0) || defined(HAS_KINETISL_UART0)
void uart0_rx_isr();
void uart0_tx(const uint8_t *b, int len);
void uart0_tx_break(uint32_t breakTime, uint32_t mabTime);
#endif  // HAS_KINETISK_UART0 || HAS_KINETISL_UART0

#if defined(HAS_KINETISK_UART1) || defined(HAS_KINETISL_UART1)
void uart1_rx_isr();
void uart1_tx(const uint8_t *b, int len);
void uart1_tx_break(uint32_t breakTime, uint32_t mabTime);
#endif  // HAS_KINETISK_UART1 || HAS_KINETISL_UART1

#if defined(HAS_KINETISK_UART2) || defined(HAS_KINETISL_UART2)
void uart2_rx_isr();
void uart2_tx(const uint8_t *b, int len);
void uart2_tx_break(uint32_t breakTime, uint32_t mabTime);
#endif  // HAS_KINETISK_UART2 || HAS_KINETISL_UART2

#if defined(HAS_KINETISK_UART3)
void uart3_rx_isr();
void uart3_tx(const uint8_t *b, int len);
void uart3_tx_break(uint32_t breakTime, uint32_t mabTime);
#endif  // HAS_KINETISK_UART3

#if defined(HAS_KINETISK_UART4)
void uart4_rx_isr();
void uart4_tx(const uint8_t *b, int len);
void uart4_tx_break(uint32_t breakTime, uint32_t mabTime);
#endif  // HAS_KINETISK_UART4

#if defined(HAS_KINETISK_UART5)
void uart5_rx_isr();
void uart5_tx(const uint8_t *b, int len);
void uart5_tx_break(uint32_t breakTime, uint32_t mabTime);
#endif  // HAS_KINETISK_UART5

#if defined(HAS_KINETISK_LPUART0)
void lpuart0_rx_isr();
void lpuart0_tx(const uint8_t *b, int len);
void lpuart0_tx_break(uint32_t breakTime, uint32_t mabTime);
#endif  // HAS_KINETISK_LPUART0

#if defined(IMXRT_LPUART6)
void lpuart6_rx_isr();
void lpuart6_tx(const uint8_t *b, int len);
void lpuart6_tx_break(uint32_t breakTime, uint32_t mabTime);
#endif  // IMXRT_LPUART6

#if defined(IMXRT_LPUART4)
void lpuart4_rx_isr();
void lpuart4_tx(const uint8_t *b, int len);
void lpuart4_tx_break(uint32_t breakTime, uint32_t mabTime);
#endif  // IMXRT_LPUART4

#if defined(IMXRT_LPUART2)
void lpuart2_rx_isr();
void lpuart2_tx(const uint8_t *b, int len);
void lpuart2_tx_break(uint32_t breakTime, uint32_t mabTime);
#endif  // IMXRT_LPUART2

#if defined(IMXRT_LPUART3)
void lpuart3_rx_isr();
void lpuart3_tx(const uint8_t *b, int len);
void lpuart3_tx_break(uint32_t breakTime, uint32_t mabTime);
#endif  // IMXRT_LPUART3

#if defined(IMXRT_LPUART8)
void lpuart8_rx_isr();
void lpuart8_tx(const uint8_t *b, int len);
void lpuart8_tx_break(uint32_t breakTime, uint32_t mabTime);
#endif  // IMXRT_LPUART8

#if defined(IMXRT_LPUART1)
void lpuart1_rx_isr();
void lpuart1_tx(const uint8_t *b, int len);
void lpuart1_tx_break(uint32_t breakTime, uint32_t mabTime);
#endif  // IMXRT_LPUART1

#if defined(IMXRT_LPUART7)
void lpuart7_rx_isr();
void lpuart7_tx(const uint8_t *b, int len);
void lpuart7_tx_break(uint32_t breakTime, uint32_t mabTime);
#endif  // IMXRT_LPUART7

#if defined(IMXRT_LPUART5) && \
    (defined(__IMXRT1052__) || defined(ARDUINO_TEENSY41))
void lpuart5_rx_isr();
void lpuart5_tx(const uint8_t *b, int len);
void lpuart5_tx_break(uint32_t breakTime, uint32_t mabTime);
#endif  // IMXRT_LPUART5 && (__IMXRT1052__ || ARDUINO_TEENSY41)

// Used by the RX ISRs.
#if defined(__IMXRT1052__) || defined(ARDUINO_TEENSY41)
static Receiver *volatile rxInstances[8]{nullptr};
#else
static Receiver *volatile rxInstances[7]{nullptr};
#endif  // __IMXRT1052__ || ARDUINO_TEENSY41

// Forward declarations of RX watch pin ISRs
void rxPinRoseSerial0_isr();
void rxPinRoseSerial1_isr();
void rxPinRoseSerial2_isr();
void rxPinRoseSerial3_isr();
void rxPinRoseSerial4_isr();
void rxPinRoseSerial5_isr();
void rxPinRoseSerial6_isr();
#if defined(__IMXRT1052__) || defined(ARDUINO_TEENSY41)
void rxPinRoseSerial7_isr();
#endif  // __IMXRT1052__ || ARDUINO_TEENSY41

#if defined(__IMXRT1052__) || defined(ARDUINO_TEENSY41)
// RX watch pin rose ISRs.
static void (*rxPinRoseISRs[8])() {
    rxPinRoseSerial0_isr,
    rxPinRoseSerial1_isr,
    rxPinRoseSerial2_isr,
    rxPinRoseSerial3_isr,
    rxPinRoseSerial4_isr,
    rxPinRoseSerial5_isr,
    rxPinRoseSerial6_isr,
    rxPinRoseSerial7_isr,
};
#else
// RX watch pin rose ISRs.
static void (*rxPinRoseISRs[7])() {
    rxPinRoseSerial0_isr,
    rxPinRoseSerial1_isr,
    rxPinRoseSerial2_isr,
    rxPinRoseSerial3_isr,
    rxPinRoseSerial4_isr,
    rxPinRoseSerial5_isr,
    rxPinRoseSerial6_isr,
};
#endif  // __IMXRT1052__ || ARDUINO_TEENSY41

Receiver::Receiver(HardwareSerial &uart)
    : TeensyDMX(uart),
      txEnabled_(true),
      began_(false),
      state_{RecvStates::kIdle},
      keepShortPackets_(false),
      buf1_{0},
      buf2_{0},
      activeBuf_(buf1_),
      inactiveBuf_(buf2_),
      activeBufIndex_(0),
      packetSize_(0),
      packetStats_{},
      lastBreakStartTime_(0),
      breakStartTime_(0),
      lastSlotEndTime_(0),
      connected_(false),
      connectChangeFunc_{nullptr},
      errorStats_{},
      responderCount_(0),
      responderOutBufLen_(0),
      setTXNotRXFunc_(nullptr),
      rxWatchPin_(-1),
      seenMABStart_(false),
      mabStartTime_(0),
      txFunc_(nullptr),
      txBreakFunc_(nullptr) {
  switch(serialIndex_) {
#if defined(HAS_KINETISK_UART0) || defined(HAS_KINETISL_UART0)
    case 0:
      txFunc_ = uart0_tx;
      txBreakFunc_ = uart0_tx_break;
      break;
#elif defined(IMXRT_LPUART6)
    case 0:
      txFunc_ = lpuart6_tx;
      txBreakFunc_ = lpuart6_tx_break;
      break;
#endif  // HAS_KINETISK_UART0 || HAS_KINETISL_UART0 || IMXRT_LPUART6

#if defined(HAS_KINETISK_UART1) || defined(HAS_KINETISL_UART1)
    case 1:
      txFunc_ = uart1_tx;
      txBreakFunc_ = uart1_tx_break;
      break;
#elif defined(IMXRT_LPUART4)
    case 1:
      txFunc_ = lpuart4_tx;
      txBreakFunc_ = lpuart4_tx_break;
      break;
#endif  // HAS_KINETISK_UART1 || HAS_KINETISL_UART1 || IMXRT_LPUART4

#if defined(HAS_KINETISK_UART2) || defined(HAS_KINETISL_UART2)
    case 2:
      txFunc_ = uart2_tx;
      txBreakFunc_ = uart2_tx_break;
      break;
#elif defined(IMXRT_LPUART2)
    case 2:
      txFunc_ = lpuart2_tx;
      txBreakFunc_ = lpuart2_tx_break;
      break;
#endif  // HAS_KINETISK_UART2 || HAS_KINETISL_UART2 || IMXRT_LPUART2

#if defined(HAS_KINETISK_UART3)
    case 3:
      txFunc_ = uart3_tx;
      txBreakFunc_ = uart3_tx_break;
      break;
#elif defined(IMXRT_LPUART3)
    case 3:
      txFunc_ = lpuart3_tx;
      txBreakFunc_ = lpuart3_tx_break;
      break;
#endif  // HAS_KINETISK_UART3 || IMXRT_LPUART3

#if defined(HAS_KINETISK_UART4)
    case 4:
      txFunc_ = uart4_tx;
      txBreakFunc_ = uart4_tx_break;
      break;
#elif defined(IMXRT_LPUART8)
    case 4:
      txFunc_ = lpuart8_tx;
      txBreakFunc_ = lpuart8_tx_break;
      break;
#endif  // HAS_KINETISK_UART4 || IMXRT_LPUART8

#if defined(HAS_KINETISK_UART5)
    case 5:
      txFunc_ = uart5_tx;
      txBreakFunc_ = uart5_tx_break;
      break;
#elif defined(HAS_KINETISK_LPUART0)
    case 5:
      txFunc_ = lpuart0_tx;
      txBreakFunc_ = lpuart0_tx_break;
      break;
#elif defined(IMXRT_LPUART1)
    case 5:
      txFunc_ = lpuart1_tx;
      txBreakFunc_ = lpuart1_tx_break;
      break;
#endif  // HAS_KINETISK_UART5 || HAS_KINETISK_LPUART0 || IMXRT_LPUART1

#if defined(IMXRT_LPUART7)
    case 6:
      txFunc_ = lpuart7_tx;
      txBreakFunc_ = lpuart7_tx_break;
      break;
#endif  // IMXRT_LPUART7

#if defined(IMXRT_LPUART5) && \
    (defined(__IMXRT1052__) || defined(ARDUINO_TEENSY41))
    case 7:
      txFunc_ = lpuart5_tx;
      txBreakFunc_ = lpuart5_tx_break;
      break;
#endif  // IMXRT_LPUART5 && (__IMXRT1052__ || ARDUINO_TEENSY41)

    default:
      txFunc_ = nullptr;
      txBreakFunc_ = nullptr;
  }
}

Receiver::~Receiver() {
  end();
}

// RX control states
#define UART_C2_RX_ENABLE UART_C2_RE | UART_C2_RIE | UART_C2_ILIE
#define LPUART_CTRL_RX_ENABLE \
  LPUART_CTRL_RE | LPUART_CTRL_RIE | LPUART_CTRL_ILIE

// Must define ACTIVATE_UART_RX_SERIAL_ERROR_N
#define ACTIVATE_UART_RX_SERIAL(N)                                \
  /* Enable receive */                                            \
  if (txEnabled_) {                                               \
    UART##N##_C2 = UART_C2_RX_ENABLE | UART_C2_TE;                \
  } else {                                                        \
    UART##N##_C2 = UART_C2_RX_ENABLE;                             \
  }                                                               \
  /* Start counting IDLE after the start bit */                   \
  UART##N##_C1 &= ~UART_C1_ILT;                                   \
  attachInterruptVector(IRQ_UART##N##_STATUS, &uart##N##_rx_isr); \
  /* Enable interrupt on frame error */                           \
  UART##N##_C3 |= UART_C3_FEIE;                                   \
  ACTIVATE_UART_RX_SERIAL_ERROR_##N

#define ACTIVATE_UART_RX_SERIAL_ERROR(N)                           \
  attachInterruptVector(IRQ_UART##N##_ERROR, &uart##N##_rx_isr);   \
  /* We fill bytes from the buffer in the framing error ISR, so we \
   * can set to the same priority. */                              \
  NVIC_SET_PRIORITY(IRQ_UART##N##_ERROR,                           \
                    NVIC_GET_PRIORITY(IRQ_UART##N##_STATUS));      \
  NVIC_ENABLE_IRQ(IRQ_UART##N##_ERROR);

#define ACTIVATE_LPUART_RX_SERIAL(N)                               \
  /* Enable receive and interrupt on frame error */                \
  if (txEnabled_) {                                                \
    LPUART##N##_CTRL =                                             \
        LPUART_CTRL_RX_ENABLE | LPUART_CTRL_TE | LPUART_CTRL_FEIE; \
  } else {                                                         \
    LPUART##N##_CTRL = LPUART_CTRL_RX_ENABLE | LPUART_CTRL_FEIE;   \
  }                                                                \
  /* Start counting IDLE after the start bit */                    \
  LPUART##N##_CTRL &= ~LPUART_CTRL_ILT;                            \
  attachInterruptVector(IRQ_LPUART##N, &lpuart##N##_rx_isr);

#define ENABLE_UART_TX(N)        \
  if (txEnabled_) {              \
    UART##N##_C2 |= UART_C2_TE;  \
  } else {                       \
    UART##N##_C2 &= ~UART_C2_TE; \
  }

#define ENABLE_LPUART_TX(N)              \
  if (txEnabled_) {                      \
    LPUART##N##_CTRL |= LPUART_CTRL_TE;  \
  } else {                               \
    LPUART##N##_CTRL &= ~LPUART_CTRL_TE; \
  }

void Receiver::setTXEnabled(bool flag) {
  if (txEnabled_ == flag) {
    return;
  }

  txEnabled_ = flag;
  if (!began_) {
    return;
  }

  switch (serialIndex_) {
#if defined(HAS_KINETISK_UART0) || defined(HAS_KINETISL_UART0)
    case 0:
      ENABLE_UART_TX(0)
      break;
#elif defined(IMXRT_LPUART6)
    case 0:
      ENABLE_LPUART_TX(6)
      break;
#endif  // HAS_KINETISK_UART0 || HAS_KINETISL_UART0 || IMXRT_LPUART6

#if defined(HAS_KINETISK_UART1) || defined(HAS_KINETISL_UART1)
    case 1:
      ENABLE_UART_TX(1)
      break;
#elif defined(IMXRT_LPUART4)
    case 1:
      ENABLE_LPUART_TX(4)
      break;
#endif  // HAS_KINETISK_UART1 || HAS_KINETISL_UART1 || IMXRT_LPUART4

#if defined(HAS_KINETISK_UART2) || defined(HAS_KINETISL_UART2)
    case 2:
      ENABLE_UART_TX(2)
      break;
#elif defined(IMXRT_LPUART2)
    case 2:
      ENABLE_LPUART_TX(2)
      break;
#endif  // HAS_KINETISK_UART2 || HAS_KINETISL_UART2 || IMXRT_LPUART2

#if defined(HAS_KINETISK_UART3)
    case 3:
      ENABLE_UART_TX(3)
      break;
#elif defined(IMXRT_LPUART3)
    case 3:
      ENABLE_LPUART_TX(3)
      break;
#endif  // HAS_KINETISK_UART3 || IMXRT_LPUART3

#if defined(HAS_KINETISK_UART4)
    case 4:
      ENABLE_UART_TX(4)
      break;
#elif defined(IMXRT_LPUART8)
    case 4:
      ENABLE_LPUART_TX(8)
      break;
#endif  // HAS_KINETISK_UART4 || IMXRT_LPUART8

#if defined(HAS_KINETISK_UART5)
    case 5:
      ENABLE_UART_TX(5)
      break;
#elif defined(HAS_KINETISK_LPUART0)
    case 5:
      ENABLE_LPUART_TX(0)
      break;
#elif defined(IMXRT_LPUART1)
    case 5:
      ENABLE_LPUART_TX(1)
      break;
#endif  // HAS_KINETISK_UART5 || HAS_KINETISK_LPUART0 || IMXRT_LPUART1

#if defined(IMXRT_LPUART7)
    case 6:
      ENABLE_LPUART_TX(7)
      break;
#endif  // IMXRT_LPUART7

#if defined(IMXRT_LPUART5) && \
    (defined(__IMXRT1052__) || defined(ARDUINO_TEENSY41))
    case 7:
      ENABLE_LPUART_TX(5)
      break;
#endif  // IMXRT_LPUART5 && (__IMXRT1052__ || ARDUINO_TEENSY41)
  }
}

void Receiver::begin() {
  if (began_) {
    return;
  }
  began_ = true;

  if (serialIndex_ < 0) {
    return;
  }

  // Reset all the stats
  resetPacketCount();
  packetSize_ = 0;
  lastBreakStartTime_ = 0;
  packetStats_ = PacketStats{};
  errorStats_ = ErrorStats{};

  // Set up the instance for the ISRs
  Receiver *r = rxInstances[serialIndex_];
  rxInstances[serialIndex_] = this;
  if (r != nullptr && r != this) {  // NOTE: Shouldn't be able to be 'this'
    r->end();
  }

  state_ = RecvStates::kIdle;
  activeBufIndex_ = 0;
  uart_.begin(kSlotsBaud, kSlotsFormat);

  // Reset "previous" state
  // NOTE: Any tampering with UART_C2 must be done after the serial port
  //       is activated because setting ILIE to 0 seems to lock things up
  setConnected(false);

  switch (serialIndex_) {
#if defined(HAS_KINETISK_UART0)
    case 0:
#define ACTIVATE_UART_RX_SERIAL_ERROR_0 ACTIVATE_UART_RX_SERIAL_ERROR(0)
      ACTIVATE_UART_RX_SERIAL(0)
#undef ACTIVATE_UART_RX_SERIAL_ERROR_0
      break;
#elif defined(HAS_KINETISL_UART0)
    case 0:
#define ACTIVATE_UART_RX_SERIAL_ERROR_0
      ACTIVATE_UART_RX_SERIAL(0)
#undef ACTIVATE_UART_RX_SERIAL_ERROR_0
      break;
#elif defined(IMXRT_LPUART6)
    case 0:
      ACTIVATE_LPUART_RX_SERIAL(6)
      break;
#endif  // HAS_KINETISK_UART0 || HAS_KINETISL_UART0 || IMXRT_LPUART6

#if defined(HAS_KINETISK_UART1)
    case 1:
#define ACTIVATE_UART_RX_SERIAL_ERROR_1 ACTIVATE_UART_RX_SERIAL_ERROR(1)
      ACTIVATE_UART_RX_SERIAL(1)
#undef ACTIVATE_UART_RX_SERIAL_ERROR_1
      break;
#elif defined(HAS_KINETISL_UART1)
    case 1:
#define ACTIVATE_UART_RX_SERIAL_ERROR_1
      ACTIVATE_UART_RX_SERIAL(1)
#undef ACTIVATE_UART_RX_SERIAL_ERROR_1
      break;
#elif defined(IMXRT_LPUART4)
    case 1:
      ACTIVATE_LPUART_RX_SERIAL(4)
      break;
#endif  // HAS_KINETISK_UART1 || HAS_KINETISL_UART1 || IMXRT_LPUART4

#if defined(HAS_KINETISK_UART2)
    case 2:
#define ACTIVATE_UART_RX_SERIAL_ERROR_2 ACTIVATE_UART_RX_SERIAL_ERROR(2)
      ACTIVATE_UART_RX_SERIAL(2)
#undef ACTIVATE_UART_RX_SERIAL_ERROR_2
      break;
#elif defined(HAS_KINETISL_UART2)
    case 2:
#define ACTIVATE_UART_RX_SERIAL_ERROR_2
      ACTIVATE_UART_RX_SERIAL(2)
#undef ACTIVATE_UART_RX_SERIAL_ERROR_2
      break;
#elif defined(IMXRT_LPUART2)
    case 2:
      ACTIVATE_LPUART_RX_SERIAL(2)
      break;
#endif  // HAS_KINETISK_UART2 || HAS_KINETISL_UART2 || IMXRT_LPUART2

#if defined(HAS_KINETISK_UART3)
    case 3:
#define ACTIVATE_UART_RX_SERIAL_ERROR_3 ACTIVATE_UART_RX_SERIAL_ERROR(3)
      ACTIVATE_UART_RX_SERIAL(3)
#undef ACTIVATE_UART_RX_SERIAL_ERROR_3
      break;
#elif defined(IMXRT_LPUART3)
    case 3:
      ACTIVATE_LPUART_RX_SERIAL(3)
      break;
#endif  // HAS_KINETISK_UART3 || IMXRT_LPUART3

#if defined(HAS_KINETISK_UART4)
    case 4:
#define ACTIVATE_UART_RX_SERIAL_ERROR_4 ACTIVATE_UART_RX_SERIAL_ERROR(4)
      ACTIVATE_UART_RX_SERIAL(4)
#undef ACTIVATE_UART_RX_SERIAL_ERROR_4
      break;
#elif defined(IMXRT_LPUART8)
    case 4:
      ACTIVATE_LPUART_RX_SERIAL(8)
      break;
#endif  // HAS_KINETISK_UART4 || IMXRT_LPUART8

#if defined(HAS_KINETISK_UART5)
    case 5:
#define ACTIVATE_UART_RX_SERIAL_ERROR_5 ACTIVATE_UART_RX_SERIAL_ERROR(5)
      ACTIVATE_UART_RX_SERIAL(5)
#undef ACTIVATE_UART_RX_SERIAL_ERROR_5
      break;
#elif defined(HAS_KINETISK_LPUART0)
    case 5:
      ACTIVATE_LPUART_RX_SERIAL(0)
      break;
#elif defined(IMXRT_LPUART1)
    case 5:
      ACTIVATE_LPUART_RX_SERIAL(1)
      break;
#endif  // HAS_KINETISK_UART5 || HAS_KINETISK_LPUART0 || IMXRT_LPUART1

#if defined(IMXRT_LPUART7)
    case 6:
      ACTIVATE_LPUART_RX_SERIAL(7)
      break;
#endif  // IMXRT_LPUART7

#if defined(IMXRT_LPUART5) && \
    (defined(__IMXRT1052__) || defined(ARDUINO_TEENSY41))
    case 7:
      ACTIVATE_LPUART_RX_SERIAL(5)
      break;
#endif  // IMXRT_LPUART5 && (__IMXRT1052__ || ARDUINO_TEENSY41)
  }

  // Enable receive
  setTXNotRX(false);
}

// Undefine these macros
#undef UART_C2_RX_ENABLE
#undef LPUART_CTRL_RX_ENABLE
#undef ACTIVATE_UART_RX_SERIAL
#undef ACTIVATE_UART_RX_SERIAL_ERROR
#undef ACTIVATE_LPUART_RX_SERIAL
#undef ENABLE_UART_TX
#undef ENABLE_LPUART_TX

void Receiver::end() {
  if (!began_) {
    return;
  }
  began_ = false;

  if (serialIndex_ < 0) {
    return;
  }

  // Remove any chance that our RX ISRs start after end() is called,
  // so disable the IRQs first

  uart_.end();

  switch (serialIndex_) {
#if defined(HAS_KINETISK_UART0)
    case 0:
      // Disable UART0 interrupt on frame error
      UART0_C3 &= ~UART_C3_FEIE;
      NVIC_DISABLE_IRQ(IRQ_UART0_ERROR);
      break;
#elif defined(HAS_KINETISL_UART0)
    case 0:
      // Disable UART0 interrupt on frame error
      UART0_C3 &= ~UART_C3_FEIE;
      break;
#endif  // HAS_KINETISK_UART0 || HAS_KINETISL_UART0

#if defined(HAS_KINETISK_UART1)
    case 1:
      UART1_C3 &= ~UART_C3_FEIE;
      NVIC_DISABLE_IRQ(IRQ_UART1_ERROR);
      break;
#elif defined(HAS_KINETISL_UART1)
    case 1:
      UART1_C3 &= ~UART_C3_FEIE;
      break;
#endif  // HAS_KINETISK_UART1 || HAS_KINETISL_UART1

#if defined(HAS_KINETISK_UART2)
    case 2:
      UART2_C3 &= ~UART_C3_FEIE;
      NVIC_DISABLE_IRQ(IRQ_UART2_ERROR);
      break;
#elif defined(HAS_KINETISL_UART2)
    case 2:
      UART2_C3 &= ~UART_C3_FEIE;
      break;
#endif  // HAS_KINETISK_UART2 || HAS_KINETISL_UART2

#if defined(HAS_KINETISK_UART3)
    case 3:
      UART3_C3 &= ~UART_C3_FEIE;
      NVIC_DISABLE_IRQ(IRQ_UART3_ERROR);
      break;
#endif  // HAS_KINETISK_UART3

#if defined(HAS_KINETISK_UART4)
    case 4:
      UART4_C3 &= ~UART_C3_FEIE;
      NVIC_DISABLE_IRQ(IRQ_UART4_ERROR);
      break;
#endif  // HAS_KINETISK_UART4

#if defined(HAS_KINETISK_UART5)
    case 5:
      UART5_C3 &= ~UART_C3_FEIE;
      NVIC_DISABLE_IRQ(IRQ_UART5_ERROR);
      break;
#endif  // HAS_KINETISK_UART5

// NOTE: Nothing needed for LPUARTs
  }

  // Remove the reference from the instances,
  // but only if we're the ones who added it
  if (rxInstances[serialIndex_] == this) {
    rxInstances[serialIndex_] = nullptr;
  }

  setConnected(false);
}

int Receiver::readPacket(uint8_t *buf, int startChannel, int len,
                         PacketStats *stats) {
  if (len <= 0 || startChannel < 0 || kMaxDMXPacketSize <= startChannel) {
    return 0;
  }

  int retval = -1;
  Lock lock{*this};
  //{
    // No need to poll for a timeout here because IDLE detection
    // handles this now

    if (packetSize_ > 0) {
      if (startChannel >= packetSize_) {
        retval = 0;
      } else {
        if (startChannel + len > packetSize_) {
          len = packetSize_ - startChannel;
        }
        std::copy_n(&inactiveBuf_[startChannel], len, &buf[0]);
        retval = len;
      }
      packetSize_ = 0;
    }
    if (stats != nullptr) {
      *stats = packetStats_;
    }
  //}
  return retval;
}

uint8_t Receiver::get(int channel, bool *rangeError) const {
  if (rangeError != nullptr) {
    *rangeError = true;
  }
  if (channel < 0 || kMaxDMXPacketSize <= channel) {
    return 0;
  }

  uint8_t b = 0;
  Lock lock{*this};
  //{
    // Since channel >= 0, lastPacketSize_ > channel implies lastPacketSize_ > 0
    if (channel < packetStats_.size) {
      if (rangeError != nullptr) {
        *rangeError = false;
      }
      b = inactiveBuf_[channel];
    }
  //}
  return b;
}

uint16_t Receiver::get16Bit(int channel, bool *rangeError) const {
  if (rangeError != nullptr) {
    *rangeError = true;
  }
  if (channel < 0 || kMaxDMXPacketSize - 1 <= channel) {
    return 0;
  }

  uint16_t v = 0;
  Lock lock{*this};
  //{
    // Since channel >= 0, lastPacketSize_ - 1 > channel
    // implies lastPacketSize_ - 1 > 0
    if (channel < packetStats_.size - 1) {
      if (rangeError != nullptr) {
        *rangeError = false;
      }
      v = (uint16_t{inactiveBuf_[channel]} << 8) |
          uint16_t{inactiveBuf_[channel + 1]};
    }
    //}
    return v;
}

std::shared_ptr<Responder> Receiver::setResponder(
    uint8_t startCode, std::shared_ptr<Responder> r) {
  // For a null responder, delete any current one for this start code
  if (r == nullptr) {
    Lock lock{*this};

    if (responders_ == nullptr) {
      return nullptr;
    }

    // Replace any previous responder
    std::shared_ptr<Responder> old{responders_[startCode]};
    if (old != nullptr) {
      responderCount_--;
    }

    // When no more responders, delete all the buffers
    if (responderCount_ == 0) {
      responderOutBuf_ = nullptr;
      responders_ = nullptr;
    }

    return old;
  }

  Lock lock{*this};

  // Allocate this first because it's done once. The output buffer may get
  // reallocated, and so letting that be the last thing deleted avoids
  // potential fragmentation.
  if (responders_ == nullptr) {
    responders_.reset(new std::shared_ptr<Responder>[256]);
    // Allocation may have failed on small systems
    if (responders_ == nullptr) {
      return nullptr;
    }
  }

  // Initialize the output buffer
  int outBufSize = r->outputBufferSize();
  if (responderOutBuf_ == nullptr || responderOutBufLen_ < outBufSize) {
    responderOutBuf_.reset(new uint8_t[outBufSize]);
    // Allocation may have failed on small systems
    if (responderOutBuf_ == nullptr) {
      responderOutBufLen_ = 0;
      responders_ = nullptr;
      responderCount_ = 0;
      return nullptr;
    }
    responderOutBufLen_ = outBufSize;
  }

  // If a responder is already set then the output buffer should be the
  // correct size
  std::shared_ptr<Responder> old{responders_[startCode]};
  // Using std::move avoids two atomic operations since the `r` parameter has
  // been copy-constructed
  // See:
  // https://stackoverflow.com/questions/41871115/why-would-i-stdmove-an-stdshared-ptr
  responders_[startCode] = std::move(r);
  if (old == nullptr) {
    responderCount_++;
  }

  return old;
}

void Receiver::completePacket() {
  uint32_t t = millis();
  state_ = RecvStates::kIdle;

  clearILT();  // Set the IDLE detection to "after start bit"

  // An empty packet isn't valid, there must be at least a start code
  if (activeBufIndex_ <= 0) {
    return;
  }

  // Check for a short packet. If found, discard the data if the "keep short
  // packets" feature is disabled; otherwise, don't discard the data but mark it
  // as "short".
  // Do this check after first checking activeBufIndex_ because a positive value
  // means that the following start and end time variables are valid
  if (lastSlotEndTime_ - breakStartTime_ < kMinDMXPacketTime) {
    errorStats_.shortPacketCount++;
    if (keepShortPackets_) {
      packetStats_.isShort = true;
    } else {
      packetStats_.isShort = false;
      activeBufIndex_ = 0;
    }
  } else {
    packetStats_.isShort = false;
  }

  // Swap the buffers
  if (activeBuf_ == buf1_) {
    activeBuf_ = buf2_;
    inactiveBuf_ = buf1_;
  } else {
    activeBuf_ = buf1_;
    inactiveBuf_ = buf2_;
  }

  incPacketCount();

  // Packet stats
  packetStats_.size = packetSize_ = activeBufIndex_;
  packetStats_.timestamp = t;
  packetStats_.packetTime = lastSlotEndTime_ - breakStartTime_;
  packetStats_.breakPlusMABTime = packetStats_.nextBreakPlusMABTime;
  packetStats_.breakTime = packetStats_.nextBreakTime;
  packetStats_.mabTime = packetStats_.nextMABTime;

  // Let the responder, if any, process the packet
  if (responders_ != nullptr) {
    Responder *r = responders_[inactiveBuf_[0]].get();
    if (r != nullptr) {
      r->receivePacket(inactiveBuf_, packetSize_);
      if (r->eatPacket()) {
        packetStats_.size = packetSize_ = 0;
      }
    }
  }

  activeBufIndex_ = 0;
}

#define UART_CLEAR_ILT(N) UART##N##_C1 &= ~UART_C1_ILT;
#define LPUART_CLEAR_ILT(N) LPUART##N##_CTRL &= ~LPUART_CTRL_ILT;
#define UART_SET_ILT(N) UART##N##_C1 |= UART_C1_ILT;
#define LPUART_SET_ILT(N) LPUART##N##_CTRL |= LPUART_CTRL_ILT;

void Receiver::clearILT() const {
  // Change the Idle Line Type Select to "Idle starts after start bit"
  switch (serialIndex_) {
#if defined(HAS_KINETISK_UART0) || defined(HAS_KINETISL_UART0)
    case 0:
      UART_CLEAR_ILT(0)
      break;
#elif defined(IMXRT_LPUART6)
    case 0:
      LPUART_CLEAR_ILT(6)
      break;
#endif  // HAS_KINETISK_UART0 || HAS_KINETISL_UART0 || IMXRT_LPUART6

#if defined(HAS_KINETISK_UART1) || defined(HAS_KINETISL_UART1)
    case 1:
      UART_CLEAR_ILT(1)
      break;
#elif defined(IMXRT_LPUART4)
    case 1:
      LPUART_CLEAR_ILT(4)
      break;
#endif  // HAS_KINETISK_UART1 || HAS_KINETISL_UART1 || IMXRT_LPUART4

#if defined(HAS_KINETISK_UART2) || defined(HAS_KINETISL_UART2)
    case 2:
      UART_CLEAR_ILT(2)
      break;
#elif defined(IMXRT_LPUART2)
    case 2:
      LPUART_CLEAR_ILT(2)
      break;
#endif  // HAS_KINETISK_UART2 || HAS_KINETISL_UART2 || IMXRT_LPUART2

#if defined(HAS_KINETISK_UART3)
    case 3:
      UART_CLEAR_ILT(3)
      break;
#elif defined(IMXRT_LPUART3)
    case 3:
      LPUART_CLEAR_ILT(3)
      break;
#endif  // HAS_KINETISK_UART3 || IMXRT_LPUART3

#if defined(HAS_KINETISK_UART4)
    case 4:
      UART_CLEAR_ILT(4)
      break;
#elif defined(IMXRT_LPUART8)
    case 4:
      LPUART_CLEAR_ILT(8)
      break;
#endif  // HAS_KINETISK_UART4 || IMXRT_LPUART8

#if defined(HAS_KINETISK_UART5)
    case 5:
      UART_CLEAR_ILT(5)
      break;
#elif defined(HAS_KINETISK_LPUART0)
    case 5:
      LPUART_CLEAR_ILT(0)
      break;
#elif defined(IMXRT_LPUART1)
    case 5:
      LPUART_CLEAR_ILT(1)
      break;
#endif  // HAS_KINETISK_UART5 || HAS_KINETISK_LPUART0 || IMXRT_LPUART1

#if defined(IMXRT_LPUART7)
    case 6:
      LPUART_CLEAR_ILT(7)
      break;
#endif  // IMXRT_LPUART7

#if defined(IMXRT_LPUART5) && \
    (defined(__IMXRT1052__) || defined(ARDUINO_TEENSY41))
    case 7:
      LPUART_CLEAR_ILT(5)
      break;
#endif  // IMXRT_LPUART5 && (__IMXRT1052__ || ARDUINO_TEENSY41)
  }
}

void Receiver::setILT() const {
  // Change the Idle Line Type Select to "Idle starts after start bit"
  switch (serialIndex_) {
#if defined(HAS_KINETISK_UART0) || defined(HAS_KINETISL_UART0)
    case 0:
      UART_SET_ILT(0)
      break;
#elif defined(IMXRT_LPUART6)
    case 0:
      LPUART_SET_ILT(6)
      break;
#endif  // HAS_KINETISK_UART0 || HAS_KINETISL_UART0 || IMXRT_LPUART6

#if defined(HAS_KINETISK_UART1) || defined(HAS_KINETISL_UART1)
    case 1:
      UART_SET_ILT(1)
      break;
#elif defined(IMXRT_LPUART4)
    case 1:
      LPUART_SET_ILT(4)
      break;
#endif  // HAS_KINETISK_UART1 || HAS_KINETISL_UART1 || IMXRT_LPUART4

#if defined(HAS_KINETISK_UART2) || defined(HAS_KINETISL_UART2)
    case 2:
      UART_SET_ILT(2)
      break;
#elif defined(IMXRT_LPUART2)
    case 2:
      LPUART_SET_ILT(2)
      break;
#endif  // HAS_KINETISK_UART2 || HAS_KINETISL_UART2 || IMXRT_LPUART2

#if defined(HAS_KINETISK_UART3)
    case 3:
      UART_SET_ILT(3)
      break;
#elif defined(IMXRT_LPUART3)
    case 3:
      LPUART_SET_ILT(3)
      break;
#endif  // HAS_KINETISK_UART3 || IMXRT_LPUART3

#if defined(HAS_KINETISK_UART4)
    case 4:
      UART_SET_ILT(4)
      break;
#elif defined(IMXRT_LPUART8)
    case 4:
      LPUART_SET_ILT(8)
      break;
#endif  // HAS_KINETISK_UART4 || IMXRT_LPUART8

#if defined(HAS_KINETISK_UART5)
    case 5:
      UART_SET_ILT(5)
      break;
#elif defined(HAS_KINETISK_LPUART0)
    case 5:
      LPUART_SET_ILT(0)
      break;
#elif defined(IMXRT_LPUART1)
    case 5:
      LPUART_SET_ILT(1)
      break;
#endif  // HAS_KINETISK_UART5 || HAS_KINETISK_LPUART0 || IMXRT_LPUART1

#if defined(IMXRT_LPUART7)
    case 6:
      LPUART_SET_ILT(7)
      break;
#endif  // IMXRT_LPUART7

#if defined(IMXRT_LPUART5) && \
    (defined(__IMXRT1052__) || defined(ARDUINO_TEENSY41))
    case 7:
      LPUART_SET_ILT(5)
      break;
#endif  // IMXRT_LPUART5 && (__IMXRT1052__ || ARDUINO_TEENSY41)
  }
}

#undef UART_CLEAR_ILT
#undef LPUART_CLEAR_ILT
#undef UART_SET_ILT
#undef LPUART_SET_ILT

void Receiver::receiveIdle(uint32_t eventTime) {
  switch (state_) {
    case RecvStates::kBreak:
      if (seenMABStart_) {
        if ((mabStartTime_ - breakStartTime_) < kMinBreakTime) {
          seenMABStart_ = false;
          receiveBadBreak();
          return;
        }
      } else {
        // This catches the case where a short BREAK is followed by a longer MAB
        if ((eventTime - breakStartTime_) < kMinBreakTime + kCharTime) {
          seenMABStart_ = false;
          receiveBadBreak();
          return;
        }

        // We can infer what the rise time is here
        seenMABStart_ = true;
        mabStartTime_ = eventTime - kCharTime;
        setILT();  // Set IDLE detection to "after stop bit"
      }
      break;

    case RecvStates::kData:
      if ((eventTime - breakStartTime_) > kMaxDMXPacketTime ||
          (eventTime - lastSlotEndTime_) >= kMaxDMXIdleTime) {
        // We'll consider this as a packet end and not as a timeout
        // errorStats_.packetTimeoutCount++;
        completePacket();
        setConnected(false);
        return;
      }
      break;

    default:
      break;
  }

  // Start a timer watching for disconnection/packet end
  periodicTimer_.begin(
      [&]() {
        periodicTimer_.end();
        completePacket();
        setConnected(false);
      },
      kMaxDMXIdleTime - kCharTime);
}

void Receiver::receivePotentialBreak(uint32_t eventTime) {
  periodicTimer_.end();

  // A potential BREAK is detected when a stop bit is expected but not
  // received, and this happens after the start bit, nine bits, and the
  // missing stop bit, about 44us.
  // Note that breakStartTime_ only represents a potential BREAK start
  // time until we receive the first character.
  breakStartTime_ = eventTime - kCharTime;

  state_ = RecvStates::kBreak;

  // At this point, we don't know whether to keep or discard any collected
  // data because the BREAK may be invalid. In other words, don't make any
  // framing error or short packet decisions until we know the nature of
  // this BREAK.

  if (rxWatchPin_ >= 0) {
    seenMABStart_ = false;
    attachInterrupt(rxWatchPin_, rxPinRoseISRs[serialIndex_], RISING);
  }
}

void Receiver::receiveBadBreak() {
  // Not a BREAK
  errorStats_.framingErrorCount++;

  // Don't keep the packet
  // See: [BREAK timing at the receiver](http://www.rdmprotocol.org/forums/showthread.php?t=1292)
  activeBufIndex_ = 0;
  completePacket();

  // Consider this case as not seeing a BREAK
  // This may be line noise, so now we can't tell for sure where the
  // last BREAK was
  setConnected(false);
}

void Receiver::receiveByte(uint8_t b, uint32_t eopTime) {
  periodicTimer_.end();

  // Bad BREAKs are detected when BREAK + MAB + character time is too short
  // BREAK: 88us
  // MAB: 8us
  // Character time: 44us

  switch (state_) {
    case RecvStates::kBreak: {
      // BREAK and MAB timing check
      // Measure the BREAK and MAB, but don't set until after a
      // potential completePacket()
      uint32_t breakTime = 0;
      uint32_t mabTime = 0;
      if (seenMABStart_) {
        seenMABStart_ = false;
        if ((mabStartTime_ - breakStartTime_ < kMinBreakTime) ||
            (eopTime - mabStartTime_ < kMinMABTime + kCharTime)) {
          receiveBadBreak();
          return;
        }
        breakTime = mabStartTime_ - breakStartTime_;
        mabTime = eopTime - kCharTime - mabStartTime_;
        if (mabTime >= kMaxDMXIdleTime) {
          completePacket();
          setConnected(false);
          return;
        }
      } else {
        seenMABStart_ = false;
        // This is only a rudimentary check for short BREAKs. It does not
        // detect short BREAKs followed by long MABs. It only detects
        // whether BREAK + MAB time is at least 88us + 8us.
        if ((eopTime - breakStartTime_) <
            kMinBreakTime + kMinMABTime + kCharTime) {
          // First byte is too early, discard any data
          receiveBadBreak();
          return;
        }
        setILT();  // Set IDLE detection to "after stop bit"
        // Since we've already received a byte, the idle detection can't start
        // again until the end of the next byte not already in the buffer
      }

      if (connected_) {  // This condition indicates we haven't seen some
                         // timeout and that lastBreakStartTime_ is valid
        // Complete any un-flushed bytes
        uint32_t dt = breakStartTime_ - lastBreakStartTime_;
        packetStats_.breakToBreakTime = dt;

        // In the following checks, the packet time limits are the same as the
        // BREAK-to-BREAK time limits
        if (dt < kMinDMXPacketTime) {
          errorStats_.shortPacketCount++;
          // Discard the data
          activeBufIndex_ = 0;
        } else if (dt > kMaxDMXPacketTime) {
          // NOTE: Zero-length packets will also trigger a timeout
          errorStats_.packetTimeoutCount++;
          // Keep the data
          // Don't disconnect because the timeout was relative to the
          // previous packet
        }
        completePacket();
      } else {
        packetStats_.breakToBreakTime = 0;
        activeBufIndex_ = 0;
      }

      // Packet BREAK and MAB measurements
      // Store 'next' values because packets aren't completed until the
      // following BREAK (or timeout or size limit) and we need the
      // previous values
      packetStats_.nextBreakPlusMABTime = eopTime - kCharTime - breakStartTime_;
      packetStats_.nextBreakTime = breakTime;
      packetStats_.nextMABTime = mabTime;

      lastBreakStartTime_ = breakStartTime_;
      setConnected(true);
      state_ = RecvStates::kData;
      break;
    }

    case RecvStates::kData:
      // Checking this here accounts for buffered input, where several
      // bytes come in at the same time
      if (eopTime - breakStartTime_ < kMinBreakTime + kMinMABTime + kCharTime +
                                          kCharTime*activeBufIndex_) {
        // First byte is too early, discard any data
        receiveBadBreak();
        return;
      }
      // NOTE: Don't need to check for inter-slot MARK time being
      //       too large because the IDLE detection will catch that
      break;

    case RecvStates::kIdle:
      return;

    default:
      // Ignore any extra bytes in a packet or any bytes outside a packet
      return;
  }

  // Check the timing and if we are out of range then complete any bytes
  // until, but not including, this one
  lastSlotEndTime_ = eopTime;
  if ((eopTime - breakStartTime_) > kMaxDMXPacketTime) {
    errorStats_.packetTimeoutCount++;
    completePacket();
    setConnected(false);
    return;
  }

  bool packetFull = false;  // Indicates whether the maximum packet size
                            // has been reached.
                            // Using this is necessary so that the responder's
                            // processByte is called before its receivePacket.
  activeBuf_[activeBufIndex_++] = b;
  if (activeBufIndex_ == kMaxDMXPacketSize) {
    packetFull = true;
  }

  // See if a responder needs to process the byte and respond
  Responder *r = nullptr;
  if (responders_ != nullptr) {
    r = responders_[activeBuf_[0]].get();
  }
  if (r == nullptr) {
    if (packetFull) {
      completePacket();
    }
    return;
  }

  // Let the responder process the data
  int respLen =
      r->processByte(activeBuf_, activeBufIndex_, responderOutBuf_.get());
  if (respLen <= 0) {
    if (packetFull) {
      // If the responder isn't done by now, it's too late for this packet
      // because the maximum packet size has been reached
      completePacket();
    }
    return;
  }
  completePacket();  // This is probably the best option, even though there may
                     // be more bytes
  if (!txEnabled_ || txFunc_ == nullptr) {
    return;
  }

  // Do the response
  if (r->isSendBreakForLastPacket()) {
    if (txBreakFunc_ == nullptr) {
      return;
    }

    uint32_t delay = r->preBreakDelay();
    uint32_t dt = micros() - eopTime;
    if (dt < delay) {
      delayMicroseconds(delay - dt);
    }

    setTXNotRX(true);
    delay = r->preDataDelay();
    if (delay > 0) {
      delayMicroseconds(delay);
    }
    txBreakFunc_(r->breakTime(), r->mabTime());
  } else {
    uint32_t delay = r->preNoBreakDelay();
    uint32_t dt = micros() - eopTime;
    if (dt < delay) {
      delayMicroseconds(delay - dt);
    }

    setTXNotRX(true);
    delay = r->preDataDelay();
    if (delay > 0) {
      delayMicroseconds(delay);
    }
  }
  txFunc_(responderOutBuf_.get(), respLen);
  setTXNotRX(false);
}

void Receiver::setConnected(bool flag) {
  if (connected_ != flag) {
    connected_ = flag;
    void (*f)(Receiver *r) = connectChangeFunc_;
    if (f != nullptr) {
      f(this);
    }
  }
}

// ---------------------------------------------------------------------------
//  IRQ management
// ---------------------------------------------------------------------------

void Receiver::disableIRQs() const {
  if (!began_) {
    return;
  }
  switch (serialIndex_) {
#if defined(HAS_KINETISK_UART0)
    case 0:
      NVIC_DISABLE_IRQ(IRQ_UART0_STATUS);
      NVIC_DISABLE_IRQ(IRQ_UART0_ERROR);
      break;
#elif defined(HAS_KINETISL_UART0)
    case 0:
      NVIC_DISABLE_IRQ(IRQ_UART0_STATUS);
      break;
#elif defined(IMXRT_LPUART6)
    case 0:
      NVIC_DISABLE_IRQ(IRQ_LPUART6);
      break;
#endif  // HAS_KINETISK_UART0 || HAS_KINETISL_UART0 || IMXRT_LPUART6

#if defined(HAS_KINETISK_UART1)
    case 1:
      NVIC_DISABLE_IRQ(IRQ_UART1_STATUS);
      NVIC_DISABLE_IRQ(IRQ_UART1_ERROR);
      break;
#elif defined(HAS_KINETISL_UART1)
    case 1:
      NVIC_DISABLE_IRQ(IRQ_UART1_STATUS);
      break;
#elif defined(IMXRT_LPUART4)
    case 1:
      NVIC_DISABLE_IRQ(IRQ_LPUART4);
      break;
#endif  // HAS_KINETISK_UART1 || HAS_KINETISL_UART1 || IMXRT_LPUART4

#if defined(HAS_KINETISK_UART2)
    case 2:
      NVIC_DISABLE_IRQ(IRQ_UART2_STATUS);
      NVIC_DISABLE_IRQ(IRQ_UART2_ERROR);
      break;
#elif defined(HAS_KINETISL_UART2)
    case 2:
      NVIC_DISABLE_IRQ(IRQ_UART2_STATUS);
      break;
#elif defined(IMXRT_LPUART2)
    case 2:
      NVIC_DISABLE_IRQ(IRQ_LPUART2);
      break;
#endif  // HAS_KINETISK_UART2 || HAS_KINETISL_UART2 || IMXRT_LPUART2

#if defined(HAS_KINETISK_UART3)
    case 3:
      NVIC_DISABLE_IRQ(IRQ_UART3_STATUS);
      NVIC_DISABLE_IRQ(IRQ_UART3_ERROR);
      break;
#elif defined(IMXRT_LPUART3)
    case 3:
      NVIC_DISABLE_IRQ(IRQ_LPUART3);
      break;
#endif  // HAS_KINETISK_UART3 || IMXRT_LPUART3

#if defined(HAS_KINETISK_UART4)
    case 4:
      NVIC_DISABLE_IRQ(IRQ_UART4_STATUS);
      NVIC_DISABLE_IRQ(IRQ_UART4_ERROR);
      break;
#elif defined(IMXRT_LPUART8)
    case 4:
      NVIC_DISABLE_IRQ(IRQ_LPUART8);
      break;
#endif  // HAS_KINETISK_UART4 || IMXRT_LPUART8

#if defined(HAS_KINETISK_UART5)
    case 5:
      NVIC_DISABLE_IRQ(IRQ_UART5_STATUS);
      NVIC_DISABLE_IRQ(IRQ_UART5_ERROR);
      break;
#elif defined(HAS_KINETISK_LPUART0)
    case 5:
      NVIC_DISABLE_IRQ(IRQ_LPUART0);
      break;
#elif defined(IMXRT_LPUART1)
    case 5:
      NVIC_DISABLE_IRQ(IRQ_LPUART1);
      break;
#endif  // HAS_KINETISK_UART5 || HAS_KINETISK_LPUART0 || IMXRT_LPUART1

#if defined(IMXRT_LPUART7)
    case 6:
      NVIC_DISABLE_IRQ(IRQ_LPUART7);
      break;
#endif  // IMXRT_LPUART7

#if defined(IMXRT_LPUART5) && \
    (defined(__IMXRT1052__) || defined(ARDUINO_TEENSY41))
    case 7:
      NVIC_DISABLE_IRQ(IRQ_LPUART5);
      break;
#endif  // IMXRT_LPUART5 && (__IMXRT1052__ || ARDUINO_TEENSY41)
  }
}

void Receiver::enableIRQs() const {
  if (!began_) {
    return;
  }
  switch (serialIndex_) {
#if defined(HAS_KINETISK_UART0)
    case 0:
      NVIC_ENABLE_IRQ(IRQ_UART0_STATUS);
      NVIC_ENABLE_IRQ(IRQ_UART0_ERROR);
      break;
#elif defined(HAS_KINETISL_UART0)
    case 0:
      NVIC_ENABLE_IRQ(IRQ_UART0_STATUS);
      break;
#elif defined(IMXRT_LPUART6)
    case 0:
      NVIC_ENABLE_IRQ(IRQ_LPUART6);
      break;
#endif  // HAS_KINETISK_UART0 || HAS_KINETISL_UART0 || IMXRT_LPUART6

#if defined(HAS_KINETISK_UART1)
    case 1:
      NVIC_ENABLE_IRQ(IRQ_UART1_STATUS);
      NVIC_ENABLE_IRQ(IRQ_UART1_ERROR);
      break;
#elif defined(HAS_KINETISL_UART1)
    case 1:
      NVIC_ENABLE_IRQ(IRQ_UART1_STATUS);
      break;
#elif defined(IMXRT_LPUART4)
    case 1:
      NVIC_ENABLE_IRQ(IRQ_LPUART4);
      break;
#endif  // HAS_KINETISK_UART1 || HAS_KINETISL_UART1 || IMXRT_LPUART4

#if defined(HAS_KINETISK_UART2)
    case 2:
      NVIC_ENABLE_IRQ(IRQ_UART2_STATUS);
      NVIC_ENABLE_IRQ(IRQ_UART2_ERROR);
      break;
#elif defined(HAS_KINETISL_UART2)
    case 2:
      NVIC_ENABLE_IRQ(IRQ_UART2_STATUS);
      break;
#elif defined(IMXRT_LPUART2)
    case 2:
      NVIC_ENABLE_IRQ(IRQ_LPUART2);
      break;
#endif  // HAS_KINETISK_UART2 || HAS_KINETISL_UART2 || IMXRT_LPUART2

#if defined(HAS_KINETISK_UART3)
    case 3:
      NVIC_ENABLE_IRQ(IRQ_UART3_STATUS);
      NVIC_ENABLE_IRQ(IRQ_UART3_ERROR);
      break;
#elif defined(IMXRT_LPUART3)
    case 3:
      NVIC_ENABLE_IRQ(IRQ_LPUART3);
      break;
#endif  // HAS_KINETISK_UART3 || IMXRT_LPUART3

#if defined(HAS_KINETISK_UART4)
    case 4:
      NVIC_ENABLE_IRQ(IRQ_UART4_STATUS);
      NVIC_ENABLE_IRQ(IRQ_UART4_ERROR);
      break;
#elif defined(IMXRT_LPUART8)
    case 4:
      NVIC_ENABLE_IRQ(IRQ_LPUART8);
      break;
#endif  // HAS_KINETISK_UART4 || IMXRT_LPUART8

#if defined(HAS_KINETISK_UART5)
    case 5:
      NVIC_ENABLE_IRQ(IRQ_UART5_STATUS);
      NVIC_ENABLE_IRQ(IRQ_UART5_ERROR);
      break;
#elif defined(HAS_KINETISK_LPUART0)
    case 5:
      NVIC_ENABLE_IRQ(IRQ_LPUART0);
      break;
#elif defined(IMXRT_LPUART1)
    case 5:
      NVIC_ENABLE_IRQ(IRQ_LPUART1);
      break;
#endif  // HAS_KINETISK_UART5 || HAS_KINETISK_LPUART0 || IMXRT_LPUART1

#if defined(IMXRT_LPUART7)
    case 6:
      NVIC_ENABLE_IRQ(IRQ_LPUART7);
      break;
#endif  // IMXRT_LPUART7

#if defined(IMXRT_LPUART5) && \
    (defined(__IMXRT1052__) || defined(ARDUINO_TEENSY41))
    case 7:
      NVIC_ENABLE_IRQ(IRQ_LPUART5);
      break;
#endif  // IMXRT_LPUART5 && (__IMXRT1052__ || ARDUINO_TEENSY41)
  }
}

// ---------------------------------------------------------------------------
//  RX pin interrupt and ISRs
// ---------------------------------------------------------------------------

void Receiver::setRXWatchPin(int pin) {
  __disable_irq();
  //{
    if (pin < 0) {
      if (rxWatchPin_ >= 0) {
        detachInterrupt(rxWatchPin_);
      }
      rxWatchPin_ = -1;
      seenMABStart_ = false;
    } else {
      if (rxWatchPin_ != pin) {
        detachInterrupt(rxWatchPin_);
        rxWatchPin_ = pin;
        seenMABStart_ = false;
      }
    }
  //}
  __enable_irq();
}

void Receiver::rxPinRose_isr() {
  mabStartTime_ = micros();
  if (!seenMABStart_) {
    seenMABStart_ = true;
    detachInterrupt(rxWatchPin_);
  } else {
    seenMABStart_ = false;
  }
  setILT();  // Set IDLE detection to "after stop bit"
}

void rxPinRoseSerial0_isr() {
  Receiver *r = rxInstances[0];
  if (r != nullptr) {
    r->rxPinRose_isr();
  }
}

void rxPinRoseSerial1_isr() {
  Receiver *r = rxInstances[1];
  if (r != nullptr) {
    r->rxPinRose_isr();
  }
}

void rxPinRoseSerial2_isr() {
  Receiver *r = rxInstances[2];
  if (r != nullptr) {
    r->rxPinRose_isr();
  }
}

void rxPinRoseSerial3_isr() {
  Receiver *r = rxInstances[3];
  if (r != nullptr) {
    r->rxPinRose_isr();
  }
}

void rxPinRoseSerial4_isr() {
  Receiver *r = rxInstances[4];
  if (r != nullptr) {
    r->rxPinRose_isr();
  }
}

void rxPinRoseSerial5_isr() {
  Receiver *r = rxInstances[5];
  if (r != nullptr) {
    r->rxPinRose_isr();
  }
}

void rxPinRoseSerial6_isr() {
  Receiver *r = rxInstances[6];
  if (r != nullptr) {
    r->rxPinRose_isr();
  }
}

#if defined(__IMXRT1052__) || defined(ARDUINO_TEENSY41)
void rxPinRoseSerial7_isr() {
  Receiver *r = rxInstances[7];
  if (r != nullptr) {
    r->rxPinRose_isr();
  }
}
#endif  // __IMXRT1052__ || ARDUINO_TEENSY41

// ---------------------------------------------------------------------------
//  UART0 synchronous TX
// ---------------------------------------------------------------------------

#if defined(HAS_KINETISK_UART0) || defined(HAS_KINETISL_UART0)

#if defined(HAS_KINETISK_UART0_FIFO)
#define UART_SYNC_TX_SEND_FIFO_0 UART_SYNC_TX_SEND_FIFO(0)
#define UART_TX_FLUSH_FIFO_0 UART_TX_FLUSH_FIFO(0)
#else
#define UART_SYNC_TX_SEND_FIFO_0
#define UART_TX_FLUSH_FIFO_0
#endif  // HAS_KINETISK_UART0_FIFO

void uart0_tx(const uint8_t *b, int len) {
  UART_SYNC_TX(0, UART0_S1, UART_S1, UART0_D)
}

void uart0_tx_break(uint32_t breakTime, uint32_t mabTime) {
  UART_TX_BREAK(0)
}

#undef UART_SYNC_TX_SEND_FIFO_0
#undef UART_TX_FLUSH_FIFO_0

#endif  // HAS_KINETISK_UART0 || HAS_KINETISL_UART0

// ---------------------------------------------------------------------------
//  UART1 synchronous TX
// ---------------------------------------------------------------------------

#if defined(HAS_KINETISK_UART1) || defined(HAS_KINETISL_UART1)

#if defined(HAS_KINETISK_UART1_FIFO)
#define UART_SYNC_TX_SEND_FIFO_1 UART_SYNC_TX_SEND_FIFO(1)
#define UART_TX_FLUSH_FIFO_1 UART_TX_FLUSH_FIFO(1)
#else
#define UART_SYNC_TX_SEND_FIFO_1
#define UART_TX_FLUSH_FIFO_1
#endif  // HAS_KINETISK_UART1_FIFO

void uart1_tx(const uint8_t *b, int len) {
  UART_SYNC_TX(1, UART1_S1, UART_S1, UART1_D)
}

void uart1_tx_break(uint32_t breakTime, uint32_t mabTime) {
  UART_TX_BREAK(1)
}

#undef UART_SYNC_TX_SEND_FIFO_1
#undef UART_TX_FLUSH_FIFO_1

#endif  // HAS_KINETISK_UART1 || HAS_KINETISL_UART1

// ---------------------------------------------------------------------------
//  UART2 synchronous TX
// ---------------------------------------------------------------------------

#if defined(HAS_KINETISK_UART2) || defined(HAS_KINETISL_UART2)

#define UART_SYNC_TX_SEND_FIFO_2
#define UART_TX_FLUSH_FIFO_2

void uart2_tx(const uint8_t *b, int len) {
  UART_SYNC_TX(2, UART2_S1, UART_S1, UART2_D)
}

void uart2_tx_break(uint32_t breakTime, uint32_t mabTime) {
  UART_TX_BREAK(2)
}

#undef UART_SYNC_TX_SEND_FIFO_2
#undef UART_TX_FLUSH_FIFO_2

#endif  // HAS_KINETISK_UART2 || HAS_KINETISL_UART2

// ---------------------------------------------------------------------------
//  UART3 synchronous TX
// ---------------------------------------------------------------------------

#if defined(HAS_KINETISK_UART3)

#define UART_SYNC_TX_SEND_FIFO_3

void uart3_tx(const uint8_t *b, int len) {
  UART_SYNC_TX(3, UART3_S1, UART_S1, UART3_D)
}

#undef UART_SYNC_TX_SEND_FIFO_3

#define UART_TX_FLUSH_FIFO_3

void uart3_tx_break(uint32_t breakTime, uint32_t mabTime) {
  UART_TX_BREAK(3)
}

#undef UART_TX_FLUSH_FIFO_3

#endif  // HAS_KINETISK_UART3

// ---------------------------------------------------------------------------
//  UART4 synchronous TX
// ---------------------------------------------------------------------------

#if defined(HAS_KINETISK_UART4)

#define UART_SYNC_TX_SEND_FIFO_4

void uart4_tx(const uint8_t *b, int len) {
  UART_SYNC_TX(4, UART4_S1, UART_S1, UART4_D)
}

#undef UART_SYNC_TX_SEND_FIFO_4

#define UART_TX_FLUSH_FIFO_4

void uart4_tx_break(uint32_t breakTime, uint32_t mabTime) {
  UART_TX_BREAK(4)
}

#undef UART_TX_FLUSH_FIFO_4

#endif  // HAS_KINETISK_UART4

// ---------------------------------------------------------------------------
//  UART5 synchronous TX
// ---------------------------------------------------------------------------

#if defined(HAS_KINETISK_UART5)

#define UART_SYNC_TX_SEND_FIFO_5

void uart5_tx(const uint8_t *b, int len) {
  UART_SYNC_TX(5, UART5_S1, UART_S1, UART5_D)
}

#undef UART_SYNC_TX_SEND_FIFO_5

#define UART_TX_FLUSH_FIFO_5

void uart5_tx_break(uint32_t breakTime, uint32_t mabTime) {
  UART_TX_BREAK(5)
}

#undef UART_TX_FLUSH_FIFO_5

#endif  // HAS_KINETISK_UART5

// ---------------------------------------------------------------------------
//  LPUART0 synchronous TX (Serial6 on Teensy 3.6)
// ---------------------------------------------------------------------------

#if defined(HAS_KINETISK_LPUART0)

#define UART_SYNC_TX_SEND_FIFO_0

void lpuart0_tx(const uint8_t *b, int len) {
  UART_SYNC_TX(0, LPUART0_STAT, LPUART_STAT, LPUART0_DATA)
}

#undef UART_SYNC_TX_SEND_FIFO_0

#define LPUART_TX_FLUSH_FIFO_0

void lpuart0_tx_break(uint32_t breakTime, uint32_t mabTime) {
  LPUART_TX_BREAK(0)
}

#undef LPUART_TX_FLUSH_FIFO_0

#endif  // HAS_KINETISK_LPUART0

// ---------------------------------------------------------------------------
//  LPUART6 synchronous TX (Serial1 on Teensy 4)
// ---------------------------------------------------------------------------

#if defined(IMXRT_LPUART6)

#define UART_SYNC_TX_SEND_FIFO_6 LPUART_SYNC_TX_SEND_FIFO(6)

void lpuart6_tx(const uint8_t *b, int len) {
  UART_SYNC_TX(6, LPUART6_STAT, LPUART_STAT, LPUART6_DATA)
}

#undef UART_SYNC_TX_SEND_FIFO_6

#define LPUART_TX_FLUSH_FIFO_6 LPUART_TX_FLUSH_FIFO(6)

void lpuart6_tx_break(uint32_t breakTime, uint32_t mabTime) {
  LPUART_TX_BREAK(6)
}

#undef LPUART_TX_FLUSH_FIFO_6

#endif  // IMXRT_LPUART6

// ---------------------------------------------------------------------------
//  LPUART4 synchronous TX (Serial2 on Teensy 4)
// ---------------------------------------------------------------------------

#if defined(IMXRT_LPUART4)

#define UART_SYNC_TX_SEND_FIFO_4 LPUART_SYNC_TX_SEND_FIFO(4)

void lpuart4_tx(const uint8_t *b, int len) {
  UART_SYNC_TX(4, LPUART4_STAT, LPUART_STAT, LPUART4_DATA)
}

#undef UART_SYNC_TX_SEND_FIFO_4

#define LPUART_TX_FLUSH_FIFO_4 LPUART_TX_FLUSH_FIFO(4)

void lpuart4_tx_break(uint32_t breakTime, uint32_t mabTime) {
  LPUART_TX_BREAK(4)
}

#undef LPUART_TX_FLUSH_FIFO_4

#endif  // IMXRT_LPUART4

// ---------------------------------------------------------------------------
//  LPUART2 synchronous TX (Serial3 on Teensy 4)
// ---------------------------------------------------------------------------

#if defined(IMXRT_LPUART2)

#define UART_SYNC_TX_SEND_FIFO_2 LPUART_SYNC_TX_SEND_FIFO(2)

void lpuart2_tx(const uint8_t *b, int len) {
  UART_SYNC_TX(2, LPUART2_STAT, LPUART_STAT, LPUART2_DATA)
}

#undef UART_SYNC_TX_SEND_FIFO_2

#define LPUART_TX_FLUSH_FIFO_2 LPUART_TX_FLUSH_FIFO(2)

void lpuart2_tx_break(uint32_t breakTime, uint32_t mabTime) {
  LPUART_TX_BREAK(2)
}

#undef LPUART_TX_FLUSH_FIFO_2

#endif  // IMXRT_LPUART2

// ---------------------------------------------------------------------------
//  LPUART3 synchronous TX (Serial4 on Teensy 4)
// ---------------------------------------------------------------------------

#if defined(IMXRT_LPUART3)

#define UART_SYNC_TX_SEND_FIFO_3 LPUART_SYNC_TX_SEND_FIFO(3)

void lpuart3_tx(const uint8_t *b, int len) {
  UART_SYNC_TX(3, LPUART3_STAT, LPUART_STAT, LPUART3_DATA)
}

#undef UART_SYNC_TX_SEND_FIFO_3

#define LPUART_TX_FLUSH_FIFO_3 LPUART_TX_FLUSH_FIFO(3)

void lpuart3_tx_break(uint32_t breakTime, uint32_t mabTime) {
  LPUART_TX_BREAK(3)
}

#undef LPUART_TX_FLUSH_FIFO_3

#endif  // IMXRT_LPUART3

// ---------------------------------------------------------------------------
//  LPUART8 synchronous TX (Serial5 on Teensy 4)
// ---------------------------------------------------------------------------

#if defined(IMXRT_LPUART8)

#define UART_SYNC_TX_SEND_FIFO_8 LPUART_SYNC_TX_SEND_FIFO(8)

void lpuart8_tx(const uint8_t *b, int len) {
  UART_SYNC_TX(8, LPUART8_STAT, LPUART_STAT, LPUART8_DATA)
}

#undef UART_SYNC_TX_SEND_FIFO_8

#define LPUART_TX_FLUSH_FIFO_8 LPUART_TX_FLUSH_FIFO(8)

void lpuart8_tx_break(uint32_t breakTime, uint32_t mabTime) {
  LPUART_TX_BREAK(8)
}

#undef LPUART_TX_FLUSH_FIFO_8

#endif  // IMXRT_LPUART8

// ---------------------------------------------------------------------------
//  LPUART1 synchronous TX (Serial6 on Teensy 4)
// ---------------------------------------------------------------------------

#if defined(IMXRT_LPUART1)

#define UART_SYNC_TX_SEND_FIFO_1 LPUART_SYNC_TX_SEND_FIFO(1)

void lpuart1_tx(const uint8_t *b, int len) {
  UART_SYNC_TX(1, LPUART1_STAT, LPUART_STAT, LPUART1_DATA)
}

#undef UART_SYNC_TX_SEND_FIFO_1

#define LPUART_TX_FLUSH_FIFO_1 LPUART_TX_FLUSH_FIFO(1)

void lpuart1_tx_break(uint32_t breakTime, uint32_t mabTime) {
  LPUART_TX_BREAK(1)
}

#undef LPUART_TX_FLUSH_FIFO_1

#endif  // IMXRT_LPUART1

// ---------------------------------------------------------------------------
//  LPUART7 synchronous TX (Serial7 on Teensy 4)
// ---------------------------------------------------------------------------

#if defined(IMXRT_LPUART7)

#define UART_SYNC_TX_SEND_FIFO_7 LPUART_SYNC_TX_SEND_FIFO(7)

void lpuart7_tx(const uint8_t *b, int len) {
  UART_SYNC_TX(7, LPUART7_STAT, LPUART_STAT, LPUART7_DATA)
}

#undef UART_SYNC_TX_SEND_FIFO_7

#define LPUART_TX_FLUSH_FIFO_7 LPUART_TX_FLUSH_FIFO(7)

void lpuart7_tx_break(uint32_t breakTime, uint32_t mabTime) {
  LPUART_TX_BREAK(7)
}

#undef LPUART_TX_FLUSH_FIFO_7

#endif  // IMXRT_LPUART7

// ---------------------------------------------------------------------------
//  LPUART5 synchronous TX (Serial8 on Teensy 4)
// ---------------------------------------------------------------------------

#if defined(IMXRT_LPUART5) && \
    (defined(__IMXRT1052__) || defined(ARDUINO_TEENSY41))

#define UART_SYNC_TX_SEND_FIFO_5 LPUART_SYNC_TX_SEND_FIFO(5)

void lpuart5_tx(const uint8_t *b, int len) {
  UART_SYNC_TX(5, LPUART5_STAT, LPUART_STAT, LPUART5_DATA)
}

#undef UART_SYNC_TX_SEND_FIFO_5

#define LPUART_TX_FLUSH_FIFO_5 LPUART_TX_FLUSH_FIFO(5)

void lpuart5_tx_break(uint32_t breakTime, uint32_t mabTime) {
  LPUART_TX_BREAK(5)
}

#undef LPUART_TX_FLUSH_FIFO_5

#endif  // IMXRT_LPUART5 && (__IMXRT1052__ || ARDUINO_TEENSY41)

// ---------------------------------------------------------------------------
//  UART0 RX ISR
// ---------------------------------------------------------------------------

#if defined(HAS_KINETISK_UART0) || defined(HAS_KINETISL_UART0)

#if defined(HAS_KINETISL_UART0)
#define UART_RX_CLEAR_ERRORS_0 UART0_S1 |= (UART_S1_FE | UART_S1_IDLE);
#else
// Reading a byte clears interrupt flags
#define UART_RX_CLEAR_ERRORS_0
#endif  // HAS_KINETISL_UART0

#if defined(HAS_KINETISK_UART0_FIFO)
#define UART_RX_ERROR_FLUSH_FIFO_0 UART_RX_ERROR_FLUSH_FIFO(0)
#define UART_RX_0 UART_RX_WITH_FIFO(0)
#else
#define UART_RX_ERROR_FLUSH_FIFO_0
#define UART_RX_0 UART_RX_NO_FIFO(0)
#if defined(HAS_KINETISL_UART0)
#define UART_RX_CLEAR_IDLE_0 UART0_S1 |= UART_S1_IDLE;
#else
#define UART_RX_CLEAR_IDLE_0 UART0_D;
#endif  // HAS_KINETISL_UART0
#endif  // HAS_KINETISK_UART0_FIFO

#if defined(__MK20DX128__) || defined(__MK20DX256__)
#define UART_RX_TEST_FIRST_STOP_BIT_0 ((UART0_C3 & UART_C3_R8) != 0)
#else
#define UART_RX_TEST_FIRST_STOP_BIT_0 (true)
#endif

void uart0_rx_isr() {
  uint8_t status = UART0_S1;
  UART_RX(0, 0, UART_S1, UART0_D)
}

#undef UART_RX_CLEAR_ERRORS_0
#undef UART_RX_ERROR_FLUSH_FIFO_0
#undef UART_RX_0
#undef UART_RX_CLEAR_IDLE_0
#undef UART_RX_TEST_FIRST_STOP_BIT_0

#endif  // HAS_KINETISK_UART0 || HAS_KINETISL_UART0

// ---------------------------------------------------------------------------
//  UART1 RX ISR
// ---------------------------------------------------------------------------

#if defined(HAS_KINETISK_UART1) || defined(HAS_KINETISL_UART1)

// Reading a byte clears interrupt flags
#define UART_RX_CLEAR_ERRORS_1
#if defined(HAS_KINETISK_UART1_FIFO)
#define UART_RX_ERROR_FLUSH_FIFO_1 UART_RX_ERROR_FLUSH_FIFO(1)
#define UART_RX_1 UART_RX_WITH_FIFO(1)
#else
#define UART_RX_ERROR_FLUSH_FIFO_1
#define UART_RX_1 UART_RX_NO_FIFO(1)
#define UART_RX_CLEAR_IDLE_1 UART1_D;
#endif  // HAS_KINETISK_UART1_FIFO

#if defined(__MK20DX128__) || defined(__MK20DX256__)
#define UART_RX_TEST_FIRST_STOP_BIT_1 ((UART1_C3 & UART_C3_R8) != 0)
#else
#define UART_RX_TEST_FIRST_STOP_BIT_1 (true)
#endif

void uart1_rx_isr() {
  uint8_t status = UART1_S1;
  UART_RX(1, 1, UART_S1, UART1_D)
}

#undef UART_RX_CLEAR_ERRORS_1
#undef UART_RX_ERROR_FLUSH_FIFO_1
#undef UART_RX_1
#undef UART_RX_CLEAR_IDLE_1
#undef UART_RX_TEST_FIRST_STOP_BIT_1

#endif  // HAS_KINETISK_UART1 || HAS_KINETISL_UART1

// ---------------------------------------------------------------------------
//  UART2 RX ISR
// ---------------------------------------------------------------------------

#if defined(HAS_KINETISK_UART2) || defined(HAS_KINETISL_UART2)

// Reading a byte clears interrupt flags
#define UART_RX_CLEAR_ERRORS_2
#define UART_RX_ERROR_FLUSH_FIFO_2
#define UART_RX_2 UART_RX_NO_FIFO(2)
#define UART_RX_CLEAR_IDLE_2 UART2_D;

#if defined(__MK20DX128__) || defined(__MK20DX256__)
#define UART_RX_TEST_FIRST_STOP_BIT_2 ((UART2_C3 & UART_C3_R8) != 0)
#else
#define UART_RX_TEST_FIRST_STOP_BIT_2 (true)
#endif

void uart2_rx_isr() {
  uint8_t status = UART2_S1;
  UART_RX(2, 2, UART_S1, UART2_D)
}

#undef UART_RX_CLEAR_ERRORS_2
#undef UART_RX_ERROR_FLUSH_FIFO_2
#undef UART_RX_2
#undef UART_RX_CLEAR_IDLE_2
#undef UART_RX_TEST_FIRST_STOP_BIT_2

#endif  // HAS_KINETISK_UART2 || HAS_KINETISL_UART2

// ---------------------------------------------------------------------------
//  UART3 RX ISR
// ---------------------------------------------------------------------------

#if defined(HAS_KINETISK_UART3)

// Reading a byte clears interrupt flags
#define UART_RX_CLEAR_ERRORS_3
#define UART_RX_ERROR_FLUSH_FIFO_3
#define UART_RX_3 UART_RX_NO_FIFO(3)
#define UART_RX_CLEAR_IDLE_3 UART3_D;
#define UART_RX_TEST_FIRST_STOP_BIT_3 (true)

void uart3_rx_isr() {
  uint8_t status = UART3_S1;
  UART_RX(3, 3, UART_S1, UART3_D)
}

#undef UART_RX_CLEAR_ERRORS_3
#undef UART_RX_ERROR_FLUSH_FIFO_3
#undef UART_RX_3
#undef UART_RX_CLEAR_IDLE_3
#undef UART_RX_TEST_FIRST_STOP_BIT_3

#endif  // HAS_KINETISK_UART3

// ---------------------------------------------------------------------------
//  UART4 RX ISR
// ---------------------------------------------------------------------------

#if defined(HAS_KINETISK_UART4)

// Reading a byte clears interrupt flags
#define UART_RX_CLEAR_ERRORS_4
#define UART_RX_ERROR_FLUSH_FIFO_4
#define UART_RX_4 UART_RX_NO_FIFO(4)
#define UART_RX_CLEAR_IDLE_4 UART4_D;
#define UART_RX_TEST_FIRST_STOP_BIT_4 (true)

void uart4_rx_isr() {
  uint8_t status = UART4_S1;
  UART_RX(4, 4, UART_S1, UART4_D)
}

#undef UART_RX_CLEAR_ERRORS_4
#undef UART_RX_ERROR_FLUSH_FIFO_4
#undef UART_RX_4
#undef UART_RX_CLEAR_IDLE_4
#undef UART_RX_TEST_FIRST_STOP_BIT_4

#endif  // HAS_KINETISK_UART4

// ---------------------------------------------------------------------------
//  UART5 RX ISR
// ---------------------------------------------------------------------------

#if defined(HAS_KINETISK_UART5)

// Reading a byte clears interrupt flags
#define UART_RX_CLEAR_ERRORS_5
#define UART_RX_ERROR_FLUSH_FIFO_5
#define UART_RX_5 UART_RX_NO_FIFO(5)
#define UART_RX_CLEAR_IDLE_5 UART5_D;
#define UART_RX_TEST_FIRST_STOP_BIT_5 (true)

void uart5_rx_isr() {
  uint8_t status = UART5_S1;
  UART_RX(5, 5, UART_S1, UART5_D)
}

#undef UART_RX_CLEAR_ERRORS_5
#undef UART_RX_ERROR_FLUSH_FIFO_5
#undef UART_RX_5
#undef UART_RX_CLEAR_IDLE_5
#undef UART_RX_TEST_FIRST_STOP_BIT_5

#endif  // HAS_KINETISK_UART5

// ---------------------------------------------------------------------------
//  LPUART0 RX ISR (Serial6 on Teensy 3.6)
// ---------------------------------------------------------------------------

#if defined(HAS_KINETISK_LPUART0)

#define UART_RX_CLEAR_ERRORS_0 \
  LPUART0_STAT |= (LPUART_STAT_FE | LPUART_STAT_IDLE);
#define UART_RX_ERROR_FLUSH_FIFO_0
#define UART_RX_0 LPUART_RX_NO_FIFO(0)

void lpuart0_rx_isr() {
  uint32_t status = LPUART0_STAT;
  UART_RX(5, 0, LPUART_STAT, LPUART0_DATA)
}

#undef UART_RX_CLEAR_ERRORS_0
#undef UART_RX_ERROR_FLUSH_FIFO_0
#undef UART_RX_0

#endif  // HAS_KINETISK_LPUART0

// ---------------------------------------------------------------------------
//  LPUART6 RX ISR (Serial1 on Teensy 4)
// ---------------------------------------------------------------------------

#if defined(IMXRT_LPUART6)

#define UART_RX_CLEAR_ERRORS_6 \
  LPUART6_STAT |= (LPUART_STAT_FE | LPUART_STAT_IDLE);
#define UART_RX_ERROR_FLUSH_FIFO_6 LPUART_RX_ERROR_FLUSH_FIFO(6)
#define UART_RX_6 LPUART_RX_WITH_FIFO(6)

void lpuart6_rx_isr() {
  uint32_t status = LPUART6_STAT;
  UART_RX(0, 6, LPUART_STAT, LPUART6_DATA)
}

#undef UART_RX_CLEAR_ERRORS_6
#undef UART_RX_ERROR_FLUSH_FIFO_6
#undef UART_RX_6

#endif  // IMXRT_LPUART6

// ---------------------------------------------------------------------------
//  LPUART4 RX ISR (Serial2 on Teensy 4)
// ---------------------------------------------------------------------------

#if defined(IMXRT_LPUART4)

#define UART_RX_CLEAR_ERRORS_4 \
  LPUART4_STAT |= (LPUART_STAT_FE | LPUART_STAT_IDLE);
#define UART_RX_ERROR_FLUSH_FIFO_4 LPUART_RX_ERROR_FLUSH_FIFO(4)
#define UART_RX_4 LPUART_RX_WITH_FIFO(4)

void lpuart4_rx_isr() {
  uint32_t status = LPUART4_STAT;
  UART_RX(1, 4, LPUART_STAT, LPUART4_DATA)
}

#undef UART_RX_CLEAR_ERRORS_4
#undef UART_RX_ERROR_FLUSH_FIFO_4
#undef UART_RX_4

#endif  // IMXRT_LPUART4

// ---------------------------------------------------------------------------
//  LPUART2 RX ISR (Serial3 on Teensy 4)
// ---------------------------------------------------------------------------

#if defined(IMXRT_LPUART2)

#define UART_RX_CLEAR_ERRORS_2 \
  LPUART2_STAT |= (LPUART_STAT_FE | LPUART_STAT_IDLE);
#define UART_RX_ERROR_FLUSH_FIFO_2 LPUART_RX_ERROR_FLUSH_FIFO(2)
#define UART_RX_2 LPUART_RX_WITH_FIFO(2)

void lpuart2_rx_isr() {
  uint32_t status = LPUART2_STAT;
  UART_RX(2, 2, LPUART_STAT, LPUART2_DATA)
}

#undef UART_RX_CLEAR_ERRORS_2
#undef UART_RX_ERROR_FLUSH_FIFO_2
#undef UART_RX_2

#endif  // IMXRT_LPUART2

// ---------------------------------------------------------------------------
//  LPUART3 RX ISR (Serial4 on Teensy 4)
// ---------------------------------------------------------------------------

#if defined(IMXRT_LPUART3)

#define UART_RX_CLEAR_ERRORS_3 \
  LPUART3_STAT |= (LPUART_STAT_FE | LPUART_STAT_IDLE);
#define UART_RX_ERROR_FLUSH_FIFO_3 LPUART_RX_ERROR_FLUSH_FIFO(3)
#define UART_RX_3 LPUART_RX_WITH_FIFO(3)

void lpuart3_rx_isr() {
  uint32_t status = LPUART3_STAT;
  UART_RX(3, 3, LPUART_STAT, LPUART3_DATA)
}

#undef UART_RX_CLEAR_ERRORS_3
#undef UART_RX_ERROR_FLUSH_FIFO_3
#undef UART_RX_3

#endif  // IMXRT_LPUART3

// ---------------------------------------------------------------------------
//  LPUART8 RX ISR (Serial5 on Teensy 4)
// ---------------------------------------------------------------------------

#if defined(IMXRT_LPUART8)

#define UART_RX_CLEAR_ERRORS_8 \
  LPUART8_STAT |= (LPUART_STAT_FE | LPUART_STAT_IDLE);
#define UART_RX_ERROR_FLUSH_FIFO_8 LPUART_RX_ERROR_FLUSH_FIFO(8)
#define UART_RX_8 LPUART_RX_WITH_FIFO(8)

void lpuart8_rx_isr() {
  uint32_t status = LPUART8_STAT;
  UART_RX(4, 8, LPUART_STAT, LPUART8_DATA)
}

#undef UART_RX_CLEAR_ERRORS_8
#undef UART_RX_ERROR_FLUSH_FIFO_8
#undef UART_RX_8

#endif  // IMXRT_LPUART8

// ---------------------------------------------------------------------------
//  LPUART1 RX ISR (Serial6 on Teensy 4)
// ---------------------------------------------------------------------------

#if defined(IMXRT_LPUART1)

#define UART_RX_CLEAR_ERRORS_1 \
  LPUART1_STAT |= (LPUART_STAT_FE | LPUART_STAT_IDLE);
#define UART_RX_ERROR_FLUSH_FIFO_1 LPUART_RX_ERROR_FLUSH_FIFO(1)
#define UART_RX_1 LPUART_RX_WITH_FIFO(1)

void lpuart1_rx_isr() {
  uint32_t status = LPUART1_STAT;
  UART_RX(5, 1, LPUART_STAT, LPUART1_DATA)
}

#undef UART_RX_CLEAR_ERRORS_1
#undef UART_RX_ERROR_FLUSH_FIFO_1
#undef UART_RX_1

#endif  // IMXRT_LPUART1

// ---------------------------------------------------------------------------
//  LPUART7 RX ISR (Serial7 on Teensy 4)
// ---------------------------------------------------------------------------

#if defined(IMXRT_LPUART7)

#define UART_RX_CLEAR_ERRORS_7 \
  LPUART7_STAT |= (LPUART_STAT_FE | LPUART_STAT_IDLE);
#define UART_RX_ERROR_FLUSH_FIFO_7 LPUART_RX_ERROR_FLUSH_FIFO(7)
#define UART_RX_7 LPUART_RX_WITH_FIFO(7)

void lpuart7_rx_isr() {
  uint32_t status = LPUART7_STAT;
  UART_RX(6, 7, LPUART_STAT, LPUART7_DATA)
}

#undef UART_RX_CLEAR_ERRORS_7
#undef UART_RX_ERROR_FLUSH_FIFO_7
#undef UART_RX_7

#endif  // IMXRT_LPUART7

// ---------------------------------------------------------------------------
//  LPUART5 RX ISR (Serial8 on Teensy 4)
// ---------------------------------------------------------------------------

#if defined(IMXRT_LPUART5) && \
    (defined(__IMXRT1052__) || defined(ARDUINO_TEENSY41))

#define UART_RX_CLEAR_ERRORS_5 \
  LPUART5_STAT |= (LPUART_STAT_FE | LPUART_STAT_IDLE);
#define UART_RX_ERROR_FLUSH_FIFO_5 LPUART_RX_ERROR_FLUSH_FIFO(5)
#define UART_RX_5 LPUART_RX_WITH_FIFO(5)

void lpuart5_rx_isr() {
  uint32_t status = LPUART5_STAT;
  UART_RX(7, 5, LPUART_STAT, LPUART5_DATA)
}

#undef UART_RX_CLEAR_ERRORS_5
#undef UART_RX_ERROR_FLUSH_FIFO_5
#undef UART_RX_5

#endif  // IMXRT_LPUART5 && (__IMXRT1052__ || ARDUINO_TEENSY41)

}  // namespace teensydmx
}  // namespace qindesign
