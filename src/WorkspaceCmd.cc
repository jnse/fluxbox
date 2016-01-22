// WorkspaceCmd.cc for Fluxbox - an X11 Window manager
// Copyright (c) 2003 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
//                and Simon Bowden (rathnor at users.sourceforge.net)
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#include "WorkspaceCmd.hh"

#include "Layer.hh"
#include "Workspace.hh"
#include "Window.hh"
#include "Screen.hh"
#include "fluxbox.hh"
#include "WinClient.hh"
#include "FocusControl.hh"
#include "WindowCmd.hh"
#include "Tile.hh"

#include "FbTk/KeyUtil.hh"
#include "FbTk/CommandParser.hh"
#include "FbTk/stringstream.hh"
#include "FbTk/StringUtil.hh"

#ifdef HAVE_CMATH
  #include <cmath>
#else
  #include <math.h>
#endif
#include <algorithm>
#include <functional>
#include <vector>

using std::string;

REGISTER_COMMAND_PARSER(map, WindowListCmd::parse, void);
REGISTER_COMMAND_PARSER(foreach, WindowListCmd::parse, void);

FbTk::Command<void> *WindowListCmd::parse(const string &command, const string &args,
                                    bool trusted) {
    FbTk::Command<void> *cmd = 0;
    FbTk::Command<bool> *filter = 0;
    std::vector<string> tokens;
    int opts = 0;
    string pat;

    FbTk::StringUtil::stringTokensBetween(tokens, args, pat, '{', '}');
    if (tokens.empty())
        return 0;

    cmd = FbTk::CommandParser<void>::instance().parse(tokens[0], trusted);
    if (!cmd)
        return 0;

    if (tokens.size() > 1) {
        FocusableList::parseArgs(tokens[1], opts, pat);

        filter = FbTk::CommandParser<bool>::instance().parse(pat, trusted);
    }

    return new WindowListCmd(FbTk::RefCount<FbTk::Command<void> >(cmd), opts,
                             FbTk::RefCount<FbTk::Command<bool> >(filter));
}

void WindowListCmd::execute() {
    BScreen *screen = Fluxbox::instance()->keyScreen();
    if (screen != 0) {
        FocusableList::Focusables win_list(FocusableList::getListFromOptions(*screen, m_opts)->clientList());

        FocusableList::Focusables::iterator it = win_list.begin(),
                                            it_end = win_list.end();
        // save old value, so we can restore it later
        WinClient *old = WindowCmd<void>::client();
        for (; it != it_end; ++it) {
            Focusable* wptr = *it;
            if (typeid(*wptr) == typeid(FluxboxWindow)) {
                WindowCmd<void>::setWindow((wptr)->fbwindow());
            } else if (typeid(*wptr) == typeid(WinClient)) {
                WindowCmd<void>::setClient(dynamic_cast<WinClient *>(wptr));
            }
            if (!m_filter || m_filter->execute()) {
                m_cmd->execute();
            }
        }
        WindowCmd<void>::setClient(old);
    }
}

FbTk::Command<bool> *SomeCmd::parse(const string &command, const string &args,
                                  bool trusted) {
    FbTk::Command<bool> *boolcmd =
            FbTk::CommandParser<bool>::instance().parse(args,
                                                                      trusted);
    if (!boolcmd)
        return 0;
    if (command == "some")
        return new SomeCmd(FbTk::RefCount<FbTk::Command<bool> >(boolcmd));
    return new EveryCmd(FbTk::RefCount<FbTk::Command<bool> >(boolcmd));
}

REGISTER_COMMAND_PARSER(some, SomeCmd::parse, bool);
REGISTER_COMMAND_PARSER(every, SomeCmd::parse, bool);

bool SomeCmd::execute() {
    BScreen *screen = Fluxbox::instance()->keyScreen();
    if (screen != 0) {
        FocusControl::Focusables win_list(screen->focusControl().creationOrderList().clientList());

        FocusControl::Focusables::iterator it = win_list.begin(),
                                           it_end = win_list.end();
        // save old value, so we can restore it later
        WinClient *old = WindowCmd<void>::client();
        for (; it != it_end; ++it) {
            WinClient *client = dynamic_cast<WinClient *>(*it);
            if (!client) continue;
            WindowCmd<void>::setClient(client);
            if (m_cmd->execute()) {
                WindowCmd<void>::setClient(old);
                return true;
            }
        }
        WindowCmd<void>::setClient(old);
    }
    return false;
}

