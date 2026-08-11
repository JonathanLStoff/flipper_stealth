# Stealth - build/install automation
#
# Quick start:
#   make setup           install the Python toolchain (ufbt) + pull the Flipper SDK
#   make build            build the .fap
#   make install SD=E:    copy the .fap onto an SD card mounted at E:
#
# Run `make help` for the full target list.

PYTHON      ?= python3
SD          ?= D:
PORT        ?=

# On Windows, MSYS2/Git-Bash `make` runs recipes through a bash whose own
# profile scripts unconditionally re-export a fake POSIX HOME (e.g.
# /home/you) - so `export`-ing the real one from this Makefile gets
# silently clobbered before the recipe even runs (`export FOO := ...`
# above the rules does NOT survive that). Windows-native Python's
# expanduser() doesn't consult HOME at all on Windows anyway - it wants
# USERPROFILE (or HOMEDRIVE+HOMEPATH), which the same bash strips
# entirely. Without it, ufbt/scons's "~" path resolution silently fails
# and builds a broken path (a literal "~" directory created inside this
# project); GCC similarly fails to create temp files without a valid
# TMP/TEMP. Resolve the real profile path via PowerShell (queries Windows
# directly, ignoring whatever bash stripped) and inject it inline on every
# recipe invocation via $(RUN) - inline assignment on the command itself
# is what actually survives, unlike `export`.
#
# Detecting Windows via $(OS) alone isn't enough: some MSYS2/Git-Bash `make`
# builds don't import OS from the environment at all (`make -p` lists no OS
# variable even though bash has it), so the guard falls through to the POSIX
# branch and the build fails in precisely the way described above. `uname -s`
# reports MINGW64_NT-*/MSYS_NT-*/CYGWIN_NT-* under those same shells, so check
# both.
UNAME_S := $(shell uname -s 2>/dev/null)
IS_WINDOWS :=
ifeq ($(OS),Windows_NT)
IS_WINDOWS := 1
else ifneq (,$(findstring MINGW,$(UNAME_S)))
IS_WINDOWS := 1
else ifneq (,$(findstring MSYS,$(UNAME_S)))
IS_WINDOWS := 1
else ifneq (,$(findstring CYGWIN,$(UNAME_S)))
IS_WINDOWS := 1
endif

ifeq ($(IS_WINDOWS),1)
WIN_USERPROFILE := $(shell powershell -NoProfile -Command "[Environment]::GetFolderPath('UserProfile')")
RUN := USERPROFILE="$(WIN_USERPROFILE)" TMP="$(WIN_USERPROFILE)\AppData\Local\Temp" TEMP="$(WIN_USERPROFILE)\AppData\Local\Temp"
else
RUN :=
endif

.PHONY: help setup build install install-sd launch clean

help:
	@echo "Stealth - make targets"
	@echo ""
	@echo "  make setup                Install Python deps (ufbt, pyserial, ...) and pull the Flipper SDK"
	@echo "  make build                Build the .fap"
	@echo "  make install SD=<path>    Copy the .fap onto an SD card mounted at <path>"
	@echo "  make install PORT=<port>  ...or push it over USB serial instead (experimental)"
	@echo "  make launch               Build, then launch on a Flipper connected over USB (via ufbt)"
	@echo "  make clean                Remove this app's build outputs from ufbt's cache"
	@echo ""
	@echo "Note: \`ufbt build\` doesn't drop the .fap in this project - it builds into"
	@echo "~/.ufbt/build/stealth.fap, a per-user cache shared across every ufbt app"
	@echo "you build. \`make install\`/\`make launch\` know where to find it."
	@echo ""
	@echo "The app is self-contained: no extra hardware, no radios, no SD-card data"
	@echo "files. Every reading it shows is generated on the fly and fake."

# ---------------------------------------------------------------- Flipper app

setup:
	$(RUN) $(PYTHON) -m pip install --upgrade -r requirements.txt
	$(RUN) $(PYTHON) -m ufbt update

build:
	$(RUN) $(PYTHON) -m ufbt build

# `make install` copies the app onto the SD card. Pass SD=<mounted path>
# (recommended) or PORT=<serial port> to try the experimental USB path instead.
install: build
ifneq ($(strip $(SD)),)
	$(RUN) $(PYTHON) scripts/install_to_sd.py --sd-path "$(SD)"
else ifneq ($(strip $(PORT)),)
	$(RUN) $(PYTHON) scripts/install_to_sd.py --serial "$(PORT)"
else
	$(RUN) $(PYTHON) scripts/install_to_sd.py
endif

install-sd: build
	$(RUN) $(PYTHON) scripts/install_to_sd.py --sd-path "$(SD)"

# Build + launch directly on a USB-connected Flipper via ufbt.
launch: build
	$(RUN) $(PYTHON) -m ufbt launch

# ---------------------------------------------------------------- housekeeping

clean:
	$(RUN) $(PYTHON) -m ufbt clean
