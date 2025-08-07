// You can use the entire C/C++ standard library, just add the relevant
// #includes. We recommend math.h ;-)

#include "usercode.h"

/*
 * This is your bot's startup function. Here you can set your snake's colors,
 * set up persistent variables, etc.
 */
bool init(Api *api)
{
    // remove the default color
    api->clearColors();

    // I'm green!
    api->addColor(40, 255, 0);
    api->addColor(20, 128, 0);
    api->addColor(10,  64, 0);
    api->addColor(20, 128, 0);

    // indicate successful startup. If anything goes wrong,
    // return false and we'll clean you up.
    return true;
}

/*
 * This function will be called by the framework on every step. Here you decide
 * where to move next!
 *
 * Use the provided Api object to interact with the world and make sure you set
 * the following outputs:
 *
 * - api->angle: Set your relative movement angle
 * - api->boost: Set this to true to move faster, but you will loose mass.
 *
 * The Api object also provides information about the world around you. See the
 * documentation for more details.
 */
bool step(Api *api)
{
    // let's start by moving in a large circle. Please note that all angles are
    // represented in radians, where -π to +π is a full circle.
    api->angle = 0.001;

    // check for other snakes
    for(size_t i = 0; i < api->getSegmentCount(); i++) {
        const IpcSegmentInfo &seg = api->getSegments()[i];

        if(!seg.is_self && seg.dist < 20) {
            // you can send log messages to your browser or any other viewer with the
            // appropriate Viewer Key.
            api->log("Oh no, I'm going to die!");
            break;
        }
    }

    // finding food is quite similar

    // Signal that everything is ok. Return false here if anything goes wrong but
    // you want to shut down cleanly.
    return true;
}
