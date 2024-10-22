
# Change this to where your
ANDROID_NDK=~/android/android-ndk-r10e
HOST=darwin-x86_64

ANDROID_GCC=$ANDROID_NDK/toolchains/arm-linux-androideabi-4.8/prebuilt/$HOST/bin/arm-linux-androideabi-g++
ANDROID_AR=$ANDROID_NDK/toolchains/arm-linux-androideabi-4.8/prebuilt/$HOST/bin/arm-linux-androideabi-ar


function RemoveTarget {
    local name=$1
    if [ -f $name ]; then
        rm $name
        echo Removed $name
    fi
}

function CompileAndroid {
    local name=$1
    local src=$2
    local targetdir=$3
    
    archs=("armv7")
    for arch in "${archs[@]}"
    do
        local archname=$arch-android
        local target=$targetdir/$archname/lib$name.a

        RemoveTarget $target
        mkdir -p $(dirname $target)
        
        local objname=/tmp/$name-$archname.o
        $ANDROID_GCC -c -g -gdwarf-2 -fpic -ffunction-sections -fstack-protector -Wno-psabi -march=armv7-a -mfloat-abi=softfp -mfpu=vfp -fomit-frame-pointer -fno-strict-aliasing -finline-limit=64 -fno-exceptions -funwind-tables $src -c -o $objname
        $ANDROID_AR rcs $target $objname

        rm $objname
        echo Wrote $target
    done
}

CompileAndroid testlib testlib.cpp ../umpandroidnative/lib

cp -v testlib.h ../umpandroidnative/include