# MeshSync
A simple mobile ad hoc network designed to synchronize EL wire or LEDs on Burning Man costumes

I created this project back in 2012 with the goal of building a low cost device (around USD $10) that would plug in between a user's EL wire inverter and the EL wire on their costume or bike and automatically synchronize flashing patterns with other users around them.

It worked pretty well, but it turned out to be nearly impossible to get enough people together at the same time to do any of the more elaborate patterns.  Around 40 units were built.

Also included is a visual simulator written in VB to test out various aspects of the protocol.  It supports automatic partitioning and merging; if a group splits or multiple groups come together, they will automatically reorganize.  Patterns are chosen that are appropriate for the number of group members.

It's a mesh network and in the pathological case of nodes strung out so that each can only hear one node to either side, it'll still converge within a few network cycles.

Very little data is carried over the network; it's just timing information, group membership, and the currently running pattern.

The lowest hardware ID chooses the pattern.  Hardware ID 0 is reserved for a manual control device with a selector button.  Absent a manual control, patterns are chosen randomly.