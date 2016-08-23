dm: deathmatch
==============

dm is a tiny unmaintainable telnet game written in about 16KB of source.

Introduction
------------

I was inspired by the [16K MUD competition](http://www.andreasen.org/16k.shtml)
to implement my own tiny MUD-like, even though the original competition was long
over.

The source is full of macro madness to pack it down into the necessary space.
And my count of 16KB is based on running it through a script to remove comments
and all unnecessary whitespace. I felt that leaving the comments in there was
the only way people would bother looking at the source code.

I started in late January 2009 after a quick discussion about small MUDs on IRC
and completed the finishing touches on the game in early February.

Features
--------
 * multiplayer real-time combat system based on on rolling many d6's.
 * who and brawls commands for seeing players online.
 * basic TELNET processing including IAC DO DONT WILL WONT.
 * travel in six possible directions: north, south, east, west, up, down.
 * six basic player stats: hit, def, rec, ini, hpmax, evade.
 * loads rooms and items from a data file
 * eight items for increased attack, defense and healing.
 * five wear locations: r.hand, l.hand, head, feet, body.
 * unbalanced combat system where sniper rifle dominates the server.
 * over 20 possible commands.

Bugs
----
 * none reported.
