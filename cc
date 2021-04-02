#!/bin/sh

BUILD_FOLDER=.
$BUILD_FOLDER/c0 $1 $1.tmp
$BUILD_FOLDER/c1 $1.tmp |  tr -d '\r'