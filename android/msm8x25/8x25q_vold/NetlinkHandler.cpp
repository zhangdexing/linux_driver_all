/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <utils/Log.h>
#include <utils/threads.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define LOG_TAG "Vold"

#include <cutils/log.h>

#include <sysutils/NetlinkEvent.h>
#include "NetlinkHandler.h"
#include "VolumeManager.h"
#include "../inc/lidbg_servicer.h"

NetlinkHandler::NetlinkHandler(int listenerSocket) :
                NetlinkListener(listenerSocket) {
}

NetlinkHandler::~NetlinkHandler() {
}

int NetlinkHandler::start() {
    return this->startListener();
}

int NetlinkHandler::stop() {
    return this->stopListener();
}

void NetlinkHandler::onEvent(NetlinkEvent *evt) {
    VolumeManager *vm = VolumeManager::Instance();
    const char *subsys = evt->getSubsystem();

    if (!subsys) {
        SLOGW("No subsystem found in netlink event");
        return;
    }

//add log by wangyihong for printing log
SLOGW("NetlinkHandler::onEvent: subsys=%s, mPath=%s",  subsys, evt->findParam("DEVPATH"));
SLOGW("NetlinkHandler::onEvent: action = %d,type =%s",  evt->getAction(),evt->findParam("DEVTYPE"));
if(!strcmp(subsys, "block"))
{
	LIDBG_PRINT("NetlinkHandler::onEvent: subsys=%s, mPath=%s",  subsys, evt->findParam("DEVPATH"));
	LIDBG_PRINT("NetlinkHandler::onEvent: action = %d,type =%s",  evt->getAction(),evt->findParam("DEVTYPE"));
}

    if (!strcmp(subsys, "block")) {
        vm->handleBlockEvent(evt);
    }
}
