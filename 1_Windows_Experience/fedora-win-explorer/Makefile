# ==============================================================================
# Makefile for Fedora Windows 10/11 Dolphin "This PC" LD_PRELOAD Helper & Tools
# Target: Fedora 44 KDE Plasma 6 (Qt6 / GCC 16)
# ==============================================================================

CXX       ?= g++
PKG_CONFIG ?= pkg-config

# Automatically resolve Qt6 flags via pkg-config with fallback to standard Fedora /usr/include/qt6 paths
QT6_CFLAGS := $(shell $(PKG_CONFIG) --cflags Qt6Core Qt6Gui Qt6Widgets 2>/dev/null || echo "-I/usr/include/qt6 -I/usr/include/qt6/QtCore -I/usr/include/qt6/QtGui -I/usr/include/qt6/QtWidgets")
QT6_LIBS   := $(shell $(PKG_CONFIG) --libs Qt6Core Qt6Gui Qt6Widgets 2>/dev/null || echo "-lQt6Widgets -lQt6Gui -lQt6Core")

CXXFLAGS  := -std=c++20 -O2  -fPIC -Wall -Wextra -Wno-unused-parameter $(QT6_CFLAGS)
LDFLAGS   :=  $(QT6_LIBS) -ldl

TARGET_SO  := libwin10_thispc.so
TARGET_QRC := dump_qrc

PREFIX    ?= $(HOME)/.config/win-explorer
ICONS_DIR ?= $(PREFIX)/icons

.PHONY: all clean verify install uninstall

all: $(TARGET_SO) $(TARGET_QRC)

install: $(TARGET_SO)
	@killall -9 dolphin || true
	@echo "[INSTALL] Installing $(TARGET_SO) and assets to $(PREFIX)..."
	@mkdir -p $(PREFIX) $(ICONS_DIR)
	@install -m 755 $(TARGET_SO) $(PREFIX)/$(TARGET_SO)
	@install -m 644 win10_thispc_helper.cpp $(PREFIX)/win10_thispc_helper.cpp
	@install -m 755 win10_recent_daemon.py $(PREFIX)/win10_recent_daemon.py
	@cp -f real_win10_icons/*.png $(ICONS_DIR)/ 2>/dev/null || true
	@echo "[OK] Installed successfully to $(PREFIX)."

uninstall:
	@echo "[UNINSTALL] Removing $(TARGET_SO) and assets from $(PREFIX)..."
	@rm -rf $(PREFIX)
	@echo "[OK] Uninstalled."

$(TARGET_SO): win10_thispc_helper.cpp
	@echo "[CXX] Building optimized shared library $@ (-O2 )..."
	$(CXX) $(CXXFLAGS) -shared -o $@ $< $(LDFLAGS)
	@echo "[OK] Built $@ ($$(stat -c%s $@) bytes)"

$(TARGET_QRC): dump_qrc.cpp
	@echo "[CXX] Building QRC resource inspector $@..."
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)
	@echo "[OK] Built $@"

verify: $(TARGET_SO)
	@echo "[VERIFY] Checking dynamic symbols & Qt6 linkage for $(TARGET_SO)..."
	@ldd $(TARGET_SO) | head -n 10
	@nm -D $(TARGET_SO) | grep -E "KFileItem|KItemList" | head -n 10 || true

clean:
	rm -f $(TARGET_SO) $(TARGET_QRC)
