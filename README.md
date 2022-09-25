Given the limited number of frames available in physical 
memory compared to the virtual memory of a process,
there must be a system that takes into account the
likelihood that any given page will be used in the future
given its past usage. This is balanced by the overhead of
such a system, ideally it would effectively predict the
unused pages while not affecting runtime.

The algorithms implemented here are first in first out,
least recently used and second chance.

* ```first in first out```: the victim frame is simply the
oldest frame still in physical memory. Requires a timestamp
of some sort to maintain.

* ```least recently used```: the victim frame is the least 
recently referenced frame. Tends to be a good predictor but
is expensive to implement.

* ```second chance```: every page has a bit associated with
it that is set when it is referenced. When a page fault occurs,
going down the frames, the reference bit is set to 0 if it is 1,
and the victim frame is chosen if the bit is 0. 
