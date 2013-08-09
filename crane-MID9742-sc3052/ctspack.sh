#!/bin/bash
make -j4 dist
./build/tools/releasetools/sign_target_files_apks -d device/softwinner/crane-MID9742-sc3052/security out/dist/crane_MID9742_sc3052-target_files-$(date +%Y%m%d).zip out/dist/signed-target-files.zip
./build/tools/releasetools/img_from_target_files out/dist/signed-target-files.zip out/dist/signed-img.zip
mkdir out/dist/signed-img
unzip out/dist/signed-img.zip -d out/dist/signed-img
cp  out/dist/signed-img/*.img $OUT/

