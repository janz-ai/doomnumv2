# ─────────────────────────────────────────────────────────────
#  DOOM-NW  ·  NumWorks External App Makefile
#
#  Prerequisites
#  ─────────────
#  1) Install nwlink:
#       npm install -g nwlink
#  2) Clone the NumWorks Epsilon SDK:
#       git clone --recurse-submodules https://github.com/numworks/epsilon
#     OR let nwlink handle it automatically via:
#       nwlink install-sdk
#
#  Build
#  ─────
#  make                  → build doom.nwa
#  make run              → build + send to connected calculator
#  make clean            → remove build artefacts
# ─────────────────────────────────────────────────────────────

# ── App metadata ───────────────────────────────────────────────
APP_NAME    = doom-nw
APP_VERSION = 1.0.0
APP_AUTHOR  = DoomNW

# ── Source files ───────────────────────────────────────────────
SRCS := src/main.cpp      \
        src/game.cpp      \
        src/raycaster.cpp \
        src/renderer.cpp

# ── NumWorks SDK location ─────────────────────────────────────
# nwlink sets this automatically. You can also set it manually:
#   export EPSILON_SOURCE_DIR=/path/to/epsilon
EPSILON_SOURCE_DIR ?= $(shell nwlink epsilon-source-dir 2>/dev/null)

ifeq ($(EPSILON_SOURCE_DIR),)
$(error "EPSILON_SOURCE_DIR not set. Run: npm install -g nwlink && nwlink install-sdk")
endif

# ── Include NumWorks build system ─────────────────────────────
include $(EPSILON_SOURCE_DIR)/build/targets.mak
