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

/*
 * Contains implementation of an abstract class V4L2Camera that defines
 * functionality expected from an emulated physical camera device:
 *  - Obtaining and setting camera parameters
 *  - Capturing frames
 *  - Streaming video
 *  - etc.
 */
#define LOG_TAG "V4L2Camera"
#include "CameraDebug.h"

#include <sys/select.h>
#include "V4L2Camera.h"
#include "Converters.h"

namespace android {

V4L2Camera::V4L2Camera(CameraHardware* camera_hal)
    : mObjectLock(),
      mCurFrameTimestamp(0),
      mCameraHAL(camera_hal),
      mState(ECDS_CONSTRUCTED),
      mTakingPicture(false),
      mTakingPictureRecord(false),
      mThreadRunning(false)
{
	F_LOG;
	
	pthread_mutex_init(&mTakePhotoMutex, NULL);
	pthread_cond_init(&mTakePhotoCond, NULL);
	
	pthread_mutex_init(&mThreadRunningMutex, NULL);
	pthread_cond_init(&mThreadRunningCond, NULL);
}

V4L2Camera::~V4L2Camera()
{
	F_LOG;

	pthread_mutex_destroy(&mTakePhotoMutex);
	pthread_cond_destroy(&mTakePhotoCond);
	
	pthread_mutex_destroy(&mThreadRunningMutex);
	pthread_cond_destroy(&mThreadRunningCond);
}

/****************************************************************************
 * V4L2Camera device public API
 ***************************************************************************/

status_t V4L2Camera::Initialize()
{
	F_LOG;
    if (isInitialized()) {
        LOGW("%s: V4L2Camera device is already initialized: mState = %d",
             __FUNCTION__, mState);
        return NO_ERROR;
    }

    /* Instantiate worker thread object. */
    mWorkerThread = new WorkerThread(this);
    if (getWorkerThread() == NULL) {
        LOGE("%s: Unable to instantiate worker thread object", __FUNCTION__);
        return ENOMEM;
    }

    mState = ECDS_INITIALIZED;

    return NO_ERROR;
}

status_t V4L2Camera::startDeliveringFrames(bool one_burst)
{
    LOGV("%s", __FUNCTION__);

	mThreadRunning = false;

    if (!isStarted()) {
        LOGE("%s: Device is not started", __FUNCTION__);
        return EINVAL;
    }

    /* Frames will be delivered from the thread routine. */
    const status_t res = startWorkerThread(one_burst);
    LOGE_IF(res != NO_ERROR, "%s: startWorkerThread failed", __FUNCTION__);
    return res;
}

status_t V4L2Camera::stopDeliveringFrames()
{
    LOGV("%s", __FUNCTION__);
	
	pthread_mutex_lock(&mTakePhotoMutex);
	if (mTakingPicture)
	{
		LOGW("wait until take picture end before stop thread ......");		
		pthread_cond_wait(&mTakePhotoCond, &mTakePhotoMutex);
		LOGW("wait take picture ok");
	}
	pthread_mutex_unlock(&mTakePhotoMutex);

	// for CTS, V4L2Camera::WorkerThread::readyToRun must be called before stopDeliveringFrames
	pthread_mutex_lock(&mThreadRunningMutex);
	if (!mThreadRunning)
	{
		LOGW("should not stop preview so quickly, wait thread running first ......");
		pthread_cond_wait(&mThreadRunningCond, &mThreadRunningMutex);
		LOGW("wait thread running ok");
	}
	pthread_mutex_unlock(&mThreadRunningMutex);
	
    if (!isStarted()) {
        LOGW("%s: Device is not started", __FUNCTION__);
        return NO_ERROR;
    }

    const status_t res = stopWorkerThread();
    LOGE_IF(res != NO_ERROR, "%s: startWorkerThread failed", __FUNCTION__);
    return res;
}

/****************************************************************************
 * V4L2Camera device private API
 ***************************************************************************/

status_t V4L2Camera::commonStartDevice(int width,
                                       int height,
                                       uint32_t pix_fmt)
{
	F_LOG;
	
    /* Validate pixel format, and calculate framebuffer size at the same time. */
    switch (pix_fmt) {
        case V4L2_PIX_FMT_YVU420:
        case V4L2_PIX_FMT_YUV420:
        case V4L2_PIX_FMT_NV21:
        case V4L2_PIX_FMT_NV12:
            mFrameBufferSize = (width * height * 12) / 8;
            break;

        default:
            LOGE("%s: Unknown pixel format %.4s",
                 __FUNCTION__, reinterpret_cast<const char*>(&pix_fmt));
            return EINVAL;
    }

    /* Cache framebuffer info. */
    mFrameWidth = width;
    mFrameHeight = height;
    mPixelFormat = pix_fmt;
    mTotalPixels = width * height;

    return NO_ERROR;
}

void V4L2Camera::commonStopDevice()
{
	F_LOG;
    mFrameWidth = mFrameHeight = mTotalPixels = 0;
    mPixelFormat = 0;
}

/****************************************************************************
 * Worker thread management.
 ***************************************************************************/

status_t V4L2Camera::startWorkerThread(bool one_burst)
{
    LOGV("%s", __FUNCTION__);

    if (!isInitialized()) {
        LOGE("%s: V4L2Camera device is not initialized", __FUNCTION__);
        return EINVAL;
    }

    const status_t res = getWorkerThread()->startThread(one_burst);
    LOGE_IF(res != NO_ERROR, "%s: Unable to start worker thread", __FUNCTION__);
    return res;
}

status_t V4L2Camera::stopWorkerThread()
{
    LOGV("%s", __FUNCTION__);

    if (!isInitialized()) {
        LOGE("%s: V4L2Camera device is not initialized", __FUNCTION__);
        return EINVAL;
    }

    const status_t res = getWorkerThread()->stopThread();
    LOGE_IF(res != NO_ERROR, "%s: Unable to stop worker thread", __FUNCTION__);
    return res;
}

bool V4L2Camera::inWorkerThread()
{
	F_LOG;
    /* This will end the thread loop, and will terminate the thread. Derived
     * classes must override this method. */
    return false;
}

/****************************************************************************
 * Worker thread implementation.
 ***************************************************************************/

status_t V4L2Camera::WorkerThread::readyToRun()
{
    LOGV("V4L2Camera::WorkerThread::readyToRun");

    LOGW_IF(mThreadControl >= 0 || mControlFD >= 0,
            "%s: Thread control FDs are opened", __FUNCTION__);
    /* Create a pair of FDs that would be used to control the thread. */
    int thread_fds[2];
    if (pipe(thread_fds) == 0) {
        mThreadControl = thread_fds[1];
        mControlFD = thread_fds[0];
        LOGV("V4L2Camera's worker thread has been started.");

        return NO_ERROR;
    } else {
        LOGE("%s: Unable to create thread control FDs: %d -> %s",
             __FUNCTION__, errno, strerror(errno));
        return errno;
    }
}

status_t V4L2Camera::WorkerThread::stopThread()
{
    LOGV("Stopping V4L2Camera device's worker thread...");

    status_t res = EINVAL;
    if (mThreadControl >= 0) {
        /* Send "stop" message to the thread loop. */
        const ControlMessage msg = THREAD_STOP;
        const int wres =
            TEMP_FAILURE_RETRY(write(mThreadControl, &msg, sizeof(msg)));
        if (wres == sizeof(msg)) {
            /* Stop the thread, and wait till it's terminated. */
            res = requestExitAndWait();
            if (res == NO_ERROR) {
                /* Close control FDs. */
                if (mThreadControl >= 0) {
                    close(mThreadControl);
                    mThreadControl = -1;
                }
                if (mControlFD >= 0) {
                    close(mControlFD);
                    mControlFD = -1;
                }
                LOGV("V4L2Camera device's worker thread has been stopped.");
            } else {
                LOGE("%s: requestExitAndWait failed: %d -> %s",
                     __FUNCTION__, res, strerror(-res));
            }
        } else {
            LOGE("%s: Unable to send THREAD_STOP message: %d -> %s",
                 __FUNCTION__, errno, strerror(errno));
            res = errno ? errno : EINVAL;
        }
    } else {
        LOGE("%s: Thread control FDs are not opened", __FUNCTION__);
    }
	
	LOGV("Stopping V4L2Camera device's worker thread... OK");

    return res;
}

V4L2Camera::WorkerThread::SelectRes
V4L2Camera::WorkerThread::Select(int fd, int timeout)
{
	// F_LOG;
    fd_set fds[1];
    struct timeval tv, *tvp = NULL;

    const int fd_num = (fd >= 0) ? max(fd, mControlFD) + 1 :
                                   mControlFD + 1;
    FD_ZERO(fds);
    FD_SET(mControlFD, fds);
    if (fd >= 0) {
        FD_SET(fd, fds);
    }
    if (timeout) {
        tv.tv_sec = timeout / 1000000;
        tv.tv_usec = timeout % 1000000;
        tvp = &tv;
    }
    int res = TEMP_FAILURE_RETRY(select(fd_num, fds, NULL, NULL, tvp));
    if (res < 0) {
        LOGE("%s: select returned %d and failed: %d -> %s",
             __FUNCTION__, res, errno, strerror(errno));
        return ERROR;
    } else if (res == 0) {
        /* Timeout. */
        return TIMEOUT;
    } else if (FD_ISSET(mControlFD, fds)) {
        /* A control event. Lets read the message. */
        ControlMessage msg;
        res = TEMP_FAILURE_RETRY(read(mControlFD, &msg, sizeof(msg)));
        if (res != sizeof(msg)) {
            LOGE("%s: Unexpected message size %d, or an error %d -> %s",
                 __FUNCTION__, res, errno, strerror(errno));
            return ERROR;
        }
        /* THREAD_STOP is the only message expected here. */
        if (msg == THREAD_STOP) {
            LOGV("%s: THREAD_STOP message is received", __FUNCTION__);
            return EXIT_THREAD;
        } else {
            LOGE("Unknown worker thread message %d", msg);
            return ERROR;
        }
    } else {
        /* Must be an FD. */
        LOGW_IF(fd < 0 || !FD_ISSET(fd, fds), "%s: Undefined 'select' result",
                __FUNCTION__);
        return READY;
    }
}

};  /* namespace android */