bool EveryCmd::execute() {
    BScreen *screen = Fluxbox::instance()->keyScreen();
    if (screen != 0) {
        FocusControl::Focusables win_list(screen->focusControl().creationOrderList().clientList());

        FocusControl::Focusables::iterator it = win_list.begin(),
                                           it_end = win_list.end();
        // save old value, so we can restore it later
        WinClient *old = WindowCmd<void>::client();
        for (; it != it_end; ++it) {
            WinClient *client = dynamic_cast<WinClient *>(*it);
            if (!client) continue;
            WindowCmd<void>::setClient(client);
            if (!m_cmd->execute()) {
                WindowCmd<void>::setClient(old);
                return false;
            }
        }
        WindowCmd<void>::setClient(old);
    }
    return true;
}

namespace {

FbTk::Command<void> *parseWindowList(const string &command,
                               const string &args, bool trusted) {
    int opts;
    string pat;
    FocusableList::parseArgs(args, opts, pat);
    if (command == "attach")
        return new AttachCmd(pat);
    else if (command == "nextwindow")
        return new NextWindowCmd(opts, pat);
    else if (command == "nextgroup") {
        opts |= FocusableList::LIST_GROUPS;
        return new NextWindowCmd(opts, pat);
    } else if (command == "prevwindow")
        return new PrevWindowCmd(opts, pat);
    else if (command == "prevgroup") {
        opts |= FocusableList::LIST_GROUPS;
        return new PrevWindowCmd(opts, pat);
    } else if ((command == "arrangewindows") 
        || (command == "tilegrid")) {
        return new TileGridCmd(pat);
    } else if ((command == "arrangewindowsvertical") 
        || (command == "tilevertical")){
        return new TileVerticalCmd(pat);
    } else if ((command == "arrangewindowshorizontal")
        || (command == "tilehorizontal")){
        return new TileHorizontalCmd(pat);
     } else if ((command == "arrangewindowsstackedleft")
        || (command == "tilestackedleft")){
        return new TileStackedLeftCmd(pat);
    } else if ((command == "arrangewindowsstackedright")
        || (command == "tilestackedright")){
        return new TileStackedRightCmd(pat);
    } else if ((command == "arrangewindowsstackedtop")
        || (command == "tilestackedtop")){
        return new TileStackedTopCmd(pat);
    } else if ((command == "arrangewindowsstackedbottom")
        || (command == "tilestackedbottom")){
        return new TileStackedBottomCmd(pat);
    }
    return 0;
}

REGISTER_COMMAND_PARSER(attach, parseWindowList, void);
REGISTER_COMMAND_PARSER(nextwindow, parseWindowList, void);
REGISTER_COMMAND_PARSER(nextgroup, parseWindowList, void);
REGISTER_COMMAND_PARSER(prevwindow, parseWindowList, void);
REGISTER_COMMAND_PARSER(prevgroup, parseWindowList, void);
REGISTER_COMMAND_PARSER(tilegrid, parseWindowList, void);
REGISTER_COMMAND_PARSER(tilevertical, parseWindowList, void);
REGISTER_COMMAND_PARSER(tilehorizontal, parseWindowList, void);
REGISTER_COMMAND_PARSER(tilestackedleft, parseWindowList, void);
REGISTER_COMMAND_PARSER(tilestackedright, parseWindowList, void);
REGISTER_COMMAND_PARSER(tilestackedtop, parseWindowList, void);
REGISTER_COMMAND_PARSER(tilestackedbottom, parseWindowList, void);

} // end anonymous namespace

