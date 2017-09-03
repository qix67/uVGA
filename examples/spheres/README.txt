Requirement: 240MHz only

This demo is in realtime, no flip book trick due to lack of memory.

Unfortunately, wiping the whole screen at each frame requires a lot of time.
After approximately 60 spheres, some of them will not be displayed because they
are drawn after their pixels are sent to the monitor.

After ~635 spheres, the whole drawing becomes too slow to be perform in 1 frame.

