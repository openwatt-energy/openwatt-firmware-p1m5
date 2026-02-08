#ifndef P1_DISPATCH_H
#define P1_DISPATCH_H

#include "p1_reader.h"

// Called from P1 reader task when a valid telegram is parsed.
// Implemented in main.cpp; forwards to WebAPI, MQTT, WebSocket.
void onP1DataReceived(const P1Data& data);

#endif
