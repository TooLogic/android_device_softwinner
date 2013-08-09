/*
 * Copyright (C) 2011 The Android Open Source Project
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

#define LOG_TAG "audio_hw_primary"
#define LOG_NDEBUG 0

#include <stdlib.h>
#include <string.h>
#include <utils/Log.h>
#include "audio_ril.h"

#include <cutils/properties.h>


int get_current_tty_dev_yuga(char * tty_dev)
{
    char temp_name[32];
    int no = -1;
    int i = 0;
    int ret = 0;

    property_get("ril.pcui", temp_name, "/dev/ttyUSB2");
    if(NULL == tty_dev){
        return -1;
    }else{
        strcpy(tty_dev, temp_name);
    }

    LOGI("get_current_tty_dev_yuga: tty_dev=%s\n", tty_dev);

    return 0;
}

int get_current_tty_dev_mu509(char * tty_dev)
{
    char temp_name[32];
    int no = -1;
    int i = 0;
    int ret = 0;

    if(NULL == tty_dev){
        return -1;
    }

    for(i = 0; i < 20; i++){
    	memset(temp_name, 0, 32);
    	sprintf(temp_name, "/dev/ttyUSB%d", i);

		ret = access(temp_name, F_OK);
		if(ret != 0){
			continue;
		}

		no = i;
    }

	if(no >= 0){
		sprintf(temp_name, "/dev/ttyUSB%d", no);
		strcpy(tty_dev, temp_name);
		LOGI("get_current_tty_dev_mu509: tty_dev=%s\n", tty_dev);
		return 0;
	}else{
		LOGI("get_current_tty_dev_mu509: can not find the AT device\n");
		return -1;
	}

	return 0;
}

int get_current_tty_dev(char * tty_dev)
{
	int ret = -1;
	char libPath[PROPERTY_VALUE_MAX];
	char libPath_yuga[] = "liballwinner-ril-yuga.so";

	memset(libPath, 0, PROPERTY_VALUE_MAX);
	ret = property_get("rild.libpath", libPath, libPath_yuga);
	LOGI("get_current_tty_dev: libPath=%s\n", libPath);
	if(!strcmp(libPath, libPath_yuga)){
		ret = get_current_tty_dev_yuga(tty_dev);
	}else{
		ret = get_current_tty_dev_mu509(tty_dev);
	}

	return ret;
}

int ril_set_call_volume(enum ril_sound_type sound_type, int volume)
{
	char tty_dev[32];
	memset(tty_dev, 0, 32);
	get_current_tty_dev(tty_dev);

	// to do real volume
	char cmdline[128];
	memset(cmdline, 0, 128);
	sprintf(cmdline, "/system/bin/busybox echo -e \"AT+CLVL=%d\r\n\" > %s", volume, tty_dev);
	system(cmdline);

    return 0;
}

int ril_set_call_audio_path(enum ril_audio_path path)
{
	char tty_dev[32];
	memset(tty_dev, 0, 32);
	get_current_tty_dev(tty_dev);

	if (path == SOUND_AUDIO_PATH_HANDSET)
	{
		char cmdline[128];
		memset(cmdline, 0, 128);
		sprintf(cmdline, "/system/bin/busybox echo -e \"AT^SWSPATH=0\r\n\" > %s", tty_dev);
		system(cmdline);

		LOGV("ril_set_call_audio_path: handset");
	}
	else if(path == SOUND_AUDIO_PATH_BLUETOOTH || path == SOUND_AUDIO_PATH_BLUETOOTH_NO_NR)
	{
		char cmdline[128];
		memset(cmdline, 0, 128);
		sprintf(cmdline, "/system/bin/busybox echo -e \"AT^SWSPATH=2\r\n\" > %s", tty_dev);
		system(cmdline);

		LOGV("ril_set_call_audio_path: bluetooth");
	}
	else
	{
		char cmdline[128];
		memset(cmdline, 0, 128);
		sprintf(cmdline, "/system/bin/busybox echo -e \"AT^SWSPATH=1\r\n\" > %s", tty_dev);
		system(cmdline);
		LOGV("ril_set_call_audio_path: speaker");
	}

	return 0;
}