void AttachCmd::execute() {
    BScreen *screen = Fluxbox::instance()->keyScreen();
    if (screen != 0) {
        FocusControl::Focusables win_list(screen->focusControl().focusedOrderWinList().clientList());

        FocusControl::Focusables::iterator it = win_list.begin(),
                                           it_end = win_list.end();
        FluxboxWindow *first = 0;
        for (; it != it_end; ++it) {
            if (m_pat.match(**it) && (*it)->fbwindow()) {
                if (first == 0)
                    first = (*it)->fbwindow();
                else
                    first->attachClient((*it)->fbwindow()->winClient());
            }
        }

    }
}

void NextWindowCmd::execute() {
    BScreen *screen = Fluxbox::instance()->keyScreen();
    if (screen != 0)
        screen->cycleFocus(m_option, &m_pat, false);
}

void PrevWindowCmd::execute() {
    BScreen *screen = Fluxbox::instance()->keyScreen();
    if (screen != 0)
        screen->cycleFocus(m_option, &m_pat, true);
}

FbTk::Command<void> *GoToWindowCmd::parse(const string &command,
                                    const string &arguments, bool trusted) {
    int num, opts;
    string args, pat;
    FbTk_istringstream iss(arguments.c_str());
    iss >> num;
    string::size_type pos = arguments.find_first_of("({");
    if (pos != string::npos && pos != arguments.size())
        args = arguments.c_str() + pos;
    FocusableList::parseArgs(args, opts, pat);
    return new GoToWindowCmd(num, opts, pat);
}

REGISTER_COMMAND_PARSER(gotowindow, GoToWindowCmd::parse, void);

void GoToWindowCmd::execute() {
    BScreen *screen = Fluxbox::instance()->keyScreen();
    if (screen != 0) {
        const FocusableList *win_list =
            FocusableList::getListFromOptions(*screen, m_option);
        screen->focusControl().goToWindowNumber(*win_list, m_num, &m_pat);
    }
}

FbTk::Command<void> *DirFocusCmd::parse(const string &command,
                                  const string &args, bool trusted) {
    if (command == "focusup")
        return new DirFocusCmd(FocusControl::FOCUSUP);
    else if (command == "focusdown")
        return new DirFocusCmd(FocusControl::FOCUSDOWN);
    else if (command == "focusleft")
        return new DirFocusCmd(FocusControl::FOCUSLEFT);
    else if (command == "focusright")
        return new DirFocusCmd(FocusControl::FOCUSRIGHT);
    return 0;
}

REGISTER_COMMAND_PARSER(focusup, DirFocusCmd::parse, void);
REGISTER_COMMAND_PARSER(focusdown, DirFocusCmd::parse, void);
REGISTER_COMMAND_PARSER(focusleft, DirFocusCmd::parse, void);
REGISTER_COMMAND_PARSER(focusright, DirFocusCmd::parse, void);

void DirFocusCmd::execute() {
    BScreen *screen = Fluxbox::instance()->keyScreen();
    if (screen == 0)
        return;

    FluxboxWindow *win = FocusControl::focusedFbWindow();
    if (win)
        screen->focusControl().dirFocus(*win, m_dir);
}

REGISTER_COMMAND(addworkspace, AddWorkspaceCmd, void);

void AddWorkspaceCmd::execute() {
    BScreen *screen = Fluxbox::instance()->mouseScreen();
    if (screen != 0)
        screen->addWorkspace();
}

REGISTER_COMMAND(removelastworkspace, RemoveLastWorkspaceCmd, void);

void RemoveLastWorkspaceCmd::execute() {
    BScreen *screen = Fluxbox::instance()->mouseScreen();
    if (screen != 0)
        screen->removeLastWorkspace();
}

namespace {

FbTk::Command<void> *parseIntCmd(const string &command, const string &args,
                           bool trusted) {
    int num = 1;
    FbTk_istringstream iss(args.c_str());
    iss >> num;
    if (command == "nextworkspace")
        return new NextWorkspaceCmd(num);
    else if (command == "prevworkspace")
        return new PrevWorkspaceCmd(num);
    else if (command == "rightworkspace")
        return new RightWorkspaceCmd(num);
    else if (command == "leftworkspace")
        return new LeftWorkspaceCmd(num);
    else if (command == "workspace")
        // workspaces appear 1-indexed to the user, hence the minus 1
        return new JumpToWorkspaceCmd(num - 1);
    return 0;
}

REGISTER_COMMAND_PARSER(nextworkspace, parseIntCmd, void);
REGISTER_COMMAND_PARSER(prevworkspace, parseIntCmd, void);
REGISTER_COMMAND_PARSER(rightworkspace, parseIntCmd, void);
REGISTER_COMMAND_PARSER(leftworkspace, parseIntCmd, void);
REGISTER_COMMAND_PARSER(workspace, parseIntCmd, void);

} // end anonymous namespace

