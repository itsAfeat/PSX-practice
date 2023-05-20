# PSn00bSDK example CMake script
# (C) 2021 spicyjpeg - MPL licensed

cmake_minimum_required(VERSION 3.21)

project(
	PSn00bSDK-practice
	LANGUAGES    C CXX ASM
	VERSION      1.0.0
	DESCRIPTION  "PSn00bSDK practice"
	HOMEPAGE_URL "http://lameguy64.net/?page=psn00bsdk"
)

psn00bsdk_add_executable(practice GPREL main.c)

psn00bsdk_add_cd_image(
	iso      # Target name
	practice # Output file name (= practice.bin + practice.cue)
	iso.xml  # Path to config file
	DEPENDS practice system.cnf
)
