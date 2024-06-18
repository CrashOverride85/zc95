#include "btstack.h"

class BluetoothSend
{

    public:
        BluetoothSend();
        void setup();
        void send(uint32_t val);
        bool is_connected();

        static void s_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);

    private:
        void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
        void send_report();

        btstack_packet_callback_registration_t _event_callback_registration;
        hci_con_handle_t _con_handle = HCI_CON_HANDLE_INVALID;
        uint32_t _last_value_sent = 0xFFFFFFFF;
        bool _connected = false;
};
