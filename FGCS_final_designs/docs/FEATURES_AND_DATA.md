# Features, directions and data scope

The HLS top consumes three node channels and four edge channels. Preserved upstream
graph-construction code defines normalized cylindrical hit coordinates (r,phi,z)
and edge channels (delta-r, wrapped delta-phi, delta-z, DeltaR), with
DeltaR = sqrt((delta-eta)^2+(delta-phi)^2) and eta calculated from hit position r,z.
Delta-phi is in radians in the DeltaR formula. The separate radial/azimuthal/z edge
channels are normalized. Native M graph0 and L graph0 numerical checks match this
structure within 5.7e-6. The original exact producer/config and raw hit mapping
have not been recovered, so the full normalization/selection provenance is not claimed.

The 40 archived M/L graph mappings contain no reciprocal pair among valid edges:
M has 1,448,521 valid directed entries, L has 22,256,637. Each candidate connection
appears in one direction only. HLS aggregates incoming messages into destination
nodes; it does not add reverse edges. The upstream PyG dataset loader can add
reverse edges, but that transformation is not present in these hardware inputs.
In this archive Ri=source and Ro=destination, verified by full endpoint matching.

The graph source is TrackML-derived. The original thesis records pT_min=0.5 GeV
for M and 0 GeV for L; the 20 M samples are sections from 11 events and the 20 L
samples are 20 distinct events. This package contains only one M sample and
one L sample. Configured capacities include padding and are not physical hit counts.
The thesis's z-section terminology and original eta-section directory naming differ;
exact boundaries/config remain unresolved. Labels/PIDs are not a substitute for
an independently recovered raw TrackML truth-to-hit mapping. No held-out train/test
split or full graph-generation provenance is asserted by this package.
