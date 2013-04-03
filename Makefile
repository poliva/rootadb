BUILD=debug
#BUILD=release

build: clean
	ndk-build
	if [ ! -e "assets" ]; then mkdir assets ; fi
	cp libs/armeabi/setpropex assets/setpropex
	ant ${BUILD}

clean:
	rm -rf bin gen libs obj assets

uninstall:
	adb shell "su -c 'LD_LIBRARY_PATH=/system/lib pm uninstall org.eslack.rootadb'"

install: uninstall build
	adb install bin/RootAdb-${BUILD}.apk
	adb shell 'LD_LIBRARY_PATH=/system/lib am start -n org.eslack.rootadb/.RootAdb'
