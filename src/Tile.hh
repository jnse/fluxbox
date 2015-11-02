#ifndef WINDOW_TILING_HH
#define WINDOW_TILING_HH

#include "FbTk/Command.hh"
#include "FbTk/RefCount.hh"
#include "FbTk/Resource.hh"

#include "Workspace.hh"
#include "ClientPattern.hh"

/// Base class for tile commands.
///
/// Don't use this class directly, it's intended as
/// a base class for the various tiling algorithms.
class TileBaseCmd: public FbTk::Command<void> {
protected:
    BScreen* m_screen;
    Workspace* m_space;
    Workspace::Windows m_windows;
    int m_head;
    int m_scr_offs_x;
    int m_scr_offs_y;
    int m_scr_width;
    int m_scr_height;
    size_t m_num_windows;
    int m_padding;
    bool m_can_tile;
    bool m_have_main_window;
    const ClientPattern m_pat;
    FluxboxWindow* m_main_window;
public:
    /// Constructor
    ///
    /// \param pat : Client pattern - which windows to tile.
    /// \param have_main_window : Set to true to exclude the
    ///     focused window from the list of windows to tile (m_windows)
    ///     and instead assign it to m_main_window.
    explicit TileBaseCmd(std::string &pat, bool have_main_window=false);
    /// Execute tile command.
    ///
    /// Intended to be called from the child class. This
    /// populates all member variables with information pretty
    /// much any tiling implementation will need.
    ///
    /// It also sets m_can_tile to true if everything checks out.
    ///
    /// In the child implementation, m_can_tile should be 
    /// tested after running TileBaseCmd::execute
    virtual void execute();
};

/// Tiles windows horizontally in current workspace.
class TileHorizontalCmd: public TileBaseCmd {
public:
    /// Constructor
    explicit TileHorizontalCmd(std::string &pat): 
        TileBaseCmd::TileBaseCmd(pat){ }; 
    /// Execute tile horizontal command
    void execute();
private:
    const ClientPattern m_pat;
};

/// Tiles windows vertically in current workspace.
class TileVerticalCmd: public TileBaseCmd {
public:
    /// Constructor
    explicit TileVerticalCmd(std::string &pat): 
        TileBaseCmd::TileBaseCmd(pat) { };
    /// Execute tile horizontal command
    void execute();
};

/// Tiles windows in current workspace in a grid.
class TileGridCmd: public TileBaseCmd {
public:
    /// Constructor
    explicit TileGridCmd(std::string &pat): 
        TileBaseCmd::TileBaseCmd(pat) { };
    /// Execute tile horizontal command
    void execute();
};

class TileStackedLeftCmd: public TileBaseCmd {
public:
    /// Constructor
    explicit TileStackedLeftCmd(std::string &pat):
        TileBaseCmd::TileBaseCmd(pat,true) { };
    /// Execute tile stacked left command
    void execute();
};

class TileStackedRightCmd: public TileBaseCmd {
public:
    /// Constructor
    explicit TileStackedRightCmd(std::string &pat):
        TileBaseCmd::TileBaseCmd(pat,true) { };
    /// Execute tile stacked left command
    void execute();
};

class TileStackedTopCmd: public TileBaseCmd {
public:
    /// Constructor
    explicit TileStackedTopCmd(std::string &pat):
        TileBaseCmd::TileBaseCmd(pat,true) { };
    /// Execute tile stacked left command
    void execute();
};

class TileStackedBottomCmd: public TileBaseCmd {
public:
    /// Constructor
    explicit TileStackedBottomCmd(std::string &pat):
        TileBaseCmd::TileBaseCmd(pat,true) { };
    /// Execute tile stacked left command
    void execute();
};
 
#endif
