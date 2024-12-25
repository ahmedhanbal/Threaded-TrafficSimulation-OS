#!/bin/bash
compiler="g++"
files="i221271_i220999_D_main.cpp"
cmd="-O1"
libs="-lsfml-graphics -lsfml-window -lsfml-system"
out="traffic"
run="./traffic"

if $compiler "i221271_i220999_D_challan.cpp" $cmd -o challan $libs; then
    clear
    echo "Compilation successful of challan"
else
    echo "Compilation failed challan"
    exit
fi

if $compiler "i221271_i220999_D_stripepayment.cpp" $cmd -o stripepayment $libs; then
    clear
    echo "Compilation successful of stripepayment"
else
    echo "Compilation failed stripepayment"
    exit
fi

if $compiler "i221271_i220999_D_userportal.cpp" $cmd -o userportal $libs; then
    clear
    echo "Compilation successful of userportal"
else
    echo "Compilation failed userportal"
    exit
fi

if $compiler $files $cmd -o $out $libs; then
    clear
    echo "Compilation successful of main"
    $run
else
    echo "Compilation failed main"
    exit 1
fi
