# Compiler / package config

NAME = httposms-server
VERSION = v0.0.0-prerelease

CC = gcc
INCLUDE = include
CFLAGS = -std=c99 -O2 -pedantic -I${INCLUDE} -Wall -DNAME="${NAME}" -DVERSION="${VERSION}"
LDFLAGS= 

SRC_DIR = src
OBJ_DIR = obj

# Install params
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man


