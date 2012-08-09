
#include <libmaple/libmaple_types.h>
#include <usart.h>
#include <wirish/Print.h>
#include <wirish/boards.h>



class HardwareSerialFlowControl : public Print {
 public:
    HardwareSerialFlowControl(usart_dev *usart_device, uint8 tx_pin, uint8 rx_pin, uint8 cts_pin,  uint8 rts_pin);

    /* Set up/tear down */
    void begin(uint32 baud);
    void end(void);

    /* I/O */
    uint32 available(void);
    uint8 read(void);
    void flush(void);
    virtual void write(unsigned char);
    using Print::write;

    /* Pin accessors */
    int txPin(void) { return this->tx_pin; }
    int rxPin(void) { return this->rx_pin; }

    /* Escape hatch into libmaple */
    /* FIXME [0.0.13] documentation */
    struct usart_dev* c_dev(void) { return this->usart_device; }
 protected:
    struct usart_dev *usart_device;
    uint8 tx_pin;
    uint8 rx_pin;

    uint8 cts_pin;
    uint8 rts_pin; 
    
};

