#pragma once

namespace Panels {
    extern bool showOmnix;
    extern bool showInspector;
    extern bool showAssets;
    extern bool showHierarchy;
    extern bool showViewport;
    extern bool showGameView;

    inline void openOmnix() { showOmnix = true; }

    inline void openInspector() { showInspector = true; }
    inline void openAssets() { showAssets = true; }
    inline void openHierarchy() { showHierarchy = true; }
    inline void openViewport() { showViewport = true; }
    inline void openGameView() { showGameView = true; }
}


