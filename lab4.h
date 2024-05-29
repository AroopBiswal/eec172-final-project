/*
 * lab4.h
 *
 *  Created on: May 29, 2024
 *      Author: benyo
 */

#ifndef EEC172_LAB4_LAB4_H_
#define EEC172_LAB4_LAB4_H_

#include "common_includes.h"

#define APPLICATION_NAME      "SSL"
#define APPLICATION_VERSION   "SQ24"
//#define SERVER_NAME           "a26ypaoxj1nj7v-ats.iot.us-west-2.amazonaws.com" // CHANGE ME
#define SERVER_NAME           "a62ofxyy56n2n-ats.iot.us-east-2.amazonaws.com"
#define GOOGLE_DST_PORT       8443

//NEED TO UPDATE THIS FOR IT TO WORK!
#define DATE                22    /* Current Date */
#define MONTH               5     /* Month 1-12 */
#define YEAR                2024  /* Current year */
#define HOUR                14    /* Time - hours */
#define MINUTE              1    /* Time - minutes */
#define SECOND              0     /* Time - seconds */


//#define POSTHEADER "POST /things/CC3200_Thing/shadow HTTP/1.1\r\n"             // CHANGE ME
#define POSTHEADER "POST /things/CC3200Board/shadow HTTP/1.1\r\n"             // CHANGE ME
//#define HOSTHEADER "Host: a26ypaoxj1nj7v-ats.iot.us-west-2.amazonaws.com\r\n"  // CHANGE ME
#define HOSTHEADER "Host: a62ofxyy56n2n-ats.iot.us-east-2.amazonaws.com\r\n"  // CHANGE ME
#define GETHEADER "GET /things/CC3200Board/shadow HTTP/1.1\r\n"
#define CHEADER "Connection: Keep-Alive\r\n"
#define CTHEADER "Content-Type: application/json; charset=utf-8\r\n"
#define CLHEADER1 "Content-Length: "
#define CLHEADER2 "\r\n\r\n"

#define DATA1 "{" \
            "\"state\": {\r\n"                                              \
                "\"desired\" : {\r\n"                                       \
                    "\"message\" :\""                                           \
                        "Hello phone, "                                     \
                        ":)!"                  \
                        "\"\r\n"                                            \
                "}"                                                         \
            "}"                                                             \
        "}\r\n\r\n"

#define DATAHEADER "{" \
            "\"state\": {\r\n"                                              \
                "\"desired\" : {\r\n"                                       \
                    "\"message\" :" \
                     " {\"default\":\"default message\", "   \
                     "\"email\":\""

#define DATATAIL    "\"}\r\n"                                                \
                "}"                                                         \
            "}"                                                             \
        "}\r\n\r\n"

static int set_time();
static void BoardInit(void);
static int http_post(int iTLSSockID, char* sending_buf);
static int http_get(int);

#endif /* EEC172_LAB4_LAB4_H_ */
