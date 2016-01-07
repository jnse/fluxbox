#include <iostream>

#include "Tile.hh"
#include "fluxbox.hh"
#include "Screen.hh"
#include "Workspace.hh"
#include "Window.hh"

// TileBaseCmd class implementation -------------------------------------------

// TileBaseCmd constructor.
TileBaseCmd::TileBaseCmd(std::string &pat, bool have_main_window) :
  m_screen(0), m_space(0), m_head(0),
  m_scr_offs_x(0), m_scr_offs_y(0), m_scr_width(0),
  m_scr_height(0), m_num_windows(0), m_padding(0), 
  m_can_tile(false), m_have_main_window(have_main_window), 
  m_pat(pat.c_str()), m_main_window(0) {
  
}

// Execute tile command.
void TileBaseCmd::execute() {
    // Before we do anything, reset m_can_tile
    // If all checks pass, it will be set back to true.
    m_can_tile = false;
    m_main_window = 0;
    m_windows.clear();
    // Get screen and workspace pointers.
    // Return if a screen cannot be obtained or
    // if there are no windows in the current workspace.
    m_screen = Fluxbox::instance()->mouseScreen();
    if (m_screen == 0) return;
    m_space = m_screen->currentWorkspace();
    if (m_space == 0) return;
    // Get padding
    FbTk::Resource<int> padding(
        m_screen->resourceManager(),
        10,
        m_screen->name()+".tiling.padding",
        m_screen->altName()+".tiling.padding");
    m_padding = padding.get();
    // If there's no windows, there's nothing to tile...
    if (m_space->windowList().empty()) return;
    // Current head
    int m_head = m_screen->getCurrHead();
    // Build an array of windows to tile.
    for ( Workspace::Windows::iterator win = m_space->windowList().begin();
          win != m_space->windowList().end(); ++win ) {
        int winhead = m_screen->getHead((*win)->fbWindow());
        if ((winhead == m_head || winhead == 0) && m_pat.match(**win)) {
            // ignoring shaded windows for now.
            // (maybe this should be an option?)
            if ((*win)->isShaded() == true) continue;
            // if we have a main window, and the current
            // window is focused, set m_main_window and
            // don't add it to the list.
            if (m_have_main_window == true) {
                if ((*win)->isFocused() == true) {
                    m_main_window = (*win);
                    continue;
                }
            }
            m_windows.push_back(*win);
        }
    }
    // Get number of windows to tile, if none then exit.
    m_num_windows = m_windows.size();
    if (m_num_windows < 1) return;
    // Get screen dimensions, offset.
    m_scr_offs_x = m_screen->maxLeft(m_head);
    m_scr_offs_y = m_screen->maxTop(m_head);
    m_scr_width = m_screen->maxRight(m_head) - m_scr_offs_x;
    m_scr_height = m_screen->maxBottom(m_head) - m_scr_offs_y;
    // If we got to this point, then we're OK to
    // start tiling windows.
    m_can_tile = true;
}

// TileHorizontalCmd implementation -------------------------------------------

// Execute tile horizontal command.
void TileHorizontalCmd::execute() {
    TileBaseCmd::execute();
    if (m_can_tile != true) return;
    // For horizontal tiling, the height of each window is
    // equal to the screen height divided by the number of windows.
    int tgt_height = m_scr_height / m_num_windows;
    // Iterate through windows and place them.
    size_t counter = 0;
    for (Workspace::Windows::iterator win = m_windows.begin() ; win != m_windows.end(); ++win)
    {
        int x = m_scr_offs_x + m_padding;
        int y = m_scr_offs_y + (counter * tgt_height) + m_padding;
        int w = m_scr_width - (m_padding*2);
        int h = tgt_height - m_padding;
        // Ensure padding to titlebar for last window.
        if ((counter+1) == m_num_windows) {
          h -= m_padding;
        }
        (*win)->moveResize(x,y,w,h);
        counter++;
    }
}

// TileVerticalCmd implementation ---------------------------------------------

// Execute tile vertical command.
void TileVerticalCmd::execute()
{
    TileBaseCmd::execute();
    if (m_can_tile != true) return; 
    // For vertical tiling, the width of each window is
    // equal to the screen width divided by the number of windows.
    int tgt_width = m_scr_width / m_num_windows;
    // Iterate through windows and place them.
    size_t counter = 0;
    for (Workspace::Windows::iterator win = m_windows.begin() ; win != m_windows.end(); ++win)
    {
        int x = m_scr_offs_x + (counter * tgt_width) + m_padding;
        int y = m_scr_offs_y + m_padding;
        int w = tgt_width - m_padding;
        int h = m_scr_height - (m_padding*2);
        // Ensure padding to titlebar for last window.
        if ((counter+1) == m_num_windows) {
          w -= m_padding;
        }
        (*win)->moveResize(x,y,w,h);
        counter++;
    }
}

// TileGridCmd implementation -------------------------------------------------

void TileGridCmd::execute()
{
    TileBaseCmd::execute();
    if (m_can_tile != true) return;  
    // Iterate through windows and place them.
    int row = 0;
    int col = 0;
    size_t counter = 0;
    // try to get the same number of rows as columns.
    int maxcol = int(sqrt((float)m_num_windows));
    int maxrow = int(0.99 + float(m_num_windows) / float(maxcol)); 
    int tgt_width = m_scr_width / maxcol;
    int tgt_height = m_scr_height / maxrow;    
    for (Workspace::Windows::iterator win = m_windows.begin() ; win != m_windows.end(); ++win)
    {
        // Calculate prelimenary position/size.
        int x = m_scr_offs_x + (col * tgt_width) + m_padding;
        int y = m_scr_offs_y + (row * tgt_height) + m_padding;
        int w = tgt_width - m_padding;
        int h = tgt_height - m_padding;
        // Padding along right/bottom screen edges.
        if (col == maxcol-1) w -= m_padding;
        if (row == maxrow-1) h -= m_padding;
        // Give last window remaining space.
        if ((counter+1) == m_num_windows)
        {
          w = m_scr_width - x - m_padding;
          h = m_scr_height - y - m_padding;
        }
        // Set window size
        (*win)->moveResize(x,y,w,h);
        // Handle counters.
        counter++;
        col++;
        if (col >= maxcol)
        {
          col = 0;
          row++;
        }        
    }
}

