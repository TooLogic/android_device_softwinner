#!/bin/bash

rm $OUT/system/app/Bluetooth.apk
rm $OUT/system/app/Contacts.apk
rm $OUT/system/app/Development.apk
#rm $OUT/system/app/Phone.apk
rm $OUT/system/app/SpeechRecorder.apk
rm $OUT/system/app/SoundRecorder.apk
rm $OUT/system/app/VideoEditor.apk

make systemimage-nodeps
