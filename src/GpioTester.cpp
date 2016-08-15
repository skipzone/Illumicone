/*
    This file is part of Illumicone.

    Illumicone is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Illumicone is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Illumicone.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include "illumiconeTypes.h"

using namespace std;

int main(int argc, char **argv)
{
    int sock;
    struct sockaddr_in server;
    int position;
    int col;
    int row;
    int n;
    int i;
    uint8_t r;
    uint8_t g;
    uint8_t b;

    uint8_t opcArray[NUM_STRINGS * PIXELS_PER_STRING * 3 + 4];
    uint8_t *pixels;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    server.sin_addr.s_addr = inet_addr(OPC_SERVER_ADDR);
    server.sin_family = AF_INET;
    server.sin_port = htons(7890);

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        cout << "SOMETHING'S FUCKY: couldn't connect to opc-server!" << endl;
        while (1);
    }

    position = atoi(argv[1]);
    cout << "position: " << position << endl;
    cout << "sizeof(opcArray): " << sizeof(opcArray) << endl;

        opcArray[0] = 0;
        opcArray[1] = 0;
        opcArray[2] = NUM_STRINGS * PIXELS_PER_STRING * 3 / 256;
        opcArray[3] = NUM_STRINGS * PIXELS_PER_STRING * 3 % 256;
        pixels = &opcArray[4];

    while (1) {

//        for (col = 0; col < NUM_STRINGS; col++) {
//            r = rand() % 255;
//            g = rand() % 255;
//            b = rand() % 255;
//            for (row = 0; row < PIXELS_PER_STRING*3; row+=3) {
//                pixels[col*PIXELS_PER_STRING*3 + row + 0] = r;
//                pixels[col*PIXELS_PER_STRING*3 + row + 1] = g;
//                pixels[col*PIXELS_PER_STRING*3 + row + 2] = b;
//            }
//        }

//        for (col = 0; col < NUM_STRINGS; col++) {
//            for (row = 0; row < PIXELS_PER_STRING*3; row+=3) {
//                pixels[col*PIXELS_PER_STRING*3 + row + 0] = rand() % 255;
//                pixels[col*PIXELS_PER_STRING*3 + row + 1] = rand() & 255;
//                pixels[col*PIXELS_PER_STRING*3 + row + 2] = rand() & 255;
//            }
//        }

    for (i = 0; i < sizeof(opcArray) - 4; i++) {
        pixels[i] = 0;
    }

    for (i = 0; i < PIXELS_PER_STRING*3; i+=3) {
        pixels[position * PIXELS_PER_STRING * 3 + i + 0] = 128;
        pixels[position * PIXELS_PER_STRING * 3 + i + 1] = 128;
        pixels[position * PIXELS_PER_STRING * 3 + i + 2] = 128;
    }

        n = send(sock, opcArray, sizeof(opcArray), 0);
        usleep(500000);
    }

}