// TileStackedLeft implementation ---------------------------------------------

void TileStackedLeftCmd::execute() {
    TileBaseCmd::execute();
    if (m_can_tile != true) return;
    if (m_main_window == 0) return;
    // Main window fills left half of the screen.
    int x = m_scr_offs_x + m_padding;
    int y = m_scr_offs_y + m_padding;
    int w = (m_scr_width / 2) - (m_padding * 2);
    int h = m_scr_height - (m_padding * 2);
    m_main_window->moveResize(x,y,w,h);
    // Iterate through remaining windows and stack
    // them on the right side of the screen.
    size_t counter = 0;
    int tgt_height = m_scr_height / m_num_windows;    
    for (Workspace::Windows::iterator win = m_windows.begin() ; win != m_windows.end(); ++win)
    {
        x = m_scr_offs_x + (m_scr_width / 2);
        y = m_scr_offs_y + (counter * tgt_height) + m_padding;
        w = (m_scr_width / 2) - m_padding;
        h = tgt_height - m_padding;
        // Ensure padding to titlebar for last window.
        if ((counter+1) == m_num_windows) {
          h -= m_padding;
        } 
        (*win)->moveResize(x,y,w,h);
        counter++;
    } 
}

// TileStackedRight implementation ---------------------------------------------

void TileStackedRightCmd::execute() {
    TileBaseCmd::execute();
    if (m_can_tile != true) return;
    if (m_main_window == 0) return;
    // Main window fills right half of the screen.
    int x = m_scr_offs_x+((m_scr_width / 2)+m_padding);
    int y = m_scr_offs_y+m_padding;
    int w = (m_scr_width / 2)-(m_padding*2);
    int h = m_scr_height-(m_padding*2);
    m_main_window->moveResize(x,y,w,h);
    // Iterate through remaining windows and stack
    // them on the left side of the screen.
    size_t counter = 0;
    int tgt_height = m_scr_height / m_num_windows;    
    for (Workspace::Windows::iterator win = m_windows.begin() ; win != m_windows.end(); ++win)
    {
        x = m_scr_offs_x + m_padding;
        y = m_scr_offs_y + (counter * tgt_height) + m_padding;
        w = (m_scr_width / 2) - m_padding;
        h = tgt_height - m_padding;
        // Ensure padding to titlebar for last window.
        if ((counter+1) == m_num_windows) {
          h -= m_padding;
        } 
        (*win)->moveResize(x,y,w,h);
        counter++;
    } 
}

// TileStackedTop implementation ----------------------------------------------

void TileStackedTopCmd::execute() {
    TileBaseCmd::execute();
    if (m_can_tile != true) return;
    if (m_main_window == 0) return;
    // Main window fills top half of the screen.
    int x = m_scr_offs_x+m_padding;
    int y = m_scr_offs_y+m_padding;
    int w = m_scr_width - (m_padding*2);
    int h = (m_scr_height/2)-(m_padding*2);
    m_main_window->moveResize(x,y,w,h);
    // Iterate through remaining windows and stack
    // them on the bottom half of the screen.
    size_t counter = 0;
    int tgt_width = m_scr_width / m_num_windows;    
    for (Workspace::Windows::iterator win = m_windows.begin() ; win != m_windows.end(); ++win)
    {
        x = m_scr_offs_x + (counter * tgt_width) + m_padding;
        y = m_scr_offs_y + m_scr_height/2;
        w = tgt_width - m_padding;
        h = (m_scr_height/2) - m_padding;
        // Ensure padding to right edge for last window.
        if ((counter+1) == m_num_windows) {
          w -= m_padding;
        } 
        (*win)->moveResize(x,y,w,h);
        counter++;
    } 
}

// TileStackedBottom implementation -------------------------------------------

void TileStackedBottomCmd::execute() {
    TileBaseCmd::execute();
    if (m_can_tile != true) return;
    if (m_main_window == 0) return;
    // Main window fills bottom half of the screen.
    int x = m_scr_offs_x+m_padding;
    int y = m_scr_offs_y+(m_scr_height/2)+m_padding;
    int w = m_scr_width - (m_padding*2);
    int h = (m_scr_height/2)-(m_padding*2);
    m_main_window->moveResize(x,y,w,h);
    // Iterate through remaining windows and stack
    // them on the top half of the screen.
    size_t counter = 0;
    int tgt_width = m_scr_width / m_num_windows;    
    for (Workspace::Windows::iterator win = m_windows.begin() ; win != m_windows.end(); ++win)
    {
        x = m_scr_offs_x + (counter * tgt_width) + m_padding;
        y = m_scr_offs_y + m_padding;
        w = tgt_width - m_padding;
        h = (m_scr_height/2) - m_padding;
        // Ensure padding to right edge for last window.
        if ((counter+1) == m_num_windows) {
          w -= m_padding;
        } 
        (*win)->moveResize(x,y,w,h);
        counter++;
    } 
}
 
