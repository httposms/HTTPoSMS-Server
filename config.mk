# Compiler / package config

CC = gcc
CFLAGS = -std=c99 -O2 -pedantic -Wall -Werror 
LDFLAGS= 

NAME = httposms-server
VERSION = v0.0.0-prerelease

SRC_DIR = src
OBJ_DIR = obj

# Install params
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man


