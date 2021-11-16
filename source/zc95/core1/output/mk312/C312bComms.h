#ifndef _C312BCOMMS_H
#define _C312BCOMMS_H



#include "../CSimpleOutputChannel.h"
#include "C312bTxBuffer.h"
#include "hardware/uart.h"
#include <cstddef>


#define ETMEM_panellock 0x400F
#define ETMEM_knoba 0x4064
#define ETMEM_knobb 0x4065
#define ETMEM_knobma 0x4061

// push panel buttons: 

#define ETMEM_pushbutton 0x4070
#define ETBUTTON_ok 3
#define ETBUTTON_setmode 4
// 5 not sure, ramps 0-100
#define ETBUTTON_up 8
#define ETBUTTON_down 9
#define ETBUTTON_menu 10
#define ETBUTTON_split 12
// 15 makes display say 'adjust?'
#define ETBUTTON_lockmode 16
#define ETBUTTON_splitmode 17

#define ETMEM_modesplit 0x4078

#define ETMODE_waves 0x76
#define ETMODE_stroke 0x77
#define ETMODE_climb 0x78
#define ETMODE_combo 0x79
#define ETMODE_intense 0x7A
#define ETMODE_rhythm 0x7B
#define ETMODE_audio1 0x7C
#define ETMODE_audio2 0x7D
#define ETMODE_audio3 0x7E
#define ETMODE_split 0x7F
#define ETMODE_random1 0x80
#define ETMODE_random2 0x81
#define ETMODE_toggle 0x82
#define ETMODE_orgasm 0x83
#define ETMODE_torment 0x84
#define ETMODE_phase1 0x85
#define ETMODE_phase2 0x86
#define ETMODE_phase3 0x87
#define ETMODE_user1 0x88
#define ETMODE_user2 0x89
#define ETMODE_user3 0x8A
#define ETMODE_user4 0x8B
#define ETMODE_user5 0x8C

#define ETMEM_mode 0x407B

// MA knob is strange, we can get/set the value 0-100 as per the other knobs, but in
// real life the range is limited depending on the mode.  There must be a memory
// location that specifies the allowed range of the knob, we'll also need to set it
// when we change mode (it doesn't change if mode changes and panel is locked)
// So location 4087 is the max MA, 4086 is the min MA (through watching all locations!)

#define ETMEM_knobmamin 0x4086
#define ETMEM_knobmamax 0x4087
#define ETMEM_gatea 0x4090
#define ETMEM_timeona 0x4098
#define ETMEM_timeoffa 0x4099
#define ETMEM_timeopta 0x409A 
#define ETMEM_ramp 0x4006 // 0-255 also ramp at 400b, 409c, 419c
#define ETMEM_levela 0x40A5
#define ETMEM_levelmina 0x40A6
#define ETMEM_levelmaxa 0x40A7
#define ETMEM_levelratea 0x40A8
#define ETMEM_levelopta 0x40AB
#define ETMEM_freqa 0x40AE
#define ETMEM_freqmaxa 0x40AF
#define ETMEM_freqmina 0x40B0
#define ETMEM_freqopta 0x40B5
#define ETMEM_widtha 0x40B7
#define ETMEM_widthmina 0x40B8
#define ETMEM_widthmaxa 0x40B9
#define ETMEM_widthopta 0x40BE
#define ETMEM_gateb 0x4190
#define ETMEM_timeonb 0x4198
#define ETMEM_timeoffb 0x4199
#define ETMEM_timeoptb 0x419A
#define ETMEM_levelb 0x41A5
#define ETMEM_levelminb 0x41A6
#define ETMEM_levelmaxb 0x41A7
#define ETMEM_levelrateb 0x41A8
#define ETMEM_leveloptb 0x41AB
#define ETMEM_freqb 0x41AE
#define ETMEM_freqmaxb 0x41AF
#define ETMEM_freqminb 0x41B0
#define ETMEM_freqoptb 0x41B5
#define ETMEM_widthb 0x41b7
#define ETMEM_widthminb 0x41B8
#define ETMEM_widthmaxb 0x41B9
#define ETMEM_widthoptb 0x41BE
#define ETMEM_powerlevel 0x41F4
#define ETMEM_ramptime 0x41F9
#define ETMEM_knobmaset 0x420d

#define MAXRXBYTES 4


class C312bComms
{
    public:
        C312bComms(uart_inst_t *uart);
        //void set_uart(uart_inst_t *uart);
        void connect();
        void loop();
        bool send(uint16_t addr, uint8_t value);

    private:

        enum class comms_state
        {
            FAILED        = 0,   // Attempt made to connect, but it failed
            NOT_CONNECTED = 1,   // Not connected. Either no attempt made so far, or disconnect() called
            HELLO_1       = 2,   // sent 0x00, waiting for 0x07
            HELLO_2       = 3,   // Got 0x07, sent 0x2f, 0x00, waiting for 0x21, .....
            HELLO_3       = 4,   // Probably conncted, waiting for response from test read
            CONNECTED     = 5
        };


 

        C312bTxBuffer _tx_buffer;

        uart_inst_t *_uart;

        //void set_byte(struct tx_command cmd);

        void send_tx_buffered();

        void set_comms_state(comms_state state);
        void clear_rx_buffer();

        void hello1();
        void hello2();
        void hello3();
        bool is_connected();
        void cp(uint8_t msg[], uint8_t n);

        bool _configured;

        uint8_t _rx_buf[MAXRXBYTES];
        uint8_t _rx_buf_pos;

        comms_state _comms_state;
        uint8_t _mod;
        
        uint64_t _last_tx_us;
        uint64_t _state_change_us;

        uint8_t _hello1_tx_count;
        uint8_t _hello1_rx_count;

        bool _hello2_sent;
        bool _hello3_sent;



        
    

};

#endif
