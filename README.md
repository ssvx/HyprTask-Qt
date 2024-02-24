# HyprTask-Qt • a Hyprland Client Selector

[Hyprland](https://hyprland.org/) • [Wiki](https://wiki.hyprland.org/) • [GitHub](https://github.com/hyprwm/Hyprland)

HyprTask-Qt interfaces with `hyprctl` to display and select window clients dynamically. It uses the JSON output to create a graphical user interface with a list of running Hyperland clients (windows/tiles).

It is primarily meant to extend the capabilities of Hyprland to function as floating window manager (i'm not yet rice enough for all the tiling... or my display is just too small).

## What it does

- Call `hyprctl clients -j` for the JSON client list
- Sort clients by focusHistoryID
- Open a QtWidget with a simple list of Hyprland clients
- Navigate through clients on a loop (next on last = go to first | back on first = go to last)
- Switch focus to the selected client on confirmation and exit
- Registers a QLocalServer to manage singleton stuff like
- - limit instance to 1
- - pass arguments of successive instances along to main instance
- - handle successive instance's arguments in singleton widget

## Command line arguments

- next: initially select the next client in the list
- back: initially select the previous client in the list
- verbose: output my crappy debug messages that are neither complete nor helpful to anyone

## Key binds

- Tab | Arrow Up: cycle next client in list
- Shift+Tab | Arrow Down: cycle previous client in list
- Alt | Enter | Space: confirm (switch to selected client) and exit
- Esc: return focus to previous clients and exit

Also: confirm&exit on release of Alt key. Small delay which may or may not be accidental to keep the widget open if Alt is released "fast fast" (💌 to [Nick Lucid](https://www.youtube.com/user/TheScienceAsylum)).

Key binds are hard coded at the moment 🤮 but at least i *got* key binds.
For now just change them inside the eventFilter() and recompile.

## Dependencies

- qt6 (~~maybe qt5 works too~~ it doesn't)
- qmake6

## Compiling / Installation

- git clone https://github.com/ssvx/HyprTask-Qt.git
- cd HyprTask-Qt
- qmake6
- make

Now place ./HyprTask wherever you like (mine sits at ~/F3/HyprTask cause it reminds me of pressing RUN on a C64 in like 1988... don't ask) and create a bind on Alt+Tab to it. Also make sure it's executable `chmod u+x HyprTask`.

Here's my ~/.config/hypr/hyperland.conf snippet regarding HyprTask:
```
# HyprTask
windowrule=float,title:HyprTask
windowrulev2 = size 600 300,title:HyprTask
windowrulev2 = move 100%-700 100%-400,title:HyprTask
bind = Alt, Tab, exec, ~/F3/HyprTask next
bind = Alt Shift, Tab, exec, ~/F3/HyprTask back
```

This places the widget in the bottom right corner of the screen, 100px away from bottom and right. It also binds Alt+Tab to open the widget with the cursor on the next client and Alt+Shift+Tab on the previous client.

## Thoughts

Using the argument `next` enables the use of the key sequence `Alt+Tab ... Alt` to cycle back and forth between the two most recent clients. That's basically the same as `hyprctl dispatch cyclenext prev;hyprctl dispatch bringactivetotop` but with the benefit of being able to cycle back and forth more than one client, whereas calling `hyprctl dispatch cyclenext prev` immediately changes the history, making successive calls rather pointless (at least when using floating windows).

The above is the main reason i went for a gui approach instead of a simple shell script because this defines a clear start and end of the clients navigation input sequence, enabling me to go up, down and up again, just to ultimately cancel by pressing ESCAPE!

Please pardon me for probable spaghetti code and awful style while i re-learn to code in cpp. Last time i did that was like 20 yrs ago.

## Todo

- ~~Make it a single instance application (simpleton?) that takes successive calls' arguments to navigate the clients list while suppressing multiple instances.~~
- ~~Include an option to *confirm and close* on *release of the Alt key* so Alt+Tab+Tab+Tab or Alt+Tab+Tab+Shift+Tab become feasible.~~
- Add command line argument like *sticky* to disable automatic close on Alt release.
