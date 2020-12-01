// This file is part of the TeensyDMX library.
// (c) 2019 Shawn Silverman

#if defined(__MK20DX128__) || defined(__MK20DX256__) || \
    defined(__MKL26Z64__) || defined(__MK64FX512__) || defined(__MK66FX1M0__)

#ifndef TEENSYDMX_UARTSENDHANDLER_H_
#define TEENSYDMX_UARTSENDHANDLER_H_

#include <kinetis.h>
#include "SendHandler.h"

namespace qindesign {
namespace teensydmx {

class UARTSendHandler final : public SendHandler {
 public:
  UARTSendHandler(int serialIndex,
                  Sender *sender,
                  KINETISK_UART_t *port,
                  IRQ_NUMBER_t irq,
                  void (*irqHandler)())
      : SendHandler(serialIndex, sender),
        port_(port),
#if defined(KINETISK)
        fifoSizeSet_(false),
        fifoSize_(1),
#endif
        irq_(irq),
        irqHandler_(irqHandler),
        slotsSerialParamsSet_(false) {}

  ~UARTSendHandler() override = default;

  void start() override;
  void end() const override;
  void setActive() const override;
  void setIRQsEnabled(bool flag) const override;
  int priority() const override;
  void irqHandler() override;

 private:
  // Stored UART parameters for quickly setting the baud rate between BREAK
  // and slots. Used for Teensy 3 and Teensy LC.
  struct SerialParams final {
    uint8_t bdh = 0;
    uint8_t bdl = 0x04;  // 13-bit SBR is 0x0004
    uint8_t s2 = 0;
    uint8_t c3 = 0;
    uint8_t c4 = 0;

    void getFrom(int serialIndex, KINETISK_UART_t *port) {
      bdh = port->BDH;
      bdl = port->BDL;
      s2 = port->S2;
      c3 = port->C3;
#if defined(__MKL26Z64__)
      if (serialIndex == 0) {
        c4 = port->C4;
      }
#else
      c4 = port->C4;
#endif
    }

    void apply(int serialIndex, KINETISK_UART_t *port) const {
      port->BDH = bdh;
      port->BDL = bdl;
      port->S2 = s2;
      port->C3 = c3;
#if defined(__MKL26Z64__)
      if (serialIndex == 0) {
        port->C4 = c4;
      }
#else
      port->C4 = c4;
#endif
    }
  };

  KINETISK_UART_t *port_;
#if defined(KINETISK)
  bool fifoSizeSet_;
  uint8_t fifoSize_;
#endif
  IRQ_NUMBER_t irq_;
  void (*irqHandler_)();

  bool slotsSerialParamsSet_;
  SerialParams breakSerialParams_;
  SerialParams slotsSerialParams_;
};

}  // namespace teensydmx
}  // namespace qindesign


#endif  // TEENSYDMX_UARTSENDHANDLER_H_

#endif  // __MK20DX128__ || __MK20DX256__ || __MKL26Z64__ || __MK64FX512__ ||
        // __MK66FX1M0__