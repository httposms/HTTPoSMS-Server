#ifndef COMMON_H
#define COMMON_H

#define NO_ERROR        0
#define MINOR_ERROR     1
#define MAJOR_ERROR     2

#define READ_FD         0
#define WRITE_FD        1


#include <stdlib.h>
#include <stdint.h>

struct transaction_response {
        uint8_t id;
        struct sms_response *resp;
};

struct transaction_request {
        uint8_t id;
        struct sms_message* req;
};

struct sms_response {
        uint8_t bytes;
        size_t len;
};

/*
 * SMS message as recieved by the modem
 * Phone numbers globally are a max of twelve digits
 *
 * content is included as part of a variably sized struct
 */
struct sms_message {
        char src[12];
        size_t len;
        char content[];
};

#endif
