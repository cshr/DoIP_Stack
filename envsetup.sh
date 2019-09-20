#!/bin/bash


basepath=$(pwd)
debugdir="/bin/Debug"
releasedir="/bin/Release"
debugpath=${basepath}${debugdir}
releasepath=${basepath}${releasedir}

export LD_LIBRARY_PATH=${debugpath}
export LD_LIBRARY_PATH=${releasepath}:$LD_LIBRARY_PATH
echo ${LD_LIBRARY_PATH}

function cleanfile()
{
    echo "************cleanfile**********"

    cd bin/Debug
    if [ $? -ne 0 ]; then
        echo "failed to cd bin/Debug"
    else
        rm -rf *
        cd ../../
        echo "clean bin/Debug"
    fi

    cd bin/Release
    if [ $? -ne 0 ]; then
        echo "failed to cd bin/Release"
    else
        rm -rf *
        cd ../../
        echo "clean bin/Release"
    fi

    cd libdoipstack/obj
    if [ $? -ne 0 ]; then
        echo "failed to cd libdoipstack"
    else
        rm -rf *
        cd ../../
        echo "clean libdoipstack"
    fi

    cd doip_server/obj
    if [ $? -ne 0 ]; then
        echo "failed to cd doip_server"
    else
        rm -rf *
        cd ../../
        echo "clean doip_server"
    fi

    cd doip_client/obj
    if [ $? -ne 0 ]; then
        echo "failed to cd doip_client"
    else
        rm -rf *
        cd ../../
        echo "clean doip_client"
    fi

    cd test_equipment/obj
    if [ $? -ne 0 ]; then
        echo "failed to cd test_equipment"
    else
        rm -rf *
        cd ../../
        echo "clean test_equipment"
    fi

    cd bin/Debug
    if [ $? -eq 0 ]; then
        cd ../../
    fi

    echo ""
}

function makefile()
{
    cleanfile

    echo "************makefile**********"
    cd libdoipstack
    make
    cd ../doip_server/
    make
    cd ../doip_client/
    make
    cd ../test_equipment/
    make
    cd ../

    cd bin/Debug
    if [ $? -eq 0 ]; then
        cd ../../
    fi
}