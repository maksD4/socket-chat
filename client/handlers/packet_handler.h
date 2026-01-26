#ifndef PACKET_HANDLER_H
#define PACKET_HANDLER_H
#include <stdint.h>

#define ID4(a, b, c, d ) ((uint32_t)(a) << 24 | (uint32_t)(b) << 16 | (uint32_t)(c) << 8 | (uint32_t)(d))

enum{
    MSG_ACCC = ID4('a','c','c','c'),
    MSG_LOGI = ID4('l','o','g','i'),
    MSG_LOGO = ID4('l','o','g','o'),
    MSG_SMSG = ID4('s','m','s','g'),
    MSG_RCRE = ID4('r','c','r','e'),
    MSG_RDEL = ID4('r','d','e','l'),
    MSG_FADD = ID4('f','a','d','d'),
    MSG_FRMV = ID4('f','r','m','v'),
    MSG_DATA = ID4('d','a','t','a'),

    MSG_FRND = ID4('f','r','n','d'),
    MSG_ROOM = ID4('r','o','o','m'),
    MSG_RMSG = ID4('r','m','s','g'),
    MSG_NTFI = ID4('n','t','f','i')
};

int check_reply_state(char* message);
int recognize_packet(char* message);

#endif
