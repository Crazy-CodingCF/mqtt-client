#########################################################################
# File Name: config.sh
# Author: caofei
# mail: caofei_lexue@163.com
# Created Time: 2020年02月27日 星期四 10时51分54秒
#########################################################################
#!/bin/bash

myPath=`pwd`/build

if [ ! -d "$myPath" ]; then
	mkdir build
else
	rm -rf build
	mkdir build
fi

cd ${myPath}

#脚本编译
#cmake -DPLATFORM=arm -DUSE_LOG=OFF \
#	-DVERSION_MAJOR=1 \
#	-DVERSION_MINOR=3 \
#	-DVERSION_RELEASE=2 ..
#本地编译
cmake -DUSE_LOG=OFF -DVERSION_MAJOR=1 -DVERSION_MINOR=0 -DVERSION_RELEASE=0 ..  
make

tar zcvf ./libmqtt1.1.tar.gz  ./libmqtt1.1.*
mv ./libmqtt1.1.tar.gz  ..