void NextWorkspaceCmd::execute() {
    BScreen *screen = Fluxbox::instance()->mouseScreen();
    if (screen != 0)
        screen->nextWorkspace(m_option == 0 ? 1 : m_option);
}

void PrevWorkspaceCmd::execute() {
    BScreen *screen = Fluxbox::instance()->mouseScreen();
    if (screen != 0)
        screen->prevWorkspace(m_option == 0 ? 1 : m_option);
}

void LeftWorkspaceCmd::execute() {
    BScreen *screen = Fluxbox::instance()->mouseScreen();
    if (screen != 0)
        screen->leftWorkspace(m_param);
}

void RightWorkspaceCmd::execute() {
    BScreen *screen = Fluxbox::instance()->mouseScreen();
    if (screen != 0)
        screen->rightWorkspace(m_param);
}

JumpToWorkspaceCmd::JumpToWorkspaceCmd(int workspace_num):m_workspace_num(workspace_num) { }

void JumpToWorkspaceCmd::execute() {
    BScreen *screen = Fluxbox::instance()->mouseScreen();
    if (screen != 0) {
        int num = screen->numberOfWorkspaces();
        int actual = m_workspace_num;
        // we need an extra +1, since it's subtracted in FbCommandFactory
        if (actual < 0) actual += num+1;
        if (actual < 0) actual = 0;
        if (actual >= num) actual = num - 1;
        screen->changeWorkspaceID(actual);
    }
}

REGISTER_COMMAND(showdesktop, ShowDesktopCmd, void);

void ShowDesktopCmd::execute() {
    BScreen *screen = Fluxbox::instance()->mouseScreen();
    if (screen == 0)
        return;

    // iconify windows in focus order, so it gets restored properly
    const std::list<Focusable *> wins =
            screen->focusControl().focusedOrderWinList().clientList();
    std::list<Focusable *>::const_iterator it = wins.begin(),
                                           it_end = wins.end();
    unsigned int space = screen->currentWorkspaceID();
    unsigned int count = 0;
    for (; it != it_end; ++it) {
        if (!(*it)->fbwindow()->isIconic() && ((*it)->fbwindow()->isStuck() ||
            (*it)->fbwindow()->workspaceNumber() == space) &&
            (*it)->fbwindow()->layerNum() < ResourceLayer::DESKTOP) {
            (*it)->fbwindow()->iconify();
            count++;
        }
    }

    if (count == 0) {
        BScreen::Icons icon_list = screen->iconList();
        BScreen::Icons::reverse_iterator iconit = icon_list.rbegin();
        BScreen::Icons::reverse_iterator itend= icon_list.rend();
        for(; iconit != itend; ++iconit) {
            if ((*iconit)->workspaceNumber() == space || (*iconit)->isStuck())
                (*iconit)->deiconify(false);
        }
    } else
        FocusControl::revertFocus(*screen);

}

REGISTER_COMMAND(closeallwindows, CloseAllWindowsCmd, void);

void CloseAllWindowsCmd::execute() {
    BScreen *screen = Fluxbox::instance()->mouseScreen();
    if (screen == 0)
        return;

    Workspace::Windows windows;

    BScreen::Workspaces::iterator workspace_it = screen->getWorkspacesList().begin();
    BScreen::Workspaces::iterator workspace_it_end = screen->getWorkspacesList().end();
    for (; workspace_it != workspace_it_end; ++workspace_it) {
        windows = (*workspace_it)->windowList();
        std::for_each(windows.begin(), windows.end(),
                std::mem_fun(&FluxboxWindow::close));
    }

    windows = screen->iconList();
    std::for_each(windows.begin(),
            windows.end(), std::mem_fun(&FluxboxWindow::close));

}
