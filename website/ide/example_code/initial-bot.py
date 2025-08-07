#!/usr/bin/env python3

import math


# This is your bot's startup function. Here you can set your snake's colors,
# set up persistent variables, etc.
def init(api):
    # remove the default color
    api.clear_colors()

    # I'm green!
    api.add_color(40, 255, 0)
    api.add_color(20, 128, 0)
    api.add_color(10,  64, 0)
    api.add_color(20, 128, 0)
    return True


# This function will be called by the framework on every step. Here you decide
# where to move next!
#
# Use the provided Api object to interact with the world and make sure you set
# the following values as the return value:
#
# success:    True in case you want to keep running
# deltaAngle: Set your relative movement angle
# boost:      Set this to true to move faster, but you will loose mass.
#
# The Api object also provides information about the world around you. See the
# documentation for more details.
def step(api):
    delta_angle = 0.0
    boost      = False
    success    = True

    # let's start by moving in a large circle. Please note that all angles are
    # represented in radians, where -π to +π is a full circle.
    delta_angle = 0.001

    # check for other snakes
    for seg in api.segments:
        if not seg.is_self and seg.dist < 20:
            # you can send log messages to your browser or any other viewer with the
            # appropriate Viewer Key.
            api.log("Oh no, I'm going to die!")
            break

    # finding food is quite similar

    # Signal that everything is ok. Return false here if anything goes wrong but
    # you want to shut down cleanly

    return success, delta_angle, boost
